#ifndef AK24_FORMS_PRIMITIVES_H
#define AK24_FORMS_PRIMITIVES_H

#include "forms.h"

typedef struct {
  ak_form_t *form;
  void *data;
} ak_form_instance_t;

typedef struct {
  ak_form_t *form;
  int is_present;
  void *value;
} ak_optional_instance_t;

typedef struct {
  ak_form_t *form;
  list_void_t items;
} ak_repeatable_instance_t;

typedef struct {
  ak_form_t *form;
  list_void_t items;
} ak_list_instance_t;

typedef struct {
  ak_form_t *form;
  map_void_t items;
} ak_map_instance_t;

ak_form_t *ak_primitive_bool(void);
ak_form_t *ak_primitive_u8(void);
ak_form_t *ak_primitive_u16(void);
ak_form_t *ak_primitive_u32(void);
ak_form_t *ak_primitive_u64(void);
ak_form_t *ak_primitive_i8(void);
ak_form_t *ak_primitive_i16(void);
ak_form_t *ak_primitive_i32(void);
ak_form_t *ak_primitive_i64(void);
ak_form_t *ak_primitive_f32(void);
ak_form_t *ak_primitive_f64(void);

void ak_form_attach_builtin_affects(ak_form_t *form);

ak_lambda_t *ak_builtin_is_none(void);
ak_lambda_t *ak_builtin_is_some(void);

ak_lambda_t *ak_builtin_append(void);
ak_lambda_t *ak_builtin_pop(void);
ak_lambda_t *ak_builtin_push(void);
ak_lambda_t *ak_builtin_rest(void);

ak_lambda_t *ak_builtin_list_index(void);
ak_lambda_t *ak_builtin_list_length(void);

ak_lambda_t *ak_builtin_map_get(void);
ak_lambda_t *ak_builtin_map_set(void);
ak_lambda_t *ak_builtin_map_has(void);

ak_optional_instance_t *ak_optional_instance_new(ak_form_t *form);
void ak_optional_instance_free(ak_optional_instance_t *inst);
void ak_optional_instance_set(ak_optional_instance_t *inst, void *value);
void *ak_optional_instance_get(ak_optional_instance_t *inst);

ak_repeatable_instance_t *ak_repeatable_instance_new(ak_form_t *form);
void ak_repeatable_instance_free(ak_repeatable_instance_t *inst);

ak_list_instance_t *ak_list_instance_new(ak_form_t *form);
void ak_list_instance_free(ak_list_instance_t *inst);

ak_map_instance_t *ak_map_instance_new(ak_form_t *form);
void ak_map_instance_free(ak_map_instance_t *inst);

#endif
