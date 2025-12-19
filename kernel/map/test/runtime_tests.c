#include "kernel.h"
#include "map.h"
#include "test/assert.h"
#include <string.h>

int map_basic_int_test(void) {
  map_int_t m;
  map_init_generic(&m, sizeof(int), map_hash_i32, map_cmp_mem);

  int key1 = 42;
  int val1 = 100;
  AK24_TEST_ASSERT_EQ(map_set_generic(&m, &key1, val1), 0);

  int *result = map_get_generic(&m, &key1);
  AK24_TEST_ASSERT_NOT_NULL(result);
  AK24_TEST_ASSERT_EQ(*result, 100);

  int key2 = 99;
  int *result2 = map_get_generic(&m, &key2);
  AK24_TEST_ASSERT_NULL(result2);

  map_deinit(&m);
  AK24_TEST_PASS();
}

int map_string_key_test(void) {
  map_int_t m;
  map_init_generic(&m, sizeof(char *), map_hash_str, map_cmp_str);

  char *key1 = "hello";
  int val1 = 123;
  AK24_TEST_ASSERT_EQ(map_set_generic(&m, &key1, val1), 0);

  char *key2 = "world";
  int val2 = 456;
  AK24_TEST_ASSERT_EQ(map_set_generic(&m, &key2, val2), 0);

  int *result1 = map_get_generic(&m, &key1);
  AK24_TEST_ASSERT_NOT_NULL(result1);
  AK24_TEST_ASSERT_EQ(*result1, 123);

  int *result2 = map_get_generic(&m, &key2);
  AK24_TEST_ASSERT_NOT_NULL(result2);
  AK24_TEST_ASSERT_EQ(*result2, 456);

  char *key3 = "missing";
  int *result3 = map_get_generic(&m, &key3);
  AK24_TEST_ASSERT_NULL(result3);

  map_deinit(&m);
  AK24_TEST_PASS();
}

int map_update_test(void) {
  map_int_t m;
  map_init_generic(&m, sizeof(int), map_hash_i32, map_cmp_mem);

  int key = 10;
  int val1 = 100;
  AK24_TEST_ASSERT_EQ(map_set_generic(&m, &key, val1), 0);

  int *result = map_get_generic(&m, &key);
  AK24_TEST_ASSERT_NOT_NULL(result);
  AK24_TEST_ASSERT_EQ(*result, 100);

  int val2 = 200;
  AK24_TEST_ASSERT_EQ(map_set_generic(&m, &key, val2), 0);

  result = map_get_generic(&m, &key);
  AK24_TEST_ASSERT_NOT_NULL(result);
  AK24_TEST_ASSERT_EQ(*result, 200);

  map_deinit(&m);
  AK24_TEST_PASS();
}

int map_remove_test(void) {
  map_int_t m;
  map_init_generic(&m, sizeof(int), map_hash_i32, map_cmp_mem);

  int key1 = 1;
  int val1 = 10;
  AK24_TEST_ASSERT_EQ(map_set_generic(&m, &key1, val1), 0);

  int key2 = 2;
  int val2 = 20;
  AK24_TEST_ASSERT_EQ(map_set_generic(&m, &key2, val2), 0);

  int *result = map_get_generic(&m, &key1);
  AK24_TEST_ASSERT_NOT_NULL(result);

  map_remove_generic(&m, &key1);

  result = map_get_generic(&m, &key1);
  AK24_TEST_ASSERT_NULL(result);

  result = map_get_generic(&m, &key2);
  AK24_TEST_ASSERT_NOT_NULL(result);
  AK24_TEST_ASSERT_EQ(*result, 20);

  map_deinit(&m);
  AK24_TEST_PASS();
}

