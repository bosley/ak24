#include "intern.h"
#include "test/assert.h"
#include <string.h>

// Test basic initialization and shutdown
int test_intern_init_shutdown(void) {
  ak_intern_init();
  ak_intern_shutdown();
  AK24_TEST_PASS();
}

// Test interning a simple string
int test_intern_basic(void) {
  ak_intern_init();

  const char *str = ak_intern("hello");
  AK24_TEST_ASSERT_NOT_NULL(str);
  AK24_TEST_ASSERT_STR_EQ(str, "hello");

  ak_intern_shutdown();
  AK24_TEST_PASS();
}

// Test that same string returns same pointer
int test_intern_deduplication(void) {
  ak_intern_init();

  const char *str1 = ak_intern("test");
  const char *str2 = ak_intern("test");

  AK24_TEST_ASSERT_NOT_NULL(str1);
  AK24_TEST_ASSERT_NOT_NULL(str2);
  AK24_TEST_ASSERT(str1 == str2); // Same pointer!

  ak_intern_shutdown();
  AK24_TEST_PASS();
}

// Test that different strings return different pointers
int test_intern_different_strings(void) {
  ak_intern_init();

  const char *str1 = ak_intern("hello");
  const char *str2 = ak_intern("world");

  AK24_TEST_ASSERT_NOT_NULL(str1);
  AK24_TEST_ASSERT_NOT_NULL(str2);
  AK24_TEST_ASSERT(str1 != str2); // Different pointers

  ak_intern_shutdown();
  AK24_TEST_PASS();
}

// Test equality function
int test_intern_eq(void) {
  ak_intern_init();

  const char *str1 = ak_intern("equal");
  const char *str2 = ak_intern("equal");
  const char *str3 = ak_intern("different");

  AK24_TEST_ASSERT(ak_intern_eq(str1, str2));
  AK24_TEST_ASSERT(!ak_intern_eq(str1, str3));

  ak_intern_shutdown();
  AK24_TEST_PASS();
}

// Test hash function
int test_intern_hash(void) {
  ak_intern_init();

  const char *str1 = ak_intern("test");
  const char *str2 = ak_intern("test");
  const char *str3 = ak_intern("other");

  uint64_t hash1 = ak_intern_hash(str1);
  uint64_t hash2 = ak_intern_hash(str2);
  uint64_t hash3 = ak_intern_hash(str3);

  AK24_TEST_ASSERT_EQ(hash1, hash2); // Same string, same hash
  AK24_TEST_ASSERT(hash1 != hash3);  // Different string, different hash

  ak_intern_shutdown();
  AK24_TEST_PASS();
}

// Test substring interning
int test_intern_n(void) {
  ak_intern_init();

  const char *source = "identifier+123";
  const char *str = ak_intern_n(source, 10);

  AK24_TEST_ASSERT_NOT_NULL(str);
  AK24_TEST_ASSERT_STR_EQ(str, "identifier");

  ak_intern_shutdown();
  AK24_TEST_PASS();
}

// Test substring deduplication
int test_intern_n_deduplication(void) {
  ak_intern_init();

  const char *full = ak_intern("hello");
  const char *source = "hello world";
  const char *sub = ak_intern_n(source, 5);

  AK24_TEST_ASSERT_NOT_NULL(full);
  AK24_TEST_ASSERT_NOT_NULL(sub);
  AK24_TEST_ASSERT(full == sub); // Same pointer!

  ak_intern_shutdown();
  AK24_TEST_PASS();
}

// Test statistics
int test_intern_stats(void) {
  ak_intern_init();

  size_t count = 0, bytes = 0;
  ak_intern_stats(&count, &bytes);
  AK24_TEST_ASSERT_EQ(count, 0);
  AK24_TEST_ASSERT_EQ(bytes, 0);

  ak_intern("hello"); // 5 chars + null = 6 bytes
  ak_intern_stats(&count, &bytes);
  AK24_TEST_ASSERT_EQ(count, 1);
  AK24_TEST_ASSERT_EQ(bytes, 6);

  ak_intern("hello"); // Same string, no new allocation
  ak_intern_stats(&count, &bytes);
  AK24_TEST_ASSERT_EQ(count, 1);
  AK24_TEST_ASSERT_EQ(bytes, 6);

  ak_intern("world"); // 5 chars + null = 6 bytes
  ak_intern_stats(&count, &bytes);
  AK24_TEST_ASSERT_EQ(count, 2);
  AK24_TEST_ASSERT_EQ(bytes, 12);

  ak_intern_shutdown();
  AK24_TEST_PASS();
}

