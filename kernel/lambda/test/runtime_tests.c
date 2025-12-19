#include "kernel.h"
#include "lambda.h"
#include "test/assert.h"
#include <stdio.h>
#include <string.h>

typedef struct {
  int value;
  char name[32];
} test_captured_ctx_t;

typedef struct {
  int x;
  int y;
} test_invoke_args_t;

static int g_invocation_count = 0;
static int g_last_captured_value = 0;
static int g_last_invoke_x = 0;
static int g_last_invoke_y = 0;

static void simple_lambda(void *captured_ctx, void *invoke_args) {
  (void)captured_ctx;
  (void)invoke_args;
  g_invocation_count++;
}

static void lambda_with_captured(void *captured_ctx, void *invoke_args) {
  (void)invoke_args;
  test_captured_ctx_t *ctx = (test_captured_ctx_t *)captured_ctx;
  if (ctx) {
    g_last_captured_value = ctx->value;
  }
  g_invocation_count++;
}

static void lambda_with_invoke_args(void *captured_ctx, void *invoke_args) {
  (void)captured_ctx;
  test_invoke_args_t *args = (test_invoke_args_t *)invoke_args;
  if (args) {
    g_last_invoke_x = args->x;
    g_last_invoke_y = args->y;
  }
  g_invocation_count++;
}

static void lambda_with_both(void *captured_ctx, void *invoke_args) {
  test_captured_ctx_t *ctx = (test_captured_ctx_t *)captured_ctx;
  test_invoke_args_t *args = (test_invoke_args_t *)invoke_args;

  if (ctx) {
    g_last_captured_value = ctx->value;
  }
  if (args) {
    g_last_invoke_x = args->x;
    g_last_invoke_y = args->y;
  }
  g_invocation_count++;
}

static void free_captured_ctx(void *ctx) {
  if (ctx) {
    AK24_FREE(ctx);
  }
}

static int test_lambda_new_null_fn(void) {
  ak_lambda_t *lambda = ak_lambda_new(NULL, NULL, NULL);
  AK24_TEST_ASSERT_NULL(lambda);
  AK24_TEST_PASS();
}

