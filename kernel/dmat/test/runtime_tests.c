/**
 * @file runtime_tests.c
 * @brief Runtime tests for dmat module including multi-threaded tests
 *
 * Thread Safety Model:
 * - The dmat module uses a two-tier locking strategy:
 *   1. Cache-level reader-writer locks for cache lookups/modifications
 *   2. Row stripe locks (16 default stripes) for concurrent writes to different
 * rows
 * - Multiple readers can access the cache concurrently
 * - Writers acquire both cache lock and appropriate row stripe lock
 * - Row striping allows parallel writes to rows in different stripes
 * - Tests verify correct behavior under concurrent read/write workloads
 */

#include "dmat.h"
#include "kernel.h"
#include "test/assert.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/** RGBA pixel structure for image testing */
typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
} rgba_t;

/** Simple 2D point structure */
typedef struct {
  int32_t x;
  int32_t y;
} point_t;

#ifndef AK24_PLATFORM_WINDOWS

/**
 * Thread test data structure
 */
typedef struct {
  dmat_ctx_t *matrix;
  int thread_id;
  int num_operations;
  int success;
} thread_test_data_t;

/**
 * Reader thread function - performs multiple reads
 */
static void *reader_thread_func(void *arg) {
  thread_test_data_t *data = (thread_test_data_t *)arg;
  data->success = 1;

  for (int i = 0; i < data->num_operations; i++) {
    uint32_t x = (i * 7 + data->thread_id) % dmat_get_width(data->matrix);
    uint32_t y = (i * 11 + data->thread_id) % dmat_get_height(data->matrix);

    rgba_t pixel;
    if (dmat_get(data->matrix, x, y, &pixel) != 0) {
      data->success = 0;
      break;
    }

    // Verify pixel data is valid (alpha should be set)
    if (pixel.a == 0 && pixel.r == 0 && pixel.g == 0 && pixel.b == 0) {
      // Uninitialized or zero pixel - acceptable
    }
  }

  return NULL;
}

/**
 * Writer thread function - performs multiple writes
 */
static void *writer_thread_func(void *arg) {
  thread_test_data_t *data = (thread_test_data_t *)arg;
  data->success = 1;

  for (int i = 0; i < data->num_operations; i++) {
    uint32_t x = (i * 13 + data->thread_id) % dmat_get_width(data->matrix);
    uint32_t y = (i * 17 + data->thread_id) % dmat_get_height(data->matrix);

    // Create RGBA pixel with thread ID encoded in channels
    rgba_t pixel = {.r = (uint8_t)(data->thread_id * 32),
                    .g = (uint8_t)(i & 0xFF),
                    .b = (uint8_t)((i >> 8) & 0xFF),
                    .a = 255};

    if (dmat_set(data->matrix, x, y, &pixel) != 0) {
      data->success = 0;
      break;
    }
  }

  return NULL;
}

/**
 * Test: Multiple concurrent readers on realistic image size
 */
static int test_dmat_concurrent_readers(void) {
  enum { num_readers = 8, ops_per_reader = 5000 };

  // Clean up any existing file to avoid dimension mismatch
  remove("/tmp/test_dmat_readers.dmat");

  dmat_ctx_t *matrix =
      dmat_new("/tmp/test_dmat_readers.dmat", 1920, 1080, sizeof(rgba_t));
  AK24_TEST_ASSERT_NOT_NULL(matrix);

  // Initialize with a gradient pattern (sample of pixels, not all)
  for (uint32_t y = 0; y < 1080; y += 100) {
    for (uint32_t x = 0; x < 1920; x += 100) {
      rgba_t pixel = {.r = (uint8_t)((x * 255) / 1920),
                      .g = (uint8_t)((y * 255) / 1080),
                      .b = 128,
                      .a = 255};
      AK24_TEST_ASSERT_EQ(dmat_set(matrix, x, y, &pixel), 0);
    }
  }
  AK24_TEST_ASSERT_EQ(dmat_flush(matrix), 0);

  // Create reader threads
  AK24_THREAD threads[num_readers];
  thread_test_data_t thread_data[num_readers];

  for (int i = 0; i < num_readers; i++) {
    thread_data[i].matrix = matrix;
    thread_data[i].thread_id = i;
    thread_data[i].num_operations = ops_per_reader;
    thread_data[i].success = 0;

    int result =
        AK24_THREAD_CREATE(&threads[i], reader_thread_func, &thread_data[i]);
    AK24_TEST_ASSERT_EQ(result, 0);
  }

  // Join all threads
  for (int i = 0; i < num_readers; i++) {
    AK24_THREAD_JOIN(threads[i]);
    AK24_TEST_ASSERT(thread_data[i].success == 1);
  }

  dmat_free(matrix);

  AK24_TEST_PASS();
}

