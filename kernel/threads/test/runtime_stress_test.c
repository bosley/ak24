/**
 * @file runtime_stress_test.c
 * @brief Comprehensive stress tests for threads module
 *
 * Tests thread pool under heavy load including high concurrency,
 * resource pressure, rapid pool cycling, mixed workloads, and
 * data validation under stress conditions.
 */

#include "../../test/assert.h"
#include "../include/threads.h"
#include "kernel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Configuration constants
#define STRESS_NUM_TASKS 1000
#define STRESS_NUM_WORKERS 8
#define STRESS_CONCURRENT_ENQUEUERS 4
#define STRESS_POOL_CYCLES 50
#define STRESS_MIXED_WORKLOAD_TASKS 500
#define STRESS_RAPID_POOLS 100
#define STRESS_LARGE_QUEUE 10000
#define STRESS_COMPUTE_TASKS 1000
#define STRESS_CHECKSUM_TASKS 500

// Shared test data structures
typedef struct {
  size_t total_executed;
  size_t total_completed;
  size_t total_failed;
  AK24_MUTEX mutex;
} shared_counter_t;

typedef struct {
  int task_id;
  int expected_value;
  int *result_array;
  AK24_MUTEX *result_mutex;
} task_data_t;

typedef struct {
  ak_thread_pool_t *pool;
  shared_counter_t *counter;
  int num_tasks;
  int enqueuer_id;
} enqueuer_args_t;

// Helper: Initialize shared counter
static shared_counter_t *counter_new(void) {
  shared_counter_t *counter = AK24_ALLOC(sizeof(shared_counter_t));
  if (!counter) {
    return NULL;
  }

  counter->total_executed = 0;
  counter->total_completed = 0;
  counter->total_failed = 0;

  if (AK24_MUTEX_INIT(&counter->mutex) != 0) {
    AK24_FREE(counter);
    return NULL;
  }

  return counter;
}

// Helper: Free shared counter
static void counter_free(shared_counter_t *counter) {
  if (!counter) {
    return;
  }
  AK24_MUTEX_DESTROY(&counter->mutex);
  AK24_FREE(counter);
}

// Helper: Increment counter safely
static void counter_increment_executed(shared_counter_t *counter) {
  AK24_MUTEX_LOCK(&counter->mutex);
  counter->total_executed++;
  AK24_MUTEX_UNLOCK(&counter->mutex);
}

// Helper: Get counter values safely
static void counter_get_values(shared_counter_t *counter, size_t *executed,
                               size_t *completed, size_t *failed) {
  AK24_MUTEX_LOCK(&counter->mutex);
  *executed = counter->total_executed;
  *completed = counter->total_completed;
  *failed = counter->total_failed;
  AK24_MUTEX_UNLOCK(&counter->mutex);
}

// Simple incrementing task
static void simple_increment_task(void *ctx, void *args) {
  shared_counter_t *counter = (shared_counter_t *)ctx;
  (void)args;

  if (counter) {
    counter_increment_executed(counter);
  }
}

// Cleanup function for dynamically allocated context
static void free_task_context(void *ctx) {
  if (ctx) {
    AK24_FREE(ctx);
  }
}

// Task that validates data and stores result
static void data_validation_task(void *ctx, void *args) {
  task_data_t *data = (task_data_t *)ctx;
  (void)args;

  if (data && data->result_array && data->result_mutex) {
    // Simulate some work
    int computed = data->expected_value * 2;

    // Store result
    AK24_MUTEX_LOCK(data->result_mutex);
    data->result_array[data->task_id] = computed;
    AK24_MUTEX_UNLOCK(data->result_mutex);
  }
}

// Task with variable workload
static void variable_workload_task(void *ctx, void *args) {
  int *workload_us = (int *)ctx;
  (void)args;

  if (workload_us && *workload_us > 0) {
    usleep(*workload_us);
  }
}

// Completion callback for stress tests
static void stress_completion_callback(void *ctx, void *args) {
  shared_counter_t *counter = (shared_counter_t *)ctx;
  ak_task_state_t *state = (ak_task_state_t *)args;

  if (!counter || !state) {
    return;
  }

  AK24_MUTEX_LOCK(&counter->mutex);
  if (*state == AK24_TASK_STATE_COMPLETED) {
    counter->total_completed++;
  } else if (*state == AK24_TASK_STATE_FAILED) {
    counter->total_failed++;
  }
  AK24_MUTEX_UNLOCK(&counter->mutex);
}

