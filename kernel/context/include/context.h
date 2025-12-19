#ifndef AK24_CONTEXT_H
#define AK24_CONTEXT_H

#include "list.h"
#include "map.h"

#define AK24_CONTEXT_VERSION "0.1.0"

typedef struct ak_context_t {
  struct ak_context_t *parent;
  map_void_t data;
  list_str_t hoist_queue;
} ak_context_t;

ak_context_t *ak_context_new(void);

ak_context_t *ak_context_push(ak_context_t *ctx);

ak_context_t *ak_context_pop(ak_context_t *ctx);

int ak_context_set(ak_context_t *ctx, const char *key, void *value);

void *ak_context_get(ak_context_t *ctx, const char *key);

void *ak_context_get_local(ak_context_t *ctx, const char *key);

ak_context_t *ak_context_get_containing_context(ak_context_t *ctx,
                                                const char *key);

int ak_context_hoist(ak_context_t *ctx, const char *key);

void ak_context_free(ak_context_t *ctx);

int ak_context_has(ak_context_t *ctx, const char *key);

int ak_context_has_local(ak_context_t *ctx, const char *key);

unsigned ak_context_depth(ak_context_t *ctx);

#endif
