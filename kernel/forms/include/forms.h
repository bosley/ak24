#ifndef AK24_FORMS_H
#define AK24_FORMS_H

#include "atom.h"
#include "context.h"
#include "lambda.h"
#include "list.h"

#define AK24_FORMS_VERSION "0.1.0"

typedef enum {
  AK_FORM_PRIMITIVE,
  AK_FORM_COMPOUND,
  AK_FORM_OPTIONAL,
  AK_FORM_REPEATABLE,
  AK_FORM_STRUCT,
  AK_FORM_LIST,
  AK_FORM_MAP,
  AK_FORM_NAMED
} ak_form_kind_e;

typedef struct ak_form_s ak_form_t;
typedef struct ak_affect_s ak_affect_t;

struct ak_affect_s {
  char *name;
  ak_lambda_t *lambda;
  list_void_t parameter_forms;
  ak_form_t *return_form;
  list_str_t also_includes;
};

struct ak_form_s {
  ak_form_kind_e kind;
  char *name;
  union {
    ak_atom_type_e primitive;
    list_void_t compound_parts;
    struct {
      ak_form_t *inner;
    } optional;
    struct {
      ak_form_t *inner;
    } repeatable;
    struct {
      ak_form_t *pattern;
      list_str_t field_names;
    } struct_form;
    struct {
      ak_form_t *element_type;
    } list_form;
    struct {
      ak_atom_type_e key_type;
      ak_form_t *value_type;
    } map_form;
    struct {
      ak_form_t *actual_form;
    } named;
  } data;
  list_void_t affects;
};

ak_form_t *ak_form_new_primitive(ak_atom_type_e type);

ak_form_t *ak_form_new_compound(list_void_t *parts);

ak_form_t *ak_form_new_optional(ak_form_t *inner);

ak_form_t *ak_form_new_repeatable(ak_form_t *inner);

ak_form_t *ak_form_new_struct(ak_form_t *pattern, list_str_t *field_names);

ak_form_t *ak_form_new_list(ak_form_t *element_type);

ak_form_t *ak_form_new_map(ak_atom_type_e key_type, ak_form_t *value_type);

ak_form_t *ak_form_new_named(const char *name, ak_form_t *form);

void ak_form_free(ak_form_t *form);

int ak_form_register(ak_context_t *ctx, const char *name, ak_form_t *form);

ak_form_t *ak_form_lookup(ak_context_t *ctx, const char *name);

ak_form_t *ak_form_lookup_local(ak_context_t *ctx, const char *name);

int ak_form_matches_pattern(ak_form_t *form, ak_form_t *pattern);

int ak_form_is_compatible(ak_form_t *form1, ak_form_t *form2);

ak_affect_t *ak_affect_new(const char *name, ak_lambda_t *lambda,
                           list_void_t *params, ak_form_t *return_type);

void ak_affect_free(ak_affect_t *affect);

int ak_form_add_affect(ak_form_t *form, ak_affect_t *affect);

ak_affect_t *ak_form_get_affect(ak_form_t *form, const char *name);

int ak_form_has_affect(ak_form_t *form, const char *name);

int ak_affect_add_also(ak_affect_t *affect, const char *other_name);

int ak_form_has_affordance(ak_form_t *form, const char *affordance_name);

list_str_t ak_form_get_affordances(ak_form_t *form);

#endif