// Test: High volume of simple tasks
static int test_high_volume_tasks(void) {
  ak_thread_pool_config_t config = ak_thread_pool_config_default();
  config.max_workers = STRESS_NUM_WORKERS;

  ak_thread_pool_t *pool = ak_thread_pool_new(&config);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  shared_counter_t *counter = counter_new();
  AK24_TEST_ASSERT_NOT_NULL(counter);

  // Enqueue many tasks
  for (int i = 0; i < STRESS_NUM_TASKS; i++) {
    ak_lambda_t *task = ak_lambda_new(simple_increment_task, counter, NULL);
    AK24_TEST_ASSERT_NOT_NULL(task);

    int result = ak_thread_pool_enqueue(pool, task, NULL);
    AK24_TEST_ASSERT_EQ(result, 0);
  }

  // Wait for completion
  ak_thread_pool_wait(pool);

  // Validate all tasks executed
  size_t executed, completed, failed;
  counter_get_values(counter, &executed, &completed, &failed);
  AK24_TEST_ASSERT_EQ(executed, STRESS_NUM_TASKS);

  counter_free(counter);
  ak_thread_pool_free(pool);

  AK24_TEST_PASS();
}

// Test: Concurrent enqueuers from multiple threads
static void *enqueuer_thread(void *arg) {
  enqueuer_args_t *args = (enqueuer_args_t *)arg;

  for (int i = 0; i < args->num_tasks; i++) {
    ak_lambda_t *task =
        ak_lambda_new(simple_increment_task, args->counter, NULL);
    if (!task) {
      continue;
    }

    ak_thread_pool_enqueue(args->pool, task, NULL);
  }

  return NULL;
}

static int test_concurrent_enqueuers(void) {
  ak_thread_pool_config_t config = ak_thread_pool_config_default();
  config.max_workers = STRESS_NUM_WORKERS;

  ak_thread_pool_t *pool = ak_thread_pool_new(&config);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  shared_counter_t *counter = counter_new();
  AK24_TEST_ASSERT_NOT_NULL(counter);

  // Create multiple enqueuer threads
  AK24_THREAD enqueuers[STRESS_CONCURRENT_ENQUEUERS];
  enqueuer_args_t args[STRESS_CONCURRENT_ENQUEUERS];

  int tasks_per_enqueuer = STRESS_NUM_TASKS / STRESS_CONCURRENT_ENQUEUERS;

  for (int i = 0; i < STRESS_CONCURRENT_ENQUEUERS; i++) {
    args[i].pool = pool;
    args[i].counter = counter;
    args[i].num_tasks = tasks_per_enqueuer;
    args[i].enqueuer_id = i;

    int result = AK24_THREAD_CREATE(&enqueuers[i], enqueuer_thread, &args[i]);
    AK24_TEST_ASSERT_EQ(result, 0);
  }

  // Wait for all enqueuers
  for (int i = 0; i < STRESS_CONCURRENT_ENQUEUERS; i++) {
    AK24_THREAD_JOIN(enqueuers[i]);
  }

  // Wait for all tasks to complete
  ak_thread_pool_wait(pool);

  // Validate results
  size_t executed, completed, failed;
  counter_get_values(counter, &executed, &completed, &failed);

  size_t expected_total =
      (size_t)(tasks_per_enqueuer * STRESS_CONCURRENT_ENQUEUERS);
  AK24_TEST_ASSERT_EQ(executed, expected_total);

  counter_free(counter);
  ak_thread_pool_free(pool);

  AK24_TEST_PASS();
}

