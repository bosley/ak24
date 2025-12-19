#include "scanner.h"
#include "test/assert.h"
#include <stdlib.h>
#include <string.h>

static int test_scanner_new_valid_position(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  AK24_TEST_ASSERT(buffer != NULL);

  uint8_t data[] = "hello world";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);
  AK24_TEST_ASSERT(scanner->buffer == buffer);
  AK24_TEST_ASSERT(scanner->position == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_scanner_new_mid_position(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  AK24_TEST_ASSERT(buffer != NULL);

  uint8_t data[] = "hello world";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 6);
  AK24_TEST_ASSERT(scanner != NULL);
  AK24_TEST_ASSERT(scanner->buffer == buffer);
  AK24_TEST_ASSERT(scanner->position == 6);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_scanner_new_end_position(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  AK24_TEST_ASSERT(buffer != NULL);

  uint8_t data[] = "hello world";
  size_t len = strlen((char *)data);
  ak_buffer_copy_to(buffer, data, len);

  ak_scanner_t *scanner = ak_scanner_new(buffer, len);
  AK24_TEST_ASSERT(scanner != NULL);
  AK24_TEST_ASSERT(scanner->buffer == buffer);
  AK24_TEST_ASSERT(scanner->position == len);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_scanner_new_invalid_position(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  AK24_TEST_ASSERT(buffer != NULL);

  uint8_t data[] = "hello world";
  size_t len = strlen((char *)data);
  ak_buffer_copy_to(buffer, data, len);

  ak_scanner_t *scanner = ak_scanner_new(buffer, len + 1);
  AK24_TEST_ASSERT(scanner == NULL);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_scanner_new_null_buffer(void) {
  ak_scanner_t *scanner = ak_scanner_new(NULL, 0);
  AK24_TEST_ASSERT(scanner == NULL);
  AK24_TEST_PASS();
}

static int test_scanner_new_empty_buffer(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  AK24_TEST_ASSERT(buffer != NULL);

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);
  AK24_TEST_ASSERT(scanner->buffer == buffer);
  AK24_TEST_ASSERT(scanner->position == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_scanner_free_null(void) {
  ak_scanner_free(NULL);
  AK24_TEST_PASS();
}

static int test_scanner_does_not_own_buffer(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  AK24_TEST_ASSERT(buffer != NULL);

  uint8_t data[] = "test data";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  ak_scanner_free(scanner);

  AK24_TEST_ASSERT(buffer->data != NULL);
  AK24_TEST_ASSERT(buffer->count == strlen((char *)data));

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_simple_symbol(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "hello";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);
  AK24_TEST_ASSERT(result.data.byte_length == 5);
  AK24_TEST_ASSERT(memcmp(result.data.data, "hello", 5) == 0);
  AK24_TEST_ASSERT(scanner->position == 5);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_simple_integer(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "42";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_INTEGER);
  AK24_TEST_ASSERT(result.data.byte_length == 2);
  AK24_TEST_ASSERT(memcmp(result.data.data, "42", 2) == 0);
  AK24_TEST_ASSERT(scanner->position == 2);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_simple_real(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "3.14";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_REAL);
  AK24_TEST_ASSERT(result.data.byte_length == 4);
  AK24_TEST_ASSERT(memcmp(result.data.data, "3.14", 4) == 0);
  AK24_TEST_ASSERT(scanner->position == 4);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_multiple_tokens(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "a +1 3.13";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);

  ak_scanner_static_type_result_t result1 =
      ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result1.success);
  AK24_TEST_ASSERT(result1.data.base == AK24_STATIC_BASE_SYMBOL);
  AK24_TEST_ASSERT(result1.data.byte_length == 1);
  AK24_TEST_ASSERT(memcmp(result1.data.data, "a", 1) == 0);

  ak_scanner_static_type_result_t result2 =
      ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result2.success);
  AK24_TEST_ASSERT(result2.data.base == AK24_STATIC_BASE_INTEGER);
  AK24_TEST_ASSERT(result2.data.byte_length == 2);
  AK24_TEST_ASSERT(memcmp(result2.data.data, "+1", 2) == 0);

  ak_scanner_static_type_result_t result3 =
      ak_scanner_read_static_base_type(scanner, NULL);
  AK24_TEST_ASSERT(result3.success);
  AK24_TEST_ASSERT(result3.data.base == AK24_STATIC_BASE_REAL);
  AK24_TEST_ASSERT(result3.data.byte_length == 4);
  AK24_TEST_ASSERT(memcmp(result3.data.data, "3.13", 4) == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_positive_integer(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "+123";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_INTEGER);
  AK24_TEST_ASSERT(result.data.byte_length == 4);
  AK24_TEST_ASSERT(memcmp(result.data.data, "+123", 4) == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_negative_integer(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "-42";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_INTEGER);
  AK24_TEST_ASSERT(result.data.byte_length == 3);
  AK24_TEST_ASSERT(memcmp(result.data.data, "-42", 3) == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_negative_real(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "-2.5";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_REAL);
  AK24_TEST_ASSERT(result.data.byte_length == 4);
  AK24_TEST_ASSERT(memcmp(result.data.data, "-2.5", 4) == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_sign_as_symbol(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "+a";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);
  AK24_TEST_ASSERT(result.data.byte_length == 2);
  AK24_TEST_ASSERT(memcmp(result.data.data, "+a", 2) == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_leading_whitespace(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "  \t\n42";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_INTEGER);
  AK24_TEST_ASSERT(result.data.byte_length == 2);
  AK24_TEST_ASSERT(memcmp(result.data.data, "42", 2) == 0);
  AK24_TEST_ASSERT(scanner->position == 6);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_whitespace_terminator(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "abc def";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);
  AK24_TEST_ASSERT(result.data.byte_length == 3);
  AK24_TEST_ASSERT(memcmp(result.data.data, "abc", 3) == 0);
  AK24_TEST_ASSERT(scanner->position == 3);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_double_period_error(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "1.11.1";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(!result.success);
  AK24_TEST_ASSERT(scanner->position == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_invalid_integer(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "123x";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(!result.success);
  AK24_TEST_ASSERT(scanner->position == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_invalid_real(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "3.14x";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(!result.success);
  AK24_TEST_ASSERT(scanner->position == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_all_whitespace(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "   \t\n";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(!result.success);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_at_end_of_buffer(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "test";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 4);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(!result.success);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_null_scanner(void) {
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(NULL, NULL);

  AK24_TEST_ASSERT(!result.success);
  AK24_TEST_PASS();
}

static int test_parse_symbol_with_digits(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "var123";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);
  AK24_TEST_ASSERT(result.data.byte_length == 6);
  AK24_TEST_ASSERT(memcmp(result.data.data, "var123", 6) == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_lone_plus(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "+ ";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);
  AK24_TEST_ASSERT(result.data.byte_length == 1);
  AK24_TEST_ASSERT(memcmp(result.data.data, "+", 1) == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_lone_minus(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "-\t";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);
  AK24_TEST_ASSERT(result.data.byte_length == 1);
  AK24_TEST_ASSERT(memcmp(result.data.data, "-", 1) == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_real_with_trailing_digits(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "0.123456789";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_REAL);
  AK24_TEST_ASSERT(result.data.byte_length == 11);
  AK24_TEST_ASSERT(memcmp(result.data.data, "0.123456789", 11) == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_zero(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "0";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_INTEGER);
  AK24_TEST_ASSERT(result.data.byte_length == 1);
  AK24_TEST_ASSERT(memcmp(result.data.data, "0", 1) == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_special_chars_in_symbol(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "foo-bar_baz!";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);
  AK24_TEST_ASSERT(result.data.byte_length == 12);
  AK24_TEST_ASSERT(memcmp(result.data.data, "foo-bar_baz!", 12) == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_with_paren_stop_symbol(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "hello)world";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  uint8_t stop_chars[] = {')', '('};
  ak_scanner_stop_symbols_t stop_symbols = {.symbols = stop_chars, .count = 2};

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, &stop_symbols);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);
  AK24_TEST_ASSERT(result.data.byte_length == 5);
  AK24_TEST_ASSERT(memcmp(result.data.data, "hello", 5) == 0);
  AK24_TEST_ASSERT(scanner->position == 5);
  AK24_TEST_ASSERT(buffer->data[scanner->position] == ')');

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_integer_with_paren_stop(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "42)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  uint8_t stop_chars[] = {')'};
  ak_scanner_stop_symbols_t stop_symbols = {.symbols = stop_chars, .count = 1};

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, &stop_symbols);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_INTEGER);
  AK24_TEST_ASSERT(result.data.byte_length == 2);
  AK24_TEST_ASSERT(memcmp(result.data.data, "42", 2) == 0);
  AK24_TEST_ASSERT(scanner->position == 2);
  AK24_TEST_ASSERT(buffer->data[scanner->position] == ')');

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_real_with_paren_stop(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "3.14)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  uint8_t stop_chars[] = {')'};
  ak_scanner_stop_symbols_t stop_symbols = {.symbols = stop_chars, .count = 1};

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, &stop_symbols);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_REAL);
  AK24_TEST_ASSERT(result.data.byte_length == 4);
  AK24_TEST_ASSERT(memcmp(result.data.data, "3.14", 4) == 0);
  AK24_TEST_ASSERT(scanner->position == 4);
  AK24_TEST_ASSERT(buffer->data[scanner->position] == ')');

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_multiple_tokens_with_stop_symbols(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "(add 42 3.14)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  uint8_t stop_chars[] = {'(', ')'};
  ak_scanner_stop_symbols_t stop_symbols = {.symbols = stop_chars, .count = 2};

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);

  scanner->position = 1;

  ak_scanner_static_type_result_t result1 =
      ak_scanner_read_static_base_type(scanner, &stop_symbols);
  AK24_TEST_ASSERT(result1.success);
  AK24_TEST_ASSERT(result1.data.base == AK24_STATIC_BASE_SYMBOL);
  AK24_TEST_ASSERT(result1.data.byte_length == 3);
  AK24_TEST_ASSERT(memcmp(result1.data.data, "add", 3) == 0);

  ak_scanner_static_type_result_t result2 =
      ak_scanner_read_static_base_type(scanner, &stop_symbols);
  AK24_TEST_ASSERT(result2.success);
  AK24_TEST_ASSERT(result2.data.base == AK24_STATIC_BASE_INTEGER);
  AK24_TEST_ASSERT(result2.data.byte_length == 2);
  AK24_TEST_ASSERT(memcmp(result2.data.data, "42", 2) == 0);

  ak_scanner_static_type_result_t result3 =
      ak_scanner_read_static_base_type(scanner, &stop_symbols);
  AK24_TEST_ASSERT(result3.success);
  AK24_TEST_ASSERT(result3.data.base == AK24_STATIC_BASE_REAL);
  AK24_TEST_ASSERT(result3.data.byte_length == 4);
  AK24_TEST_ASSERT(memcmp(result3.data.data, "3.14", 4) == 0);
  AK24_TEST_ASSERT(scanner->position == 12);
  AK24_TEST_ASSERT(buffer->data[scanner->position] == ')');

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_stop_symbol_at_start(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = ")hello";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  uint8_t stop_chars[] = {')'};
  ak_scanner_stop_symbols_t stop_symbols = {.symbols = stop_chars, .count = 1};

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, &stop_symbols);

  AK24_TEST_ASSERT(!result.success);
  AK24_TEST_ASSERT(scanner->position == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_parse_null_stop_symbols_same_as_before(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "test)data";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);
  AK24_TEST_ASSERT(result.data.byte_length == 9);
  AK24_TEST_ASSERT(memcmp(result.data.data, "test)data", 9) == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int run_scanner_tests(void) {
  AK24_TEST_RUN(test_scanner_new_valid_position);
  AK24_TEST_RUN(test_scanner_new_mid_position);
  AK24_TEST_RUN(test_scanner_new_end_position);
  AK24_TEST_RUN(test_scanner_new_invalid_position);
  AK24_TEST_RUN(test_scanner_new_null_buffer);
  AK24_TEST_RUN(test_scanner_new_empty_buffer);
  AK24_TEST_RUN(test_scanner_free_null);
  AK24_TEST_RUN(test_scanner_does_not_own_buffer);

  AK24_TEST_RUN(test_parse_simple_symbol);
  AK24_TEST_RUN(test_parse_simple_integer);
  AK24_TEST_RUN(test_parse_simple_real);
  AK24_TEST_RUN(test_parse_multiple_tokens);
  AK24_TEST_RUN(test_parse_positive_integer);
  AK24_TEST_RUN(test_parse_negative_integer);
  AK24_TEST_RUN(test_parse_negative_real);
  AK24_TEST_RUN(test_parse_sign_as_symbol);
  AK24_TEST_RUN(test_parse_leading_whitespace);
  AK24_TEST_RUN(test_parse_whitespace_terminator);
  AK24_TEST_RUN(test_parse_double_period_error);
  AK24_TEST_RUN(test_parse_invalid_integer);
  AK24_TEST_RUN(test_parse_invalid_real);
  AK24_TEST_RUN(test_parse_all_whitespace);
  AK24_TEST_RUN(test_parse_at_end_of_buffer);
  AK24_TEST_RUN(test_parse_null_scanner);
  AK24_TEST_RUN(test_parse_symbol_with_digits);
  AK24_TEST_RUN(test_parse_lone_plus);
  AK24_TEST_RUN(test_parse_lone_minus);
  AK24_TEST_RUN(test_parse_real_with_trailing_digits);
  AK24_TEST_RUN(test_parse_zero);
  AK24_TEST_RUN(test_parse_special_chars_in_symbol);

  AK24_TEST_RUN(test_parse_with_paren_stop_symbol);
  AK24_TEST_RUN(test_parse_integer_with_paren_stop);
  AK24_TEST_RUN(test_parse_real_with_paren_stop);
  AK24_TEST_RUN(test_parse_multiple_tokens_with_stop_symbols);
  AK24_TEST_RUN(test_parse_stop_symbol_at_start);
  AK24_TEST_RUN(test_parse_null_stop_symbols_same_as_before);

  return 0;
}
