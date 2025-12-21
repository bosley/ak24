#include "context.h"
#include "kernel.h"
#include "test/assert.h"
#include <string.h>

int context_basic_operations_test(void) {
  ak_context_t *ctx = ak_context_new();
  AK24_TEST_ASSERT_NOT_NULL(ctx);
  AK24_TEST_ASSERT_NULL(ctx->parent);
  AK24_TEST_ASSERT_EQ(ak_context_depth(ctx), 1);

  int value1 = 42;
  int value2 = 100;

  AK24_TEST_ASSERT_EQ(ak_context_set(ctx, "key1", &value1), 0);
  AK24_TEST_ASSERT_EQ(ak_context_set(ctx, "key2", &value2), 0);

  int *result1 = ak_context_get(ctx, "key1");
  AK24_TEST_ASSERT_NOT_NULL(result1);
  AK24_TEST_ASSERT_EQ(*result1, 42);

  int *result2 = ak_context_get(ctx, "key2");
  AK24_TEST_ASSERT_NOT_NULL(result2);
  AK24_TEST_ASSERT_EQ(*result2, 100);

  void *result3 = ak_context_get(ctx, "nonexistent");
  AK24_TEST_ASSERT_NULL(result3);

  ak_context_free(ctx);
  AK24_TEST_PASS();
}

int context_push_pop_test(void) {
  ak_context_t *root = ak_context_new();
  AK24_TEST_ASSERT_NOT_NULL(root);

  int root_value = 10;
  ak_context_set(root, "root_key", &root_value);

  ak_context_t *child = ak_context_push(root);
  AK24_TEST_ASSERT_NOT_NULL(child);
  AK24_TEST_ASSERT_EQ(child->parent, root);
  AK24_TEST_ASSERT_EQ(ak_context_depth(child), 2);

  int child_value = 20;
  ak_context_set(child, "child_key", &child_value);

  int *result = ak_context_get(child, "root_key");
  AK24_TEST_ASSERT_NOT_NULL(result);
  AK24_TEST_ASSERT_EQ(*result, 10);

  result = ak_context_get(child, "child_key");
  AK24_TEST_ASSERT_NOT_NULL(result);
  AK24_TEST_ASSERT_EQ(*result, 20);

  ak_context_t *popped = ak_context_pop(child);
  AK24_TEST_ASSERT_EQ(popped, root);

  result = ak_context_get(root, "root_key");
  AK24_TEST_ASSERT_NOT_NULL(result);
  AK24_TEST_ASSERT_EQ(*result, 10);

  result = ak_context_get(root, "child_key");
  AK24_TEST_ASSERT_NULL(result);

  ak_context_free(root);
  AK24_TEST_PASS();
}

int context_scope_shadowing_test(void) {
  ak_context_t *root = ak_context_new();

  int root_value = 100;
  ak_context_set(root, "shared_key", &root_value);

  ak_context_t *child = ak_context_push(root);

  int child_value = 200;
  ak_context_set(child, "shared_key", &child_value);

  int *result = ak_context_get(child, "shared_key");
  AK24_TEST_ASSERT_NOT_NULL(result);
  AK24_TEST_ASSERT_EQ(*result, 200);

  result = ak_context_get_local(child, "shared_key");
  AK24_TEST_ASSERT_NOT_NULL(result);
  AK24_TEST_ASSERT_EQ(*result, 200);

  result = ak_context_get_local(root, "shared_key");
  AK24_TEST_ASSERT_NOT_NULL(result);
  AK24_TEST_ASSERT_EQ(*result, 100);

  ak_context_pop(child);
  ak_context_free(root);
  AK24_TEST_PASS();
}

int context_hoist_test(void) {
  ak_context_t *root = ak_context_new();
  ak_context_t *child = ak_context_push(root);

  int value = 42;
  ak_context_set(child, "hoist_me", &value);

  AK24_TEST_ASSERT_EQ(ak_context_hoist(child, "hoist_me"), 0);

  int *result = ak_context_get_local(root, "hoist_me");
  AK24_TEST_ASSERT_NULL(result);

  ak_context_t *popped = ak_context_pop(child);
  AK24_TEST_ASSERT_EQ(popped, root);

  result = ak_context_get(root, "hoist_me");
  AK24_TEST_ASSERT_NOT_NULL(result);
  AK24_TEST_ASSERT_EQ(*result, 42);

  ak_context_free(root);
  AK24_TEST_PASS();
}

