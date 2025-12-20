#include "threads.h"
#include "kernel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @internal
 * @brief Task node in queue
 */
typedef struct ak_task_node_t {
  ak_task_t *task;
  struct ak_task_node_t *next;
} ak_task_node_t;

/**
 * @internal
 * @brief Task structure
 */
struct ak_task_t {
  ak_lambda_t *task_lambda;
  ak_lambda_t *completion_callback;
  ak_task_state_t state;
  pthread_mutex_t state_mutex;
};

/**
 * @internal
 * @brief Thread pool structure
 */
struct ak_thread_pool_t {
  // Configuration
  size_t max_workers;
  size_t max_queue_size;

  // Worker threads
  pthread_t *workers;
  size_t worker_count;
  size_t active_count;

  // Task queue
  ak_task_node_t *queue_head;
  ak_task_node_t *queue_tail;
  size_t queue_size;

  // Synchronization
  pthread_mutex_t mutex;
  pthread_cond_t work_available;
  pthread_cond_t work_complete;

  // State
  ak_pool_state_t state;
};

/**
 * @internal
 * @brief Validate and transition task state
 */
static int task_transition_state(ak_task_t *task, ak_task_state_t from,
                                 ak_task_state_t to) {
  if (!task) {
    return -1;
  }

  pthread_mutex_lock(&task->state_mutex);

  if (task->state != from) {
    pthread_mutex_unlock(&task->state_mutex);
    return -1;
  }

  task->state = to;
  pthread_mutex_unlock(&task->state_mutex);

  return 0;
}

/**
 * @internal
 * @brief Get task state safely
 */
static ak_task_state_t task_get_state(ak_task_t *task) {
  if (!task) {
    return AK24_TASK_STATE_FAILED;
  }

  pthread_mutex_lock(&task->state_mutex);
  ak_task_state_t state = task->state;
  pthread_mutex_unlock(&task->state_mutex);

  return state;
}

/**
 * @internal
 * @brief Create a new task
 */
static ak_task_t *task_new(ak_lambda_t *task_lambda,
                           ak_lambda_t *completion_callback) {
  if (!task_lambda) {
    return NULL;
  }

  ak_task_t *task = AK24_ALLOC(sizeof(ak_task_t));
  if (!task) {
    return NULL;
  }

  task->task_lambda = task_lambda;
  task->completion_callback = completion_callback;
  task->state = AK24_TASK_STATE_PENDING;

  if (pthread_mutex_init(&task->state_mutex, NULL) != 0) {
    AK24_FREE(task);
    return NULL;
  }

  return task;
}

/**
 * @internal
 * @brief Free task
 */
static void task_free(ak_task_t *task) {
  if (!task) {
    return;
  }

  if (task->task_lambda) {
    ak_lambda_free(task->task_lambda);
  }

  if (task->completion_callback) {
    ak_lambda_free(task->completion_callback);
  }

  pthread_mutex_destroy(&task->state_mutex);
  AK24_FREE(task);
}

/**
 * @internal
 * @brief Execute task
 */
static void task_execute(ak_task_t *task) {
  if (!task) {
    return;
  }

  // Transition to RUNNING
  if (task_transition_state(task, AK24_TASK_STATE_PENDING,
                            AK24_TASK_STATE_RUNNING) != 0) {
    // Invalid state transition
    return;
  }

  // Execute the task lambda
  ak_lambda_invoke(task->task_lambda, NULL);

  // Transition to COMPLETED
  if (task_transition_state(task, AK24_TASK_STATE_RUNNING,
                            AK24_TASK_STATE_COMPLETED) != 0) {
    // Mark as failed if we couldn't transition properly
    pthread_mutex_lock(&task->state_mutex);
    task->state = AK24_TASK_STATE_FAILED;
    pthread_mutex_unlock(&task->state_mutex);
  }

  // Invoke completion callback if provided
  if (task->completion_callback) {
    ak_task_state_t final_state = task_get_state(task);
    ak_lambda_invoke(task->completion_callback, &final_state);
  }
}

/**
 * @internal
 * @brief Dequeue next task
 */
static ak_task_t *pool_dequeue_task(ak_thread_pool_t *pool) {
  if (!pool || !pool->queue_head) {
    return NULL;
  }

  ak_task_node_t *node = pool->queue_head;
  ak_task_t *task = node->task;

  pool->queue_head = node->next;
  if (!pool->queue_head) {
    pool->queue_tail = NULL;
  }

  pool->queue_size--;
  AK24_FREE(node);

  return task;
}

/**
 * @internal
 * @brief Worker thread function
 */
