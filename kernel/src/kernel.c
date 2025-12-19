#include "kernel.h"
#include <pthread.h>
#include <stdio.h>

#if AK24_GC_ENABLED
#include <sched.h>
#include <unistd.h>
#endif

static list_void_t shutdown_lambdas;
static int shutdown_lambdas_initialized = 0;
static time_t kernel_start_time;

#if AK24_BUILD_DEBUG_MEMORY

static pthread_mutex_t mem_stats_mutex = PTHREAD_MUTEX_INITIALIZER;
static ak_memory_stats_t mem_stats = {0};

#if AK24_GC_ENABLED

void *ak_mem_alloc_tracked(size_t size, const char *file, int line) {
  (void)file;
  (void)line;
  void *ptr = GC_MALLOC(size);
  if (ptr) {
    pthread_mutex_lock(&mem_stats_mutex);
    mem_stats.total_allocations++;
    mem_stats.bytes_allocated += size;
    mem_stats.current_bytes += size;
    if (mem_stats.current_bytes > mem_stats.peak_bytes) {
      mem_stats.peak_bytes = mem_stats.current_bytes;
    }
    pthread_mutex_unlock(&mem_stats_mutex);
  }
  return ptr;
}

void *ak_mem_alloc_atomic_tracked(size_t size, const char *file, int line) {
  (void)file;
  (void)line;
  void *ptr = GC_MALLOC_ATOMIC(size);
  if (ptr) {
    pthread_mutex_lock(&mem_stats_mutex);
    mem_stats.total_allocations++;
    mem_stats.bytes_allocated += size;
    mem_stats.current_bytes += size;
    if (mem_stats.current_bytes > mem_stats.peak_bytes) {
      mem_stats.peak_bytes = mem_stats.current_bytes;
    }
    pthread_mutex_unlock(&mem_stats_mutex);
  }
  return ptr;
}

void *ak_mem_realloc_tracked(void *ptr, size_t size, const char *file,
                             int line) {
  (void)file;
  (void)line;
  void *new_ptr = GC_REALLOC(ptr, size);
  if (new_ptr) {
    pthread_mutex_lock(&mem_stats_mutex);
    mem_stats.total_reallocs++;
    mem_stats.bytes_allocated += size;
    mem_stats.current_bytes += size;
    if (mem_stats.current_bytes > mem_stats.peak_bytes) {
      mem_stats.peak_bytes = mem_stats.current_bytes;
    }
    pthread_mutex_unlock(&mem_stats_mutex);
  }
  return new_ptr;
}

void ak_mem_free_tracked(void *ptr, const char *file, int line) {
  (void)file;
  (void)line;
  if (ptr) {
    pthread_mutex_lock(&mem_stats_mutex);
    mem_stats.total_frees++;
    pthread_mutex_unlock(&mem_stats_mutex);
    GC_FREE(ptr);
  }
}

#else

void *ak_mem_alloc_tracked(size_t size, const char *file, int line) {
  (void)file;
  (void)line;
  void *ptr = malloc(size);
  if (ptr) {
    pthread_mutex_lock(&mem_stats_mutex);
    mem_stats.total_allocations++;
    mem_stats.bytes_allocated += size;
    mem_stats.current_bytes += size;
    if (mem_stats.current_bytes > mem_stats.peak_bytes) {
      mem_stats.peak_bytes = mem_stats.current_bytes;
    }
    pthread_mutex_unlock(&mem_stats_mutex);
  }
  return ptr;
}

void *ak_mem_alloc_atomic_tracked(size_t size, const char *file, int line) {
  (void)file;
  (void)line;
  void *ptr = malloc(size);
  if (ptr) {
    pthread_mutex_lock(&mem_stats_mutex);
    mem_stats.total_allocations++;
    mem_stats.bytes_allocated += size;
    mem_stats.current_bytes += size;
    if (mem_stats.current_bytes > mem_stats.peak_bytes) {
      mem_stats.peak_bytes = mem_stats.current_bytes;
    }
    pthread_mutex_unlock(&mem_stats_mutex);
  }
  return ptr;
}

