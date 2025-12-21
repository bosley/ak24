#include "scanner.h"
#include "test/assert.h"
#include <stdlib.h>
#include <string.h>

// =============================================================================
// UTF-8 Symbol Parsing Tests
// =============================================================================

static int test_scanner_utf8_simple_symbol(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  AK24_TEST_ASSERT(buffer != NULL);

  // "café" - 5 bytes, 4 characters
  uint8_t data[] = "café";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);
  AK24_TEST_ASSERT(result.data.byte_length == 5); // 5 bytes total
  AK24_TEST_ASSERT(memcmp(result.data.data, "café", 5) == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_scanner_utf8_cjk_symbol(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  AK24_TEST_ASSERT(buffer != NULL);

  // "日本語" - 9 bytes, 3 characters
  uint8_t data[] = "日本語";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);
  AK24_TEST_ASSERT(result.data.byte_length == 9); // 9 bytes
  AK24_TEST_ASSERT(memcmp(result.data.data, "日本語", 9) == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_scanner_utf8_emoji_symbol(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  AK24_TEST_ASSERT(buffer != NULL);

  // "🎉test" - emoji (4 bytes) + "test" (4 bytes) = 8 bytes total
  uint8_t data[] = "🎉test";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);
  AK24_TEST_ASSERT(result.data.byte_length == 8);
  AK24_TEST_ASSERT(memcmp(result.data.data, "🎉test", 8) == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_scanner_utf8_mixed_symbols(void) {
  ak_buffer_t *buffer = ak_buffer_new(128);
  AK24_TEST_ASSERT(buffer != NULL);

  // "hello café 日本 🎉"
  uint8_t data[] = "hello café 日本 🎉";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  // First symbol: "hello"
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);
  AK24_TEST_ASSERT(memcmp(result.data.data, "hello", 5) == 0);

  // Second symbol: "café"
  result = ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);
  AK24_TEST_ASSERT(memcmp(result.data.data, "café", 5) == 0);

  // Third symbol: "日本"
  result = ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);
  AK24_TEST_ASSERT(memcmp(result.data.data, "日本", 6) == 0);

  // Fourth symbol: "🎉"
  result = ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);
  AK24_TEST_ASSERT(memcmp(result.data.data, "🎉", 4) == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

// =============================================================================
// UTF-8 Whitespace Handling Tests
// =============================================================================

static int test_scanner_utf8_unicode_whitespace(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  AK24_TEST_ASSERT(buffer != NULL);

  // Using ideographic space (U+3000) between words
  // "hello\u3000world" where \u3000 is 0xE3 0x80 0x80
  uint8_t data[] = "hello\xE3\x80\x80world";
  ak_buffer_copy_to(buffer, data, 14);

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  // First symbol: "hello"
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);
  AK24_TEST_ASSERT(memcmp(result.data.data, "hello", 5) == 0);

  // Should skip ideographic space and get "world"
  result = ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);
  AK24_TEST_ASSERT(memcmp(result.data.data, "world", 5) == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_scanner_utf8_goto_next_non_white(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  AK24_TEST_ASSERT(buffer != NULL);

  // Mix of ASCII and Unicode whitespace
  uint8_t data[] = " \t\xE3\x80\x80test"; // space, tab, ideographic space, test
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  bool result = ak_scanner_goto_next_non_white(scanner);
  AK24_TEST_ASSERT(result == true);
  // Should be at 't' in "test"
  AK24_TEST_ASSERT(scanner->position == 5); // 1 + 1 + 3 = 5 bytes

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

// =============================================================================
// UTF-8 Integer and Real Parsing Tests (ensure UTF-8 doesn't break ASCII)
// =============================================================================

static int test_scanner_utf8_integer_with_unicode_context(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  AK24_TEST_ASSERT(buffer != NULL);

  // "café 123 日本"
  uint8_t data[] = "café 123 日本";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  // Skip "café"
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);

  // Parse "123"
  result = ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_INTEGER);
  AK24_TEST_ASSERT(memcmp(result.data.data, "123", 3) == 0);

  // Skip "日本"
  result = ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_scanner_utf8_real_with_unicode_context(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  AK24_TEST_ASSERT(buffer != NULL);

  // "🎉 3.14 test"
  uint8_t data[] = "🎉 3.14 test";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  // Skip emoji
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);

  // Parse "3.14"
  result = ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_REAL);
  AK24_TEST_ASSERT(memcmp(result.data.data, "3.14", 4) == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

// =============================================================================
// UTF-8 Edge Cases and Backward Compatibility Tests
// =============================================================================

static int test_scanner_ascii_still_works(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  AK24_TEST_ASSERT(buffer != NULL);

  uint8_t data[] = "hello 123 world 3.14";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  ak_scanner_static_type_result_t result;

  // "hello"
  result = ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);

  // "123"
  result = ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_INTEGER);

  // "world"
  result = ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);

  // "3.14"
  result = ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_REAL);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_scanner_utf8_complex_mixed_content(void) {
  ak_buffer_t *buffer = ak_buffer_new(256);
  AK24_TEST_ASSERT(buffer != NULL);

  // Complex mix: ASCII, numbers, UTF-8 symbols from different scripts
  // "Hello 42 café -123 日本語 +3.14 🎉world"
  uint8_t data[] = "Hello 42 café -123 日本語 +3.14 🎉world";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  ak_scanner_static_type_result_t result;

  // "Hello" - ASCII symbol
  result = ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);

  // "42" - positive integer
  result = ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_INTEGER);

  // "café" - UTF-8 symbol
  result = ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);

  // "-123" - negative integer
  result = ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_INTEGER);

  // "日本語" - CJK symbol
  result = ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);

  // "+3.14" - positive real
  result = ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_REAL);

  // "🎉world" - emoji + ASCII
  result = ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result.success == true);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

// =============================================================================
// Test Runner
// =============================================================================

int main(void) {
  // UTF-8 Symbol Tests
  AK24_TEST_RUN(test_scanner_utf8_simple_symbol);
  AK24_TEST_RUN(test_scanner_utf8_cjk_symbol);
  AK24_TEST_RUN(test_scanner_utf8_emoji_symbol);
  AK24_TEST_RUN(test_scanner_utf8_mixed_symbols);

  // UTF-8 Whitespace Tests
  AK24_TEST_RUN(test_scanner_utf8_unicode_whitespace);
  AK24_TEST_RUN(test_scanner_utf8_goto_next_non_white);

  // UTF-8 with Numbers Tests
  AK24_TEST_RUN(test_scanner_utf8_integer_with_unicode_context);
  AK24_TEST_RUN(test_scanner_utf8_real_with_unicode_context);

  // Edge Cases and Compatibility
  AK24_TEST_RUN(test_scanner_ascii_still_works);
  AK24_TEST_RUN(test_scanner_utf8_complex_mixed_content);

  return 0;
}
