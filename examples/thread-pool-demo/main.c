#include "kernel.h"
#include "threads.h"
#include <stdio.h>
#include <unistd.h>

typedef struct {
  int task_id;
  int result;
} task_data_t;

void my_task(void *ctx, void *args) {
  task_data_t *data = (task_data_t *)ctx;
  (void)args;

  printf("[Task %d] Starting...\n", data->task_id);

  // Simulate work
  usleep(100000 + (data->task_id * 10000)); // 100-200ms

  data->result = data->task_id * 2;
  printf("[Task %d] Completed with result: %d\n", data->task_id, data->result);
}

void on_complete(void *ctx, void *args) {
  task_data_t *data = (task_data_t *)ctx;
  ak_task_state_t *state = (ak_task_state_t *)args;

  printf("[Callback %d] Task finished with state: %s\n", data->task_id,
         ak_task_state_str(*state));
}

void cleanup_task_data(void *ctx) {
  task_data_t *data = (task_data_t *)ctx;
  printf("[Cleanup %d] Freeing task data\n", data->task_id);
  AK24_FREE(data);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  ak_kernel_init("thread-pool-demo");

  printf("\n=== Thread Pool Demo ===\n\n");

  // Create thread pool with custom configuration
  ak_thread_pool_config_t config = ak_thread_pool_config_default();
  config.max_workers = 4;
  config.max_queue_size = 0; // Unlimited

  printf("Creating thread pool with %zu workers...\n", config.max_workers);
  ak_thread_pool_t *pool = ak_thread_pool_new(&config);

  if (!pool) {
    fprintf(stderr, "Failed to create thread pool\n");
    ak_kernel_deinit();
    return 1;
  }

  printf("Pool created successfully!\n\n");

  // Enqueue multiple tasks
  const int num_tasks = 10;
  printf("Enqueuing %d tasks...\n\n", num_tasks);

  for (int i = 0; i < num_tasks; i++) {
    // Create task data
    task_data_t *data = AK24_ALLOC(sizeof(task_data_t));
    data->task_id = i;
    data->result = 0;

    // Create task lambda
    ak_lambda_t *task = ak_lambda_new(my_task, data, cleanup_task_data);

    // Create completion callback
    task_data_t *cb_data = AK24_ALLOC(sizeof(task_data_t));
    cb_data->task_id = i;
    ak_lambda_t *cb = ak_lambda_new(on_complete, cb_data, cleanup_task_data);

    // Enqueue task
    if (ak_thread_pool_enqueue(pool, task, cb) != 0) {
      fprintf(stderr, "Failed to enqueue task %d\n", i);
      ak_lambda_free(task);
      ak_lambda_free(cb);
    }

    // Show pool status periodically
    if (i % 3 == 0) {
      printf("Pool status: %zu pending, %zu active\n",
             ak_thread_pool_pending_count(pool),
             ak_thread_pool_active_count(pool));
    }
  }

  printf("\nAll tasks enqueued. Waiting for completion...\n\n");

  // Wait for all tasks to complete
  ak_thread_pool_wait(pool);

  printf("\n=== All tasks completed! ===\n");
  printf("Final pool status: %zu pending, %zu active\n",
         ak_thread_pool_pending_count(pool), ak_thread_pool_active_count(pool));

  // Cleanup
  printf("\nShutting down thread pool...\n");
  ak_thread_pool_free(pool);

  printf("Thread pool shut down successfully!\n");

  ak_kernel_deinit();

  return 0;
}
