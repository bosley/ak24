#include "forms_primitives.h"
#include "kernel.h"
#include <string.h>

static ak_form_t *primitives[11] = {NULL};

ak_form_t *ak_primitive_bool(void) {
  if (!primitives[0]) {
    primitives[0] = ak_form_new_primitive(AK24_ATOM_U8);
  }
  return primitives[0];
}

ak_form_t *ak_primitive_u8(void) {
  if (!primitives[1]) {
    primitives[1] = ak_form_new_primitive(AK24_ATOM_U8);
  }
  return primitives[1];
}

ak_form_t *ak_primitive_u16(void) {
  if (!primitives[2]) {
    primitives[2] = ak_form_new_primitive(AK24_ATOM_U16);
  }
  return primitives[2];
}

ak_form_t *ak_primitive_u32(void) {
  if (!primitives[3]) {
    primitives[3] = ak_form_new_primitive(AK24_ATOM_U32);
  }
  return primitives[3];
}

ak_form_t *ak_primitive_u64(void) {
  if (!primitives[4]) {
    primitives[4] = ak_form_new_primitive(AK24_ATOM_U64);
  }
  return primitives[4];
}

ak_form_t *ak_primitive_i8(void) {
  if (!primitives[5]) {
    primitives[5] = ak_form_new_primitive(AK24_ATOM_I8);
  }
  return primitives[5];
}

ak_form_t *ak_primitive_i16(void) {
  if (!primitives[6]) {
    primitives[6] = ak_form_new_primitive(AK24_ATOM_I16);
  }
  return primitives[6];
}

ak_form_t *ak_primitive_i32(void) {
  if (!primitives[7]) {
    primitives[7] = ak_form_new_primitive(AK24_ATOM_I32);
  }
  return primitives[7];
}

ak_form_t *ak_primitive_i64(void) {
  if (!primitives[8]) {
    primitives[8] = ak_form_new_primitive(AK24_ATOM_I64);
  }
  return primitives[8];
}

ak_form_t *ak_primitive_f32(void) {
  if (!primitives[9]) {
    primitives[9] = ak_form_new_primitive(AK24_ATOM_F32);
  }
  return primitives[9];
}

ak_form_t *ak_primitive_f64(void) {
  if (!primitives[10]) {
    primitives[10] = ak_form_new_primitive(AK24_ATOM_F64);
  }
  return primitives[10];
}

typedef struct {
  void *result;
} affect_result_t;

static void builtin_is_none_fn(void *captured_ctx, void *invoke_args) {
  affect_result_t *result = (affect_result_t *)captured_ctx;
  ak_optional_instance_t *inst = (ak_optional_instance_t *)invoke_args;
  int *val = AK24_ALLOC(sizeof(int));
  *val = !inst->is_present;
  result->result = val;
}

static void builtin_is_some_fn(void *captured_ctx, void *invoke_args) {
  affect_result_t *result = (affect_result_t *)captured_ctx;
  ak_optional_instance_t *inst = (ak_optional_instance_t *)invoke_args;
  int *val = AK24_ALLOC(sizeof(int));
  *val = inst->is_present;
  result->result = val;
}

static void builtin_append_fn(void *captured_ctx, void *invoke_args) {
  (void)captured_ctx;
  void **args = (void **)invoke_args;
  ak_repeatable_instance_t *inst = (ak_repeatable_instance_t *)args[0];
  void *value = args[1];
  list_push(&inst->items, value);
}

static void builtin_pop_fn(void *captured_ctx, void *invoke_args) {
  affect_result_t *result = (affect_result_t *)captured_ctx;
  ak_repeatable_instance_t *inst = (ak_repeatable_instance_t *)invoke_args;
  if (list_count(&inst->items) > 0) {
    void **item = (void **)list_pop(&inst->items);
    result->result = item ? *item : NULL;
  } else {
    result->result = NULL;
  }
}

static void builtin_push_fn(void *captured_ctx, void *invoke_args) {
  (void)captured_ctx;
  void **args = (void **)invoke_args;
  ak_repeatable_instance_t *inst = (ak_repeatable_instance_t *)args[0];
  void *value = args[1];
  list_push(&inst->items, value);
}

