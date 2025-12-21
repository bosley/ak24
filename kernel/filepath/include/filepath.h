/**
 * @file filepath.h
 * @brief Cross-platform file path manipulation and system directory access
 *
 * Provides a platform-agnostic API for working with file system paths,
 * handling the differences between Windows (backslash) and POSIX (forward
 * slash) path separators. Includes utilities for path joining, normalization,
 * getting common system directories (home, cache, temp, etc.), and path
 * component extraction.
 *
 * Key features:
 * - Cross-platform path separator handling
 * - Safe path joining with automatic separator handling
 * - Path normalization (resolving . and .., redundant separators)
 * - System directory access (home, cache, temp, config, data)
 * - Path component extraction (basename, dirname, extension)
 * - Absolute/relative path handling
 * - Thread-safe operations
 *
 * Usage pattern:
 * @code
 * ak_filepath_init();
 *
 * // Join paths correctly regardless of platform
 * ak_buffer_t *path = ak_filepath_join(3, "/home", "user", "file.txt");
 * // Result: "/home/user/file.txt" on POSIX, "\home\user\file.txt" on Windows
 *
 * // Get system directories
 * ak_buffer_t *home = ak_filepath_home();
 * ak_buffer_t *cache = ak_filepath_cache();
 *
 * // Extract path components
 * ak_buffer_t *base = ak_filepath_basename(path);
 * ak_buffer_t *dir = ak_filepath_dirname(path);
 * ak_buffer_t *ext = ak_filepath_extension(path);
 *
 * ak_filepath_shutdown();
 * @endcode
 *
 * @note All returned ak_buffer_t* must be freed by caller
 * @note Thread-safe for all operations
 * @note Paths are UTF-8 encoded on all platforms
 */

#ifndef AK24_FILEPATH_H
#define AK24_FILEPATH_H

#include "buffer.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Initialize the filepath system
 *
 * Must be called before any other filepath functions. Sets up internal
 * state and platform-specific path handling.
 *
 * @threadsafe Yes - but only call once at program start
 *
 * @note Typically called from ak_kernel_init()
 */
void ak_filepath_init(void);

/**
 * @brief Shutdown and cleanup the filepath system
 *
 * Frees any internal resources. After this call, filepath functions
 * should not be used.
 *
 * @threadsafe Yes - but only call once at program end
 *
 * @note Typically called from ak_kernel_deinit()
 */
void ak_filepath_shutdown(void);

/**
 * @brief Get the platform-specific path separator
 *
 * @return '/' on POSIX systems, '\\' on Windows
 *
 * @threadsafe Yes
 *
 * Example:
 * @code
 * char sep = ak_filepath_separator();
 * // sep == '/' on Linux/macOS
 * // sep == '\\' on Windows
 * @endcode
 */
char ak_filepath_separator(void);

/**
 * @brief Get the platform-specific path list separator
 *
 * Used in PATH, LD_LIBRARY_PATH, etc.
 *
 * @return ':' on POSIX systems, ';' on Windows
 *
 * @threadsafe Yes
 */
char ak_filepath_list_separator(void);

/**
 * @brief Join multiple path components with correct separators
 *
 * Combines multiple path components into a single path using the
 * platform-appropriate separator. Handles trailing/leading separators
 * correctly and avoids double separators.
 *
 * @param count Number of path components
 * @param ... Variable number of const char* path components
 * @return New buffer containing joined path, or NULL on error
 *
 * @threadsafe Yes
 *
 * @note Caller must free returned buffer
 * @note Accepts NULL or empty components (skipped)
 *
 * Example:
 * @code
 * ak_buffer_t *path = ak_filepath_join(3, "/home", "user", "file.txt");
 * // POSIX: "/home/user/file.txt"
 * // Windows: "\home\user\file.txt"
 * ak_buffer_free(path);
 * @endcode
 */
ak_buffer_t *ak_filepath_join(size_t count, ...);

/**
 * @brief Normalize a path
 *
 * Resolves relative components (. and ..), removes redundant separators,
 * and converts to platform-appropriate separator.
 *
 * @param path Path to normalize
 * @return New buffer containing normalized path, or NULL on error
 *
 * @threadsafe Yes
 *
 * @note Caller must free returned buffer
 * @note Does not resolve symlinks or verify path exists
 *
 * Example:
 * @code
 * ak_buffer_t *norm = ak_filepath_normalize("/home/user/../other/./file.txt");
 * // Result: "/home/other/file.txt"
 * ak_buffer_free(norm);
 * @endcode
 */
ak_buffer_t *ak_filepath_normalize(const char *path);

/**
 * @brief Check if path is absolute
 *
 * Determines if a path is absolute (starts with / on POSIX,
 * or drive letter/UNC on Windows).
 *
 * @param path Path to check
 * @return true if absolute, false if relative or NULL
 *
 * @threadsafe Yes
 *
 * Example:
 * @code
 * bool abs1 = ak_filepath_is_absolute("/home/user");     // true on POSIX
 * bool abs2 = ak_filepath_is_absolute("C:\\Users");      // true on Windows
 * bool abs3 = ak_filepath_is_absolute("relative/path");  // false
 * @endcode
 */
bool ak_filepath_is_absolute(const char *path);

/**
 * @brief Get the basename of a path
 *
 * Extracts the final component of a path (filename with extension).
 *
 * @param path Path to extract basename from
 * @return New buffer containing basename, or NULL on error
 *
 * @threadsafe Yes
 *
 * @note Caller must free returned buffer
 *
 * Example:
 * @code
 * ak_buffer_t *base = ak_filepath_basename("/home/user/file.txt");
 * // Result: "file.txt"
 * ak_buffer_free(base);
 * @endcode
 */
