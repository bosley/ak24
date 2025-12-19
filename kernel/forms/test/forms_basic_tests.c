#include "forms.h"
#include "kernel.h"
#include "test/assert.h"

static int test_primitive_form_creation(void) {
  ak_form_t *form = ak_form_new_primitive(AK24_ATOM_I32);
  AK24_TEST_ASSERT_NOT_NULL(form);
  AK24_TEST_ASSERT_EQ(form->kind, AK_FORM_PRIMITIVE);
  AK24_TEST_ASSERT_EQ(form->data.primitive, AK24_ATOM_I32);
  ak_form_free(form);
  AK24_TEST_PASS();
}

static int test_compound_form_creation(void) {
  ak_form_t *i32_form = ak_form_new_primitive(AK24_ATOM_I32);
  ak_form_t *f64_form = ak_form_new_primitive(AK24_ATOM_F64);

  list_void_t parts;
  list_init(&parts);
  list_push(&parts, i32_form);
  list_push(&parts, f64_form);

  ak_form_t *compound = ak_form_new_compound(&parts);
  AK24_TEST_ASSERT_NOT_NULL(compound);
  AK24_TEST_ASSERT_EQ(compound->kind, AK_FORM_COMPOUND);
  AK24_TEST_ASSERT_EQ(list_count(&compound->data.compound_parts), 2);

  list_deinit(&parts);
  ak_form_free(compound);
  AK24_TEST_PASS();
}

static int test_optional_form_creation(void) {
  ak_form_t *inner = ak_form_new_primitive(AK24_ATOM_I32);
  ak_form_t *optional = ak_form_new_optional(inner);

  AK24_TEST_ASSERT_NOT_NULL(optional);
  AK24_TEST_ASSERT_EQ(optional->kind, AK_FORM_OPTIONAL);
  AK24_TEST_ASSERT_EQ(optional->data.optional.inner, inner);

  ak_form_free(optional);
  AK24_TEST_PASS();
}

static int test_repeatable_form_creation(void) {
  ak_form_t *inner = ak_form_new_primitive(AK24_ATOM_I8);
  ak_form_t *repeatable = ak_form_new_repeatable(inner);

  AK24_TEST_ASSERT_NOT_NULL(repeatable);
  AK24_TEST_ASSERT_EQ(repeatable->kind, AK_FORM_REPEATABLE);
  AK24_TEST_ASSERT_EQ(repeatable->data.repeatable.inner, inner);

  ak_form_free(repeatable);
  AK24_TEST_PASS();
}

static int test_struct_form_creation(void) {
  ak_form_t *i32_form = ak_form_new_primitive(AK24_ATOM_I32);
  ak_form_t *i64_form = ak_form_new_primitive(AK24_ATOM_I64);

  list_void_t parts;
  list_init(&parts);
  list_push(&parts, i32_form);
  list_push(&parts, i64_form);

  ak_form_t *pattern = ak_form_new_compound(&parts);

  list_str_t fields;
  list_init(&fields);
  list_push(&fields, "name");
  list_push(&fields, "age");

  ak_form_t *struct_form = ak_form_new_struct(pattern, &fields);

  AK24_TEST_ASSERT_NOT_NULL(struct_form);
  AK24_TEST_ASSERT_EQ(struct_form->kind, AK_FORM_STRUCT);
  AK24_TEST_ASSERT_EQ(list_count(&struct_form->data.struct_form.field_names),
                      2);

  list_deinit(&parts);
  list_deinit(&fields);
  ak_form_free(struct_form);
  AK24_TEST_PASS();
}

static int test_list_form_creation(void) {
  ak_form_t *element = ak_form_new_primitive(AK24_ATOM_I32);
  ak_form_t *list_form = ak_form_new_list(element);

  AK24_TEST_ASSERT_NOT_NULL(list_form);
  AK24_TEST_ASSERT_EQ(list_form->kind, AK_FORM_LIST);
  AK24_TEST_ASSERT_EQ(list_form->data.list_form.element_type, element);

  ak_form_free(list_form);
  AK24_TEST_PASS();
}

