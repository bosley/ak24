/**
 * @file ak_gc.h
 * @brief Garbage collection enabled kernel definitions
 *
 * Memory allocation macros and kernel initialization for GC-enabled builds.
 * Uses Boehm GC for automatic memory management.
 * Conditionally included by kernel.h when AK24_GC_ENABLED is defined.
 */

#ifndef AK24_AK_GC_H
#define AK24_AK_GC_H

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
 * @brief Initialize kernel with GC
 *
 * Must be called before using kernel functions.
 * Initializes Boehm GC and kernel subsystems.
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

#endif // AK24_AK_GC_H