static int test_lambda_new_simple(void) {
  ak_lambda_t *lambda = ak_lambda_new(simple_lambda, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(lambda);
  AK24_TEST_ASSERT(lambda->fn == simple_lambda);
  AK24_TEST_ASSERT_NULL(lambda->captured_ctx);
  AK24_TEST_ASSERT_NULL(lambda->ctx_free);
  ak_lambda_free(lambda);
  AK24_TEST_PASS();
}

static int test_lambda_invoke_no_args(void) {
  g_invocation_count = 0;
  ak_lambda_t *lambda = ak_lambda_new(simple_lambda, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(lambda);

  ak_lambda_invoke(lambda, NULL);
  AK24_TEST_ASSERT_EQ(g_invocation_count, 1);

  ak_lambda_invoke(lambda, NULL);
  AK24_TEST_ASSERT_EQ(g_invocation_count, 2);

  ak_lambda_free(lambda);
  AK24_TEST_PASS();
}

static int test_lambda_with_captured_context(void) {
  g_invocation_count = 0;
  g_last_captured_value = 0;

  test_captured_ctx_t *ctx = AK24_ALLOC(sizeof(test_captured_ctx_t));
  AK24_TEST_ASSERT_NOT_NULL(ctx);
  ctx->value = 42;
  strcpy(ctx->name, "test");

  ak_lambda_t *lambda =
      ak_lambda_new(lambda_with_captured, ctx, free_captured_ctx);
  AK24_TEST_ASSERT_NOT_NULL(lambda);

  ak_lambda_invoke(lambda, NULL);
  AK24_TEST_ASSERT_EQ(g_invocation_count, 1);
  AK24_TEST_ASSERT_EQ(g_last_captured_value, 42);

  ak_lambda_free(lambda);
  AK24_TEST_PASS();
}

static int test_lambda_with_invoke_args(void) {
  g_invocation_count = 0;
  g_last_invoke_x = 0;
  g_last_invoke_y = 0;

  ak_lambda_t *lambda = ak_lambda_new(lambda_with_invoke_args, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(lambda);

  test_invoke_args_t args1 = {10, 20};
  ak_lambda_invoke(lambda, &args1);
  AK24_TEST_ASSERT_EQ(g_invocation_count, 1);
  AK24_TEST_ASSERT_EQ(g_last_invoke_x, 10);
  AK24_TEST_ASSERT_EQ(g_last_invoke_y, 20);

  test_invoke_args_t args2 = {30, 40};
  ak_lambda_invoke(lambda, &args2);
  AK24_TEST_ASSERT_EQ(g_invocation_count, 2);
  AK24_TEST_ASSERT_EQ(g_last_invoke_x, 30);
  AK24_TEST_ASSERT_EQ(g_last_invoke_y, 40);

  ak_lambda_free(lambda);
  AK24_TEST_PASS();
}

static int test_lambda_with_both_contexts(void) {
  g_invocation_count = 0;
  g_last_captured_value = 0;
  g_last_invoke_x = 0;
  g_last_invoke_y = 0;

  test_captured_ctx_t *ctx = AK24_ALLOC(sizeof(test_captured_ctx_t));
  AK24_TEST_ASSERT_NOT_NULL(ctx);
  ctx->value = 99;
  strcpy(ctx->name, "captured");

  ak_lambda_t *lambda = ak_lambda_new(lambda_with_both, ctx, free_captured_ctx);
  AK24_TEST_ASSERT_NOT_NULL(lambda);

  test_invoke_args_t args1 = {5, 15};
  ak_lambda_invoke(lambda, &args1);
  AK24_TEST_ASSERT_EQ(g_invocation_count, 1);
  AK24_TEST_ASSERT_EQ(g_last_captured_value, 99);
  AK24_TEST_ASSERT_EQ(g_last_invoke_x, 5);
  AK24_TEST_ASSERT_EQ(g_last_invoke_y, 15);

  test_invoke_args_t args2 = {25, 35};
  ak_lambda_invoke(lambda, &args2);
  AK24_TEST_ASSERT_EQ(g_invocation_count, 2);
  AK24_TEST_ASSERT_EQ(g_last_captured_value, 99);
  AK24_TEST_ASSERT_EQ(g_last_invoke_x, 25);
  AK24_TEST_ASSERT_EQ(g_last_invoke_y, 35);

  ak_lambda_free(lambda);
  AK24_TEST_PASS();
}

static int test_lambda_clone(void) {
  test_captured_ctx_t *ctx = AK24_ALLOC(sizeof(test_captured_ctx_t));
  AK24_TEST_ASSERT_NOT_NULL(ctx);
  ctx->value = 123;

  ak_lambda_t *lambda =
      ak_lambda_new(lambda_with_captured, ctx, free_captured_ctx);
  AK24_TEST_ASSERT_NOT_NULL(lambda);

  ak_lambda_t *clone = ak_lambda_clone(lambda);
  AK24_TEST_ASSERT_NOT_NULL(clone);
  AK24_TEST_ASSERT(clone->fn == lambda->fn);
  AK24_TEST_ASSERT(clone->captured_ctx == lambda->captured_ctx);
  AK24_TEST_ASSERT_NULL(clone->ctx_free);

  g_invocation_count = 0;
  g_last_captured_value = 0;

  ak_lambda_invoke(clone, NULL);
  AK24_TEST_ASSERT_EQ(g_invocation_count, 1);
  AK24_TEST_ASSERT_EQ(g_last_captured_value, 123);

  ak_lambda_free(clone);
  ak_lambda_free(lambda);
  AK24_TEST_PASS();
}

static int test_lambda_has_context(void) {
  ak_lambda_t *lambda1 = ak_lambda_new(simple_lambda, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(lambda1);
  AK24_TEST_ASSERT_EQ(ak_lambda_has_context(lambda1), 0);
  ak_lambda_free(lambda1);

  test_captured_ctx_t *ctx = AK24_ALLOC(sizeof(test_captured_ctx_t));
  ak_lambda_t *lambda2 = ak_lambda_new(simple_lambda, ctx, free_captured_ctx);
  AK24_TEST_ASSERT_NOT_NULL(lambda2);
  AK24_TEST_ASSERT_EQ(ak_lambda_has_context(lambda2), 1);
  ak_lambda_free(lambda2);

  AK24_TEST_PASS();
}

static int test_lambda_get_context(void) {
  test_captured_ctx_t *ctx = AK24_ALLOC(sizeof(test_captured_ctx_t));
  ctx->value = 777;

  ak_lambda_t *lambda = ak_lambda_new(simple_lambda, ctx, free_captured_ctx);
  AK24_TEST_ASSERT_NOT_NULL(lambda);

  test_captured_ctx_t *retrieved =
      (test_captured_ctx_t *)ak_lambda_get_context(lambda);
  AK24_TEST_ASSERT_NOT_NULL(retrieved);
  AK24_TEST_ASSERT_EQ(retrieved->value, 777);

  ak_lambda_free(lambda);
  AK24_TEST_PASS();
}

static int test_lambda_set_context(void) {
  ak_lambda_t *lambda = ak_lambda_new(simple_lambda, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(lambda);
  AK24_TEST_ASSERT_EQ(ak_lambda_has_context(lambda), 0);

  test_captured_ctx_t *ctx = AK24_ALLOC(sizeof(test_captured_ctx_t));
  ctx->value = 888;

  int result = ak_lambda_set_context(lambda, ctx, free_captured_ctx);
  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(ak_lambda_has_context(lambda), 1);

  test_captured_ctx_t *retrieved =
      (test_captured_ctx_t *)ak_lambda_get_context(lambda);
  AK24_TEST_ASSERT_NOT_NULL(retrieved);
  AK24_TEST_ASSERT_EQ(retrieved->value, 888);

  ak_lambda_free(lambda);
  AK24_TEST_PASS();
}

static int test_lambda_invoke_null(void) {
  ak_lambda_invoke(NULL, NULL);
  AK24_TEST_PASS();
}

static int test_lambda_free_null(void) {
  ak_lambda_free(NULL);
  AK24_TEST_PASS();
}

int run_lambda_tests(void) {
  AK24_TEST_RUN(test_lambda_new_null_fn);
  AK24_TEST_RUN(test_lambda_new_simple);
  AK24_TEST_RUN(test_lambda_invoke_no_args);
  AK24_TEST_RUN(test_lambda_with_captured_context);
  AK24_TEST_RUN(test_lambda_with_invoke_args);
  AK24_TEST_RUN(test_lambda_with_both_contexts);
  AK24_TEST_RUN(test_lambda_clone);
  AK24_TEST_RUN(test_lambda_has_context);
  AK24_TEST_RUN(test_lambda_get_context);
  AK24_TEST_RUN(test_lambda_set_context);
  AK24_TEST_RUN(test_lambda_invoke_null);
  AK24_TEST_RUN(test_lambda_free_null);
  return 0;
}

int main(void) {
  ak_kernel_init();
  int result = run_lambda_tests();
  ak_kernel_deinit();
  return result;
}
