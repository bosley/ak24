/**
 * Copyright (c) 2014 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * - Modified for AK24 by bosley 2025
 */

#ifndef AK24_MAP_H
#define AK24_MAP_H

#include <string.h>

#define AK24_MAP_VERSION "0.2.0"

struct map_node_t;
typedef struct map_node_t map_node_t;

typedef unsigned (*map_hash_fn)(const void *key, int ksize);
typedef int (*map_cmp_fn)(const void *a, const void *b, int ksize);

typedef struct {
  map_node_t **buckets;
  unsigned nbuckets, nnodes;
  int ksize;
  map_hash_fn hash_fn;
  map_cmp_fn cmp_fn;
} map_base_t;

typedef struct {
  unsigned bucketidx;
  map_node_t *node;
} map_iter_t;

#define map_t(T)                                                               \
  struct {                                                                     \
    map_base_t base;                                                           \
    T *ref;                                                                    \
    T tmp;                                                                     \
  }

#define map_init_generic(m, keysize, hashfn, cmpfn)                            \
  do {                                                                         \
    memset(m, 0, sizeof(*(m)));                                                \
    (m)->base.ksize = (keysize);                                               \
    (m)->base.hash_fn = (hashfn);                                              \
    (m)->base.cmp_fn = (cmpfn);                                                \
  } while (0)

#define map_deinit(m) map_deinit_(&(m)->base)

#define map_get_generic(m, key) ((m)->ref = map_get_(&(m)->base, key))

#define map_set_generic(m, key, value)                                         \
  ((m)->tmp = (value), map_set_(&(m)->base, key, &(m)->tmp, sizeof((m)->tmp)))

#define map_remove_generic(m, key) map_remove_(&(m)->base, key)

#define map_iter(m) map_iter_()

#define map_next_generic(m, iter) map_next_(&(m)->base, iter)

void map_deinit_(map_base_t *m);
void *map_get_(map_base_t *m, const void *key);
int map_set_(map_base_t *m, const void *key, void *value, int vsize);
void map_remove_(map_base_t *m, const void *key);
map_iter_t map_iter_(void);
void *map_next_(map_base_t *m, map_iter_t *iter);

unsigned map_hash_str(const void *key, int ksize);
unsigned map_hash_i8(const void *key, int ksize);
unsigned map_hash_i16(const void *key, int ksize);
unsigned map_hash_i32(const void *key, int ksize);
unsigned map_hash_i64(const void *key, int ksize);
unsigned map_hash_u8(const void *key, int ksize);
unsigned map_hash_u16(const void *key, int ksize);
unsigned map_hash_u32(const void *key, int ksize);
unsigned map_hash_u64(const void *key, int ksize);
unsigned map_hash_f32(const void *key, int ksize);
unsigned map_hash_f64(const void *key, int ksize);
unsigned map_hash_bool(const void *key, int ksize);

int map_cmp_str(const void *a, const void *b, int ksize);
int map_cmp_mem(const void *a, const void *b, int ksize);

typedef map_t(void *) map_void_t;
typedef map_t(char *) map_str_t;
typedef map_t(int) map_int_t;
typedef map_t(char) map_char_t;
typedef map_t(float) map_float_t;
typedef map_t(double) map_double_t;

#endif
