#include "sourceloc.h"
#include "test/assert.h"
#include <string.h>

// Test basic initialization and shutdown
int test_sourceloc_init_shutdown(void) {
  ak_kernel_init();
  ak_kernel_deinit();
  AK24_TEST_PASS();
}

// Test creating a source file from memory
int test_source_file_new(void) {
  ak_kernel_init();

  const char *code = "int main() {\n  return 0;\n}\n";
  ak_source_file_t *file = ak_source_file_new("test.c", code, strlen(code));

  AK24_TEST_ASSERT_NOT_NULL(file);
  AK24_TEST_ASSERT_STR_EQ(ak_source_file_name(file), "test.c");
  AK24_TEST_ASSERT_EQ(ak_source_file_length(file), strlen(code));
  AK24_TEST_ASSERT_NOT_NULL(ak_source_file_contents(file));

  ak_source_file_release(file);
  ak_kernel_deinit();
  AK24_TEST_PASS();
}

// Test reference counting
int test_source_file_reference_counting(void) {
  ak_kernel_init();

  const char *code = "test";
  ak_source_file_t *file1 = ak_source_file_new("test.c", code, strlen(code));
  AK24_TEST_ASSERT_NOT_NULL(file1);

  ak_source_file_t *file2 = ak_source_file_retain(file1);
  AK24_TEST_ASSERT_EQ(file1, file2);

  // Release once - should still be valid
  ak_source_file_release(file1);

  // File should still be valid through file2
  AK24_TEST_ASSERT_STR_EQ(ak_source_file_name(file2), "test.c");

  // Release again - now it should be freed
  ak_source_file_release(file2);

  ak_kernel_deinit();
  AK24_TEST_PASS();
}

// Test creating a location with explicit values
int test_source_loc_new(void) {
  ak_kernel_init();

  const char *code = "line1\nline2\nline3\n";
  ak_source_file_t *file = ak_source_file_new("test.c", code, strlen(code));
  AK24_TEST_ASSERT_NOT_NULL(file);

  ak_source_loc_t loc = ak_source_loc_new(file, 2, 3, 8);

  AK24_TEST_ASSERT_EQ(loc.file, file);
  AK24_TEST_ASSERT_EQ(loc.line, 2);
  AK24_TEST_ASSERT_EQ(loc.column, 3);
  AK24_TEST_ASSERT_EQ(loc.offset, 8);

  ak_source_file_release(file);
  ak_kernel_deinit();
  AK24_TEST_PASS();
}

// Test computing location from offset
int test_source_loc_from_offset(void) {
  ak_kernel_init();

  const char *code = "line1\nline2\nline3\n";
  ak_source_file_t *file = ak_source_file_new("test.c", code, strlen(code));
  AK24_TEST_ASSERT_NOT_NULL(file);

  // Offset 0 should be line 1, column 1
  ak_source_loc_t loc1 = ak_source_loc_from_offset(file, 0);
  AK24_TEST_ASSERT_EQ(loc1.line, 1);
  AK24_TEST_ASSERT_EQ(loc1.column, 1);

  // Offset 6 should be start of line 2 (after "line1\n")
  ak_source_loc_t loc2 = ak_source_loc_from_offset(file, 6);
  AK24_TEST_ASSERT_EQ(loc2.line, 2);
  AK24_TEST_ASSERT_EQ(loc2.column, 1);

  // Offset 12 should be start of line 3 (after "line1\nline2\n")
  ak_source_loc_t loc3 = ak_source_loc_from_offset(file, 12);
  AK24_TEST_ASSERT_EQ(loc3.line, 3);
  AK24_TEST_ASSERT_EQ(loc3.column, 1);

  // Offset 8 should be line 2, column 3 (the 'n' in "line2")
  ak_source_loc_t loc4 = ak_source_loc_from_offset(file, 8);
  AK24_TEST_ASSERT_EQ(loc4.line, 2);
  AK24_TEST_ASSERT_EQ(loc4.column, 3);

  ak_source_file_release(file);
  ak_kernel_deinit();
  AK24_TEST_PASS();
}

// Test UTF-8 column counting
int test_source_loc_utf8_columns(void) {
  ak_kernel_init();

  // String with UTF-8 characters: "Hello 世界\n"
  // H=0, e=1, l=2, l=3, o=4, space=5, 世=6-8 (3 bytes), 界=9-11 (3 bytes),
  // \n=12
  const char *code = "Hello 世界\n";
  ak_source_file_t *file = ak_source_file_new("test.c", code, strlen(code));
  AK24_TEST_ASSERT_NOT_NULL(file);

  // Offset 0 = 'H' should be column 1
  ak_source_loc_t loc1 = ak_source_loc_from_offset(file, 0);
  AK24_TEST_ASSERT_EQ(loc1.column, 1);

  // Offset 6 = start of '世' should be column 7
  ak_source_loc_t loc2 = ak_source_loc_from_offset(file, 6);
  AK24_TEST_ASSERT_EQ(loc2.column, 7);

  // Offset 9 = start of '界' should be column 8
  ak_source_loc_t loc3 = ak_source_loc_from_offset(file, 9);
  AK24_TEST_ASSERT_EQ(loc3.column, 8);

  ak_source_file_release(file);
  ak_kernel_deinit();
  AK24_TEST_PASS();
}

