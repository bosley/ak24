/*
 * Copyright (c) 2020 rxi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "log.h"
#include <string.h>

#define AK24_LOG_MAX_CALLBACKS 32

typedef struct {
  ak_log_fn_t fn;
  void *udata;
  ak_log_level_t level;
} Callback;

static struct {
  void *udata;
  ak_log_lock_fn_t lock;
  ak_log_level_t level;
  bool quiet;
  bool use_color;
  ak_log_path_format_t path_format;
  Callback callbacks[AK24_LOG_MAX_CALLBACKS];
} L;

static const char *level_strings[] = {"TRACE", "DEBUG", "INFO",
                                      "WARN",  "ERROR", "FATAL"};

static const char *level_colors[] = {"\x1b[94m", "\x1b[36m", "\x1b[32m",
                                     "\x1b[33m", "\x1b[31m", "\x1b[35m"};

static const char *format_file_path(const char *file) {
  if (L.path_format == AK24_LOG_PATH_ABBREV) {
    const char *last_slash = strrchr(file, '/');
    if (last_slash) {
      return last_slash + 1;
    }
  }
  return file;
}

static void stdout_callback(ak_log_event_t *ev) {
  char buf[16];
  buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';
  const char *file_display = format_file_path(ev->file);
  if (L.use_color) {
    fprintf(ev->udata, "%s %s%-5s\x1b[0m \x1b[90m%s:%zu:\x1b[0m ", buf,
            level_colors[ev->level], level_strings[ev->level], file_display,
            ev->line);
  } else {
    fprintf(ev->udata, "%s %-5s %s:%zu: ", buf, level_strings[ev->level],
            file_display, ev->line);
  }
  vfprintf(ev->udata, ev->fmt, ev->ap);
  fprintf(ev->udata, "\n");
  fflush(ev->udata);
}

static void file_callback(ak_log_event_t *ev) {
  char buf[64];
  buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ev->time)] = '\0';
  const char *file_display = format_file_path(ev->file);
  fprintf(ev->udata, "%s %-5s %s:%zu: ", buf, level_strings[ev->level],
          file_display, ev->line);
  vfprintf(ev->udata, ev->fmt, ev->ap);
  fprintf(ev->udata, "\n");
  fflush(ev->udata);
}

static void lock(void) {
  if (L.lock) {
    L.lock(true, L.udata);
  }
}

static void unlock(void) {
  if (L.lock) {
    L.lock(false, L.udata);
  }
}

const char *ak_log_level_string(ak_log_level_t level) {
  return level_strings[level];
}

void ak_log_set_lock(ak_log_lock_fn_t fn, void *udata) {
  L.lock = fn;
  L.udata = udata;
}

void ak_log_set_level(ak_log_level_t level) { L.level = level; }

void ak_log_set_quiet(bool enable) { L.quiet = enable; }

void ak_log_set_color(bool enable) { L.use_color = enable; }

void ak_log_set_path_format(ak_log_path_format_t format) {
  L.path_format = format;
}

int ak_log_add_callback(ak_log_fn_t fn, void *udata, ak_log_level_t level) {
  for (size_t i = 0; i < AK24_LOG_MAX_CALLBACKS; i++) {
    if (!L.callbacks[i].fn) {
      L.callbacks[i] = (Callback){fn, udata, level};
      return 0;
    }
  }
  return -1;
}

int ak_log_add_fp(FILE *fp, ak_log_level_t level) {
  return ak_log_add_callback(file_callback, fp, level);
}

static void init_event(ak_log_event_t *ev, void *udata) {
  if (!ev->time) {
    time_t t = time(NULL);
    ev->time = localtime(&t);
  }
  ev->udata = udata;
}

void ak_log_log(ak_log_level_t level, const char *file, size_t line,
                const char *fmt, ...) {
  ak_log_event_t ev = {
      .fmt = fmt,
      .file = file,
      .line = line,
      .level = level,
  };

  lock();

  if (!L.quiet && level >= L.level) {
    init_event(&ev, stderr);
    va_start(ev.ap, fmt);
    stdout_callback(&ev);
    va_end(ev.ap);
  }

  for (size_t i = 0; i < AK24_LOG_MAX_CALLBACKS && L.callbacks[i].fn; i++) {
    Callback *cb = &L.callbacks[i];
    if (level >= cb->level) {
      init_event(&ev, cb->udata);
      va_start(ev.ap, fmt);
      cb->fn(&ev);
      va_end(ev.ap);
    }
  }

  unlock();
}
