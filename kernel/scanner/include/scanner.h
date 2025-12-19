#ifndef AK24_SCANNER_H
#define AK24_SCANNER_H

#include "buffer.h"
#include <stdbool.h>
#include <stdint.h>

/*
    Types are produced by a scanner off a buffer. The types listed here
    do not own the data they point to. they point into a buffer. the owner
    of the buffer is to maintain/manage that memory

    static_base_e represents the MOST BASIC data possible. no lists. no dynamic
   implications just shit we can infer from a small state machine off of a
   buffer

    start
        - if buffer at pos = [- or +] check next. if int -> switch to int parse
   state and keep sign, else: symbol
        - if while reading int we get "." -> real reading state (still consuming
   ints)
        - if white space \n \t ' ' etc, we are done
        - if parsing ints and non-whitespace non-peiord shows up (special case
   for real) -> error
*/
typedef enum ak_static_base_e {
  AK24_STATIC_BASE_NONE = 0,
  AK24_STATIC_BASE_INTEGER,
  AK24_STATIC_BASE_REAL,
  AK24_STATIC_BASE_SYMBOL,
} ak_static_base_e;

typedef uint8_t *ak_buffer_unowned_reference_t;

/*
    The static types and base types - really everything uses buffers. We don't
   own the buffers. We typedef like this to visually remind us not to try
   freeing them
*/
typedef struct ak_static_type_s {
  ak_static_base_e base;
  ak_buffer_unowned_reference_t data; // note: we typedef'd this for ownership
                                      // clarity. be sure to respect it!
  size_t byte_length;
} ak_static_type_t;

typedef struct ak_scanner_s {
  ak_buffer_t *buffer;
  size_t position;
} ak_scanner_t;

ak_scanner_t *ak_scanner_new(ak_buffer_t *buffer, size_t position);

void ak_scanner_free(ak_scanner_t *scanner);

typedef struct ak_scanner_static_type_result_s {
  bool success;
  size_t start_position;
  size_t error_position;
  ak_static_type_t data;
} ak_scanner_static_type_result_t;

typedef struct ak_scanner_stop_symbols_s {
  uint8_t *symbols;
  size_t count;
} ak_scanner_stop_symbols_t;

/*
reads from buffer:

  ints (with sign)
  real
  symbols

Terminates by default on all whitespace.
Optionally accepts additional stop symbols that will terminate parsing
without consuming the stop character. stop_symbols may be NULL.
Stop symbols must NOT include '.', '+', or '-'.

Failure to read an int, real, r symbol will result in an error - indicated in
the result Success will automatically move the position of the scanner
*/
ak_scanner_static_type_result_t
ak_scanner_read_static_base_type(ak_scanner_t *scanner,
                                 ak_scanner_stop_symbols_t *stop_symbols);

/*
A start/end range to view into a buffer some grouping of data (a subset)
*/
typedef struct ak_scanner_find_group_result_s {
  bool success;
  size_t index_of_start_symbol;
  size_t index_of_closing_symbol;
} ak_scanner_find_group_result_t;

/*
scans the buffer from the current position
if the current position does not match must_start_with, return failure
(a user could check themselves, but we're completionists)
must_end_with is the byte that says "stop here"
if can_escape_with is not NULL then any byte matching must_end_with may be
skipped over and included into the group iff immediatly preceeded by the
*can_escape_with

consume_leading_ws when true will skip any leading whitespace before checking
for must_start_with. when false the check happens at the current position.
the returned index_of_start_symbol will point to the delimiter not the
whitespace

this provides the ability for us to get a view into something of a list "form"
without limiting us to JUST processing () [] {} etc. We can say parse this
group:   !a b +1 2$ where ! is start and $ is end then it also means we can do
"hello \"world!\"!" my providing '\' as the escape byte and having start AND end
both be: " Naturally this means we can also find () [] {} || <> etc provided
that the buffer position starts with the must_start_with so this enforces/
implies that the caller is doing something specific, and we just don't need to
know

*/
ak_scanner_find_group_result_t ak_scanner_find_group(ak_scanner_t *scanner,
                                                     uint8_t must_start_with,
                                                     uint8_t must_end_with,
                                                     uint8_t *can_escape_with,
                                                     bool consume_leading_ws);

/*
Skips all leading whitespace and returns true if the next non-whitespace byte is
found returns false if the end of the buffer is reached
*/
bool ak_scanner_goto_next_non_white(ak_scanner_t *scanner);

bool ak_scanner_skip_whitespace_and_comments(ak_scanner_t *scanner);

/*
Skips to the next byte matching the given target_byte and returns true if found
returns false if the end of the buffer is reached
*/
bool ak_scanner_goto_next_target(ak_scanner_t *scanner, uint8_t target_byte);

#endif