// Test creating a range
int test_source_range_new(void) {
  ak_kernel_init();

  const char *code = "0123456789\n";
  ak_source_file_t *file = ak_source_file_new("test.c", code, strlen(code));
  AK24_TEST_ASSERT_NOT_NULL(file);

  ak_source_loc_t start = ak_source_loc_from_offset(file, 2);
  ak_source_loc_t end = ak_source_loc_from_offset(file, 7);
  ak_source_range_t range = ak_source_range_new(start, end);

  AK24_TEST_ASSERT_EQ(range.start.file, file);
  AK24_TEST_ASSERT_EQ(range.end.file, file);
  AK24_TEST_ASSERT_EQ(range.start.offset, 2);
  AK24_TEST_ASSERT_EQ(range.end.offset, 7);

  ak_source_file_release(file);
  ak_kernel_deinit();
  AK24_TEST_PASS();
}

// Test range contains
int test_source_range_contains(void) {
  ak_kernel_init();

  const char *code = "0123456789\n";
  ak_source_file_t *file = ak_source_file_new("test.c", code, strlen(code));
  AK24_TEST_ASSERT_NOT_NULL(file);

  ak_source_loc_t start = ak_source_loc_from_offset(file, 2);
  ak_source_loc_t end = ak_source_loc_from_offset(file, 7);
  ak_source_range_t range = ak_source_range_new(start, end);

  // Offset 2 (start) should be contained
  ak_source_loc_t loc1 = ak_source_loc_from_offset(file, 2);
  AK24_TEST_ASSERT(ak_source_range_contains(&range, loc1));

  // Offset 5 (middle) should be contained
  ak_source_loc_t loc2 = ak_source_loc_from_offset(file, 5);
  AK24_TEST_ASSERT(ak_source_range_contains(&range, loc2));

  // Offset 6 (one before end) should be contained
  ak_source_loc_t loc3 = ak_source_loc_from_offset(file, 6);
  AK24_TEST_ASSERT(ak_source_range_contains(&range, loc3));

  // Offset 7 (end) should NOT be contained (exclusive)
  ak_source_loc_t loc4 = ak_source_loc_from_offset(file, 7);
  AK24_TEST_ASSERT(!ak_source_range_contains(&range, loc4));

  // Offset 1 (before start) should NOT be contained
  ak_source_loc_t loc5 = ak_source_loc_from_offset(file, 1);
  AK24_TEST_ASSERT(!ak_source_range_contains(&range, loc5));

  ak_source_file_release(file);
  ak_kernel_deinit();
  AK24_TEST_PASS();
}

// Test range overlaps
int test_source_range_overlaps(void) {
  ak_kernel_init();

  const char *code = "0123456789\n";
  ak_source_file_t *file = ak_source_file_new("test.c", code, strlen(code));
  AK24_TEST_ASSERT_NOT_NULL(file);

  // Range 1: [2, 7)
  ak_source_range_t range1 = ak_source_range_new(
      ak_source_loc_from_offset(file, 2), ak_source_loc_from_offset(file, 7));

  // Range 2: [5, 9) - overlaps with range1
  ak_source_range_t range2 = ak_source_range_new(
      ak_source_loc_from_offset(file, 5), ak_source_loc_from_offset(file, 9));

  // Range 3: [7, 10) - does NOT overlap (starts where range1 ends)
  ak_source_range_t range3 = ak_source_range_new(
      ak_source_loc_from_offset(file, 7), ak_source_loc_from_offset(file, 10));

  // Range 4: [0, 2) - does NOT overlap (ends where range1 starts)
  ak_source_range_t range4 = ak_source_range_new(
      ak_source_loc_from_offset(file, 0), ak_source_loc_from_offset(file, 2));

  AK24_TEST_ASSERT(ak_source_range_overlaps(&range1, &range2));
  AK24_TEST_ASSERT(!ak_source_range_overlaps(&range1, &range3));
  AK24_TEST_ASSERT(!ak_source_range_overlaps(&range1, &range4));

  ak_source_file_release(file);
  ak_kernel_deinit();
  AK24_TEST_PASS();
}