void *ak_mem_realloc_tracked(void *ptr, size_t size, const char *file,
                             int line) {
  (void)file;
  (void)line;
  void *new_ptr = realloc(ptr, size);
  if (new_ptr) {
    pthread_mutex_lock(&mem_stats_mutex);
    mem_stats.total_reallocs++;
    mem_stats.bytes_allocated += size;
    mem_stats.current_bytes += size;
    if (mem_stats.current_bytes > mem_stats.peak_bytes) {
      mem_stats.peak_bytes = mem_stats.current_bytes;
    }
    pthread_mutex_unlock(&mem_stats_mutex);
  }
  return new_ptr;
}

void ak_mem_free_tracked(void *ptr, const char *file, int line) {
  (void)file;
  (void)line;
  if (ptr) {
    pthread_mutex_lock(&mem_stats_mutex);
    mem_stats.total_frees++;
    pthread_mutex_unlock(&mem_stats_mutex);
    free(ptr);
  }
}

#endif

ak_memory_stats_t ak_mem_get_stats(void) {
  pthread_mutex_lock(&mem_stats_mutex);
  ak_memory_stats_t stats = mem_stats;
  pthread_mutex_unlock(&mem_stats_mutex);
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

#endif

#if AK24_GC_ENABLED

void ak_kernel_init(void) {
  kernel_start_time = time(NULL);
  GC_set_warn_proc(GC_ignore_warn_proc);
  GC_INIT();
  list_init(&shutdown_lambdas);
  shutdown_lambdas_initialized = 1;
}

void ak_kernel_deinit(void) {
  if (shutdown_lambdas_initialized) {
    kernel_shutdown_info_t info;
    info.start_time = kernel_start_time;
#if AK24_BUILD_DEBUG_MEMORY
    info.memory_stats = ak_mem_get_stats();
#endif

    list_iter_t iter = list_iter(&shutdown_lambdas);
    void **lambda_ptr;
    while ((lambda_ptr = list_next(&shutdown_lambdas, &iter))) {
      ak_lambda_t *lambda = (ak_lambda_t *)*lambda_ptr;
      if (lambda) {
        ak_lambda_invoke(lambda, &info);
      }
    }
    list_deinit(&shutdown_lambdas);
    shutdown_lambdas_initialized = 0;
  }
  sched_yield();
  sched_yield();
}

#else

void ak_kernel_init(void) {
  kernel_start_time = time(NULL);
  list_init(&shutdown_lambdas);
  shutdown_lambdas_initialized = 1;
}

void ak_kernel_deinit(void) {
  if (shutdown_lambdas_initialized) {
    kernel_shutdown_info_t info;
    info.start_time = kernel_start_time;
#if AK24_BUILD_DEBUG_MEMORY
    info.memory_stats = ak_mem_get_stats();
#endif

    list_iter_t iter = list_iter(&shutdown_lambdas);
    void **lambda_ptr;
    while ((lambda_ptr = list_next(&shutdown_lambdas, &iter))) {
      ak_lambda_t *lambda = (ak_lambda_t *)*lambda_ptr;
      if (lambda) {
        ak_lambda_invoke(lambda, &info);
      }
    }
    list_deinit(&shutdown_lambdas);
    shutdown_lambdas_initialized = 0;
  }
}

#endif

void ak_on_shutdown(ak_lambda_t *lambda) {
  if (!lambda || !shutdown_lambdas_initialized) {
    return;
  }
  list_push(&shutdown_lambdas, lambda);
}

list_str_t ak_args_to_list(int argc, char **argv) {
  list_str_t args;
  list_init(&args);
  for (int i = 0; i < argc; i++) {
    list_push(&args, argv[i]);
  }
  return args;
}
