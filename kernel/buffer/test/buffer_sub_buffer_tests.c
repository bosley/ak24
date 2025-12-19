#include "buffer.h"
#include "test/assert.h"
#include <string.h>

int test_sub_buffer_basic_extraction(void) {
  ak_buffer_t *buffer = ak_buffer_new(50);
  uint8_t data[50];
  for (int i = 0; i < 50; i++) {
    data[i] = i;
  }
  ak_buffer_copy_to(buffer, data, 50);

  int bytes_copied = 0;
  ak_buffer_t *sub = ak_buffer_sub_buffer(buffer, 10, 20, &bytes_copied);

  AK24_TEST_ASSERT_NOT_NULL(sub);
  AK24_TEST_ASSERT_EQ(bytes_copied, 20);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(sub), 20);

  uint8_t *sub_data = ak_buffer_data(sub);
  for (int i = 0; i < 20; i++) {
    AK24_TEST_ASSERT_EQ(sub_data[i], 10 + i);
  }

  ak_buffer_free(sub);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_sub_buffer_multiple_ranges(void) {
  ak_buffer_t *buffer = ak_buffer_new(100);
  uint8_t data[100];
  for (int i = 0; i < 100; i++) {
    data[i] = i % 256;
  }
  ak_buffer_copy_to(buffer, data, 100);

  int bytes1 = 0, bytes2 = 0, bytes3 = 0, bytes4 = 0;
  ak_buffer_t *sub1 = ak_buffer_sub_buffer(buffer, 0, 10, &bytes1);
  ak_buffer_t *sub2 = ak_buffer_sub_buffer(buffer, 25, 15, &bytes2);
  ak_buffer_t *sub3 = ak_buffer_sub_buffer(buffer, 50, 20, &bytes3);
  ak_buffer_t *sub4 = ak_buffer_sub_buffer(buffer, 90, 10, &bytes4);

  AK24_TEST_ASSERT_NOT_NULL(sub1);
  AK24_TEST_ASSERT_NOT_NULL(sub2);
  AK24_TEST_ASSERT_NOT_NULL(sub3);
  AK24_TEST_ASSERT_NOT_NULL(sub4);

  AK24_TEST_ASSERT_EQ(bytes1, 10);
  AK24_TEST_ASSERT_EQ(bytes2, 15);
  AK24_TEST_ASSERT_EQ(bytes3, 20);
  AK24_TEST_ASSERT_EQ(bytes4, 10);

  uint8_t *data1 = ak_buffer_data(sub1);
  for (int i = 0; i < 10; i++) {
    AK24_TEST_ASSERT_EQ(data1[i], i);
  }

  uint8_t *data2 = ak_buffer_data(sub2);
  for (int i = 0; i < 15; i++) {
    AK24_TEST_ASSERT_EQ(data2[i], 25 + i);
  }

  uint8_t *data3 = ak_buffer_data(sub3);
  for (int i = 0; i < 20; i++) {
    AK24_TEST_ASSERT_EQ(data3[i], 50 + i);
  }

  uint8_t *data4 = ak_buffer_data(sub4);
  for (int i = 0; i < 10; i++) {
    AK24_TEST_ASSERT_EQ(data4[i], 90 + i);
  }

  ak_buffer_free(sub1);
  ak_buffer_free(sub2);
  ak_buffer_free(sub3);
  ak_buffer_free(sub4);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_sub_buffer_offset_at_start(void) {
  ak_buffer_t *buffer = ak_buffer_new(30);
  uint8_t data[30];
  for (int i = 0; i < 30; i++) {
    data[i] = i + 100;
  }
  ak_buffer_copy_to(buffer, data, 30);

  int bytes_copied = 0;
  ak_buffer_t *sub = ak_buffer_sub_buffer(buffer, 0, 15, &bytes_copied);

  AK24_TEST_ASSERT_NOT_NULL(sub);
  AK24_TEST_ASSERT_EQ(bytes_copied, 15);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(sub), 15);

  uint8_t *sub_data = ak_buffer_data(sub);
  for (int i = 0; i < 15; i++) {
    AK24_TEST_ASSERT_EQ(sub_data[i], i + 100);
  }

  ak_buffer_free(sub);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_sub_buffer_offset_at_end(void) {
  ak_buffer_t *buffer = ak_buffer_new(50);
  uint8_t data[50];
  for (int i = 0; i < 50; i++) {
    data[i] = i * 2;
  }
  ak_buffer_copy_to(buffer, data, 50);

  int bytes_copied = 0;
  ak_buffer_t *sub = ak_buffer_sub_buffer(buffer, 45, 5, &bytes_copied);

  AK24_TEST_ASSERT_NOT_NULL(sub);
  AK24_TEST_ASSERT_EQ(bytes_copied, 5);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(sub), 5);

  uint8_t *sub_data = ak_buffer_data(sub);
  for (int i = 0; i < 5; i++) {
    AK24_TEST_ASSERT_EQ(sub_data[i], (45 + i) * 2);
  }

  ak_buffer_free(sub);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_sub_buffer_length_exceeds_available(void) {
  ak_buffer_t *buffer = ak_buffer_new(40);
  uint8_t data[40];
  for (int i = 0; i < 40; i++) {
    data[i] = 255 - i;
  }
  ak_buffer_copy_to(buffer, data, 40);

  int bytes_copied = 0;
  ak_buffer_t *sub = ak_buffer_sub_buffer(buffer, 30, 100, &bytes_copied);

  AK24_TEST_ASSERT_NOT_NULL(sub);
  AK24_TEST_ASSERT_EQ(bytes_copied, 10);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(sub), 10);

  uint8_t *sub_data = ak_buffer_data(sub);
  for (int i = 0; i < 10; i++) {
    AK24_TEST_ASSERT_EQ(sub_data[i], 255 - (30 + i));
  }

  ak_buffer_free(sub);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_sub_buffer_exact_boundary(void) {
  ak_buffer_t *buffer = ak_buffer_new(60);
  uint8_t data[60];
  for (int i = 0; i < 60; i++) {
    data[i] = i + 50;
  }
  ak_buffer_copy_to(buffer, data, 60);

  int bytes_copied = 0;
  ak_buffer_t *sub = ak_buffer_sub_buffer(buffer, 20, 40, &bytes_copied);

  AK24_TEST_ASSERT_NOT_NULL(sub);
  AK24_TEST_ASSERT_EQ(bytes_copied, 40);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(sub), 40);

  uint8_t *sub_data = ak_buffer_data(sub);
  for (int i = 0; i < 40; i++) {
    AK24_TEST_ASSERT_EQ(sub_data[i], 20 + i + 50);
  }

  ak_buffer_free(sub);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_sub_buffer_zero_length(void) {
  ak_buffer_t *buffer = ak_buffer_new(30);
  uint8_t data[30];
  for (int i = 0; i < 30; i++) {
    data[i] = i;
  }
  ak_buffer_copy_to(buffer, data, 30);

  int bytes_copied = 0;
  ak_buffer_t *sub = ak_buffer_sub_buffer(buffer, 10, 0, &bytes_copied);

  AK24_TEST_ASSERT_NOT_NULL(sub);
  AK24_TEST_ASSERT_EQ(bytes_copied, 0);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(sub), 0);

  ak_buffer_free(sub);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_sub_buffer_invalid_offset(void) {
  ak_buffer_t *buffer = ak_buffer_new(30);
  uint8_t data[30];
  for (int i = 0; i < 30; i++) {
    data[i] = i;
  }
  ak_buffer_copy_to(buffer, data, 30);

  int bytes_copied = 0;
  ak_buffer_t *sub = ak_buffer_sub_buffer(buffer, 50, 10, &bytes_copied);

  AK24_TEST_ASSERT_NULL(sub);
  AK24_TEST_ASSERT_EQ(bytes_copied, 0);

  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_sub_buffer_bytes_copied_null(void) {
  ak_buffer_t *buffer = ak_buffer_new(30);
  uint8_t data[30];
  for (int i = 0; i < 30; i++) {
    data[i] = i + 10;
  }
  ak_buffer_copy_to(buffer, data, 30);

  ak_buffer_t *sub = ak_buffer_sub_buffer(buffer, 5, 10, NULL);

  AK24_TEST_ASSERT_NOT_NULL(sub);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(sub), 10);

  uint8_t *sub_data = ak_buffer_data(sub);
  for (int i = 0; i < 10; i++) {
    AK24_TEST_ASSERT_EQ(sub_data[i], 5 + i + 10);
  }

  ak_buffer_free(sub);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_sub_buffer_null_buffer(void) {
  int bytes_copied = 99;
  ak_buffer_t *sub = ak_buffer_sub_buffer(NULL, 0, 10, &bytes_copied);

  AK24_TEST_ASSERT_NULL(sub);
  AK24_TEST_ASSERT_EQ(bytes_copied, 0);
  AK24_TEST_PASS();
}

int test_sub_buffer_sequential_extractions(void) {
  ak_buffer_t *buffer = ak_buffer_new(80);
  uint8_t data[80];
  for (int i = 0; i < 80; i++) {
    data[i] = i % 256;
  }
  ak_buffer_copy_to(buffer, data, 80);

  int b1 = 0, b2 = 0, b3 = 0;
  ak_buffer_t *sub1 = ak_buffer_sub_buffer(buffer, 10, 20, &b1);
  ak_buffer_t *sub2 = ak_buffer_sub_buffer(buffer, 15, 25, &b2);
  ak_buffer_t *sub3 = ak_buffer_sub_buffer(buffer, 30, 10, &b3);

  AK24_TEST_ASSERT_NOT_NULL(sub1);
  AK24_TEST_ASSERT_NOT_NULL(sub2);
  AK24_TEST_ASSERT_NOT_NULL(sub3);

  AK24_TEST_ASSERT_EQ(b1, 20);
  AK24_TEST_ASSERT_EQ(b2, 25);
  AK24_TEST_ASSERT_EQ(b3, 10);

  uint8_t *data1 = ak_buffer_data(sub1);
  for (int i = 0; i < 20; i++) {
    AK24_TEST_ASSERT_EQ(data1[i], (10 + i) % 256);
  }

  uint8_t *data2 = ak_buffer_data(sub2);
  for (int i = 0; i < 25; i++) {
    AK24_TEST_ASSERT_EQ(data2[i], (15 + i) % 256);
  }

  uint8_t *data3 = ak_buffer_data(sub3);
  for (int i = 0; i < 10; i++) {
    AK24_TEST_ASSERT_EQ(data3[i], (30 + i) % 256);
  }

  ak_buffer_free(sub1);
  ak_buffer_free(sub2);
  ak_buffer_free(sub3);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_sub_buffer_large_buffer_chunks(void) {
  ak_buffer_t *buffer = ak_buffer_new(1000);
  uint8_t data[1000];
  for (int i = 0; i < 1000; i++) {
    data[i] = i % 256;
  }
  ak_buffer_copy_to(buffer, data, 1000);

  ak_buffer_t *subs[10];
  int bytes[10];

  for (int chunk = 0; chunk < 10; chunk++) {
    subs[chunk] = ak_buffer_sub_buffer(buffer, chunk * 100, 100, &bytes[chunk]);
    AK24_TEST_ASSERT_NOT_NULL(subs[chunk]);
    AK24_TEST_ASSERT_EQ(bytes[chunk], 100);
    AK24_TEST_ASSERT_EQ(ak_buffer_count(subs[chunk]), 100);
  }

  for (int chunk = 0; chunk < 10; chunk++) {
    uint8_t *chunk_data = ak_buffer_data(subs[chunk]);
    for (int i = 0; i < 100; i++) {
      int expected_value = (chunk * 100 + i) % 256;
      AK24_TEST_ASSERT_EQ(chunk_data[i], expected_value);
    }
  }

  for (int chunk = 0; chunk < 10; chunk++) {
    ak_buffer_free(subs[chunk]);
  }
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_sub_buffer_full_copy(void) {
  ak_buffer_t *buffer = ak_buffer_new(25);
  uint8_t data[25];
  for (int i = 0; i < 25; i++) {
    data[i] = i * 3;
  }
  ak_buffer_copy_to(buffer, data, 25);

  int bytes_copied = 0;
  ak_buffer_t *sub = ak_buffer_sub_buffer(buffer, 0, 25, &bytes_copied);

  AK24_TEST_ASSERT_NOT_NULL(sub);
  AK24_TEST_ASSERT_EQ(bytes_copied, 25);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(sub), 25);

  uint8_t *sub_data = ak_buffer_data(sub);
  for (int i = 0; i < 25; i++) {
    AK24_TEST_ASSERT_EQ(sub_data[i], i * 3);
  }

  ak_buffer_free(sub);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int test_sub_buffer_single_byte(void) {
  ak_buffer_t *buffer = ak_buffer_new(20);
  uint8_t data[20];
  for (int i = 0; i < 20; i++) {
    data[i] = i + 200;
  }
  ak_buffer_copy_to(buffer, data, 20);

  int bytes_copied = 0;
  ak_buffer_t *sub = ak_buffer_sub_buffer(buffer, 10, 1, &bytes_copied);

  AK24_TEST_ASSERT_NOT_NULL(sub);
  AK24_TEST_ASSERT_EQ(bytes_copied, 1);
  AK24_TEST_ASSERT_EQ(ak_buffer_count(sub), 1);

  uint8_t *sub_data = ak_buffer_data(sub);
  AK24_TEST_ASSERT_EQ(sub_data[0], 210);

  ak_buffer_free(sub);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}
