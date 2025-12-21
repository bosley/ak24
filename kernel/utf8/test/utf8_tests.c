#include "test/assert.h"
#include "utf8.h"
#include <string.h>

// =============================================================================
// UTF-8 Character Length Tests
// =============================================================================

static int test_utf8_char_len_ascii(void) {
  const uint8_t *text = (uint8_t *)"Hello";
  AK24_TEST_ASSERT(ak_utf8_char_len(text, 5) == 1);     // 'H'
  AK24_TEST_ASSERT(ak_utf8_char_len(text + 1, 4) == 1); // 'e'
  AK24_TEST_PASS();
}

static int test_utf8_char_len_two_byte(void) {
  const uint8_t *text = (uint8_t *)"café";              // é is 0xC3 0xA9
  AK24_TEST_ASSERT(ak_utf8_char_len(text + 3, 2) == 2); // 'é'
  AK24_TEST_PASS();
}

static int test_utf8_char_len_three_byte(void) {
  const uint8_t *text = (uint8_t *)"日本語";            // Each char is 3 bytes
  AK24_TEST_ASSERT(ak_utf8_char_len(text, 9) == 3);     // '日'
  AK24_TEST_ASSERT(ak_utf8_char_len(text + 3, 6) == 3); // '本'
  AK24_TEST_ASSERT(ak_utf8_char_len(text + 6, 3) == 3); // '語'
  AK24_TEST_PASS();
}

static int test_utf8_char_len_four_byte(void) {
  const uint8_t *text = (uint8_t *)"🎉🎊";              // Each emoji is 4 bytes
  AK24_TEST_ASSERT(ak_utf8_char_len(text, 8) == 4);     // '🎉'
  AK24_TEST_ASSERT(ak_utf8_char_len(text + 4, 4) == 4); // '🎊'
  AK24_TEST_PASS();
}

static int test_utf8_char_len_mixed(void) {
  const uint8_t *text = (uint8_t *)"A日é🎉"; // ASCII, 3-byte, 2-byte, 4-byte
  AK24_TEST_ASSERT(ak_utf8_char_len(text, 10) == 1);    // 'A'
  AK24_TEST_ASSERT(ak_utf8_char_len(text + 1, 9) == 3); // '日'
  AK24_TEST_ASSERT(ak_utf8_char_len(text + 4, 6) == 2); // 'é'
  AK24_TEST_ASSERT(ak_utf8_char_len(text + 6, 4) == 4); // '🎉'
  AK24_TEST_PASS();
}

static int test_utf8_char_len_invalid(void) {
  // Invalid continuation byte at start
  const uint8_t invalid[] = {0x80, 0x00};
  AK24_TEST_ASSERT(ak_utf8_char_len(invalid, 2) == 1); // Treat as single byte

  // Incomplete sequence
  const uint8_t incomplete[] = {0xC3, 0x00}; // Missing continuation
  AK24_TEST_ASSERT(ak_utf8_char_len(incomplete, 2) == 1);

  AK24_TEST_PASS();
}

// =============================================================================
// UTF-8 Decode Tests
// =============================================================================

static int test_utf8_decode_ascii(void) {
  const uint8_t *text = (uint8_t *)"A";
  ak_utf8_decode_result_t result = ak_utf8_decode(text, 1);

  AK24_TEST_ASSERT(result.valid == true);
  AK24_TEST_ASSERT(result.codepoint == 0x41); // 'A'
  AK24_TEST_ASSERT(result.bytes == 1);
  AK24_TEST_PASS();
}

static int test_utf8_decode_two_byte(void) {
  const uint8_t *text = (uint8_t *)"é"; // U+00E9 = 0xC3 0xA9
  ak_utf8_decode_result_t result = ak_utf8_decode(text, 2);

  AK24_TEST_ASSERT(result.valid == true);
  AK24_TEST_ASSERT(result.codepoint == 0x00E9);
  AK24_TEST_ASSERT(result.bytes == 2);
  AK24_TEST_PASS();
}

