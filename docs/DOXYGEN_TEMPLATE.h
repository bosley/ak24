/**
 * @file module_name.h
 * @brief One-line description of what this module does
 *
 * Longer description explaining the purpose of this module, its design,
 * and any important implementation details. Mention algorithms, data
 * structures, or papers that influenced the design.
 *
 * Key features:
 * - Feature or capability 1
 * - Feature or capability 2
 * - Feature or capability 3
 *
 * @note Important notes about usage or limitations
 * @see Related modules or external references
 */

#ifndef AK24_MODULE_NAME_H
#define AK24_MODULE_NAME_H

#include <stddef.h>

#define AK24_MODULE_VERSION "0.1.0"

/**
 * @brief Brief description of this structure
 *
 * Detailed explanation of what this structure represents,
 * how it's used, and any invariants that must be maintained.
 */
typedef struct {
  int field1;   /**< Description of field1 */
  char *field2; /**< Description of field2 */
  size_t count; /**< Number of items */
} ak_module_data_t;

/**
 * @brief Main structure for this module
 *
 * This is the opaque handle users interact with. Explain
 * what it represents and its lifecycle.
 */
typedef struct ak_module_t {
  ak_module_data_t *data; /**< Internal data */
  size_t capacity;        /**< Maximum capacity */
} ak_module_t;

/**
 * @brief Create a new module instance
 *
 * Allocates and initializes a new module instance with the
 * specified parameters. Explain any transformations to parameters
 * (e.g., rounding, validation).
 *
 * @param param1 Description of first parameter and valid values
 * @param param2 Description of second parameter and constraints
 * @return Pointer to new instance, or NULL on failure
 *
 * @threadsafe
 *
 * @note Caller is responsible for freeing with ak_module_free()
 *
 * @par Example:
 * @code
 * ak_module_t *mod = ak_module_new(100, "config");
 * if (!mod) {
 *   fprintf(stderr, "Failed to create module\n");
 *   return -1;
 * }
 * @endcode
 */
ak_module_t *ak_module_new(size_t param1, const char *param2);

/**
 * @brief Free a module instance
 *
 * Releases all resources associated with the module. Does not
 * free items stored in the module - caller must handle cleanup.
 *
 * @param module Module to free (NULL is safe)
 *
 * @notthreadsafe
 * @warning Ensure no concurrent operations before calling
 */
void ak_module_free(ak_module_t *module);

/**
 * @brief Perform an operation on the module
 *
 * Detailed description of what this operation does, including
 * any side effects, state changes, or special behavior.
 *
 * @param module Module to operate on
 * @param item Item to process
 * @return 0 on success, negative error code on failure
 * @retval 0 Success
 * @retval -1 Invalid parameters
 * @retval -2 Operation failed
 *
 * @threadsafe
 * @lockfree
 *
 * @note Any important notes about behavior
 * @warning Any warnings about edge cases or misuse
 *
 * @par Example:
 * @code
 * int result = ak_module_operation(mod, item);
 * if (result != 0) {
 *   fprintf(stderr, "Operation failed: %d\n", result);
 * }
 * @endcode
 */
int ak_module_operation(ak_module_t *module, void *item);

/**
 * @brief Query module state
 *
 * Returns information about the current state of the module.
 * Explain if the result can be stale in concurrent scenarios.
 *
 * @param module Module to query
 * @return Current state value, or 0 if module is NULL
 *
 * @threadsafe
 *
 * @note Result may be stale in concurrent scenarios
 */
size_t ak_module_get_state(ak_module_t *module);

/**
 * @brief Check a condition
 *
 * Returns whether a specific condition is true for the module.
 *
 * @param module Module to check
 * @return 1 if condition is true, 0 if false or module is NULL
 *
 * @threadsafe
 */
int ak_module_is_condition(ak_module_t *module);

#endif