static int test_map_form_creation(void) {
  ak_form_t *value_type = ak_form_new_primitive(AK24_ATOM_I32);
  ak_form_t *map_form = ak_form_new_map(AK24_ATOM_U32, value_type);

  AK24_TEST_ASSERT_NOT_NULL(map_form);
  AK24_TEST_ASSERT_EQ(map_form->kind, AK_FORM_MAP);
  AK24_TEST_ASSERT_EQ(map_form->data.map_form.key_type, AK24_ATOM_U32);
  AK24_TEST_ASSERT_EQ(map_form->data.map_form.value_type, value_type);

  ak_form_free(map_form);
  AK24_TEST_PASS();
}

static int test_named_form_creation(void) {
  ak_form_t *inner = ak_form_new_primitive(AK24_ATOM_I32);
  ak_form_t *named = ak_form_new_named("my_type", inner);

  AK24_TEST_ASSERT_NOT_NULL(named);
  AK24_TEST_ASSERT_EQ(named->kind, AK_FORM_NAMED);
  AK24_TEST_ASSERT_STR_EQ(named->name, "my_type");
  AK24_TEST_ASSERT_EQ(named->data.named.actual_form, inner);

  ak_form_free(named);
  AK24_TEST_PASS();
}

static int test_form_registration_and_lookup(void) {
  ak_context_t *ctx = ak_context_new();
  ak_form_t *form = ak_form_new_primitive(AK24_ATOM_I32);

  int result = ak_form_register(ctx, "my_form", form);
  AK24_TEST_ASSERT_EQ(result, 0);

  ak_form_t *found = ak_form_lookup(ctx, "my_form");
  AK24_TEST_ASSERT_EQ(found, form);

  ak_context_free(ctx);
  AK24_TEST_PASS();
}

static int test_form_lookup_with_scoping(void) {
  ak_context_t *parent = ak_context_new();
  ak_form_t *parent_form = ak_form_new_primitive(AK24_ATOM_I32);
  ak_form_register(parent, "shared", parent_form);

  ak_context_t *child = ak_context_push(parent);
  ak_form_t *child_form = ak_form_new_primitive(AK24_ATOM_F64);
  ak_form_register(child, "local", child_form);

  ak_form_t *found_shared = ak_form_lookup(child, "shared");
  AK24_TEST_ASSERT_EQ(found_shared, parent_form);

  ak_form_t *found_local = ak_form_lookup(child, "local");
  AK24_TEST_ASSERT_EQ(found_local, child_form);

  ak_form_t *not_in_parent = ak_form_lookup(parent, "local");
  AK24_TEST_ASSERT_NULL(not_in_parent);

  child = ak_context_pop(child);
  ak_context_free(parent);
  AK24_TEST_PASS();
}

static int test_structural_equality_primitives(void) {
  ak_form_t *form1 = ak_form_new_primitive(AK24_ATOM_I32);
  ak_form_t *form2 = ak_form_new_primitive(AK24_ATOM_I32);
  ak_form_t *form3 = ak_form_new_primitive(AK24_ATOM_F64);

  AK24_TEST_ASSERT_EQ(ak_form_is_compatible(form1, form2), 1);
  AK24_TEST_ASSERT_EQ(ak_form_is_compatible(form1, form3), 0);

  ak_form_free(form1);
  ak_form_free(form2);
  ak_form_free(form3);
  AK24_TEST_PASS();
}

static int test_structural_equality_compound(void) {
  ak_form_t *i32_1 = ak_form_new_primitive(AK24_ATOM_I32);
  ak_form_t *i32_2 = ak_form_new_primitive(AK24_ATOM_I32);
  ak_form_t *f64_1 = ak_form_new_primitive(AK24_ATOM_F64);
  ak_form_t *f64_2 = ak_form_new_primitive(AK24_ATOM_F64);

  list_void_t parts1;
  list_init(&parts1);
  list_push(&parts1, i32_1);
  list_push(&parts1, f64_1);

  list_void_t parts2;
  list_init(&parts2);
  list_push(&parts2, i32_2);
  list_push(&parts2, f64_2);

  ak_form_t *compound1 = ak_form_new_compound(&parts1);
  ak_form_t *compound2 = ak_form_new_compound(&parts2);

  AK24_TEST_ASSERT_EQ(ak_form_is_compatible(compound1, compound2), 1);

  list_deinit(&parts1);
  list_deinit(&parts2);
  ak_form_free(compound1);
  ak_form_free(compound2);
  AK24_TEST_PASS();
}