static int test_utf8_decode_three_byte(void) {
  const uint8_t *text = (uint8_t *)"日"; // U+65E5
  ak_utf8_decode_result_t result = ak_utf8_decode(text, 3);

  AK24_TEST_ASSERT(result.valid == true);
  AK24_TEST_ASSERT(result.codepoint == 0x65E5);
  AK24_TEST_ASSERT(result.bytes == 3);
  AK24_TEST_PASS();
}

static int test_utf8_decode_four_byte(void) {
  const uint8_t *text = (uint8_t *)"🎉"; // U+1F389
  ak_utf8_decode_result_t result = ak_utf8_decode(text, 4);

  AK24_TEST_ASSERT(result.valid == true);
  AK24_TEST_ASSERT(result.codepoint == 0x1F389);
  AK24_TEST_ASSERT(result.bytes == 4);
  AK24_TEST_PASS();
}

static int test_utf8_decode_invalid_continuation(void) {
  const uint8_t invalid[] = {0x80}; // Continuation byte at start
  ak_utf8_decode_result_t result = ak_utf8_decode(invalid, 1);

  AK24_TEST_ASSERT(result.valid == false);
  AK24_TEST_ASSERT(result.bytes == 1);
  AK24_TEST_PASS();
}

// =============================================================================
// UTF-8 Character Count Tests
// =============================================================================

static int test_utf8_char_count_ascii(void) {
  const uint8_t *text = (uint8_t *)"Hello World";
  size_t count = ak_utf8_char_count(text, strlen((char *)text));
  AK24_TEST_ASSERT(count == 11); // 11 ASCII characters
  AK24_TEST_PASS();
}

static int test_utf8_char_count_mixed(void) {
  const uint8_t *text = (uint8_t *)"café"; // 5 bytes, 4 characters
  size_t count = ak_utf8_char_count(text, strlen((char *)text));
  AK24_TEST_ASSERT(count == 4);
  AK24_TEST_PASS();
}

static int test_utf8_char_count_multibyte(void) {
  const uint8_t *text = (uint8_t *)"日本語"; // 9 bytes, 3 characters
  size_t count = ak_utf8_char_count(text, strlen((char *)text));
  AK24_TEST_ASSERT(count == 3);
  AK24_TEST_PASS();
}

static int test_utf8_char_count_emoji(void) {
  const uint8_t *text = (uint8_t *)"🎉🎊🎈"; // 12 bytes, 3 characters
  size_t count = ak_utf8_char_count(text, strlen((char *)text));
  AK24_TEST_ASSERT(count == 3);
  AK24_TEST_PASS();
}

static int test_utf8_char_count_complex(void) {
  const uint8_t *text =
      (uint8_t *)"Hello 世界 café 🎉"; // Mix of ASCII, CJK, Latin, emoji
  size_t count = ak_utf8_char_count(text, strlen((char *)text));
  // H e l l o space 世 界 space c a f é space 🎉
  // 5 + 1 + 2 + 1 + 4 + 1 + 1 = 15 characters
  AK24_TEST_ASSERT(count == 15);
  AK24_TEST_PASS();
}

// =============================================================================
// UTF-8 Whitespace Tests
// =============================================================================

static int test_utf8_is_whitespace_ascii(void) {
  AK24_TEST_ASSERT(ak_utf8_is_whitespace(0x20) == true);  // Space
  AK24_TEST_ASSERT(ak_utf8_is_whitespace(0x09) == true);  // Tab
  AK24_TEST_ASSERT(ak_utf8_is_whitespace(0x0A) == true);  // LF
  AK24_TEST_ASSERT(ak_utf8_is_whitespace(0x0D) == true);  // CR
  AK24_TEST_ASSERT(ak_utf8_is_whitespace(0x41) == false); // 'A'
  AK24_TEST_PASS();
}

