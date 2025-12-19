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

#define AK24_LOG_VERSION "0.1.0"

typedef enum {
  AK24_LOG_LEVEL_TRACE,
  AK24_LOG_LEVEL_DEBUG,
  AK24_LOG_LEVEL_INFO,
  AK24_LOG_LEVEL_WARN,
  AK24_LOG_LEVEL_ERROR,
  AK24_LOG_LEVEL_FATAL
} ak_log_level_t;

typedef enum { AK24_LOG_PATH_FULL, AK24_LOG_PATH_ABBREV } ak_log_path_format_t;

typedef struct {
  va_list ap;
  const char *fmt;
  const char *file;
  struct tm *time;
  void *udata;
  size_t line;
  int level;
} ak_log_event_t;

typedef void (*ak_log_fn_t)(ak_log_event_t *ev);
typedef void (*ak_log_lock_fn_t)(bool lock, void *udata);

#define AK24_LOG_TRACE(...)                                                    \
  ak_log_log(AK24_LOG_LEVEL_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define AK24_LOG_DEBUG(...)                                                    \
  ak_log_log(AK24_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define AK24_LOG_INFO(...)                                                     \
  ak_log_log(AK24_LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define AK24_LOG_WARN(...)                                                     \
  ak_log_log(AK24_LOG_LEVEL_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define AK24_LOG_ERROR(...)                                                    \
  ak_log_log(AK24_LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define AK24_LOG_FATAL(...)                                                    \
  ak_log_log(AK24_LOG_LEVEL_FATAL, __FILE__, __LINE__, __VA_ARGS__)

const char *ak_log_level_string(ak_log_level_t level);
void ak_log_set_lock(ak_log_lock_fn_t fn, void *udata);
void ak_log_set_level(ak_log_level_t level);
void ak_log_set_quiet(bool enable);
void ak_log_set_color(bool enable);
void ak_log_set_path_format(ak_log_path_format_t format);
int ak_log_add_callback(ak_log_fn_t fn, void *udata, ak_log_level_t level);
int ak_log_add_fp(FILE *fp, ak_log_level_t level);

void ak_log_log(ak_log_level_t level, const char *file, size_t line,
                const char *fmt, ...);

#endif
