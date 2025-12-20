/**
 * @file ak_nix.h
 * @brief POSIX/Unix platform-specific kernel definitions
 *
 * Platform-specific types, includes, and initializers for POSIX systems
 * (Linux, macOS, BSD, etc.). This file is conditionally included by kernel.h
 * when building for POSIX platforms.
 */

#ifndef AK24_AK_NIX_H
#define AK24_AK_NIX_H

#include <pthread.h>

/**
 * @brief Platform-agnostic mutex type (POSIX implementation)
 */
typedef struct {
  pthread_mutex_t handle;
} AK_MUTEX;

/**
 * @brief Platform-agnostic condition variable type (POSIX implementation)
 */
typedef struct {
  pthread_cond_t handle;
} AK_COND;

/**
 * @brief Platform-agnostic thread type (POSIX implementation)
 */
typedef struct {
  pthread_t handle;
} AK_THREAD;

/**
 * @def AK_MUTEX_INITIALIZER
 * @brief Static mutex initializer for POSIX
 */
#define AK_MUTEX_INITIALIZER {PTHREAD_MUTEX_INITIALIZER}

/**
 * @def AK_COND_INITIALIZER
 * @brief Static condition variable initializer for POSIX
 */
#define AK_COND_INITIALIZER {PTHREAD_COND_INITIALIZER}

#endif // AK24_AK_NIX_H
