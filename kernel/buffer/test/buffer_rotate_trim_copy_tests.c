#include "buffer.h"
#include "kernel.h"
#include "test/assert.h"
#include <string.h>

int test_rotate_left_basic(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5};
  ak_buffer_copy_to(buffer, data, 5);

  ak_buffer_rotate_left(buffer, 2);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 3);
  AK24_TEST_ASSERT_EQ(buf_data[1], 4);
  AK24_TEST_ASSERT_EQ(buf_data[2], 5);
  AK24_TEST_ASSERT_EQ(buf_data[3], 1);
  AK24_TEST_ASSERT_EQ(buf_data[4], 2);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
  AK24_TEST_PASS();
}

int test_rotate_left_zero(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {10, 20, 30, 40};
  ak_buffer_copy_to(buffer, data, 4);

  ak_buffer_rotate_left(buffer, 0);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 10);
  AK24_TEST_ASSERT_EQ(buf_data[1], 20);
  AK24_TEST_ASSERT_EQ(buf_data[2], 30);
  AK24_TEST_ASSERT_EQ(buf_data[3], 40);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_rotate_left_exact_count(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5};
  ak_buffer_copy_to(buffer, data, 5);

  ak_buffer_rotate_left(buffer, 5);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 1);
  AK24_TEST_ASSERT_EQ(buf_data[1], 2);
  AK24_TEST_ASSERT_EQ(buf_data[2], 3);
  AK24_TEST_ASSERT_EQ(buf_data[3], 4);
  AK24_TEST_ASSERT_EQ(buf_data[4], 5);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_rotate_left_wrapping(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5};
  ak_buffer_copy_to(buffer, data, 5);

  ak_buffer_rotate_left(buffer, 7);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 3);
  AK24_TEST_ASSERT_EQ(buf_data[1], 4);
  AK24_TEST_ASSERT_EQ(buf_data[2], 5);
  AK24_TEST_ASSERT_EQ(buf_data[3], 1);
  AK24_TEST_ASSERT_EQ(buf_data[4], 2);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_rotate_left_single_element(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {42};
  ak_buffer_copy_to(buffer, data, 1);

  ak_buffer_rotate_left(buffer, 10);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 42);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_rotate_left_empty(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);

  ak_buffer_rotate_left(buffer, 5);

  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 0);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_rotate_left_null(void) {
  ak_buffer_rotate_left(NULL, 5);
  AK24_TEST_PASS();
}

int test_rotate_right_basic(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5};
  ak_buffer_copy_to(buffer, data, 5);

  ak_buffer_rotate_right(buffer, 2);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 4);
  AK24_TEST_ASSERT_EQ(buf_data[1], 5);
  AK24_TEST_ASSERT_EQ(buf_data[2], 1);
  AK24_TEST_ASSERT_EQ(buf_data[3], 2);
  AK24_TEST_ASSERT_EQ(buf_data[4], 3);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_rotate_right_zero(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {10, 20, 30, 40};
  ak_buffer_copy_to(buffer, data, 4);

  ak_buffer_rotate_right(buffer, 0);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 10);
  AK24_TEST_ASSERT_EQ(buf_data[1], 20);
  AK24_TEST_ASSERT_EQ(buf_data[2], 30);
  AK24_TEST_ASSERT_EQ(buf_data[3], 40);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_rotate_right_exact_count(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5};
  ak_buffer_copy_to(buffer, data, 5);

  ak_buffer_rotate_right(buffer, 5);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 1);
  AK24_TEST_ASSERT_EQ(buf_data[1], 2);
  AK24_TEST_ASSERT_EQ(buf_data[2], 3);
  AK24_TEST_ASSERT_EQ(buf_data[3], 4);
  AK24_TEST_ASSERT_EQ(buf_data[4], 5);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_rotate_right_wrapping(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5};
  ak_buffer_copy_to(buffer, data, 5);

  ak_buffer_rotate_right(buffer, 7);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 4);
  AK24_TEST_ASSERT_EQ(buf_data[1], 5);
  AK24_TEST_ASSERT_EQ(buf_data[2], 1);
  AK24_TEST_ASSERT_EQ(buf_data[3], 2);
  AK24_TEST_ASSERT_EQ(buf_data[4], 3);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_rotate_right_single_element(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {42};
  ak_buffer_copy_to(buffer, data, 1);

  ak_buffer_rotate_right(buffer, 10);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 42);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_rotate_right_empty(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);

  ak_buffer_rotate_right(buffer, 5);

  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 0);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_rotate_right_null(void) {
  ak_buffer_rotate_right(NULL, 5);
  AK24_TEST_PASS();
}

