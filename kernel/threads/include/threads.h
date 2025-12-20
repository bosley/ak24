/**
 * @file threads.h
 * @brief Thread pool with lambda-based task management
 *
 * Provides a professional-grade thread pool system for managing concurrent
 * execution of lambda tasks. Supports configurable worker threads, task
 * queuing, completion callbacks, and safe lifecycle management with strict
 * state validation.
 *
 * Key features:
 * - Configurable thread pool with min/max workers
 * - Lambda-based task submission
 * - Optional completion callbacks
 * - Task state tracking and validation
 * - Safe pool shutdown and cleanup
 * - Thread-safe task enqueuing
 * - Worker thread lifecycle management
 *
 * @note All public APIs are thread-safe
 */

#ifndef AK24_THREADS_H
#define AK24_THREADS_H

#include "lambda.h"
#include <pthread.h>
#include <stddef.h>

/**
 * @def THREADS_VERSION
 * @brief Threads module version string
 */
#define THREADS_VERSION "0.0.1-dev"

/**
 * @def AK24_THREAD_POOL_DEFAULT_MAX_WORKERS
 * @brief Default maximum worker threads
 */
#define AK24_THREAD_POOL_DEFAULT_MAX_WORKERS 8

/**
 * @def AK24_THREAD_POOL_DEFAULT_QUEUE_SIZE
 * @brief Default maximum queue size (0 = unlimited)
 */
#define AK24_THREAD_POOL_DEFAULT_QUEUE_SIZE 0

/**
 * @brief Task state enumeration
 *
 * Strict state machine for task lifecycle validation.
 * Each state transition is validated to ensure correctness.
 */
typedef enum {
  AK24_TASK_STATE_PENDING = 0,   /**< Task queued, awaiting execution */
  AK24_TASK_STATE_RUNNING = 1,   /**< Task currently executing */
  AK24_TASK_STATE_COMPLETED = 2, /**< Task finished successfully */
  AK24_TASK_STATE_CANCELLED = 3, /**< Task cancelled before execution */
  AK24_TASK_STATE_FAILED = 4     /**< Task execution failed */
} ak_task_state_t;

/**
 * @brief Thread pool state enumeration
 *
 * Tracks pool lifecycle for safe shutdown and operation validation.
 */
typedef enum {
  AK24_POOL_STATE_INITIALIZING = 0,  /**< Pool being created */
  AK24_POOL_STATE_RUNNING = 1,       /**< Pool accepting tasks */
  AK24_POOL_STATE_SHUTTING_DOWN = 2, /**< Pool draining tasks */
  AK24_POOL_STATE_TERMINATED = 3     /**< Pool fully stopped */
} ak_pool_state_t;

/**
 * @brief Thread pool configuration
 *
 * Configuration structure for thread pool creation.
 */
typedef struct {
  size_t max_workers;    /**< Number of worker threads (fixed size pool) */
  size_t max_queue_size; /**< Maximum task queue size (0 = unlimited) */
} ak_thread_pool_config_t;

/**
 * @brief Task structure (opaque)
 *
 * Internal task representation. Do not access directly.
 */
typedef struct ak_task_t ak_task_t;

/**
 * @brief Thread pool structure (opaque)
 *
 * Internal pool representation. Do not access directly.
 */
typedef struct ak_thread_pool_t ak_thread_pool_t;

/**
 * @brief Create default thread pool configuration
 *
 * Initializes config with default values.
 *
 * @return Default configuration
 *
 * @threadsafe
 *
 * @par Example:
 * @code
 * ak_thread_pool_config_t config = ak_thread_pool_config_default();
 * config.max_workers = 16;
 * ak_thread_pool_t *pool = ak_thread_pool_new(&config);
 * @endcode
 */
ak_thread_pool_config_t ak_thread_pool_config_default(void);

/**
 * @brief Create a new thread pool
 *
 * Allocates and initializes a thread pool with specified configuration.
 * Worker threads are started immediately. Pass NULL for default config.
 *
 * @param config Pool configuration (NULL for defaults)
 * @return Pointer to new thread pool, or NULL on failure
 *
 * @threadsafe
 *
 * @note Caller must free with ak_thread_pool_free()
 *
 * @par Example:
 * @code
 * ak_thread_pool_config_t config = ak_thread_pool_config_default();
 * config.max_workers = 4;
 * ak_thread_pool_t *pool = ak_thread_pool_new(&config);
 * if (!pool) {
 *   fprintf(stderr, "Failed to create pool\n");
 *   return;
 * }
 * @endcode
 */
