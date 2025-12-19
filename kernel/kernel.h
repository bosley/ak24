#ifndef AK24_KERNEL_H
#define AK24_KERNEL_H

#include "arbuff.h"
#include "atom.h"
#include "buffer.h"
#include "context.h"
#include "lambda.h"
#include "list.h"
#include "log.h"
#include "map.h"
#include "scanner.h"

#include <pthread.h>
#include <stddef.h>
#include <time.h>

#if AK24_BUILD_DEBUG_MEMORY

typedef struct {
  size_t total_allocations;
  size_t total_frees;
  size_t total_reallocs;
  size_t bytes_allocated;
  size_t bytes_freed;
  size_t current_bytes;
  size_t peak_bytes;
} ak_memory_stats_t;

#endif

typedef struct kernel_shutdown_info_s {
  time_t start_time;
#if AK24_BUILD_DEBUG_MEMORY
  ak_memory_stats_t memory_stats;
#endif
} kernel_shutdown_info_t;

#if AK24_BUILD_DEBUG_MEMORY

void *ak_mem_alloc_tracked(size_t size, const char *file, int line);
void *ak_mem_alloc_atomic_tracked(size_t size, const char *file, int line);
void *ak_mem_realloc_tracked(void *ptr, size_t size, const char *file,
                             int line);
void ak_mem_free_tracked(void *ptr, const char *file, int line);
ak_memory_stats_t ak_mem_get_stats(void);
void ak_mem_print_stats(void);

#endif

#if AK24_GC_ENABLED

#define GC_THREADS 1
#include <gc.h>

#if AK24_BUILD_DEBUG_MEMORY
#define AK24_ALLOC(size) ak_mem_alloc_tracked(size, __FILE__, __LINE__)
#define AK24_ALLOC_ATOMIC(size)                                                \
  ak_mem_alloc_atomic_tracked(size, __FILE__, __LINE__)
#define AK24_REALLOC(ptr, size)                                                \
  ak_mem_realloc_tracked(ptr, size, __FILE__, __LINE__)
#define AK24_FREE(ptr) ak_mem_free_tracked(ptr, __FILE__, __LINE__)
#else
#define AK24_ALLOC(size) GC_MALLOC(size)
#define AK24_ALLOC_ATOMIC(size) GC_MALLOC_ATOMIC(size)
#define AK24_REALLOC(ptr, size) GC_REALLOC(ptr, size)
#define AK24_FREE(ptr) GC_FREE(ptr)
#endif

#define AK24_THREAD_CREATE(thread, attr, start_routine, arg)                   \
  GC_pthread_create(thread, attr, start_routine, arg)
#define AK24_THREAD_JOIN(thread, retval) pthread_join(thread, retval)
#define AK24_THREAD_DETACH(thread) pthread_detach(thread)

void ak_kernel_init(void);
void ak_kernel_deinit(void);

#else

#include <stdlib.h>

#if AK24_BUILD_DEBUG_MEMORY
#define AK24_ALLOC(size) ak_mem_alloc_tracked(size, __FILE__, __LINE__)
#define AK24_ALLOC_ATOMIC(size)                                                \
  ak_mem_alloc_atomic_tracked(size, __FILE__, __LINE__)
#define AK24_REALLOC(ptr, size)                                                \
  ak_mem_realloc_tracked(ptr, size, __FILE__, __LINE__)
#define AK24_FREE(ptr) ak_mem_free_tracked(ptr, __FILE__, __LINE__)
#else
#define AK24_ALLOC(size) malloc(size)
#define AK24_ALLOC_ATOMIC(size) malloc(size)
#define AK24_REALLOC(ptr, size) realloc(ptr, size)
#define AK24_FREE(ptr) free(ptr)
#endif

#define AK24_THREAD_CREATE(thread, attr, start_routine, arg)                   \
  pthread_create(thread, attr, start_routine, arg)
#define AK24_THREAD_JOIN(thread, retval) pthread_join(thread, retval)
#define AK24_THREAD_DETACH(thread) pthread_detach(thread)

void ak_kernel_init(void);
void ak_kernel_deinit(void);

#endif

void ak_on_shutdown(ak_lambda_t *lambda);

list_str_t ak_args_to_list(int argc, char **argv);

#endif