// Test extracting text from range
int test_source_extract(void) {
  ak_kernel_init();

  const char *code = "Hello, World!\n";
  ak_source_file_t *file = ak_source_file_new("test.c", code, strlen(code));
  AK24_TEST_ASSERT_NOT_NULL(file);

  // Extract "World"
  ak_source_range_t range = ak_source_range_new(
      ak_source_loc_from_offset(file, 7), ak_source_loc_from_offset(file, 12));

  char *text = ak_source_extract(range);
  AK24_TEST_ASSERT_NOT_NULL(text);
  AK24_TEST_ASSERT_STR_EQ(text, "World");

  AK24_FREE(text);
  ak_source_file_release(file);
  ak_kernel_deinit();
  AK24_TEST_PASS();
}

// Test getting a specific line
int test_source_get_line(void) {
  ak_kernel_init();

  const char *code = "line1\nline2\nline3\n";
  ak_source_file_t *file = ak_source_file_new("test.c", code, strlen(code));
  AK24_TEST_ASSERT_NOT_NULL(file);

  // Get line 1
  const char *line1 = ak_source_get_line(file, 1);
  size_t len1 = ak_source_get_line_len(file, 1);
  AK24_TEST_ASSERT_NOT_NULL(line1);
  AK24_TEST_ASSERT_EQ(len1, 5);
  AK24_TEST_ASSERT(strncmp(line1, "line1", len1) == 0);

  // Get line 2
  const char *line2 = ak_source_get_line(file, 2);
  size_t len2 = ak_source_get_line_len(file, 2);
  AK24_TEST_ASSERT_NOT_NULL(line2);
  AK24_TEST_ASSERT_EQ(len2, 5);
  AK24_TEST_ASSERT(strncmp(line2, "line2", len2) == 0);

  // Get line 3
  const char *line3 = ak_source_get_line(file, 3);
  size_t len3 = ak_source_get_line_len(file, 3);
  AK24_TEST_ASSERT_NOT_NULL(line3);
  AK24_TEST_ASSERT_EQ(len3, 5);
  AK24_TEST_ASSERT(strncmp(line3, "line3", len3) == 0);

  // Invalid line number
  const char *line_invalid = ak_source_get_line(file, 999);
  AK24_TEST_ASSERT_NULL(line_invalid);

  ak_source_file_release(file);
  ak_kernel_deinit();
  AK24_TEST_PASS();
}

// Test formatting a location
int test_source_loc_format(void) {
  ak_kernel_init();

  const char *code = "test\n";
  ak_source_file_t *file = ak_source_file_new("test.c", code, strlen(code));
  AK24_TEST_ASSERT_NOT_NULL(file);

  ak_source_loc_t loc = ak_source_loc_new(file, 10, 5, 42);

  char buf[256];
  int written = ak_source_loc_format(loc, buf, sizeof(buf));

  AK24_TEST_ASSERT(written > 0);
  AK24_TEST_ASSERT_STR_EQ(buf, "test.c:10:5");

  ak_source_file_release(file);
  ak_kernel_deinit();
  AK24_TEST_PASS();
}

// Test formatting a range (single line)
int test_source_range_format_single_line(void) {
  ak_kernel_init();

  const char *code = "test\n";
  ak_source_file_t *file = ak_source_file_new("test.c", code, strlen(code));
  AK24_TEST_ASSERT_NOT_NULL(file);

  ak_source_loc_t start = ak_source_loc_new(file, 5, 10, 0);
  ak_source_loc_t end = ak_source_loc_new(file, 5, 20, 0);
  ak_source_range_t range = ak_source_range_new(start, end);

  char buf[256];
  int written = ak_source_range_format(range, buf, sizeof(buf));

  AK24_TEST_ASSERT(written > 0);
  AK24_TEST_ASSERT_STR_EQ(buf, "test.c:5:10-20");

  ak_source_file_release(file);
  ak_kernel_deinit();
  AK24_TEST_PASS();
}

// Test formatting a range (multi-line)
int test_source_range_format_multi_line(void) {
  ak_kernel_init();

  const char *code = "test\n";
  ak_source_file_t *file = ak_source_file_new("test.c", code, strlen(code));
  AK24_TEST_ASSERT_NOT_NULL(file);

  ak_source_loc_t start = ak_source_loc_new(file, 5, 10, 0);
  ak_source_loc_t end = ak_source_loc_new(file, 8, 5, 0);
  ak_source_range_t range = ak_source_range_new(start, end);

  char buf[256];
  int written = ak_source_range_format(range, buf, sizeof(buf));

  AK24_TEST_ASSERT(written > 0);
  AK24_TEST_ASSERT_STR_EQ(buf, "test.c:5:10-8:5");

  ak_source_file_release(file);
  ak_kernel_deinit();
  AK24_TEST_PASS();
}

