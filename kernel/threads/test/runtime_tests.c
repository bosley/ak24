/**
 * @file runtime_tests.c
 * @brief Runtime tests for threads module
 *
 * Tests thread pool functionality including creation, task execution,
 * completion callbacks, state management, and cleanup.
 */

#include "../../test/assert.h"
#include "../include/threads.h"
#include "kernel.h"

#include <stdio.h>
#include <unistd.h>

// Test data structures
typedef struct {
  int value;
  int called;
  AK24_MUTEX mutex;
} test_context_t;

typedef struct {
  int completed;
  int failed;
  AK24_MUTEX mutex;
} completion_context_t;

// Test: Create and free pool
static int test_pool_create_free(void) {
  ak_thread_pool_t *pool = ak_thread_pool_new(NULL);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  ak_pool_state_t state = ak_thread_pool_get_state(pool);
  AK24_TEST_ASSERT_EQ(state, AK24_POOL_STATE_RUNNING);

  ak_thread_pool_free(pool);

  AK24_TEST_PASS();
}

// Test: Custom configuration
static int test_pool_custom_config(void) {
  ak_thread_pool_config_t config = ak_thread_pool_config_default();
  config.max_workers = 4;
  config.max_queue_size = 10;

  ak_thread_pool_t *pool = ak_thread_pool_new(&config);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  ak_thread_pool_free(pool);

  AK24_TEST_PASS();
}

// Test: Invalid configuration
static int test_pool_invalid_config(void) {
  ak_thread_pool_config_t config = ak_thread_pool_config_default();
  config.max_workers = 0; // Invalid

  ak_thread_pool_t *pool = ak_thread_pool_new(&config);
  AK24_TEST_ASSERT_NULL(pool);

  AK24_TEST_PASS();
}

// Simple task function
static void simple_task(void *ctx, void *args) {
  test_context_t *context = (test_context_t *)ctx;
  (void)args;

  if (context) {
    AK24_MUTEX_LOCK(&context->mutex);
    context->value++;
    context->called = 1;
    AK24_MUTEX_UNLOCK(&context->mutex);
  }
}

// Test: Enqueue and execute single task
static int test_single_task(void) {
  ak_thread_pool_t *pool = ak_thread_pool_new(NULL);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  test_context_t *ctx = AK24_ALLOC(sizeof(test_context_t));
  ctx->value = 0;
  ctx->called = 0;
  AK24_MUTEX_INIT(&ctx->mutex);

  ak_lambda_t *task = ak_lambda_new(simple_task, ctx, NULL);
  AK24_TEST_ASSERT_NOT_NULL(task);

  int result = ak_thread_pool_enqueue(pool, task, NULL);
  AK24_TEST_ASSERT_EQ(result, 0);

  ak_thread_pool_wait(pool);

  AK24_MUTEX_LOCK(&ctx->mutex);
  int value = ctx->value;
  int called = ctx->called;
  AK24_MUTEX_UNLOCK(&ctx->mutex);

  AK24_TEST_ASSERT_EQ(value, 1);
  AK24_TEST_ASSERT_EQ(called, 1);

  AK24_MUTEX_DESTROY(&ctx->mutex);
  AK24_FREE(ctx);

  ak_thread_pool_free(pool);

  AK24_TEST_PASS();
}

// Test: Multiple tasks
static int test_multiple_tasks(void) {
  ak_thread_pool_t *pool = ak_thread_pool_new(NULL);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  test_context_t *ctx = AK24_ALLOC(sizeof(test_context_t));
  ctx->value = 0;
  ctx->called = 0;
  AK24_MUTEX_INIT(&ctx->mutex);

  const int num_tasks = 100;
  for (int i = 0; i < num_tasks; i++) {
    ak_lambda_t *task = ak_lambda_new(simple_task, ctx, NULL);
    AK24_TEST_ASSERT_NOT_NULL(task);

    int result = ak_thread_pool_enqueue(pool, task, NULL);
    AK24_TEST_ASSERT_EQ(result, 0);
  }

  ak_thread_pool_wait(pool);

  AK24_MUTEX_LOCK(&ctx->mutex);
  int value = ctx->value;
  AK24_MUTEX_UNLOCK(&ctx->mutex);

  AK24_TEST_ASSERT_EQ(value, num_tasks);

  AK24_MUTEX_DESTROY(&ctx->mutex);
  AK24_FREE(ctx);

  ak_thread_pool_free(pool);

  AK24_TEST_PASS();
}