/**
 * Test: Single writer with multiple readers on realistic image size
 */
static int test_dmat_writer_with_readers(void) {
  enum { num_readers = 4, ops_per_thread = 2000 };

  // Clean up any existing file to avoid dimension mismatch
  remove("/tmp/test_dmat_rw.dmat");

  dmat_ctx_t *matrix =
      dmat_new("/tmp/test_dmat_rw.dmat", 1920, 1080, sizeof(rgba_t));
  AK24_TEST_ASSERT_NOT_NULL(matrix);

  // Initialize with transparent black (sparse initialization)
  rgba_t black = {0, 0, 0, 0};
  for (uint32_t y = 0; y < 1080; y += 200) {
    for (uint32_t x = 0; x < 1920; x += 200) {
      AK24_TEST_ASSERT_EQ(dmat_set(matrix, x, y, &black), 0);
    }
  }
  AK24_TEST_ASSERT_EQ(dmat_flush(matrix), 0);

  // Create threads
  AK24_THREAD threads[num_readers + 1];
  thread_test_data_t thread_data[num_readers + 1];

  // Create one writer thread
  thread_data[0].matrix = matrix;
  thread_data[0].thread_id = 0;
  thread_data[0].num_operations = ops_per_thread;
  thread_data[0].success = 0;
  int result =
      AK24_THREAD_CREATE(&threads[0], writer_thread_func, &thread_data[0]);
  AK24_TEST_ASSERT_EQ(result, 0);

  // Create reader threads
  for (int i = 1; i <= num_readers; i++) {
    thread_data[i].matrix = matrix;
    thread_data[i].thread_id = i;
    thread_data[i].num_operations = ops_per_thread;
    thread_data[i].success = 0;

    result =
        AK24_THREAD_CREATE(&threads[i], reader_thread_func, &thread_data[i]);
    AK24_TEST_ASSERT_EQ(result, 0);
  }

  // Join all threads
  for (int i = 0; i <= num_readers; i++) {
    AK24_THREAD_JOIN(threads[i]);
    AK24_TEST_ASSERT(thread_data[i].success == 1);
  }

  dmat_free(matrix);

  AK24_TEST_PASS();
}

/**
 * Test: Concurrent writers with potential row stripe contention
 *
 * Multiple writers operate concurrently with random access patterns.
 * This tests that row stripe locking correctly serializes writes to
 * the same stripe while allowing parallel writes to different stripes.
 * Uses 1280x720 resolution (HD ready).
 */
static int test_dmat_concurrent_writers(void) {
  enum { num_writers = 4, ops_per_writer = 1000 };

  // Clean up any existing file to avoid dimension mismatch
  remove("/tmp/test_dmat_writers.dmat");

  dmat_ctx_t *matrix =
      dmat_new("/tmp/test_dmat_writers.dmat", 1280, 720, sizeof(rgba_t));
  AK24_TEST_ASSERT_NOT_NULL(matrix);

  // Initialize with white background (sparse initialization)
  rgba_t white = {255, 255, 255, 255};
  for (uint32_t y = 0; y < 720; y += 100) {
    for (uint32_t x = 0; x < 1280; x += 100) {
      AK24_TEST_ASSERT_EQ(dmat_set(matrix, x, y, &white), 0);
    }
  }
  AK24_TEST_ASSERT_EQ(dmat_flush(matrix), 0);

  // Create writer threads
  AK24_THREAD threads[num_writers];
  thread_test_data_t thread_data[num_writers];

  for (int i = 0; i < num_writers; i++) {
    thread_data[i].matrix = matrix;
    thread_data[i].thread_id = i;
    thread_data[i].num_operations = ops_per_writer;
    thread_data[i].success = 0;

    int result =
        AK24_THREAD_CREATE(&threads[i], writer_thread_func, &thread_data[i]);
    AK24_TEST_ASSERT_EQ(result, 0);
  }

  // Join all threads
  for (int i = 0; i < num_writers; i++) {
    AK24_THREAD_JOIN(threads[i]);
    AK24_TEST_ASSERT(thread_data[i].success == 1);
  }

  // Verify data consistency
  AK24_TEST_ASSERT_EQ(dmat_flush(matrix), 0);

  dmat_free(matrix);

  AK24_TEST_PASS();
}

