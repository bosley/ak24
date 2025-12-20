#include "kernel.h"
#include <stdlib.h>

#if !AK24_GC_ENABLED

#if AK24_BUILD_DEBUG_MEMORY

static AK_MUTEX mem_stats_mutex = AK_MUTEX_INITIALIZER;
static ak_memory_stats_t mem_stats = {0};

void *ak_mem_alloc_tracked(size_t size, const char *file, int line) {
  (void)file;
  (void)line;
  void *ptr = malloc(size);
  if (ptr) {
    AK_MUTEX_LOCK(&mem_stats_mutex);
    mem_stats.total_allocations++;
    mem_stats.bytes_allocated += size;
    mem_stats.current_bytes += size;
    if (mem_stats.current_bytes > mem_stats.peak_bytes) {
      mem_stats.peak_bytes = mem_stats.current_bytes;
    }
    AK_MUTEX_UNLOCK(&mem_stats_mutex);
  }
  return ptr;
}

void *ak_mem_alloc_atomic_tracked(size_t size, const char *file, int line) {
  (void)file;
  (void)line;
  void *ptr = malloc(size);
  if (ptr) {
    AK_MUTEX_LOCK(&mem_stats_mutex);
    mem_stats.total_allocations++;
    mem_stats.bytes_allocated += size;
    mem_stats.current_bytes += size;
    if (mem_stats.current_bytes > mem_stats.peak_bytes) {
      mem_stats.peak_bytes = mem_stats.current_bytes;
    }
    AK_MUTEX_UNLOCK(&mem_stats_mutex);
  }
  return ptr;
}

void *ak_mem_realloc_tracked(void *ptr, size_t size, const char *file,
                             int line) {
  (void)file;
  (void)line;
  void *new_ptr = realloc(ptr, size);
  if (new_ptr) {
    AK_MUTEX_LOCK(&mem_stats_mutex);
    mem_stats.total_reallocs++;
    mem_stats.bytes_allocated += size;
    mem_stats.current_bytes += size;
    if (mem_stats.current_bytes > mem_stats.peak_bytes) {
      mem_stats.peak_bytes = mem_stats.current_bytes;
    }
    AK_MUTEX_UNLOCK(&mem_stats_mutex);
  }
  return new_ptr;
}

void ak_mem_free_tracked(void *ptr, const char *file, int line) {
  (void)file;
  (void)line;
  if (ptr) {
    AK_MUTEX_LOCK(&mem_stats_mutex);
    mem_stats.total_frees++;
    AK_MUTEX_UNLOCK(&mem_stats_mutex);
    free(ptr);
  }
}

ak_memory_stats_t ak_mem_get_stats(void) {
  AK_MUTEX_LOCK(&mem_stats_mutex);
  ak_memory_stats_t stats = mem_stats;
  AK_MUTEX_UNLOCK(&mem_stats_mutex);
  return stats;
}

void ak_mem_print_stats(void) {
  ak_memory_stats_t stats = ak_mem_get_stats();
  printf("\n=== Memory Statistics ===\n");
  printf("Total allocations: %zu\n", stats.total_allocations);
  printf("Total frees: %zu\n", stats.total_frees);
  printf("Total reallocs: %zu\n", stats.total_reallocs);
  printf("Bytes allocated: %zu\n", stats.bytes_allocated);
  printf("Bytes freed: %zu\n", stats.bytes_freed);
  printf("Current bytes: %zu\n", stats.current_bytes);
  printf("Peak bytes: %zu\n", stats.peak_bytes);
  printf("=========================\n\n");
}

#endif // AK24_BUILD_DEBUG_MEMORY

#endif // !AK24_GC_ENABLED