// Test clear operation
int test_intern_clear(void) {
  ak_intern_init();

  ak_intern("test1"); // 6 bytes
  ak_intern("test2"); // 6 bytes

  size_t count = 0, bytes = 0;
  ak_intern_stats(&count, &bytes);
  AK24_TEST_ASSERT_EQ(count, 2);
  AK24_TEST_ASSERT_EQ(bytes, 12);

  ak_intern_clear();

  ak_intern_stats(&count, &bytes);
  AK24_TEST_ASSERT_EQ(count, 0);
  AK24_TEST_ASSERT_EQ(bytes, 0); // Verify memory freed

  // Can still intern after clear
  const char *str = ak_intern("new");
  AK24_TEST_ASSERT_NOT_NULL(str);

  ak_intern_shutdown();
  AK24_TEST_PASS();
}

// Test that substring deduplication doesn't allocate extra memory
int test_intern_n_no_extra_allocation(void) {
  ak_intern_init();

  // Intern full string first
  ak_intern("hello");

  size_t count1 = 0, bytes1 = 0;
  ak_intern_stats(&count1, &bytes1);
  AK24_TEST_ASSERT_EQ(count1, 1);
  AK24_TEST_ASSERT_EQ(bytes1, 6); // 5 chars + null

  // Intern substring - should use same memory
  const char *source = "hello world";
  const char *sub = ak_intern_n(source, 5);
  AK24_TEST_ASSERT_NOT_NULL(sub);

  size_t count2 = 0, bytes2 = 0;
  ak_intern_stats(&count2, &bytes2);
  AK24_TEST_ASSERT_EQ(count2, 1); // No new string
  AK24_TEST_ASSERT_EQ(bytes2, 6); // No new bytes

  ak_intern_shutdown();
  AK24_TEST_PASS();
}

// Test NULL handling
int test_intern_null_handling(void) {
  ak_intern_init();

  const char *str1 = ak_intern(NULL);
  AK24_TEST_ASSERT_NULL(str1);

  const char *str2 = ak_intern_n(NULL, 5);
  AK24_TEST_ASSERT_NULL(str2);

  ak_intern_shutdown();
  AK24_TEST_PASS();
}

// Test empty string
int test_intern_empty_string(void) {
  ak_intern_init();

  const char *str1 = ak_intern("");
  const char *str2 = ak_intern("");

  AK24_TEST_ASSERT_NOT_NULL(str1);
  AK24_TEST_ASSERT_NOT_NULL(str2);
  AK24_TEST_ASSERT(str1 == str2);
  AK24_TEST_ASSERT_STR_EQ(str1, "");

  ak_intern_shutdown();
  AK24_TEST_PASS();
}

// Test many strings (stress test for hash table)
int test_intern_many_strings(void) {
  ak_intern_init();

  const int num_strings = 10000;
  const char **strings = AK24_ALLOC(num_strings * sizeof(char *));

  // Intern many unique strings
  for (int i = 0; i < num_strings; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "string_%d", i);
    strings[i] = ak_intern(buf);
    AK24_TEST_ASSERT_NOT_NULL(strings[i]);
  }

  // Verify deduplication still works
  for (int i = 0; i < num_strings; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "string_%d", i);
    const char *str = ak_intern(buf);
    AK24_TEST_ASSERT(str == strings[i]);
  }

  // Verify exact count and that no duplicates were created
  size_t count = 0, bytes = 0;
  ak_intern_stats(&count, &bytes);
  AK24_TEST_ASSERT_EQ(count, num_strings);

  // Calculate expected bytes:
  // "string_0" through "string_9" = 9 chars each = 10 bytes each
  // "string_10" through "string_99" = 10-11 chars
  // "string_100" through "string_999" = 11-12 chars
  // etc.
  // Just verify we have reasonable amount (should be > count since strings vary
  // in length)
  AK24_TEST_ASSERT(bytes >= count); // At least 1 byte per string (sanity check)
  AK24_TEST_ASSERT(bytes < count * 32); // Less than max buffer size per string

  AK24_FREE(strings);
  ak_intern_shutdown();
  AK24_TEST_PASS();
}

// Test strings with special characters
int test_intern_special_chars(void) {
  ak_intern_init();

  const char *str1 = ak_intern("hello\nworld");
  const char *str2 = ak_intern("tab\there");
  const char *str3 = ak_intern("quote\"test");
  const char *str4 = ak_intern("null\0byte"); // Only up to null

  AK24_TEST_ASSERT_NOT_NULL(str1);
  AK24_TEST_ASSERT_NOT_NULL(str2);
  AK24_TEST_ASSERT_NOT_NULL(str3);
  AK24_TEST_ASSERT_NOT_NULL(str4);

  AK24_TEST_ASSERT_STR_EQ(str4, "null");

  ak_intern_shutdown();
  AK24_TEST_PASS();
}

