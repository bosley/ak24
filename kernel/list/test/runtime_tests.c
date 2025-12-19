#include "kernel.h"
#include "list.h"
#include "test/assert.h"
#include <string.h>

int list_basic_operations_test(void) {
  list_int_t l;
  list_init(&l);

  AK24_TEST_ASSERT_EQ(list_count(&l), 0);
  AK24_TEST_ASSERT_EQ(list_capacity(&l), 0);

  AK24_TEST_ASSERT_EQ(list_push(&l, 10), 0);
  AK24_TEST_ASSERT_EQ(list_count(&l), 1);
  AK24_TEST_ASSERT(list_capacity(&l) >= 1);

  AK24_TEST_ASSERT_EQ(list_push(&l, 20), 0);
  AK24_TEST_ASSERT_EQ(list_push(&l, 30), 0);
  AK24_TEST_ASSERT_EQ(list_count(&l), 3);

  int *val = list_get(&l, 0);
  AK24_TEST_ASSERT_NOT_NULL(val);
  AK24_TEST_ASSERT_EQ(*val, 10);

  val = list_get(&l, 1);
  AK24_TEST_ASSERT_NOT_NULL(val);
  AK24_TEST_ASSERT_EQ(*val, 20);

  val = list_get(&l, 2);
  AK24_TEST_ASSERT_NOT_NULL(val);
  AK24_TEST_ASSERT_EQ(*val, 30);

  val = list_get(&l, 3);
  AK24_TEST_ASSERT_NULL(val);

  AK24_TEST_ASSERT_EQ(list_set(&l, 1, 25), 0);
  val = list_get(&l, 1);
  AK24_TEST_ASSERT_NOT_NULL(val);
  AK24_TEST_ASSERT_EQ(*val, 25);

  val = list_pop(&l);
  AK24_TEST_ASSERT_NOT_NULL(val);
  AK24_TEST_ASSERT_EQ(*val, 30);
  AK24_TEST_ASSERT_EQ(list_count(&l), 2);

  list_deinit(&l);
  AK24_TEST_PASS();
}

int list_insert_remove_test(void) {
  list_int_t l;
  list_init(&l);

  list_push(&l, 10);
  list_push(&l, 30);
  list_push(&l, 40);

  AK24_TEST_ASSERT_EQ(list_insert(&l, 1, 20), 0);
  AK24_TEST_ASSERT_EQ(list_count(&l), 4);

  int *val = list_get(&l, 0);
  AK24_TEST_ASSERT_EQ(*val, 10);
  val = list_get(&l, 1);
  AK24_TEST_ASSERT_EQ(*val, 20);
  val = list_get(&l, 2);
  AK24_TEST_ASSERT_EQ(*val, 30);
  val = list_get(&l, 3);
  AK24_TEST_ASSERT_EQ(*val, 40);

  AK24_TEST_ASSERT_EQ(list_insert(&l, 0, 5), 0);
  AK24_TEST_ASSERT_EQ(list_count(&l), 5);
  val = list_get(&l, 0);
  AK24_TEST_ASSERT_EQ(*val, 5);

  AK24_TEST_ASSERT_EQ(list_insert(&l, 5, 50), 0);
  AK24_TEST_ASSERT_EQ(list_count(&l), 6);
  val = list_get(&l, 5);
  AK24_TEST_ASSERT_EQ(*val, 50);

  AK24_TEST_ASSERT_EQ(list_remove(&l, 2), 0);
  AK24_TEST_ASSERT_EQ(list_count(&l), 5);
  val = list_get(&l, 2);
  AK24_TEST_ASSERT_EQ(*val, 30);

  AK24_TEST_ASSERT_EQ(list_remove(&l, 0), 0);
  AK24_TEST_ASSERT_EQ(list_count(&l), 4);
  val = list_get(&l, 0);
  AK24_TEST_ASSERT_EQ(*val, 10);

  AK24_TEST_ASSERT_EQ(list_remove(&l, 3), 0);
  AK24_TEST_ASSERT_EQ(list_count(&l), 3);

  list_deinit(&l);
  AK24_TEST_PASS();
}

