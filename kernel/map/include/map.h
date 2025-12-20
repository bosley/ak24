/**
 * @file map.h
 * @brief Type-safe generic hash map using C macro templates
 *
 * Provides a macro-based generic hash map implementation with customizable
 * hash and comparison functions. Based on rxi's map library, modified for
 * AK24 kernel integration. Supports arbitrary key and value types through
 * compile-time macro expansion.
 *
 * Key features:
 * - Type-safe generic key-value storage
 * - Customizable hash functions for different key types
 * - Automatic bucket management and resizing
 * - Built-in hash functions for common types
 * - Iterator pattern for traversal
 * - Separate chaining collision resolution
 *
 * @note All operations are NOT thread-safe
 * @see https://github.com/rxi/map
 */

/**
 * Copyright (c) 2014 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * - Modified for AK24 by bosley 2025
 */

#ifndef AK24_MAP_H
#define AK24_MAP_H

#include <string.h>

#define MAP_VERSION "0.0.1-dev"

struct map_node_t;
typedef struct map_node_t map_node_t;

/**
 * @brief Hash function type for map keys
 *
 * @param key Pointer to key data
 * @param ksize Size of key in bytes
 * @return Hash value for the key
 */
typedef unsigned (*map_hash_fn)(const void *key, int ksize);

/**
 * @brief Comparison function type for map keys
 *
 * @param a First key to compare
 * @param b Second key to compare
 * @param ksize Size of keys in bytes
 * @return 0 if equal, non-zero otherwise
 */
typedef int (*map_cmp_fn)(const void *a, const void *b, int ksize);

/**
 * @brief Internal base structure for map implementation
 *
 * Used internally by macro expansion. Users should not access this directly.
 */
typedef struct {
  map_node_t **buckets; /**< Hash table buckets */
  unsigned nbuckets;    /**< Number of buckets */
  unsigned nnodes;      /**< Number of stored nodes */
  int ksize;            /**< Size of keys in bytes */
  map_hash_fn hash_fn;  /**< Hash function */
  map_cmp_fn cmp_fn;    /**< Comparison function */
} map_base_t;

/**
 * @brief Iterator state for map traversal
 *
 * Tracks position during iteration. Initialize with map_iter().
 */
typedef struct {
  unsigned bucketidx; /**< Current bucket index */
  map_node_t *node;   /**< Current node */
} map_iter_t;

/**
 * @def map_t(T)
 * @brief Define a typed map structure
 *
 * Creates a map structure for the specified value type. Keys are specified
 * during initialization. The resulting structure should be initialized with
 * map_init_generic() before use.
 *
 * @param T Type of values to store
 *
 * @par Example:
 * @code
 * map_t(int) age_map;
 * map_init_generic(&age_map, sizeof(char*), map_hash_str, map_cmp_str);
 * int age = 42;
 * map_set_generic(&age_map, "alice", age);
 * map_deinit(&age_map);
 * @endcode
 */
#define map_t(T)                                                               \
  struct {                                                                     \
    map_base_t base; /**< Internal map state and bookkeeping */                \
    T *ref;          /**< Reference pointer for value access */                \
    T tmp;           /**< Temporary variable for type inference */             \
  }

/**
 * @def map_init_generic(m, keysize, hashfn, cmpfn)
 * @brief Initialize a map with custom hash and comparison functions
 *
 * Must be called before using a map. Sets up internal structures and
 * specifies how keys should be hashed and compared.
 *
 * @param m Pointer to map to initialize
 * @param keysize Size of keys in bytes
 * @param hashfn Hash function for keys
 * @param cmpfn Comparison function for keys
 */
#define map_init_generic(m, keysize, hashfn, cmpfn)                            \
  do {                                                                         \
    memset(m, 0, sizeof(*(m)));                                                \
    (m)->base.ksize = (keysize);                                               \
    (m)->base.hash_fn = (hashfn);                                              \
    (m)->base.cmp_fn = (cmpfn);                                                \
  } while (0)

/**
 * @def map_deinit(m)
 * @brief Deinitialize a map
 *
 * Frees all internal memory. Map must not be used after this call.
 *
 * @param m Pointer to map to deinitialize
 */
#define map_deinit(m) map_deinit_(&(m)->base)

/**
 * @def map_get_generic(m, key)
 * @brief Get value associated with key
 *
 * Retrieves the value for the specified key.
 *
 * @param m Pointer to map
 * @param key Pointer to key
 * @return Pointer to value, or NULL if key not found
 */
#define map_get_generic(m, key) ((m)->ref = map_get_(&(m)->base, key))