int context_multiple_hoist_test(void) {
  ak_context_t *root = ak_context_new();
  ak_context_t *child = ak_context_push(root);

  int val1 = 10, val2 = 20, val3 = 30;
  ak_context_set(child, "key1", &val1);
  ak_context_set(child, "key2", &val2);
  ak_context_set(child, "key3", &val3);

  ak_context_hoist(child, "key1");
  ak_context_hoist(child, "key3");

  ak_context_pop(child);

  int *r1 = ak_context_get(root, "key1");
  AK24_TEST_ASSERT_NOT_NULL(r1);
  AK24_TEST_ASSERT_EQ(*r1, 10);

  int *r2 = ak_context_get(root, "key2");
  AK24_TEST_ASSERT_NULL(r2);

  int *r3 = ak_context_get(root, "key3");
  AK24_TEST_ASSERT_NOT_NULL(r3);
  AK24_TEST_ASSERT_EQ(*r3, 30);

  ak_context_free(root);
  AK24_TEST_PASS();
}

int context_deep_nesting_test(void) {
  ak_context_t *ctx = ak_context_new();

  int val1 = 1, val2 = 2, val3 = 3;
  ak_context_set(ctx, "level1", &val1);

  ak_context_t *level2 = ak_context_push(ctx);
  ak_context_set(level2, "level2", &val2);
  AK24_TEST_ASSERT_EQ(ak_context_depth(level2), 2);

  ak_context_t *level3 = ak_context_push(level2);
  ak_context_set(level3, "level3", &val3);
  AK24_TEST_ASSERT_EQ(ak_context_depth(level3), 3);

  int *r1 = ak_context_get(level3, "level1");
  AK24_TEST_ASSERT_NOT_NULL(r1);
  AK24_TEST_ASSERT_EQ(*r1, 1);

  int *r2 = ak_context_get(level3, "level2");
  AK24_TEST_ASSERT_NOT_NULL(r2);
  AK24_TEST_ASSERT_EQ(*r2, 2);

  int *r3 = ak_context_get(level3, "level3");
  AK24_TEST_ASSERT_NOT_NULL(r3);
  AK24_TEST_ASSERT_EQ(*r3, 3);

  level2 = ak_context_pop(level3);
  AK24_TEST_ASSERT_EQ(ak_context_depth(level2), 2);

  ctx = ak_context_pop(level2);
  AK24_TEST_ASSERT_EQ(ak_context_depth(ctx), 1);

  ak_context_free(ctx);
  AK24_TEST_PASS();
}

int context_has_test(void) {
  ak_context_t *root = ak_context_new();
  ak_context_t *child = ak_context_push(root);

  int val = 42;
  ak_context_set(root, "root_key", &val);
  ak_context_set(child, "child_key", &val);

  AK24_TEST_ASSERT(ak_context_has(child, "root_key"));
  AK24_TEST_ASSERT(ak_context_has(child, "child_key"));
  AK24_TEST_ASSERT(!ak_context_has(child, "nonexistent"));

  AK24_TEST_ASSERT(!ak_context_has_local(child, "root_key"));
  AK24_TEST_ASSERT(ak_context_has_local(child, "child_key"));

  AK24_TEST_ASSERT(ak_context_has_local(root, "root_key"));
  AK24_TEST_ASSERT(!ak_context_has_local(root, "child_key"));

  ak_context_pop(child);
  ak_context_free(root);
  AK24_TEST_PASS();
}

int context_string_values_test(void) {
  ak_context_t *ctx = ak_context_new();

  char *str1 = "hello";
  char *str2 = "world";

  ak_context_set(ctx, "greeting", str1);
  ak_context_set(ctx, "target", str2);

  char *result1 = ak_context_get(ctx, "greeting");
  AK24_TEST_ASSERT_NOT_NULL(result1);
  AK24_TEST_ASSERT_STR_EQ(result1, "hello");

  char *result2 = ak_context_get(ctx, "target");
  AK24_TEST_ASSERT_NOT_NULL(result2);
  AK24_TEST_ASSERT_STR_EQ(result2, "world");

  ak_context_free(ctx);
  AK24_TEST_PASS();
}

