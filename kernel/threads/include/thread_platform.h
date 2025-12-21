/**
 * @file thread_platform.h
 * @brief Platform abstraction layer for threading primitives
 *
 * Provides a unified interface for threading operations by wrapping
 * kernel-level thread primitives. This allows the thread pool implementation
 * to remain platform-agnostic.
 *
 * NOTE: This layer wraps kernel.h's AK24_ threading functions.
 * All platform (POSIX/Windows) and GC concerns are handled by the kernel.
 *
 * Supported platforms:
 * - POSIX (Linux, macOS, BSD) - Fully implemented
 * - Windows - Fully implemented (stubs in kernel)
 */

#ifndef AK24_THREAD_PLATFORM_H
#define AK24_THREAD_PLATFORM_H

#include "kernel.h"
#include <stddef.h>

/**
 * @brief Platform-agnostic mutex type
 *
 * Wraps kernel's AK24_MUTEX which handles platform differences.
 */
typedef struct ak_mutex_t {
  AK24_MUTEX mutex;
} ak_mutex_t;

/**
 * @brief Platform-agnostic condition variable type
 *
 * Wraps kernel's AK24_COND which handles platform differences.
 */
typedef struct ak_cond_t {
  AK24_COND cond;
} ak_cond_t;

/**
 * @brief Platform-agnostic thread type
 *
 * Wraps kernel's AK24_THREAD which handles platform differences.
 */
typedef struct ak_thread_t {
  AK24_THREAD thread;
} ak_thread_t;

/**
 * @brief Thread start routine signature
 */
typedef void *(*ak_thread_start_fn)(void *);

/**
 * @brief Initialize a mutex
 *
 * @param mutex Mutex to initialize
 * @return 0 on success, -1 on failure
 */
int ak_mutex_init(ak_mutex_t *mutex);

/**
 * @brief Destroy a mutex
 *
 * @param mutex Mutex to destroy
 * @return 0 on success, -1 on failure
 */
int ak_mutex_destroy(ak_mutex_t *mutex);

/**
 * @brief Lock a mutex
 *
 * @param mutex Mutex to lock
 * @return 0 on success, -1 on failure
 */
int ak_mutex_lock(ak_mutex_t *mutex);

/**
 * @brief Unlock a mutex
 *
 * @param mutex Mutex to unlock
 * @return 0 on success, -1 on failure
 */
int ak_mutex_unlock(ak_mutex_t *mutex);

/**
 * @brief Initialize a condition variable
 *
 * @param cond Condition variable to initialize
 * @return 0 on success, -1 on failure
 */
int ak_cond_init(ak_cond_t *cond);

/**
 * @brief Destroy a condition variable
 *
 * @param cond Condition variable to destroy
 * @return 0 on success, -1 on failure
 */
int ak_cond_destroy(ak_cond_t *cond);

/**
 * @brief Wait on a condition variable
 *
 * Atomically unlocks mutex and waits on condition variable.
 * Reacquires mutex before returning.
 *
 * @param cond Condition variable to wait on
 * @param mutex Mutex associated with condition variable
 * @return 0 on success, -1 on failure
 */
int ak_cond_wait(ak_cond_t *cond, ak_mutex_t *mutex);

/**
 * @brief Signal one thread waiting on condition variable
 *
 * @param cond Condition variable to signal
 * @return 0 on success, -1 on failure
 */
int ak_cond_signal(ak_cond_t *cond);

/**
 * @brief Broadcast to all threads waiting on condition variable
 *
 * @param cond Condition variable to broadcast
 * @return 0 on success, -1 on failure
 */
int ak_cond_broadcast(ak_cond_t *cond);

/**
 * @brief Create a new thread
 *
 * @param thread Thread handle to initialize
 * @param start_routine Function to execute in new thread
 * @param arg Argument to pass to start_routine
 * @return 0 on success, -1 on failure
 */
int ak_thread_create(ak_thread_t *thread, ak_thread_start_fn start_routine,
                     void *arg);

/**
 * @brief Wait for thread to terminate
 *
 * @param thread Thread to join
 * @return 0 on success, -1 on failure
 */
int ak_thread_join(ak_thread_t thread);

#endif // AK24_THREAD_PLATFORM_H
