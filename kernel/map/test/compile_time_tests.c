#include "map.h"
#include <stddef.h>

_Static_assert(sizeof(map_node_t *) == sizeof(void *),
               "map_node_t pointer size must match void pointer size");

_Static_assert(sizeof(map_base_t) > 0, "map_base_t must have non-zero size");

_Static_assert(offsetof(map_base_t, buckets) == 0,
               "buckets must be first member of map_base_t");

_Static_assert(sizeof(map_iter_t) > 0, "map_iter_t must have non-zero size");

_Static_assert(sizeof(map_hash_fn) == sizeof(void *),
               "map_hash_fn must be pointer-sized");

_Static_assert(sizeof(map_cmp_fn) == sizeof(void *),
               "map_cmp_fn must be pointer-sized");

typedef map_t(int) test_map_int_t;
_Static_assert(sizeof(test_map_int_t) > sizeof(map_base_t),
               "map_t(int) must be larger than base type");

typedef map_t(char *) test_map_str_t;
_Static_assert(sizeof(test_map_str_t) > sizeof(map_base_t),
               "map_t(char*) must be larger than base type");

typedef map_t(double) test_map_double_t;
_Static_assert(sizeof(test_map_double_t) > sizeof(map_base_t),
               "map_t(double) must be larger than base type");

_Static_assert(sizeof(map_void_t) == sizeof(test_map_str_t),
               "map_void_t should have same size as other pointer map types");

_Static_assert(sizeof(map_int_t) > 0, "map_int_t must have non-zero size");

_Static_assert(sizeof(map_str_t) > 0, "map_str_t must have non-zero size");

_Static_assert(sizeof(map_char_t) > 0, "map_char_t must have non-zero size");

_Static_assert(sizeof(map_float_t) > 0, "map_float_t must have non-zero size");

_Static_assert(sizeof(map_double_t) > 0,
               "map_double_t must have non-zero size");

_Static_assert(offsetof(test_map_int_t, base) == 0,
               "base must be first member of map_t");

_Static_assert(sizeof(unsigned) >= 4,
               "unsigned must be at least 4 bytes for hash values");

_Static_assert(sizeof(int) >= 4, "int must be at least 4 bytes for sizes");

typedef struct {
  map_base_t base;
  int *ref;
  int tmp;
} explicit_map_int_t;

_Static_assert(sizeof(explicit_map_int_t) == sizeof(map_int_t),
               "explicit map structure must match map_t(int) size");

_Static_assert(offsetof(explicit_map_int_t, base) == offsetof(map_int_t, base),
               "base offset must match in explicit and macro-generated types");

typedef struct {
  unsigned bucketidx;
  map_node_t *node;
} explicit_iter_t;

_Static_assert(sizeof(explicit_iter_t) == sizeof(map_iter_t),
               "explicit iter structure must match map_iter_t size");

_Static_assert(sizeof(char) == 1,
               "char must be 1 byte for key size calculations");

_Static_assert(sizeof(short) >= 2,
               "short must be at least 2 bytes for i16/u16 hash functions");

_Static_assert(sizeof(long long) >= 8,
               "long long must be at least 8 bytes for i64/u64 hash functions");

_Static_assert(sizeof(float) >= 4,
               "float must be at least 4 bytes for f32 hash function");

_Static_assert(sizeof(double) >= 8,
               "double must be at least 8 bytes for f64 hash function");

int main(void) { return 0; }