int context_hoist_duplicate_test(void) {
  ak_context_t *root = ak_context_new();
  ak_context_t *child = ak_context_push(root);

  int val = 100;
  ak_context_set(child, "key", &val);

  AK24_TEST_ASSERT_EQ(ak_context_hoist(child, "key"), 0);
  AK24_TEST_ASSERT_EQ(ak_context_hoist(child, "key"), 0);
  AK24_TEST_ASSERT_EQ(ak_context_hoist(child, "key"), 0);

  ak_context_pop(child);

  int *result = ak_context_get(root, "key");
  AK24_TEST_ASSERT_NOT_NULL(result);
  AK24_TEST_ASSERT_EQ(*result, 100);

  ak_context_free(root);
  AK24_TEST_PASS();
}

int context_get_containing_context_test(void) {
  ak_context_t *root = ak_context_new();
  ak_context_t *child1 = ak_context_push(root);
  ak_context_t *child2 = ak_context_push(child1);

  int root_val = 10, child1_val = 20, child2_val = 30;
  ak_context_set(root, "root_key", &root_val);
  ak_context_set(child1, "child1_key", &child1_val);
  ak_context_set(child2, "child2_key", &child2_val);

  ak_context_t *found = ak_context_get_containing_context(child2, "root_key");
  AK24_TEST_ASSERT_EQ(found, root);

  found = ak_context_get_containing_context(child2, "child1_key");
  AK24_TEST_ASSERT_EQ(found, child1);

  found = ak_context_get_containing_context(child2, "child2_key");
  AK24_TEST_ASSERT_EQ(found, child2);

  found = ak_context_get_containing_context(child2, "nonexistent");
  AK24_TEST_ASSERT_NULL(found);

  found = ak_context_get_containing_context(root, "child1_key");
  AK24_TEST_ASSERT_NULL(found);

  ak_context_pop(child2);
  ak_context_pop(child1);
  ak_context_free(root);
  AK24_TEST_PASS();
}

int context_null_handling_test(void) {
  AK24_TEST_ASSERT_NULL(ak_context_push(NULL));
  AK24_TEST_ASSERT_NULL(ak_context_pop(NULL));
  AK24_TEST_ASSERT_EQ(ak_context_set(NULL, "key", NULL), -1);
  AK24_TEST_ASSERT_NULL(ak_context_get(NULL, "key"));
  AK24_TEST_ASSERT_NULL(ak_context_get_local(NULL, "key"));
  AK24_TEST_ASSERT_NULL(ak_context_get_containing_context(NULL, "key"));
  AK24_TEST_ASSERT_EQ(ak_context_hoist(NULL, "key"), -1);
  AK24_TEST_ASSERT_EQ(ak_context_has(NULL, "key"), 0);
  AK24_TEST_ASSERT_EQ(ak_context_has_local(NULL, "key"), 0);
  AK24_TEST_ASSERT_EQ(ak_context_depth(NULL), 0);

  ak_context_free(NULL);

  AK24_TEST_PASS();
}

int run_context_tests(void) {
  AK24_TEST_RUN(context_basic_operations_test);
  AK24_TEST_RUN(context_push_pop_test);
  AK24_TEST_RUN(context_scope_shadowing_test);
  AK24_TEST_RUN(context_hoist_test);
  AK24_TEST_RUN(context_multiple_hoist_test);
  AK24_TEST_RUN(context_deep_nesting_test);
  AK24_TEST_RUN(context_has_test);
  AK24_TEST_RUN(context_string_values_test);
  AK24_TEST_RUN(context_hoist_duplicate_test);
  AK24_TEST_RUN(context_get_containing_context_test);
  AK24_TEST_RUN(context_null_handling_test);
  return 0;
}

int main(void) {
  ak_kernel_init("ak24-test");
  int result = run_context_tests();
  ak_kernel_deinit();
  return result;
}