static void builtin_rest_fn(void *captured_ctx, void *invoke_args) {
  affect_result_t *result = (affect_result_t *)captured_ctx;
  ak_repeatable_instance_t *inst = (ak_repeatable_instance_t *)invoke_args;

  ak_repeatable_instance_t *rest_inst =
      AK24_ALLOC(sizeof(ak_repeatable_instance_t));
  rest_inst->form = inst->form;
  list_init(&rest_inst->items);

  unsigned count = list_count(&inst->items);
  for (unsigned i = 1; i < count; i++) {
    void **item = (void **)list_get(&inst->items, i);
    if (item) {
      list_push(&rest_inst->items, *item);
    }
  }

  result->result = rest_inst;
}

static void builtin_list_index_fn(void *captured_ctx, void *invoke_args) {
  affect_result_t *result = (affect_result_t *)captured_ctx;
  void **args = (void **)invoke_args;
  ak_list_instance_t *inst = (ak_list_instance_t *)args[0];
  unsigned index = *(unsigned *)args[1];

  if (index < list_count(&inst->items)) {
    void **item = (void **)list_get(&inst->items, index);
    result->result = item ? *item : NULL;
  } else {
    result->result = NULL;
  }
}

static void builtin_list_length_fn(void *captured_ctx, void *invoke_args) {
  affect_result_t *result = (affect_result_t *)captured_ctx;
  ak_list_instance_t *inst = (ak_list_instance_t *)invoke_args;
  unsigned *len = AK24_ALLOC(sizeof(unsigned));
  *len = list_count(&inst->items);
  result->result = len;
}

static void builtin_map_get_fn(void *captured_ctx, void *invoke_args) {
  affect_result_t *result = (affect_result_t *)captured_ctx;
  void **args = (void **)invoke_args;
  ak_map_instance_t *inst = (ak_map_instance_t *)args[0];
  void *key = args[1];

  void **value = (void **)map_get_generic(&inst->items, &key);
  result->result = value ? *value : NULL;
}

static void builtin_map_set_fn(void *captured_ctx, void *invoke_args) {
  (void)captured_ctx;
  void **args = (void **)invoke_args;
  ak_map_instance_t *inst = (ak_map_instance_t *)args[0];
  void *key = args[1];
  void *value = args[2];

  map_set_generic(&inst->items, &key, value);
}

static void builtin_map_has_fn(void *captured_ctx, void *invoke_args) {
  affect_result_t *result = (affect_result_t *)captured_ctx;
  void **args = (void **)invoke_args;
  ak_map_instance_t *inst = (ak_map_instance_t *)args[0];
  void *key = args[1];

  void **value = (void **)map_get_generic(&inst->items, &key);
  int *has = AK24_ALLOC(sizeof(int));
  *has = (value != NULL);
  result->result = has;
}

ak_lambda_t *ak_builtin_is_none(void) {
  return ak_lambda_new(builtin_is_none_fn, NULL, NULL);
}

ak_lambda_t *ak_builtin_is_some(void) {
  return ak_lambda_new(builtin_is_some_fn, NULL, NULL);
}

ak_lambda_t *ak_builtin_append(void) {
  return ak_lambda_new(builtin_append_fn, NULL, NULL);
}

ak_lambda_t *ak_builtin_pop(void) {
  return ak_lambda_new(builtin_pop_fn, NULL, NULL);
}

ak_lambda_t *ak_builtin_push(void) {
  return ak_lambda_new(builtin_push_fn, NULL, NULL);
}

ak_lambda_t *ak_builtin_rest(void) {
  return ak_lambda_new(builtin_rest_fn, NULL, NULL);
}

ak_lambda_t *ak_builtin_list_index(void) {
  return ak_lambda_new(builtin_list_index_fn, NULL, NULL);
}

ak_lambda_t *ak_builtin_list_length(void) {
  return ak_lambda_new(builtin_list_length_fn, NULL, NULL);
}

ak_lambda_t *ak_builtin_map_get(void) {
  return ak_lambda_new(builtin_map_get_fn, NULL, NULL);
}

ak_lambda_t *ak_builtin_map_set(void) {
  return ak_lambda_new(builtin_map_set_fn, NULL, NULL);
}

ak_lambda_t *ak_builtin_map_has(void) {
  return ak_lambda_new(builtin_map_has_fn, NULL, NULL);
}