static int test_affect_creation(void) {
  list_void_t params;
  list_init(&params);

  ak_form_t *param_form = ak_form_new_primitive(AK24_ATOM_I32);
  list_push(&params, param_form);

  ak_form_t *return_form = ak_form_new_primitive(AK24_ATOM_I32);

  ak_affect_t *affect = ak_affect_new("my_method", NULL, &params, return_form);

  AK24_TEST_ASSERT_NOT_NULL(affect);
  AK24_TEST_ASSERT_STR_EQ(affect->name, "my_method");
  AK24_TEST_ASSERT_EQ(list_count(&affect->parameter_forms), 1);
  AK24_TEST_ASSERT_EQ(affect->return_form, return_form);

  list_deinit(&params);
  ak_affect_free(affect);
  ak_form_free(param_form);
  ak_form_free(return_form);
  AK24_TEST_PASS();
}

static int test_form_add_affect(void) {
  ak_form_t *form = ak_form_new_primitive(AK24_ATOM_I32);
  ak_affect_t *affect = ak_affect_new("method", NULL, NULL, NULL);

  int result = ak_form_add_affect(form, affect);
  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(list_count(&form->affects), 1);

  ak_form_free(form);
  AK24_TEST_PASS();
}

static int test_form_get_affect(void) {
  ak_form_t *form = ak_form_new_primitive(AK24_ATOM_I32);
  ak_affect_t *affect = ak_affect_new("my_method", NULL, NULL, NULL);

  ak_form_add_affect(form, affect);

  ak_affect_t *found = ak_form_get_affect(form, "my_method");
  AK24_TEST_ASSERT_EQ(found, affect);

  ak_affect_t *not_found = ak_form_get_affect(form, "other_method");
  AK24_TEST_ASSERT_NULL(not_found);

  ak_form_free(form);
  AK24_TEST_PASS();
}

static int test_optional_affordances(void) {
  ak_form_t *inner = ak_form_new_primitive(AK24_ATOM_I32);
  ak_form_t *optional = ak_form_new_optional(inner);

  AK24_TEST_ASSERT_EQ(ak_form_has_affordance(optional, "is_none"), 1);
  AK24_TEST_ASSERT_EQ(ak_form_has_affordance(optional, "is_some"), 1);
  AK24_TEST_ASSERT_EQ(ak_form_has_affordance(optional, "invalid"), 0);

  ak_form_free(optional);
  AK24_TEST_PASS();
}

static int test_repeatable_affordances(void) {
  ak_form_t *inner = ak_form_new_primitive(AK24_ATOM_I8);
  ak_form_t *repeatable = ak_form_new_repeatable(inner);

  AK24_TEST_ASSERT_EQ(ak_form_has_affordance(repeatable, "append"), 1);
  AK24_TEST_ASSERT_EQ(ak_form_has_affordance(repeatable, "pop"), 1);
  AK24_TEST_ASSERT_EQ(ak_form_has_affordance(repeatable, "push"), 1);
  AK24_TEST_ASSERT_EQ(ak_form_has_affordance(repeatable, "rest"), 1);
  AK24_TEST_ASSERT_EQ(ak_form_has_affordance(repeatable, "iterate"), 1);

  ak_form_free(repeatable);
  AK24_TEST_PASS();
}

static int test_list_affordances(void) {
  ak_form_t *element = ak_form_new_primitive(AK24_ATOM_I32);
  ak_form_t *list_form = ak_form_new_list(element);

  AK24_TEST_ASSERT_EQ(ak_form_has_affordance(list_form, "index"), 1);
  AK24_TEST_ASSERT_EQ(ak_form_has_affordance(list_form, "iterate"), 1);
  AK24_TEST_ASSERT_EQ(ak_form_has_affordance(list_form, "length"), 1);

  ak_form_free(list_form);
  AK24_TEST_PASS();
}

