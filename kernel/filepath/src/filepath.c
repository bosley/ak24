#include "filepath.h"
#include "kernel.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef AK24_PLATFORM_WINDOWS
#include <shlobj.h>
#include <windows.h>
#define PATH_MAX 260
#else
#include <limits.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#endif

/**
 * @brief Global filepath state
 */
typedef struct {
  bool initialized;
  char separator;
  char list_separator;
} filepath_state_t;

static filepath_state_t g_filepath_state = {0};

void ak_filepath_init(void) {
  if (g_filepath_state.initialized) {
    return;
  }

#ifdef AK24_PLATFORM_WINDOWS
  g_filepath_state.separator = '\\';
  g_filepath_state.list_separator = ';';
#else
  g_filepath_state.separator = '/';
  g_filepath_state.list_separator = ':';
#endif

  g_filepath_state.initialized = true;
}

void ak_filepath_shutdown(void) { g_filepath_state.initialized = false; }

char ak_filepath_separator(void) { return g_filepath_state.separator; }

char ak_filepath_list_separator(void) {
  return g_filepath_state.list_separator;
}

/**
 * @brief Check if character is a path separator
 */
static bool is_separator(char c) {
#ifdef AK24_PLATFORM_WINDOWS
  return c == '/' || c == '\\';
#else
  return c == '/';
#endif
}

/**
 * @brief Create buffer from C string
 */
static ak_buffer_t *buffer_from_str(const char *str) {
  if (!str) {
    return NULL;
  }
  size_t len = strlen(str);
  ak_buffer_t *buf = ak_buffer_new(len + 1);
  if (!buf) {
    return NULL;
  }
  if (ak_buffer_copy_to(buf, (uint8_t *)str, len) != 0) {
    ak_buffer_free(buf);
    return NULL;
  }
  return buf;
}

/**
 * @brief Append C string to buffer
 */
static int buffer_append_str(ak_buffer_t *buf, const char *str) {
  if (!buf || !str) {
    return -1;
  }
  size_t len = strlen(str);
  size_t old_count = ak_buffer_count(buf);
  uint8_t *data = ak_buffer_data(buf);

  // Ensure capacity
  if (buf->capacity < old_count + len) {
    size_t new_capacity = (old_count + len) * 2;
    uint8_t *new_data = AK24_ALLOC_ATOMIC(new_capacity);
    if (!new_data) {
      return -1;
    }
    memcpy(new_data, data, old_count);
    AK24_FREE(data);
    buf->data = new_data;
    buf->capacity = new_capacity;
    data = buf->data;
  }

  memcpy(data + old_count, str, len);
  buf->count = old_count + len;
  return 0;
}

/**
 * @brief Append character to buffer
 */
static int buffer_append_char(ak_buffer_t *buf, char c) {
  char str[2] = {c, '\0'};
  return buffer_append_str(buf, str);
}

/**
 * @brief Get null-terminated string from buffer
 */
static const char *buffer_to_cstr(ak_buffer_t *buf) {
  if (!buf) {
    return NULL;
  }

  uint8_t *data = ak_buffer_data(buf);
  size_t count = ak_buffer_count(buf);

  // Ensure null termination
  if (buf->capacity < count + 1) {
    uint8_t *new_data = AK24_ALLOC_ATOMIC(count + 1);
    if (!new_data) {
      return NULL;
    }
    memcpy(new_data, data, count);
    AK24_FREE(data);
    buf->data = new_data;
    buf->capacity = count + 1;
    data = buf->data;
  }

  data[count] = '\0';
  return (const char *)data;
}

ak_buffer_t *ak_filepath_join(size_t count, ...) {
  if (!g_filepath_state.initialized) {
    return NULL;
  }

  ak_buffer_t *result = ak_buffer_new(256);
  if (!result) {
    return NULL;
  }

  va_list args;
  va_start(args, count);

  for (size_t i = 0; i < count; i++) {
    const char *component = va_arg(args, const char *);

    // Skip NULL or empty components
    if (!component || !*component) {
      continue;
    }

    size_t result_count = ak_buffer_count(result);
    uint8_t *result_data = ak_buffer_data(result);

    // If result is empty, just add the component
    if (result_count == 0) {
      buffer_append_str(result, component);
      continue;
    }

    // Check if we need to add a separator
    char last = result_data[result_count - 1];
    bool needs_sep = !is_separator(last);
    bool component_starts_with_sep = is_separator(component[0]);

    if (needs_sep && !component_starts_with_sep) {
      buffer_append_char(result, g_filepath_state.separator);
    } else if (!needs_sep && component_starts_with_sep) {
      // Skip the leading separator in component
      component++;
    }

    buffer_append_str(result, component);
  }

  va_end(args);

  return result;
}