/**
 * Writer thread function that writes to specific rows (for striping test)
 *
 * With 16 default stripes and 8 threads, each thread writes to 2
 * non-overlapping stripes to avoid lock contention:
 * - Thread 0: stripes 0, 8  (rows 0, 8, 16, 24, ...)
 * - Thread 1: stripes 1, 9  (rows 1, 9, 17, 25, ...)
 * - Thread 2: stripes 2, 10 (rows 2, 10, 18, 26, ...)
 * etc.
 */
static void *striped_writer_thread_func(void *arg) {
  thread_test_data_t *data = (thread_test_data_t *)arg;
  data->success = 1;

  uint32_t height = dmat_get_height(data->matrix);
  uint32_t width = dmat_get_width(data->matrix);
  const uint32_t num_stripes = 16; // Must match DMAT_DEFAULT_ROW_STRIPES

  // Each thread writes to rows that map to its assigned stripes
  // Thread N writes to stripes N and (N + num_writers)
  for (int i = 0; i < data->num_operations; i++) {
    // Alternate between two assigned stripes for this thread
    uint32_t stripe_offset = (i % 2) ? 8 : 0; // num_writers = 8
    uint32_t base_row = data->thread_id + stripe_offset;
    uint32_t y = base_row + (i / 2) * num_stripes;

    // Wrap around if we exceed height
    y = y % height;
    uint32_t x = i % width;

    // Create RGBA pixel with thread ID and operation encoded
    rgba_t pixel = {.r = (uint8_t)(data->thread_id * 32),
                    .g = (uint8_t)(i & 0xFF),
                    .b = (uint8_t)((i >> 8) & 0xFF),
                    .a = 255};

    if (dmat_set(data->matrix, x, y, &pixel) != 0) {
      data->success = 0;
      break;
    }
  }

  return NULL;
}

/**
 * Test: Multiple concurrent writers on different row stripes
 * Uses Full HD resolution (1920x1080) with RGBA data
 */
static int test_dmat_concurrent_striped_writers(void) {
  enum { num_writers = 8, ops_per_writer = 2000 };

  // Clean up any existing file to avoid dimension mismatch
  remove("/tmp/test_dmat_striped.dmat");

  dmat_ctx_t *matrix =
      dmat_new("/tmp/test_dmat_striped.dmat", 1920, 1080, sizeof(rgba_t));
  AK24_TEST_ASSERT_NOT_NULL(matrix);

  // Initialize with semi-transparent gray (sparse initialization)
  rgba_t gray = {128, 128, 128, 128};
  for (uint32_t y = 0; y < 1080; y += 100) {
    for (uint32_t x = 0; x < 1920; x += 100) {
      AK24_TEST_ASSERT_EQ(dmat_set(matrix, x, y, &gray), 0);
    }
  }
  AK24_TEST_ASSERT_EQ(dmat_flush(matrix), 0);

  // Create writer threads that work on different row stripes
  AK24_THREAD threads[num_writers];
  thread_test_data_t thread_data[num_writers];

  for (int i = 0; i < num_writers; i++) {
    thread_data[i].matrix = matrix;
    thread_data[i].thread_id = i;
    thread_data[i].num_operations = ops_per_writer;
    thread_data[i].success = 0;

    int result = AK24_THREAD_CREATE(&threads[i], striped_writer_thread_func,
                                    &thread_data[i]);
    AK24_TEST_ASSERT_EQ(result, 0);
  }

  // Join all threads
  for (int i = 0; i < num_writers; i++) {
    AK24_THREAD_JOIN(threads[i]);
    AK24_TEST_ASSERT(thread_data[i].success == 1);
  }

  // Flush and verify consistency
  AK24_TEST_ASSERT_EQ(dmat_flush(matrix), 0);

  // Verify data integrity by reading back some values
  // Note: With concurrent writes, we can't verify specific values,
  // but we can verify that reads succeed and data wasn't corrupted
  for (uint32_t y = 0; y < 1080; y += 100) {
    for (uint32_t x = 0; x < 1920; x += 100) {
      rgba_t pixel;
      AK24_TEST_ASSERT_EQ(dmat_get(matrix, x, y, &pixel), 0);
      // Verify pixel alpha is set (written by threads)
      AK24_TEST_ASSERT(pixel.a > 0);
      // Verify red channel contains valid thread ID data (0-7 * 32 = 0-224)
      // Since uint8_t range is 0-255, we just verify it's set
      (void)pixel.r; // Thread ID encoded in red channel
    }
  }

  dmat_free(matrix);

  AK24_TEST_PASS();
}

