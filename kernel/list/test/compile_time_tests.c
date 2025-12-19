#include "list.h"
#include <stddef.h>

_Static_assert(sizeof(void *) > 0, "void pointer must have non-zero size");

_Static_assert(sizeof(list_base_t) > 0, "list_base_t must have non-zero size");

_Static_assert(offsetof(list_base_t, items) == 0,
               "items must be first member of list_base_t");

_Static_assert(sizeof(list_iter_t) > 0, "list_iter_t must have non-zero size");

_Static_assert(offsetof(list_iter_t, index) == 0,
               "index must be first member of list_iter_t");

typedef list_t(int) test_list_int_t;
_Static_assert(sizeof(test_list_int_t) > sizeof(list_base_t),
               "list_t(int) must be larger than base type");

typedef list_t(char *) test_list_str_t;
_Static_assert(sizeof(test_list_str_t) > sizeof(list_base_t),
               "list_t(char*) must be larger than base type");

typedef list_t(double) test_list_double_t;
_Static_assert(sizeof(test_list_double_t) > sizeof(list_base_t),
               "list_t(double) must be larger than base type");

_Static_assert(sizeof(list_void_t) > 0, "list_void_t must have non-zero size");

_Static_assert(sizeof(list_int_t) > 0, "list_int_t must have non-zero size");

_Static_assert(sizeof(list_str_t) > 0, "list_str_t must have non-zero size");

_Static_assert(sizeof(list_float_t) > 0,
               "list_float_t must have non-zero size");

_Static_assert(sizeof(list_double_t) > 0,
               "list_double_t must have non-zero size");

_Static_assert(offsetof(test_list_int_t, base) == 0,
               "base must be first member of list_t");

_Static_assert(sizeof(unsigned) >= 4,
               "unsigned must be at least 4 bytes for capacity and count");

_Static_assert(sizeof(int) >= 4, "int must be at least 4 bytes for item_size");

typedef struct {
  list_base_t base;
  int *ref;
  int tmp;
} explicit_list_int_t;

_Static_assert(sizeof(explicit_list_int_t) == sizeof(list_int_t),
               "explicit list structure must match list_t(int) size");

_Static_assert(offsetof(explicit_list_int_t, base) ==
                   offsetof(list_int_t, base),
               "base offset must match in explicit and macro-generated types");

typedef struct {
  unsigned index;
  int valid;
} explicit_iter_t;

_Static_assert(sizeof(explicit_iter_t) == sizeof(list_iter_t),
               "explicit iter structure must match list_iter_t size");

_Static_assert(sizeof(char) == 1, "char must be 1 byte for size calculations");

_Static_assert(sizeof(float) >= 4, "float must be at least 4 bytes");

_Static_assert(sizeof(double) >= 8, "double must be at least 8 bytes");

typedef list_t(list_int_t *) nested_list_t;
_Static_assert(sizeof(nested_list_t) > sizeof(list_base_t),
               "nested list type must be larger than base type");

_Static_assert(sizeof(void **) == sizeof(void *),
               "pointer to pointer must have same size as pointer");

int main(void) { return 0; }