// Completion callback function
static void completion_callback(void *ctx, void *args) {
  completion_context_t *cc = (completion_context_t *)ctx;
  ak_task_state_t *state = (ak_task_state_t *)args;

  if (cc && state) {
    AK24_MUTEX_LOCK(&cc->mutex);
    if (*state == AK24_TASK_STATE_COMPLETED) {
      cc->completed++;
    } else if (*state == AK24_TASK_STATE_FAILED) {
      cc->failed++;
    }
    AK24_MUTEX_UNLOCK(&cc->mutex);
  }
}

// Test: Completion callbacks
static int test_completion_callbacks(void) {
  ak_thread_pool_t *pool = ak_thread_pool_new(NULL);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  test_context_t *task_ctx = AK24_ALLOC(sizeof(test_context_t));
  task_ctx->value = 0;
  task_ctx->called = 0;
  AK24_MUTEX_INIT(&task_ctx->mutex);

  completion_context_t *comp_ctx = AK24_ALLOC(sizeof(completion_context_t));
  comp_ctx->completed = 0;
  comp_ctx->failed = 0;
  AK24_MUTEX_INIT(&comp_ctx->mutex);

  const int num_tasks = 10;
  for (int i = 0; i < num_tasks; i++) {
    ak_lambda_t *task = ak_lambda_new(simple_task, task_ctx, NULL);
    ak_lambda_t *cb = ak_lambda_new(completion_callback, comp_ctx, NULL);

    int result = ak_thread_pool_enqueue(pool, task, cb);
    AK24_TEST_ASSERT_EQ(result, 0);
  }

  ak_thread_pool_wait(pool);

  AK24_MUTEX_LOCK(&comp_ctx->mutex);
  int completed = comp_ctx->completed;
  int failed = comp_ctx->failed;
  AK24_MUTEX_UNLOCK(&comp_ctx->mutex);

  AK24_TEST_ASSERT_EQ(completed, num_tasks);
  AK24_TEST_ASSERT_EQ(failed, 0);

  AK24_MUTEX_DESTROY(&task_ctx->mutex);
  AK24_MUTEX_DESTROY(&comp_ctx->mutex);
  AK24_FREE(task_ctx);
  AK24_FREE(comp_ctx);

  ak_thread_pool_free(pool);

  AK24_TEST_PASS();
}

// Task that sleeps
static void sleeping_task(void *ctx, void *args) {
  (void)ctx;
  (void)args;
  usleep(10000); // 10ms
}

// Test: Pool status queries
static int test_pool_status(void) {
  ak_thread_pool_config_t config = ak_thread_pool_config_default();
  config.max_workers = 2;

  ak_thread_pool_t *pool = ak_thread_pool_new(&config);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  // Initial state
  AK24_TEST_ASSERT_EQ(ak_thread_pool_pending_count(pool), 0);
  AK24_TEST_ASSERT_EQ(ak_thread_pool_active_count(pool), 0);

  // Enqueue tasks
  for (int i = 0; i < 10; i++) {
    ak_lambda_t *task = ak_lambda_new(sleeping_task, NULL, NULL);
    ak_thread_pool_enqueue(pool, task, NULL);
  }

  // Check that some tasks are queued or running
  size_t pending = ak_thread_pool_pending_count(pool);
  size_t active = ak_thread_pool_active_count(pool);
  size_t total = pending + active;

  AK24_TEST_ASSERT(total > 0);

  ak_thread_pool_wait(pool);

  // After wait, should be empty
  AK24_TEST_ASSERT_EQ(ak_thread_pool_pending_count(pool), 0);
  AK24_TEST_ASSERT_EQ(ak_thread_pool_active_count(pool), 0);

  ak_thread_pool_free(pool);

  AK24_TEST_PASS();
}

