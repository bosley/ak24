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

#include <stddef.h>
#include <time.h>

/**
 * @brief Platform detection
 */
#if defined(_WIN32) || defined(_WIN64)
#define AK24_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__) || defined(__linux__)
#define AK24_PLATFORM_POSIX
#include <pthread.h>
#else
#error "Unsupported platform - only POSIX and Windows are supported"
#endif

/**
 * @brief Platform-agnostic mutex type
 *
 * Wrapper around platform-specific mutex implementation.
 * On POSIX: pthread_mutex_t, on Windows: CRITICAL_SECTION
 */
typedef struct {
#ifdef AK24_PLATFORM_POSIX
  pthread_mutex_t handle;
#elif defined(AK24_PLATFORM_WINDOWS)
  CRITICAL_SECTION handle;
#endif
} AK_MUTEX;

/**
 * @brief Platform-agnostic condition variable type
 *
 * Wrapper around platform-specific condition variable.
 * On POSIX: pthread_cond_t, on Windows: CONDITION_VARIABLE
 */
typedef struct {
#ifdef AK24_PLATFORM_POSIX
  pthread_cond_t handle;
#elif defined(AK24_PLATFORM_WINDOWS)
  CONDITION_VARIABLE handle;
#endif
} AK_COND;

/**
 * @brief Platform-agnostic thread type
 *
 * Wrapper around platform-specific thread handle.
 * On POSIX: pthread_t, on Windows: HANDLE
 */
typedef struct {
#ifdef AK24_PLATFORM_POSIX
  pthread_t handle;
#elif defined(AK24_PLATFORM_WINDOWS)
  HANDLE handle;
#endif
} AK_THREAD;

/**
 * @def AK_MUTEX_INITIALIZER
 * @brief Static mutex initializer
 *
 * Platform-agnostic static initialization for mutexes.
 * Use with static/global mutex declarations.
 */
#ifdef AK24_PLATFORM_POSIX
#define AK_MUTEX_INITIALIZER {PTHREAD_MUTEX_INITIALIZER}
#elif defined(AK24_PLATFORM_WINDOWS)
#define AK_MUTEX_INITIALIZER {0}
#endif

/**
 * @def AK_COND_INITIALIZER
 * @brief Static condition variable initializer
 *
 * Platform-agnostic static initialization for condition variables.
 */
#ifdef AK24_PLATFORM_POSIX
#define AK_COND_INITIALIZER {PTHREAD_COND_INITIALIZER}
#elif defined(AK24_PLATFORM_WINDOWS)
#define AK_COND_INITIALIZER {0}
#endif

/**
 * @def AK_MUTEX_INIT
 * @brief Initialize a mutex at runtime
 */
#define AK_MUTEX_INIT(m) AK_mutex_init(m)

/**
 * @def AK_MUTEX_DESTROY
 * @brief Destroy a mutex
 */
#define AK_MUTEX_DESTROY(m) AK_mutex_destroy(m)

/**
 * @def AK_MUTEX_LOCK
 * @brief Lock a mutex
 */
#define AK_MUTEX_LOCK(m) AK_mutex_lock(m)

/**
 * @def AK_MUTEX_UNLOCK
 * @brief Unlock a mutex
 */
#define AK_MUTEX_UNLOCK(m) AK_mutex_unlock(m)

/**
 * @def AK_COND_INIT
 * @brief Initialize a condition variable at runtime
 */
#define AK_COND_INIT(c) AK_cond_init(c)

/**
 * @def AK_COND_DESTROY
 * @brief Destroy a condition variable
 */
#define AK_COND_DESTROY(c) AK_cond_destroy(c)

/**
 * @def AK_COND_WAIT
 * @brief Wait on a condition variable
 */
#define AK_COND_WAIT(c, m) AK_cond_wait(c, m)

/**
 * @def AK_COND_SIGNAL
 * @brief Signal one thread waiting on condition variable
 */
#define AK_COND_SIGNAL(c) AK_cond_signal(c)

/**
 * @def AK_COND_BROADCAST
 * @brief Broadcast to all threads waiting on condition variable
 */
#define AK_COND_BROADCAST(c) AK_cond_broadcast(c)

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

/**
 * @brief Initialize a mutex at runtime
 *
 * @param mutex Mutex to initialize
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 */
int AK_mutex_init(AK_MUTEX *mutex);

/**
 * @brief Destroy a mutex
 *
 * @param mutex Mutex to destroy
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 */
int AK_mutex_destroy(AK_MUTEX *mutex);

/**
 * @brief Lock a mutex
 *
 * @param mutex Mutex to lock
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 */
int AK_mutex_lock(AK_MUTEX *mutex);

/**
 * @brief Unlock a mutex
 *
 * @param mutex Mutex to unlock
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 */
int AK_mutex_unlock(AK_MUTEX *mutex);

/**
 * @brief Initialize a condition variable at runtime
 *
 * @param cond Condition variable to initialize
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 */
int AK_cond_init(AK_COND *cond);

/**
 * @brief Destroy a condition variable
 *
 * @param cond Condition variable to destroy
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 */
int AK_cond_destroy(AK_COND *cond);