// Test: Data validation under concurrent execution
static int test_data_validation_stress(void) {
  ak_thread_pool_t *pool = ak_thread_pool_new(NULL);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  const int num_tasks = 500;
  int *results = AK24_ALLOC(sizeof(int) * num_tasks);
  AK24_TEST_ASSERT_NOT_NULL(results);

  AK24_MUTEX result_mutex;
  AK24_MUTEX_INIT(&result_mutex);

  // Initialize results to sentinel value
  for (int i = 0; i < num_tasks; i++) {
    results[i] = -1;
  }

  // Create tasks with unique data
  for (int i = 0; i < num_tasks; i++) {
    task_data_t *data = AK24_ALLOC(sizeof(task_data_t));
    AK24_TEST_ASSERT_NOT_NULL(data);

    data->task_id = i;
    data->expected_value = i;
    data->result_array = results;
    data->result_mutex = &result_mutex;

    ak_lambda_t *task =
        ak_lambda_new(data_validation_task, data, free_task_context);
    ak_thread_pool_enqueue(pool, task, NULL);
  }

  ak_thread_pool_wait(pool);

  // Validate all results are correct
  for (int i = 0; i < num_tasks; i++) {
    int expected = i * 2;
    AK24_TEST_ASSERT_EQ(results[i], expected);
  }

  AK24_MUTEX_DESTROY(&result_mutex);
  AK24_FREE(results);
  ak_thread_pool_free(pool);

  AK24_TEST_PASS();
}

// Test: Mixed workload (fast and slow tasks)
static int test_mixed_workload(void) {
  ak_thread_pool_config_t config = ak_thread_pool_config_default();
  config.max_workers = STRESS_NUM_WORKERS;

  ak_thread_pool_t *pool = ak_thread_pool_new(&config);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  shared_counter_t *counter = counter_new();
  AK24_TEST_ASSERT_NOT_NULL(counter);

  // Mix of fast and slow tasks
  for (int i = 0; i < STRESS_MIXED_WORKLOAD_TASKS; i++) {
    int *workload = AK24_ALLOC(sizeof(int));
    *workload = (i % 10 == 0) ? 5000 : 100; // 10% slow, 90% fast

    ak_lambda_t *task =
        ak_lambda_new(variable_workload_task, workload, free_task_context);
    ak_lambda_t *cb = ak_lambda_new(stress_completion_callback, counter, NULL);

    ak_thread_pool_enqueue(pool, task, cb);
  }

  ak_thread_pool_wait(pool);

  // Validate completion
  size_t executed, completed, failed;
  counter_get_values(counter, &executed, &completed, &failed);
  AK24_TEST_ASSERT_EQ(completed, STRESS_MIXED_WORKLOAD_TASKS);
  AK24_TEST_ASSERT_EQ(failed, 0);

  counter_free(counter);
  ak_thread_pool_free(pool);

  AK24_TEST_PASS();
}

// Test: Rapid pool creation and destruction
static int test_rapid_pool_cycling(void) {
  for (int i = 0; i < STRESS_POOL_CYCLES; i++) {
    ak_thread_pool_t *pool = ak_thread_pool_new(NULL);
    AK24_TEST_ASSERT_NOT_NULL(pool);

    // Enqueue a few tasks
    for (int j = 0; j < 10; j++) {
      ak_lambda_t *task = ak_lambda_new(simple_increment_task, NULL, NULL);
      ak_thread_pool_enqueue(pool, task, NULL);
    }

    ak_thread_pool_wait(pool);
    ak_thread_pool_free(pool);
  }

  AK24_TEST_PASS();
}

// Test: Queue size limits under pressure
static int test_queue_size_limit(void) {
  ak_thread_pool_config_t config = ak_thread_pool_config_default();
  config.max_workers = 2;
  config.max_queue_size = 50; // Limited queue

  ak_thread_pool_t *pool = ak_thread_pool_new(&config);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  int successful_enqueues = 0;
  int failed_enqueues = 0;

  // Try to enqueue more than queue can hold
  for (int i = 0; i < 200; i++) {
    int *workload = AK24_ALLOC(sizeof(int));
    *workload = 10000; // Slow tasks to fill queue

    ak_lambda_t *task =
        ak_lambda_new(variable_workload_task, workload, free_task_context);

    int result = ak_thread_pool_enqueue(pool, task, NULL);
    if (result == 0) {
      successful_enqueues++;
    } else {
      failed_enqueues++;
      ak_lambda_free(task);
    }
  }

  // Should have some failures due to queue limit
  AK24_TEST_ASSERT(failed_enqueues > 0);
  AK24_TEST_ASSERT(successful_enqueues > 0);

  ak_thread_pool_wait(pool);
  ak_thread_pool_free(pool);

  AK24_TEST_PASS();
}

