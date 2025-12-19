#include "buffer.h"
#include "test/assert.h"
#include <string.h>

int test_split_out_of_bounds_index(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5};
  ak_buffer_copy_to(buffer, data, 5);

  split_buffer_t split = ak_buffer_split(buffer, 10, 32, 32);

  AK24_TEST_ASSERT_NULL(split.left);
  AK24_TEST_ASSERT_NULL(split.right);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_split_null_buffer(void) {
  split_buffer_t split = ak_buffer_split(NULL, 2, 32, 32);

  AK24_TEST_ASSERT_NULL(split.left);
  AK24_TEST_ASSERT_NULL(split.right);
  AK24_TEST_PASS();
}

int test_split_empty_buffer(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);

  split_buffer_t split = ak_buffer_split(buffer, 0, 32, 32);

  AK24_TEST_ASSERT_NOT_NULL(split.left);
  AK24_TEST_ASSERT_NOT_NULL(split.right);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.left), 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.right), 0);

  ak_split_buffer_free(&split);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_split_l_and_r_size_validation(void) {
  ak_buffer_t *buffer = ak_buffer_new(100);
  uint8_t data[50];
  for (int i = 0; i < 50; i++) {
    data[i] = i;
  }
  ak_buffer_copy_to(buffer, data, 50);

  split_buffer_t split = ak_buffer_split(buffer, 25, 30, 40);

  AK24_TEST_ASSERT_NOT_NULL(split.left);
  AK24_TEST_ASSERT_NOT_NULL(split.right);

  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.left), 25);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.right), 25);

  AK24_TEST_ASSERT_EQ(split.left->capacity, 25);
  AK24_TEST_ASSERT_EQ(split.right->capacity, 25);

  ak_split_buffer_free(&split);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_split_value_validation_basic(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {10, 20, 30, 40, 50, 60, 70, 80};
  ak_buffer_copy_to(buffer, data, 8);

  split_buffer_t split = ak_buffer_split(buffer, 5, 32, 32);

  AK24_TEST_ASSERT_NOT_NULL(split.left);
  AK24_TEST_ASSERT_NOT_NULL(split.right);

  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.left), 5);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.right), 3);

  uint8_t *left_data = ak_buffer_data(split.left);
  AK24_TEST_ASSERT_EQ(left_data[0], 10);
  AK24_TEST_ASSERT_EQ(left_data[1], 20);
  AK24_TEST_ASSERT_EQ(left_data[2], 30);
  AK24_TEST_ASSERT_EQ(left_data[3], 40);
  AK24_TEST_ASSERT_EQ(left_data[4], 50);

  uint8_t *right_data = ak_buffer_data(split.right);
  AK24_TEST_ASSERT_EQ(right_data[0], 60);
  AK24_TEST_ASSERT_EQ(right_data[1], 70);
  AK24_TEST_ASSERT_EQ(right_data[2], 80);

  ak_split_buffer_free(&split);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_split_index_exclusive(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  ak_buffer_copy_to(buffer, data, 10);

  split_buffer_t split = ak_buffer_split(buffer, 3, 32, 32);

  AK24_TEST_ASSERT_NOT_NULL(split.left);
  AK24_TEST_ASSERT_NOT_NULL(split.right);

  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.left), 3);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.right), 7);

  uint8_t *left_data = ak_buffer_data(split.left);
  AK24_TEST_ASSERT_EQ(left_data[0], 0);
  AK24_TEST_ASSERT_EQ(left_data[1], 1);
  AK24_TEST_ASSERT_EQ(left_data[2], 2);

  uint8_t *right_data = ak_buffer_data(split.right);
  AK24_TEST_ASSERT_EQ(right_data[0], 3);
  AK24_TEST_ASSERT_EQ(right_data[1], 4);
  AK24_TEST_ASSERT_EQ(right_data[2], 5);

  ak_split_buffer_free(&split);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_split_left_under_min_buffer_size(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                    11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
  ak_buffer_copy_to(buffer, data, 20);

  split_buffer_t split = ak_buffer_split(buffer, 5, 100, 100);

  AK24_TEST_ASSERT_NOT_NULL(split.left);
  AK24_TEST_ASSERT_NOT_NULL(split.right);

  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.left), 5);
  AK24_TEST_ASSERT_EQ(split.left->capacity, 16);

  uint8_t *left_data = ak_buffer_data(split.left);
  for (int i = 0; i < 5; i++) {
    AK24_TEST_ASSERT_EQ(left_data[i], i + 1);
  }

  ak_split_buffer_free(&split);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_split_right_under_min_buffer_size(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                    11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
  ak_buffer_copy_to(buffer, data, 20);

  split_buffer_t split = ak_buffer_split(buffer, 15, 100, 100);

  AK24_TEST_ASSERT_NOT_NULL(split.left);
  AK24_TEST_ASSERT_NOT_NULL(split.right);

  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.right), 5);
  AK24_TEST_ASSERT_EQ(split.right->capacity, 16);

  uint8_t *right_data = ak_buffer_data(split.right);
  for (int i = 0; i < 5; i++) {
    AK24_TEST_ASSERT_EQ(right_data[i], 16 + i);
  }

  ak_split_buffer_free(&split);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_split_both_under_min_buffer_size(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  ak_buffer_copy_to(buffer, data, 10);

  split_buffer_t split = ak_buffer_split(buffer, 5, 100, 100);

  AK24_TEST_ASSERT_NOT_NULL(split.left);
  AK24_TEST_ASSERT_NOT_NULL(split.right);

  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.left), 5);
  AK24_TEST_ASSERT_EQ(split.left->capacity, 16);

  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.right), 5);
  AK24_TEST_ASSERT_EQ(split.right->capacity, 16);

  uint8_t *left_data = ak_buffer_data(split.left);
  for (int i = 0; i < 5; i++) {
    AK24_TEST_ASSERT_EQ(left_data[i], i + 1);
  }

  uint8_t *right_data = ak_buffer_data(split.right);
  for (int i = 0; i < 5; i++) {
    AK24_TEST_ASSERT_EQ(right_data[i], 6 + i);
  }

  ak_split_buffer_free(&split);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_split_at_start(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5};
  ak_buffer_copy_to(buffer, data, 5);

  split_buffer_t split = ak_buffer_split(buffer, 0, 32, 32);

  AK24_TEST_ASSERT_NOT_NULL(split.left);
  AK24_TEST_ASSERT_NOT_NULL(split.right);

  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.left), 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.right), 5);

  uint8_t *right_data = ak_buffer_data(split.right);
  for (int i = 0; i < 5; i++) {
    AK24_TEST_ASSERT_EQ(right_data[i], i + 1);
  }

  ak_split_buffer_free(&split);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_split_at_end(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5};
  ak_buffer_copy_to(buffer, data, 5);

  split_buffer_t split = ak_buffer_split(buffer, 5, 32, 32);

  AK24_TEST_ASSERT_NOT_NULL(split.left);
  AK24_TEST_ASSERT_NOT_NULL(split.right);

  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.left), 5);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.right), 0);

  uint8_t *left_data = ak_buffer_data(split.left);
  for (int i = 0; i < 5; i++) {
    AK24_TEST_ASSERT_EQ(left_data[i], i + 1);
  }

  ak_split_buffer_free(&split);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_split_middle_large_buffer(void) {
  ak_buffer_t *buffer = ak_buffer_new(100);
  uint8_t data[100];
  for (int i = 0; i < 100; i++) {
    data[i] = i % 256;
  }
  ak_buffer_copy_to(buffer, data, 100);

  split_buffer_t split = ak_buffer_split(buffer, 50, 60, 60);

  AK24_TEST_ASSERT_NOT_NULL(split.left);
  AK24_TEST_ASSERT_NOT_NULL(split.right);

  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.left), 50);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.right), 50);

  uint8_t *left_data = ak_buffer_data(split.left);
  for (int i = 0; i < 50; i++) {
    AK24_TEST_ASSERT_EQ(left_data[i], i % 256);
  }

  uint8_t *right_data = ak_buffer_data(split.right);
  for (int i = 0; i < 50; i++) {
    AK24_TEST_ASSERT_EQ(right_data[i], (50 + i) % 256);
  }

  ak_split_buffer_free(&split);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_split_with_small_l_capacity(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  ak_buffer_copy_to(buffer, data, 10);

  split_buffer_t split = ak_buffer_split(buffer, 5, 3, 20);

  AK24_TEST_ASSERT_NOT_NULL(split.left);
  AK24_TEST_ASSERT_NOT_NULL(split.right);

  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.left), 5);
  AK24_TEST_ASSERT_EQ(split.left->capacity, 16);

  ak_split_buffer_free(&split);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_split_with_small_r_capacity(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  ak_buffer_copy_to(buffer, data, 10);

  split_buffer_t split = ak_buffer_split(buffer, 5, 20, 3);

  AK24_TEST_ASSERT_NOT_NULL(split.left);
  AK24_TEST_ASSERT_NOT_NULL(split.right);

  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.right), 5);
  AK24_TEST_ASSERT_EQ(split.right->capacity, 16);

  ak_split_buffer_free(&split);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_split_single_element_buffer(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {42};
  ak_buffer_copy_to(buffer, data, 1);

  split_buffer_t split = ak_buffer_split(buffer, 1, 32, 32);

  AK24_TEST_ASSERT_NOT_NULL(split.left);
  AK24_TEST_ASSERT_NOT_NULL(split.right);

  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.left), 1);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(split.right), 0);

  uint8_t *left_data = ak_buffer_data(split.left);
  AK24_TEST_ASSERT_EQ(left_data[0], 42);

  ak_split_buffer_free(&split);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_split_destroy_null(void) {
  ak_split_buffer_free(NULL);
  AK24_TEST_PASS();
}

