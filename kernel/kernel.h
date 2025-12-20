/**
 * @file kernel.h
 * @brief Main kernel header with memory management and initialization
 *
 * Central kernel header that includes all kernel modules and provides
 * memory management abstractions, garbage collection integration, and
 * kernel initialization/deinitialization. Supports both GC and non-GC
 * builds with optional memory tracking for debugging.
 *
 * Key features:
 * - Unified memory allocation macros (GC or standard malloc)
 * - Optional memory tracking and statistics
 * - Thread creation abstractions
 * - Kernel initialization and shutdown
 * - Shutdown callback registration
 * - Command-line argument processing
 *
 * @note Conditional compilation based on AK24_GC_ENABLED and
 * AK24_BUILD_DEBUG_MEMORY
 */

#ifndef AK24_KERNEL_H
#define AK24_KERNEL_H

#include "arbuff.h"
#include "atom.h"
#include "buffer.h"
#include "context.h"
#include "forms.h"
#include "forms_primitives.h"
#include "interfaces.h"
#include "lambda.h"
#include "list.h"
#include "log.h"
#include "map.h"
#include "scanner.h"

#include <pthread.h>
#include <stddef.h>
#include <time.h>

#if AK24_BUILD_DEBUG_MEMORY

/**
 * @brief Memory allocation statistics
 *
 * Tracks memory usage for debugging and profiling.
 */
typedef struct {
  size_t total_allocations; /**< Total number of allocations */
  size_t total_frees;       /**< Total number of frees */
  size_t total_reallocs;    /**< Total number of reallocs */
  size_t bytes_allocated;   /**< Total bytes allocated */
  size_t bytes_freed;       /**< Total bytes freed */
  size_t current_bytes;     /**< Current bytes in use */
  size_t peak_bytes;        /**< Peak memory usage */
} ak_memory_stats_t;

#endif

/**
 * @brief Kernel shutdown information
 *
 * Passed to shutdown callbacks with runtime statistics.
 */
typedef struct kernel_shutdown_info_s {
  time_t start_time; /**< Kernel start time */
#if AK24_BUILD_DEBUG_MEMORY
  ak_memory_stats_t memory_stats; /**< Memory statistics if tracking enabled */
#endif
} kernel_shutdown_info_t;

#if AK24_BUILD_DEBUG_MEMORY

/**
 * @brief Tracked memory allocation
 *
 * Internal allocation with tracking metadata.
 *
 * @param size Size in bytes
 * @param file Source file
 * @param line Source line
 * @return Pointer to allocated memory, or NULL on failure
 *
 * @threadsafe
 */
void *ak_mem_alloc_tracked(size_t size, const char *file, int line);

/**
 * @brief Tracked atomic memory allocation
 *
 * Allocates memory suitable for atomic data (no pointers).
 *
 * @param size Size in bytes
 * @param file Source file
 * @param line Source line
 * @return Pointer to allocated memory, or NULL on failure
 *
 * @threadsafe
 */
void *ak_mem_alloc_atomic_tracked(size_t size, const char *file, int line);

/**
 * @brief Tracked memory reallocation
 *
 * @param ptr Pointer to reallocate
 * @param size New size in bytes
 * @param file Source file
 * @param line Source line
 * @return Pointer to reallocated memory, or NULL on failure
 *
 * @threadsafe
 */
void *ak_mem_realloc_tracked(void *ptr, size_t size, const char *file,
                             int line);

/**
 * @brief Tracked memory free
 *
 * @param ptr Pointer to free
 * @param file Source file
 * @param line Source line
 *
 * @threadsafe
 */
void ak_mem_free_tracked(void *ptr, const char *file, int line);

/**
 * @brief Get memory statistics
 *
 * @return Current memory statistics
 *
 * @threadsafe
 */
ak_memory_stats_t ak_mem_get_stats(void);

/**
 * @brief Print memory statistics to stdout
 *
 * @threadsafe
 */
void ak_mem_print_stats(void);

#endif

#if AK24_GC_ENABLED

/**
 * @def GC_THREADS
 * @brief Enable thread support in Boehm GC
 */
#define GC_THREADS 1
#include <gc.h>

#if AK24_BUILD_DEBUG_MEMORY
/**
 * @def AK24_ALLOC
 * @brief Allocate memory (GC with tracking)
 */
#define AK24_ALLOC(size) ak_mem_alloc_tracked(size, __FILE__, __LINE__)

/**
 * @def AK24_ALLOC_ATOMIC
 * @brief Allocate atomic memory (GC with tracking)
 */
#define AK24_ALLOC_ATOMIC(size)                                                \
  ak_mem_alloc_atomic_tracked(size, __FILE__, __LINE__)

/**
 * @def AK24_REALLOC
 * @brief Reallocate memory (GC with tracking)
 */
#define AK24_REALLOC(ptr, size)                                                \
  ak_mem_realloc_tracked(ptr, size, __FILE__, __LINE__)

/**
 * @def AK24_FREE
 * @brief Free memory (GC with tracking)
 */
#define AK24_FREE(ptr) ak_mem_free_tracked(ptr, __FILE__, __LINE__)
#else
/**
 * @def AK24_ALLOC
 * @brief Allocate memory (GC)
 */
#define AK24_ALLOC(size) GC_MALLOC(size)

/**
 * @def AK24_ALLOC_ATOMIC
 * @brief Allocate atomic memory (GC)
 */