int list_iteration_test(void) {
  list_int_t l;
  list_init(&l);

  list_push(&l, 10);
  list_push(&l, 20);
  list_push(&l, 30);
  list_push(&l, 40);
  list_push(&l, 50);

  int count = 0;
  int sum = 0;
  list_iter_t iter = list_iter(&l);
  int *val;
  while ((val = list_next(&l, &iter))) {
    count++;
    sum += *val;
  }

  AK24_TEST_ASSERT_EQ(count, 5);
  AK24_TEST_ASSERT_EQ(sum, 150);

  list_deinit(&l);
  AK24_TEST_PASS();
}

int list_clear_test(void) {
  list_int_t l;
  list_init(&l);

  list_push(&l, 10);
  list_push(&l, 20);
  list_push(&l, 30);
  AK24_TEST_ASSERT_EQ(list_count(&l), 3);

  unsigned old_capacity = list_capacity(&l);
  list_clear(&l);
  AK24_TEST_ASSERT_EQ(list_count(&l), 0);
  AK24_TEST_ASSERT_EQ(list_capacity(&l), old_capacity);

  list_push(&l, 100);
  AK24_TEST_ASSERT_EQ(list_count(&l), 1);
  int *val = list_get(&l, 0);
  AK24_TEST_ASSERT_EQ(*val, 100);

  list_deinit(&l);
  AK24_TEST_PASS();
}

int list_resize_test(void) {
  list_int_t l;
  list_init(&l);

  AK24_TEST_ASSERT_EQ(list_capacity(&l), 0);

  for (int i = 0; i < 100; i++) {
    AK24_TEST_ASSERT_EQ(list_push(&l, i), 0);
  }

  AK24_TEST_ASSERT_EQ(list_count(&l), 100);
  AK24_TEST_ASSERT(list_capacity(&l) >= 100);

  for (int i = 0; i < 100; i++) {
    int *val = list_get(&l, i);
    AK24_TEST_ASSERT_NOT_NULL(val);
    AK24_TEST_ASSERT_EQ(*val, i);
  }

  list_deinit(&l);
  AK24_TEST_PASS();
}

int list_multiple_types_test(void) {
  list_int_t l_int;
  list_init(&l_int);
  list_push(&l_int, 42);
  int *vi = list_get(&l_int, 0);
  AK24_TEST_ASSERT_EQ(*vi, 42);
  list_deinit(&l_int);

  list_float_t l_float;
  list_init(&l_float);
  list_push(&l_float, 3.14f);
  float *vf = list_get(&l_float, 0);
  AK24_TEST_ASSERT(*vf > 3.13f && *vf < 3.15f);
  list_deinit(&l_float);

  list_double_t l_double;
  list_init(&l_double);
  list_push(&l_double, 2.71828);
  double *vd = list_get(&l_double, 0);
  AK24_TEST_ASSERT(*vd > 2.71 && *vd < 2.72);
  list_deinit(&l_double);

  list_str_t l_str;
  list_init(&l_str);
  char *str1 = "hello";
  char *str2 = "world";
  list_push(&l_str, str1);
  list_push(&l_str, str2);
  char **vs = list_get(&l_str, 0);
  AK24_TEST_ASSERT_STR_EQ(*vs, "hello");
  vs = list_get(&l_str, 1);
  AK24_TEST_ASSERT_STR_EQ(*vs, "world");
  list_deinit(&l_str);

  AK24_TEST_PASS();
}

int list_nested_list_test(void) {
  typedef list_t(list_int_t *) list_list_int_t;

  list_list_int_t outer;
  list_init(&outer);

  list_int_t *inner1 = AK24_ALLOC(sizeof(list_int_t));
  list_init(inner1);
  list_push(inner1, 10);
  list_push(inner1, 20);
  list_push(&outer, inner1);

  list_int_t *inner2 = AK24_ALLOC(sizeof(list_int_t));
  list_init(inner2);
  list_push(inner2, 30);
  list_push(inner2, 40);
  list_push(inner2, 50);
  list_push(&outer, inner2);

  AK24_TEST_ASSERT_EQ(list_count(&outer), 2);

  list_int_t **retrieved1 = list_get(&outer, 0);
  AK24_TEST_ASSERT_NOT_NULL(retrieved1);
  AK24_TEST_ASSERT_EQ(list_count(*retrieved1), 2);
  int *val = list_get(*retrieved1, 0);
  AK24_TEST_ASSERT_EQ(*val, 10);
  val = list_get(*retrieved1, 1);
  AK24_TEST_ASSERT_EQ(*val, 20);

  list_int_t **retrieved2 = list_get(&outer, 1);
  AK24_TEST_ASSERT_NOT_NULL(retrieved2);
  AK24_TEST_ASSERT_EQ(list_count(*retrieved2), 3);
  val = list_get(*retrieved2, 0);
  AK24_TEST_ASSERT_EQ(*val, 30);
  val = list_get(*retrieved2, 1);
  AK24_TEST_ASSERT_EQ(*val, 40);
  val = list_get(*retrieved2, 2);
  AK24_TEST_ASSERT_EQ(*val, 50);

  list_deinit(inner1);
  AK24_FREE(inner1);
  list_deinit(inner2);
  AK24_FREE(inner2);
  list_deinit(&outer);

  AK24_TEST_PASS();
}

