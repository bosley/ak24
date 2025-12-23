/**
 * @file box.h
 * @brief Thread-safe container for atomic operations on any type
 *
 * Provides a box container that wraps any pointer and enables
 * atomic operations through mutex-protected access. The box does
 * NOT own the contained data - callers are responsible for memory
 * management of the boxed value.
 *
 * Key features:
 * - Atomic swap of contained data
 * - Lambda-based visitor pattern with automatic locking
 * - Thread-safe access to any pointer type
 * - Non-owning semantics (does not free contained data)
 *
 * @note All operations are thread-safe
 */

#ifndef AK24_BOX_H
#define AK24_BOX_H

#include "lambda.h"

/**
 * @def AK24_BOX_VERSION
 * @brief Box module version string
 */
#define AK24_BOX_VERSION "0.0.1-dev"

/**
 * @brief Opaque box structure
 *
 * Contains a pointer and mutex for thread-safe operations.
 * Created with ak_box_new() and freed with ak_box_free().
 */
typedef struct ak_box_t ak_box_t;

/**
 * @brief Create a new box
 *
 * Allocates and initializes a box containing the provided data pointer.
 * The box does NOT take ownership of the data - the caller is responsible
 * for freeing the data separately.
 *
 * @param data Pointer to wrap in the box (can be NULL)
 * @return Pointer to new box, or NULL on allocation failure
 *
 * @threadsafe
 *
 * @note Caller must free with ak_box_free()
 * @note The box does NOT own the data - ak_box_free() will NOT free data
 *
 * @par Example:
 * @code
 * int *value = malloc(sizeof(int));
 * *value = 42;
 * ak_box_t *box = ak_box_new(value);
 *
 * // ... use box ...
 *
 * ak_box_free(box);
 * free(value);  // caller must free data
 * @endcode
 */
ak_box_t *ak_box_new(void *data);

/**
 * @brief Free a box
 *
 * Releases the box structure. Does NOT free the contained data.
 *
 * @param box Box to free (NULL is safe)
 *
 * @threadsafe
 *
 * @warning Does NOT free the data pointer - caller must manage data lifetime
 */
void ak_box_free(ak_box_t *box);

/**
 * @brief Atomically swap the contained data
 *
 * Replaces the boxed data with new data and returns the old data.
 * The operation is atomic with respect to other box operations.
 *
 * @param box Box to modify
 * @param new_data New data to store in the box
 * @return Previous data pointer, or NULL if box is NULL
 *
 * @threadsafe
 *
 * @par Example:
 * @code
 * int *old_value = (int *)ak_box_swap(box, new_value);
 * free(old_value);  // caller responsible for old data
 * @endcode
 */
void *ak_box_swap(ak_box_t *box, void *new_data);

/**
 * @brief Get the current data without locking
 *
 * Returns the current data pointer. For thread-safe access with
 * operations, use ak_box_visit() instead.
 *
 * @param box Box to query
 * @return Current data pointer, or NULL if box is NULL
 *
 * @threadsafe (read only)
 *
 * @warning The returned pointer may become stale if another thread
 *          swaps the data. Use ak_box_visit() for safe operations.
 */
void *ak_box_get(ak_box_t *box);

/**
 * @brief Visit the box with a lambda under lock
 *
 * Locks the box, invokes the lambda with the boxed data as the
 * invoke_args, then unlocks the box. This ensures the lambda
 * executes atomically with respect to other box operations.
 *
 * The lambda receives:
 * - captured_ctx: The lambda's captured context (set at lambda creation)
 * - invoke_args: The boxed data pointer
 *
 * @param box Box to visit
 * @param lambda Lambda to invoke with boxed data
 * @return 0 on success, -1 if box or lambda is NULL
 *
 * @threadsafe
 *
 * @par Example:
 * @code
 * void increment_fn(void *captured, void *args) {
 *   (void)captured;
 *   int *value = (int *)args;
 *   (*value)++;
 * }
 *
 * ak_lambda_t *lambda = ak_lambda_new(increment_fn, NULL, NULL);
 * ak_box_visit(box, lambda);
 * ak_lambda_free(lambda);
 * @endcode
 */
int ak_box_visit(ak_box_t *box, ak_lambda_t *lambda);

#endif // AK24_BOX_H
