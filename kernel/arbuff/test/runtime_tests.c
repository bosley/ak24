#include "arbuff.h"
#include "kernel.h"
#include "test/assert.h"
#include <stdio.h>
#include <unistd.h>

static int test_arbuff_new_zero_capacity(void) {
  ak_arbuff_t *arbuff = ak_arbuff_new(0);
  AK24_TEST_ASSERT_NULL(arbuff);
  AK24_TEST_PASS();
}

static int test_arbuff_new_valid(void) {
  ak_arbuff_t *arbuff = ak_arbuff_new(10);
  AK24_TEST_ASSERT_NOT_NULL(arbuff);
  AK24_TEST_ASSERT_NOT_NULL(arbuff->slots);
  AK24_TEST_ASSERT_EQ(ak_arbuff_count(arbuff), 0);
  AK24_TEST_ASSERT_EQ(ak_arbuff_is_empty(arbuff), 1);
  AK24_TEST_ASSERT_EQ(ak_arbuff_is_full(arbuff), 0);
  ak_arbuff_free(arbuff);
  AK24_TEST_PASS();
}

static int test_arbuff_push_pop_single(void) {
  ak_arbuff_t *arbuff = ak_arbuff_new(5);
  AK24_TEST_ASSERT_NOT_NULL(arbuff);

  int value = 42;
  int result = ak_arbuff_push(arbuff, &value);
  AK24_TEST_ASSERT_EQ(result, 0);
  AK24_TEST_ASSERT_EQ(ak_arbuff_count(arbuff), 1);
  AK24_TEST_ASSERT_EQ(ak_arbuff_is_empty(arbuff), 0);

  int *retrieved = (int *)ak_arbuff_pop(arbuff);
  AK24_TEST_ASSERT_NOT_NULL(retrieved);
  AK24_TEST_ASSERT_EQ(*retrieved, 42);
  AK24_TEST_ASSERT_EQ(ak_arbuff_count(arbuff), 0);
  AK24_TEST_ASSERT_EQ(ak_arbuff_is_empty(arbuff), 1);

  ak_arbuff_free(arbuff);
  AK24_TEST_PASS();
}

static int test_arbuff_push_pop_multiple(void) {
  ak_arbuff_t *arbuff = ak_arbuff_new(8);
  AK24_TEST_ASSERT_NOT_NULL(arbuff);

  int values[] = {10, 20, 30, 40, 50};

  for (int i = 0; i < 5; i++) {
    int result = ak_arbuff_push(arbuff, &values[i]);
    AK24_TEST_ASSERT_EQ(result, 0);
  }

  AK24_TEST_ASSERT_EQ(ak_arbuff_count(arbuff), 5);

  for (int i = 0; i < 5; i++) {
    int *retrieved = (int *)ak_arbuff_pop(arbuff);
    AK24_TEST_ASSERT_NOT_NULL(retrieved);
    AK24_TEST_ASSERT_EQ(*retrieved, values[i]);
  }

  AK24_TEST_ASSERT_EQ(ak_arbuff_count(arbuff), 0);
  AK24_TEST_ASSERT_EQ(ak_arbuff_is_empty(arbuff), 1);

  ak_arbuff_free(arbuff);
  AK24_TEST_PASS();
}

static int test_arbuff_overflow(void) {
  ak_arbuff_t *arbuff = ak_arbuff_new(4);
  AK24_TEST_ASSERT_NOT_NULL(arbuff);

  int values[] = {1, 2, 3, 4, 5};

  AK24_TEST_ASSERT_EQ(ak_arbuff_push(arbuff, &values[0]), 0);
  AK24_TEST_ASSERT_EQ(ak_arbuff_push(arbuff, &values[1]), 0);
  AK24_TEST_ASSERT_EQ(ak_arbuff_push(arbuff, &values[2]), 0);
  AK24_TEST_ASSERT_EQ(ak_arbuff_push(arbuff, &values[3]), 0);

  AK24_TEST_ASSERT_EQ(ak_arbuff_is_full(arbuff), 1);

  int result = ak_arbuff_push(arbuff, &values[4]);
  AK24_TEST_ASSERT_EQ(result, -1);

  AK24_TEST_ASSERT_EQ(ak_arbuff_count(arbuff), 4);

  ak_arbuff_free(arbuff);
  AK24_TEST_PASS();
}

