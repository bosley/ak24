#include "forms.h"
#include "kernel.h"
#include <string.h>

ak_form_t *ak_form_new_primitive(ak_atom_type_e type) {
  ak_form_t *form = AK24_ALLOC(sizeof(ak_form_t));
  if (!form) {
    return NULL;
  }

  memset(form, 0, sizeof(ak_form_t));
  form->kind = AK_FORM_PRIMITIVE;
  form->data.primitive = type;
  list_init(&form->affects);

  return form;
}

ak_form_t *ak_form_new_compound(list_void_t *parts) {
  if (!parts) {
    return NULL;
  }

  ak_form_t *form = AK24_ALLOC(sizeof(ak_form_t));
  if (!form) {
    return NULL;
  }

  memset(form, 0, sizeof(ak_form_t));
  form->kind = AK_FORM_COMPOUND;
  list_init(&form->data.compound_parts);
  list_init(&form->affects);

  list_iter_t iter = list_iter(parts);
  ak_form_t **part;
  while ((part = (ak_form_t **)list_next(parts, &iter))) {
    list_push(&form->data.compound_parts, *part);
  }

  return form;
}

ak_form_t *ak_form_new_optional(ak_form_t *inner) {
  if (!inner) {
    return NULL;
  }

  ak_form_t *form = AK24_ALLOC(sizeof(ak_form_t));
  if (!form) {
    return NULL;
  }

  memset(form, 0, sizeof(ak_form_t));
  form->kind = AK_FORM_OPTIONAL;
  form->data.optional.inner = inner;
  list_init(&form->affects);

  return form;
}

ak_form_t *ak_form_new_repeatable(ak_form_t *inner) {
  if (!inner) {
    return NULL;
  }

  ak_form_t *form = AK24_ALLOC(sizeof(ak_form_t));
  if (!form) {
    return NULL;
  }

  memset(form, 0, sizeof(ak_form_t));
  form->kind = AK_FORM_REPEATABLE;
  form->data.repeatable.inner = inner;
  list_init(&form->affects);

  return form;
}

ak_form_t *ak_form_new_struct(ak_form_t *pattern, list_str_t *field_names) {
  if (!pattern || !field_names) {
    return NULL;
  }

  ak_form_t *form = AK24_ALLOC(sizeof(ak_form_t));
  if (!form) {
    return NULL;
  }

  memset(form, 0, sizeof(ak_form_t));
  form->kind = AK_FORM_STRUCT;
  form->data.struct_form.pattern = pattern;
  list_init(&form->data.struct_form.field_names);
  list_init(&form->affects);

  list_iter_t iter = list_iter(field_names);
  char **field;
  while ((field = list_next(field_names, &iter))) {
    list_push(&form->data.struct_form.field_names, *field);
  }

  return form;
}

ak_form_t *ak_form_new_list(ak_form_t *element_type) {
  if (!element_type) {
    return NULL;
  }

  ak_form_t *form = AK24_ALLOC(sizeof(ak_form_t));
  if (!form) {
    return NULL;
  }

  memset(form, 0, sizeof(ak_form_t));
  form->kind = AK_FORM_LIST;
  form->data.list_form.element_type = element_type;
  list_init(&form->affects);

  return form;
}

ak_form_t *ak_form_new_map(ak_atom_type_e key_type, ak_form_t *value_type) {
  if (!value_type) {
    return NULL;
  }

  ak_form_t *form = AK24_ALLOC(sizeof(ak_form_t));
  if (!form) {
    return NULL;
  }

  memset(form, 0, sizeof(ak_form_t));
  form->kind = AK_FORM_MAP;
  form->data.map_form.key_type = key_type;
  form->data.map_form.value_type = value_type;
  list_init(&form->affects);

  return form;
}

ak_form_t *ak_form_new_named(const char *name, ak_form_t *form) {
  if (!name || !form) {
    return NULL;
  }

  ak_form_t *named_form = AK24_ALLOC(sizeof(ak_form_t));
  if (!named_form) {
    return NULL;
  }

  memset(named_form, 0, sizeof(ak_form_t));
  named_form->kind = AK_FORM_NAMED;
  named_form->name = AK24_ALLOC(strlen(name) + 1);
  if (!named_form->name) {
    AK24_FREE(named_form);
    return NULL;
  }
  strcpy(named_form->name, name);
  named_form->data.named.actual_form = form;
  list_init(&named_form->affects);

  return named_form;
}