ak_thread_pool_t *ak_thread_pool_new(const ak_thread_pool_config_t *config);

/**
 * @brief Enqueue task to thread pool
 *
 * Submits a lambda task to the pool for execution. The task will be
 * executed by an available worker thread. Optional completion callback
 * is invoked after task completes (receives task state as invoke_args).
 *
 * @param pool Thread pool
 * @param task_lambda Lambda to execute
 * @param completion_callback Optional completion callback (can be NULL)
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 *
 * @note task_lambda ownership is transferred to pool
 * @note completion_callback ownership is transferred to pool
 *
 * @par Example:
 * @code
 * void my_task(void *ctx, void *args) {
 *   printf("Task executing\n");
 * }
 *
 * void on_complete(void *ctx, void *args) {
 *   ak_task_state_t *state = (ak_task_state_t *)args;
 *   printf("Task completed with state: %d\n", *state);
 * }
 *
 * ak_lambda_t *task = ak_lambda_new(my_task, NULL, NULL);
 * ak_lambda_t *cb = ak_lambda_new(on_complete, NULL, NULL);
 * ak_thread_pool_enqueue(pool, task, cb);
 * @endcode
 */
int ak_thread_pool_enqueue(ak_thread_pool_t *pool, ak_lambda_t *task_lambda,
                           ak_lambda_t *completion_callback);

/**
 * @brief Wait for all tasks to complete
 *
 * Blocks until all currently queued and running tasks finish.
 * Does not prevent new tasks from being enqueued.
 *
 * @param pool Thread pool
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 *
 * @par Example:
 * @code
 * ak_thread_pool_enqueue(pool, task1, NULL);
 * ak_thread_pool_enqueue(pool, task2, NULL);
 * ak_thread_pool_wait(pool);
 * printf("All tasks completed\n");
 * @endcode
 */
int ak_thread_pool_wait(ak_thread_pool_t *pool);

/**
 * @brief Get number of pending tasks
 *
 * Returns count of tasks in queue (not including running tasks).
 *
 * @param pool Thread pool
 * @return Number of pending tasks, or 0 on failure
 *
 * @threadsafe
 */
size_t ak_thread_pool_pending_count(ak_thread_pool_t *pool);

/**
 * @brief Get number of active worker threads
 *
 * Returns count of currently active workers.
 *
 * @param pool Thread pool
 * @return Number of active workers, or 0 on failure
 *
 * @threadsafe
 */
size_t ak_thread_pool_active_count(ak_thread_pool_t *pool);

/**
 * @brief Get thread pool state
 *
 * Returns current pool state.
 *
 * @param pool Thread pool
 * @return Current pool state
 *
 * @threadsafe
 */
ak_pool_state_t ak_thread_pool_get_state(ak_thread_pool_t *pool);

/**
 * @brief Shutdown and free thread pool
 *
 * Initiates graceful shutdown: stops accepting new tasks, waits for
 * all queued and running tasks to complete, then frees all resources.
 *
 * @param pool Thread pool to free (NULL is safe)
 *
 * @threadsafe
 *
 * @warning Pool must not be used after this call
 *
 * @par Example:
 * @code
 * ak_thread_pool_free(pool);
 * pool = NULL;
 * @endcode
 */
void ak_thread_pool_free(ak_thread_pool_t *pool);

/**
 * @brief Get task state string representation
 *
 * Returns human-readable string for task state.
 *
 * @param state Task state
 * @return State string (never NULL)
 *
 * @threadsafe
 */
const char *ak_task_state_str(ak_task_state_t state);

/**
 * @brief Get pool state string representation
 *
 * Returns human-readable string for pool state.
 *
 * @param state Pool state
 * @return State string (never NULL)
 *
 * @threadsafe
 */
const char *ak_pool_state_str(ak_pool_state_t state);

#endif
