# Filepath Module

## Overview

The filepath module provides cross-platform file path manipulation and system directory access. It abstracts away the differences between Windows (backslash `\`) and POSIX (forward slash `/`) path separators, providing a clean API for working with file system paths without worrying about platform-specific details.

## Key Features

- **Cross-Platform Path Handling**: Automatic separator handling for Windows and POSIX
- **Safe Path Joining**: Correctly joins path components with proper separators
- **Path Normalization**: Resolves `.` and `..`, removes redundant separators
- **System Directories**: Platform-aware access to home, cache, config, data, and temp directories
- **Path Component Extraction**: Get basename, dirname, and file extensions
- **Absolute/Relative Detection**: Determine if paths are absolute or relative
- **UTF-8 Encoding**: Paths are UTF-8 encoded on all platforms

## Platform Support

The module automatically adapts to the host platform:

- **Windows**: Uses backslash `\` separators, handles drive letters (`C:\`) and UNC paths (`\\server\share`)
- **Linux**: Uses forward slash `/` separators, follows XDG Base Directory Specification
- **macOS**: Uses forward slash `/` separators, follows macOS conventions for system directories
- **Other UNIX**: Uses forward slash `/` separators

## API Reference

### Initialization

```c
void ak_filepath_init(void);
void ak_filepath_shutdown(void);
```

Initialize and shutdown the filepath system. Called automatically by `ak_kernel_init("my-app")` and `ak_kernel_deinit()` - users don't need to call these directly.

### Platform Information

```c
char ak_filepath_separator(void);
char ak_filepath_list_separator(void);
```

Get the platform-specific path separator (`/` or `\`) and path list separator (`:` or `;` used in `PATH` environment variables).

### Path Joining

```c
ak_buffer_t *ak_filepath_join(size_t count, ...);
```

Join multiple path components with correct separators. Automatically handles trailing and leading separators, avoiding double separators.

```c
// Join paths regardless of platform
ak_buffer_t *path = ak_filepath_join(3, "/home", "user", "file.txt");
// Result on Linux/macOS: "/home/user/file.txt"
// Result on Windows: "\home\user\file.txt"
// Access string: const char *str = (const char *)ak_buffer_data(path);
ak_buffer_free(path);

// Handles mixed separators correctly
ak_buffer_t *path2 = ak_filepath_join(3, "/home/", "user/", "file.txt");
// Result: "/home/user/file.txt" (no double slashes)
ak_buffer_free(path2);
```

### Path Normalization

```c
ak_buffer_t *ak_filepath_normalize(const char *path);
```

Normalize a path by resolving `.` (current directory) and `..` (parent directory) components, removing redundant separators, and converting to platform-appropriate separators.

```c
ak_buffer_t *norm = ak_filepath_normalize("/home/user/../other/./file.txt");
// Result: "/home/other/file.txt"
ak_buffer_free(norm);

ak_buffer_t *norm2 = ak_filepath_normalize("./src/../include/header.h");
// Result: "include/header.h"
ak_buffer_free(norm2);
```

### Path Properties

```c
bool ak_filepath_is_absolute(const char *path);
```

Determine if a path is absolute. On POSIX, absolute paths start with `/`. On Windows, they start with a drive letter (`C:\`) or UNC prefix (`\\server\share`).

```c
bool abs1 = ak_filepath_is_absolute("/home/user");     // true on POSIX
bool abs2 = ak_filepath_is_absolute("C:\\Users");      // true on Windows
bool abs3 = ak_filepath_is_absolute("relative/path");  // false
```

### Path Component Extraction

```c
ak_buffer_t *ak_filepath_basename(const char *path);
ak_buffer_t *ak_filepath_dirname(const char *path);
ak_buffer_t *ak_filepath_extension(const char *path);
```

Extract components from paths:

- **basename**: The final component (filename with extension)
- **dirname**: The directory portion (everything except basename)
- **extension**: The file extension (including the dot)

```c
const char *path = "/home/user/document.txt";

ak_buffer_t *base = ak_filepath_basename(path);
// Result: "document.txt"
ak_buffer_free(base);

ak_buffer_t *dir = ak_filepath_dirname(path);
// Result: "/home/user"
ak_buffer_free(dir);

ak_buffer_t *ext = ak_filepath_extension(path);
// Result: ".txt"
ak_buffer_free(ext);

// Hidden files without extensions return NULL
ak_buffer_t *ext2 = ak_filepath_extension(".bashrc");
// Result: NULL

// Files without extensions return NULL
ak_buffer_t *ext3 = ak_filepath_extension("README");
// Result: NULL
```

### System Directories

```c
ak_buffer_t *ak_filepath_home(void);
ak_buffer_t *ak_filepath_cache(void);
ak_buffer_t *ak_filepath_config(void);
ak_buffer_t *ak_filepath_data(void);
ak_buffer_t *ak_filepath_temp(void);
ak_buffer_t *ak_filepath_cwd(void);
```

Get platform-specific system directories:

| Function | Linux | macOS | Windows |
|----------|-------|-------|---------|
| `home()` | `$HOME` or `/home/user` | `/Users/user` | `%USERPROFILE%` |
| `cache()` | `$XDG_CACHE_HOME` or `~/.cache` | `~/Library/Caches` | `%LOCALAPPDATA%` |
| `config()` | `$XDG_CONFIG_HOME` or `~/.config` | `~/Library/Application Support` | `%APPDATA%` |
| `data()` | `$XDG_DATA_HOME` or `~/.local/share` | `~/Library/Application Support` | `%LOCALAPPDATA%` |
| `temp()` | `$TMPDIR` or `/tmp` | `/tmp` | `%TEMP%` |
| `cwd()` | Current working directory | Current working directory | Current working directory |

```c
ak_buffer_t *home = ak_filepath_home();
ak_buffer_t *config = ak_filepath_config();
ak_buffer_t *app_config = ak_filepath_join(2, ak_buffer_cstr(config), "myapp");

// Use the paths...

ak_buffer_free(app_config);
ak_buffer_free(config);
ak_buffer_free(home);
```

### Path Conversion

```c
ak_buffer_t *ak_filepath_to_native(const char *path);
```

Convert all path separators to the platform-appropriate separator. Useful when dealing with paths from external sources or cross-platform data.

```c
// On Windows:
ak_buffer_t *native = ak_filepath_to_native("C:/Users/name/file.txt");
// Result: "C:\Users\name\file.txt"
ak_buffer_free(native);

// On POSIX:
ak_buffer_t *native2 = ak_filepath_to_native("home\\user\\file.txt");
// Result: "home/user/file.txt"
ak_buffer_free(native2);
```

## Usage Examples

### Basic Usage

```c
#include "kernel.h"

int main(void) {
  ak_kernel_init("my-app");

  // Join paths
  ak_buffer_t *path = ak_filepath_join(3, "/home", "user", "file.txt");
  // Access string data from buffer (ensure null termination if needed)
  printf("Path: %s\n", (const char *)ak_buffer_data(path));

  // Extract components
  const char *path_str = (const char *)ak_buffer_data(path);
  ak_buffer_t *base = ak_filepath_basename(path_str);
  ak_buffer_t *dir = ak_filepath_dirname(path_str);
  printf("Basename: %s\n", (const char *)ak_buffer_data(base));
  printf("Dirname: %s\n", (const char *)ak_buffer_data(dir));

  // Get system directories
  ak_buffer_t *home = ak_filepath_home();
  ak_buffer_t *cache = ak_filepath_cache();
  printf("Home: %s\n", (const char *)ak_buffer_data(home));
  printf("Cache: %s\n", (const char *)ak_buffer_data(cache));

  // Cleanup
  ak_buffer_free(cache);
  ak_buffer_free(home);
  ak_buffer_free(dir);
  ak_buffer_free(base);
  ak_buffer_free(path);

  ak_kernel_deinit();
  return 0;
}
```

### Application Config Directory

```c
#include "kernel.h"

// Get or create application-specific config directory
ak_buffer_t *get_app_config_dir(const char *app_name) {
  ak_buffer_t *config_base = ak_filepath_config();
  if (!config_base) {
    return NULL;
  }

  const char *config_str = (const char *)ak_buffer_data(config_base);
  ak_buffer_t *app_config = ak_filepath_join(2, config_str, app_name);
  ak_buffer_free(config_base);

  return app_config;
}

int main(void) {
  ak_kernel_init("my-app");

  ak_buffer_t *config_dir = get_app_config_dir("myapp");
  if (config_dir) {
    const char *config_dir_str = (const char *)ak_buffer_data(config_dir);
    ak_buffer_t *config_file = ak_filepath_join(2, config_dir_str, "settings.json");

    // Linux: ~/.config/myapp/settings.json
    // macOS: ~/Library/Application Support/myapp/settings.json
    // Windows: C:\Users\name\AppData\Roaming\myapp\settings.json

    printf("Config file: %s\n", (const char *)ak_buffer_data(config_file));

    ak_buffer_free(config_file);
    ak_buffer_free(config_dir);
  }

  ak_kernel_deinit();
  return 0;
}
```

### Path Normalization

```c
#include "kernel.h"

int main(void) {
  ak_kernel_init("my-app");

  // Normalize messy paths
  const char *messy = "/home/user/../other/./subdir//file.txt";
  ak_buffer_t *clean = ak_filepath_normalize(messy);

  printf("Original: %s\n", messy);
  printf("Normalized: %s\n", (const char *)ak_buffer_data(clean));
  // Output: /home/other/subdir/file.txt

  ak_buffer_free(clean);
  ak_kernel_deinit();
  return 0;
}
```

### Cross-Platform Path Building

```c
#include "kernel.h"

// Build paths that work on any platform
void create_project_structure(const char *project_root) {
  // Join paths - automatically uses correct separators
  ak_buffer_t *src_dir = ak_filepath_join(2, project_root, "src");
  ak_buffer_t *include_dir = ak_filepath_join(2, project_root, "include");
  ak_buffer_t *test_dir = ak_filepath_join(2, project_root, "test");

  // Create main source file path
  const char *src_str = (const char *)ak_buffer_data(src_dir);
  ak_buffer_t *main_file = ak_filepath_join(2, src_str, "main.c");

  printf("Project structure:\n");
  printf("  Source: %s\n", (const char *)ak_buffer_data(src_dir));
  printf("  Include: %s\n", (const char *)ak_buffer_data(include_dir));
  printf("  Test: %s\n", (const char *)ak_buffer_data(test_dir));
  printf("  Main: %s\n", (const char *)ak_buffer_data(main_file));

  ak_buffer_free(main_file);
  ak_buffer_free(test_dir);
  ak_buffer_free(include_dir);
  ak_buffer_free(src_dir);
}

int main(void) {
  ak_kernel_init("my-app");

  ak_buffer_t *home = ak_filepath_home();
  const char *home_str = (const char *)ak_buffer_data(home);
  ak_buffer_t *project = ak_filepath_join(2, home_str, "my_project");

  create_project_structure((const char *)ak_buffer_data(project));

  ak_buffer_free(project);
  ak_buffer_free(home);
  ak_kernel_deinit();
  return 0;
}
```

## Implementation Notes

- All returned `ak_buffer_t*` pointers must be freed by the caller using `ak_buffer_free()`
- Paths are UTF-8 encoded on all platforms
- The module uses the existing `buffer` and `scanner` libraries for string manipulation
- Path normalization does not resolve symlinks or verify paths exist
- Follows XDG Base Directory Specification on Linux
- Follows macOS conventions for system directories
- Handles Windows drive letters and UNC paths correctly

## Concurrency

Filepath functions can be called from multiple threads. The module maintains minimal read-only global state (platform separators) that is set once during kernel initialization.

## Performance

- Path joining: O(n) where n is total length of all components
- Path normalization: O(n) where n is path length
- Component extraction: O(n) where n is path length
- System directory lookup: O(1) with environment variable lookups

## See Also

- [buffer.md](../../buffer/docs/buffer.md) - String buffer manipulation
- [scanner.md](../../scanner/docs/scanner.md) - String scanning and tokenization
- [kernel.md](../../../docs/kernel.md) - Kernel initialization and memory management