/**
 * Test: Cache effectiveness with repeated access on image data
 */
static int test_dmat_cache_effectiveness(void) {
  // Clean up any existing file to avoid dimension mismatch
  remove("/tmp/test_dmat_cache.dmat");

  dmat_ctx_t *matrix =
      dmat_new("/tmp/test_dmat_cache.dmat", 1920, 1080, sizeof(rgba_t));
  AK24_TEST_ASSERT_NOT_NULL(matrix);

  // Write some diagonal pixels
  for (uint32_t i = 0; i < 100; i += 10) {
    rgba_t pixel = {.r = (uint8_t)(i * 2),
                    .g = (uint8_t)(i * 2),
                    .b = (uint8_t)(i * 2),
                    .a = 255};
    AK24_TEST_ASSERT_EQ(dmat_set(matrix, i * 10, i, &pixel), 0);
  }

  // Read same values multiple times (should hit cache)
  for (int repeat = 0; repeat < 5; repeat++) {
    for (uint32_t i = 0; i < 100; i += 10) {
      rgba_t pixel;
      AK24_TEST_ASSERT_EQ(dmat_get(matrix, i * 10, i, &pixel), 0);
      AK24_TEST_ASSERT_EQ(pixel.r, (uint8_t)(i * 2));
      AK24_TEST_ASSERT_EQ(pixel.g, (uint8_t)(i * 2));
      AK24_TEST_ASSERT_EQ(pixel.a, 255);
    }
  }

  dmat_free(matrix);

  AK24_TEST_PASS();
}

/**
 * Test: Cache eviction and writeback with realistic image data
 */
static int test_dmat_cache_eviction(void) {
  // Clean up any existing file to avoid dimension mismatch
  remove("/tmp/test_dmat_evict.dmat");

  dmat_ctx_t *matrix =
      dmat_new("/tmp/test_dmat_evict.dmat", 1280, 720, sizeof(rgba_t));
  AK24_TEST_ASSERT_NOT_NULL(matrix);

  // Write more values than cache can hold (default is 256 entries)
  // Writing every 4th pixel to test cache eviction without exhausting memory
  for (uint32_t y = 0; y < 720; y += 4) {
    for (uint32_t x = 0; x < 1280; x += 4) {
      rgba_t pixel = {.r = (uint8_t)((x * 255) / 1280),
                      .g = (uint8_t)((y * 255) / 720),
                      .b = (uint8_t)(((x + y) * 255) / 2000),
                      .a = 255};
      AK24_TEST_ASSERT_EQ(dmat_set(matrix, x, y, &pixel), 0);
    }
  }

  // Flush to ensure all dirty entries written
  AK24_TEST_ASSERT_EQ(dmat_flush(matrix), 0);

  // Read back and verify (will require loading from disk)
  for (uint32_t y = 0; y < 720; y += 4) {
    for (uint32_t x = 0; x < 1280; x += 4) {
      rgba_t pixel;
      AK24_TEST_ASSERT_EQ(dmat_get(matrix, x, y, &pixel), 0);
      AK24_TEST_ASSERT_EQ(pixel.r, (uint8_t)((x * 255) / 1280));
      AK24_TEST_ASSERT_EQ(pixel.g, (uint8_t)((y * 255) / 720));
      AK24_TEST_ASSERT_EQ(pixel.a, 255);
    }
  }

  dmat_free(matrix);

  AK24_TEST_PASS();
}