static int test_map_affordances(void) {
  ak_form_t *value_type = ak_form_new_primitive(AK24_ATOM_I32);
  ak_form_t *map_form = ak_form_new_map(AK24_ATOM_U32, value_type);

  AK24_TEST_ASSERT_EQ(ak_form_has_affordance(map_form, "get"), 1);
  AK24_TEST_ASSERT_EQ(ak_form_has_affordance(map_form, "set"), 1);
  AK24_TEST_ASSERT_EQ(ak_form_has_affordance(map_form, "has"), 1);
  AK24_TEST_ASSERT_EQ(ak_form_has_affordance(map_form, "iterate"), 1);

  ak_form_free(map_form);
  AK24_TEST_PASS();
}

static int test_struct_field_affordances(void) {
  ak_form_t *pattern = ak_form_new_primitive(AK24_ATOM_I32);

  list_str_t fields;
  list_init(&fields);
  list_push(&fields, "name");
  list_push(&fields, "age");

  ak_form_t *struct_form = ak_form_new_struct(pattern, &fields);

  AK24_TEST_ASSERT_EQ(ak_form_has_affordance(struct_form, "name"), 1);
  AK24_TEST_ASSERT_EQ(ak_form_has_affordance(struct_form, "age"), 1);
  AK24_TEST_ASSERT_EQ(ak_form_has_affordance(struct_form, "invalid"), 0);

  list_deinit(&fields);
  ak_form_free(struct_form);
  AK24_TEST_PASS();
}

static int test_get_affordances_list(void) {
  ak_form_t *inner = ak_form_new_primitive(AK24_ATOM_I32);
  ak_form_t *optional = ak_form_new_optional(inner);

  list_str_t affordances = ak_form_get_affordances(optional);
  AK24_TEST_ASSERT_EQ(list_count(&affordances), 2);

  list_deinit(&affordances);
  ak_form_free(optional);
  AK24_TEST_PASS();
}

static int test_affect_also_includes(void) {
  ak_affect_t *affect = ak_affect_new("method", NULL, NULL, NULL);

  int result = ak_affect_add_also(affect, "other_affect");
  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(list_count(&affect->also_includes), 1);

  ak_affect_free(affect);
  AK24_TEST_PASS();
}

typedef struct {
  ak_form_t *form;
  list_int_t values;
} i32_repeatable_instance_t;

typedef struct {
  void *result;
} affect_result_t;

static void append_i32_affect(void *captured_ctx, void *invoke_args) {
  (void)captured_ctx;
  void **args = (void **)invoke_args;
  i32_repeatable_instance_t *inst = (i32_repeatable_instance_t *)args[0];
  int val = *(int *)args[1];
  list_push(&inst->values, val);
}

static void pop_i32_affect(void *captured_ctx, void *invoke_args) {
  affect_result_t *result = (affect_result_t *)captured_ctx;
  i32_repeatable_instance_t *inst = (i32_repeatable_instance_t *)invoke_args;
  if (list_count(&inst->values) > 0) {
    result->result = list_pop(&inst->values);
  } else {
    result->result = NULL;
  }
}

static void length_i32_affect(void *captured_ctx, void *invoke_args) {
  affect_result_t *result = (affect_result_t *)captured_ctx;
  i32_repeatable_instance_t *inst = (i32_repeatable_instance_t *)invoke_args;
  int *len = AK24_ALLOC(sizeof(int));
  *len = list_count(&inst->values);
  result->result = len;
}

