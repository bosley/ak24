#include "forms_primitives.h"
#include "kernel.h"
#include <string.h>

root_form_ctx_t *ak_root_form_ctx_new(void) {
  root_form_ctx_t *root = AK24_ALLOC(sizeof(root_form_ctx_t));
  if (!root) {
    return NULL;
  }

  root->ctx = ak_context_new();
  if (!root->ctx) {
    AK24_FREE(root);
    return NULL;
  }

  return root;
}

void ak_root_form_ctx_free(root_form_ctx_t *root) {
  if (!root) {
    return;
  }

  if (root->ctx) {
    map_iter_t iter = map_iter();
    void *key_value_pair;
    while ((key_value_pair = map_next_generic(&root->ctx->data, &iter))) {
      const char **key_ptr = (const char **)key_value_pair;
      void **value_ptr = map_get_generic(&root->ctx->data, key_ptr);
      if (value_ptr) {
        ak_form_t *form = (ak_form_t *)*value_ptr;
        ak_form_free(form);
      }
    }
    ak_context_free(root->ctx);
  }

  AK24_FREE(root);
}

static ak_form_t *root_form_ctx_get_or_create(root_form_ctx_t *root,
                                              const char *name,
                                              ak_form_primitive_type_e type) {
  if (!root || !root->ctx) {
    return NULL;
  }

  ak_form_t *form = ak_form_lookup_local(root->ctx, name);
  if (form) {
    return form;
  }

  form = ak_form_new_primitive(type);
  if (!form) {
    return NULL;
  }

  if (ak_form_register(root->ctx, name, form) != 0) {
    ak_form_free(form);
    return NULL;
  }

  return form;
}

ak_form_t *ak_root_form_ctx_get_bool(root_form_ctx_t *root) {
  return root_form_ctx_get_or_create(root, "bool", AK_FORM_PRIMITIVE_U8);
}

ak_form_t *ak_root_form_ctx_get_u8(root_form_ctx_t *root) {
  return root_form_ctx_get_or_create(root, "u8", AK_FORM_PRIMITIVE_U8);
}

ak_form_t *ak_root_form_ctx_get_u16(root_form_ctx_t *root) {
  return root_form_ctx_get_or_create(root, "u16", AK_FORM_PRIMITIVE_U16);
}

ak_form_t *ak_root_form_ctx_get_u32(root_form_ctx_t *root) {
  return root_form_ctx_get_or_create(root, "u32", AK_FORM_PRIMITIVE_U32);
}

ak_form_t *ak_root_form_ctx_get_u64(root_form_ctx_t *root) {
  return root_form_ctx_get_or_create(root, "u64", AK_FORM_PRIMITIVE_U64);
}

ak_form_t *ak_root_form_ctx_get_i8(root_form_ctx_t *root) {
  return root_form_ctx_get_or_create(root, "i8", AK_FORM_PRIMITIVE_I8);
}

ak_form_t *ak_root_form_ctx_get_i16(root_form_ctx_t *root) {
  return root_form_ctx_get_or_create(root, "i16", AK_FORM_PRIMITIVE_I16);
}

ak_form_t *ak_root_form_ctx_get_i32(root_form_ctx_t *root) {
  return root_form_ctx_get_or_create(root, "i32", AK_FORM_PRIMITIVE_I32);
}

ak_form_t *ak_root_form_ctx_get_i64(root_form_ctx_t *root) {
  return root_form_ctx_get_or_create(root, "i64", AK_FORM_PRIMITIVE_I64);
}

ak_form_t *ak_root_form_ctx_get_f32(root_form_ctx_t *root) {
  return root_form_ctx_get_or_create(root, "f32", AK_FORM_PRIMITIVE_F32);
}

ak_form_t *ak_root_form_ctx_get_f64(root_form_ctx_t *root) {
  return root_form_ctx_get_or_create(root, "f64", AK_FORM_PRIMITIVE_F64);
}

ak_form_t *ak_primitive_bool(void) {
  return ak_form_new_primitive(AK_FORM_PRIMITIVE_U8);
}

ak_form_t *ak_primitive_u8(void) {
  return ak_form_new_primitive(AK_FORM_PRIMITIVE_U8);
}

ak_form_t *ak_primitive_u16(void) {
  return ak_form_new_primitive(AK_FORM_PRIMITIVE_U16);
}

ak_form_t *ak_primitive_u32(void) {
  return ak_form_new_primitive(AK_FORM_PRIMITIVE_U32);
}

ak_form_t *ak_primitive_u64(void) {
  return ak_form_new_primitive(AK_FORM_PRIMITIVE_U64);
}

ak_form_t *ak_primitive_i8(void) {
  return ak_form_new_primitive(AK_FORM_PRIMITIVE_I8);
}

ak_form_t *ak_primitive_i16(void) {
  return ak_form_new_primitive(AK_FORM_PRIMITIVE_I16);
}

ak_form_t *ak_primitive_i32(void) {
  return ak_form_new_primitive(AK_FORM_PRIMITIVE_I32);
}

ak_form_t *ak_primitive_i64(void) {
  return ak_form_new_primitive(AK_FORM_PRIMITIVE_I64);
}

ak_form_t *ak_primitive_f32(void) {
  return ak_form_new_primitive(AK_FORM_PRIMITIVE_F32);
}

ak_form_t *ak_primitive_f64(void) {
  return ak_form_new_primitive(AK_FORM_PRIMITIVE_F64);
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
