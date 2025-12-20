/**
 * @file scanner.h
 * @brief Lexical scanner for parsing basic types and delimited groups from
 * buffers
 *
 * Provides a state-machine-based scanner for extracting integers, reals, and
 * symbols from byte buffers. Supports configurable stop symbols, whitespace
 * handling, and delimited group extraction with escape sequences.
 *
 * Key features:
 * - Integer parsing with sign support
 * - Real number parsing with decimal points
 * - Symbol extraction
 * - Configurable stop symbols and whitespace handling
 * - Delimited group finding with escape sequences
 * - Non-owning buffer references for zero-copy parsing
 * - Position tracking for error reporting
 *
 * @note Scanner does NOT own buffer data - caller manages buffer lifetime
 * @note All operations are NOT thread-safe
 */

#ifndef AK24_SCANNER_H
#define AK24_SCANNER_H

#include "buffer.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Basic static type categories
 *
 * Represents the most basic data types that can be inferred from a simple
 * state machine operating on a buffer.
 */
typedef enum ak_static_base_e {
  AK24_STATIC_BASE_NONE = 0, /**< No type or error */
  AK24_STATIC_BASE_INTEGER,  /**< Signed integer */
  AK24_STATIC_BASE_REAL,     /**< Floating point number */
  AK24_STATIC_BASE_SYMBOL,   /**< Symbol or identifier */
} ak_static_base_e;

/**
 * @brief Unowned pointer to buffer data
 *
 * Type alias to emphasize that this pointer does NOT own the data and
 * should not be freed.
 */
typedef uint8_t *ak_buffer_unowned_reference_t;

/**
 * @brief Static type with non-owning buffer reference
 *
 * Represents a parsed type that points into a buffer without owning the data.
 * The buffer owner is responsible for memory management.
 */
typedef struct ak_static_type_s {
  ak_static_base_e base;              /**< Type category */
  ak_buffer_unowned_reference_t data; /**< Pointer into buffer (NOT owned) */
  size_t byte_length;                 /**< Length of data in bytes */
} ak_static_type_t;

/**
 * @brief Scanner state structure
 *
 * Maintains position within a buffer for sequential parsing operations.
 */
typedef struct ak_scanner_s {
  ak_buffer_t *buffer; /**< Buffer being scanned (NOT owned) */
  size_t position;     /**< Current byte position */
} ak_scanner_t;

/**
 * @brief Create a new scanner
 *
 * Allocates a scanner positioned at the specified location in the buffer.
 *
 * @param buffer Buffer to scan (NOT owned by scanner)
 * @param position Initial position in buffer
 * @return Pointer to new scanner, or NULL on allocation failure
 *
 * @notthreadsafe
 *
 * @note Caller must free with ak_scanner_free()
 * @note Buffer must remain valid for scanner lifetime
 */
ak_scanner_t *ak_scanner_new(ak_buffer_t *buffer, size_t position);

/**
 * @brief Free a scanner
 *
 * Releases scanner memory. Does NOT free the buffer.
 *
 * @param scanner Scanner to free (NULL is safe)
 *
 * @notthreadsafe
 */
void ak_scanner_free(ak_scanner_t *scanner);

/**
 * @brief Result of static type parsing
 *
 * Contains success status, position information, and parsed data.
 */
typedef struct ak_scanner_static_type_result_s {
  bool success;          /**< Whether parsing succeeded */
  size_t start_position; /**< Position where parsing started */
  size_t error_position; /**< Position of error if failed */
  ak_static_type_t data; /**< Parsed type data */
} ak_scanner_static_type_result_t;

/**
 * @brief Stop symbol configuration
 *
 * Specifies additional bytes that should terminate parsing.
 */
typedef struct ak_scanner_stop_symbols_s {
  uint8_t *symbols; /**< Array of stop bytes */
  size_t count;     /**< Number of stop symbols */
} ak_scanner_stop_symbols_t;

