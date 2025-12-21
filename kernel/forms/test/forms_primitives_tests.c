#include "forms_primitives.h"
#include "kernel.h"
#include "test/assert.h"

static int test_primitive_forms_not_singleton(void) {
  ak_form_t *i32_1 = ak_primitive_i32();
  ak_form_t *i32_2 = ak_primitive_i32();

  AK24_TEST_ASSERT_NEQ(i32_1, i32_2);
  AK24_TEST_ASSERT_EQ(i32_1->kind, AK_FORM_PRIMITIVE);
  AK24_TEST_ASSERT_EQ(i32_1->data.primitive, AK_FORM_PRIMITIVE_I32);
  AK24_TEST_ASSERT_EQ(i32_2->kind, AK_FORM_PRIMITIVE);
  AK24_TEST_ASSERT_EQ(i32_2->data.primitive, AK_FORM_PRIMITIVE_I32);

  ak_form_free(i32_1);
  ak_form_free(i32_2);

  AK24_TEST_PASS();
}

static int test_all_primitive_forms(void) {
  ak_form_t *forms[] = {
      ak_primitive_bool(), ak_primitive_u8(),  ak_primitive_u16(),
      ak_primitive_u32(),  ak_primitive_u64(), ak_primitive_i8(),
      ak_primitive_i16(),  ak_primitive_i32(), ak_primitive_i64(),
      ak_primitive_f32(),  ak_primitive_f64()};

  for (int i = 0; i < 11; i++) {
    AK24_TEST_ASSERT_NOT_NULL(forms[i]);
    ak_form_free(forms[i]);
  }

  AK24_TEST_PASS();
}

static int test_optional_with_builtin_affects(void) {
  ak_form_t *i32_form = ak_primitive_i32();
  ak_form_t *optional = ak_form_new_optional(i32_form);

  ak_form_attach_builtin_affects(optional);

  AK24_TEST_ASSERT(ak_form_has_affect(optional, "is_none"));
  AK24_TEST_ASSERT(ak_form_has_affect(optional, "is_some"));

  ak_optional_instance_t *inst_none = ak_optional_instance_new(optional);

  ak_affect_t *is_none_aff = ak_form_get_affect(optional, "is_none");
  AK24_TEST_ASSERT_NOT_NULL(is_none_aff);

  typedef struct {
    void *result;
  } affect_result_t;

  affect_result_t result = {0};
  ak_lambda_set_context(is_none_aff->lambda, &result, NULL);
  ak_lambda_invoke(is_none_aff->lambda, inst_none);

  int *is_none_val = (int *)result.result;
  AK24_TEST_ASSERT_NOT_NULL(is_none_val);
  AK24_TEST_ASSERT_EQ(*is_none_val, 1);
  AK24_FREE(is_none_val);

  ak_optional_instance_set(inst_none, (void *)42);

  result.result = NULL;
  ak_lambda_invoke(is_none_aff->lambda, inst_none);
  is_none_val = (int *)result.result;
  AK24_TEST_ASSERT_NOT_NULL(is_none_val);
  AK24_TEST_ASSERT_EQ(*is_none_val, 0);
  AK24_FREE(is_none_val);

  ak_optional_instance_free(inst_none);
  ak_form_free(optional);
  AK24_TEST_PASS();
}

