#include "buffer.h"
#include "test/assert.h"
#include <string.h>

static int increment_byte(uint8_t *byte, size_t idx, void *callback_data) {
  (void)idx;
  (void)callback_data;
  (*byte)++;
  return 1;
}

static int stop_at_5(uint8_t *byte, size_t idx, void *callback_data) {
  (void)callback_data;
  if (idx >= 5) {
    return 0;
  }
  (*byte) *= 2;
  return 1;
}

static int skip_every_other(uint8_t *byte, size_t idx, void *callback_data) {
  (void)idx;
  (void)callback_data;
  (*byte) += 10;
  return 2;
}

static int count_callback_calls = 0;
static int counting_callback(uint8_t *byte, size_t idx, void *callback_data) {
  (void)byte;
  (void)idx;
  (void)callback_data;
  count_callback_calls++;
  return 1;
}

int test_buffer_create_destroy(void) {
  ak_buffer_t *buffer = ak_buffer_new(100);
  AK24_TEST_ASSERT_NOT_NULL(buffer);
  AK24_TEST_ASSERT_NOT_NULL(buffer->data);
  AK24_TEST_ASSERT_EQ(buffer->capacity, 100);
  AK24_TEST_ASSERT_EQ(buffer->count, 0);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_buffer_create_min_size(void) {
  ak_buffer_t *buffer = ak_buffer_new(1);
  AK24_TEST_ASSERT_NOT_NULL(buffer);
  AK24_TEST_ASSERT_EQ(buffer->capacity, 16);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_buffer_copy_to_basic(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5};

  int result = ak_buffer_copy_to(buffer, data, 5);
  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 5);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_NOT_NULL(buf_data);
  AK24_TEST_ASSERT_EQ(buf_data[0], 1);
  AK24_TEST_ASSERT_EQ(buf_data[1], 2);
  AK24_TEST_ASSERT_EQ(buf_data[2], 3);
  AK24_TEST_ASSERT_EQ(buf_data[3], 4);
  AK24_TEST_ASSERT_EQ(buf_data[4], 5);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_buffer_copy_to_multiple(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data1[] = {1, 2, 3};
  uint8_t data2[] = {4, 5, 6};
  uint8_t data3[] = {7, 8, 9};

  ak_buffer_copy_to(buffer, data1, 3);
  ak_buffer_copy_to(buffer, data2, 3);
  ak_buffer_copy_to(buffer, data3, 3);

  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 9);

  uint8_t *buf_data = ak_buffer_data(buffer);
  for (int i = 0; i < 9; i++) {
    AK24_TEST_ASSERT_EQ(buf_data[i], i + 1);
  }

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_buffer_reallocation(void) {
  ak_buffer_t *buffer = ak_buffer_new(8);
  AK24_TEST_ASSERT_EQ(buffer->capacity, 16);

  uint8_t data[40];
  for (int i = 0; i < 40; i++) {
    data[i] = i;
  }

  int result = ak_buffer_copy_to(buffer, data, 40);
  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 40);
  AK24_TEST_ASSERT(buffer->capacity >= 40);

  uint8_t *buf_data = ak_buffer_data(buffer);
  for (int i = 0; i < 40; i++) {
    AK24_TEST_ASSERT_EQ(buf_data[i], i);
  }

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_buffer_clear(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5};

  ak_buffer_copy_to(buffer, data, 5);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 5);

  ak_buffer_clear(buffer);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 0);

  ak_buffer_copy_to(buffer, data, 3);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 3);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_buffer_for_each_increment(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {0, 1, 2, 3, 4};

  ak_buffer_copy_to(buffer, data, 5);
  ak_buffer_for_each(buffer, increment_byte, NULL);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 1);
  AK24_TEST_ASSERT_EQ(buf_data[1], 2);
  AK24_TEST_ASSERT_EQ(buf_data[2], 3);
  AK24_TEST_ASSERT_EQ(buf_data[3], 4);
  AK24_TEST_ASSERT_EQ(buf_data[4], 5);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_buffer_for_each_stop(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

  ak_buffer_copy_to(buffer, data, 10);
  ak_buffer_for_each(buffer, stop_at_5, NULL);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 2);
  AK24_TEST_ASSERT_EQ(buf_data[1], 2);
  AK24_TEST_ASSERT_EQ(buf_data[2], 2);
  AK24_TEST_ASSERT_EQ(buf_data[3], 2);
  AK24_TEST_ASSERT_EQ(buf_data[4], 2);
  AK24_TEST_ASSERT_EQ(buf_data[5], 1);
  AK24_TEST_ASSERT_EQ(buf_data[6], 1);
  AK24_TEST_ASSERT_EQ(buf_data[7], 1);
  AK24_TEST_ASSERT_EQ(buf_data[8], 1);
  AK24_TEST_ASSERT_EQ(buf_data[9], 1);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_buffer_for_each_skip(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {0, 0, 0, 0, 0, 0, 0, 0};

  ak_buffer_copy_to(buffer, data, 8);
  ak_buffer_for_each(buffer, skip_every_other, NULL);

  uint8_t *buf_data = ak_buffer_data(buffer);
  AK24_TEST_ASSERT_EQ(buf_data[0], 10);
  AK24_TEST_ASSERT_EQ(buf_data[1], 0);
  AK24_TEST_ASSERT_EQ(buf_data[2], 10);
  AK24_TEST_ASSERT_EQ(buf_data[3], 0);
  AK24_TEST_ASSERT_EQ(buf_data[4], 10);
  AK24_TEST_ASSERT_EQ(buf_data[5], 0);
  AK24_TEST_ASSERT_EQ(buf_data[6], 10);
  AK24_TEST_ASSERT_EQ(buf_data[7], 0);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_buffer_empty(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);

  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 0);

  count_callback_calls = 0;
  ak_buffer_for_each(buffer, counting_callback, NULL);
  AK24_TEST_ASSERT_EQ(count_callback_calls, 0);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_buffer_null_checks(void) {
  ak_buffer_free(NULL);

  AK24_TEST_ASSERT_EQ(ak_buffer_count(NULL), 0);
  AK24_TEST_ASSERT_NULL(ak_buffer_data(NULL));

  ak_buffer_clear(NULL);

  ak_buffer_t *buffer = ak_buffer_new(32);
  AK24_TEST_ASSERT_EQ(ak_buffer_copy_to(buffer, NULL, 10), -1);
  AK24_TEST_ASSERT_EQ(ak_buffer_copy_to(NULL, (uint8_t *)"test", 4), -1);

  ak_buffer_for_each(NULL, increment_byte, NULL);
  ak_buffer_for_each(buffer, NULL, NULL);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_buffer_large_data(void) {
  ak_buffer_t *buffer = ak_buffer_new(16);

  uint8_t large_data[1000];
  for (int i = 0; i < 1000; i++) {
    large_data[i] = i % 256;
  }

  int result = ak_buffer_copy_to(buffer, large_data, 1000);
  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 1000);

  uint8_t *buf_data = ak_buffer_data(buffer);
  for (int i = 0; i < 1000; i++) {
    AK24_TEST_ASSERT_EQ(buf_data[i], i % 256);
  }

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_buffer_zero_length_copy(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[] = {1, 2, 3};

  int result = ak_buffer_copy_to(buffer, data, 0);
  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 0);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_buffer_shrink_to_fit(void) {
  ak_buffer_t *buffer = ak_buffer_new(100);
  AK24_TEST_ASSERT_EQ(buffer->capacity, 100);

  uint8_t data[] = {1, 2, 3, 4, 5};
  ak_buffer_copy_to(buffer, data, 5);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 5);
  AK24_TEST_ASSERT_EQ(buffer->capacity, 100);

  int result = ak_buffer_shrink_to_fit(buffer);
  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(buffer->capacity, 5);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 5);

  uint8_t *buf_data = ak_buffer_data(buffer);
  for (int i = 0; i < 5; i++) {
    AK24_TEST_ASSERT_EQ(buf_data[i], i + 1);
  }

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_buffer_shrink_to_fit_empty(void) {
  ak_buffer_t *buffer = ak_buffer_new(100);

  int result = ak_buffer_shrink_to_fit(buffer);
  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(buffer->capacity, 100);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_buffer_shrink_to_fit_already_fit(void) {
  ak_buffer_t *buffer = ak_buffer_new(32);
  uint8_t data[32];
  for (int i = 0; i < 32; i++) {
    data[i] = i;
  }

  ak_buffer_copy_to(buffer, data, 32);
  AK24_TEST_ASSERT_EQ(buffer->capacity, 32);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(buffer), 32);

  int result = ak_buffer_shrink_to_fit(buffer);
  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(buffer->capacity, 32);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_buffer_shrink_to_fit_null(void) {
  int result = ak_buffer_shrink_to_fit(NULL);
  AK24_TEST_ASSERT_EQ(result, -1);
  AK24_TEST_PASS();
}

int test_buffer_sub_buffer_validation(void) {
  ak_buffer_t *buffer = ak_buffer_new(100);
  uint8_t data[100];
  for (int i = 0; i < 100; i++) {
    data[i] = i;
  }
  ak_buffer_copy_to(buffer, data, 100);

  int bytes_copied = 0;
  ak_buffer_t *sub = ak_buffer_sub_buffer(buffer, 20, 30, &bytes_copied);

  AK24_TEST_ASSERT_NOT_NULL(sub);
  AK24_TEST_ASSERT_EQ(bytes_copied, 30);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(sub), 30);

  uint8_t *sub_data = ak_buffer_data(sub);
  for (int i = 0; i < 30; i++) {
    AK24_TEST_ASSERT_EQ(sub_data[i], 20 + i);
  }

  ak_buffer_free(sub);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}