// Test that shutdown properly cleans up memory
int test_intern_shutdown_cleanup(void) {
  ak_intern_init();

  // Intern several strings
  ak_intern("test1");
  ak_intern("test2");
  ak_intern("test3");

  size_t count = 0, bytes = 0;
  ak_intern_stats(&count, &bytes);
  AK24_TEST_ASSERT_EQ(count, 3);
  AK24_TEST_ASSERT_EQ(bytes, 18); // 6 bytes each

  ak_intern_shutdown();

  // After shutdown, init again should start fresh
  ak_intern_init();

  ak_intern_stats(&count, &bytes);
  AK24_TEST_ASSERT_EQ(count, 0);
  AK24_TEST_ASSERT_EQ(bytes, 0);

  ak_intern_shutdown();
  AK24_TEST_PASS();
}

// Test exact byte accounting with various string lengths
int test_intern_exact_byte_accounting(void) {
  ak_intern_init();

  size_t count = 0, bytes = 0;

  // Empty string: 1 byte (just null terminator)
  ak_intern("");
  ak_intern_stats(&count, &bytes);
  AK24_TEST_ASSERT_EQ(count, 1);
  AK24_TEST_ASSERT_EQ(bytes, 1);

  // 1-char string: 2 bytes
  ak_intern("a");
  ak_intern_stats(&count, &bytes);
  AK24_TEST_ASSERT_EQ(count, 2);
  AK24_TEST_ASSERT_EQ(bytes, 3);

  // 5-char string: 6 bytes
  ak_intern("hello");
  ak_intern_stats(&count, &bytes);
  AK24_TEST_ASSERT_EQ(count, 3);
  AK24_TEST_ASSERT_EQ(bytes, 9);

  // Duplicate - no new bytes
  ak_intern("hello");
  ak_intern_stats(&count, &bytes);
  AK24_TEST_ASSERT_EQ(count, 3);
  AK24_TEST_ASSERT_EQ(bytes, 9);

  // 10-char string: 11 bytes
  ak_intern("0123456789");
  ak_intern_stats(&count, &bytes);
  AK24_TEST_ASSERT_EQ(count, 4);
  AK24_TEST_ASSERT_EQ(bytes, 20);

  ak_intern_shutdown();
  AK24_TEST_PASS();
}

// Test thread safety (basic smoke test)
#ifdef AK24_THREAD_TESTS
void *thread_intern_worker(void *arg) {
  int thread_id = *(int *)arg;

  for (int i = 0; i < 100; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "thread_%d_str_%d", thread_id, i % 10);
    const char *str = ak_intern(buf);
    AK24_TEST_ASSERT_NOT_NULL(str);
  }

  return NULL;
}

int test_intern_thread_safety(void) {
  ak_intern_init();

  const int num_threads = 4;
  AK24_THREAD threads[4];
  int thread_ids[4] = {0, 1, 2, 3};

  for (int i = 0; i < num_threads; i++) {
    AK24_THREAD_CREATE(&threads[i], thread_intern_worker, &thread_ids[i]);
  }

  for (int i = 0; i < num_threads; i++) {
    AK24_THREAD_JOIN(threads[i]);
  }

  // Verify no duplicates were created
  size_t count = 0;
  ak_intern_stats(&count, NULL);
  // Each thread creates 10 unique strings (i % 10)
  // So we should have 40 unique strings total
  AK24_TEST_ASSERT_EQ(count, 40);

  ak_intern_shutdown();
  AK24_TEST_PASS();
}
#endif

int main(void) {
  ak_kernel_init();

  AK24_TEST_RUN(test_intern_init_shutdown);
  AK24_TEST_RUN(test_intern_basic);
  AK24_TEST_RUN(test_intern_deduplication);
  AK24_TEST_RUN(test_intern_different_strings);
  AK24_TEST_RUN(test_intern_eq);
  AK24_TEST_RUN(test_intern_hash);
  AK24_TEST_RUN(test_intern_n);
  AK24_TEST_RUN(test_intern_n_deduplication);
  AK24_TEST_RUN(test_intern_n_no_extra_allocation);
  AK24_TEST_RUN(test_intern_stats);
  AK24_TEST_RUN(test_intern_clear);
  AK24_TEST_RUN(test_intern_null_handling);
  AK24_TEST_RUN(test_intern_empty_string);
  AK24_TEST_RUN(test_intern_many_strings);
  AK24_TEST_RUN(test_intern_special_chars);
  AK24_TEST_RUN(test_intern_shutdown_cleanup);
  AK24_TEST_RUN(test_intern_exact_byte_accounting);

#ifdef AK24_THREAD_TESTS
  AK24_TEST_RUN(test_intern_thread_safety);
#endif

  ak_kernel_deinit();
  return 0;
}