void ak_form_free(ak_form_t *form) {
  if (!form) {
    return;
  }

  if (form->name) {
    AK24_FREE(form->name);
  }

  switch (form->kind) {
  case AK_FORM_PRIMITIVE:
    break;

  case AK_FORM_COMPOUND: {
    list_iter_t iter = list_iter(&form->data.compound_parts);
    ak_form_t **part;
    while (
        (part = (ak_form_t **)list_next(&form->data.compound_parts, &iter))) {
      ak_form_free(*part);
    }
    list_deinit(&form->data.compound_parts);
    break;
  }

  case AK_FORM_OPTIONAL:
    ak_form_free(form->data.optional.inner);
    break;

  case AK_FORM_REPEATABLE:
    ak_form_free(form->data.repeatable.inner);
    break;

  case AK_FORM_STRUCT:
    ak_form_free(form->data.struct_form.pattern);
    list_deinit(&form->data.struct_form.field_names);
    break;

  case AK_FORM_LIST:
    ak_form_free(form->data.list_form.element_type);
    break;

  case AK_FORM_MAP:
    ak_form_free(form->data.map_form.value_type);
    break;

  case AK_FORM_NAMED:
    ak_form_free(form->data.named.actual_form);
    break;
  }

  list_iter_t iter = list_iter(&form->affects);
  ak_affect_t **affect;
  while ((affect = (ak_affect_t **)list_next(&form->affects, &iter))) {
    ak_affect_free(*affect);
  }
  list_deinit(&form->affects);

  AK24_FREE(form);
}

int ak_form_register(ak_context_t *ctx, const char *name, ak_form_t *form) {
  if (!ctx || !name || !form) {
    return -1;
  }

  return ak_context_set(ctx, name, form);
}

ak_form_t *ak_form_lookup(ak_context_t *ctx, const char *name) {
  if (!ctx || !name) {
    return NULL;
  }

  return (ak_form_t *)ak_context_get(ctx, name);
}

ak_form_t *ak_form_lookup_local(ak_context_t *ctx, const char *name) {
  if (!ctx || !name) {
    return NULL;
  }

  return (ak_form_t *)ak_context_get_local(ctx, name);
}

static int forms_structurally_equal(ak_form_t *form1, ak_form_t *form2);

int ak_form_matches_pattern(ak_form_t *form, ak_form_t *pattern) {
  if (!form || !pattern) {
    return 0;
  }

  return forms_structurally_equal(form, pattern);
}

int ak_form_is_compatible(ak_form_t *form1, ak_form_t *form2) {
  if (!form1 || !form2) {
    return 0;
  }

  return forms_structurally_equal(form1, form2);
}

static int forms_structurally_equal(ak_form_t *form1, ak_form_t *form2) {
  if (!form1 || !form2) {
    return 0;
  }

  if (form1->kind == AK_FORM_NAMED) {
    form1 = form1->data.named.actual_form;
  }
  if (form2->kind == AK_FORM_NAMED) {
    form2 = form2->data.named.actual_form;
  }

  if (form1->kind != form2->kind) {
    return 0;
  }

  switch (form1->kind) {
  case AK_FORM_PRIMITIVE:
    return form1->data.primitive == form2->data.primitive;

  case AK_FORM_COMPOUND: {
    unsigned count1 = list_count(&form1->data.compound_parts);
    unsigned count2 = list_count(&form2->data.compound_parts);
    if (count1 != count2) {
      return 0;
    }

    for (unsigned i = 0; i < count1; i++) {
      ak_form_t **part1 =
          (ak_form_t **)list_get(&form1->data.compound_parts, i);
      ak_form_t **part2 =
          (ak_form_t **)list_get(&form2->data.compound_parts, i);
      if (!part1 || !part2 || !forms_structurally_equal(*part1, *part2)) {
        return 0;
      }
    }
    return 1;
  }

  case AK_FORM_OPTIONAL:
    return forms_structurally_equal(form1->data.optional.inner,
                                    form2->data.optional.inner);

  case AK_FORM_REPEATABLE:
    return forms_structurally_equal(form1->data.repeatable.inner,
                                    form2->data.repeatable.inner);

  case AK_FORM_STRUCT: {
    unsigned count1 = list_count(&form1->data.struct_form.field_names);
    unsigned count2 = list_count(&form2->data.struct_form.field_names);
    if (count1 != count2) {
      return 0;
    }

    return forms_structurally_equal(form1->data.struct_form.pattern,
                                    form2->data.struct_form.pattern);
  }

  case AK_FORM_LIST:
    return forms_structurally_equal(form1->data.list_form.element_type,
                                    form2->data.list_form.element_type);

  case AK_FORM_MAP:
    if (form1->data.map_form.key_type != form2->data.map_form.key_type) {
      return 0;
    }
    return forms_structurally_equal(form1->data.map_form.value_type,
                                    form2->data.map_form.value_type);

  case AK_FORM_NAMED:
    return 0;
  }

  return 0;
}

