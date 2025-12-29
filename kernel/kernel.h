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
#include "arena.h"
#include "buffer.h"
#include "cjit.h"
#include "context.h"
#include "filepath.h"
#include "forms.h"
#include "forms_primitives.h"
#include "interfaces.h"
#include "intern.h"
#include "lambda.h"
#include "list.h"
#include "log.h"
#include "map.h"
#include "scanner.h"
#include "sourceloc.h"

#include <stddef.h>
#include <time.h>

/**
 * @brief Platform detection and inclusion
 *
 * Detects the target platform and includes the appropriate platform-specific
 * header (kernel_nix.h or kernel_win.h) which defines threading primitives
 * and platform-specific types.
 */
#if defined(_WIN32) || defined(_WIN64)
#ifndef AK24_PLATFORM_WINDOWS
#define AK24_PLATFORM_WINDOWS
#endif
#include "ak_win.h"
#elif defined(__APPLE__)
#ifndef AK24_PLATFORM_POSIX
#define AK24_PLATFORM_POSIX
#endif
#ifndef AK24_PLATFORM_APPLE
#define AK24_PLATFORM_APPLE
#endif
#include "ak_nix.h"
#elif defined(__linux__)
#ifndef AK24_PLATFORM_POSIX
#define AK24_PLATFORM_POSIX
#endif
#ifndef AK24_PLATFORM_LINUX
#define AK24_PLATFORM_LINUX
#endif
#include "ak_nix.h"
#elif defined(__unix__)
#ifndef AK24_PLATFORM_POSIX
#define AK24_PLATFORM_POSIX
#endif
#ifndef AK24_PLATFORM_UNIX
#define AK24_PLATFORM_UNIX
#endif
#include "ak_nix.h"
#else
#error "Unsupported platform - only POSIX and Windows are supported"
#endif

/**
 * @def AK24_MUTEX_INIT
 * @brief Initialize a mutex at runtime
 */
#define AK24_MUTEX_INIT(m) AK24_mutex_init(m)

/**
 * @def AK24_MUTEX_DESTROY
 * @brief Destroy a mutex
 */
#define AK24_MUTEX_DESTROY(m) AK24_mutex_destroy(m)

/**
 * @def AK24_MUTEX_LOCK
 * @brief Lock a mutex
 */
#define AK24_MUTEX_LOCK(m) AK24_mutex_lock(m)

/**
 * @def AK24_MUTEX_UNLOCK
 * @brief Unlock a mutex
 */
#define AK24_MUTEX_UNLOCK(m) AK24_mutex_unlock(m)

/**
 * @def AK24_COND_INIT
 * @brief Initialize a condition variable at runtime
 */
#define AK24_COND_INIT(c) AK24_cond_init(c)

/**
 * @def AK24_COND_DESTROY
 * @brief Destroy a condition variable
 */
#define AK24_COND_DESTROY(c) AK24_cond_destroy(c)

/**
 * @def AK24_COND_WAIT
 * @brief Wait on a condition variable
 */
#define AK24_COND_WAIT(c, m) AK24_cond_wait(c, m)

/**
 * @def AK24_COND_TIMEDWAIT
 * @brief Wait on a condition variable with timeout
 * @param c Condition variable
 * @param m Mutex
 * @param timeout_ms Timeout in milliseconds
 * @return 0 on signal, 1 on timeout, -1 on error
 */
#define AK24_COND_TIMEDWAIT(c, m, timeout_ms)                                  \
  AK24_cond_timedwait(c, m, timeout_ms)

/**
 * @def AK24_COND_SIGNAL
 * @brief Signal one thread waiting on condition variable
 */
#define AK24_COND_SIGNAL(c) AK24_cond_signal(c)

/**
 * @def AK24_COND_BROADCAST
 * @brief Broadcast to all threads waiting on condition variable
 */
#define AK24_COND_BROADCAST(c) AK24_cond_broadcast(c)

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

/**
 * @brief Memory debugging support
 *
 * When AK24_BUILD_DEBUG_MEMORY is enabled, includes memory tracking
 * functions and statistics collection.
 */