ak_buffer_t *ak_filepath_normalize(const char *path) {
  if (!g_filepath_state.initialized || !path) {
    return NULL;
  }

  // Split path by separators
  list_str_t components;
  list_init(&components);

  bool is_absolute = ak_filepath_is_absolute(path);

  // Parse path components
  const char *p = path;
  char component_buf[PATH_MAX];
  size_t comp_idx = 0;

  while (*p) {
    if (is_separator(*p)) {
      if (comp_idx > 0) {
        component_buf[comp_idx] = '\0';

        // Handle . and ..
        if (strcmp(component_buf, ".") == 0) {
          // Skip current directory
        } else if (strcmp(component_buf, "..") == 0) {
          // Go up one level
          if (list_count(&components) > 0) {
            char **last = list_pop(&components);
            if (last && *last) {
              AK24_FREE(*last);
            }
          }
        } else {
          // Add normal component
          char *comp_copy = AK24_ALLOC_ATOMIC(comp_idx + 1);
          if (comp_copy) {
            strcpy(comp_copy, component_buf);
            list_push(&components, comp_copy);
          }
        }

        comp_idx = 0;
      }
      p++;
    } else {
      if (comp_idx < PATH_MAX - 1) {
        component_buf[comp_idx++] = *p;
      }
      p++;
    }
  }

  // Handle last component
  if (comp_idx > 0) {
    component_buf[comp_idx] = '\0';

    if (strcmp(component_buf, ".") != 0) {
      if (strcmp(component_buf, "..") == 0) {
        if (list_count(&components) > 0) {
          char **last = list_pop(&components);
          if (last && *last) {
            AK24_FREE(*last);
          }
        }
      } else {
        char *comp_copy = AK24_ALLOC_ATOMIC(comp_idx + 1);
        if (comp_copy) {
          strcpy(comp_copy, component_buf);
          list_push(&components, comp_copy);
        }
      }
    }
  }

  // Build result
  ak_buffer_t *result = ak_buffer_new(256);
  if (!result) {
    // Cleanup components
    list_iter_t iter = list_iter(&components);
    char **comp;
    while ((comp = list_next(&components, &iter))) {
      if (*comp) {
        AK24_FREE(*comp);
      }
    }
    list_deinit(&components);
    return NULL;
  }

  // Add root if absolute
  if (is_absolute) {
    buffer_append_char(result, g_filepath_state.separator);
  }

  // Add components
  list_iter_t iter = list_iter(&components);
  char **comp;
  bool first = true;

  while ((comp = list_next(&components, &iter))) {
    if (*comp) {
      if (!first) {
        buffer_append_char(result, g_filepath_state.separator);
      }
      buffer_append_str(result, *comp);
      first = false;
      AK24_FREE(*comp);
    }
  }

  list_deinit(&components);

  // If result is empty and wasn't absolute, return "."
  if (ak_buffer_count(result) == 0 && !is_absolute) {
    buffer_append_str(result, ".");
  }

  return result;
}

bool ak_filepath_is_absolute(const char *path) {
  if (!path || !*path) {
    return false;
  }

#ifdef AK24_PLATFORM_WINDOWS
  // Windows absolute paths:
  // - Drive letter: C:\ or C:/
  // - UNC path: \\server\share or //server/share
  if (strlen(path) >= 2) {
    // Check for drive letter
    if (((path[0] >= 'A' && path[0] <= 'Z') ||
         (path[0] >= 'a' && path[0] <= 'z')) &&
        path[1] == ':') {
      return true;
    }
    // Check for UNC path
    if (is_separator(path[0]) && is_separator(path[1])) {
      return true;
    }
  }
  return false;
#else
  // POSIX: starts with /
  return path[0] == '/';
#endif
}