// Test statistics
int test_sourceloc_stats(void) {
  ak_kernel_init();

  size_t count1, bytes1;
  ak_sourceloc_stats(&count1, &bytes1);
  AK24_TEST_ASSERT_EQ(count1, 0);
  AK24_TEST_ASSERT_EQ(bytes1, 0);

  const char *code1 = "file1";
  ak_source_file_t *file1 = ak_source_file_new("test1.c", code1, strlen(code1));

  size_t count2, bytes2;
  ak_sourceloc_stats(&count2, &bytes2);
  AK24_TEST_ASSERT_EQ(count2, 1);
  AK24_TEST_ASSERT_EQ(bytes2, strlen(code1));

  const char *code2 = "file2content";
  ak_source_file_t *file2 = ak_source_file_new("test2.c", code2, strlen(code2));

  size_t count3, bytes3;
  ak_sourceloc_stats(&count3, &bytes3);
  AK24_TEST_ASSERT_EQ(count3, 2);
  AK24_TEST_ASSERT_EQ(bytes3, strlen(code1) + strlen(code2));

  ak_source_file_release(file1);

  size_t count4, bytes4;
  ak_sourceloc_stats(&count4, &bytes4);
  AK24_TEST_ASSERT_EQ(count4, 1);
  AK24_TEST_ASSERT_EQ(bytes4, strlen(code2));

  ak_source_file_release(file2);

  ak_kernel_deinit();
  AK24_TEST_PASS();
}

// Test empty file
int test_source_file_empty(void) {
  ak_kernel_init();

  const char *code = "";
  ak_source_file_t *file = ak_source_file_new("empty.c", code, 0);
  AK24_TEST_ASSERT_NOT_NULL(file);

  AK24_TEST_ASSERT_EQ(ak_source_file_length(file), 0);

  // Empty file should still have 1 line (line 1)
  ak_source_loc_t loc = ak_source_loc_from_offset(file, 0);
  AK24_TEST_ASSERT_EQ(loc.line, 1);
  AK24_TEST_ASSERT_EQ(loc.column, 1);

  ak_source_file_release(file);
  ak_kernel_deinit();
  AK24_TEST_PASS();
}

// Test single line file
int test_source_file_single_line(void) {
  ak_kernel_init();

  const char *code = "single line no newline";
  ak_source_file_t *file = ak_source_file_new("single.c", code, strlen(code));
  AK24_TEST_ASSERT_NOT_NULL(file);

  ak_source_loc_t loc1 = ak_source_loc_from_offset(file, 0);
  AK24_TEST_ASSERT_EQ(loc1.line, 1);

  ak_source_loc_t loc2 = ak_source_loc_from_offset(file, strlen(code) - 1);
  AK24_TEST_ASSERT_EQ(loc2.line, 1);

  ak_source_file_release(file);
  ak_kernel_deinit();
  AK24_TEST_PASS();
}

// Test file with Windows line endings
int test_source_file_crlf(void) {
  ak_kernel_init();

  const char *code = "line1\r\nline2\r\nline3\r\n";
  ak_source_file_t *file = ak_source_file_new("win.c", code, strlen(code));
  AK24_TEST_ASSERT_NOT_NULL(file);

  // Line 1 should be "line1" (without \r\n)
  const char *line1 = ak_source_get_line(file, 1);
  size_t len1 = ak_source_get_line_len(file, 1);
  AK24_TEST_ASSERT_NOT_NULL(line1);
  AK24_TEST_ASSERT_EQ(len1, 5);
  AK24_TEST_ASSERT(strncmp(line1, "line1", len1) == 0);

  ak_source_file_release(file);
  ak_kernel_deinit();
  AK24_TEST_PASS();
}

// Main test runner
int main(void) {
  AK24_TEST_RUN(test_sourceloc_init_shutdown);
  AK24_TEST_RUN(test_source_file_new);
  AK24_TEST_RUN(test_source_file_reference_counting);
  AK24_TEST_RUN(test_source_loc_new);
  AK24_TEST_RUN(test_source_loc_from_offset);
  AK24_TEST_RUN(test_source_loc_utf8_columns);
  AK24_TEST_RUN(test_source_range_new);
  AK24_TEST_RUN(test_source_range_contains);
  AK24_TEST_RUN(test_source_range_overlaps);
  AK24_TEST_RUN(test_source_extract);
  AK24_TEST_RUN(test_source_get_line);
  AK24_TEST_RUN(test_source_loc_format);
  AK24_TEST_RUN(test_source_range_format_single_line);
  AK24_TEST_RUN(test_source_range_format_multi_line);
  AK24_TEST_RUN(test_sourceloc_stats);
  AK24_TEST_RUN(test_source_file_empty);
  AK24_TEST_RUN(test_source_file_single_line);
  AK24_TEST_RUN(test_source_file_crlf);

  fprintf(stdout, "\n=================================\n");
  fprintf(stdout, "All sourceloc tests passed!\n");
  fprintf(stdout, "=================================\n");
  return 0;
}