static int test_arbuff_underflow(void) {
  ak_arbuff_t *arbuff = ak_arbuff_new(5);
  AK24_TEST_ASSERT_NOT_NULL(arbuff);

  void *item = ak_arbuff_pop(arbuff);
  AK24_TEST_ASSERT_NULL(item);

  ak_arbuff_free(arbuff);
  AK24_TEST_PASS();
}

static int test_arbuff_peek(void) {
  ak_arbuff_t *arbuff = ak_arbuff_new(5);
  AK24_TEST_ASSERT_NOT_NULL(arbuff);

  int value = 99;
  ak_arbuff_push(arbuff, &value);

  int *peeked = (int *)ak_arbuff_peek(arbuff);
  AK24_TEST_ASSERT_NOT_NULL(peeked);
  AK24_TEST_ASSERT_EQ(*peeked, 99);
  AK24_TEST_ASSERT_EQ(ak_arbuff_count(arbuff), 1);

  int *peeked2 = (int *)ak_arbuff_peek(arbuff);
  AK24_TEST_ASSERT_NOT_NULL(peeked2);
  AK24_TEST_ASSERT_EQ(*peeked2, 99);
  AK24_TEST_ASSERT_EQ(ak_arbuff_count(arbuff), 1);

  ak_arbuff_free(arbuff);
  AK24_TEST_PASS();
}

static int test_arbuff_wrap_around(void) {
  ak_arbuff_t *arbuff = ak_arbuff_new(4);
  AK24_TEST_ASSERT_NOT_NULL(arbuff);

  int values[] = {1, 2, 3, 4, 5};

  ak_arbuff_push(arbuff, &values[0]);
  ak_arbuff_push(arbuff, &values[1]);
  ak_arbuff_push(arbuff, &values[2]);

  int *item1 = (int *)ak_arbuff_pop(arbuff);
  AK24_TEST_ASSERT_EQ(*item1, 1);

  ak_arbuff_push(arbuff, &values[3]);

  int *item2 = (int *)ak_arbuff_pop(arbuff);
  AK24_TEST_ASSERT_EQ(*item2, 2);

  ak_arbuff_push(arbuff, &values[4]);

  AK24_TEST_ASSERT_EQ(ak_arbuff_count(arbuff), 3);

  int *item3 = (int *)ak_arbuff_pop(arbuff);
  int *item4 = (int *)ak_arbuff_pop(arbuff);
  int *item5 = (int *)ak_arbuff_pop(arbuff);

  AK24_TEST_ASSERT_EQ(*item3, 3);
  AK24_TEST_ASSERT_EQ(*item4, 4);
  AK24_TEST_ASSERT_EQ(*item5, 5);

  ak_arbuff_free(arbuff);
  AK24_TEST_PASS();
}

static int test_arbuff_clear(void) {
  ak_arbuff_t *arbuff = ak_arbuff_new(5);
  AK24_TEST_ASSERT_NOT_NULL(arbuff);

  int values[] = {1, 2, 3};
  ak_arbuff_push(arbuff, &values[0]);
  ak_arbuff_push(arbuff, &values[1]);
  ak_arbuff_push(arbuff, &values[2]);

  AK24_TEST_ASSERT_EQ(ak_arbuff_count(arbuff), 3);

  ak_arbuff_clear(arbuff);

  AK24_TEST_ASSERT_EQ(ak_arbuff_count(arbuff), 0);
  AK24_TEST_ASSERT_EQ(ak_arbuff_is_empty(arbuff), 1);

  ak_arbuff_free(arbuff);
  AK24_TEST_PASS();
}

static int test_arbuff_null_handling(void) {
  AK24_TEST_ASSERT_EQ(ak_arbuff_push(NULL, (void *)0x1234), -1);
  AK24_TEST_ASSERT_NULL(ak_arbuff_pop(NULL));
  AK24_TEST_ASSERT_NULL(ak_arbuff_peek(NULL));
  AK24_TEST_ASSERT_EQ(ak_arbuff_count(NULL), 0);
  AK24_TEST_ASSERT_EQ(ak_arbuff_capacity(NULL), 0);
  AK24_TEST_ASSERT_EQ(ak_arbuff_is_empty(NULL), 1);
  AK24_TEST_ASSERT_EQ(ak_arbuff_is_full(NULL), 0);
  ak_arbuff_clear(NULL);
  ak_arbuff_free(NULL);
  AK24_TEST_PASS();
}

