/**
 * @file atom.h
 * @brief Multi-dimensional atomic data nodes with neighbor relationships
 *
 * Provides a novel multi-dimensional data structure where atoms can form
 * neighbor relationships in N-dimensional space. Unlike traditional linked
 * structures (1-dimensional chains), atoms support arbitrary dimensional
 * connectivity where "next" returns all neighbors at the next level
 * emanating outwards from the atom.
 *
 * Key features:
 * - Atomic primitive types (integers, floats, chars, bytes)
 * - Multi-dimensional neighbor relationships
 * - Thread-safe atomic operations on values and structure
 * - Atom clustering for grouping related atoms
 * - Category-based type system
 * - Distance-based neighbor queries
 *
 * @note Most operations are thread-safe via atomic operations
 * @see Dimensional neighbor system allows graph-like structures beyond linear
 * chains
 */

#ifndef AK24_ATOM_H
#define AK24_ATOM_H

#include <stdatomic.h>
#include <stdint.h>

/**
 * @brief Atom primitive type enumeration
 *
 * Defines the concrete type stored in an atom's value union.
 */
typedef enum {
  AK24_ATOM_BYTE,    /**< Unsigned byte */
  AK24_ATOM_U8,      /**< Unsigned 8-bit integer */
  AK24_ATOM_U16,     /**< Unsigned 16-bit integer */
  AK24_ATOM_U32,     /**< Unsigned 32-bit integer */
  AK24_ATOM_U64,     /**< Unsigned 64-bit integer */
  AK24_ATOM_I8,      /**< Signed 8-bit integer */
  AK24_ATOM_I16,     /**< Signed 16-bit integer */
  AK24_ATOM_I32,     /**< Signed 32-bit integer */
  AK24_ATOM_I64,     /**< Signed 64-bit integer */
  AK24_ATOM_F32,     /**< 32-bit floating point */
  AK24_ATOM_F64,     /**< 64-bit floating point */
  AK24_ATOM_CHAR,    /**< Character (1 byte signed) */
  AK24_ATOM_CLUSTER, /**< Group of atoms */
} ak_atom_type_e;

/**
 * @brief Atom category for type classification
 *
 * Allows grouping atoms by properties rather than concrete types.
 */
typedef enum {
  AK24_ANY,      /**< Any type */
  AK24_NUMERIC,  /**< Numeric types (integers and floats) */
  AK24_UNSIGNED, /**< Unsigned integer types */
  AK24_SIGNED,   /**< Signed types (integers and floats) */
  AK24_REAL,     /**< Floating point types */
  AK24_SYMBOLIC, /**< Character/symbolic types */
} ak_atom_category_e;

/**
 * @brief Atom metadata with type categories
 *
 * Associates an atom type with its applicable categories.
 */
typedef struct {
  ak_atom_type_e type;             /**< Concrete type */
  ak_atom_category_e categories[]; /**< Array of applicable categories */
} ak_atom_meta_t;

/**
 * @brief Neighbor link in multi-dimensional space
 *
 * Forms a linked list of neighboring atoms, enabling N-dimensional
 * connectivity.
 */
typedef struct ak_atom_neighbor_t {
  _Atomic(struct ak_atom_t *) atom;          /**< Neighboring atom */
  _Atomic(struct ak_atom_neighbor_t *) next; /**< Next neighbor in list */
} ak_atom_neighbor_t;

/**
 * @brief Multi-dimensional atomic data node
 *
 * Core structure representing a value with atomic operations and
 * multi-dimensional neighbor relationships.
 */
typedef struct ak_atom_t {
  _Atomic(ak_atom_type_e) type;            /**< Type of value stored */
  _Atomic size_t dimensionality;           /**< Dimensional connectivity */
  _Atomic(ak_atom_neighbor_t *) neighbors; /**< List of neighbor atoms */
  union {
    _Atomic uint8_t u8;   /**< Unsigned 8-bit value */
    _Atomic uint16_t u16; /**< Unsigned 16-bit value */
    _Atomic uint32_t u32; /**< Unsigned 32-bit value */
    _Atomic uint64_t u64; /**< Unsigned 64-bit value */
    _Atomic int8_t i8;    /**< Signed 8-bit value */
    _Atomic int16_t i16;  /**< Signed 16-bit value */
    _Atomic int32_t i32;  /**< Signed 32-bit value */
    _Atomic int64_t i64;  /**< Signed 64-bit value */
    _Atomic float f32;    /**< 32-bit float value */
    _Atomic double f64;   /**< 64-bit float value */
    _Atomic char c;       /**< Character value */
    _Atomic uint8_t byte; /**< Byte value */
    void *cluster;        /**< Pointer to atom cluster */
  } value;                /**< Atomic value storage union */
} ak_atom_t;

/**
 * @brief Collection of atoms
 *
 * Groups multiple atoms together with dynamic capacity management.
 */
typedef struct ak_atom_cluster_t {
  ak_atom_t **atoms; /**< Array of atom pointers */
  size_t count;      /**< Number of atoms */
  size_t capacity;   /**< Allocated capacity */
} ak_atom_cluster_t;

/**
 * @brief Union for atom values
 *
 * Non-atomic version for passing values to/from atoms.
 */
typedef union {
  uint8_t u8;    /**< Unsigned 8-bit value */
  uint16_t u16;  /**< Unsigned 16-bit value */
  uint32_t u32;  /**< Unsigned 32-bit value */
  uint64_t u64;  /**< Unsigned 64-bit value */
  int8_t i8;     /**< Signed 8-bit value */
  int16_t i16;   /**< Signed 16-bit value */
  int32_t i32;   /**< Signed 32-bit value */
  int64_t i64;   /**< Signed 64-bit value */
  float f32;     /**< 32-bit float value */
  float f64;     /**< 64-bit float value */
  char c;        /**< Character value */
  uint8_t byte;  /**< Byte value */
  void *cluster; /**< Pointer to cluster */
} ak_atom_value_u;