// Test: Large queue stress
static int test_large_queue_stress(void) {
  ak_thread_pool_config_t config = ak_thread_pool_config_default();
  config.max_workers = 4;
  config.max_queue_size = 0; // Unlimited

  ak_thread_pool_t *pool = ak_thread_pool_new(&config);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  shared_counter_t *counter = counter_new();
  AK24_TEST_ASSERT_NOT_NULL(counter);

  // Enqueue large number of tasks
  for (int i = 0; i < STRESS_LARGE_QUEUE; i++) {
    ak_lambda_t *task = ak_lambda_new(simple_increment_task, counter, NULL);
    int result = ak_thread_pool_enqueue(pool, task, NULL);
    AK24_TEST_ASSERT_EQ(result, 0);
  }

  // Check queue size before wait
  size_t pending = ak_thread_pool_pending_count(pool);
  AK24_TEST_ASSERT(pending > 0);

  ak_thread_pool_wait(pool);

  // Validate all completed
  size_t executed, completed, failed;
  counter_get_values(counter, &executed, &completed, &failed);
  AK24_TEST_ASSERT_EQ(executed, STRESS_LARGE_QUEUE);

  counter_free(counter);
  ak_thread_pool_free(pool);

  AK24_TEST_PASS();
}

// Test: Pool status queries under load
static int test_status_queries_under_load(void) {
  ak_thread_pool_config_t config = ak_thread_pool_config_default();
  config.max_workers = 4;

  ak_thread_pool_t *pool = ak_thread_pool_new(&config);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  // Enqueue tasks with varying durations
  for (int i = 0; i < 100; i++) {
    int *workload = AK24_ALLOC(sizeof(int));
    *workload = 5000 + (i * 100);

    ak_lambda_t *task =
        ak_lambda_new(variable_workload_task, workload, free_task_context);
    ak_thread_pool_enqueue(pool, task, NULL);
  }

  // Query status multiple times during execution
  for (int i = 0; i < 10; i++) {
    size_t pending = ak_thread_pool_pending_count(pool);
    size_t active = ak_thread_pool_active_count(pool);
    ak_pool_state_t state = ak_thread_pool_get_state(pool);

    // Validate queries return reasonable values
    AK24_TEST_ASSERT(active <= config.max_workers);
    AK24_TEST_ASSERT(pending >= 0); // Always true, but uses variable
    AK24_TEST_ASSERT_EQ(state, AK24_POOL_STATE_RUNNING);

    usleep(10000);
  }

  ak_thread_pool_wait(pool);

  // After wait, queue should be empty
  AK24_TEST_ASSERT_EQ(ak_thread_pool_pending_count(pool), 0);
  AK24_TEST_ASSERT_EQ(ak_thread_pool_active_count(pool), 0);

  ak_thread_pool_free(pool);

  AK24_TEST_PASS();
}

// Test: Completion callback stress
static int test_completion_callback_stress(void) {
  ak_thread_pool_t *pool = ak_thread_pool_new(NULL);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  shared_counter_t *counter = counter_new();
  AK24_TEST_ASSERT_NOT_NULL(counter);

  // Enqueue tasks with completion callbacks
  for (int i = 0; i < 1000; i++) {
    ak_lambda_t *task = ak_lambda_new(simple_increment_task, counter, NULL);
    ak_lambda_t *cb = ak_lambda_new(stress_completion_callback, counter, NULL);

    ak_thread_pool_enqueue(pool, task, cb);
  }

  ak_thread_pool_wait(pool);

  // Verify all callbacks were invoked
  size_t executed, completed, failed;
  counter_get_values(counter, &executed, &completed, &failed);
  AK24_TEST_ASSERT_EQ(executed, 1000);
  AK24_TEST_ASSERT_EQ(completed, 1000);
  AK24_TEST_ASSERT_EQ(failed, 0);

  counter_free(counter);
  ak_thread_pool_free(pool);

  AK24_TEST_PASS();
}