ak_buffer_t *ak_filepath_basename(const char *path) {
  if (!g_filepath_state.initialized || !path || !*path) {
    return NULL;
  }

  const char *last_sep = NULL;
  const char *p = path;

  // Find last separator
  while (*p) {
    if (is_separator(*p)) {
      last_sep = p;
    }
    p++;
  }

  // If no separator found, the whole path is the basename
  if (!last_sep) {
    return buffer_from_str(path);
  }

  // Return everything after the last separator
  return buffer_from_str(last_sep + 1);
}

ak_buffer_t *ak_filepath_dirname(const char *path) {
  if (!g_filepath_state.initialized || !path || !*path) {
    return NULL;
  }

  const char *last_sep = NULL;
  const char *p = path;

  // Find last separator
  while (*p) {
    if (is_separator(*p)) {
      last_sep = p;
    }
    p++;
  }

  // If no separator found, return "."
  if (!last_sep) {
    return buffer_from_str(".");
  }

  // If separator is at the start, return root
  if (last_sep == path) {
    char sep_str[2] = {g_filepath_state.separator, '\0'};
    return buffer_from_str(sep_str);
  }

  // Return everything before the last separator
  size_t len = last_sep - path;
  ak_buffer_t *buf = ak_buffer_new(len + 1);
  if (!buf) {
    return NULL;
  }
  ak_buffer_copy_to(buf, (uint8_t *)path, len);
  return buf;
}

ak_buffer_t *ak_filepath_extension(const char *path) {
  if (!g_filepath_state.initialized || !path || !*path) {
    return NULL;
  }

  // Get basename first
  ak_buffer_t *base = ak_filepath_basename(path);
  if (!base) {
    return NULL;
  }

  const char *base_str = buffer_to_cstr(base);
  if (!base_str) {
    ak_buffer_free(base);
    return NULL;
  }

  // Find last dot
  const char *last_dot = NULL;
  const char *p = base_str;

  while (*p) {
    if (*p == '.') {
      last_dot = p;
    }
    p++;
  }

  // No dot found, or dot is at the start (hidden file with no extension)
  if (!last_dot || last_dot == base_str) {
    ak_buffer_free(base);
    return NULL;
  }

  // Return extension including the dot
  ak_buffer_t *ext = buffer_from_str(last_dot);
  ak_buffer_free(base);
  return ext;
}

ak_buffer_t *ak_filepath_home(void) {
  if (!g_filepath_state.initialized) {
    return NULL;
  }

#ifdef AK24_PLATFORM_WINDOWS
  // Try USERPROFILE first
  char *userprofile = getenv("USERPROFILE");
  if (userprofile && *userprofile) {
    return buffer_from_str(userprofile);
  }

  // Try HOMEDRIVE + HOMEPATH
  char *homedrive = getenv("HOMEDRIVE");
  char *homepath = getenv("HOMEPATH");
  if (homedrive && homepath) {
    return ak_filepath_join(2, homedrive, homepath);
  }

  return NULL;
#else
  // Try HOME environment variable first
  char *home = getenv("HOME");
  if (home && *home) {
    return buffer_from_str(home);
  }

  // Fall back to passwd entry
  struct passwd *pw = getpwuid(getuid());
  if (pw && pw->pw_dir) {
    return buffer_from_str(pw->pw_dir);
  }

  return NULL;
#endif
}

ak_buffer_t *ak_filepath_cache(void) {
  if (!g_filepath_state.initialized) {
    return NULL;
  }

#ifdef AK24_PLATFORM_WINDOWS
  // Windows: LOCALAPPDATA
  char *localappdata = getenv("LOCALAPPDATA");
  if (localappdata && *localappdata) {
    return buffer_from_str(localappdata);
  }
  return NULL;
#elif defined(AK24_PLATFORM_APPLE)
  // macOS: ~/Library/Caches
  ak_buffer_t *home = ak_filepath_home();
  if (!home) {
    return NULL;
  }
  ak_buffer_t *cache =
      ak_filepath_join(2, buffer_to_cstr(home), "Library/Caches");
  ak_buffer_free(home);
  return cache;
#else
  // Linux/Unix: XDG_CACHE_HOME or ~/.cache
  char *xdg_cache = getenv("XDG_CACHE_HOME");
  if (xdg_cache && *xdg_cache) {
    return buffer_from_str(xdg_cache);
  }

  ak_buffer_t *home = ak_filepath_home();
  if (!home) {
    return NULL;
  }
  ak_buffer_t *cache = ak_filepath_join(2, buffer_to_cstr(home), ".cache");
  ak_buffer_free(home);
  return cache;
#endif
}

