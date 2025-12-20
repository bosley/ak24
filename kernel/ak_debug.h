/**
 * @file ak_debug.h
 * @brief Memory debugging and tracking definitions
 *
 * Provides memory allocation tracking and statistics for debugging builds.
 * Included by kernel.h when AK24_BUILD_DEBUG_MEMORY is enabled.
 */

#ifndef AK24_AK_DEBUG_H
#define AK24_AK_DEBUG_H

#include <stddef.h>

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

#endif // AK24_AK_DEBUG_H
