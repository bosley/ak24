/**
 * @file context.h
 * @brief Hierarchical scoped context with variable hoisting
 *
 * Provides a lexical scoping system with parent-child relationships for
 * managing variables and data across nested contexts. Supports variable
 * hoisting for promoting local variables to parent scopes.
 *
 * Key features:
 * - Hierarchical parent-child context chains
 * - Automatic parent scope lookup
 * - Variable hoisting to parent contexts
 * - Local and global variable access
 * - Context depth tracking
 * - Stack-like push/pop operations
 *
 * @note All operations are NOT thread-safe
 */

#ifndef AK24_CONTEXT_H
#define AK24_CONTEXT_H

#include "list.h"
#include "map.h"

/**
 * @def AK24_CONTEXT_VERSION
 * @brief Context module version string
 */
#define CONTEXT_VERSION "0.0.1-dev"

/**
 * @brief Hierarchical context structure
 *
 * Represents a scope with parent linkage for variable resolution.
 * Variables are stored in a map and can be hoisted to parent contexts.
 */
typedef struct ak_context_t {
  struct ak_context_t *parent; /**< Parent context (NULL for root) */
  map_void_t data;             /**< Key-value storage for this scope */
  list_str_t hoist_queue;      /**< Queue of keys to hoist to parent */
} ak_context_t;

/**
 * @brief Create a new root context
 *
 * Allocates a new context with no parent.
 *
 * @return Pointer to new context, or NULL on allocation failure
 *
 * @notthreadsafe
 *
 * @note Caller must free with ak_context_free()
 *
 * @par Example:
 * @code
 * ak_context_t *global = ak_context_new();
 * ak_context_set(global, "version", "1.0");
 * @endcode
 */
ak_context_t *ak_context_new(void);

/**
 * @brief Push a new child context
 *
 * Creates a new context with the given context as its parent.
 *
 * @param ctx Parent context (can be NULL)
 * @return Pointer to new child context, or NULL on failure
 *
 * @notthreadsafe
 *
 * @par Example:
 * @code
 * ak_context_t *global = ak_context_new();
 * ak_context_t *local = ak_context_push(global);
 * @endcode
 */
ak_context_t *ak_context_push(ak_context_t *ctx);

/**
 * @brief Pop context and return parent
 *
 * Frees the current context and returns its parent. Processes any
 * pending hoists before freeing.
 *
 * @param ctx Context to pop
 * @return Parent context, or NULL if ctx was root
 *
 * @notthreadsafe
 *
 * @warning Context is freed and must not be used after this call
 */
ak_context_t *ak_context_pop(ak_context_t *ctx);

/**
 * @brief Set value in context
 *
 * Stores a key-value pair in the current context scope.
 *
 * @param ctx Context to modify
 * @param key Key string
 * @param value Value to store
 * @return 0 on success, -1 on failure
 *
 * @notthreadsafe
 */
int ak_context_set(ak_context_t *ctx, const char *key, void *value);

/**
 * @brief Get value from context or parent chain
 *
 * Searches for key in current context, then walks up parent chain
 * until found or root is reached.
 *
 * @param ctx Context to search
 * @param key Key to look up
 * @return Pointer to value, or NULL if not found
 *
 * @notthreadsafe
 */
void *ak_context_get(ak_context_t *ctx, const char *key);

/**
 * @brief Get value from local context only
 *
 * Searches only the current context without checking parents.
 *
 * @param ctx Context to search
 * @param key Key to look up
 * @return Pointer to value, or NULL if not found locally
 *
 * @notthreadsafe
 */
void *ak_context_get_local(ak_context_t *ctx, const char *key);

/**
 * @brief Find which context contains key
 *
 * Walks up the parent chain to find which context contains the key.
 *
 * @param ctx Starting context
 * @param key Key to search for
 * @return Context containing the key, or NULL if not found
 *
 * @notthreadsafe
 */
ak_context_t *ak_context_get_containing_context(ak_context_t *ctx,
                                                const char *key);

/**
 * @brief Mark key for hoisting to parent
 *
 * Queues a key to be moved to the parent context when this context
 * is popped. The value will be promoted up one level.
 *
 * @param ctx Context containing the key
 * @param key Key to hoist
 * @return 0 on success, -1 on failure
 *
 * @notthreadsafe
 *
 * @note Hoist is processed during ak_context_pop()
 */
int ak_context_hoist(ak_context_t *ctx, const char *key);

/**
 * @brief Free context and all children
 *
 * Recursively frees the context. Does NOT process hoists.
 *
 * @param ctx Context to free (NULL is safe)
 *
 * @notthreadsafe
 *
 * @warning Use ak_context_pop() instead to properly handle hoists
 */
void ak_context_free(ak_context_t *ctx);

/**
 * @brief Check if key exists in context or parents
 *
 * Searches current context and parent chain.
 *
 * @param ctx Context to search
 * @param key Key to check
 * @return 1 if found, 0 otherwise
 *
 * @notthreadsafe
 */
int ak_context_has(ak_context_t *ctx, const char *key);

/**
 * @brief Check if key exists in local context only
 *
 * Searches only the current context.
 *
 * @param ctx Context to search
 * @param key Key to check
 * @return 1 if found locally, 0 otherwise
 *
 * @notthreadsafe
 */
int ak_context_has_local(ak_context_t *ctx, const char *key);

/**
 * @brief Get depth of context in hierarchy
 *
 * Counts the number of parent links from this context to root.
 *
 * @param ctx Context to measure
 * @return Depth (0 for root context)
 *
 * @notthreadsafe
 */
unsigned ak_context_depth(ak_context_t *ctx);

#endif