ak_buffer_t *ak_filepath_config(void) {
  if (!g_filepath_state.initialized) {
    return NULL;
  }

#ifdef AK24_PLATFORM_WINDOWS
  // Windows: APPDATA
  char *appdata = getenv("APPDATA");
  if (appdata && *appdata) {
    return buffer_from_str(appdata);
  }
  return NULL;
#elif defined(AK24_PLATFORM_APPLE)
  // macOS: ~/Library/Application Support
  ak_buffer_t *home = ak_filepath_home();
  if (!home) {
    return NULL;
  }
  ak_buffer_t *config =
      ak_filepath_join(2, buffer_to_cstr(home), "Library/Application Support");
  ak_buffer_free(home);
  return config;
#else
  // Linux/Unix: XDG_CONFIG_HOME or ~/.config
  char *xdg_config = getenv("XDG_CONFIG_HOME");
  if (xdg_config && *xdg_config) {
    return buffer_from_str(xdg_config);
  }

  ak_buffer_t *home = ak_filepath_home();
  if (!home) {
    return NULL;
  }
  ak_buffer_t *config = ak_filepath_join(2, buffer_to_cstr(home), ".config");
  ak_buffer_free(home);
  return config;
#endif
}

ak_buffer_t *ak_filepath_data(void) {
  if (!g_filepath_state.initialized) {
    return NULL;
  }

#ifdef AK24_PLATFORM_WINDOWS
  // Windows: LOCALAPPDATA
  char *localappdata = getenv("LOCALAPPDATA");
  if (localappdata && *localappdata) {
    return buffer_from_str(localappdata);
  }
  return NULL;
#elif defined(AK24_PLATFORM_APPLE)
  // macOS: ~/Library/Application Support (same as config)
  return ak_filepath_config();
#else
  // Linux/Unix: XDG_DATA_HOME or ~/.local/share
  char *xdg_data = getenv("XDG_DATA_HOME");
  if (xdg_data && *xdg_data) {
    return buffer_from_str(xdg_data);
  }

  ak_buffer_t *home = ak_filepath_home();
  if (!home) {
    return NULL;
  }
  ak_buffer_t *data = ak_filepath_join(2, buffer_to_cstr(home), ".local/share");
  ak_buffer_free(home);
  return data;
#endif
}

ak_buffer_t *ak_filepath_temp(void) {
  if (!g_filepath_state.initialized) {
    return NULL;
  }

#ifdef AK24_PLATFORM_WINDOWS
  // Windows: TEMP or TMP
  char *temp = getenv("TEMP");
  if (temp && *temp) {
    return buffer_from_str(temp);
  }
  temp = getenv("TMP");
  if (temp && *temp) {
    return buffer_from_str(temp);
  }
  return buffer_from_str("C:\\Temp");
#else
  // POSIX: TMPDIR or /tmp
  char *tmpdir = getenv("TMPDIR");
  if (tmpdir && *tmpdir) {
    return buffer_from_str(tmpdir);
  }
  return buffer_from_str("/tmp");
#endif
}

ak_buffer_t *ak_filepath_cwd(void) {
  if (!g_filepath_state.initialized) {
    return NULL;
  }

  char *cwd = AK24_ALLOC_ATOMIC(PATH_MAX);
  if (!cwd) {
    return NULL;
  }

#ifdef AK24_PLATFORM_WINDOWS
  if (!GetCurrentDirectoryA(PATH_MAX, cwd)) {
    AK24_FREE(cwd);
    return NULL;
  }
#else
  if (!getcwd(cwd, PATH_MAX)) {
    AK24_FREE(cwd);
    return NULL;
  }
#endif

  ak_buffer_t *result = buffer_from_str(cwd);
  AK24_FREE(cwd);
  return result;
}

ak_buffer_t *ak_filepath_to_native(const char *path) {
  if (!g_filepath_state.initialized || !path) {
    return NULL;
  }

  ak_buffer_t *result = ak_buffer_new(strlen(path) + 1);
  if (!result) {
    return NULL;
  }

  const char *p = path;
  while (*p) {
    if (is_separator(*p)) {
      buffer_append_char(result, g_filepath_state.separator);
    } else {
      buffer_append_char(result, *p);
    }
    p++;
  }

  return result;
}
