/**
 * @file runtime_tests.c
 * @brief Runtime tests for the box module
 */

#include "box.h"
#include "kernel.h"
#include "lambda.h"
#include "test/assert.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Test Helpers
 * ========================================================================== */

static int g_visit_count = 0;
static int g_last_visited_value = 0;

static void visit_increment(void *captured_ctx, void *invoke_args) {
  (void)captured_ctx;
  int *value = (int *)invoke_args;
  if (value) {
    (*value)++;
  }
  g_visit_count++;
}

static void visit_read_value(void *captured_ctx, void *invoke_args) {
  (void)captured_ctx;
  int *value = (int *)invoke_args;
  if (value) {
    g_last_visited_value = *value;
  }
  g_visit_count++;
}

static void visit_set_from_captured(void *captured_ctx, void *invoke_args) {
  int *target = (int *)invoke_args;
  int *source = (int *)captured_ctx;
  if (target && source) {
    *target = *source;
  }
  g_visit_count++;
}

/* ============================================================================
 * Basic Tests
 * ========================================================================== */

static int test_box_new_null_data(void) {
  ak_box_t *box = ak_box_new(NULL);
  AK24_TEST_ASSERT_NOT_NULL(box);

  void *data = ak_box_get(box);
  AK24_TEST_ASSERT_NULL(data);

  ak_box_free(box);
  AK24_TEST_PASS();
}

static int test_box_new_with_data(void) {
  int value = 42;
  ak_box_t *box = ak_box_new(&value);
  AK24_TEST_ASSERT_NOT_NULL(box);

  int *data = (int *)ak_box_get(box);
  AK24_TEST_ASSERT_NOT_NULL(data);
  AK24_TEST_ASSERT_EQ(*data, 42);

  ak_box_free(box);
  AK24_TEST_PASS();
}

static int test_box_free_null(void) {
  ak_box_free(NULL);
  AK24_TEST_PASS();
}

/* ============================================================================
 * Swap Tests
 * ========================================================================== */

static int test_box_swap_basic(void) {
  int value1 = 10;
  int value2 = 20;

  ak_box_t *box = ak_box_new(&value1);
  AK24_TEST_ASSERT_NOT_NULL(box);

  int *old = (int *)ak_box_swap(box, &value2);
  AK24_TEST_ASSERT_EQ(old, &value1);

  int *current = (int *)ak_box_get(box);
  AK24_TEST_ASSERT_EQ(current, &value2);
  AK24_TEST_ASSERT_EQ(*current, 20);

  ak_box_free(box);
  AK24_TEST_PASS();
}

static int test_box_swap_to_null(void) {
  int value = 42;
  ak_box_t *box = ak_box_new(&value);
  AK24_TEST_ASSERT_NOT_NULL(box);

  int *old = (int *)ak_box_swap(box, NULL);
  AK24_TEST_ASSERT_EQ(old, &value);

  void *current = ak_box_get(box);
  AK24_TEST_ASSERT_NULL(current);

  ak_box_free(box);
  AK24_TEST_PASS();
}

static int test_box_swap_from_null(void) {
  ak_box_t *box = ak_box_new(NULL);
  AK24_TEST_ASSERT_NOT_NULL(box);

  int value = 99;
  void *old = ak_box_swap(box, &value);
  AK24_TEST_ASSERT_NULL(old);

  int *current = (int *)ak_box_get(box);
  AK24_TEST_ASSERT_EQ(current, &value);
  AK24_TEST_ASSERT_EQ(*current, 99);

  ak_box_free(box);
  AK24_TEST_PASS();
}

static int test_box_swap_null_box(void) {
  int value = 42;
  void *result = ak_box_swap(NULL, &value);
  AK24_TEST_ASSERT_NULL(result);
  AK24_TEST_PASS();
}

/* ============================================================================
 * Visit Tests
 * ========================================================================== */

