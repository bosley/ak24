/**
 * @file log.h
 * @brief Thread-safe logging system with multiple output targets and levels
 *
 * Provides a flexible logging framework supporting multiple severity levels,
 * custom callbacks, file output, and optional thread synchronization. Based
 * on rxi's log.c library, modified for AK24 kernel integration.
 *
 * Key features:
 * - Six severity levels from TRACE to FATAL
 * - Multiple simultaneous output targets
 * - Custom callback support for log processing
 * - Optional color output for terminals
 * - Thread-safe with user-provided locking
 * - Configurable path formatting
 *
 * @note Thread safety requires user to provide lock function via
 * ak_log_set_lock()
 * @see https://github.com/rxi/log.c
 */

/**
 * Copyright (c) 2020 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `log.c` for details.
 * - Modified for AK24 by bosley 2025
 */

#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define LOG_VERSION "0.0.1-dev"

/**
 * @brief Log severity levels
 *
 * Ordered from most verbose (TRACE) to most severe (FATAL).
 * Use ak_log_set_level() to filter messages below a threshold.
 */
typedef enum {
  AK24_LOG_LEVEL_TRACE, /**< Detailed trace information */
  AK24_LOG_LEVEL_DEBUG, /**< Debug information */
  AK24_LOG_LEVEL_INFO,  /**< Informational messages */
  AK24_LOG_LEVEL_WARN,  /**< Warning messages */
  AK24_LOG_LEVEL_ERROR, /**< Error messages */
  AK24_LOG_LEVEL_FATAL  /**< Fatal error messages */
} ak_log_level_t;

/**
 * @brief File path formatting options
 */
typedef enum {
  AK24_LOG_PATH_FULL,  /**< Show full file path */
  AK24_LOG_PATH_ABBREV /**< Show abbreviated file path */
} ak_log_path_format_t;

/**
 * @brief Log event structure passed to callbacks
 *
 * Contains all information about a log event including timestamp,
 * source location, severity, and formatted message.
 */
typedef struct {
  va_list ap;       /**< Variable argument list for formatting */
  const char *fmt;  /**< Format string */
  const char *file; /**< Source file name */
  struct tm *time;  /**< Timestamp of log event */
  void *udata;      /**< User data from callback registration */
  size_t line;      /**< Source line number */
  int level;        /**< Log level (ak_log_level_t) */
} ak_log_event_t;

/**
 * @brief Callback function type for custom log handlers
 *
 * @param ev Log event containing message and metadata
 */
typedef void (*ak_log_fn_t)(ak_log_event_t *ev);

/**
 * @brief Lock function type for thread synchronization
 *
 * @param lock True to acquire lock, false to release
 * @param udata User data provided during lock registration
 */
typedef void (*ak_log_lock_fn_t)(bool lock, void *udata);

/**
 * @def AK24_LOG_TRACE
 * @brief Log a trace-level message with automatic file and line information
 */
#define AK24_LOG_TRACE(...)                                                    \
  ak_log_log(AK24_LOG_LEVEL_TRACE, __FILE__, __LINE__, __VA_ARGS__)

/**
 * @def AK24_LOG_DEBUG
 * @brief Log a debug-level message with automatic file and line information
 */
#define AK24_LOG_DEBUG(...)                                                    \
  ak_log_log(AK24_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)

/**
 * @def AK24_LOG_INFO
 * @brief Log an info-level message with automatic file and line information
 */
#define AK24_LOG_INFO(...)                                                     \
  ak_log_log(AK24_LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)

/**
 * @def AK24_LOG_WARN
 * @brief Log a warning-level message with automatic file and line information
 */
#define AK24_LOG_WARN(...)                                                     \
  ak_log_log(AK24_LOG_LEVEL_WARN, __FILE__, __LINE__, __VA_ARGS__)

/**
 * @def AK24_LOG_ERROR
 * @brief Log an error-level message with automatic file and line information
 */
#define AK24_LOG_ERROR(...)                                                    \
  ak_log_log(AK24_LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)

/**
 * @def AK24_LOG_FATAL
 * @brief Log a fatal-level message with automatic file and line information
 */
#define AK24_LOG_FATAL(...)                                                    \
  ak_log_log(AK24_LOG_LEVEL_FATAL, __FILE__, __LINE__, __VA_ARGS__)

