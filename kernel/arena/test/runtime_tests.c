#include "arena.h"
#include "test/assert.h"
#include <string.h>

// Test basic initialization and cleanup
int test_arena_new_free(void) {
  ak_arena_t *arena = ak_arena_new(4096);
  AK24_TEST_ASSERT_NOT_NULL(arena);

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test default arena creation
int test_arena_new_default(void) {
  ak_arena_t *arena = ak_arena_new_default();
  AK24_TEST_ASSERT_NOT_NULL(arena);

  size_t allocated, used;
  ak_arena_stats(arena, &allocated, &used);
  AK24_TEST_ASSERT_EQ(allocated, AK_ARENA_DEFAULT_BLOCK_SIZE);
  AK24_TEST_ASSERT_EQ(used, 0);

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test basic allocation
int test_arena_alloc_basic(void) {
  ak_arena_t *arena = ak_arena_new(4096);

  void *ptr = ak_arena_alloc(arena, 100);
  AK24_TEST_ASSERT_NOT_NULL(ptr);

  size_t allocated, used;
  ak_arena_stats(arena, &allocated, &used);
  AK24_TEST_ASSERT_EQ(allocated, 4096);
  AK24_TEST_ASSERT(used >= 100 && used <= 100 + AK_ARENA_DEFAULT_ALIGNMENT);

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test multiple allocations in same block
int test_arena_multiple_allocs(void) {
  ak_arena_t *arena = ak_arena_new(4096);

  void *ptr1 = ak_arena_alloc(arena, 100);
  void *ptr2 = ak_arena_alloc(arena, 200);
  void *ptr3 = ak_arena_alloc(arena, 300);

  AK24_TEST_ASSERT_NOT_NULL(ptr1);
  AK24_TEST_ASSERT_NOT_NULL(ptr2);
  AK24_TEST_ASSERT_NOT_NULL(ptr3);

  // Pointers should be different
  AK24_TEST_ASSERT(ptr1 != ptr2);
  AK24_TEST_ASSERT(ptr2 != ptr3);
  AK24_TEST_ASSERT(ptr1 != ptr3);

  size_t allocated, used;
  ak_arena_stats(arena, &allocated, &used);
  AK24_TEST_ASSERT_EQ(allocated, 4096); // Still one block
  AK24_TEST_ASSERT(used >= 600);        // At least 600 bytes used

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test allocation larger than block size
int test_arena_large_alloc(void) {
  ak_arena_t *arena = ak_arena_new(1024);

  void *ptr = ak_arena_alloc(arena, 2048); // Larger than block size
  AK24_TEST_ASSERT_NOT_NULL(ptr);

  size_t allocated, used;
  ak_arena_stats(arena, &allocated, &used);
  AK24_TEST_ASSERT(allocated >= 3072); // Original block + large block
  AK24_TEST_ASSERT(used >= 2048);

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test block overflow creates new block
int test_arena_block_overflow(void) {
  ak_arena_t *arena = ak_arena_new(512);

  // Fill first block
  void *ptr1 = ak_arena_alloc(arena, 300);
  void *ptr2 = ak_arena_alloc(arena, 300); // Should trigger new block

  AK24_TEST_ASSERT_NOT_NULL(ptr1);
  AK24_TEST_ASSERT_NOT_NULL(ptr2);

  size_t allocated, used;
  ak_arena_stats(arena, &allocated, &used);
  AK24_TEST_ASSERT(allocated >= 1024); // At least 2 blocks
  AK24_TEST_ASSERT(used >= 600);

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test aligned allocation
int test_arena_alloc_aligned(void) {
  ak_arena_t *arena = ak_arena_new(4096);

  void *ptr16 = ak_arena_alloc_aligned(arena, 100, 16);
  void *ptr32 = ak_arena_alloc_aligned(arena, 100, 32);
  void *ptr64 = ak_arena_alloc_aligned(arena, 100, 64);

  AK24_TEST_ASSERT_NOT_NULL(ptr16);
  AK24_TEST_ASSERT_NOT_NULL(ptr32);
  AK24_TEST_ASSERT_NOT_NULL(ptr64);

  // Check alignment
  AK24_TEST_ASSERT_EQ((uintptr_t)ptr16 % 16, 0);
  AK24_TEST_ASSERT_EQ((uintptr_t)ptr32 % 32, 0);
  AK24_TEST_ASSERT_EQ((uintptr_t)ptr64 % 64, 0);

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test calloc (zero-initialized allocation)
int test_arena_calloc(void) {
  ak_arena_t *arena = ak_arena_new(4096);

  int *array = ak_arena_calloc(arena, 10, sizeof(int));
  AK24_TEST_ASSERT_NOT_NULL(array);

  // Verify all zeros
  for (int i = 0; i < 10; i++) {
    AK24_TEST_ASSERT_EQ(array[i], 0);
  }

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test realloc - grow in place
int test_arena_realloc_grow_in_place(void) {
  ak_arena_t *arena = ak_arena_new(4096);

  char *ptr = ak_arena_alloc(arena, 100);
  AK24_TEST_ASSERT_NOT_NULL(ptr);

  // Write some data
  memset(ptr, 'A', 100);

  // Grow (should be in place since it's the last allocation)
  char *new_ptr = ak_arena_realloc(arena, ptr, 100, 200);
  AK24_TEST_ASSERT_NOT_NULL(new_ptr);
  AK24_TEST_ASSERT(new_ptr == ptr); // Same pointer

  // Verify old data still there
  for (int i = 0; i < 100; i++) {
    AK24_TEST_ASSERT_EQ(new_ptr[i], 'A');
  }

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test realloc - relocation
int test_arena_realloc_relocate(void) {
  ak_arena_t *arena = ak_arena_new(4096);

  char *ptr1 = ak_arena_alloc(arena, 100);
  AK24_TEST_ASSERT_NOT_NULL(ptr1);
  memset(ptr1, 'A', 100);

  char *ptr2 = ak_arena_alloc(arena, 100);
  AK24_TEST_ASSERT_NOT_NULL(ptr2);

  // Try to grow ptr1 (not the last allocation, will relocate)
  char *new_ptr1 = ak_arena_realloc(arena, ptr1, 100, 200);
  AK24_TEST_ASSERT_NOT_NULL(new_ptr1);

  // Verify data copied
  for (int i = 0; i < 100; i++) {
    AK24_TEST_ASSERT_EQ(new_ptr1[i], 'A');
  }

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test realloc - shrink
int test_arena_realloc_shrink(void) {
  ak_arena_t *arena = ak_arena_new(4096);

  char *ptr = ak_arena_alloc(arena, 200);
  AK24_TEST_ASSERT_NOT_NULL(ptr);
  memset(ptr, 'B', 200);

  // Shrink (should return same pointer)
  char *new_ptr = ak_arena_realloc(arena, ptr, 200, 100);
  AK24_TEST_ASSERT_NOT_NULL(new_ptr);
  AK24_TEST_ASSERT(new_ptr == ptr); // Same pointer

  // Verify data intact
  for (int i = 0; i < 100; i++) {
    AK24_TEST_ASSERT_EQ(new_ptr[i], 'B');
  }

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test reset
int test_arena_reset(void) {
  ak_arena_t *arena = ak_arena_new(4096);

  void *ptr1 = ak_arena_alloc(arena, 100);
  void *ptr2 = ak_arena_alloc(arena, 200);
  AK24_TEST_ASSERT_NOT_NULL(ptr1);
  AK24_TEST_ASSERT_NOT_NULL(ptr2);

  size_t allocated1, used1;
  ak_arena_stats(arena, &allocated1, &used1);
  AK24_TEST_ASSERT(used1 >= 300);

  // Reset
  ak_arena_reset(arena);

  size_t allocated2, used2;
  ak_arena_stats(arena, &allocated2, &used2);
  AK24_TEST_ASSERT_EQ(allocated2, allocated1); // Same allocation
  AK24_TEST_ASSERT_EQ(used2, 0);               // No usage

  // Can allocate again
  void *ptr3 = ak_arena_alloc(arena, 150);
  AK24_TEST_ASSERT_NOT_NULL(ptr3);

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test snapshot and restore
int test_arena_snapshot_restore(void) {
  ak_arena_t *arena = ak_arena_new(4096);

  void *ptr1 = ak_arena_alloc(arena, 100);
  AK24_TEST_ASSERT_NOT_NULL(ptr1);

  // Take snapshot
  ak_arena_mark_t mark = ak_arena_snapshot(arena);

  void *ptr2 = ak_arena_alloc(arena, 200);
  void *ptr3 = ak_arena_alloc(arena, 300);
  AK24_TEST_ASSERT_NOT_NULL(ptr2);
  AK24_TEST_ASSERT_NOT_NULL(ptr3);

  size_t allocated1, used1;
  ak_arena_stats(arena, &allocated1, &used1);
  AK24_TEST_ASSERT(used1 >= 600);

  // Restore to snapshot
  ak_arena_restore(arena, mark);

  size_t allocated2, used2;
  ak_arena_stats(arena, &allocated2, &used2);
  AK24_TEST_ASSERT_EQ(allocated2, allocated1); // Same blocks
  AK24_TEST_ASSERT(used2 < used1);             // Less used

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test nested snapshots
int test_arena_nested_snapshots(void) {
  ak_arena_t *arena = ak_arena_new(4096);

  void *ptr1 = ak_arena_alloc(arena, 100);
  ak_arena_mark_t mark1 = ak_arena_snapshot(arena);

  void *ptr2 = ak_arena_alloc(arena, 200);
  ak_arena_mark_t mark2 = ak_arena_snapshot(arena);

  void *ptr3 = ak_arena_alloc(arena, 300);

  AK24_TEST_ASSERT_NOT_NULL(ptr1);
  AK24_TEST_ASSERT_NOT_NULL(ptr2);
  AK24_TEST_ASSERT_NOT_NULL(ptr3);

  size_t _, used3;
  ak_arena_stats(arena, &_, &used3);

  // Restore to mark2
  ak_arena_restore(arena, mark2);
  size_t used2;
  ak_arena_stats(arena, &_, &used2);
  AK24_TEST_ASSERT(used2 < used3);

  // Restore to mark1
  ak_arena_restore(arena, mark1);
  size_t used1;
  ak_arena_stats(arena, &_, &used1);
  AK24_TEST_ASSERT(used1 < used2);

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test strdup
int test_arena_strdup(void) {
  ak_arena_t *arena = ak_arena_new(4096);

  const char *original = "hello world";
  char *copy = ak_arena_strdup(arena, original);

  AK24_TEST_ASSERT_NOT_NULL(copy);
  AK24_TEST_ASSERT_STR_EQ(copy, original);
  AK24_TEST_ASSERT(copy != original); // Different pointers

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test strndup
int test_arena_strndup(void) {
  ak_arena_t *arena = ak_arena_new(4096);

  const char *source = "identifier+123";
  char *copy = ak_arena_strndup(arena, source, 10);

  AK24_TEST_ASSERT_NOT_NULL(copy);
  AK24_TEST_ASSERT_STR_EQ(copy, "identifier");
  AK24_TEST_ASSERT_EQ(strlen(copy), 10);

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test strndup with string shorter than n
int test_arena_strndup_short_string(void) {
  ak_arena_t *arena = ak_arena_new(4096);

  const char *source = "short";
  char *copy = ak_arena_strndup(arena, source, 100);

  AK24_TEST_ASSERT_NOT_NULL(copy);
  AK24_TEST_ASSERT_STR_EQ(copy, "short");
  AK24_TEST_ASSERT_EQ(strlen(copy), 5);

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test statistics accuracy
int test_arena_stats_accuracy(void) {
  ak_arena_t *arena = ak_arena_new(1024);

  size_t allocated, used;

  // Initial state
  ak_arena_stats(arena, &allocated, &used);
  AK24_TEST_ASSERT_EQ(allocated, 1024);
  AK24_TEST_ASSERT_EQ(used, 0);

  // After allocation
  ak_arena_alloc(arena, 100);
  ak_arena_stats(arena, &allocated, &used);
  AK24_TEST_ASSERT_EQ(allocated, 1024);
  AK24_TEST_ASSERT(used >= 100 && used <= 100 + AK_ARENA_DEFAULT_ALIGNMENT);

  // After block overflow
  ak_arena_alloc(arena, 1000); // Triggers new block
  ak_arena_stats(arena, &allocated, &used);
  AK24_TEST_ASSERT(allocated >= 2048);
  AK24_TEST_ASSERT(used >= 1100);

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test NULL handling
int test_arena_null_handling(void) {
  // Free NULL arena
  ak_arena_free(NULL);

  // Alloc from NULL arena
  void *ptr = ak_arena_alloc(NULL, 100);
  AK24_TEST_ASSERT_NULL(ptr);

  // Stats on NULL arena
  size_t allocated, used;
  ak_arena_stats(NULL, &allocated, &used);
  AK24_TEST_ASSERT_EQ(allocated, 0);
  AK24_TEST_ASSERT_EQ(used, 0);

  // Strdup on NULL arena
  char *str = ak_arena_strdup(NULL, "test");
  AK24_TEST_ASSERT_NULL(str);

  ak_arena_t *arena = ak_arena_new(4096);

  // Strdup NULL string
  str = ak_arena_strdup(arena, NULL);
  AK24_TEST_ASSERT_NULL(str);

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test zero-size allocations
int test_arena_zero_size(void) {
  ak_arena_t *arena = ak_arena_new(4096);

  void *ptr = ak_arena_alloc(arena, 0);
  AK24_TEST_ASSERT_NULL(ptr);

  void *ptr2 = ak_arena_calloc(arena, 0, 10);
  AK24_TEST_ASSERT_NULL(ptr2);

  void *ptr3 = ak_arena_calloc(arena, 10, 0);
  AK24_TEST_ASSERT_NULL(ptr3);

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test zero-size arena creation
int test_arena_zero_block_size(void) {
  ak_arena_t *arena = ak_arena_new(0);
  AK24_TEST_ASSERT_NULL(arena);

  AK24_TEST_PASS();
}

// Test many small allocations
int test_arena_many_small_allocs(void) {
  ak_arena_t *arena = ak_arena_new(4096);

  const int count = 1000;
  void *ptrs[1000];

  for (int i = 0; i < count; i++) {
    ptrs[i] = ak_arena_alloc(arena, 16);
    AK24_TEST_ASSERT_NOT_NULL(ptrs[i]);
  }

  // All pointers should be valid and different
  for (int i = 0; i < count - 1; i++) {
    AK24_TEST_ASSERT(ptrs[i] != ptrs[i + 1]);
  }

  size_t allocated, used;
  ak_arena_stats(arena, &allocated, &used);
  AK24_TEST_ASSERT(used >= count * 16);

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test reuse after reset
int test_arena_reset_reuse(void) {
  ak_arena_t *arena = ak_arena_new(4096);

  // First round
  for (int i = 0; i < 100; i++) {
    void *ptr = ak_arena_alloc(arena, 32);
    AK24_TEST_ASSERT_NOT_NULL(ptr);
  }

  size_t allocated1, used1;
  ak_arena_stats(arena, &allocated1, &used1);

  ak_arena_reset(arena);

  // Second round
  for (int i = 0; i < 100; i++) {
    void *ptr = ak_arena_alloc(arena, 32);
    AK24_TEST_ASSERT_NOT_NULL(ptr);
  }

  size_t allocated2, used2;
  ak_arena_stats(arena, &allocated2, &used2);

  // Should have same block allocation
  AK24_TEST_ASSERT_EQ(allocated1, allocated2);
  // Should have similar usage
  AK24_TEST_ASSERT(used1 == used2 ||
                   (used1 >= used2 - 64 && used1 <= used2 + 64));

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test alignment validation
int test_arena_alignment_validation(void) {
  ak_arena_t *arena = ak_arena_new(4096);

  // Invalid alignment (not power of 2)
  void *ptr1 = ak_arena_alloc_aligned(arena, 100, 3);
  AK24_TEST_ASSERT_NULL(ptr1);

  void *ptr2 = ak_arena_alloc_aligned(arena, 100, 7);
  AK24_TEST_ASSERT_NULL(ptr2);

  // Zero alignment
  void *ptr3 = ak_arena_alloc_aligned(arena, 100, 0);
  AK24_TEST_ASSERT_NULL(ptr3);

  // Valid alignments should work
  void *ptr4 = ak_arena_alloc_aligned(arena, 100, 8);
  AK24_TEST_ASSERT_NOT_NULL(ptr4);

  void *ptr5 = ak_arena_alloc_aligned(arena, 100, 16);
  AK24_TEST_ASSERT_NOT_NULL(ptr5);

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Test overflow protection in calloc
int test_arena_calloc_overflow(void) {
  ak_arena_t *arena = ak_arena_new(4096);

  // This would overflow: SIZE_MAX / 2 * SIZE_MAX / 2
  size_t large = SIZE_MAX / 2;
  void *ptr = ak_arena_calloc(arena, large, large);
  AK24_TEST_ASSERT_NULL(ptr);

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

// Stress test: allocate and reset repeatedly
int test_arena_stress_reset(void) {
  ak_arena_t *arena = ak_arena_new(8192);

  for (int round = 0; round < 100; round++) {
    for (int i = 0; i < 50; i++) {
      void *ptr = ak_arena_alloc(arena, 64);
      AK24_TEST_ASSERT_NOT_NULL(ptr);
    }
    ak_arena_reset(arena);
  }

  size_t allocated, used;
  ak_arena_stats(arena, &allocated, &used);
  AK24_TEST_ASSERT_EQ(used, 0);

  ak_arena_free(arena);
  AK24_TEST_PASS();
}

int main(void) {
  ak_kernel_init();

  AK24_TEST_RUN(test_arena_new_free);
  AK24_TEST_RUN(test_arena_new_default);
  AK24_TEST_RUN(test_arena_alloc_basic);
  AK24_TEST_RUN(test_arena_multiple_allocs);
  AK24_TEST_RUN(test_arena_large_alloc);
  AK24_TEST_RUN(test_arena_block_overflow);
  AK24_TEST_RUN(test_arena_alloc_aligned);
  AK24_TEST_RUN(test_arena_calloc);
  AK24_TEST_RUN(test_arena_realloc_grow_in_place);
  AK24_TEST_RUN(test_arena_realloc_relocate);
  AK24_TEST_RUN(test_arena_realloc_shrink);
  AK24_TEST_RUN(test_arena_reset);
  AK24_TEST_RUN(test_arena_snapshot_restore);
  AK24_TEST_RUN(test_arena_nested_snapshots);
  AK24_TEST_RUN(test_arena_strdup);
  AK24_TEST_RUN(test_arena_strndup);
  AK24_TEST_RUN(test_arena_strndup_short_string);
  AK24_TEST_RUN(test_arena_stats_accuracy);
  AK24_TEST_RUN(test_arena_null_handling);
  AK24_TEST_RUN(test_arena_zero_size);
  AK24_TEST_RUN(test_arena_zero_block_size);
  AK24_TEST_RUN(test_arena_many_small_allocs);
  AK24_TEST_RUN(test_arena_reset_reuse);
  AK24_TEST_RUN(test_arena_alignment_validation);
  AK24_TEST_RUN(test_arena_calloc_overflow);
  AK24_TEST_RUN(test_arena_stress_reset);

  ak_kernel_deinit();
  return 0;
}