int test_split_destroy_partial(void) {
  split_buffer_t split;
  split.left = ak_buffer_new(32);
  split.right = NULL;

  ak_split_buffer_free(&split);

  AK24_TEST_ASSERT_NULL(split.left);
  AK24_TEST_ASSERT_NULL(split.right);
  AK24_TEST_PASS();
}

int test_split_sequential_operations(void) {
  ak_buffer_t *buffer = ak_buffer_new(50);
  uint8_t data[30];
  for (int i = 0; i < 30; i++) {
    data[i] = i + 100;
  }
  ak_buffer_copy_to(buffer, data, 30);

  split_buffer_t split1 = ak_buffer_split(buffer, 10, 50, 50);
  AK24_TEST_ASSERT_NOT_NULL(split1.left);
  AK24_TEST_ASSERT_NOT_NULL(split1.right);

  split_buffer_t split2 = ak_buffer_split(split1.right, 10, 50, 50);
  AK24_TEST_ASSERT_NOT_NULL(split2.left);
  AK24_TEST_ASSERT_NOT_NULL(split2.right);

  AK24_TEST_ASSERT_EQ(ak_buffer_count(split1.left), 10);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(split2.left), 10);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(split2.right), 10);

  uint8_t *data1 = ak_buffer_data(split1.left);
  for (int i = 0; i < 10; i++) {
    AK24_TEST_ASSERT_EQ(data1[i], 100 + i);
  }

  uint8_t *data2 = ak_buffer_data(split2.left);
  for (int i = 0; i < 10; i++) {
    AK24_TEST_ASSERT_EQ(data2[i], 110 + i);
  }

  uint8_t *data3 = ak_buffer_data(split2.right);
  for (int i = 0; i < 10; i++) {
    AK24_TEST_ASSERT_EQ(data3[i], 120 + i);
  }

  ak_split_buffer_free(&split2);
  ak_split_buffer_free(&split1);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}
