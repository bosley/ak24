/**
 * @file thread_platform.h
 * @brief Platform abstraction layer for threading primitives
 *
 * Provides a unified interface for threading operations across POSIX and
 * Windows platforms. This abstraction allows the thread pool implementation
 * to remain platform-agnostic.
 *
 * Supported platforms:
 * - POSIX (Linux, macOS, BSD) - Fully implemented
 * - Windows - Prepared structure, implementation pending
 */

#ifndef AK24_THREAD_PLATFORM_H
#define AK24_THREAD_PLATFORM_H

#include <stddef.h>

/**
 * @brief Platform detection
 */
#if defined(_WIN32) || defined(_WIN64)
#define AK24_PLATFORM_WINDOWS
#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__) || defined(__linux__)
#define AK24_PLATFORM_POSIX
#include <pthread.h>
#else
#error "Unsupported platform - only POSIX and Windows are supported"
#endif

/**
 * @brief Platform-agnostic mutex type
 */
typedef struct {
#ifdef AK24_PLATFORM_POSIX
  pthread_mutex_t handle; /**< POSIX mutex handle */
#else
  CRITICAL_SECTION handle; /**< Windows critical section handle */
#endif
} ak_mutex_t;

/**
 * @brief Platform-agnostic condition variable type
 */
typedef struct {
#ifdef AK24_PLATFORM_POSIX
  pthread_cond_t handle; /**< POSIX condition variable handle */
#else
  CONDITION_VARIABLE handle; /**< Windows condition variable handle */
#endif
} ak_cond_t;

/**
 * @brief Platform-agnostic thread type
 */
typedef struct {
#ifdef AK24_PLATFORM_POSIX
  pthread_t handle; /**< POSIX thread handle */
#else
  HANDLE handle; /**< Windows thread handle */
#endif
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
