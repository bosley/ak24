/**
 * @file compile_time_tests.c
 * @brief Compile-time tests for threads module
 *
 * These tests verify that the threads API compiles correctly and that
 * types are defined properly.
 */

#include "../include/threads.h"
#include "kernel.h"
#include <stddef.h>

// Test that all types are defined
static void test_types_defined(void) {
  ak_thread_pool_t *pool = NULL;
  ak_task_state_t state = AK24_TASK_STATE_PENDING;
  ak_pool_state_t pool_state = AK24_POOL_STATE_RUNNING;
  ak_thread_pool_config_t config;

  (void)pool;
  (void)state;
  (void)pool_state;
  (void)config;
}

// Test that all enums are complete
static void test_enums_complete(void) {
  ak_task_state_t states[] = {AK24_TASK_STATE_PENDING, AK24_TASK_STATE_RUNNING,
                              AK24_TASK_STATE_COMPLETED,
                              AK24_TASK_STATE_CANCELLED,
                              AK24_TASK_STATE_FAILED};

  ak_pool_state_t pool_states[] = {
      AK24_POOL_STATE_INITIALIZING, AK24_POOL_STATE_RUNNING,
      AK24_POOL_STATE_SHUTTING_DOWN, AK24_POOL_STATE_TERMINATED};

  (void)states;
  (void)pool_states;
}

// Test that config struct has required fields
static void test_config_struct(void) {
  ak_thread_pool_config_t config;
  config.max_workers = 8;
  config.min_workers = 2;
  config.max_queue_size = 100;

  (void)config;
}

// Test that default config works
static void test_default_config(void) {
  ak_thread_pool_config_t config = ak_thread_pool_config_default();

  // Check defaults are reasonable
  _Static_assert(AK24_THREAD_POOL_DEFAULT_MAX_WORKERS > 0,
                 "Default max workers must be > 0");
  _Static_assert(AK24_THREAD_POOL_DEFAULT_MIN_WORKERS > 0,
                 "Default min workers must be > 0");
  _Static_assert(AK24_THREAD_POOL_DEFAULT_MIN_WORKERS <=
                     AK24_THREAD_POOL_DEFAULT_MAX_WORKERS,
                 "Default min workers must be <= max workers");

  (void)config;
}

// Test that all functions are declared
static void test_functions_declared(void) {
  ak_thread_pool_config_t config = ak_thread_pool_config_default();
  ak_thread_pool_t *pool = ak_thread_pool_new(&config);

  if (pool) {
    ak_lambda_t *task = NULL;
    ak_lambda_t *cb = NULL;

    ak_thread_pool_enqueue(pool, task, cb);
    ak_thread_pool_wait(pool);
    ak_thread_pool_pending_count(pool);
    ak_thread_pool_active_count(pool);
    ak_thread_pool_get_state(pool);
    ak_thread_pool_free(pool);
  }

  ak_task_state_str(AK24_TASK_STATE_PENDING);
  ak_pool_state_str(AK24_POOL_STATE_RUNNING);
}

// Test version constant
static void test_version_defined(void) {
  const char *version = THREADS_VERSION;
  (void)version;
}

int main(void) {
  test_types_defined();
  test_enums_complete();
  test_config_struct();
  test_default_config();
  test_functions_declared();
  test_version_defined();

  return 0;
}