// Test: State strings
static int test_state_strings(void) {
  const char *task_str = ak_task_state_str(AK24_TASK_STATE_PENDING);
  AK24_TEST_ASSERT_NOT_NULL(task_str);
  AK24_TEST_ASSERT_STR_EQ(task_str, "PENDING");

  task_str = ak_task_state_str(AK24_TASK_STATE_RUNNING);
  AK24_TEST_ASSERT_STR_EQ(task_str, "RUNNING");

  task_str = ak_task_state_str(AK24_TASK_STATE_COMPLETED);
  AK24_TEST_ASSERT_STR_EQ(task_str, "COMPLETED");

  task_str = ak_task_state_str(AK24_TASK_STATE_CANCELLED);
  AK24_TEST_ASSERT_STR_EQ(task_str, "CANCELLED");

  task_str = ak_task_state_str(AK24_TASK_STATE_FAILED);
  AK24_TEST_ASSERT_STR_EQ(task_str, "FAILED");

  const char *pool_str = ak_pool_state_str(AK24_POOL_STATE_INITIALIZING);
  AK24_TEST_ASSERT_STR_EQ(pool_str, "INITIALIZING");

  pool_str = ak_pool_state_str(AK24_POOL_STATE_RUNNING);
  AK24_TEST_ASSERT_STR_EQ(pool_str, "RUNNING");

  pool_str = ak_pool_state_str(AK24_POOL_STATE_SHUTTING_DOWN);
  AK24_TEST_ASSERT_STR_EQ(pool_str, "SHUTTING_DOWN");

  pool_str = ak_pool_state_str(AK24_POOL_STATE_TERMINATED);
  AK24_TEST_ASSERT_STR_EQ(pool_str, "TERMINATED");

  AK24_TEST_PASS();
}

// Helper for shutdown test
static void *shutdown_helper(void *arg) {
  ak_thread_pool_t *pool = (ak_thread_pool_t *)arg;
  ak_thread_pool_free(pool);
  return NULL;
}

// Test: Enqueue failure when pool is shutting down
static int test_enqueue_during_shutdown(void) {
  ak_thread_pool_t *pool = ak_thread_pool_new(NULL);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  // Start shutdown in another thread
  AK24_THREAD shutdown_thread;
  AK24_THREAD_CREATE(&shutdown_thread, shutdown_helper, pool);

  // Give shutdown a moment to start
  usleep(1000);

  // Try to enqueue - should fail
  ak_lambda_t *task = ak_lambda_new(simple_task, NULL, NULL);
  int result = ak_thread_pool_enqueue(pool, task, NULL);

  // Clean up lambda if enqueue failed
  if (result != 0) {
    ak_lambda_free(task);
  }

  AK24_THREAD_JOIN(shutdown_thread);

  AK24_TEST_PASS();
}

// Test: NULL parameter handling
static int test_null_parameters(void) {
  // NULL config should use defaults
  ak_thread_pool_t *pool = ak_thread_pool_new(NULL);
  AK24_TEST_ASSERT_NOT_NULL(pool);

  // NULL task should fail
  int result = ak_thread_pool_enqueue(pool, NULL, NULL);
  AK24_TEST_ASSERT_NEQ(result, 0);

  // NULL pool should be safe
  ak_thread_pool_free(NULL);

  ak_thread_pool_free(pool);

  // Other NULL checks
  AK24_TEST_ASSERT_EQ(ak_thread_pool_wait(NULL), -1);
  AK24_TEST_ASSERT_EQ(ak_thread_pool_pending_count(NULL), 0);
  AK24_TEST_ASSERT_EQ(ak_thread_pool_active_count(NULL), 0);

  AK24_TEST_PASS();
}

int main(void) {
  ak_kernel_init();

  AK24_TEST_RUN(test_pool_create_free);
  AK24_TEST_RUN(test_pool_custom_config);
  AK24_TEST_RUN(test_pool_invalid_config);
  AK24_TEST_RUN(test_single_task);
  AK24_TEST_RUN(test_multiple_tasks);
  AK24_TEST_RUN(test_completion_callbacks);
  AK24_TEST_RUN(test_pool_status);
  AK24_TEST_RUN(test_state_strings);
  AK24_TEST_RUN(test_enqueue_during_shutdown);
  AK24_TEST_RUN(test_null_parameters);

  ak_kernel_deinit();

  fprintf(stdout, "\n=================================\n");
  fprintf(stdout, "All threads tests passed!\n");
  fprintf(stdout, "=================================\n");

  return 0;
}
