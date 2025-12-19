#include "scanner.h"
#include <stddef.h>

_Static_assert(sizeof(void *) > 0, "void pointer must have non-zero size");

_Static_assert(sizeof(ak_scanner_t) > 0,
               "ak_scanner_t must have non-zero size");

_Static_assert(offsetof(ak_scanner_t, buffer) == 0,
               "buffer must be first member of ak_scanner_t");

_Static_assert(sizeof(ak_static_type_t) > 0,
               "ak_static_type_t must have non-zero size");

_Static_assert(offsetof(ak_static_type_t, base) == 0,
               "base must be first member of ak_static_type_t");

_Static_assert(sizeof(ak_static_base_e) > 0,
               "ak_static_base_e must have non-zero size");

_Static_assert(sizeof(ak_buffer_unowned_reference_t) == sizeof(uint8_t *),
               "ak_buffer_unowned_reference_t must be pointer-sized");

_Static_assert(sizeof(ak_scanner_static_type_result_t) > 0,
               "ak_scanner_static_type_result_t must have non-zero size");

_Static_assert(sizeof(ak_scanner_find_group_result_t) > 0,
               "ak_scanner_find_group_result_t must have non-zero size");

_Static_assert(sizeof(ak_scanner_stop_symbols_t) > 0,
               "ak_scanner_stop_symbols_t must have non-zero size");

_Static_assert(sizeof(size_t) >= sizeof(unsigned),
               "size_t must be at least as large as unsigned");

_Static_assert(sizeof(uint8_t) == 1, "uint8_t must be 1 byte");

_Static_assert(AK24_STATIC_BASE_NONE == 0, "AK24_STATIC_BASE_NONE must be 0");

_Static_assert(AK24_STATIC_BASE_INTEGER != AK24_STATIC_BASE_NONE,
               "AK24_STATIC_BASE_INTEGER must be distinct from NONE");

_Static_assert(AK24_STATIC_BASE_REAL != AK24_STATIC_BASE_NONE,
               "AK24_STATIC_BASE_REAL must be distinct from NONE");

_Static_assert(AK24_STATIC_BASE_SYMBOL != AK24_STATIC_BASE_NONE,
               "AK24_STATIC_BASE_SYMBOL must be distinct from NONE");

_Static_assert(AK24_STATIC_BASE_INTEGER != AK24_STATIC_BASE_REAL,
               "INTEGER and REAL must be distinct types");

_Static_assert(AK24_STATIC_BASE_INTEGER != AK24_STATIC_BASE_SYMBOL,
               "INTEGER and SYMBOL must be distinct types");

_Static_assert(AK24_STATIC_BASE_REAL != AK24_STATIC_BASE_SYMBOL,
               "REAL and SYMBOL must be distinct types");

int main(void) { return 0; }