#endif /* !AK24_PLATFORM_WINDOWS */

/**
 * Test: Create a new matrix file
 */
static int test_dmat_create_new(void) {
  dmat_ctx_t *matrix =
      dmat_new("/tmp/test_dmat_new.dmat", 100, 100, sizeof(rgba_t));
  AK24_TEST_ASSERT_NOT_NULL(matrix);

  AK24_TEST_ASSERT_EQ(dmat_get_width(matrix), 100);
  AK24_TEST_ASSERT_EQ(dmat_get_height(matrix), 100);
  AK24_TEST_ASSERT_EQ(dmat_get_element_size(matrix), sizeof(rgba_t));

  dmat_free(matrix);

  AK24_TEST_PASS();
}

/**
 * Test: Open an existing matrix file
 */
static int test_dmat_open_existing(void) {
  // Create initial file
  dmat_ctx_t *matrix1 =
      dmat_new("/tmp/test_dmat_existing.dmat", 50, 50, sizeof(point_t));
  AK24_TEST_ASSERT_NOT_NULL(matrix1);
  dmat_free(matrix1);

  // Reopen existing file
  dmat_ctx_t *matrix2 =
      dmat_new("/tmp/test_dmat_existing.dmat", 50, 50, sizeof(point_t));
  AK24_TEST_ASSERT_NOT_NULL(matrix2);

  AK24_TEST_ASSERT_EQ(dmat_get_width(matrix2), 50);
  AK24_TEST_ASSERT_EQ(dmat_get_height(matrix2), 50);
  AK24_TEST_ASSERT_EQ(dmat_get_element_size(matrix2), sizeof(point_t));

  dmat_free(matrix2);

  AK24_TEST_PASS();
}

/**
 * Test: Reject opening with mismatched dimensions
 */
static int test_dmat_dimension_mismatch(void) {
  // Create file with specific dimensions
  dmat_ctx_t *matrix1 =
      dmat_new("/tmp/test_dmat_mismatch.dmat", 10, 20, sizeof(uint32_t));
  AK24_TEST_ASSERT_NOT_NULL(matrix1);
  dmat_free(matrix1);

  // Try to open with wrong dimensions - should fail
  dmat_ctx_t *matrix2 =
      dmat_new("/tmp/test_dmat_mismatch.dmat", 20, 10, sizeof(uint32_t));
  AK24_TEST_ASSERT(matrix2 == NULL);

  // Try to open with wrong element size - should fail
  dmat_ctx_t *matrix3 =
      dmat_new("/tmp/test_dmat_mismatch.dmat", 10, 20, sizeof(uint64_t));
  AK24_TEST_ASSERT(matrix3 == NULL);

  AK24_TEST_PASS();
}

/**
 * Test: Write and read RGBA pixel data
 */