ak_buffer_t *ak_filepath_basename(const char *path);

/**
 * @brief Get the directory name of a path
 *
 * Extracts the directory portion of a path (everything except the basename).
 *
 * @param path Path to extract dirname from
 * @return New buffer containing dirname, or NULL on error
 *
 * @threadsafe Yes
 *
 * @note Caller must free returned buffer
 *
 * Example:
 * @code
 * ak_buffer_t *dir = ak_filepath_dirname("/home/user/file.txt");
 * // Result: "/home/user"
 * ak_buffer_free(dir);
 * @endcode
 */
ak_buffer_t *ak_filepath_dirname(const char *path);

/**
 * @brief Get the file extension
 *
 * Extracts the extension from a filename (everything after the last dot).
 *
 * @param path Path to extract extension from
 * @return New buffer containing extension (including dot), or NULL if no
 * extension
 *
 * @threadsafe Yes
 *
 * @note Caller must free returned buffer
 * @note Returns NULL for paths with no extension or hidden files with no
 * extension
 *
 * Example:
 * @code
 * ak_buffer_t *ext = ak_filepath_extension("file.txt");
 * // Result: ".txt"
 * ak_buffer_free(ext);
 *
 * ak_buffer_t *ext2 = ak_filepath_extension(".bashrc");
 * // Result: NULL (hidden file, no extension)
 * @endcode
 */
ak_buffer_t *ak_filepath_extension(const char *path);

/**
 * @brief Get user's home directory
 *
 * Returns the current user's home directory path.
 * - POSIX: $HOME or /etc/passwd lookup
 * - Windows: %USERPROFILE% or %HOMEDRIVE%%HOMEPATH%
 *
 * @return New buffer containing home directory path, or NULL on error
 *
 * @threadsafe Yes
 *
 * @note Caller must free returned buffer
 *
 * Example:
 * @code
 * ak_buffer_t *home = ak_filepath_home();
 * // Linux: "/home/username"
 * // macOS: "/Users/username"
 * // Windows: "C:\Users\username"
 * ak_buffer_free(home);
 * @endcode
 */
ak_buffer_t *ak_filepath_home(void);

/**
 * @brief Get cache directory
 *
 * Returns the appropriate cache directory for the platform.
 * - Linux: $XDG_CACHE_HOME or ~/.cache
 * - macOS: ~/Library/Caches
 * - Windows: %LOCALAPPDATA%
 *
 * @return New buffer containing cache directory path, or NULL on error
 *
 * @threadsafe Yes
 *
 * @note Caller must free returned buffer
 *
 * Example:
 * @code
 * ak_buffer_t *cache = ak_filepath_cache();
 * ak_buffer_free(cache);
 * @endcode
 */
ak_buffer_t *ak_filepath_cache(void);

/**
 * @brief Get configuration directory
 *
 * Returns the appropriate config directory for the platform.
 * - Linux: $XDG_CONFIG_HOME or ~/.config
 * - macOS: ~/Library/Application Support
 * - Windows: %APPDATA%
 *
 * @return New buffer containing config directory path, or NULL on error
 *
 * @threadsafe Yes
 *
 * @note Caller must free returned buffer
 *
 * Example:
 * @code
 * ak_buffer_t *config = ak_filepath_config();
 * ak_buffer_free(config);
 * @endcode
 */
ak_buffer_t *ak_filepath_config(void);

/**
 * @brief Get data directory
 *
 * Returns the appropriate data directory for the platform.
 * - Linux: $XDG_DATA_HOME or ~/.local/share
 * - macOS: ~/Library/Application Support
 * - Windows: %LOCALAPPDATA%
 *
 * @return New buffer containing data directory path, or NULL on error
 *
 * @threadsafe Yes
 *
 * @note Caller must free returned buffer
 *
 * Example:
 * @code
 * ak_buffer_t *data = ak_filepath_data();
 * ak_buffer_free(data);
 * @endcode
 */
ak_buffer_t *ak_filepath_data(void);

/**
 * @brief Get temporary directory
 *
 * Returns the system temporary directory.
 * - POSIX: $TMPDIR or /tmp
 * - Windows: %TEMP% or %TMP%
 *
 * @return New buffer containing temp directory path, or NULL on error
 *
 * @threadsafe Yes
 *
 * @note Caller must free returned buffer
 *
 * Example:
 * @code
 * ak_buffer_t *temp = ak_filepath_temp();
 * ak_buffer_free(temp);
 * @endcode
 */
ak_buffer_t *ak_filepath_temp(void);

/**
 * @brief Get current working directory
 *
 * Returns the current working directory of the process.
 *
 * @return New buffer containing current directory path, or NULL on error
 *
 * @threadsafe Yes
 *
 * @note Caller must free returned buffer
 *
 * Example:
 * @code
 * ak_buffer_t *cwd = ak_filepath_cwd();
 * ak_buffer_free(cwd);
 * @endcode
 */
ak_buffer_t *ak_filepath_cwd(void);

/**
 * @brief Convert path to use platform-specific separators
 *
 * Converts all path separators to the platform-appropriate separator.
 * Useful when dealing with paths from external sources or cross-platform
 * data.
 *
 * @param path Path to convert
 * @return New buffer containing converted path, or NULL on error
 *
 * @threadsafe Yes
 *
 * @note Caller must free returned buffer
 *
 * Example:
 * @code
 * ak_buffer_t *win_path = ak_filepath_to_native("C:/Users/name/file.txt");
 * // Windows: "C:\Users\name\file.txt"
 * // POSIX: "C:/Users/name/file.txt" (unchanged)
 * ak_buffer_free(win_path);
 * @endcode
 */
ak_buffer_t *ak_filepath_to_native(const char *path);

#endif // AK24_FILEPATH_H