// Test: Memory pressure (many tasks with contexts)
static int test_memory_pressure(void) {
  ak_thread_pool_config_t config = ak_thread_pool_config_default();
  config.max_workers = 8;

  ak_thread_pool_t *pool = ak_thread_pool_new(&config);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  const int num_tasks = 5000;

  // Each task gets its own context
  for (int i = 0; i < num_tasks; i++) {
    int *workload = AK24_ALLOC(sizeof(int));
    *workload = 100;

    ak_lambda_t *task =
        ak_lambda_new(variable_workload_task, workload, free_task_context);
    ak_thread_pool_enqueue(pool, task, NULL);
  }

  ak_thread_pool_wait(pool);
  ak_thread_pool_free(pool);

  AK24_TEST_PASS();
}

// Test: State transitions under stress
static int test_state_transitions_stress(void) {
  for (int cycle = 0; cycle < 20; cycle++) {
    ak_thread_pool_t *pool = ak_thread_pool_new(NULL);
    AK24_TEST_ASSERT_NOT_NULL(pool);

    // Verify initial state
    AK24_TEST_ASSERT_EQ(ak_thread_pool_get_state(pool),
                        AK24_POOL_STATE_RUNNING);

    // Enqueue some work
    for (int i = 0; i < 50; i++) {
      ak_lambda_t *task = ak_lambda_new(simple_increment_task, NULL, NULL);
      ak_thread_pool_enqueue(pool, task, NULL);
    }

    // State should still be running
    AK24_TEST_ASSERT_EQ(ak_thread_pool_get_state(pool),
                        AK24_POOL_STATE_RUNNING);

    ak_thread_pool_wait(pool);
    ak_thread_pool_free(pool);
  }

  AK24_TEST_PASS();
}

// ============================================================================
// COMPUTATION VALIDATION TESTS
// ============================================================================

// Fibonacci computation task
typedef struct {
  int n;
  int *result;
  AK24_MUTEX *mutex;
} fib_task_t;

static int compute_fibonacci(int n) {
  if (n <= 1)
    return n;
  int a = 0, b = 1;
  for (int i = 2; i <= n; i++) {
    int temp = a + b;
    a = b;
    b = temp;
  }
  return b;
}

static void fibonacci_task(void *ctx, void *args) {
  fib_task_t *task = (fib_task_t *)ctx;
  (void)args;

  if (task && task->result && task->mutex) {
    int computed = compute_fibonacci(task->n);
    AK24_MUTEX_LOCK(task->mutex);
    task->result[task->n] = computed;
    AK24_MUTEX_UNLOCK(task->mutex);
  }
}

// Test: Validate Fibonacci computations
static int test_fibonacci_computation_stress(void) {
  ak_thread_pool_config_t config = ak_thread_pool_config_default();
  config.max_workers = 8;

  ak_thread_pool_t *pool = ak_thread_pool_new(&config);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  const int max_fib = 30;
  int *results = AK24_ALLOC(sizeof(int) * (max_fib + 1));
  AK24_TEST_ASSERT_NOT_NULL(results);

  AK24_MUTEX mutex;
  AK24_MUTEX_INIT(&mutex);

  // Initialize results
  for (int i = 0; i <= max_fib; i++) {
    results[i] = -1;
  }

  // Create tasks for each Fibonacci number
  for (int i = 0; i <= max_fib; i++) {
    fib_task_t *task_data = AK24_ALLOC(sizeof(fib_task_t));
    task_data->n = i;
    task_data->result = results;
    task_data->mutex = &mutex;

    ak_lambda_t *task =
        ak_lambda_new(fibonacci_task, task_data, free_task_context);
    ak_thread_pool_enqueue(pool, task, NULL);
  }

  ak_thread_pool_wait(pool);

  // Validate all Fibonacci results
  int expected_fibs[] = {0,      1,      1,     2,     3,     5,      8,
                         13,     21,     34,    55,    89,    144,    233,
                         377,    610,    987,   1597,  2584,  4181,   6765,
                         10946,  17711,  28657, 46368, 75025, 121393, 196418,
                         317811, 514229, 832040};

  for (int i = 0; i <= max_fib; i++) {
    AK24_TEST_ASSERT_EQ(results[i], expected_fibs[i]);
  }

  AK24_MUTEX_DESTROY(&mutex);
  AK24_FREE(results);
  ak_thread_pool_free(pool);

  AK24_TEST_PASS();
}