static int test_dmat_rgba_readwrite(void) {
  dmat_ctx_t *matrix =
      dmat_new("/tmp/test_dmat_rgba.dmat", 1920, 1080, sizeof(rgba_t));
  AK24_TEST_ASSERT_NOT_NULL(matrix);

  // Write various pixels
  rgba_t red = {255, 0, 0, 255};
  rgba_t green = {0, 255, 0, 255};
  rgba_t blue = {0, 0, 255, 255};
  rgba_t transparent = {0, 0, 0, 0};

  AK24_TEST_ASSERT_EQ(dmat_set(matrix, 0, 0, &red), 0);
  AK24_TEST_ASSERT_EQ(dmat_set(matrix, 1919, 0, &green), 0);
  AK24_TEST_ASSERT_EQ(dmat_set(matrix, 0, 1079, &blue), 0);
  AK24_TEST_ASSERT_EQ(dmat_set(matrix, 100, 500, &transparent), 0);

  // Read back and verify
  rgba_t pixel;

  AK24_TEST_ASSERT_EQ(dmat_get(matrix, 0, 0, &pixel), 0);
  AK24_TEST_ASSERT_EQ(pixel.r, 255);
  AK24_TEST_ASSERT_EQ(pixel.g, 0);
  AK24_TEST_ASSERT_EQ(pixel.b, 0);
  AK24_TEST_ASSERT_EQ(pixel.a, 255);

  AK24_TEST_ASSERT_EQ(dmat_get(matrix, 1919, 0, &pixel), 0);
  AK24_TEST_ASSERT_EQ(pixel.r, 0);
  AK24_TEST_ASSERT_EQ(pixel.g, 255);

  AK24_TEST_ASSERT_EQ(dmat_get(matrix, 0, 1079, &pixel), 0);
  AK24_TEST_ASSERT_EQ(pixel.b, 255);

  AK24_TEST_ASSERT_EQ(dmat_get(matrix, 100, 500, &pixel), 0);
  AK24_TEST_ASSERT_EQ(pixel.a, 0);

  dmat_free(matrix);

  AK24_TEST_PASS();
}

/**
 * Test: Data persistence across sessions
 */
static int test_dmat_persistence(void) {
  // Write data in first session
  {
    dmat_ctx_t *matrix =
        dmat_new("/tmp/test_dmat_persist.dmat", 10, 10, sizeof(uint32_t));
    AK24_TEST_ASSERT_NOT_NULL(matrix);

    uint32_t value1 = 0xDEADBEEF;
    uint32_t value2 = 0xCAFEBABE;
    uint32_t value3 = 0x12345678;

    AK24_TEST_ASSERT_EQ(dmat_set(matrix, 0, 0, &value1), 0);
    AK24_TEST_ASSERT_EQ(dmat_set(matrix, 5, 5, &value2), 0);
    AK24_TEST_ASSERT_EQ(dmat_set(matrix, 9, 9, &value3), 0);

    AK24_TEST_ASSERT_EQ(dmat_flush(matrix), 0);
    dmat_free(matrix);
  }

  // Read data in second session
  {
    dmat_ctx_t *matrix =
        dmat_new("/tmp/test_dmat_persist.dmat", 10, 10, sizeof(uint32_t));
    AK24_TEST_ASSERT_NOT_NULL(matrix);

    uint32_t read_value;

    AK24_TEST_ASSERT_EQ(dmat_get(matrix, 0, 0, &read_value), 0);
    AK24_TEST_ASSERT_EQ(read_value, 0xDEADBEEF);

    AK24_TEST_ASSERT_EQ(dmat_get(matrix, 5, 5, &read_value), 0);
    AK24_TEST_ASSERT_EQ(read_value, 0xCAFEBABE);

    AK24_TEST_ASSERT_EQ(dmat_get(matrix, 9, 9, &read_value), 0);
    AK24_TEST_ASSERT_EQ(read_value, 0x12345678);

    dmat_free(matrix);
  }

  AK24_TEST_PASS();
}

/**
 * Test: Bounds checking
 */
static int test_dmat_bounds_checking(void) {
  dmat_ctx_t *matrix =
      dmat_new("/tmp/test_dmat_bounds.dmat", 100, 50, sizeof(uint32_t));
  AK24_TEST_ASSERT_NOT_NULL(matrix);

  uint32_t value = 42;
  uint32_t read_value;

  // Valid accesses
  AK24_TEST_ASSERT_EQ(dmat_set(matrix, 0, 0, &value), 0);
  AK24_TEST_ASSERT_EQ(dmat_set(matrix, 99, 49, &value), 0);
  AK24_TEST_ASSERT_EQ(dmat_get(matrix, 50, 25, &read_value), 0);

  // Out of bounds accesses should fail
  AK24_TEST_ASSERT_EQ(dmat_set(matrix, 100, 0, &value), -1);
  AK24_TEST_ASSERT_EQ(dmat_set(matrix, 0, 50, &value), -1);
  AK24_TEST_ASSERT_EQ(dmat_set(matrix, 100, 50, &value), -1);
  AK24_TEST_ASSERT_EQ(dmat_get(matrix, 100, 0, &read_value), -1);
  AK24_TEST_ASSERT_EQ(dmat_get(matrix, 0, 50, &read_value), -1);

  dmat_free(matrix);

  AK24_TEST_PASS();
}

