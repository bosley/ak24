/**
 * @file intern.h
 * @brief String interning system for memory optimization and fast equality
 *
 * Provides a global string interning system that ensures only one copy of each
 * unique string exists in memory. Interned strings enable O(1) pointer-based
 * equality checks instead of O(n) strcmp operations, making them ideal for
 * identifiers, symbol table keys, type names, and string literals.
 *
 * Key features:
 * - Global string table with automatic deduplication
 * - O(1) equality checks via pointer comparison
 * - Thread-safe operations with internal locking
 * - Statistics tracking for monitoring
 * - Stable pointers valid for program lifetime
 * - Works with or without GC enabled
 *
 * Usage pattern:
 * @code
 * ak_intern_init();
 *
 * const char *str1 = ak_intern("hello");
 * const char *str2 = ak_intern("hello");
 * assert(str1 == str2);  // Same pointer!
 * assert(ak_intern_eq(str1, str2));  // Fast equality
 *
 * ak_intern_shutdown();
 * @endcode
 *
 * @note All interned strings persist until ak_intern_shutdown()
 * @note Thread-safe for concurrent interning
 * @note Intended for use with identifiers and symbols, not large text
 */

#ifndef AK24_INTERN_H
#define AK24_INTERN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Initialize the global string interning system
 *
 * Must be called before any other intern functions. Sets up the internal
 * hash table and synchronization primitives.
 *
 * @threadsafe Yes - but only call once at program start
 *
 * @note Typically called from ak_kernel_init()
 */
void ak_intern_init(void);

/**
 * @brief Shutdown and cleanup the interning system
 *
 * Frees all interned strings and associated memory. After this call,
 * all previously returned interned string pointers become invalid.
 *
 * @threadsafe Yes - but only call once at program end
 *
 * @note Typically called from ak_kernel_deinit()
 * @warning Invalidates all interned string pointers
 */
void ak_intern_shutdown(void);

/**
 * @brief Intern a null-terminated string
 *
 * Returns a canonical pointer for the given string. If the string already
 * exists in the intern table, returns the existing pointer. Otherwise,
 * allocates a new copy and adds it to the table.
 *
 * @param str Null-terminated string to intern
 * @return Canonical pointer to interned string, or NULL if str is NULL
 *
 * @threadsafe Yes
 *
 * @note Returned pointer remains valid until ak_intern_shutdown()
 * @note Multiple calls with equal strings return the same pointer
 *
 * Example:
 * @code
 * const char *name = ak_intern("variable_name");
 * @endcode
 */
const char *ak_intern(const char *str);

/**
 * @brief Intern a substring (non-null-terminated)
 *
 * Interns the first `len` characters of `str`, regardless of null terminators.
 * Useful for interning tokens directly from source buffers.
 *
 * @param str Pointer to character data
 * @param len Number of characters to intern
 * @return Canonical pointer to interned string, or NULL if str is NULL
 *
 * @threadsafe Yes
 *
 * @note Returned string will be null-terminated
 * @note Returned pointer remains valid until ak_intern_shutdown()
 *
 * Example:
 * @code
 * const char *source = "identifier+123";
 * const char *id = ak_intern_n(source, 10);  // "identifier"
 * @endcode
 */
const char *ak_intern_n(const char *str, size_t len);

/**
 * @brief Fast equality check for interned strings
 *
 * Compares two pointers that should be interned strings. Since interned
 * strings have unique pointers, this is just a pointer comparison.
 *
 * @param a First interned string
 * @param b Second interned string
 * @return true if strings are equal, false otherwise
 *
 * @threadsafe Yes
 *
 * @note Only valid for strings obtained from ak_intern() or ak_intern_n()
 * @note O(1) operation vs O(n) for strcmp
 *
 * Example:
 * @code
 * const char *key1 = ak_intern("name");
 * const char *key2 = ak_intern("name");
 * if (ak_intern_eq(key1, key2)) {
 *   // Always true for same strings
 * }
 * @endcode
 */
static inline bool ak_intern_eq(const char *a, const char *b) { return a == b; }

/**
 * @brief Fast hash of interned string
 *
 * Returns a hash value for an interned string. Since interned strings
 * have unique pointers, this hashes the pointer itself rather than the
 * string contents.
 *
 * @param str Interned string to hash
 * @return Hash value
 *
 * @threadsafe Yes
 *
 * @note Only valid for strings obtained from ak_intern() or ak_intern_n()
 * @note O(1) operation vs O(n) for string hashing
 *
 * Example:
 * @code
 * const char *key = ak_intern("identifier");
 * uint64_t hash = ak_intern_hash(key);
 * @endcode
 */
static inline uint64_t ak_intern_hash(const char *str) {
  return (uint64_t)(uintptr_t)str;
}

/**
 * @brief Get interning statistics
 *
 * Retrieves current statistics about the interning system for monitoring
 * and debugging purposes.
 *
 * @param count Output: number of unique interned strings (may be NULL)
 * @param bytes Output: total bytes used by interned strings (may be NULL)
 *
 * @threadsafe Yes
 *
 * Example:
 * @code
 * size_t count, bytes;
 * ak_intern_stats(&count, &bytes);
 * printf("Interned: %zu strings, %zu bytes\n", count, bytes);
 * @endcode
 */
void ak_intern_stats(size_t *count, size_t *bytes);

/**
 * @brief Clear all interned strings
 *
 * Removes all strings from the intern table and frees their memory.
 * Unlike shutdown, this keeps the interning system initialized and
 * ready for new strings.
 *
 * @threadsafe Yes
 *
 * @note Useful for testing or REPL environments
 * @warning Invalidates all previously returned interned string pointers
 *
 * Example:
 * @code
 * // Between test runs
 * ak_intern_clear();
 * @endcode
 */
void ak_intern_clear(void);

#endif // AK24_INTERN_H