static int test_repeatable_form_with_real_affects(void) {
  ak_form_t *i32_form = ak_form_new_primitive(AK24_ATOM_I32);
  ak_form_t *repeatable = ak_form_new_repeatable(i32_form);

  ak_lambda_t *append_lambda = ak_lambda_new(append_i32_affect, NULL, NULL);
  ak_lambda_t *pop_lambda = ak_lambda_new(pop_i32_affect, NULL, NULL);
  ak_lambda_t *length_lambda = ak_lambda_new(length_i32_affect, NULL, NULL);

  ak_affect_t *append_affect =
      ak_affect_new("append", append_lambda, NULL, NULL);
  ak_affect_t *pop_affect = ak_affect_new("pop", pop_lambda, NULL, NULL);
  ak_affect_t *length_affect =
      ak_affect_new("length", length_lambda, NULL, NULL);

  ak_form_add_affect(repeatable, append_affect);
  ak_form_add_affect(repeatable, pop_affect);
  ak_form_add_affect(repeatable, length_affect);

  i32_repeatable_instance_t instance;
  instance.form = repeatable;
  list_init(&instance.values);

  ak_affect_t *append = ak_form_get_affect(repeatable, "append");
  AK24_TEST_ASSERT_NOT_NULL(append);
  AK24_TEST_ASSERT_NOT_NULL(append->lambda);

  int val1 = 42;
  int val2 = 100;
  int val3 = 999;

  void *args1[] = {&instance, &val1};
  void *args2[] = {&instance, &val2};
  void *args3[] = {&instance, &val3};

  ak_lambda_invoke(append->lambda, args1);
  ak_lambda_invoke(append->lambda, args2);
  ak_lambda_invoke(append->lambda, args3);

  AK24_TEST_ASSERT_EQ(list_count(&instance.values), 3);
  int *retrieved = list_get(&instance.values, 0);
  AK24_TEST_ASSERT_EQ(*retrieved, 42);
  retrieved = list_get(&instance.values, 1);
  AK24_TEST_ASSERT_EQ(*retrieved, 100);
  retrieved = list_get(&instance.values, 2);
  AK24_TEST_ASSERT_EQ(*retrieved, 999);

  ak_affect_t *pop = ak_form_get_affect(repeatable, "pop");
  affect_result_t pop_result = {0};
  ak_lambda_set_context(pop->lambda, &pop_result, NULL);
  ak_lambda_invoke(pop->lambda, &instance);

  int *popped = (int *)pop_result.result;
  AK24_TEST_ASSERT_NOT_NULL(popped);
  AK24_TEST_ASSERT_EQ(*popped, 999);
  AK24_TEST_ASSERT_EQ(list_count(&instance.values), 2);

  ak_affect_t *length = ak_form_get_affect(repeatable, "length");
  affect_result_t length_result = {0};
  ak_lambda_set_context(length->lambda, &length_result, NULL);
  ak_lambda_invoke(length->lambda, &instance);

  int *len = (int *)length_result.result;
  AK24_TEST_ASSERT_NOT_NULL(len);
  AK24_TEST_ASSERT_EQ(*len, 2);
  AK24_FREE(len);

  list_deinit(&instance.values);
  ak_form_free(repeatable);
  AK24_TEST_PASS();
}

typedef struct {
  ak_form_t *form;
  int is_present;
  int value;
} i32_optional_instance_t;

static void is_none_affect(void *captured_ctx, void *invoke_args) {
  affect_result_t *result = (affect_result_t *)captured_ctx;
  i32_optional_instance_t *inst = (i32_optional_instance_t *)invoke_args;
  int *val = AK24_ALLOC(sizeof(int));
  *val = !inst->is_present;
  result->result = val;
}

static void is_some_affect(void *captured_ctx, void *invoke_args) {
  affect_result_t *result = (affect_result_t *)captured_ctx;
  i32_optional_instance_t *inst = (i32_optional_instance_t *)invoke_args;
  int *val = AK24_ALLOC(sizeof(int));
  *val = inst->is_present;
  result->result = val;
}

static void get_value_affect(void *captured_ctx, void *invoke_args) {
  affect_result_t *result = (affect_result_t *)captured_ctx;
  i32_optional_instance_t *inst = (i32_optional_instance_t *)invoke_args;
  if (inst->is_present) {
    int *val = AK24_ALLOC(sizeof(int));
    *val = inst->value;
    result->result = val;
  } else {
    result->result = NULL;
  }
}

