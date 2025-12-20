/**
 * @file list.h
 * @brief Type-safe generic dynamic array using C macro templates
 *
 * Provides a macro-based generic list implementation that maintains type
 * safety through compile-time expansion. The list automatically resizes
 * as needed and supports common operations like push, pop, insert, and
 * iteration.
 *
 * Key features:
 * - Type-safe generic containers via macros
 * - Automatic capacity management and resizing
 * - Stack-like push/pop operations
 * - Random access with get/set
 * - Insert and remove at arbitrary positions
 * - Rotation operations
 * - Iterator pattern for traversal
 *
 * @note All operations are NOT thread-safe
 * @see list_t() macro for creating typed list instances
 */

#ifndef AK24_LIST_H
#define AK24_LIST_H

#include <string.h>

/**
 * @def AK24_LIST_VERSION
 * @brief List module version string
 */
#define LIST_VERSION "0.0.1-dev"

/**
 * @brief Internal base structure for list implementation
 *
 * Used internally by macro expansion. Users should not access this directly.
 */
typedef struct {
  void **items;      /**< Array of item pointers */
  unsigned capacity; /**< Total allocated capacity */
  unsigned count;    /**< Current number of items */
  int item_size;     /**< Size of each item in bytes */
} list_base_t;

/**
 * @brief Iterator state for list traversal
 *
 * Tracks position during iteration. Initialize with list_iter().
 */
typedef struct {
  unsigned index; /**< Current position */
  int valid;      /**< Whether iterator is valid */
} list_iter_t;

/**
 * @def list_t(T)
 * @brief Define a typed list structure
 *
 * Creates a list structure for the specified type. The resulting structure
 * should be initialized with list_init() before use.
 *
 * @param T Type of elements to store
 *
 * @par Example:
 * @code
 * list_t(int) my_list;
 * list_init(&my_list);
 * list_push(&my_list, 42);
 * list_deinit(&my_list);
 * @endcode
 */
#define list_t(T)                                                              \
  struct {                                                                     \
    list_base_t base; /**< Internal list state and bookkeeping */              \
    T *ref;           /**< Reference pointer for element access */             \
    T tmp;            /**< Temporary variable for type inference */            \
  }

/**
 * @def list_init(l)
 * @brief Initialize a list
 *
 * Must be called before using a list. Zeros the structure and sets up
 * internal bookkeeping.
 *
 * @param l Pointer to list to initialize
 */
#define list_init(l)                                                           \
  do {                                                                         \
    memset(l, 0, sizeof(*(l)));                                                \
    (l)->base.item_size = sizeof((l)->tmp);                                    \
  } while (0)

/**
 * @def list_deinit(l)
 * @brief Deinitialize a list
 *
 * Frees all internal memory. List must not be used after this call.
 *
 * @param l Pointer to list to deinitialize
 */
#define list_deinit(l) list_deinit_(&(l)->base)

/**
 * @def list_push(l, value)
 * @brief Push value onto end of list
 *
 * Adds an item to the end of the list, resizing if necessary.
 *
 * @param l Pointer to list
 * @param value Value to push
 * @return 0 on success, -1 on allocation failure
 */
#define list_push(l, value)                                                    \
  ((l)->tmp = (value), list_push_(&(l)->base, &(l)->tmp, sizeof((l)->tmp)))

/**
 * @def list_pop(l)
 * @brief Remove and return last item
 *
 * Removes the last item from the list and returns a pointer to it.
 *
 * @param l Pointer to list
 * @return Pointer to popped item, or NULL if list is empty
 */
#define list_pop(l) ((l)->ref = list_pop_(&(l)->base))

/**
 * @def list_get(l, index)
 * @brief Get item at index
 *
 * Returns a pointer to the item at the specified index.
 *
 * @param l Pointer to list
 * @param index Index of item to retrieve
 * @return Pointer to item, or NULL if index is out of bounds
 */
#define list_get(l, index) ((l)->ref = list_get_(&(l)->base, index))

/**
 * @def list_set(l, index, value)
 * @brief Set item at index
 *
 * Replaces the item at the specified index with a new value.
 *
 * @param l Pointer to list
 * @param index Index to set
 * @param value New value
 * @return 0 on success, -1 if index is out of bounds
 */
#define list_set(l, index, value)                                              \
  ((l)->tmp = (value),                                                         \
   list_set_(&(l)->base, index, &(l)->tmp, sizeof((l)->tmp)))

/**
 * @def list_insert(l, index, value)
 * @brief Insert value at index
 *
 * Inserts a new item at the specified index, shifting existing items right.
 *
 * @param l Pointer to list
 * @param index Index to insert at
 * @param value Value to insert
 * @return 0 on success, -1 on failure
 */
