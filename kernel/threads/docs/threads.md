# Threads

The threads module provides a thread pool system for managing concurrent execution of lambda-based tasks. It supports configurable worker threads, task queuing, completion callbacks, and safe lifecycle management with strict state validation.

## Core Concept

A thread pool is a concurrent execution environment that:
- Manages a pool of worker threads
- Queues tasks for execution
- Executes tasks using lambda functions
- Tracks task states with strict validation
- Invokes completion callbacks when tasks finish
- Safely shuts down and cleans up resources

## Task States

Tasks follow a strict state machine:

```
PENDING → RUNNING → COMPLETED
           ↓
         FAILED
```

- **PENDING**: Task is queued, awaiting execution
- **RUNNING**: Task is currently executing
- **COMPLETED**: Task finished successfully
- **CANCELLED**: Task was cancelled before execution (reserved for future use)
- **FAILED**: Task execution failed or invalid state transition occurred

## Pool States

Thread pools have their own lifecycle:

```
INITIALIZING → RUNNING → SHUTTING_DOWN → TERMINATED
```

- **INITIALIZING**: Pool is being created
- **RUNNING**: Pool is accepting and executing tasks
- **SHUTTING_DOWN**: Pool is draining remaining tasks
- **TERMINATED**: Pool is fully stopped and freed

## Basic Usage

```c
#include "kernel.h"

// Create a task function
void my_task(void *ctx, void *args) {
    int *data = (int *)ctx;
    printf("Processing data: %d\n", *data);
}

// Create a completion callback
void on_complete(void *ctx, void *args) {
    ak_task_state_t *state = (ak_task_state_t *)args;
    printf("Task completed with state: %s\n", ak_task_state_str(*state));
}

int main(int argc, char **argv) {
    ak_kernel_init();

    // Create thread pool with default config
    ak_thread_pool_t *pool = ak_thread_pool_new(NULL);
    if (!pool) {
        fprintf(stderr, "Failed to create pool\n");
        return 1;
    }

    // Create task with context
    int *data = AK24_ALLOC(sizeof(int));
    *data = 42;
    ak_lambda_t *task = ak_lambda_new(my_task, data, AK24_FREE);
    ak_lambda_t *cb = ak_lambda_new(on_complete, NULL, NULL);

    // Enqueue task
    ak_thread_pool_enqueue(pool, task, cb);

    // Wait for all tasks to complete
    ak_thread_pool_wait(pool);

    // Cleanup
    ak_thread_pool_free(pool);
    ak_kernel_deinit();

    return 0;
}
```

## Custom Configuration

```c
ak_thread_pool_config_t config = ak_thread_pool_config_default();
config.max_workers = 16;      // Maximum worker threads
config.min_workers = 4;       // Initial worker threads
config.max_queue_size = 1000; // Maximum pending tasks (0 = unlimited)

ak_thread_pool_t *pool = ak_thread_pool_new(&config);
```

## Multiple Tasks

```c
ak_thread_pool_t *pool = ak_thread_pool_new(NULL);

for (int i = 0; i < 100; i++) {
    int *data = AK24_ALLOC(sizeof(int));
    *data = i;

    ak_lambda_t *task = ak_lambda_new(my_task, data, AK24_FREE);
    ak_thread_pool_enqueue(pool, task, NULL); // No completion callback
}

// Wait for all tasks
ak_thread_pool_wait(pool);

// Check pool status
printf("Pending: %zu\n", ak_thread_pool_pending_count(pool));
printf("Active: %zu\n", ak_thread_pool_active_count(pool));

ak_thread_pool_free(pool);
```

## Task Context Management

```c
typedef struct {
    char *filename;
    int priority;
} file_job_t;

void process_file(void *ctx, void *args) {
    file_job_t *job = (file_job_t *)ctx;
    printf("Processing %s (priority %d)\n", job->filename, job->priority);
    // Process file...
}

void cleanup_job(void *ctx) {
    file_job_t *job = (file_job_t *)ctx;
    AK24_FREE(job->filename);
    AK24_FREE(job);
}

// Create job
file_job_t *job = AK24_ALLOC(sizeof(file_job_t));
job->filename = strdup("data.txt");
job->priority = 5;

ak_lambda_t *task = ak_lambda_new(process_file, job, cleanup_job);
ak_thread_pool_enqueue(pool, task, NULL);
```

## Completion Callbacks

```c
typedef struct {
    int total;
    AK_MUTEX mutex;
} counter_t;

void task_with_counter(void *ctx, void *args) {
    counter_t *counter = (counter_t *)ctx;
    // Do work...
}

void on_task_done(void *ctx, void *args) {
    counter_t *counter = (counter_t *)ctx;
    ak_task_state_t *state = (ak_task_state_t *)args;

    if (*state == AK24_TASK_STATE_COMPLETED) {
        AK_MUTEX_LOCK(&counter->mutex);
        counter->total++;
        AK_MUTEX_UNLOCK(&counter->mutex);
    }
}

counter_t *counter = AK24_ALLOC(sizeof(counter_t));
counter->total = 0;
AK_MUTEX_INIT(&counter->mutex);

for (int i = 0; i < 10; i++) {
    ak_lambda_t *task = ak_lambda_new(task_with_counter, counter, NULL);
    ak_lambda_t *cb = ak_lambda_new(on_task_done, counter, NULL);
    ak_thread_pool_enqueue(pool, task, cb);
}

ak_thread_pool_wait(pool);
printf("Completed: %d/10\n", counter->total);
```

## Thread Safety

All public APIs are thread-safe and can be called from multiple threads:
- `ak_thread_pool_new()` - Thread-safe
- `ak_thread_pool_enqueue()` - Thread-safe
- `ak_thread_pool_wait()` - Thread-safe
- `ak_thread_pool_pending_count()` - Thread-safe
- `ak_thread_pool_active_count()` - Thread-safe
- `ak_thread_pool_get_state()` - Thread-safe
- `ak_thread_pool_free()` - Thread-safe

**Important**: Lambda functions and their contexts are NOT automatically thread-safe. If your lambda modifies shared state, you must provide your own synchronization.

## State Validation

The thread pool strictly validates all state transitions:

```c
// Task state transitions are validated:
PENDING → RUNNING   ✓ Valid
RUNNING → COMPLETED ✓ Valid
RUNNING → FAILED    ✓ Valid (on error)
PENDING → COMPLETED ✗ Invalid
COMPLETED → RUNNING ✗ Invalid
```

Invalid transitions result in the task being marked as FAILED.

## Error Handling

Functions return error codes:
- `0` on success
- `-1` on failure

Check return values:

```c
if (ak_thread_pool_enqueue(pool, task, cb) != 0) {
    fprintf(stderr, "Failed to enqueue task\n");
    // Task and callback are NOT freed on failure
    ak_lambda_free(task);
    ak_lambda_free(cb);
}
```

## Shutdown and Cleanup

The thread pool performs graceful shutdown:

```c
ak_thread_pool_free(pool);
```

This will:
1. Stop accepting new tasks
2. Let queued tasks complete
3. Wait for running tasks to finish
4. Clean up all resources
5. Free the pool

**Important**: Do not use the pool after calling `ak_thread_pool_free()`.
