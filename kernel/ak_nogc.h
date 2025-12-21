/**
 * @file ak_nogc.h
 * @brief Non-garbage collection kernel definitions
 *
 * Memory allocation macros and kernel initialization for non-GC builds.
 * Uses standard malloc/free for manual memory management.
 * Conditionally included by kernel.h when AK24_GC_ENABLED is not defined.
 */

#ifndef AK24_AK_NOGC_H
#define AK24_AK_NOGC_H

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
 * @brief Initialize kernel without GC
 *
 * Must be called before using kernel functions.
 * Initializes kernel subsystems without GC.
 * Creates application-specific runtime directory and lock file.
 *
 * @param app_id Application identity string (used for runtime directory
 * isolation)
 *
 * @notthreadsafe
 */
void ak_kernel_init(const char *app_id);

/**
 * @brief Deinitialize kernel
 *
 * Cleans up kernel resources and invokes shutdown callbacks.
 *
 * @notthreadsafe
 */
void ak_kernel_deinit(void);

#endif // AK24_AK_NOGC_H