static int test_utf8_is_whitespace_unicode(void) {
  AK24_TEST_ASSERT(ak_utf8_is_whitespace(0x00A0) == true); // No-break space
  AK24_TEST_ASSERT(ak_utf8_is_whitespace(0x2003) == true); // Em space
  AK24_TEST_ASSERT(ak_utf8_is_whitespace(0x3000) == true); // Ideographic space
  AK24_TEST_PASS();
}

// =============================================================================
// UTF-8 Character Classification Tests
// =============================================================================

static int test_utf8_is_digit(void) {
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x30) == true);  // '0'
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x39) == true);  // '9'
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x35) == true);  // '5'
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x41) == false); // 'A'
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x2F) == false); // '/' (before '0')
  AK24_TEST_ASSERT(ak_utf8_is_digit(0x3A) == false); // ':' (after '9')
  AK24_TEST_PASS();
}

static int test_utf8_is_alpha(void) {
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x41) == true);  // 'A'
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x5A) == true);  // 'Z'
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x61) == true);  // 'a'
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x7A) == true);  // 'z'
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x30) == false); // '0'
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x40) == false); // '@' (before 'A')
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x5B) == false); // '[' (after 'Z')
  AK24_TEST_ASSERT(ak_utf8_is_alpha(0x60) == false); // '`' (before 'a')
  AK24_TEST_PASS();
}

static int test_utf8_is_alnum(void) {
  AK24_TEST_ASSERT(ak_utf8_is_alnum(0x41) == true);  // 'A'
  AK24_TEST_ASSERT(ak_utf8_is_alnum(0x61) == true);  // 'a'
  AK24_TEST_ASSERT(ak_utf8_is_alnum(0x30) == true);  // '0'
  AK24_TEST_ASSERT(ak_utf8_is_alnum(0x20) == false); // Space
  AK24_TEST_PASS();
}

// =============================================================================
// UTF-8 Byte Classification Tests
// =============================================================================

static int test_utf8_is_start_byte(void) {
  AK24_TEST_ASSERT(ak_utf8_is_start_byte(0x41) == true);  // ASCII 'A'
  AK24_TEST_ASSERT(ak_utf8_is_start_byte(0xC3) == true);  // 2-byte start
  AK24_TEST_ASSERT(ak_utf8_is_start_byte(0xE6) == true);  // 3-byte start
  AK24_TEST_ASSERT(ak_utf8_is_start_byte(0xF0) == true);  // 4-byte start
  AK24_TEST_ASSERT(ak_utf8_is_start_byte(0x80) == false); // Continuation
  AK24_TEST_ASSERT(ak_utf8_is_start_byte(0xBF) == false); // Continuation
  AK24_TEST_PASS();
}

static int test_utf8_is_continuation_byte(void) {
  AK24_TEST_ASSERT(ak_utf8_is_continuation_byte(0x80) == true);
  AK24_TEST_ASSERT(ak_utf8_is_continuation_byte(0xBF) == true);
  AK24_TEST_ASSERT(ak_utf8_is_continuation_byte(0xA9) == true);
  AK24_TEST_ASSERT(ak_utf8_is_continuation_byte(0x41) == false); // ASCII
  AK24_TEST_ASSERT(ak_utf8_is_continuation_byte(0xC3) == false); // Start byte
  AK24_TEST_PASS();
}

// =============================================================================
// UTF-8 Navigation Tests
// =============================================================================

static int test_utf8_prev_char_ascii(void) {
  const uint8_t *text = (uint8_t *)"Hello";
  AK24_TEST_ASSERT(ak_utf8_prev_char(text, 5) == 4);
  AK24_TEST_ASSERT(ak_utf8_prev_char(text, 4) == 3);
  AK24_TEST_ASSERT(ak_utf8_prev_char(text, 1) == 0);
  AK24_TEST_ASSERT(ak_utf8_prev_char(text, 0) == 0);
  AK24_TEST_PASS();
}