/**
 * @brief Create a new atom
 *
 * Allocates and initializes an atom with the specified type, value, and
 * dimensional connectivity.
 *
 * @param type Type of atom to create
 * @param value Initial value
 * @param dimensionality Dimensional connectivity (1 for linear chain)
 * @return Pointer to new atom, or NULL on allocation failure
 *
 * @threadsafe
 * @lockfree
 *
 * @note Caller must free with ak_atom_free()
 *
 * @par Example:
 * @code
 * ak_atom_value_u val;
 * val.i32 = 42;
 * ak_atom_t *atom = ak_atom_new(AK24_ATOM_I32, val, 1);
 * @endcode
 */
ak_atom_t *ak_atom_new(ak_atom_type_e type, ak_atom_value_u value,
                       size_t dimensionality);

/**
 * @brief Free an atom
 *
 * Releases atom memory. Does NOT free neighbors or clusters.
 *
 * @param atom Atom to free (NULL is safe)
 *
 * @notthreadsafe
 */
void ak_atom_free(ak_atom_t *atom);

/**
 * @brief Get atom's value atomically
 *
 * Reads the atom's current value using atomic operations.
 *
 * @param atom Atom to read
 * @return Current value
 *
 * @threadsafe
 * @lockfree
 */
ak_atom_value_u ak_atom_get_value(ak_atom_t *atom);

/**
 * @brief Set atom's value atomically
 *
 * Updates the atom's value using atomic operations.
 *
 * @param atom Atom to modify
 * @param value New value
 *
 * @threadsafe
 * @lockfree
 */
void ak_atom_set_value(ak_atom_t *atom, ak_atom_value_u value);

/**
 * @brief Get atom's type atomically
 *
 * @param atom Atom to query
 * @return Atom type
 *
 * @threadsafe
 * @lockfree
 */
ak_atom_type_e ak_atom_get_type(ak_atom_t *atom);

/**
 * @brief Get atom's dimensionality atomically
 *
 * @param atom Atom to query
 * @return Dimensional connectivity value
 *
 * @threadsafe
 * @lockfree
 */
size_t ak_atom_get_dimensionality(ak_atom_t *atom);

/**
 * @brief Create bidirectional bond between atoms
 *
 * Establishes a neighbor relationship in both directions, enabling
 * multi-dimensional connectivity.
 *
 * @param atom1 First atom
 * @param atom2 Second atom
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 * @lockfree
 *
 * @par Example:
 * @code
 * ak_atom_t *a = ak_atom_new(AK24_ATOM_I32, val1, 2);
 * ak_atom_t *b = ak_atom_new(AK24_ATOM_I32, val2, 2);
 * ak_atom_bond(a, b);
 * @endcode
 */
int ak_atom_bond(ak_atom_t *atom1, ak_atom_t *atom2);

/**
 * @brief Remove bond between atoms
 *
 * Removes neighbor relationship in both directions.
 *
 * @param atom1 First atom
 * @param atom2 Second atom
 * @return 0 on success, -1 on failure
 *
 * @threadsafe
 * @lockfree
 */
int ak_atom_unbond(ak_atom_t *atom1, ak_atom_t *atom2);

/**
 * @brief Get neighbors at specified distance
 *
 * Returns a cluster of all atoms at the given distance from this atom.
 * Distance 1 returns immediate neighbors, distance 2 returns neighbors
 * of neighbors, etc.
 *
 * @param atom Starting atom
 * @param distance Distance to traverse
 * @return Cluster of atoms at distance, or NULL on failure
 *
 * @threadsafe
 *
 * @note Caller must free returned cluster with ak_atom_cluster_free()
 */
ak_atom_cluster_t *ak_atom_neighbors(ak_atom_t *atom, size_t distance);

/**
 * @brief Create a new atom cluster
 *
 * Allocates a cluster with the specified initial capacity.
 *
 * @param initial_capacity Initial capacity
 * @return Pointer to new cluster, or NULL on failure
 *
 * @notthreadsafe
 *
 * @note Caller must free with ak_atom_cluster_free()
 */
ak_atom_cluster_t *ak_atom_cluster_new(size_t initial_capacity);

/**
 * @brief Free an atom cluster
 *
 * Releases cluster memory. Does NOT free the atoms themselves.
 *
 * @param cluster Cluster to free (NULL is safe)
 *
 * @notthreadsafe
 */
void ak_atom_cluster_free(ak_atom_cluster_t *cluster);

/**
 * @brief Add atom to cluster
 *
 * Appends an atom to the cluster, resizing if necessary.
 *
 * @param cluster Cluster to modify
 * @param atom Atom to add
 * @return 0 on success, -1 on failure
 *
 * @notthreadsafe
 */
int ak_atom_cluster_add(ak_atom_cluster_t *cluster, ak_atom_t *atom);

/**
 * @brief Get atom from cluster by index
 *
 * @param cluster Cluster to query
 * @param index Index of atom
 * @return Pointer to atom, or NULL if index out of bounds
 *
 * @notthreadsafe
 */
ak_atom_t *ak_atom_cluster_get(ak_atom_cluster_t *cluster, size_t index);

/**
 * @brief Get number of atoms in cluster
 *
 * @param cluster Cluster to query
 * @return Number of atoms
 *
 * @notthreadsafe
 */
size_t ak_atom_cluster_count(ak_atom_cluster_t *cluster);

#endif
