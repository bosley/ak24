#ifndef AK24_ARBUFF_H
#define AK24_ARBUFF_H

#include <stdatomic.h>
#include <stddef.h>

#define AK24_ARBUFF_VERSION "0.1.0"

typedef struct {
  _Atomic(void *) value;
  _Atomic size_t sequence;
} ak_arbuff_slot_t;

typedef struct ak_arbuff_t {
  ak_arbuff_slot_t *slots;
  size_t capacity;
  size_t mask;
  _Atomic size_t head;
  _Atomic size_t tail;
} ak_arbuff_t;

ak_arbuff_t *ak_arbuff_new(size_t capacity);

void ak_arbuff_free(ak_arbuff_t *arbuff);

int ak_arbuff_push(ak_arbuff_t *arbuff, void *item);

void *ak_arbuff_pop(ak_arbuff_t *arbuff);

void *ak_arbuff_peek(ak_arbuff_t *arbuff);

size_t ak_arbuff_count(ak_arbuff_t *arbuff);

size_t ak_arbuff_capacity(ak_arbuff_t *arbuff);

int ak_arbuff_is_empty(ak_arbuff_t *arbuff);

int ak_arbuff_is_full(ak_arbuff_t *arbuff);

void ak_arbuff_clear(ak_arbuff_t *arbuff);

#endif
