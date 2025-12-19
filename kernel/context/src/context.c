#include "context.h"
#include "kernel.h"
#include <string.h>

ak_context_t *ak_context_new(void) {
  ak_context_t *ctx = AK24_ALLOC(sizeof(ak_context_t));
  if (!ctx) {
    return NULL;
  }

  ctx->parent = NULL;
  map_init_generic(&ctx->data, sizeof(char *), map_hash_str, map_cmp_str);
  list_init(&ctx->hoist_queue);

  return ctx;
}

ak_context_t *ak_context_push(ak_context_t *ctx) {
  if (!ctx) {
    return NULL;
  }

  ak_context_t *child = ak_context_new();
  if (!child) {
    return NULL;
  }

  child->parent = ctx;
  return child;
}

ak_context_t *ak_context_pop(ak_context_t *ctx) {
  if (!ctx) {
    return NULL;
  }

  ak_context_t *parent = ctx->parent;

  if (parent) {
    list_iter_t iter = list_iter(&ctx->hoist_queue);
    char **key_ptr;
    while ((key_ptr = list_next(&ctx->hoist_queue, &iter))) {
      const char *key = *key_ptr;
      void **value_ptr = map_get_generic(&ctx->data, &key);
      if (value_ptr) {
        void *value = *value_ptr;
        map_set_generic(&parent->data, &key, value);
      }
    }
  }

  map_deinit(&ctx->data);
  list_deinit(&ctx->hoist_queue);
  AK24_FREE(ctx);

  return parent;
}

int ak_context_set(ak_context_t *ctx, const char *key, void *value) {
  if (!ctx || !key) {
    return -1;
  }

  return map_set_generic(&ctx->data, &key, value);
}

void *ak_context_get(ak_context_t *ctx, const char *key) {
  if (!ctx || !key) {
    return NULL;
  }

  ak_context_t *current = ctx;
  while (current) {
    void **value = map_get_generic(&current->data, &key);
    if (value) {
      return *value;
    }
    current = current->parent;
  }

  return NULL;
}

void *ak_context_get_local(ak_context_t *ctx, const char *key) {
  if (!ctx || !key) {
    return NULL;
  }

  void **value = map_get_generic(&ctx->data, &key);
  return value ? *value : NULL;
}

ak_context_t *ak_context_get_containing_context(ak_context_t *ctx,
                                                const char *key) {
  if (!ctx || !key) {
    return NULL;
  }

  ak_context_t *current = ctx;
  while (current) {
    void **value = map_get_generic(&current->data, &key);
    if (value) {
      return current;
    }
    current = current->parent;
  }

  return NULL;
}

int ak_context_hoist(ak_context_t *ctx, const char *key) {
  if (!ctx || !key || !ctx->parent) {
    return -1;
  }

  list_iter_t iter = list_iter(&ctx->hoist_queue);
  char **existing;
  while ((existing = list_next(&ctx->hoist_queue, &iter))) {
    if (strcmp(*existing, key) == 0) {
      return 0;
    }
  }

  return list_push(&ctx->hoist_queue, (char *)key);
}

void ak_context_free(ak_context_t *ctx) {
  if (!ctx) {
    return;
  }

  map_deinit(&ctx->data);
  list_deinit(&ctx->hoist_queue);
  AK24_FREE(ctx);
}

int ak_context_has(ak_context_t *ctx, const char *key) {
  if (!ctx || !key) {
    return 0;
  }

  ak_context_t *current = ctx;
  while (current) {
    void **value = map_get_generic(&current->data, &key);
    if (value) {
      return 1;
    }
    current = current->parent;
  }

  return 0;
}

int ak_context_has_local(ak_context_t *ctx, const char *key) {
  if (!ctx || !key) {
    return 0;
  }

  void **value = map_get_generic(&ctx->data, &key);
  return value != NULL;
}

unsigned ak_context_depth(ak_context_t *ctx) {
  unsigned depth = 0;
  ak_context_t *current = ctx;

  while (current) {
    depth++;
    current = current->parent;
  }

  return depth;
}