// Prime number computation task
typedef struct {
  int start;
  int end;
  int *prime_count;
  AK24_MUTEX *mutex;
} prime_task_t;

static int is_prime(int n) {
  if (n < 2)
    return 0;
  if (n == 2)
    return 1;
  if (n % 2 == 0)
    return 0;
  for (int i = 3; i * i <= n; i += 2) {
    if (n % i == 0)
      return 0;
  }
  return 1;
}

static void prime_counting_task(void *ctx, void *args) {
  prime_task_t *task = (prime_task_t *)ctx;
  (void)args;

  if (task && task->prime_count && task->mutex) {
    int count = 0;
    for (int i = task->start; i < task->end; i++) {
      if (is_prime(i)) {
        count++;
      }
    }

    AK24_MUTEX_LOCK(task->mutex);
    *task->prime_count += count;
    AK24_MUTEX_UNLOCK(task->mutex);
  }
}

// Test: Count primes with parallel computation
static int test_prime_counting_stress(void) {
  ak_thread_pool_t *pool = ak_thread_pool_new(NULL);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  int total_primes = 0;
  AK24_MUTEX mutex;
  AK24_MUTEX_INIT(&mutex);

  const int max_number = 10000;
  const int chunk_size = 100;

  // Break work into chunks
  for (int start = 0; start < max_number; start += chunk_size) {
    prime_task_t *task_data = AK24_ALLOC(sizeof(prime_task_t));
    task_data->start = start;
    task_data->end = start + chunk_size;
    task_data->prime_count = &total_primes;
    task_data->mutex = &mutex;

    ak_lambda_t *task =
        ak_lambda_new(prime_counting_task, task_data, free_task_context);
    ak_thread_pool_enqueue(pool, task, NULL);
  }

  ak_thread_pool_wait(pool);

  // There are 1229 primes less than 10000
  AK24_TEST_ASSERT_EQ(total_primes, 1229);

  AK24_MUTEX_DESTROY(&mutex);
  ak_thread_pool_free(pool);

  AK24_TEST_PASS();
}

// Checksum validation task
typedef struct {
  unsigned char *data;
  size_t length;
  unsigned int *checksum;
  AK24_MUTEX *mutex;
} checksum_task_t;

static unsigned int compute_checksum(unsigned char *data, size_t length) {
  unsigned int sum = 0;
  for (size_t i = 0; i < length; i++) {
    sum = (sum << 5) + sum + data[i]; // hash = hash * 33 + byte
  }
  return sum;
}

static void free_checksum_context(void *ctx) {
  if (ctx) {
    checksum_task_t *task = (checksum_task_t *)ctx;
    if (task->data) {
      AK24_FREE(task->data);
    }
    AK24_FREE(task);
  }
}

static void checksum_task(void *ctx, void *args) {
  checksum_task_t *task = (checksum_task_t *)ctx;
  (void)args;

  if (task && task->data && task->checksum && task->mutex) {
    unsigned int sum = compute_checksum(task->data, task->length);

    AK24_MUTEX_LOCK(task->mutex);
    *task->checksum ^= sum; // XOR checksums together
    AK24_MUTEX_UNLOCK(task->mutex);
  }
}

// Test: Checksum validation of data blocks
static int test_checksum_validation_stress(void) {
  ak_thread_pool_t *pool = ak_thread_pool_new(NULL);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  unsigned int computed_checksum = 0;
  unsigned int expected_checksum = 0;
  AK24_MUTEX mutex;
  AK24_MUTEX_INIT(&mutex);

  const int num_blocks = STRESS_CHECKSUM_TASKS;
  const size_t block_size = 256;

  // Create data blocks and compute expected checksum
  for (int i = 0; i < num_blocks; i++) {
    unsigned char *data = AK24_ALLOC(block_size);

    // Fill with predictable data
    for (size_t j = 0; j < block_size; j++) {
      data[j] = (unsigned char)((i * block_size + j) % 256);
    }

    // Compute expected checksum
    expected_checksum ^= compute_checksum(data, block_size);

    // Create task
    checksum_task_t *task_data = AK24_ALLOC(sizeof(checksum_task_t));
    task_data->data = data;
    task_data->length = block_size;
    task_data->checksum = &computed_checksum;
    task_data->mutex = &mutex;

    ak_lambda_t *task =
        ak_lambda_new(checksum_task, task_data, free_checksum_context);
    ak_thread_pool_enqueue(pool, task, NULL);
  }

  ak_thread_pool_wait(pool);

  // Validate checksum matches
  AK24_TEST_ASSERT_EQ(computed_checksum, expected_checksum);

  AK24_MUTEX_DESTROY(&mutex);
  ak_thread_pool_free(pool);

  AK24_TEST_PASS();
}