static void *worker_thread(void *arg) {
  ak_thread_pool_t *pool = (ak_thread_pool_t *)arg;

  while (1) {
    pthread_mutex_lock(&pool->mutex);

    // Wait for work or shutdown signal
    while (pool->state == AK24_POOL_STATE_RUNNING && pool->queue_size == 0) {
      pthread_cond_wait(&pool->work_available, &pool->mutex);
    }

    // Check if we should exit
    if (pool->state != AK24_POOL_STATE_RUNNING && pool->queue_size == 0) {
      pthread_mutex_unlock(&pool->mutex);
      break;
    }

    // Get next task
    ak_task_t *task = pool_dequeue_task(pool);
    pool->active_count++;

    pthread_mutex_unlock(&pool->mutex);

    // Execute task outside of lock
    if (task) {
      task_execute(task);
      task_free(task);
    }

    // Mark as done
    pthread_mutex_lock(&pool->mutex);
    pool->active_count--;

    // Signal if all work is complete
    if (pool->queue_size == 0 && pool->active_count == 0) {
      pthread_cond_broadcast(&pool->work_complete);
    }

    pthread_mutex_unlock(&pool->mutex);
  }

  return NULL;
}

ak_thread_pool_config_t ak_thread_pool_config_default(void) {
  ak_thread_pool_config_t config;
  config.max_workers = AK24_THREAD_POOL_DEFAULT_MAX_WORKERS;
  config.max_queue_size = AK24_THREAD_POOL_DEFAULT_QUEUE_SIZE;
  return config;
}

ak_thread_pool_t *ak_thread_pool_new(const ak_thread_pool_config_t *config) {
  ak_thread_pool_config_t default_config;
  if (!config) {
    default_config = ak_thread_pool_config_default();
    config = &default_config;
  }

  if (config->max_workers == 0) {
    return NULL;
  }

  ak_thread_pool_t *pool = AK24_ALLOC(sizeof(ak_thread_pool_t));
  if (!pool) {
    return NULL;
  }

  pool->max_workers = config->max_workers;
  pool->max_queue_size = config->max_queue_size;
  pool->worker_count = config->max_workers;
  pool->active_count = 0;
  pool->queue_head = NULL;
  pool->queue_tail = NULL;
  pool->queue_size = 0;
  pool->state = AK24_POOL_STATE_INITIALIZING;

  // Initialize synchronization primitives
  if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
    AK24_FREE(pool);
    return NULL;
  }

  if (pthread_cond_init(&pool->work_available, NULL) != 0) {
    pthread_mutex_destroy(&pool->mutex);
    AK24_FREE(pool);
    return NULL;
  }

  if (pthread_cond_init(&pool->work_complete, NULL) != 0) {
    pthread_cond_destroy(&pool->work_available);
    pthread_mutex_destroy(&pool->mutex);
    AK24_FREE(pool);
    return NULL;
  }

  // Allocate worker threads
  pool->workers = AK24_ALLOC(sizeof(pthread_t) * pool->worker_count);
  if (!pool->workers) {
    pthread_cond_destroy(&pool->work_complete);
    pthread_cond_destroy(&pool->work_available);
    pthread_mutex_destroy(&pool->mutex);
    AK24_FREE(pool);
    return NULL;
  }

  // Start worker threads
  pool->state = AK24_POOL_STATE_RUNNING;
  for (size_t i = 0; i < pool->worker_count; i++) {
    if (AK24_THREAD_CREATE(&pool->workers[i], NULL, worker_thread, pool) != 0) {
      // Cleanup on failure
      pool->state = AK24_POOL_STATE_SHUTTING_DOWN;
      pthread_cond_broadcast(&pool->work_available);

      for (size_t j = 0; j < i; j++) {
        AK24_THREAD_JOIN(pool->workers[j], NULL);
      }

      AK24_FREE(pool->workers);
      pthread_cond_destroy(&pool->work_complete);
      pthread_cond_destroy(&pool->work_available);
      pthread_mutex_destroy(&pool->mutex);
      AK24_FREE(pool);
      return NULL;
    }
  }

  return pool;
}

