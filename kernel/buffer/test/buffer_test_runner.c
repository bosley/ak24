#include "buffer.h"
#include "test/assert.h"

extern int test_rotate_left_basic(void);
extern int test_rotate_left_zero(void);
extern int test_rotate_left_exact_count(void);
extern int test_rotate_left_wrapping(void);
extern int test_rotate_left_single_element(void);
extern int test_rotate_left_empty(void);
extern int test_rotate_left_null(void);
extern int test_rotate_right_basic(void);
extern int test_rotate_right_zero(void);
extern int test_rotate_right_exact_count(void);
extern int test_rotate_right_wrapping(void);
extern int test_rotate_right_single_element(void);
extern int test_rotate_right_empty(void);
extern int test_rotate_right_null(void);
extern int test_trim_left_basic(void);
extern int test_trim_left_no_match(void);
extern int test_trim_left_all_match(void);
extern int test_trim_left_partial_match(void);
extern int test_trim_left_empty(void);
extern int test_trim_left_null(void);
extern int test_trim_left_single_byte_match(void);
extern int test_trim_left_single_byte_no_match(void);
extern int test_trim_right_basic(void);
extern int test_trim_right_no_match(void);
extern int test_trim_right_all_match(void);
extern int test_trim_right_partial_match(void);
extern int test_trim_right_empty(void);
extern int test_trim_right_null(void);
extern int test_trim_right_single_byte_match(void);
extern int test_trim_right_single_byte_no_match(void);
extern int test_copy_buffer_basic(void);
extern int test_copy_buffer_independence(void);
extern int test_copy_buffer_empty(void);
extern int test_copy_buffer_null(void);
extern int test_copy_buffer_large(void);
extern int test_rotate_left_then_right(void);
extern int test_trim_both_sides(void);

int run_buffer_rotate_trim_copy_tests(void) {
  AK24_TEST_RUN(test_rotate_left_basic);
  AK24_TEST_RUN(test_rotate_left_zero);
  AK24_TEST_RUN(test_rotate_left_exact_count);
  AK24_TEST_RUN(test_rotate_left_wrapping);
  AK24_TEST_RUN(test_rotate_left_single_element);
  AK24_TEST_RUN(test_rotate_left_empty);
  AK24_TEST_RUN(test_rotate_left_null);
  AK24_TEST_RUN(test_rotate_right_basic);
  AK24_TEST_RUN(test_rotate_right_zero);
  AK24_TEST_RUN(test_rotate_right_exact_count);
  AK24_TEST_RUN(test_rotate_right_wrapping);
  AK24_TEST_RUN(test_rotate_right_single_element);
  AK24_TEST_RUN(test_rotate_right_empty);
  AK24_TEST_RUN(test_rotate_right_null);
  AK24_TEST_RUN(test_trim_left_basic);
  AK24_TEST_RUN(test_trim_left_no_match);
  AK24_TEST_RUN(test_trim_left_all_match);
  AK24_TEST_RUN(test_trim_left_partial_match);
  AK24_TEST_RUN(test_trim_left_empty);
  AK24_TEST_RUN(test_trim_left_null);
  AK24_TEST_RUN(test_trim_left_single_byte_match);
  AK24_TEST_RUN(test_trim_left_single_byte_no_match);
  AK24_TEST_RUN(test_trim_right_basic);
  AK24_TEST_RUN(test_trim_right_no_match);
  AK24_TEST_RUN(test_trim_right_all_match);
  AK24_TEST_RUN(test_trim_right_partial_match);
  AK24_TEST_RUN(test_trim_right_empty);
  AK24_TEST_RUN(test_trim_right_null);
  AK24_TEST_RUN(test_trim_right_single_byte_match);
  AK24_TEST_RUN(test_trim_right_single_byte_no_match);
  AK24_TEST_RUN(test_copy_buffer_basic);
  AK24_TEST_RUN(test_copy_buffer_independence);
  AK24_TEST_RUN(test_copy_buffer_empty);
  AK24_TEST_RUN(test_copy_buffer_null);
  AK24_TEST_RUN(test_copy_buffer_large);
  AK24_TEST_RUN(test_rotate_left_then_right);
  AK24_TEST_RUN(test_trim_both_sides);
  return 0;
}

int main(void) { return run_buffer_rotate_trim_copy_tests(); }
