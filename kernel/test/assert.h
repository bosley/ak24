#ifndef AK24_TEST_ASSERT_H
#define AK24_TEST_ASSERT_H

#include "kernel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static AK_MUTEX ak24_test_mutex = AK_MUTEX_INITIALIZER;

#define AK24_TEST_ASSERT(expr)                                                 \
  do {                                                                         \
    if (!(expr)) {                                                             \
      fprintf(stderr, "[FAIL] %s:%d: Assertion failed: %s\n", __FILE__,        \
              __LINE__, #expr);                                                \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define AK24_TEST_ASSERT_EQ(a, b)                                              \
  do {                                                                         \
    if ((a) != (b)) {                                                          \
      fprintf(stderr, "[FAIL] %s:%d: Expected %s == %s\n", __FILE__, __LINE__, \
              #a, #b);                                                         \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define AK24_TEST_ASSERT_NEQ(a, b)                                             \
  do {                                                                         \
    if ((a) == (b)) {                                                          \
      fprintf(stderr, "[FAIL] %s:%d: Expected %s != %s\n", __FILE__, __LINE__, \
              #a, #b);                                                         \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define AK24_TEST_ASSERT_NULL(ptr)                                             \
  do {                                                                         \
    if ((ptr) != NULL) {                                                       \
      fprintf(stderr, "[FAIL] %s:%d: Expected %s to be NULL\n", __FILE__,      \
              __LINE__, #ptr);                                                 \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define AK24_TEST_ASSERT_NOT_NULL(ptr)                                         \
  do {                                                                         \
    if ((ptr) == NULL) {                                                       \
      fprintf(stderr, "[FAIL] %s:%d: Expected %s to not be NULL\n", __FILE__,  \
              __LINE__, #ptr);                                                 \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define AK24_TEST_ASSERT_STR_EQ(a, b)                                          \
  do {                                                                         \
    if (strcmp((a), (b)) != 0) {                                               \
      fprintf(stderr, "[FAIL] %s:%d: Expected '%s' == '%s'\n", __FILE__,       \
              __LINE__, (a), (b));                                             \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define AK24_TEST_ASSERT_STR_NEQ(a, b)                                         \
  do {                                                                         \
    if (strcmp((a), (b)) == 0) {                                               \
      fprintf(stderr, "[FAIL] %s:%d: Expected '%s' != '%s'\n", __FILE__,       \
              __LINE__, (a), (b));                                             \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define AK24_TEST_PASS()                                                       \
  do {                                                                         \
    fprintf(stdout, "[PASS] %s\n", __func__);                                  \
    return 0;                                                                  \
  } while (0)

#define AK24_TEST_RUN(test_fn)                                                 \
  do {                                                                         \
    fprintf(stdout, "[RUN ] %s\n", #test_fn);                                  \
    int result = test_fn();                                                    \
    if (result != 0) {                                                         \
      fprintf(stderr, "[FAIL] %s returned %d\n", #test_fn, result);            \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define AK24_TEST_ASSERT_ATOMIC(expr)                                          \
  do {                                                                         \
    if (!(expr)) {                                                             \
      AK_MUTEX_LOCK(&ak24_test_mutex);                                         \
      fprintf(stderr, "[FAIL] %s:%d: Assertion failed: %s\n", __FILE__,        \
              __LINE__, #expr);                                                \
      AK_MUTEX_UNLOCK(&ak24_test_mutex);                                       \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define AK24_TEST_ASSERT_EQ_ATOMIC(a, b)                                       \
  do {                                                                         \
    if ((a) != (b)) {                                                          \
      AK_MUTEX_LOCK(&ak24_test_mutex);                                         \
      fprintf(stderr, "[FAIL] %s:%d: Expected %s == %s\n", __FILE__, __LINE__, \
              #a, #b);                                                         \
      AK_MUTEX_UNLOCK(&ak24_test_mutex);                                       \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#endif