/**
 * @def map_set_generic(m, key, value)
 * @brief Set value for key
 *
 * Inserts or updates the value associated with the key. The map will
 * automatically resize if needed.
 *
 * @param m Pointer to map
 * @param key Pointer to key
 * @param value Value to store
 * @return 0 on success, -1 on allocation failure
 */
#define map_set_generic(m, key, value)                                         \
  ((m)->tmp = (value), map_set_(&(m)->base, key, &(m)->tmp, sizeof((m)->tmp)))

/**
 * @def map_remove_generic(m, key)
 * @brief Remove key-value pair from map
 *
 * Removes the entry for the specified key if it exists.
 *
 * @param m Pointer to map
 * @param key Pointer to key to remove
 */
#define map_remove_generic(m, key) map_remove_(&(m)->base, key)

/**
 * @def map_iter(m)
 * @brief Create a new iterator
 *
 * @param m Pointer to map
 * @return New iterator positioned at start
 */
#define map_iter(m) map_iter_()

/**
 * @def map_next_generic(m, iter)
 * @brief Get next key-value pair from iterator
 *
 * Advances iterator and returns pointer to next value. To get the key,
 * access the node structure directly.
 *
 * @param m Pointer to map
 * @param iter Pointer to iterator
 * @return Pointer to next value, or NULL if at end
 */
#define map_next_generic(m, iter) map_next_(&(m)->base, iter)

/**
 * @brief Internal deinit implementation
 * @notthreadsafe
 */
void map_deinit_(map_base_t *m);

/**
 * @brief Internal get implementation
 * @notthreadsafe
 */
void *map_get_(map_base_t *m, const void *key);

/**
 * @brief Internal set implementation
 * @notthreadsafe
 */
int map_set_(map_base_t *m, const void *key, void *value, int vsize);

/**
 * @brief Internal remove implementation
 * @notthreadsafe
 */
void map_remove_(map_base_t *m, const void *key);

/**
 * @brief Internal iterator creation
 * @notthreadsafe
 */
map_iter_t map_iter_(void);

/**
 * @brief Internal iterator next implementation
 * @notthreadsafe
 */
void *map_next_(map_base_t *m, map_iter_t *iter);

/**
 * @brief Hash function for string keys
 * @threadsafe
 */
unsigned map_hash_str(const void *key, int ksize);

/**
 * @brief Hash function for int8_t keys
 * @threadsafe
 */
unsigned map_hash_i8(const void *key, int ksize);

/**
 * @brief Hash function for int16_t keys
 * @threadsafe
 */
unsigned map_hash_i16(const void *key, int ksize);

/**
 * @brief Hash function for int32_t keys
 * @threadsafe
 */
unsigned map_hash_i32(const void *key, int ksize);

/**
 * @brief Hash function for int64_t keys
 * @threadsafe
 */
unsigned map_hash_i64(const void *key, int ksize);

/**
 * @brief Hash function for uint8_t keys
 * @threadsafe
 */
unsigned map_hash_u8(const void *key, int ksize);

/**
 * @brief Hash function for uint16_t keys
 * @threadsafe
 */
unsigned map_hash_u16(const void *key, int ksize);

/**
 * @brief Hash function for uint32_t keys
 * @threadsafe
 */
unsigned map_hash_u32(const void *key, int ksize);

/**
 * @brief Hash function for uint64_t keys
 * @threadsafe
 */
unsigned map_hash_u64(const void *key, int ksize);

/**
 * @brief Hash function for float keys
 * @threadsafe
 */
unsigned map_hash_f32(const void *key, int ksize);

/**
 * @brief Hash function for double keys
 * @threadsafe
 */
unsigned map_hash_f64(const void *key, int ksize);

/**
 * @brief Hash function for bool keys
 * @threadsafe
 */
unsigned map_hash_bool(const void *key, int ksize);

/**
 * @brief String comparison function
 * @threadsafe
 */
int map_cmp_str(const void *a, const void *b, int ksize);

/**
 * @brief Memory comparison function
 * @threadsafe
 */
int map_cmp_mem(const void *a, const void *b, int ksize);

/**
 * @brief Predefined map type for void pointers
 */
typedef map_t(void *) map_void_t;

/**
 * @brief Predefined map type for strings
 */
typedef map_t(char *) map_str_t;

/**
 * @brief Predefined map type for integers
 */
typedef map_t(int) map_int_t;

/**
 * @brief Predefined map type for characters
 */
typedef map_t(char) map_char_t;

/**
 * @brief Predefined map type for floats
 */
typedef map_t(float) map_float_t;

/**
 * @brief Predefined map type for doubles
 */
typedef map_t(double) map_double_t;

#endif