static int test_utf8_prev_char_multibyte(void) {
  const uint8_t *text = (uint8_t *)"café"; // c a f é (5 bytes)
  // Positions: c=0, a=1, f=2, é=3-4
  AK24_TEST_ASSERT(ak_utf8_prev_char(text, 5) == 3); // From end to 'é'
  AK24_TEST_ASSERT(ak_utf8_prev_char(text, 3) == 2); // From 'é' to 'f'
  AK24_TEST_ASSERT(ak_utf8_prev_char(text, 2) == 1); // From 'f' to 'a'
  AK24_TEST_PASS();
}

// =============================================================================
// UTF-8 Validation Tests
// =============================================================================

static int test_utf8_validate_valid_ascii(void) {
  const uint8_t *text = (uint8_t *)"Hello World";
  AK24_TEST_ASSERT(ak_utf8_validate(text, strlen((char *)text)) == true);
  AK24_TEST_PASS();
}

static int test_utf8_validate_valid_multibyte(void) {
  const uint8_t *text = (uint8_t *)"Hello 世界 café 🎉";
  AK24_TEST_ASSERT(ak_utf8_validate(text, strlen((char *)text)) == true);
  AK24_TEST_PASS();
}

static int test_utf8_validate_invalid_continuation(void) {
  const uint8_t invalid[] = {0x48, 0x65, 0x80, 0x6C}; // "He<invalid>l"
  AK24_TEST_ASSERT(ak_utf8_validate(invalid, 4) == false);
  AK24_TEST_PASS();
}

static int test_utf8_validate_incomplete_sequence(void) {
  const uint8_t invalid[] = {0x48, 0xC3}; // "H<incomplete>"
  AK24_TEST_ASSERT(ak_utf8_validate(invalid, 2) == false);
  AK24_TEST_PASS();
}

// =============================================================================
// Test Runner
// =============================================================================

int main(void) {
  // Character length tests
  AK24_TEST_RUN(test_utf8_char_len_ascii);
  AK24_TEST_RUN(test_utf8_char_len_two_byte);
  AK24_TEST_RUN(test_utf8_char_len_three_byte);
  AK24_TEST_RUN(test_utf8_char_len_four_byte);
  AK24_TEST_RUN(test_utf8_char_len_mixed);
  AK24_TEST_RUN(test_utf8_char_len_invalid);

  // Decode tests
  AK24_TEST_RUN(test_utf8_decode_ascii);
  AK24_TEST_RUN(test_utf8_decode_two_byte);
  AK24_TEST_RUN(test_utf8_decode_three_byte);
  AK24_TEST_RUN(test_utf8_decode_four_byte);
  AK24_TEST_RUN(test_utf8_decode_invalid_continuation);

  // Character count tests
  AK24_TEST_RUN(test_utf8_char_count_ascii);
  AK24_TEST_RUN(test_utf8_char_count_mixed);
  AK24_TEST_RUN(test_utf8_char_count_multibyte);
  AK24_TEST_RUN(test_utf8_char_count_emoji);
  AK24_TEST_RUN(test_utf8_char_count_complex);

  // Whitespace tests
  AK24_TEST_RUN(test_utf8_is_whitespace_ascii);
  AK24_TEST_RUN(test_utf8_is_whitespace_unicode);

  // Character classification tests
  AK24_TEST_RUN(test_utf8_is_digit);
  AK24_TEST_RUN(test_utf8_is_alpha);
  AK24_TEST_RUN(test_utf8_is_alnum);

  // Byte classification tests
  AK24_TEST_RUN(test_utf8_is_start_byte);
  AK24_TEST_RUN(test_utf8_is_continuation_byte);

  // Navigation tests
  AK24_TEST_RUN(test_utf8_prev_char_ascii);
  AK24_TEST_RUN(test_utf8_prev_char_multibyte);

  // Validation tests
  AK24_TEST_RUN(test_utf8_validate_valid_ascii);
  AK24_TEST_RUN(test_utf8_validate_valid_multibyte);
  AK24_TEST_RUN(test_utf8_validate_invalid_continuation);
  AK24_TEST_RUN(test_utf8_validate_incomplete_sequence);

  return 0;
}