typedef struct {
  ak_arbuff_t *arbuff;
  int thread_id;
  int iterations;
  int **values;
} thread_data_t;

static void *producer_thread(void *arg) {
  thread_data_t *data = (thread_data_t *)arg;

  for (int i = 0; i < data->iterations; i++) {
    while (ak_arbuff_push(data->arbuff, data->values[i]) != 0) {
      usleep(1);
    }
  }

  return NULL;
}

static void *consumer_thread(void *arg) {
  thread_data_t *data = (thread_data_t *)arg;
  int consumed = 0;

  while (consumed < data->iterations) {
    void *item = ak_arbuff_pop(data->arbuff);
    if (item) {
      consumed++;
    } else {
      usleep(1);
    }
  }

  return NULL;
}

static int test_arbuff_concurrent_access(void) {
  ak_arbuff_t *arbuff = ak_arbuff_new(100);
  AK24_TEST_ASSERT_NOT_NULL(arbuff);

#define NUM_PRODUCERS 2
#define NUM_CONSUMERS 2
#define ITERATIONS 100

  int **all_values = AK24_ALLOC(sizeof(int *) * NUM_PRODUCERS * ITERATIONS);
  for (int i = 0; i < NUM_PRODUCERS * ITERATIONS; i++) {
    all_values[i] = AK24_ALLOC_ATOMIC(sizeof(int));
    *all_values[i] = i;
  }

  AK24_THREAD producers[NUM_PRODUCERS];
  AK24_THREAD consumers[NUM_CONSUMERS];
  thread_data_t producer_data[NUM_PRODUCERS];
  thread_data_t consumer_data[NUM_CONSUMERS];

  for (int i = 0; i < NUM_PRODUCERS; i++) {
    producer_data[i].arbuff = arbuff;
    producer_data[i].thread_id = i;
    producer_data[i].iterations = ITERATIONS;
    producer_data[i].values = &all_values[i * ITERATIONS];
    AK24_THREAD_CREATE(&producers[i], producer_thread, &producer_data[i]);
  }

  for (int i = 0; i < NUM_CONSUMERS; i++) {
    consumer_data[i].arbuff = arbuff;
    consumer_data[i].thread_id = i;
    consumer_data[i].iterations = ITERATIONS;
    consumer_data[i].values = NULL;
    AK24_THREAD_CREATE(&consumers[i], consumer_thread, &consumer_data[i]);
  }

  for (int i = 0; i < NUM_PRODUCERS; i++) {
    AK24_THREAD_JOIN(producers[i]);
  }

  for (int i = 0; i < NUM_CONSUMERS; i++) {
    AK24_THREAD_JOIN(consumers[i]);
  }

  AK24_TEST_ASSERT_EQ(ak_arbuff_is_empty(arbuff), 1);

#undef NUM_PRODUCERS
#undef NUM_CONSUMERS
#undef ITERATIONS

  ak_arbuff_free(arbuff);
  AK24_TEST_PASS();
}

int run_arbuff_tests(void) {
  AK24_TEST_RUN(test_arbuff_new_zero_capacity);
  AK24_TEST_RUN(test_arbuff_new_valid);
  AK24_TEST_RUN(test_arbuff_push_pop_single);
  AK24_TEST_RUN(test_arbuff_push_pop_multiple);
  AK24_TEST_RUN(test_arbuff_overflow);
  AK24_TEST_RUN(test_arbuff_underflow);
  AK24_TEST_RUN(test_arbuff_peek);
  AK24_TEST_RUN(test_arbuff_wrap_around);
  AK24_TEST_RUN(test_arbuff_clear);
  AK24_TEST_RUN(test_arbuff_null_handling);
  AK24_TEST_RUN(test_arbuff_concurrent_access);
  return 0;
}

int main(void) {
  ak_kernel_init("ak24-test");
  int result = run_arbuff_tests();
  ak_kernel_deinit();
  return result;
}
