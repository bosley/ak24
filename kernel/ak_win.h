/**
 * @file ak_win.h
 * @brief Windows platform-specific kernel definitions
 *
 * Platform-specific types, includes, and initializers for Windows systems.
 * This file is conditionally included by kernel.h when building for Windows.
 */

#ifndef AK24_AK_WIN_H
#define AK24_AK_WIN_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

/**
 * @brief Platform-agnostic mutex type (Windows implementation)
 */
typedef struct {
  CRITICAL_SECTION handle;
} AK24_MUTEX;

/**
 * @brief Platform-agnostic condition variable type (Windows implementation)
 */
typedef struct {
  CONDITION_VARIABLE handle;
} AK24_COND;

/**
 * @brief Platform-agnostic thread type (Windows implementation)
 */
typedef struct {
  HANDLE handle;
} AK24_THREAD;

/**
 * @def AK24_MUTEX_INITIALIZER
 * @brief Static mutex initializer for Windows
 */
#define AK24_MUTEX_INITIALIZER {0}

/**
 * @def AK24_COND_INITIALIZER
 * @brief Static condition variable initializer for Windows
 */
#define AK24_COND_INITIALIZER {0}

#endif // AK24_AK_WIN_H