int ak_thread_pool_enqueue(ak_thread_pool_t *pool, ak_lambda_t *task_lambda,
                           ak_lambda_t *completion_callback) {
  if (!pool || !task_lambda) {
    return -1;
  }

  pthread_mutex_lock(&pool->mutex);

  if (pool->state != AK24_POOL_STATE_RUNNING) {
    pthread_mutex_unlock(&pool->mutex);
    return -1;
  }

  // Check queue size limit
  if (pool->max_queue_size > 0 && pool->queue_size >= pool->max_queue_size) {
    pthread_mutex_unlock(&pool->mutex);
    return -1;
  }

  // Create task
  ak_task_t *task = task_new(task_lambda, completion_callback);
  if (!task) {
    pthread_mutex_unlock(&pool->mutex);
    return -1;
  }

  // Create queue node
  ak_task_node_t *node = AK24_ALLOC(sizeof(ak_task_node_t));
  if (!node) {
    task_free(task);
    pthread_mutex_unlock(&pool->mutex);
    return -1;
  }

  node->task = task;
  node->next = NULL;

  // Add to queue
  if (pool->queue_tail) {
    pool->queue_tail->next = node;
  } else {
    pool->queue_head = node;
  }
  pool->queue_tail = node;
  pool->queue_size++;

  // Signal worker threads
  pthread_cond_signal(&pool->work_available);
  pthread_mutex_unlock(&pool->mutex);

  return 0;
}

int ak_thread_pool_wait(ak_thread_pool_t *pool) {
  if (!pool) {
    return -1;
  }

  pthread_mutex_lock(&pool->mutex);

  while (pool->queue_size > 0 || pool->active_count > 0) {
    pthread_cond_wait(&pool->work_complete, &pool->mutex);
  }

  pthread_mutex_unlock(&pool->mutex);

  return 0;
}

size_t ak_thread_pool_pending_count(ak_thread_pool_t *pool) {
  if (!pool) {
    return 0;
  }

  pthread_mutex_lock(&pool->mutex);
  size_t count = pool->queue_size;
  pthread_mutex_unlock(&pool->mutex);

  return count;
}

size_t ak_thread_pool_active_count(ak_thread_pool_t *pool) {
  if (!pool) {
    return 0;
  }

  pthread_mutex_lock(&pool->mutex);
  size_t count = pool->active_count;
  pthread_mutex_unlock(&pool->mutex);

  return count;
}

ak_pool_state_t ak_thread_pool_get_state(ak_thread_pool_t *pool) {
  if (!pool) {
    return AK24_POOL_STATE_TERMINATED;
  }

  pthread_mutex_lock(&pool->mutex);
  ak_pool_state_t state = pool->state;
  pthread_mutex_unlock(&pool->mutex);

  return state;
}

void ak_thread_pool_free(ak_thread_pool_t *pool) {
  if (!pool) {
    return;
  }

  // Signal shutdown
  pthread_mutex_lock(&pool->mutex);
  pool->state = AK24_POOL_STATE_SHUTTING_DOWN;
  pthread_cond_broadcast(&pool->work_available);
  pthread_mutex_unlock(&pool->mutex);

  // Wait for all workers to finish
  for (size_t i = 0; i < pool->worker_count; i++) {
    AK24_THREAD_JOIN(pool->workers[i], NULL);
  }

  // Mark as terminated
  pthread_mutex_lock(&pool->mutex);
  pool->state = AK24_POOL_STATE_TERMINATED;
  pthread_mutex_unlock(&pool->mutex);

  // Clean up remaining tasks in queue
  while (pool->queue_head) {
    ak_task_t *task = pool_dequeue_task(pool);
    if (task) {
      task_free(task);
    }
  }

  // Free resources
  AK24_FREE(pool->workers);
  pthread_cond_destroy(&pool->work_complete);
  pthread_cond_destroy(&pool->work_available);
  pthread_mutex_destroy(&pool->mutex);
  AK24_FREE(pool);
}

const char *ak_task_state_str(ak_task_state_t state) {
  switch (state) {
  case AK24_TASK_STATE_PENDING:
    return "PENDING";
  case AK24_TASK_STATE_RUNNING:
    return "RUNNING";
  case AK24_TASK_STATE_COMPLETED:
    return "COMPLETED";
  case AK24_TASK_STATE_CANCELLED:
    return "CANCELLED";
  case AK24_TASK_STATE_FAILED:
    return "FAILED";
  default:
    return "UNKNOWN";
  }
}

const char *ak_pool_state_str(ak_pool_state_t state) {
  switch (state) {
  case AK24_POOL_STATE_INITIALIZING:
    return "INITIALIZING";
  case AK24_POOL_STATE_RUNNING:
    return "RUNNING";
  case AK24_POOL_STATE_SHUTTING_DOWN:
    return "SHUTTING_DOWN";
  case AK24_POOL_STATE_TERMINATED:
    return "TERMINATED";
  default:
    return "UNKNOWN";
  }
}