int map_iteration_test(void) {
  map_int_t m;
  map_init_generic(&m, sizeof(int), map_hash_i32, map_cmp_mem);

  int key1 = 1, val1 = 10;
  int key2 = 2, val2 = 20;
  int key3 = 3, val3 = 30;

  AK24_TEST_ASSERT_EQ(map_set_generic(&m, &key1, val1), 0);
  AK24_TEST_ASSERT_EQ(map_set_generic(&m, &key2, val2), 0);
  AK24_TEST_ASSERT_EQ(map_set_generic(&m, &key3, val3), 0);

  int count = 0;
  map_iter_t iter = map_iter(&m);
  void *key;
  while ((key = map_next_generic(&m, &iter))) {
    count++;
  }

  AK24_TEST_ASSERT_EQ(count, 3);

  map_deinit(&m);
  AK24_TEST_PASS();
}

int map_multiple_types_test(void) {
  map_int_t m_i8;
  map_init_generic(&m_i8, sizeof(signed char), map_hash_i8, map_cmp_mem);
  signed char k_i8 = 5;
  int v_i8 = 50;
  AK24_TEST_ASSERT_EQ(map_set_generic(&m_i8, &k_i8, v_i8), 0);
  int *r_i8 = map_get_generic(&m_i8, &k_i8);
  AK24_TEST_ASSERT_NOT_NULL(r_i8);
  AK24_TEST_ASSERT_EQ(*r_i8, 50);
  map_deinit(&m_i8);

  map_int_t m_u32;
  map_init_generic(&m_u32, sizeof(unsigned int), map_hash_u32, map_cmp_mem);
  unsigned int k_u32 = 12345;
  int v_u32 = 999;
  AK24_TEST_ASSERT_EQ(map_set_generic(&m_u32, &k_u32, v_u32), 0);
  int *r_u32 = map_get_generic(&m_u32, &k_u32);
  AK24_TEST_ASSERT_NOT_NULL(r_u32);
  AK24_TEST_ASSERT_EQ(*r_u32, 999);
  map_deinit(&m_u32);

  map_float_t m_f32;
  map_init_generic(&m_f32, sizeof(float), map_hash_f32, map_cmp_mem);
  float k_f32 = 3.14f;
  float v_f32 = 2.71f;
  AK24_TEST_ASSERT_EQ(map_set_generic(&m_f32, &k_f32, v_f32), 0);
  float *r_f32 = map_get_generic(&m_f32, &k_f32);
  AK24_TEST_ASSERT_NOT_NULL(r_f32);
  map_deinit(&m_f32);

  AK24_TEST_PASS();
}

int map_large_dataset_test(void) {
  map_int_t m;
  map_init_generic(&m, sizeof(int), map_hash_i32, map_cmp_mem);

  for (int i = 0; i < 100; i++) {
    int key = i;
    int val = i * 10;
    AK24_TEST_ASSERT_EQ(map_set_generic(&m, &key, val), 0);
  }

  for (int i = 0; i < 100; i++) {
    int key = i;
    int *result = map_get_generic(&m, &key);
    AK24_TEST_ASSERT_NOT_NULL(result);
    AK24_TEST_ASSERT_EQ(*result, i * 10);
  }

  int count = 0;
  map_iter_t iter = map_iter(&m);
  void *key;
  while ((key = map_next_generic(&m, &iter))) {
    count++;
  }
  AK24_TEST_ASSERT_EQ(count, 100);

  map_deinit(&m);
  AK24_TEST_PASS();
}

int run_map_tests(void) {
  AK24_TEST_RUN(map_basic_int_test);
  AK24_TEST_RUN(map_string_key_test);
  AK24_TEST_RUN(map_update_test);
  AK24_TEST_RUN(map_remove_test);
  AK24_TEST_RUN(map_iteration_test);
  AK24_TEST_RUN(map_multiple_types_test);
  AK24_TEST_RUN(map_large_dataset_test);
  return 0;
}

int main(void) {
  ak_kernel_init();
  int result = run_map_tests();
  ak_kernel_deinit();
  return result;
}