ak_affect_t *ak_affect_new(const char *name, ak_lambda_t *lambda,
                           list_void_t *params, ak_form_t *return_type) {
  if (!name) {
    return NULL;
  }

  ak_affect_t *affect = AK24_ALLOC(sizeof(ak_affect_t));
  if (!affect) {
    return NULL;
  }

  memset(affect, 0, sizeof(ak_affect_t));
  affect->name = AK24_ALLOC(strlen(name) + 1);
  if (!affect->name) {
    AK24_FREE(affect);
    return NULL;
  }
  strcpy(affect->name, name);

  affect->lambda = lambda;
  affect->return_form = return_type;

  list_init(&affect->parameter_forms);
  if (params) {
    list_iter_t iter = list_iter(params);
    ak_form_t **param;
    while ((param = (ak_form_t **)list_next(params, &iter))) {
      list_push(&affect->parameter_forms, *param);
    }
  }

  list_init(&affect->also_includes);

  return affect;
}

void ak_affect_free(ak_affect_t *affect) {
  if (!affect) {
    return;
  }

  if (affect->name) {
    AK24_FREE(affect->name);
  }

  if (affect->lambda) {
    ak_lambda_free(affect->lambda);
  }

  list_deinit(&affect->parameter_forms);
  list_deinit(&affect->also_includes);

  AK24_FREE(affect);
}

int ak_form_add_affect(ak_form_t *form, ak_affect_t *affect) {
  if (!form || !affect) {
    return -1;
  }

  return list_push(&form->affects, affect);
}

ak_affect_t *ak_form_get_affect(ak_form_t *form, const char *name) {
  if (!form || !name) {
    return NULL;
  }

  ak_form_t *current = form;
  if (current->kind == AK_FORM_NAMED) {
    current = current->data.named.actual_form;
  }

  list_iter_t iter = list_iter(&current->affects);
  ak_affect_t **affect;
  while ((affect = (ak_affect_t **)list_next(&current->affects, &iter))) {
    if (strcmp((*affect)->name, name) == 0) {
      return *affect;
    }
  }

  return NULL;
}

int ak_form_has_affect(ak_form_t *form, const char *name) {
  return ak_form_get_affect(form, name) != NULL;
}

int ak_affect_add_also(ak_affect_t *affect, const char *other_name) {
  if (!affect || !other_name) {
    return -1;
  }

  char *name_copy = AK24_ALLOC(strlen(other_name) + 1);
  if (!name_copy) {
    return -1;
  }
  strcpy(name_copy, other_name);

  return list_push(&affect->also_includes, name_copy);
}

static int form_has_structural_affordance(ak_form_t *form,
                                          const char *affordance_name);

int ak_form_has_affordance(ak_form_t *form, const char *affordance_name) {
  if (!form || !affordance_name) {
    return 0;
  }

  if (ak_form_has_affect(form, affordance_name)) {
    return 1;
  }

  return form_has_structural_affordance(form, affordance_name);
}