/**
 * @brief Get string representation of log level
 *
 * @param level Log level to convert
 * @return String name of level (e.g., "TRACE", "DEBUG", "INFO")
 *
 * @threadsafe
 */
const char *ak_log_level_string(ak_log_level_t level);

/**
 * @brief Set lock function for thread-safe logging
 *
 * Provides a locking mechanism for thread-safe log operations. The lock
 * function will be called with true before logging and false after.
 *
 * @param fn Lock function to call, or NULL to disable locking
 * @param udata User data passed to lock function
 *
 * @notthreadsafe
 *
 * @par Example:
 * @code
 * pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
 *
 * void log_lock(bool lock, void *udata) {
 *   pthread_mutex_t *mtx = (pthread_mutex_t *)udata;
 *   if (lock) {
 *     pthread_mutex_lock(mtx);
 *   } else {
 *     pthread_mutex_unlock(mtx);
 *   }
 * }
 *
 * ak_log_set_lock(log_lock, &log_mutex);
 * @endcode
 */
void ak_log_set_lock(ak_log_lock_fn_t fn, void *udata);

/**
 * @brief Set minimum log level threshold
 *
 * Messages below this level will be filtered out and not logged.
 *
 * @param level Minimum level to log
 *
 * @notthreadsafe
 */
void ak_log_set_level(ak_log_level_t level);

/**
 * @brief Enable or disable console output
 *
 * When quiet mode is enabled, log messages are not printed to stdout.
 * Callbacks and file outputs are unaffected.
 *
 * @param enable True to suppress console output, false to enable
 *
 * @notthreadsafe
 */
void ak_log_set_quiet(bool enable);

/**
 * @brief Enable or disable color output
 *
 * When enabled, console output uses ANSI color codes for different
 * log levels. Has no effect when quiet mode is enabled.
 *
 * @param enable True to enable colors, false to disable
 *
 * @notthreadsafe
 */
void ak_log_set_color(bool enable);

/**
 * @brief Set file path formatting style
 *
 * Controls how file paths are displayed in log output.
 *
 * @param format Path format (full or abbreviated)
 *
 * @notthreadsafe
 */
void ak_log_set_path_format(ak_log_path_format_t format);

/**
 * @brief Add custom callback for log processing
 *
 * Registers a callback function to receive log events. Multiple callbacks
 * can be registered. Each callback can specify its own minimum level.
 *
 * @param fn Callback function to invoke for log events
 * @param udata User data passed to callback
 * @param level Minimum level for this callback
 * @return 0 on success, -1 on failure
 *
 * @notthreadsafe
 *
 * @par Example:
 * @code
 * void my_log_handler(ak_log_event_t *ev) {
 *   if (ev->level >= AK24_LOG_LEVEL_ERROR) {
 *     send_to_monitoring_system(ev);
 *   }
 * }
 *
 * ak_log_add_callback(my_log_handler, NULL, AK24_LOG_LEVEL_ERROR);
 * @endcode
 */
int ak_log_add_callback(ak_log_fn_t fn, void *udata, ak_log_level_t level);

/**
 * @brief Add file output target
 *
 * Registers a file pointer to receive log output. Multiple file outputs
 * can be registered. Each can specify its own minimum level.
 *
 * @param fp File pointer (must remain valid)
 * @param level Minimum level for this output
 * @return 0 on success, -1 on failure
 *
 * @notthreadsafe
 *
 * @note Caller is responsible for opening and closing the file
 *
 * @par Example:
 * @code
 * FILE *log_file = fopen("app.log", "a");
 * if (log_file) {
 *   ak_log_add_fp(log_file, AK24_LOG_LEVEL_INFO);
 * }
 * @endcode
 */
int ak_log_add_fp(FILE *fp, ak_log_level_t level);

/**
 * @brief Core logging function
 *
 * Processes a log message at the specified level. Typically called via
 * convenience macros (AK24_LOG_INFO, etc.) rather than directly.
 *
 * @param level Log severity level
 * @param file Source file name
 * @param line Source line number
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 *
 * @threadsafe (if lock function is set via ak_log_set_lock)
 */
void ak_log_log(ak_log_level_t level, const char *file, size_t line,
                const char *fmt, ...);

#endif