int test_trim_left_basic(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {0, 0, 0, 5, 6, 0};
  ak_buffer_copy_to(buffer, data, 6);

  int result = ak_buffer_trim_left(buffer, 0);

  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 3);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 5);
  AK24_TEST_ASSERT_EQ(buf_data[1], 6);
  AK24_TEST_ASSERT_EQ(buf_data[2], 0);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_trim_left_no_match(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5};
  ak_buffer_copy_to(buffer, data, 5);

  int result = ak_buffer_trim_left(buffer, 0);

  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 5);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 1);
  AK24_TEST_ASSERT_EQ(buf_data[1], 2);
  AK24_TEST_ASSERT_EQ(buf_data[2], 3);
  AK24_TEST_ASSERT_EQ(buf_data[3], 4);
  AK24_TEST_ASSERT_EQ(buf_data[4], 5);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_trim_left_all_match(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {7, 7, 7, 7, 7};
  ak_buffer_copy_to(buffer, data, 5);

  int result = ak_buffer_trim_left(buffer, 7);

  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 0);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_trim_left_partial_match(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {255, 255, 100, 255, 255};
  ak_buffer_copy_to(buffer, data, 5);

  int result = ak_buffer_trim_left(buffer, 255);

  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 3);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 100);
  AK24_TEST_ASSERT_EQ(buf_data[1], 255);
  AK24_TEST_ASSERT_EQ(buf_data[2], 255);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_trim_left_empty(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);

  int result = ak_buffer_trim_left(buffer, 0);

  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 0);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_trim_left_null(void) {
  int result = ak_buffer_trim_left(NULL, 0);
  AK24_TEST_ASSERT_EQ(result, -1);
  AK24_TEST_PASS();
}

int test_trim_left_single_byte_match(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {9};
  ak_buffer_copy_to(buffer, data, 1);

  int result = ak_buffer_trim_left(buffer, 9);

  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 0);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_trim_left_single_byte_no_match(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {9};
  ak_buffer_copy_to(buffer, data, 1);

  int result = ak_buffer_trim_left(buffer, 8);

  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 1);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 9);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_trim_right_basic(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {0, 5, 6, 0, 0, 0};
  ak_buffer_copy_to(buffer, data, 6);

  int result = ak_buffer_trim_right(buffer, 0);

  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 3);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 0);
  AK24_TEST_ASSERT_EQ(buf_data[1], 5);
  AK24_TEST_ASSERT_EQ(buf_data[2], 6);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_trim_right_no_match(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5};
  ak_buffer_copy_to(buffer, data, 5);

  int result = ak_buffer_trim_right(buffer, 0);

  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 5);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 1);
  AK24_TEST_ASSERT_EQ(buf_data[1], 2);
  AK24_TEST_ASSERT_EQ(buf_data[2], 3);
  AK24_TEST_ASSERT_EQ(buf_data[3], 4);
  AK24_TEST_ASSERT_EQ(buf_data[4], 5);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_trim_right_all_match(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {7, 7, 7, 7, 7};
  ak_buffer_copy_to(buffer, data, 5);

  int result = ak_buffer_trim_right(buffer, 7);

  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 0);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_trim_right_partial_match(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {255, 255, 100, 255, 255};
  ak_buffer_copy_to(buffer, data, 5);

  int result = ak_buffer_trim_right(buffer, 255);

  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 3);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 255);
  AK24_TEST_ASSERT_EQ(buf_data[1], 255);
  AK24_TEST_ASSERT_EQ(buf_data[2], 100);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_trim_right_empty(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);

  int result = ak_buffer_trim_right(buffer, 0);

  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 0);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_trim_right_null(void) {
  int result = ak_buffer_trim_right(NULL, 0);
  AK24_TEST_ASSERT_EQ(result, -1);
  AK24_TEST_PASS();
}