static int form_has_structural_affordance(ak_form_t *form,
                                          const char *affordance_name) {
  if (!form || !affordance_name) {
    return 0;
  }

  ak_form_t *current = form;
  if (current->kind == AK_FORM_NAMED) {
    current = current->data.named.actual_form;
  }

  switch (current->kind) {
  case AK_FORM_OPTIONAL:
    if (strcmp(affordance_name, "is_none") == 0 ||
        strcmp(affordance_name, "is_some") == 0) {
      return 1;
    }
    break;

  case AK_FORM_REPEATABLE:
    if (strcmp(affordance_name, "append") == 0 ||
        strcmp(affordance_name, "pop") == 0 ||
        strcmp(affordance_name, "push") == 0 ||
        strcmp(affordance_name, "rest") == 0 ||
        strcmp(affordance_name, "iterate") == 0) {
      return 1;
    }
    break;

  case AK_FORM_LIST:
    if (strcmp(affordance_name, "index") == 0 ||
        strcmp(affordance_name, "iterate") == 0 ||
        strcmp(affordance_name, "length") == 0) {
      return 1;
    }
    break;

  case AK_FORM_MAP:
    if (strcmp(affordance_name, "get") == 0 ||
        strcmp(affordance_name, "set") == 0 ||
        strcmp(affordance_name, "has") == 0 ||
        strcmp(affordance_name, "iterate") == 0) {
      return 1;
    }
    break;

  case AK_FORM_STRUCT: {
    list_iter_t iter = list_iter(&current->data.struct_form.field_names);
    char **field;
    while ((field = list_next(&current->data.struct_form.field_names, &iter))) {
      if (strcmp(affordance_name, *field) == 0) {
        return 1;
      }
    }
    break;
  }

  default:
    break;
  }

  return 0;
}

list_str_t ak_form_get_affordances(ak_form_t *form) {
  list_str_t affordances;
  list_init(&affordances);

  if (!form) {
    return affordances;
  }

  ak_form_t *current = form;
  if (current->kind == AK_FORM_NAMED) {
    current = current->data.named.actual_form;
  }

  list_iter_t iter = list_iter(&current->affects);
  ak_affect_t **affect;
  while ((affect = (ak_affect_t **)list_next(&current->affects, &iter))) {
    char *name_copy = AK24_ALLOC(strlen((*affect)->name) + 1);
    if (name_copy) {
      strcpy(name_copy, (*affect)->name);
      list_push(&affordances, name_copy);
    }
  }

  switch (current->kind) {
  case AK_FORM_OPTIONAL: {
    char *is_none = AK24_ALLOC(8);
    char *is_some = AK24_ALLOC(8);
    if (is_none && is_some) {
      strcpy(is_none, "is_none");
      strcpy(is_some, "is_some");
      list_push(&affordances, is_none);
      list_push(&affordances, is_some);
    }
    break;
  }

  case AK_FORM_REPEATABLE: {
    const char *names[] = {"append", "pop", "push", "rest", "iterate"};
    for (int i = 0; i < 5; i++) {
      char *name = AK24_ALLOC(strlen(names[i]) + 1);
      if (name) {
        strcpy(name, names[i]);
        list_push(&affordances, name);
      }
    }
    break;
  }

  case AK_FORM_LIST: {
    const char *names[] = {"index", "iterate", "length"};
    for (int i = 0; i < 3; i++) {
      char *name = AK24_ALLOC(strlen(names[i]) + 1);
      if (name) {
        strcpy(name, names[i]);
        list_push(&affordances, name);
      }
    }
    break;
  }

  case AK_FORM_MAP: {
    const char *names[] = {"get", "set", "has", "iterate"};
    for (int i = 0; i < 4; i++) {
      char *name = AK24_ALLOC(strlen(names[i]) + 1);
      if (name) {
        strcpy(name, names[i]);
        list_push(&affordances, name);
      }
    }
    break;
  }

  case AK_FORM_STRUCT: {
    list_iter_t field_iter = list_iter(&current->data.struct_form.field_names);
    char **field;
    while ((field = list_next(&current->data.struct_form.field_names,
                              &field_iter))) {
      char *name = AK24_ALLOC(strlen(*field) + 1);
      if (name) {
        strcpy(name, *field);
        list_push(&affordances, name);
      }
    }
    break;
  }

  default:
    break;
  }

  return affordances;
}
