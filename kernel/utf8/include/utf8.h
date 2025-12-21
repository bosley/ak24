/**
 * @file utf8.h
 * @brief UTF-8 character encoding utilities
 *
 * Provides utilities for working with UTF-8 encoded text, including:
 * - Character boundary detection
 * - Codepoint extraction and decoding
 * - Character classification (whitespace, digits, alphanumeric)
 * - Safe iteration over UTF-8 sequences
 * - ASCII compatibility layer
 *
 * All functions handle both UTF-8 multi-byte sequences and ASCII (single-byte)
 * characters correctly. Invalid UTF-8 sequences are treated as single bytes.
 *
 * @note All functions are NOT thread-safe
 * @note Assumes input is UTF-8 encoded (or ASCII, which is UTF-8 compatible)
 */

#ifndef AK24_UTF8_H
#define AK24_UTF8_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief UTF-8 codepoint value
 *
 * Represents a single Unicode character as a 32-bit codepoint.
 * Valid range: 0x0000 to 0x10FFFF
 */
typedef uint32_t ak_utf8_codepoint_t;

/**
 * @brief UTF-8 character decode result
 *
 * Contains the decoded codepoint and number of bytes consumed.
 */
typedef struct {
  ak_utf8_codepoint_t codepoint; /**< Decoded Unicode codepoint */
  size_t bytes;                  /**< Number of bytes in sequence (1-4) */
  bool valid;                    /**< Whether sequence is valid UTF-8 */
} ak_utf8_decode_result_t;

/**
 * @brief Get byte length of UTF-8 character at position
 *
 * Returns the number of bytes in the UTF-8 character starting at the
 * given position. Handles 1-4 byte sequences.
 *
 * @param data Pointer to byte sequence
 * @param max_bytes Maximum bytes available to read
 * @return Number of bytes (1-4), or 1 for invalid sequences
 *
 * @notthreadsafe
 *
 * @par Example:
 * @code
 * const uint8_t *text = (uint8_t *)"café";
 * size_t len = ak_utf8_char_len(text + 3, 2); // 'é' = 2 bytes
 * @endcode
 */
size_t ak_utf8_char_len(const uint8_t *data, size_t max_bytes);

/**
 * @brief Decode UTF-8 character to codepoint
 *
 * Extracts the Unicode codepoint from a UTF-8 byte sequence.
 * Validates the sequence and returns error status for invalid input.
 *
 * @param data Pointer to byte sequence
 * @param max_bytes Maximum bytes available to read
 * @return Decode result with codepoint, byte count, and validity
 *
 * @notthreadsafe
 *
 * @par Example:
 * @code
 * const uint8_t *text = (uint8_t *)"日本語";
 * ak_utf8_decode_result_t result = ak_utf8_decode(text, 3);
 * if (result.valid) {
 *   printf("Codepoint: U+%04X, bytes: %zu\n",
 *          result.codepoint, result.bytes);
 * }
 * @endcode
 */
ak_utf8_decode_result_t ak_utf8_decode(const uint8_t *data, size_t max_bytes);

/**
 * @brief Count UTF-8 characters in byte sequence
 *
 * Counts the number of UTF-8 codepoints (characters) in the given
 * byte sequence. Multi-byte characters count as one.
 *
 * @param data Pointer to byte sequence
 * @param byte_len Length in bytes
 * @return Number of UTF-8 characters
 *
 * @notthreadsafe
 *
 * @par Example:
 * @code
 * const uint8_t *text = (uint8_t *)"café"; // 5 bytes
 * size_t chars = ak_utf8_char_count(text, 5); // Returns 4
 * @endcode
 */
size_t ak_utf8_char_count(const uint8_t *data, size_t byte_len);

/**
 * @brief Check if codepoint is whitespace
 *
 * Returns true for ASCII whitespace (space, tab, newline, carriage return)
 * and common Unicode whitespace characters.
 *
 * Supported whitespace:
 * - ASCII: space (0x20), tab (0x09), LF (0x0A), CR (0x0D)
 * - Unicode: non-breaking space (U+00A0), en space (U+2002),
 *   em space (U+2003), thin space (U+2009), etc.
 *
 * @param codepoint Unicode codepoint
 * @return True if whitespace, false otherwise
 *
 * @notthreadsafe
 */
bool ak_utf8_is_whitespace(ak_utf8_codepoint_t codepoint);