// Matrix computation task
typedef struct {
  int row;
  int *matrix_a;
  int *matrix_b;
  int *result;
  int size;
  AK24_MUTEX *mutex;
} matrix_task_t;

static void matrix_row_multiply(void *ctx, void *args) {
  matrix_task_t *task = (matrix_task_t *)ctx;
  (void)args;

  if (task && task->matrix_a && task->matrix_b && task->result) {
    int row = task->row;
    int size = task->size;

    for (int col = 0; col < size; col++) {
      int sum = 0;
      for (int k = 0; k < size; k++) {
        sum += task->matrix_a[row * size + k] * task->matrix_b[k * size + col];
      }

      AK24_MUTEX_LOCK(task->mutex);
      task->result[row * size + col] = sum;
      AK24_MUTEX_UNLOCK(task->mutex);
    }
  }
}

// Test: Matrix multiplication with row-parallel computation
static int test_matrix_multiplication_stress(void) {
  ak_thread_pool_t *pool = ak_thread_pool_new(NULL);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  const int size = 20;
  int *matrix_a = AK24_ALLOC(sizeof(int) * size * size);
  int *matrix_b = AK24_ALLOC(sizeof(int) * size * size);
  int *result = AK24_ALLOC(sizeof(int) * size * size);
  int *expected = AK24_ALLOC(sizeof(int) * size * size);

  AK24_MUTEX mutex;
  AK24_MUTEX_INIT(&mutex);

  // Initialize matrices with simple values
  for (int i = 0; i < size * size; i++) {
    matrix_a[i] = i % 10;
    matrix_b[i] = (i * 2) % 10;
    result[i] = 0;
  }

  // Compute expected result (single-threaded)
  for (int row = 0; row < size; row++) {
    for (int col = 0; col < size; col++) {
      int sum = 0;
      for (int k = 0; k < size; k++) {
        sum += matrix_a[row * size + k] * matrix_b[k * size + col];
      }
      expected[row * size + col] = sum;
    }
  }

  // Create task for each row (parallel)
  for (int row = 0; row < size; row++) {
    matrix_task_t *task_data = AK24_ALLOC(sizeof(matrix_task_t));
    task_data->row = row;
    task_data->matrix_a = matrix_a;
    task_data->matrix_b = matrix_b;
    task_data->result = result;
    task_data->size = size;
    task_data->mutex = &mutex;

    ak_lambda_t *task =
        ak_lambda_new(matrix_row_multiply, task_data, free_task_context);
    ak_thread_pool_enqueue(pool, task, NULL);
  }

  ak_thread_pool_wait(pool);

  // Validate result matches expected
  for (int i = 0; i < size * size; i++) {
    AK24_TEST_ASSERT_EQ(result[i], expected[i]);
  }

  AK24_MUTEX_DESTROY(&mutex);
  AK24_FREE(matrix_a);
  AK24_FREE(matrix_b);
  AK24_FREE(result);
  AK24_FREE(expected);
  ak_thread_pool_free(pool);

  AK24_TEST_PASS();
}

// Array sorting validation task
typedef struct {
  int *array;
  int length;
  int *sorted_array;
  int task_id;
  AK24_MUTEX *mutex;
} sort_task_t;

static void bubble_sort(int *arr, int n) {
  for (int i = 0; i < n - 1; i++) {
    for (int j = 0; j < n - i - 1; j++) {
      if (arr[j] > arr[j + 1]) {
        int temp = arr[j];
        arr[j] = arr[j + 1];
        arr[j + 1] = temp;
      }
    }
  }
}