static int test_repeatable_with_builtin_affects(void) {
  ak_form_t *i32_form = ak_primitive_i32();
  ak_form_t *repeatable = ak_form_new_repeatable(i32_form);

  ak_form_attach_builtin_affects(repeatable);

  AK24_TEST_ASSERT(ak_form_has_affect(repeatable, "append"));
  AK24_TEST_ASSERT(ak_form_has_affect(repeatable, "pop"));
  AK24_TEST_ASSERT(ak_form_has_affect(repeatable, "push"));
  AK24_TEST_ASSERT(ak_form_has_affect(repeatable, "rest"));

  ak_repeatable_instance_t *inst = ak_repeatable_instance_new(repeatable);

  ak_affect_t *append_aff = ak_form_get_affect(repeatable, "append");
  AK24_TEST_ASSERT_NOT_NULL(append_aff);

  void *val1 = (void *)10;
  void *val2 = (void *)20;
  void *val3 = (void *)30;

  void *append_args1[] = {inst, val1};
  void *append_args2[] = {inst, val2};
  void *append_args3[] = {inst, val3};

  ak_lambda_invoke(append_aff->lambda, append_args1);
  ak_lambda_invoke(append_aff->lambda, append_args2);
  ak_lambda_invoke(append_aff->lambda, append_args3);

  AK24_TEST_ASSERT_EQ(list_count(&inst->items), 3);

  ak_affect_t *pop_aff = ak_form_get_affect(repeatable, "pop");

  typedef struct {
    void *result;
  } affect_result_t;

  affect_result_t pop_result = {0};
  ak_lambda_set_context(pop_aff->lambda, &pop_result, NULL);
  ak_lambda_invoke(pop_aff->lambda, inst);

  void *popped = pop_result.result;
  AK24_TEST_ASSERT_NOT_NULL(popped);
  AK24_TEST_ASSERT_EQ((long)popped, 30);
  AK24_TEST_ASSERT_EQ(list_count(&inst->items), 2);

  ak_repeatable_instance_free(inst);
  ak_form_free(repeatable);
  AK24_TEST_PASS();
}

static int test_list_with_builtin_affects(void) {
  ak_form_t *i32_form = ak_primitive_i32();
  ak_form_t *list_form = ak_form_new_list(i32_form);

  ak_form_attach_builtin_affects(list_form);

  AK24_TEST_ASSERT(ak_form_has_affect(list_form, "index"));
  AK24_TEST_ASSERT(ak_form_has_affect(list_form, "length"));

  ak_list_instance_t *inst = ak_list_instance_new(list_form);

  int val1 = 100;
  int val2 = 200;
  list_push(&inst->items, &val1);
  list_push(&inst->items, &val2);

  ak_affect_t *length_aff = ak_form_get_affect(list_form, "length");

  typedef struct {
    void *result;
  } affect_result_t;

  affect_result_t result = {0};
  ak_lambda_set_context(length_aff->lambda, &result, NULL);
  ak_lambda_invoke(length_aff->lambda, inst);

  unsigned *len = (unsigned *)result.result;
  AK24_TEST_ASSERT_NOT_NULL(len);
  AK24_TEST_ASSERT_EQ(*len, 2);
  AK24_FREE(len);

  ak_affect_t *index_aff = ak_form_get_affect(list_form, "index");
  unsigned idx = 1;
  void *index_args[] = {inst, &idx};

  result.result = NULL;
  ak_lambda_set_context(index_aff->lambda, &result, NULL);
  ak_lambda_invoke(index_aff->lambda, index_args);

  int *indexed_val = (int *)result.result;
  AK24_TEST_ASSERT_NOT_NULL(indexed_val);
  AK24_TEST_ASSERT_EQ(*indexed_val, 200);

  ak_list_instance_free(inst);
  ak_form_free(list_form);
  AK24_TEST_PASS();
}