static int test_optional_form_with_real_affects(void) {
  ak_form_t *i32_form = ak_form_new_primitive(AK24_ATOM_I32);
  ak_form_t *optional = ak_form_new_optional(i32_form);

  ak_lambda_t *is_none_lambda = ak_lambda_new(is_none_affect, NULL, NULL);
  ak_lambda_t *is_some_lambda = ak_lambda_new(is_some_affect, NULL, NULL);
  ak_lambda_t *get_value_lambda = ak_lambda_new(get_value_affect, NULL, NULL);

  ak_affect_t *is_none = ak_affect_new("is_none", is_none_lambda, NULL, NULL);
  ak_affect_t *is_some = ak_affect_new("is_some", is_some_lambda, NULL, NULL);
  ak_affect_t *get_value =
      ak_affect_new("get_value", get_value_lambda, NULL, NULL);

  ak_form_add_affect(optional, is_none);
  ak_form_add_affect(optional, is_some);
  ak_form_add_affect(optional, get_value);

  i32_optional_instance_t instance_none;
  instance_none.form = optional;
  instance_none.is_present = 0;
  instance_none.value = 0;

  ak_affect_t *is_none_aff = ak_form_get_affect(optional, "is_none");
  affect_result_t is_none_result = {0};
  ak_lambda_set_context(is_none_aff->lambda, &is_none_result, NULL);
  ak_lambda_invoke(is_none_aff->lambda, &instance_none);
  int *result = (int *)is_none_result.result;
  AK24_TEST_ASSERT_NOT_NULL(result);
  AK24_TEST_ASSERT_EQ(*result, 1);
  AK24_FREE(result);

  ak_affect_t *is_some_aff = ak_form_get_affect(optional, "is_some");
  affect_result_t is_some_result = {0};
  ak_lambda_set_context(is_some_aff->lambda, &is_some_result, NULL);
  ak_lambda_invoke(is_some_aff->lambda, &instance_none);
  result = (int *)is_some_result.result;
  AK24_TEST_ASSERT_NOT_NULL(result);
  AK24_TEST_ASSERT_EQ(*result, 0);
  AK24_FREE(result);

  i32_optional_instance_t instance_some;
  instance_some.form = optional;
  instance_some.is_present = 1;
  instance_some.value = 42;

  is_none_result.result = NULL;
  ak_lambda_invoke(is_none_aff->lambda, &instance_some);
  result = (int *)is_none_result.result;
  AK24_TEST_ASSERT_NOT_NULL(result);
  AK24_TEST_ASSERT_EQ(*result, 0);
  AK24_FREE(result);

  is_some_result.result = NULL;
  ak_lambda_invoke(is_some_aff->lambda, &instance_some);
  result = (int *)is_some_result.result;
  AK24_TEST_ASSERT_NOT_NULL(result);
  AK24_TEST_ASSERT_EQ(*result, 1);
  AK24_FREE(result);

  ak_affect_t *get_val_aff = ak_form_get_affect(optional, "get_value");
  affect_result_t get_val_result = {0};
  ak_lambda_set_context(get_val_aff->lambda, &get_val_result, NULL);
  ak_lambda_invoke(get_val_aff->lambda, &instance_some);
  result = (int *)get_val_result.result;
  AK24_TEST_ASSERT_NOT_NULL(result);
  AK24_TEST_ASSERT_EQ(*result, 42);
  AK24_FREE(result);

  get_val_result.result = NULL;
  ak_lambda_invoke(get_val_aff->lambda, &instance_none);
  result = (int *)get_val_result.result;
  AK24_TEST_ASSERT_NULL(result);

  ak_form_free(optional);
  AK24_TEST_PASS();
}

typedef struct {
  ak_form_t *form;
  char *name;
  int age;
} person_instance_t;

static void get_name_affect(void *captured_ctx, void *invoke_args) {
  affect_result_t *result = (affect_result_t *)captured_ctx;
  person_instance_t *inst = (person_instance_t *)invoke_args;
  result->result = inst->name;
}

static void get_age_affect(void *captured_ctx, void *invoke_args) {
  affect_result_t *result = (affect_result_t *)captured_ctx;
  person_instance_t *inst = (person_instance_t *)invoke_args;
  int *val = AK24_ALLOC(sizeof(int));
  *val = inst->age;
  result->result = val;
}

static void set_age_affect(void *captured_ctx, void *invoke_args) {
  (void)captured_ctx;
  void **args = (void **)invoke_args;
  person_instance_t *inst = (person_instance_t *)args[0];
  int new_age = *(int *)args[1];
  inst->age = new_age;
}

