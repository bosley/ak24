#include "scanner.h"
#include "test/assert.h"
#include <stdlib.h>
#include <string.h>

static int test_find_group_simple_parens(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "(hello)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 6);
  AK24_TEST_ASSERT(scanner->position == 6);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_simple_brackets(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "[data]";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '[', ']', NULL, false);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 5);
  AK24_TEST_ASSERT(scanner->position == 5);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_simple_braces(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "{content}";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '{', '}', NULL, false);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 8);
  AK24_TEST_ASSERT(scanner->position == 8);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_custom_delimiters(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "!a b +1 2$";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '!', '$', NULL, false);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 9);
  AK24_TEST_ASSERT(scanner->position == 9);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_sequential_groups(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "(first)(second)(third)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);

  ak_scanner_find_group_result_t result1 =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);
  AK24_TEST_ASSERT(result1.success);
  AK24_TEST_ASSERT(result1.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result1.index_of_closing_symbol == 6);
  AK24_TEST_ASSERT(scanner->position == 6);

  scanner->position = 7;
  ak_scanner_find_group_result_t result2 =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);
  AK24_TEST_ASSERT(result2.success);
  AK24_TEST_ASSERT(result2.index_of_start_symbol == 7);
  AK24_TEST_ASSERT(result2.index_of_closing_symbol == 14);
  AK24_TEST_ASSERT(scanner->position == 14);

  scanner->position = 15;
  ak_scanner_find_group_result_t result3 =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);
  AK24_TEST_ASSERT(result3.success);
  AK24_TEST_ASSERT(result3.index_of_start_symbol == 15);
  AK24_TEST_ASSERT(result3.index_of_closing_symbol == 21);
  AK24_TEST_ASSERT(scanner->position == 21);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_mixed_delimiters(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "(a)[b]{c}";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);

  ak_scanner_find_group_result_t result1 =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);
  AK24_TEST_ASSERT(result1.success);
  AK24_TEST_ASSERT(result1.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result1.index_of_closing_symbol == 2);

  scanner->position = 3;
  ak_scanner_find_group_result_t result2 =
      ak_scanner_find_group(scanner, '[', ']', NULL, false);
  AK24_TEST_ASSERT(result2.success);
  AK24_TEST_ASSERT(result2.index_of_start_symbol == 3);
  AK24_TEST_ASSERT(result2.index_of_closing_symbol == 5);

  scanner->position = 6;
  ak_scanner_find_group_result_t result3 =
      ak_scanner_find_group(scanner, '{', '}', NULL, false);
  AK24_TEST_ASSERT(result3.success);
  AK24_TEST_ASSERT(result3.index_of_start_symbol == 6);
  AK24_TEST_ASSERT(result3.index_of_closing_symbol == 8);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_different_custom_delimiters(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "!foo$<bar>@baz#";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);

  ak_scanner_find_group_result_t result1 =
      ak_scanner_find_group(scanner, '!', '$', NULL, false);
  AK24_TEST_ASSERT(result1.success);
  AK24_TEST_ASSERT(result1.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result1.index_of_closing_symbol == 4);

  scanner->position = 5;
  ak_scanner_find_group_result_t result2 =
      ak_scanner_find_group(scanner, '<', '>', NULL, false);
  AK24_TEST_ASSERT(result2.success);
  AK24_TEST_ASSERT(result2.index_of_start_symbol == 5);
  AK24_TEST_ASSERT(result2.index_of_closing_symbol == 9);

  scanner->position = 10;
  ak_scanner_find_group_result_t result3 =
      ak_scanner_find_group(scanner, '@', '#', NULL, false);
  AK24_TEST_ASSERT(result3.success);
  AK24_TEST_ASSERT(result3.index_of_start_symbol == 10);
  AK24_TEST_ASSERT(result3.index_of_closing_symbol == 14);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_escaped_quotes(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "\"hello \\\"world\\\"!\"";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  uint8_t escape = '\\';
  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '"', '"', &escape, false);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 17);
  AK24_TEST_ASSERT(scanner->position == 17);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_multiple_escaped_end_symbols(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "(a\\)b\\)c)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  uint8_t escape = '\\';
  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', &escape, false);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 8);
  AK24_TEST_ASSERT(scanner->position == 8);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_escape_at_end_of_buffer(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "(hello\\";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  uint8_t escape = '\\';
  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', &escape, false);

  AK24_TEST_ASSERT(!result.success);
  AK24_TEST_ASSERT(scanner->position == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_escape_followed_by_non_end_symbol(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "(hello\\world)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  uint8_t escape = '\\';
  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', &escape, false);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 12);
  AK24_TEST_ASSERT(scanner->position == 12);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_wrong_start_symbol(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "[hello)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);

  AK24_TEST_ASSERT(!result.success);
  AK24_TEST_ASSERT(scanner->position == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_null_scanner(void) {
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(NULL, '(', ')', NULL, false);

  AK24_TEST_ASSERT(!result.success);
  AK24_TEST_PASS();
}

static int test_find_group_empty_buffer(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);

  AK24_TEST_ASSERT(!result.success);
  AK24_TEST_ASSERT(scanner->position == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_position_at_end(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "(hello)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 7);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);

  AK24_TEST_ASSERT(!result.success);
  AK24_TEST_ASSERT(scanner->position == 7);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_position_not_at_start_symbol(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "x(hello)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);

  AK24_TEST_ASSERT(!result.success);
  AK24_TEST_ASSERT(scanner->position == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_missing_end_symbol(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "(hello world";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);

  AK24_TEST_ASSERT(!result.success);
  AK24_TEST_ASSERT(scanner->position == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_only_start_symbol(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "(";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);

  AK24_TEST_ASSERT(!result.success);
  AK24_TEST_ASSERT(scanner->position == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_all_escaped_no_real_end(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "(hello\\)world\\)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  uint8_t escape = '\\';
  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', &escape, false);

  AK24_TEST_ASSERT(!result.success);
  AK24_TEST_ASSERT(scanner->position == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_same_start_end_symbols(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "|content|";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '|', '|', NULL, false);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 8);
  AK24_TEST_ASSERT(scanner->position == 8);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_empty_group(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "()";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 1);
  AK24_TEST_ASSERT(scanner->position == 1);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_nested_groups(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "(outer(inner))";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 13);
  AK24_TEST_ASSERT(scanner->position == 13);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_no_escape_byte(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "(hello)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 6);
  AK24_TEST_ASSERT(scanner->position == 6);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_complex_content(void) {
  ak_buffer_t *buffer = ak_buffer_new(128);
  uint8_t data[] = "(add 1 2 (mul 3 4) 5)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 20);
  AK24_TEST_ASSERT(scanner->position == 20);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_with_whitespace(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "( hello world )";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 14);
  AK24_TEST_ASSERT(scanner->position == 14);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_deeply_nested(void) {
  ak_buffer_t *buffer = ak_buffer_new(128);
  uint8_t data[] = "(a(b(c(d(e)f)g)h)i)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 18);
  AK24_TEST_ASSERT(scanner->position == 18);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_multiple_groups_in_buffer(void) {
  ak_buffer_t *buffer = ak_buffer_new(256);
  uint8_t data[] =
      "(first) some text (second (nested)) more [different] {another}";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);

  ak_scanner_find_group_result_t result1 =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);
  AK24_TEST_ASSERT(result1.success);
  AK24_TEST_ASSERT(result1.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result1.index_of_closing_symbol == 6);

  scanner->position = 18;
  ak_scanner_find_group_result_t result2 =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);
  AK24_TEST_ASSERT(result2.success);
  AK24_TEST_ASSERT(result2.index_of_start_symbol == 18);
  AK24_TEST_ASSERT(result2.index_of_closing_symbol == 34);

  scanner->position = 41;
  ak_scanner_find_group_result_t result3 =
      ak_scanner_find_group(scanner, '[', ']', NULL, false);
  AK24_TEST_ASSERT(result3.success);
  AK24_TEST_ASSERT(result3.index_of_start_symbol == 41);
  AK24_TEST_ASSERT(result3.index_of_closing_symbol == 51);

  scanner->position = 53;
  ak_scanner_find_group_result_t result4 =
      ak_scanner_find_group(scanner, '{', '}', NULL, false);
  AK24_TEST_ASSERT(result4.success);
  AK24_TEST_ASSERT(result4.index_of_start_symbol == 53);
  AK24_TEST_ASSERT(result4.index_of_closing_symbol == 61);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_escaped_quotes_complex(void) {
  ak_buffer_t *buffer = ak_buffer_new(128);
  uint8_t data[] = "\"start \\\"nested \\\"double\\\" escape\\\" end\"";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  uint8_t escape = '\\';
  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '"', '"', &escape, false);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 39);
  AK24_TEST_ASSERT(scanner->position == 39);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_mixed_nested_with_escapes(void) {
  ak_buffer_t *buffer = ak_buffer_new(128);
  uint8_t data[] = "(outer \"with \\\"quotes\\\" inside\" (inner))";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 39);

  uint8_t escape = '\\';
  scanner->position = 7;
  ak_scanner_find_group_result_t result2 =
      ak_scanner_find_group(scanner, '"', '"', &escape, false);
  AK24_TEST_ASSERT(result2.success);
  AK24_TEST_ASSERT(result2.index_of_start_symbol == 7);
  AK24_TEST_ASSERT(result2.index_of_closing_symbol == 30);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_asymmetric_nesting(void) {
  ak_buffer_t *buffer = ak_buffer_new(128);
  uint8_t data[] = "((()))";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 5);

  scanner->position = 1;
  ak_scanner_find_group_result_t result2 =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);
  AK24_TEST_ASSERT(result2.success);
  AK24_TEST_ASSERT(result2.index_of_start_symbol == 1);
  AK24_TEST_ASSERT(result2.index_of_closing_symbol == 4);

  scanner->position = 2;
  ak_scanner_find_group_result_t result3 =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);
  AK24_TEST_ASSERT(result3.success);
  AK24_TEST_ASSERT(result3.index_of_start_symbol == 2);
  AK24_TEST_ASSERT(result3.index_of_closing_symbol == 3);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_unbalanced_inside_quotes(void) {
  ak_buffer_t *buffer = ak_buffer_new(128);
  uint8_t data[] = "(text \"with ) inside\" more)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 12);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_large_buffer_with_many_groups(void) {
  ak_buffer_t *buffer = ak_buffer_new(512);
  uint8_t data[] = "(a)(b)(c)(d)(e)(f)(g)(h)(i)(j)(k)(l)(m)(n)(o)(p)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);

  for (size_t i = 0; i < 16; i++) {
    scanner->position = i * 3;
    ak_scanner_find_group_result_t result =
        ak_scanner_find_group(scanner, '(', ')', NULL, false);
    AK24_TEST_ASSERT(result.success);
    AK24_TEST_ASSERT(result.index_of_start_symbol == i * 3);
    AK24_TEST_ASSERT(result.index_of_closing_symbol == i * 3 + 2);
  }

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_escape_escape_character(void) {
  ak_buffer_t *buffer = ak_buffer_new(128);
  uint8_t data[] = "(text with \\\\ backslash)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  uint8_t escape = '\\';
  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', &escape, false);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 23);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_find_group_max_depth_stress(void) {
  ak_buffer_t *buffer = ak_buffer_new(512);

  size_t depth = 50;
  for (size_t i = 0; i < depth; i++) {
    ak_buffer_copy_to(buffer, (uint8_t *)"(", 1);
  }
  ak_buffer_copy_to(buffer, (uint8_t *)"x", 1);
  for (size_t i = 0; i < depth; i++) {
    ak_buffer_copy_to(buffer, (uint8_t *)")", 1);
  }

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 100);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_consume_leading_ws_with_spaces(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "   (hello)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, true);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 3);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 9);
  AK24_TEST_ASSERT(scanner->position == 9);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_consume_leading_ws_with_tabs(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "\t\t[data]";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '[', ']', NULL, true);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 2);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 7);
  AK24_TEST_ASSERT(scanner->position == 7);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_consume_leading_ws_with_newlines(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "\n\n\r{content}";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '{', '}', NULL, true);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 3);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 11);
  AK24_TEST_ASSERT(scanner->position == 11);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_consume_leading_ws_mixed_whitespace(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = " \t\n\r  (test)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, true);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 6);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 11);
  AK24_TEST_ASSERT(scanner->position == 11);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_consume_leading_ws_false_fails_on_whitespace(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "   (hello)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, false);

  AK24_TEST_ASSERT(!result.success);
  AK24_TEST_ASSERT(scanner->position == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_consume_leading_ws_all_whitespace_buffer(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "   \t\n\r  ";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, true);

  AK24_TEST_ASSERT(!result.success);
  AK24_TEST_ASSERT(scanner->position == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_consume_leading_ws_no_whitespace(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "(immediate)";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, true);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 0);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 10);
  AK24_TEST_ASSERT(scanner->position == 10);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_consume_leading_ws_nested_groups(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  uint8_t data[] = "  (outer(inner))";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  ak_scanner_find_group_result_t result =
      ak_scanner_find_group(scanner, '(', ')', NULL, true);

  AK24_TEST_ASSERT(result.success);
  AK24_TEST_ASSERT(result.index_of_start_symbol == 2);
  AK24_TEST_ASSERT(result.index_of_closing_symbol == 15);
  AK24_TEST_ASSERT(scanner->position == 15);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int run_scanner_find_group_tests(void) {
  AK24_TEST_RUN(test_find_group_simple_parens);
  AK24_TEST_RUN(test_find_group_simple_brackets);
  AK24_TEST_RUN(test_find_group_simple_braces);
  AK24_TEST_RUN(test_find_group_custom_delimiters);

  AK24_TEST_RUN(test_find_group_sequential_groups);
  AK24_TEST_RUN(test_find_group_mixed_delimiters);
  AK24_TEST_RUN(test_find_group_different_custom_delimiters);

  AK24_TEST_RUN(test_find_group_escaped_quotes);
  AK24_TEST_RUN(test_find_group_multiple_escaped_end_symbols);
  AK24_TEST_RUN(test_find_group_escape_at_end_of_buffer);
  AK24_TEST_RUN(test_find_group_escape_followed_by_non_end_symbol);

  AK24_TEST_RUN(test_find_group_wrong_start_symbol);
  AK24_TEST_RUN(test_find_group_null_scanner);
  AK24_TEST_RUN(test_find_group_empty_buffer);
  AK24_TEST_RUN(test_find_group_position_at_end);
  AK24_TEST_RUN(test_find_group_position_not_at_start_symbol);

  AK24_TEST_RUN(test_find_group_missing_end_symbol);
  AK24_TEST_RUN(test_find_group_only_start_symbol);
  AK24_TEST_RUN(test_find_group_all_escaped_no_real_end);

  AK24_TEST_RUN(test_find_group_same_start_end_symbols);
  AK24_TEST_RUN(test_find_group_empty_group);
  AK24_TEST_RUN(test_find_group_nested_groups);
  AK24_TEST_RUN(test_find_group_no_escape_byte);
  AK24_TEST_RUN(test_find_group_complex_content);
  AK24_TEST_RUN(test_find_group_with_whitespace);

  AK24_TEST_RUN(test_find_group_deeply_nested);
  AK24_TEST_RUN(test_find_group_multiple_groups_in_buffer);
  AK24_TEST_RUN(test_find_group_escaped_quotes_complex);
  AK24_TEST_RUN(test_find_group_mixed_nested_with_escapes);
  AK24_TEST_RUN(test_find_group_asymmetric_nesting);
  AK24_TEST_RUN(test_find_group_unbalanced_inside_quotes);
  AK24_TEST_RUN(test_find_group_large_buffer_with_many_groups);
  AK24_TEST_RUN(test_find_group_escape_escape_character);
  AK24_TEST_RUN(test_find_group_max_depth_stress);

  AK24_TEST_RUN(test_consume_leading_ws_with_spaces);
  AK24_TEST_RUN(test_consume_leading_ws_with_tabs);
  AK24_TEST_RUN(test_consume_leading_ws_with_newlines);
  AK24_TEST_RUN(test_consume_leading_ws_mixed_whitespace);
  AK24_TEST_RUN(test_consume_leading_ws_false_fails_on_whitespace);
  AK24_TEST_RUN(test_consume_leading_ws_all_whitespace_buffer);
  AK24_TEST_RUN(test_consume_leading_ws_no_whitespace);
  AK24_TEST_RUN(test_consume_leading_ws_nested_groups);

  return 0;
}