static int test_map_with_builtin_affects(void) {
  ak_form_t *i32_form = ak_primitive_i32();
  ak_form_t *map_form = ak_form_new_map(AK_FORM_PRIMITIVE_U32, i32_form);

  ak_form_attach_builtin_affects(map_form);

  AK24_TEST_ASSERT(ak_form_has_affect(map_form, "get"));
  AK24_TEST_ASSERT(ak_form_has_affect(map_form, "set"));
  AK24_TEST_ASSERT(ak_form_has_affect(map_form, "has"));

  ak_map_instance_t *inst = ak_map_instance_new(map_form);

  ak_affect_t *set_aff = ak_form_get_affect(map_form, "set");

  void *key1 = (void *)1;
  int val1 = 42;
  void *set_args[] = {inst, key1, &val1};

  ak_lambda_invoke(set_aff->lambda, set_args);

  ak_affect_t *has_aff = ak_form_get_affect(map_form, "has");
  void *has_args[] = {inst, key1};

  typedef struct {
    void *result;
  } affect_result_t;

  affect_result_t result = {0};
  ak_lambda_set_context(has_aff->lambda, &result, NULL);
  ak_lambda_invoke(has_aff->lambda, has_args);

  int *has_val = (int *)result.result;
  AK24_TEST_ASSERT_NOT_NULL(has_val);
  AK24_TEST_ASSERT_EQ(*has_val, 1);
  AK24_FREE(has_val);

  ak_affect_t *get_aff = ak_form_get_affect(map_form, "get");
  void *get_args[] = {inst, key1};

  result.result = NULL;
  ak_lambda_set_context(get_aff->lambda, &result, NULL);
  ak_lambda_invoke(get_aff->lambda, get_args);

  int *retrieved = (int *)result.result;
  AK24_TEST_ASSERT_NOT_NULL(retrieved);
  AK24_TEST_ASSERT_EQ(*retrieved, 42);

  ak_map_instance_free(inst);
  ak_form_free(map_form);
  AK24_TEST_PASS();
}

static int test_root_form_ctx_factory(void) {
  root_form_ctx_t *root = ak_root_form_ctx_new();
  AK24_TEST_ASSERT_NOT_NULL(root);
  AK24_TEST_ASSERT_NOT_NULL(root->ctx);

  ak_form_t *i32_1 = ak_root_form_ctx_get_i32(root);
  AK24_TEST_ASSERT_NOT_NULL(i32_1);
  AK24_TEST_ASSERT_EQ(i32_1->kind, AK_FORM_PRIMITIVE);
  AK24_TEST_ASSERT_EQ(i32_1->data.primitive, AK_FORM_PRIMITIVE_I32);

  ak_form_t *i32_2 = ak_root_form_ctx_get_i32(root);
  AK24_TEST_ASSERT_EQ(i32_1, i32_2);

  ak_form_t *u32 = ak_root_form_ctx_get_u32(root);
  AK24_TEST_ASSERT_NOT_NULL(u32);
  AK24_TEST_ASSERT_NEQ(i32_1, u32);

  ak_root_form_ctx_free(root);
  AK24_TEST_PASS();
}

static int test_context_stores_forms(void) {
  ak_context_t *ctx = ak_context_new();
  AK24_TEST_ASSERT_NOT_NULL(ctx);

  ak_form_t *i32 = ak_form_new_primitive(AK_FORM_PRIMITIVE_I32);
  AK24_TEST_ASSERT_NOT_NULL(i32);

  int result = ak_form_register(ctx, "my_i32", i32);
  AK24_TEST_ASSERT_EQ(result, 0);

  ak_form_t *retrieved = ak_form_lookup(ctx, "my_i32");
  AK24_TEST_ASSERT_EQ(retrieved, i32);

  ak_context_free(ctx);
  ak_form_free(i32);

  AK24_TEST_PASS();
}

int main(void) {
  ak_kernel_init("ak24-test");

  AK24_TEST_RUN(test_primitive_forms_not_singleton);
  AK24_TEST_RUN(test_all_primitive_forms);
  AK24_TEST_RUN(test_root_form_ctx_factory);
  AK24_TEST_RUN(test_context_stores_forms);
  AK24_TEST_RUN(test_optional_with_builtin_affects);
  AK24_TEST_RUN(test_repeatable_with_builtin_affects);
  AK24_TEST_RUN(test_list_with_builtin_affects);
  AK24_TEST_RUN(test_map_with_builtin_affects);

  ak_kernel_deinit();
  return 0;
}