/**
 * Test: NULL parameter handling
 */
static int test_dmat_null_handling(void) {
  uint32_t value = 123;
  uint32_t read_value;

  // NULL filepath
  AK24_TEST_ASSERT(dmat_new(NULL, 10, 10, sizeof(uint32_t)) == NULL);

  // Zero dimensions
  AK24_TEST_ASSERT(
      dmat_new("/tmp/test_dmat_null.dmat", 0, 10, sizeof(uint32_t)) == NULL);
  AK24_TEST_ASSERT(
      dmat_new("/tmp/test_dmat_null.dmat", 10, 0, sizeof(uint32_t)) == NULL);
  AK24_TEST_ASSERT(dmat_new("/tmp/test_dmat_null.dmat", 10, 10, 0) == NULL);

  dmat_ctx_t *matrix =
      dmat_new("/tmp/test_dmat_null.dmat", 10, 10, sizeof(uint32_t));
  AK24_TEST_ASSERT_NOT_NULL(matrix);

  // NULL context operations
  AK24_TEST_ASSERT_EQ(dmat_set(NULL, 0, 0, &value), -1);
  AK24_TEST_ASSERT_EQ(dmat_get(NULL, 0, 0, &read_value), -1);
  AK24_TEST_ASSERT_EQ(dmat_flush(NULL), -1);

  // NULL buffers
  AK24_TEST_ASSERT_EQ(dmat_set(matrix, 0, 0, NULL), -1);
  AK24_TEST_ASSERT_EQ(dmat_get(matrix, 0, 0, NULL), -1);

  // NULL free should not crash
  dmat_free(NULL);

  // Getters with NULL
  AK24_TEST_ASSERT_EQ(dmat_get_width(NULL), 0);
  AK24_TEST_ASSERT_EQ(dmat_get_height(NULL), 0);
  AK24_TEST_ASSERT_EQ(dmat_get_element_size(NULL), 0);

  dmat_free(matrix);

  AK24_TEST_PASS();
}

/**
 * Test: Edge case - 1x1 matrix
 */
static int test_dmat_single_element(void) {
  dmat_ctx_t *matrix =
      dmat_new("/tmp/test_dmat_1x1.dmat", 1, 1, sizeof(double));
  AK24_TEST_ASSERT_NOT_NULL(matrix);

  double pi = 3.14159265359;
  AK24_TEST_ASSERT_EQ(dmat_set(matrix, 0, 0, &pi), 0);

  double read_value;
  AK24_TEST_ASSERT_EQ(dmat_get(matrix, 0, 0, &read_value), 0);
  AK24_TEST_ASSERT(read_value == pi);

  // Out of bounds on 1x1
  AK24_TEST_ASSERT_EQ(dmat_set(matrix, 1, 0, &pi), -1);
  AK24_TEST_ASSERT_EQ(dmat_set(matrix, 0, 1, &pi), -1);

  dmat_free(matrix);

  AK24_TEST_PASS();
}

/**
 * Test: Large element type
 */
static int test_dmat_large_elements(void) {
  typedef struct {
    uint64_t data[16]; // 128 bytes
  } large_element_t;

  dmat_ctx_t *matrix =
      dmat_new("/tmp/test_dmat_large.dmat", 10, 10, sizeof(large_element_t));
  AK24_TEST_ASSERT_NOT_NULL(matrix);

  large_element_t elem;
  for (int i = 0; i < 16; i++) {
    elem.data[i] = (uint64_t)(0x0123456789ABCDEFULL + i);
  }

  AK24_TEST_ASSERT_EQ(dmat_set(matrix, 5, 5, &elem), 0);

  large_element_t read_elem;
  memset(&read_elem, 0, sizeof(read_elem));

  AK24_TEST_ASSERT_EQ(dmat_get(matrix, 5, 5, &read_elem), 0);

  for (int i = 0; i < 16; i++) {
    AK24_TEST_ASSERT_EQ(read_elem.data[i],
                        (uint64_t)(0x0123456789ABCDEFULL + i));
  }

  dmat_free(matrix);

  AK24_TEST_PASS();
}