void ak_form_attach_builtin_affects(ak_form_t *form) {
  if (!form) {
    return;
  }

  ak_form_t *current = form;
  if (current->kind == AK_FORM_NAMED) {
    current = current->data.named.actual_form;
  }

  switch (current->kind) {
  case AK_FORM_OPTIONAL: {
    ak_affect_t *is_none =
        ak_affect_new("is_none", ak_builtin_is_none(), NULL, NULL);
    ak_affect_t *is_some =
        ak_affect_new("is_some", ak_builtin_is_some(), NULL, NULL);
    ak_form_add_affect(form, is_none);
    ak_form_add_affect(form, is_some);
    break;
  }

  case AK_FORM_REPEATABLE: {
    ak_affect_t *append =
        ak_affect_new("append", ak_builtin_append(), NULL, NULL);
    ak_affect_t *pop = ak_affect_new("pop", ak_builtin_pop(), NULL, NULL);
    ak_affect_t *push = ak_affect_new("push", ak_builtin_push(), NULL, NULL);
    ak_affect_t *rest = ak_affect_new("rest", ak_builtin_rest(), NULL, NULL);
    ak_form_add_affect(form, append);
    ak_form_add_affect(form, pop);
    ak_form_add_affect(form, push);
    ak_form_add_affect(form, rest);
    break;
  }

  case AK_FORM_LIST: {
    ak_affect_t *index =
        ak_affect_new("index", ak_builtin_list_index(), NULL, NULL);
    ak_affect_t *length =
        ak_affect_new("length", ak_builtin_list_length(), NULL, NULL);
    ak_form_add_affect(form, index);
    ak_form_add_affect(form, length);
    break;
  }

  case AK_FORM_MAP: {
    ak_affect_t *get = ak_affect_new("get", ak_builtin_map_get(), NULL, NULL);
    ak_affect_t *set = ak_affect_new("set", ak_builtin_map_set(), NULL, NULL);
    ak_affect_t *has = ak_affect_new("has", ak_builtin_map_has(), NULL, NULL);
    ak_form_add_affect(form, get);
    ak_form_add_affect(form, set);
    ak_form_add_affect(form, has);
    break;
  }

  default:
    break;
  }
}

ak_optional_instance_t *ak_optional_instance_new(ak_form_t *form) {
  ak_optional_instance_t *inst = AK24_ALLOC(sizeof(ak_optional_instance_t));
  if (!inst) {
    return NULL;
  }
  inst->form = form;
  inst->is_present = 0;
  inst->value = NULL;
  return inst;
}

void ak_optional_instance_free(ak_optional_instance_t *inst) {
  if (!inst) {
    return;
  }
  AK24_FREE(inst);
}

void ak_optional_instance_set(ak_optional_instance_t *inst, void *value) {
  if (!inst) {
    return;
  }
  inst->is_present = 1;
  inst->value = value;
}

void *ak_optional_instance_get(ak_optional_instance_t *inst) {
  if (!inst || !inst->is_present) {
    return NULL;
  }
  return inst->value;
}

ak_repeatable_instance_t *ak_repeatable_instance_new(ak_form_t *form) {
  ak_repeatable_instance_t *inst = AK24_ALLOC(sizeof(ak_repeatable_instance_t));
  if (!inst) {
    return NULL;
  }
  inst->form = form;
  list_init(&inst->items);
  return inst;
}

void ak_repeatable_instance_free(ak_repeatable_instance_t *inst) {
  if (!inst) {
    return;
  }
  list_deinit(&inst->items);
  AK24_FREE(inst);
}

ak_list_instance_t *ak_list_instance_new(ak_form_t *form) {
  ak_list_instance_t *inst = AK24_ALLOC(sizeof(ak_list_instance_t));
  if (!inst) {
    return NULL;
  }
  inst->form = form;
  list_init(&inst->items);
  return inst;
}

void ak_list_instance_free(ak_list_instance_t *inst) {
  if (!inst) {
    return;
  }
  list_deinit(&inst->items);
  AK24_FREE(inst);
}

ak_map_instance_t *ak_map_instance_new(ak_form_t *form) {
  ak_map_instance_t *inst = AK24_ALLOC(sizeof(ak_map_instance_t));
  if (!inst) {
    return NULL;
  }
  inst->form = form;
  map_init_generic(&inst->items, sizeof(void *), map_hash_u64, map_cmp_mem);
  return inst;
}

void ak_map_instance_free(ak_map_instance_t *inst) {
  if (!inst) {
    return;
  }
  map_deinit(&inst->items);
  AK24_FREE(inst);
}