#if AK24_BUILD_DEBUG_MEMORY
#include "ak_debug.h"
#endif

/**
 * @brief Memory management and initialization
 *
 * Conditionally includes GC or non-GC memory management based on
 * AK24_GC_ENABLED build flag. Defines AK24_ALLOC family of macros
 * and ak_kernel_init/deinit functions.
 */
#if AK24_GC_ENABLED
#include "ak_gc.h"
#else
#include "ak_nogc.h"
#endif

/**
 * @brief Mutex and condition variable functions
 *
 * Platform-agnostic synchronization primitives implemented in kernel.c
 */

/**
 * @brief Initialize a mutex at runtime
 *
 * @param mutex Mutex to initialize
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 */
int AK24_mutex_init(AK24_MUTEX *mutex);

/**
 * @brief Destroy a mutex
 *
 * @param mutex Mutex to destroy
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 */
int AK24_mutex_destroy(AK24_MUTEX *mutex);

/**
 * @brief Lock a mutex
 *
 * @param mutex Mutex to lock
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 */
int AK24_mutex_lock(AK24_MUTEX *mutex);

/**
 * @brief Unlock a mutex
 *
 * @param mutex Mutex to unlock
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 */
int AK24_mutex_unlock(AK24_MUTEX *mutex);

/**
 * @brief Initialize a condition variable at runtime
 *
 * @param cond Condition variable to initialize
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 */
int AK24_cond_init(AK24_COND *cond);

/**
 * @brief Destroy a condition variable
 *
 * @param cond Condition variable to destroy
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 */
int AK24_cond_destroy(AK24_COND *cond);

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
int AK24_cond_wait(AK24_COND *cond, AK24_MUTEX *mutex);

/**
 * @brief Wait on a condition variable with timeout
 *
 * Atomically unlocks mutex and waits on condition variable
 * with a timeout. Reacquires mutex before returning.
 *
 * @param cond Condition variable to wait on
 * @param mutex Mutex associated with condition variable
 * @param timeout_ms Timeout in milliseconds
 * @return 0 on signal, 1 on timeout, -1 on error
 *
 * @threadsafe
 */
int AK24_cond_timedwait(AK24_COND *cond, AK24_MUTEX *mutex,
                        uint32_t timeout_ms);

/**
 * @brief Signal one thread waiting on condition variable
 *
 * @param cond Condition variable to signal
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 */
int AK24_cond_signal(AK24_COND *cond);

/**
 * @brief Broadcast to all threads waiting on condition variable
 *
 * @param cond Condition variable to broadcast
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 */
int AK24_cond_broadcast(AK24_COND *cond);

/**
 * @brief Threading functions
 *
 * Platform and GC-agnostic thread creation, join, and detach operations.
 * Implementation handles platform differences and GC requirements.
 */

/**
 * @brief Create a new thread
 *
 * Platform-agnostic thread creation.
 * Automatically uses GC-aware thread creation when GC is enabled,
 * and platform-specific primitives (pthread on POSIX, Windows threads on
 * Windows).
 *
 * @param thread Pointer to thread handle
 * @param start_routine Thread entry point
 * @param arg Argument to pass to thread
 * @return 0 on success, non-zero on failure
 *
 * @threadsafe
 */
int AK24_THREAD_CREATE(AK24_THREAD *thread, void *(*start_routine)(void *),
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
int AK24_THREAD_JOIN(AK24_THREAD thread);

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
int AK24_THREAD_DETACH(AK24_THREAD thread);

/**
 * @brief Register shutdown callback
 */
void ak_on_shutdown(ak_lambda_t *lambda);

/**
 * @brief Convert argc/argv to list
 */
list_str_t ak_args_to_list(int argc, char **argv);

/**
 * @brief Register a signal handler
 *
 * Registers an AK lambda to handle a specific signal.
 *
 * @param signum Signal number to handle (e.g., SIGINT, SIGTERM)
 * @param handler Lambda function to call when signal is raised
 *
 * @threadsafe
 *
 * @note The handler will be called in signal context
 *
 * Example:
 * @code
 * ak_lambda_t *handler = ...;
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
