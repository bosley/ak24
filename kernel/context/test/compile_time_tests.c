#include "context.h"
#include <stddef.h>

_Static_assert(sizeof(ak_context_t) > 0,
               "ak_context_t must have non-zero size");

_Static_assert(offsetof(ak_context_t, parent) == 0,
               "parent must be first member of ak_context_t");

_Static_assert(sizeof(ak_context_t *) == sizeof(void *),
               "context pointer must match void pointer size");

_Static_assert(offsetof(ak_context_t, data) > 0,
               "data must be after parent in ak_context_t");

_Static_assert(offsetof(ak_context_t, hoist_queue) >
                   offsetof(ak_context_t, data),
               "hoist_queue must be after data in ak_context_t");

int main(void) { return 0; }