int list_large_dataset_test(void) {
  list_int_t l;
  list_init(&l);

  for (int i = 0; i < 1000; i++) {
    AK24_TEST_ASSERT_EQ(list_push(&l, i * 2), 0);
  }

  AK24_TEST_ASSERT_EQ(list_count(&l), 1000);

  for (int i = 0; i < 1000; i++) {
    int *val = list_get(&l, i);
    AK24_TEST_ASSERT_NOT_NULL(val);
    AK24_TEST_ASSERT_EQ(*val, i * 2);
  }

  int count = 0;
  list_iter_t iter = list_iter(&l);
  int *val;
  while ((val = list_next(&l, &iter))) {
    count++;
  }
  AK24_TEST_ASSERT_EQ(count, 1000);

  for (int i = 0; i < 500; i++) {
    AK24_TEST_ASSERT_EQ(list_remove(&l, 0), 0);
  }
  AK24_TEST_ASSERT_EQ(list_count(&l), 500);

  val = list_get(&l, 0);
  AK24_TEST_ASSERT_EQ(*val, 1000);

  list_deinit(&l);
  AK24_TEST_PASS();
}

int list_rotate_test(void) {
  list_int_t l;
  list_init(&l);

  for (int i = 1; i <= 5; i++) {
    list_push(&l, i);
  }

  AK24_TEST_ASSERT_EQ(list_rotate_left(&l, 2), 0);

  int *val = list_get(&l, 0);
  AK24_TEST_ASSERT_EQ(*val, 3);
  val = list_get(&l, 1);
  AK24_TEST_ASSERT_EQ(*val, 4);
  val = list_get(&l, 2);
  AK24_TEST_ASSERT_EQ(*val, 5);
  val = list_get(&l, 3);
  AK24_TEST_ASSERT_EQ(*val, 1);
  val = list_get(&l, 4);
  AK24_TEST_ASSERT_EQ(*val, 2);

  AK24_TEST_ASSERT_EQ(list_rotate_right(&l, 1), 0);

  val = list_get(&l, 0);
  AK24_TEST_ASSERT_EQ(*val, 2);
  val = list_get(&l, 1);
  AK24_TEST_ASSERT_EQ(*val, 3);
  val = list_get(&l, 2);
  AK24_TEST_ASSERT_EQ(*val, 4);
  val = list_get(&l, 3);
  AK24_TEST_ASSERT_EQ(*val, 5);
  val = list_get(&l, 4);
  AK24_TEST_ASSERT_EQ(*val, 1);

  AK24_TEST_ASSERT_EQ(list_rotate_left(&l, 10), 0);

  val = list_get(&l, 0);
  AK24_TEST_ASSERT_EQ(*val, 2);
  val = list_get(&l, 1);
  AK24_TEST_ASSERT_EQ(*val, 3);

  AK24_TEST_ASSERT_EQ(list_rotate_right(&l, 0), 0);
  val = list_get(&l, 0);
  AK24_TEST_ASSERT_EQ(*val, 2);

  list_deinit(&l);
  AK24_TEST_PASS();
}

int run_list_tests(void) {
  AK24_TEST_RUN(list_basic_operations_test);
  AK24_TEST_RUN(list_insert_remove_test);
  AK24_TEST_RUN(list_iteration_test);
  AK24_TEST_RUN(list_clear_test);
  AK24_TEST_RUN(list_resize_test);
  AK24_TEST_RUN(list_multiple_types_test);
  AK24_TEST_RUN(list_nested_list_test);
  AK24_TEST_RUN(list_large_dataset_test);
  AK24_TEST_RUN(list_rotate_test);
  return 0;
}

int main(void) {
  ak_kernel_init();
  int result = run_list_tests();
  ak_kernel_deinit();
  return result;
}