static int test_box_visit_basic(void) {
  g_visit_count = 0;
  int value = 10;
  ak_box_t *box = ak_box_new(&value);
  AK24_TEST_ASSERT_NOT_NULL(box);

  ak_lambda_t *lambda = ak_lambda_new(visit_increment, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(lambda);

  int result = ak_box_visit(box, lambda);
  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(g_visit_count, 1);
  AK24_TEST_ASSERT_EQ(value, 11);

  ak_lambda_free(lambda);
  ak_box_free(box);
  AK24_TEST_PASS();
}

static int test_box_visit_multiple(void) {
  g_visit_count = 0;
  int value = 0;
  ak_box_t *box = ak_box_new(&value);
  AK24_TEST_ASSERT_NOT_NULL(box);

  ak_lambda_t *lambda = ak_lambda_new(visit_increment, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(lambda);

  for (int i = 0; i < 5; i++) {
    int result = ak_box_visit(box, lambda);
    AK24_TEST_ASSERT_EQ(result, 0);
  }

  AK24_TEST_ASSERT_EQ(g_visit_count, 5);
  AK24_TEST_ASSERT_EQ(value, 5);

  ak_lambda_free(lambda);
  ak_box_free(box);
  AK24_TEST_PASS();
}

static int test_box_visit_with_captured_context(void) {
  g_visit_count = 0;
  int target = 0;
  int source = 42;

  ak_box_t *box = ak_box_new(&target);
  AK24_TEST_ASSERT_NOT_NULL(box);

  ak_lambda_t *lambda = ak_lambda_new(visit_set_from_captured, &source, NULL);
  AK24_TEST_ASSERT_NOT_NULL(lambda);

  int result = ak_box_visit(box, lambda);
  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(g_visit_count, 1);
  AK24_TEST_ASSERT_EQ(target, 42);

  ak_lambda_free(lambda);
  ak_box_free(box);
  AK24_TEST_PASS();
}

static int test_box_visit_null_box(void) {
  ak_lambda_t *lambda = ak_lambda_new(visit_increment, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(lambda);

  int result = ak_box_visit(NULL, lambda);
  AK24_TEST_ASSERT_EQ(result, -1);

  ak_lambda_free(lambda);
  AK24_TEST_PASS();
}

static int test_box_visit_null_lambda(void) {
  int value = 10;
  ak_box_t *box = ak_box_new(&value);
  AK24_TEST_ASSERT_NOT_NULL(box);

  int result = ak_box_visit(box, NULL);
  AK24_TEST_ASSERT_EQ(result, -1);

  ak_box_free(box);
  AK24_TEST_PASS();
}

static int test_box_visit_reads_value(void) {
  g_visit_count = 0;
  g_last_visited_value = 0;

  int value = 123;
  ak_box_t *box = ak_box_new(&value);
  AK24_TEST_ASSERT_NOT_NULL(box);

  ak_lambda_t *lambda = ak_lambda_new(visit_read_value, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(lambda);

  int result = ak_box_visit(box, lambda);
  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(g_visit_count, 1);
  AK24_TEST_ASSERT_EQ(g_last_visited_value, 123);

  ak_lambda_free(lambda);
  ak_box_free(box);
  AK24_TEST_PASS();
}

/* ============================================================================
 * Thread Safety Tests
 * ========================================================================== */

typedef struct {
  ak_box_t *box;
  int iterations;
  ak_lambda_t *lambda;
} thread_test_ctx_t;

static void *thread_visitor(void *arg) {
  thread_test_ctx_t *ctx = (thread_test_ctx_t *)arg;
  for (int i = 0; i < ctx->iterations; i++) {
    ak_box_visit(ctx->box, ctx->lambda);
  }
  return NULL;
}

static int test_box_concurrent_visits(void) {
  int value = 0;
  ak_box_t *box = ak_box_new(&value);
  AK24_TEST_ASSERT_NOT_NULL(box);

  ak_lambda_t *lambda = ak_lambda_new(visit_increment, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(lambda);

  thread_test_ctx_t ctx = {.box = box, .iterations = 1000, .lambda = lambda};

  AK24_THREAD threads[4];
  for (int i = 0; i < 4; i++) {
    int result = AK24_THREAD_CREATE(&threads[i], thread_visitor, &ctx);
    AK24_TEST_ASSERT_EQ(result, 0);
  }

  for (int i = 0; i < 4; i++) {
    AK24_THREAD_JOIN(threads[i]);
  }

  AK24_TEST_ASSERT_EQ(value, 4000);

  ak_lambda_free(lambda);
  ak_box_free(box);
  AK24_TEST_PASS();
}

static void *thread_swapper(void *arg) {
  thread_test_ctx_t *ctx = (thread_test_ctx_t *)arg;
  int local_value = 0;
  for (int i = 0; i < ctx->iterations; i++) {
    void *old = ak_box_swap(ctx->box, &local_value);
    (void)old;
  }
  return NULL;
}

static int test_box_concurrent_swaps(void) {
  int value = 0;
  ak_box_t *box = ak_box_new(&value);
  AK24_TEST_ASSERT_NOT_NULL(box);

  thread_test_ctx_t ctx = {.box = box, .iterations = 1000, .lambda = NULL};

  AK24_THREAD threads[4];
  for (int i = 0; i < 4; i++) {
    int result = AK24_THREAD_CREATE(&threads[i], thread_swapper, &ctx);
    AK24_TEST_ASSERT_EQ(result, 0);
  }

  for (int i = 0; i < 4; i++) {
    AK24_THREAD_JOIN(threads[i]);
  }

  ak_box_free(box);
  AK24_TEST_PASS();
}

/* ============================================================================
 * Get Tests
 * ========================================================================== */

static int test_box_get_null(void) {
  void *result = ak_box_get(NULL);
  AK24_TEST_ASSERT_NULL(result);
  AK24_TEST_PASS();
}

/* ============================================================================
 * Test Runner
 * ========================================================================== */

static int run_box_tests(void) {
  printf("\n=== Box Runtime Tests ===\n\n");

  AK24_TEST_RUN(test_box_new_null_data);
  AK24_TEST_RUN(test_box_new_with_data);
  AK24_TEST_RUN(test_box_free_null);

  AK24_TEST_RUN(test_box_swap_basic);
  AK24_TEST_RUN(test_box_swap_to_null);
  AK24_TEST_RUN(test_box_swap_from_null);
  AK24_TEST_RUN(test_box_swap_null_box);

  AK24_TEST_RUN(test_box_visit_basic);
  AK24_TEST_RUN(test_box_visit_multiple);
  AK24_TEST_RUN(test_box_visit_with_captured_context);
  AK24_TEST_RUN(test_box_visit_null_box);
  AK24_TEST_RUN(test_box_visit_null_lambda);
  AK24_TEST_RUN(test_box_visit_reads_value);

  AK24_TEST_RUN(test_box_get_null);

  AK24_TEST_RUN(test_box_concurrent_visits);
  AK24_TEST_RUN(test_box_concurrent_swaps);

  printf("\n=== All Box Tests Passed ===\n\n");
  return 0;
}

int main(void) {
  ak_kernel_init("ak24-box-test");
  int result = run_box_tests();
  ak_kernel_deinit();
  return result;
}