#define list_insert(l, index, value)                                           \
  ((l)->tmp = (value),                                                         \
   list_insert_(&(l)->base, index, &(l)->tmp, sizeof((l)->tmp)))

/**
 * @def list_remove(l, index)
 * @brief Remove item at index
 *
 * Removes the item at the specified index, shifting remaining items left.
 *
 * @param l Pointer to list
 * @param index Index to remove
 * @return 0 on success, -1 if index is out of bounds
 */
#define list_remove(l, index) list_remove_(&(l)->base, index)

/**
 * @def list_count(l)
 * @brief Get number of items in list
 *
 * @param l Pointer to list
 * @return Number of items
 */
#define list_count(l) ((l)->base.count)

/**
 * @def list_capacity(l)
 * @brief Get current capacity of list
 *
 * @param l Pointer to list
 * @return Current allocated capacity
 */
#define list_capacity(l) ((l)->base.capacity)

/**
 * @def list_clear(l)
 * @brief Remove all items from list
 *
 * Clears the list but maintains allocated capacity.
 *
 * @param l Pointer to list
 */
#define list_clear(l) list_clear_(&(l)->base)

/**
 * @def list_rotate_left(l, n)
 * @brief Rotate list contents left
 *
 * Moves first n items to the end of the list.
 *
 * @param l Pointer to list
 * @param n Number of positions to rotate
 */
#define list_rotate_left(l, n) list_rotate_left_(&(l)->base, n)

/**
 * @def list_rotate_right(l, n)
 * @brief Rotate list contents right
 *
 * Moves last n items to the beginning of the list.
 *
 * @param l Pointer to list
 * @param n Number of positions to rotate
 */
#define list_rotate_right(l, n) list_rotate_right_(&(l)->base, n)

/**
 * @def list_iter(l)
 * @brief Create a new iterator
 *
 * @param l Pointer to list
 * @return New iterator positioned at start
 */
#define list_iter(l) list_iter_()

/**
 * @def list_next(l, iter)
 * @brief Get next item from iterator
 *
 * Advances iterator and returns pointer to next item.
 *
 * @param l Pointer to list
 * @param iter Pointer to iterator
 * @return Pointer to next item, or NULL if at end
 *
 * @par Example:
 * @code
 * list_t(int) nums;
 * list_init(&nums);
 * list_push(&nums, 1);
 * list_push(&nums, 2);
 *
 * list_iter_t it = list_iter(&nums);
 * int *val;
 * while ((val = list_next(&nums, &it))) {
 *   printf("%d\n", *val);
 * }
 *
 * list_deinit(&nums);
 * @endcode
 */
#define list_next(l, iter) ((l)->ref = list_next_(&(l)->base, iter))

/**
 * @brief Internal deinit implementation
 * @notthreadsafe
 */
void list_deinit_(list_base_t *l);

/**
 * @brief Internal push implementation
 * @notthreadsafe
 */
int list_push_(list_base_t *l, void *value, int vsize);

/**
 * @brief Internal pop implementation
 * @notthreadsafe
 */
void *list_pop_(list_base_t *l);

/**
 * @brief Internal get implementation
 * @notthreadsafe
 */
void *list_get_(list_base_t *l, unsigned index);

/**
 * @brief Internal set implementation
 * @notthreadsafe
 */
int list_set_(list_base_t *l, unsigned index, void *value, int vsize);

/**
 * @brief Internal insert implementation
 * @notthreadsafe
 */
int list_insert_(list_base_t *l, unsigned index, void *value, int vsize);

/**
 * @brief Internal remove implementation
 * @notthreadsafe
 */
int list_remove_(list_base_t *l, unsigned index);

/**
 * @brief Internal clear implementation
 * @notthreadsafe
 */
void list_clear_(list_base_t *l);

/**
 * @brief Internal rotate left implementation
 * @notthreadsafe
 */
int list_rotate_left_(list_base_t *l, unsigned n);

/**
 * @brief Internal rotate right implementation
 * @notthreadsafe
 */
int list_rotate_right_(list_base_t *l, unsigned n);

/**
 * @brief Internal iterator creation
 * @notthreadsafe
 */
list_iter_t list_iter_(void);

/**
 * @brief Internal iterator next implementation
 * @notthreadsafe
 */
void *list_next_(list_base_t *l, list_iter_t *iter);

/**
 * @brief Predefined list type for void pointers
 */
typedef list_t(void *) list_void_t;

/**
 * @brief Predefined list type for integers
 */
typedef list_t(int) list_int_t;

/**
 * @brief Predefined list type for strings
 */
typedef list_t(char *) list_str_t;

/**
 * @brief Predefined list type for floats
 */
typedef list_t(float) list_float_t;

/**
 * @brief Predefined list type for doubles
 */
typedef list_t(double) list_double_t;

#endif