int test_trim_right_single_byte_match(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {9};
  ak_buffer_copy_to(buffer, data, 1);

  int result = ak_buffer_trim_right(buffer, 9);

  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 0);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_trim_right_single_byte_no_match(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {9};
  ak_buffer_copy_to(buffer, data, 1);

  int result = ak_buffer_trim_right(buffer, 8);

  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 1);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 9);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_copy_buffer_basic(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5};
  ak_buffer_copy_to(buffer, data, 5);

  ak_buffer_t *copy = ak_buffer_copy(buffer);

  AK24_TEST_ASSERT_NOT_NULL(copy);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(copy), 5);
  AK24_TEST_ASSERT_EQ(copy->capacity, buffer->capacity);

  uint8_t *copy_data = ak_buffer_data(copy);
  AK24_TEST_ASSERT_EQ(copy_data[0], 1);
  AK24_TEST_ASSERT_EQ(copy_data[1], 2);
  AK24_TEST_ASSERT_EQ(copy_data[2], 3);
  AK24_TEST_ASSERT_EQ(copy_data[3], 4);
  AK24_TEST_ASSERT_EQ(copy_data[4], 5);

  ak_buffer_free(copy);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_copy_buffer_independence(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {10, 20, 30, 40, 50};
  ak_buffer_copy_to(buffer, data, 5);

  ak_buffer_t *copy = ak_buffer_copy(buffer);

  uint8_t *buf_data = ak_buffer_data(buffer);
  buf_data[0] = 99;
  buf_data[1] = 88;

  uint8_t *copy_data = ak_buffer_data(copy);
  AK24_TEST_ASSERT_EQ(copy_data[0], 10);
  AK24_TEST_ASSERT_EQ(copy_data[1], 20);
  AK24_TEST_ASSERT_EQ(copy_data[2], 30);
  AK24_TEST_ASSERT_EQ(copy_data[3], 40);
  AK24_TEST_ASSERT_EQ(copy_data[4], 50);

  ak_buffer_free(copy);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_copy_buffer_empty(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);

  ak_buffer_t *copy = ak_buffer_copy(buffer);

  AK24_TEST_ASSERT_NOT_NULL(copy);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(copy), 0);

  ak_buffer_free(copy);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_copy_buffer_null(void) {
  ak_buffer_t *copy = ak_buffer_copy(NULL);
  AK24_TEST_ASSERT_NULL(copy);
  AK24_TEST_PASS();
}

int test_copy_buffer_large(void) {
  ak_buffer_t *buffer = ak_buffer_new(1000);
  uint8_t data[1000];
  for (int i = 0; i < 1000; i++) {
    data[i] = i % 256;
  }
  ak_buffer_copy_to(buffer, data, 1000);

  ak_buffer_t *copy = ak_buffer_copy(buffer);

  AK24_TEST_ASSERT_NOT_NULL(copy);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(copy), 1000);

  uint8_t *copy_data = ak_buffer_data(copy);
  for (int i = 0; i < 1000; i++) {
    AK24_TEST_ASSERT_EQ(copy_data[i], i % 256);
  }

  ak_buffer_free(copy);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_rotate_left_then_right(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8};
  ak_buffer_copy_to(buffer, data, 8);

  ak_buffer_rotate_left(buffer, 3);
  ak_buffer_rotate_right(buffer, 3);

  uint8_t *buf_data = ak_buffer_data(buffer);
  for (int i = 0; i < 8; i++) {
    AK24_TEST_ASSERT_EQ(buf_data[i], i + 1);
  }

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_trim_both_sides(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {0, 0, 5, 6, 7, 0, 0};
  ak_buffer_copy_to(buffer, data, 7);

  ak_buffer_trim_left(buffer, 0);
  ak_buffer_trim_right(buffer, 0);

  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 3);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 5);
  AK24_TEST_ASSERT_EQ(buf_data[1], 6);
  AK24_TEST_ASSERT_EQ(buf_data[2], 7);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}