#define AK24_ALLOC_ATOMIC(size) GC_MALLOC_ATOMIC(size)

/**
 * @def AK24_REALLOC
 * @brief Reallocate memory (GC)
 */
#define AK24_REALLOC(ptr, size) GC_REALLOC(ptr, size)

/**
 * @def AK24_FREE
 * @brief Free memory (GC)
 */
#define AK24_FREE(ptr) GC_FREE(ptr)
#endif

/**
 * @def AK24_THREAD_CREATE
 * @brief Create thread (GC-aware)
 */
#define AK24_THREAD_CREATE(thread, attr, start_routine, arg)                   \
  GC_pthread_create(thread, attr, start_routine, arg)

/**
 * @def AK24_THREAD_JOIN
 * @brief Join thread
 */
#define AK24_THREAD_JOIN(thread, retval) pthread_join(thread, retval)

/**
 * @def AK24_THREAD_DETACH
 * @brief Detach thread
 */
#define AK24_THREAD_DETACH(thread) pthread_detach(thread)

/**
 * @brief Initialize kernel with GC
 *
 * Must be called before using kernel functions.
 *
 * @notthreadsafe
 */
void ak_kernel_init(void);

/**
 * @brief Deinitialize kernel
 *
 * Cleans up kernel resources and invokes shutdown callbacks.
 *
 * @notthreadsafe
 */
void ak_kernel_deinit(void);

#else

#include <stdlib.h>

#if AK24_BUILD_DEBUG_MEMORY
/**
 * @def AK24_ALLOC
 * @brief Allocate memory (malloc with tracking)
 */
#define AK24_ALLOC(size) ak_mem_alloc_tracked(size, __FILE__, __LINE__)

/**
 * @def AK24_ALLOC_ATOMIC
 * @brief Allocate atomic memory (malloc with tracking)
 */
#define AK24_ALLOC_ATOMIC(size)                                                \
  ak_mem_alloc_atomic_tracked(size, __FILE__, __LINE__)

/**
 * @def AK24_REALLOC
 * @brief Reallocate memory (realloc with tracking)
 */
#define AK24_REALLOC(ptr, size)                                                \
  ak_mem_realloc_tracked(ptr, size, __FILE__, __LINE__)

/**
 * @def AK24_FREE
 * @brief Free memory (free with tracking)
 */
#define AK24_FREE(ptr) ak_mem_free_tracked(ptr, __FILE__, __LINE__)
#else
/**
 * @def AK24_ALLOC
 * @brief Allocate memory (malloc)
 */
#define AK24_ALLOC(size) malloc(size)

/**
 * @def AK24_ALLOC_ATOMIC
 * @brief Allocate atomic memory (malloc)
 */
#define AK24_ALLOC_ATOMIC(size) malloc(size)

/**
 * @def AK24_REALLOC
 * @brief Reallocate memory (realloc)
 */
#define AK24_REALLOC(ptr, size) realloc(ptr, size)

/**
 * @def AK24_FREE
 * @brief Free memory (free)
 */
#define AK24_FREE(ptr) free(ptr)
#endif

/**
 * @def AK24_THREAD_CREATE
 * @brief Create thread (standard pthread)
 */
#define AK24_THREAD_CREATE(thread, attr, start_routine, arg)                   \
  pthread_create(thread, attr, start_routine, arg)

/**
 * @def AK24_THREAD_JOIN
 * @brief Join thread
 */
#define AK24_THREAD_JOIN(thread, retval) pthread_join(thread, retval)

/**
 * @def AK24_THREAD_DETACH
 * @brief Detach thread
 */
#define AK24_THREAD_DETACH(thread) pthread_detach(thread)

/**
 * @brief Initialize kernel without GC
 *
 * Must be called before using kernel functions.
 *
 * @notthreadsafe
 */
void ak_kernel_init(void);

/**
 * @brief Deinitialize kernel
 *
 * Cleans up kernel resources and invokes shutdown callbacks.
 *
 * @notthreadsafe
 */
void ak_kernel_deinit(void);

#endif

/**
 * @brief Register shutdown callback
 *
 * Registers a lambda to be invoked during kernel deinitialization.
 * The lambda receives kernel_shutdown_info_t as invoke_args.
 *
 * @param lambda Callback to register
 *
 * @notthreadsafe
 *
 * @par Example:
 * @code
 * void cleanup(void *ctx, void *args) {
 *   kernel_shutdown_info_t *info = (kernel_shutdown_info_t *)args;
 *   printf("Kernel ran for %ld seconds\n",
 *          time(NULL) - info->start_time);
 * }
 *
 * ak_lambda_t *cb = ak_lambda_new(cleanup, NULL, NULL);
 * ak_on_shutdown(cb);
 * @endcode
 */
void ak_on_shutdown(ak_lambda_t *lambda);

/**
 * @brief Convert command-line arguments to list
 *
 * Creates a list of strings from argc/argv.
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @return List of argument strings
 *
 * @notthreadsafe
 *
 * @note Caller must deinitialize returned list with list_deinit()
 */
list_str_t ak_args_to_list(int argc, char **argv);

// ---- New need to doc

/*
    Returns owned pointer to a module context. when done, call the
    matching free. this sill not stop or unload any modules, only
    the handle to control module that was received from get_ctx
*/
ak_module_ctx_t *ak_module_get_system_ctx(void);

void ak_module_free_system_ctx(ak_module_ctx_t *ctx);

#endif