/**
 * @brief Wait on a condition variable
 *
 * Atomically unlocks mutex and waits on condition variable.
 * Reacquires mutex before returning.
 *
 * @param cond Condition variable to wait on
 * @param mutex Mutex associated with condition variable
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 */
int AK_cond_wait(AK_COND *cond, AK_MUTEX *mutex);

/**
 * @brief Signal one thread waiting on condition variable
 *
 * @param cond Condition variable to signal
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 */
int AK_cond_signal(AK_COND *cond);

/**
 * @brief Broadcast to all threads waiting on condition variable
 *
 * @param cond Condition variable to broadcast
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 */
int AK_cond_broadcast(AK_COND *cond);

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
 * @brief Create a new thread (GC-aware)
 *
 * Platform-agnostic thread creation with GC support.
 * Uses GC_pthread_create on POSIX with GC enabled.
 *
 * @param thread Pointer to thread handle
 * @param start_routine Thread entry point
 * @param arg Argument to pass to thread
 * @return 0 on success, non-zero on failure
 *
 * @threadsafe
 */
int AK_THREAD_CREATE(AK_THREAD *thread, void *(*start_routine)(void *),
                     void *arg);

/**
 * @brief Join with a terminated thread
 *
 * Waits for the specified thread to terminate.
 *
 * @param thread Thread to join
 * @return 0 on success, non-zero on failure
 *
 * @threadsafe
 */
int AK_THREAD_JOIN(AK_THREAD thread);

/**
 * @brief Detach a thread
 *
 * Marks thread as detached; resources released on termination.
 *
 * @param thread Thread to detach
 * @return 0 on success, non-zero on failure
 *
 * @threadsafe
 */
int AK_THREAD_DETACH(AK_THREAD thread);

/**
 * @def AK24_THREAD_CREATE
 * @brief Backwards compatibility macro for thread creation
 *
 * @deprecated Use AK_THREAD_CREATE instead
 */
#define AK24_THREAD_CREATE(thread, attr, start_routine, arg)                   \
  AK_THREAD_CREATE(thread, start_routine, arg)

/**
 * @def AK24_THREAD_JOIN
 * @brief Backwards compatibility macro for thread join
 *
 * @deprecated Use AK_THREAD_JOIN instead
 */
#define AK24_THREAD_JOIN(thread, retval) AK_THREAD_JOIN(thread)

/**
 * @def AK24_THREAD_DETACH
 * @brief Backwards compatibility macro for thread detach
 *
 * @deprecated Use AK_THREAD_DETACH instead
 */
#define AK24_THREAD_DETACH(thread) AK_THREAD_DETACH(thread)

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
 * @brief Create a new thread (non-GC)
 *
 * Platform-agnostic thread creation without GC.
 * Uses standard pthread_create on POSIX.
 *
 * @param thread Pointer to thread handle
 * @param start_routine Thread entry point
 * @param arg Argument to pass to thread
 * @return 0 on success, non-zero on failure
 *
 * @threadsafe
 */
int AK_THREAD_CREATE(AK_THREAD *thread, void *(*start_routine)(void *),
                     void *arg);

/**
 * @brief Join with a terminated thread
 *
 * Waits for the specified thread to terminate.
 *
 * @param thread Thread to join
 * @return 0 on success, non-zero on failure
 *
 * @threadsafe
 */
int AK_THREAD_JOIN(AK_THREAD thread);

/**
 * @brief Detach a thread
 *
 * Marks thread as detached; resources released on termination.
 *
 * @param thread Thread to detach
 * @return 0 on success, non-zero on failure
 *
 * @threadsafe
 */
int AK_THREAD_DETACH(AK_THREAD thread);

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

/**
 * @brief Register a signal handler
 *
 * Registers a lambda to be invoked when a specific signal is received.
 * The lambda receives a pointer to int containing the signal number as
 * invoke_args.
 *
 * @param signum Signal number (SIGINT, SIGTERM, etc.)
 * @param handler Lambda to invoke on signal
 *
 * @threadsafe
 *
 * @par Example:
 * @code
 * void handle_interrupt(void *ctx, void *args) {
 *   int signum = *(int *)args;
 *   printf("Caught signal %d\n", signum);
 * }
 *
 * ak_lambda_t *handler = ak_lambda_new(handle_interrupt, NULL, NULL);
 * ak_register_signal_handler(SIGINT, handler);
 * @endcode
 */
void ak_register_signal_handler(int signum, ak_lambda_t *handler);

/**
 * @brief Unregister a signal handler
 *
 * Removes the handler for a specific signal and restores the previous
 * signal handler.
 *
 * @param signum Signal number to unregister
 *
 * @threadsafe
 */
void ak_unregister_signal_handler(int signum);

// ---- New need to doc

/*
    Returns owned pointer to a module context. when done, call the
    matching free. this sill not stop or unload any modules, only
    the handle to control module that was received from get_ctx
*/
ak_module_ctx_t *ak_module_get_system_ctx(void);

void ak_module_free_system_ctx(ak_module_ctx_t *ctx);

#endif