/**
 * @brief Check if codepoint is a digit
 *
 * Returns true for digits from any Unicode script including:
 * - ASCII digits (0-9)
 * - Arabic-Indic digits (٠-٩)
 * - Devanagari digits (०-९)
 * - Bengali, Thai, Khmer, and many other numeric scripts
 *
 * @param codepoint Unicode codepoint
 * @return True if digit from any script, false otherwise
 *
 * @notthreadsafe
 */
bool ak_utf8_is_digit(ak_utf8_codepoint_t codepoint);

/**
 * @brief Check if codepoint is a letter
 *
 * Returns true for letters from any Unicode script including:
 * - ASCII letters (A-Z, a-z)
 * - Latin Extended (À-ÿ, etc.)
 * - Greek and Coptic (Α-ω)
 * - Cyrillic (А-я)
 * - Arabic, Hebrew, Devanagari
 * - CJK Unified Ideographs (一, 中, etc.)
 * - Hiragana (あ-ん), Katakana (ア-ン)
 * - Hangul (가-힣)
 * - And many other scripts
 *
 * @param codepoint Unicode codepoint
 * @return True if letter from any script, false otherwise
 *
 * @notthreadsafe
 */
bool ak_utf8_is_alpha(ak_utf8_codepoint_t codepoint);

/**
 * @brief Check if codepoint is ASCII alphanumeric
 *
 * Returns true for ASCII letters or digits.
 *
 * @param codepoint Unicode codepoint
 * @return True if ASCII letter or digit, false otherwise
 *
 * @notthreadsafe
 */
bool ak_utf8_is_alnum(ak_utf8_codepoint_t codepoint);

/**
 * @brief Check if byte is valid UTF-8 start byte
 *
 * Returns true if the byte can start a valid UTF-8 sequence.
 * Returns false for continuation bytes (10xxxxxx) and invalid bytes.
 *
 * @param byte Byte to check
 * @return True if valid UTF-8 start byte
 *
 * @notthreadsafe
 */
bool ak_utf8_is_start_byte(uint8_t byte);

/**
 * @brief Check if byte is UTF-8 continuation byte
 *
 * Returns true if the byte is a UTF-8 continuation byte (10xxxxxx).
 *
 * @param byte Byte to check
 * @return True if continuation byte
 *
 * @notthreadsafe
 */
bool ak_utf8_is_continuation_byte(uint8_t byte);

/**
 * @brief Find start of previous UTF-8 character
 *
 * Scans backwards from the current position to find the start of
 * the previous UTF-8 character. Useful for reverse iteration.
 *
 * @param data Pointer to byte sequence
 * @param current_pos Current position in sequence
 * @return Position of previous character start, or current_pos if at start
 *
 * @notthreadsafe
 */
size_t ak_utf8_prev_char(const uint8_t *data, size_t current_pos);

/**
 * @brief Validate UTF-8 byte sequence
 *
 * Checks if the byte sequence is valid UTF-8 encoding.
 *
 * @param data Pointer to byte sequence
 * @param byte_len Length in bytes
 * @return True if valid UTF-8, false otherwise
 *
 * @notthreadsafe
 */
bool ak_utf8_validate(const uint8_t *data, size_t byte_len);

/**
 * @brief Check if codepoint is a combining character
 *
 * Returns true for Unicode combining marks that modify the previous
 * base character (accents, diacritics, etc.).
 *
 * @param codepoint Unicode codepoint
 * @return True if combining character, false otherwise
 *
 * @notthreadsafe
 */
bool ak_utf8_is_combining_mark(ak_utf8_codepoint_t codepoint);

/**
 * @brief Count grapheme clusters in UTF-8 text
 *
 * Counts grapheme clusters (user-perceived characters) rather than
 * codepoints. For example, "é" composed of 'e' + combining accent
 * counts as 1 grapheme cluster, not 2 codepoints.
 *
 * @param data Pointer to byte sequence
 * @param byte_len Length in bytes
 * @return Number of grapheme clusters
 *
 * @notthreadsafe
 */
size_t ak_utf8_grapheme_count(const uint8_t *data, size_t byte_len);

/**
 * @brief Get byte length of next grapheme cluster
 *
 * Returns the number of bytes in the next grapheme cluster starting
 * at the given position. A grapheme cluster may consist of a base
 * character followed by zero or more combining marks.
 *
 * @param data Pointer to byte sequence
 * @param max_bytes Maximum bytes available to read
 * @return Number of bytes in grapheme cluster
 *
 * @notthreadsafe
 */
size_t ak_utf8_grapheme_len(const uint8_t *data, size_t max_bytes);

#endif // AK24_UTF8_H