static void free_sort_context(void *ctx) {
  if (ctx) {
    sort_task_t *task = (sort_task_t *)ctx;
    if (task->array) {
      AK24_FREE(task->array);
    }
    AK24_FREE(task);
  }
}

static void sorting_task(void *ctx, void *args) {
  sort_task_t *task = (sort_task_t *)ctx;
  (void)args;

  if (task && task->array && task->sorted_array && task->mutex) {
    // Make a local copy
    int *local_copy = AK24_ALLOC(sizeof(int) * task->length);
    for (int i = 0; i < task->length; i++) {
      local_copy[i] = task->array[i];
    }

    // Sort local copy
    bubble_sort(local_copy, task->length);

    // Store sorted result
    AK24_MUTEX_LOCK(task->mutex);
    for (int i = 0; i < task->length; i++) {
      task->sorted_array[task->task_id * task->length + i] = local_copy[i];
    }
    AK24_MUTEX_UNLOCK(task->mutex);

    AK24_FREE(local_copy);
  }
}

// Test: Sort multiple arrays and validate results
static int test_array_sorting_stress(void) {
  ak_thread_pool_t *pool = ak_thread_pool_new(NULL);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  const int num_arrays = 100;
  const int array_length = 50;

  int *sorted_results = AK24_ALLOC(sizeof(int) * num_arrays * array_length);
  AK24_MUTEX mutex;
  AK24_MUTEX_INIT(&mutex);

  for (int i = 0; i < num_arrays; i++) {
    int *array = AK24_ALLOC(sizeof(int) * array_length);

    // Fill with pseudo-random values
    for (int j = 0; j < array_length; j++) {
      array[j] = (i * 31 + j * 17) % 1000;
    }

    sort_task_t *task_data = AK24_ALLOC(sizeof(sort_task_t));
    task_data->array = array;
    task_data->length = array_length;
    task_data->sorted_array = sorted_results;
    task_data->task_id = i;
    task_data->mutex = &mutex;

    ak_lambda_t *task =
        ak_lambda_new(sorting_task, task_data, free_sort_context);
    ak_thread_pool_enqueue(pool, task, NULL);
  }

  ak_thread_pool_wait(pool);

  // Validate each sorted array is in order
  for (int i = 0; i < num_arrays; i++) {
    for (int j = 0; j < array_length - 1; j++) {
      int current = sorted_results[i * array_length + j];
      int next = sorted_results[i * array_length + j + 1];
      AK24_TEST_ASSERT(current <= next);
    }
  }

  AK24_MUTEX_DESTROY(&mutex);
  AK24_FREE(sorted_results);
  ak_thread_pool_free(pool);

  AK24_TEST_PASS();
}

int main(void) {
  ak_kernel_init();

  fprintf(stdout, "\n=================================\n");
  fprintf(stdout, "Running Thread Pool Stress Tests\n");
  fprintf(stdout, "=================================\n\n");

  AK24_TEST_RUN(test_high_volume_tasks);
  AK24_TEST_RUN(test_concurrent_enqueuers);
  AK24_TEST_RUN(test_data_validation_stress);
  AK24_TEST_RUN(test_mixed_workload);
  AK24_TEST_RUN(test_rapid_pool_cycling);
  AK24_TEST_RUN(test_queue_size_limit);
  AK24_TEST_RUN(test_large_queue_stress);
  AK24_TEST_RUN(test_status_queries_under_load);
  AK24_TEST_RUN(test_completion_callback_stress);
  AK24_TEST_RUN(test_memory_pressure);
  AK24_TEST_RUN(test_state_transitions_stress);

  fprintf(stdout, "\n=================================\n");
  fprintf(stdout, "Running Computation Tests\n");
  fprintf(stdout, "=================================\n\n");

  AK24_TEST_RUN(test_fibonacci_computation_stress);
  AK24_TEST_RUN(test_prime_counting_stress);
  AK24_TEST_RUN(test_checksum_validation_stress);
  AK24_TEST_RUN(test_matrix_multiplication_stress);
  AK24_TEST_RUN(test_array_sorting_stress);

  ak_kernel_deinit();

  fprintf(stdout, "\n=================================\n");
  fprintf(stdout, "All stress tests passed!\n");
  fprintf(stdout, "=================================\n");

  return 0;
}