/**
 * @brief Read a static base type from buffer
 *
 * Parses an integer (with sign), real number, or symbol from the current
 * scanner position. Parsing terminates on whitespace by default, with
 * optional additional stop symbols.
 *
 * The scanner position advances automatically on success. On failure,
 * position is left at the error location.
 *
 * @param scanner Scanner to read from
 * @param stop_symbols Additional stop bytes (can be NULL)
 * @return Result containing success status and parsed data
 *
 * @notthreadsafe
 *
 * @note Stop symbols must NOT include '.', '+', or '-'
 * @note Parsing terminates without consuming the stop character
 *
 * @par Example:
 * @code
 * ak_scanner_t *s = ak_scanner_new(buf, 0);
 * ak_scanner_static_type_result_t result =
 *     ak_scanner_read_static_base_type(s, NULL);
 * if (result.success) {
 *   if (result.data.base == AK24_STATIC_BASE_INTEGER) {
 *     printf("Found integer\n");
 *   }
 * }
 * @endcode
 */
ak_scanner_static_type_result_t
ak_scanner_read_static_base_type(ak_scanner_t *scanner,
                                 ak_scanner_stop_symbols_t *stop_symbols);

/**
 * @brief Result of group finding operation
 *
 * Contains indices of start and end delimiters for a delimited group.
 */
typedef struct ak_scanner_find_group_result_s {
  bool success;                   /**< Whether group was found */
  size_t index_of_start_symbol;   /**< Index of opening delimiter */
  size_t index_of_closing_symbol; /**< Index of closing delimiter */
} ak_scanner_find_group_result_t;

/**
 * @brief Find a delimited group in buffer
 *
 * Scans for a group bounded by start and end delimiters, with optional
 * escape sequence support. Useful for parsing strings, lists, and other
 * delimited structures.
 *
 * The function can handle:
 * - Symmetric delimiters (e.g., "string")
 * - Asymmetric delimiters (e.g., [list], {block}, <tag>)
 * - Custom delimiters (e.g., !data$)
 * - Escaped delimiters within content
 *
 * @param scanner Scanner to search from
 * @param must_start_with Opening delimiter byte
 * @param must_end_with Closing delimiter byte
 * @param can_escape_with Escape byte (NULL to disable escaping)
 * @param consume_leading_ws Skip whitespace before checking start delimiter
 * @return Result with indices of delimiters, or failure
 *
 * @notthreadsafe
 *
 * @note Returns index of delimiter, not whitespace
 * @note Fails if current position doesn't match start delimiter
 *
 * @par Example:
 * @code
 * uint8_t escape = '\\';
 * ak_scanner_find_group_result_t result =
 *     ak_scanner_find_group(s, '"', '"', &escape, true);
 * if (result.success) {
 *   size_t content_start = result.index_of_start_symbol + 1;
 *   size_t content_len = result.index_of_closing_symbol - content_start;
 * }
 * @endcode
 */
ak_scanner_find_group_result_t ak_scanner_find_group(ak_scanner_t *scanner,
                                                     uint8_t must_start_with,
                                                     uint8_t must_end_with,
                                                     uint8_t *can_escape_with,
                                                     bool consume_leading_ws);

/**
 * @brief Skip to next non-whitespace byte
 *
 * Advances scanner position past all whitespace characters.
 *
 * @param scanner Scanner to advance
 * @return True if non-whitespace found, false if end of buffer reached
 *
 * @notthreadsafe
 */
bool ak_scanner_goto_next_non_white(ak_scanner_t *scanner);

/**
 * @brief Skip whitespace and comments
 *
 * Advances scanner position past whitespace and comment blocks.
 *
 * @param scanner Scanner to advance
 * @return True if content found, false if end of buffer reached
 *
 * @notthreadsafe
 */
bool ak_scanner_skip_whitespace_and_comments(ak_scanner_t *scanner);

/**
 * @brief Skip to next occurrence of target byte
 *
 * Advances scanner position to the next occurrence of the specified byte.
 *
 * @param scanner Scanner to advance
 * @param target_byte Byte to search for
 * @return True if target found, false if end of buffer reached
 *
 * @notthreadsafe
 */
bool ak_scanner_goto_next_target(ak_scanner_t *scanner, uint8_t target_byte);

#endif