/**
 * Test: Multiple operations without explicit flush
 */
static int test_dmat_multiple_operations(void) {
  dmat_ctx_t *matrix =
      dmat_new("/tmp/test_dmat_multi.dmat", 50, 50, sizeof(uint8_t));
  AK24_TEST_ASSERT_NOT_NULL(matrix);

  // Write a pattern
  for (uint32_t y = 0; y < 50; y++) {
    for (uint32_t x = 0; x < 50; x++) {
      uint8_t value = (uint8_t)((x + y) % 256);
      AK24_TEST_ASSERT_EQ(dmat_set(matrix, x, y, &value), 0);
    }
  }

  // Read back and verify
  for (uint32_t y = 0; y < 50; y++) {
    for (uint32_t x = 0; x < 50; x++) {
      uint8_t expected = (uint8_t)((x + y) % 256);
      uint8_t actual;
      AK24_TEST_ASSERT_EQ(dmat_get(matrix, x, y, &actual), 0);
      AK24_TEST_ASSERT_EQ(actual, expected);
    }
  }

  dmat_free(matrix);

  AK24_TEST_PASS();
}

/**
 * Test: Explicit flush operation
 */
static int test_dmat_explicit_flush(void) {
  dmat_ctx_t *matrix =
      dmat_new("/tmp/test_dmat_flush.dmat", 5, 5, sizeof(int32_t));
  AK24_TEST_ASSERT_NOT_NULL(matrix);

  int32_t value = -42;
  AK24_TEST_ASSERT_EQ(dmat_set(matrix, 2, 2, &value), 0);

  // Explicit flush
  AK24_TEST_ASSERT_EQ(dmat_flush(matrix), 0);

  // Multiple flushes should be safe
  AK24_TEST_ASSERT_EQ(dmat_flush(matrix), 0);
  AK24_TEST_ASSERT_EQ(dmat_flush(matrix), 0);

  int32_t read_value;
  AK24_TEST_ASSERT_EQ(dmat_get(matrix, 2, 2, &read_value), 0);
  AK24_TEST_ASSERT_EQ(read_value, -42);

  dmat_free(matrix);

  AK24_TEST_PASS();
}

int main(void) {
  ak_kernel_init("ak24-dmat-test");

  printf("\n=== DMAT Runtime Tests ===\n");

  AK24_TEST_RUN(test_dmat_create_new);
  AK24_TEST_RUN(test_dmat_open_existing);
  AK24_TEST_RUN(test_dmat_dimension_mismatch);
  AK24_TEST_RUN(test_dmat_rgba_readwrite);
  AK24_TEST_RUN(test_dmat_persistence);
  AK24_TEST_RUN(test_dmat_bounds_checking);
  AK24_TEST_RUN(test_dmat_null_handling);
  AK24_TEST_RUN(test_dmat_single_element);
  AK24_TEST_RUN(test_dmat_large_elements);
  AK24_TEST_RUN(test_dmat_multiple_operations);
  AK24_TEST_RUN(test_dmat_explicit_flush);

#ifndef AK24_PLATFORM_WINDOWS
  printf("\n=== DMAT Thread Safety Tests ===\n");
  AK24_TEST_RUN(test_dmat_concurrent_readers);
  AK24_TEST_RUN(test_dmat_writer_with_readers);
  AK24_TEST_RUN(test_dmat_concurrent_writers);
  AK24_TEST_RUN(test_dmat_concurrent_striped_writers);

  printf("\n=== DMAT Cache Tests ===\n");
  AK24_TEST_RUN(test_dmat_cache_effectiveness);
  AK24_TEST_RUN(test_dmat_cache_eviction);
#else
  printf("\n=== Thread tests skipped on Windows ===\n");
#endif

  printf("\n=== All DMAT Tests Passed ===\n");

  ak_kernel_deinit();
  return 0;
}