static int test_struct_form_with_field_affects(void) {
  ak_form_t *i32_form = ak_form_new_primitive(AK24_ATOM_I32);
  ak_form_t *str_form =
      ak_form_new_repeatable(ak_form_new_primitive(AK24_ATOM_I8));

  list_void_t parts;
  list_init(&parts);
  list_push(&parts, str_form);
  list_push(&parts, i32_form);

  ak_form_t *pattern = ak_form_new_compound(&parts);

  list_str_t fields;
  list_init(&fields);
  list_push(&fields, "name");
  list_push(&fields, "age");

  ak_form_t *person_form = ak_form_new_struct(pattern, &fields);

  ak_lambda_t *get_name_lambda = ak_lambda_new(get_name_affect, NULL, NULL);
  ak_lambda_t *get_age_lambda = ak_lambda_new(get_age_affect, NULL, NULL);
  ak_lambda_t *set_age_lambda = ak_lambda_new(set_age_affect, NULL, NULL);

  ak_affect_t *get_name = ak_affect_new("name", get_name_lambda, NULL, NULL);
  ak_affect_t *get_age = ak_affect_new("age", get_age_lambda, NULL, NULL);
  ak_affect_t *set_age = ak_affect_new("set_age", set_age_lambda, NULL, NULL);

  ak_form_add_affect(person_form, get_name);
  ak_form_add_affect(person_form, get_age);
  ak_form_add_affect(person_form, set_age);

  person_instance_t alice;
  alice.form = person_form;
  alice.name = "Alice";
  alice.age = 30;

  ak_affect_t *name_aff = ak_form_get_affect(person_form, "name");
  affect_result_t name_result = {0};
  ak_lambda_set_context(name_aff->lambda, &name_result, NULL);
  ak_lambda_invoke(name_aff->lambda, &alice);
  char *name = (char *)name_result.result;
  AK24_TEST_ASSERT_NOT_NULL(name);
  AK24_TEST_ASSERT_STR_EQ(name, "Alice");

  ak_affect_t *age_aff = ak_form_get_affect(person_form, "age");
  affect_result_t age_result = {0};
  ak_lambda_set_context(age_aff->lambda, &age_result, NULL);
  ak_lambda_invoke(age_aff->lambda, &alice);
  int *age = (int *)age_result.result;
  AK24_TEST_ASSERT_NOT_NULL(age);
  AK24_TEST_ASSERT_EQ(*age, 30);
  AK24_FREE(age);

  ak_affect_t *set_age_aff = ak_form_get_affect(person_form, "set_age");
  int new_age = 31;
  void *set_args[] = {&alice, &new_age};
  ak_lambda_invoke(set_age_aff->lambda, set_args);
  AK24_TEST_ASSERT_EQ(alice.age, 31);

  age_result.result = NULL;
  ak_lambda_invoke(age_aff->lambda, &alice);
  age = (int *)age_result.result;
  AK24_TEST_ASSERT_NOT_NULL(age);
  AK24_TEST_ASSERT_EQ(*age, 31);
  AK24_FREE(age);

  list_deinit(&parts);
  list_deinit(&fields);
  ak_form_free(person_form);
  AK24_TEST_PASS();
}

int main(void) {
  ak_kernel_init();

  AK24_TEST_RUN(test_primitive_form_creation);
  AK24_TEST_RUN(test_compound_form_creation);
  AK24_TEST_RUN(test_optional_form_creation);
  AK24_TEST_RUN(test_repeatable_form_creation);
  AK24_TEST_RUN(test_struct_form_creation);
  AK24_TEST_RUN(test_list_form_creation);
  AK24_TEST_RUN(test_map_form_creation);
  AK24_TEST_RUN(test_named_form_creation);
  AK24_TEST_RUN(test_form_registration_and_lookup);
  AK24_TEST_RUN(test_form_lookup_with_scoping);
  AK24_TEST_RUN(test_structural_equality_primitives);
  AK24_TEST_RUN(test_structural_equality_compound);
  AK24_TEST_RUN(test_affect_creation);
  AK24_TEST_RUN(test_form_add_affect);
  AK24_TEST_RUN(test_form_get_affect);
  AK24_TEST_RUN(test_optional_affordances);
  AK24_TEST_RUN(test_repeatable_affordances);
  AK24_TEST_RUN(test_list_affordances);
  AK24_TEST_RUN(test_map_affordances);
  AK24_TEST_RUN(test_struct_field_affordances);
  AK24_TEST_RUN(test_get_affordances_list);
  AK24_TEST_RUN(test_affect_also_includes);
  AK24_TEST_RUN(test_repeatable_form_with_real_affects);
  AK24_TEST_RUN(test_optional_form_with_real_affects);
  AK24_TEST_RUN(test_struct_form_with_field_affects);

  ak_kernel_deinit();
  return 0;
}
