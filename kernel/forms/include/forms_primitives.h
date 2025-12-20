/**
 * @file forms_primitives.h
 * @brief Form instances and built-in primitive forms with affordances
 *
 * Provides runtime instances of forms and built-in primitive type forms
 * with standard affordances. Includes factory functions for common types
 * and built-in behavioral affordances for optional, list, and map forms.
 *
 * Key features:
 * - Form instance wrappers for runtime values
 * - Built-in primitive form factories
 * - Root form context for primitive management
 * - Standard affordances (is_none, is_some, append, etc.)
 * - Optional, repeatable, list, and map instances
 *
 * @note All operations are NOT thread-safe
 */

#ifndef AK24_FORMS_PRIMITIVES_H
#define AK24_FORMS_PRIMITIVES_H

#include "forms.h"

/**
 * @brief Generic form instance
 *
 * Associates a form type with runtime data.
 */
typedef struct {
  ak_form_t *form; /**< Form type definition */
  void *data;      /**< Runtime data */
} ak_form_instance_t;

/**
 * @brief Optional form instance
 *
 * Represents an optional value that may be present or absent.
 */
typedef struct {
  ak_form_t *form; /**< Form type definition */
  int is_present;  /**< Whether value is present */
  void *value;     /**< Value if present (NULL if absent) */
} ak_optional_instance_t;

/**
 * @brief Repeatable form instance
 *
 * Represents a repeating sequence of values.
 */
typedef struct {
  ak_form_t *form;   /**< Form type definition */
  list_void_t items; /**< List of repeated items */
} ak_repeatable_instance_t;

/**
 * @brief List form instance
 *
 * Represents a homogeneous list of elements.
 */
typedef struct {
  ak_form_t *form;   /**< Form type definition */
  list_void_t items; /**< List of elements */
} ak_list_instance_t;

/**
 * @brief Map form instance
 *
 * Represents a key-value mapping.
 */
typedef struct {
  ak_form_t *form;  /**< Form type definition */
  map_void_t items; /**< Map of key-value pairs */
} ak_map_instance_t;

/**
 * @brief Root form context
 *
 * Manages built-in primitive forms in a context hierarchy.
 */
typedef struct {
  ak_context_t *ctx; /**< Context containing primitives */
} root_form_ctx_t;

/**
 * @brief Create a new root form context
 *
 * Initializes a context with all built-in primitive forms registered.
 *
 * @return Pointer to new root context, or NULL on failure
 *
 * @notthreadsafe
 *
 * @note Caller must free with ak_root_form_ctx_free()
 */
root_form_ctx_t *ak_root_form_ctx_new(void);

/**
 * @brief Free a root form context
 *
 * @param root Root context to free (NULL is safe)
 *
 * @notthreadsafe
 */
void ak_root_form_ctx_free(root_form_ctx_t *root);

/**
 * @brief Get bool form from root context
 * @notthreadsafe
 */
ak_form_t *ak_root_form_ctx_get_bool(root_form_ctx_t *root);

/**
 * @brief Get u8 form from root context
 * @notthreadsafe
 */
ak_form_t *ak_root_form_ctx_get_u8(root_form_ctx_t *root);

/**
 * @brief Get u16 form from root context
 * @notthreadsafe
 */
ak_form_t *ak_root_form_ctx_get_u16(root_form_ctx_t *root);

/**
 * @brief Get u32 form from root context
 * @notthreadsafe
 */
ak_form_t *ak_root_form_ctx_get_u32(root_form_ctx_t *root);

/**
 * @brief Get u64 form from root context
 * @notthreadsafe
 */
ak_form_t *ak_root_form_ctx_get_u64(root_form_ctx_t *root);

/**
 * @brief Get i8 form from root context
 * @notthreadsafe
 */
ak_form_t *ak_root_form_ctx_get_i8(root_form_ctx_t *root);

/**
 * @brief Get i16 form from root context
 * @notthreadsafe
 */
ak_form_t *ak_root_form_ctx_get_i16(root_form_ctx_t *root);

/**
 * @brief Get i32 form from root context
 * @notthreadsafe
 */
ak_form_t *ak_root_form_ctx_get_i32(root_form_ctx_t *root);

/**
 * @brief Get i64 form from root context
 * @notthreadsafe
 */
ak_form_t *ak_root_form_ctx_get_i64(root_form_ctx_t *root);

/**
 * @brief Get f32 form from root context
 * @notthreadsafe
 */
ak_form_t *ak_root_form_ctx_get_f32(root_form_ctx_t *root);

/**
 * @brief Get f64 form from root context
 * @notthreadsafe
 */
ak_form_t *ak_root_form_ctx_get_f64(root_form_ctx_t *root);

/**
 * @brief Create bool primitive form
 * @notthreadsafe
 */
ak_form_t *ak_primitive_bool(void);

/**
 * @brief Create u8 primitive form
 * @notthreadsafe
 */
ak_form_t *ak_primitive_u8(void);

/**
 * @brief Create u16 primitive form
 * @notthreadsafe
 */
ak_form_t *ak_primitive_u16(void);

/**
 * @brief Create u32 primitive form
 * @notthreadsafe
 */
ak_form_t *ak_primitive_u32(void);

/**
 * @brief Create u64 primitive form
 * @notthreadsafe
 */
ak_form_t *ak_primitive_u64(void);

/**
 * @brief Create i8 primitive form
 * @notthreadsafe
 */
ak_form_t *ak_primitive_i8(void);

/**
 * @brief Create i16 primitive form
 * @notthreadsafe
 */
ak_form_t *ak_primitive_i16(void);

/**
 * @brief Create i32 primitive form
 * @notthreadsafe
 */
ak_form_t *ak_primitive_i32(void);

/**
 * @brief Create i64 primitive form
 * @notthreadsafe
 */
ak_form_t *ak_primitive_i64(void);

/**
 * @brief Create f32 primitive form
 * @notthreadsafe
 */
ak_form_t *ak_primitive_f32(void);

/**
 * @brief Create f64 primitive form
 * @notthreadsafe
 */
ak_form_t *ak_primitive_f64(void);

/**
 * @brief Attach built-in affordances to form
 *
 * Adds standard affordances based on form kind.
 *
 * @param form Form to modify
 *
 * @notthreadsafe
 */
void ak_form_attach_builtin_affects(ak_form_t *form);

/**
 * @brief Get is_none affordance lambda
 *
 * Returns lambda that checks if optional is absent.
 *
 * @return Lambda for is_none check
 *
 * @notthreadsafe
 */
ak_lambda_t *ak_builtin_is_none(void);

/**
 * @brief Get is_some affordance lambda
 *
 * Returns lambda that checks if optional is present.
 *
 * @return Lambda for is_some check
 *
 * @notthreadsafe
 */
ak_lambda_t *ak_builtin_is_some(void);

/**
 * @brief Get append affordance lambda
 *
 * Returns lambda that appends to repeatable.
 *
 * @return Lambda for append operation
 *
 * @notthreadsafe
 */
ak_lambda_t *ak_builtin_append(void);

/**
 * @brief Get pop affordance lambda
 *
 * Returns lambda that removes last item.
 *
 * @return Lambda for pop operation
 *
 * @notthreadsafe
 */
ak_lambda_t *ak_builtin_pop(void);

/**
 * @brief Get push affordance lambda
 *
 * Returns lambda that adds item to end.
 *
 * @return Lambda for push operation
 *
 * @notthreadsafe
 */
ak_lambda_t *ak_builtin_push(void);

/**
 * @brief Get rest affordance lambda
 *
 * Returns lambda that gets all items except first.
 *
 * @return Lambda for rest operation
 *
 * @notthreadsafe
 */
ak_lambda_t *ak_builtin_rest(void);

/**
 * @brief Get list index affordance lambda
 *
 * Returns lambda that accesses list by index.
 *
 * @return Lambda for index operation
 *
 * @notthreadsafe
 */
ak_lambda_t *ak_builtin_list_index(void);

/**
 * @brief Get list length affordance lambda
 *
 * Returns lambda that gets list length.
 *
 * @return Lambda for length operation
 *
 * @notthreadsafe
 */
ak_lambda_t *ak_builtin_list_length(void);

/**
 * @brief Get map get affordance lambda
 *
 * Returns lambda that retrieves value by key.
 *
 * @return Lambda for map get operation
 *
 * @notthreadsafe
 */
ak_lambda_t *ak_builtin_map_get(void);

/**
 * @brief Get map set affordance lambda
 *
 * Returns lambda that sets key-value pair.
 *
 * @return Lambda for map set operation
 *
 * @notthreadsafe
 */
ak_lambda_t *ak_builtin_map_set(void);

/**
 * @brief Get map has affordance lambda
 *
 * Returns lambda that checks if key exists.
 *
 * @return Lambda for map has operation
 *
 * @notthreadsafe
 */
ak_lambda_t *ak_builtin_map_has(void);

/**
 * @brief Create optional instance
 *
 * @param form Form type for optional
 * @return Pointer to new instance, or NULL on failure
 *
 * @notthreadsafe
 *
 * @note Caller must free with ak_optional_instance_free()
 */
ak_optional_instance_t *ak_optional_instance_new(ak_form_t *form);

/**
 * @brief Free optional instance
 *
 * @param inst Instance to free (NULL is safe)
 *
 * @notthreadsafe
 */
void ak_optional_instance_free(ak_optional_instance_t *inst);

/**
 * @brief Set optional instance value
 *
 * @param inst Instance to modify
 * @param value Value to set (NULL to clear)
 *
 * @notthreadsafe
 */
void ak_optional_instance_set(ak_optional_instance_t *inst, void *value);

/**
 * @brief Get optional instance value
 *
 * @param inst Instance to query
 * @return Value if present, NULL otherwise
 *
 * @notthreadsafe
 */
void *ak_optional_instance_get(ak_optional_instance_t *inst);

/**
 * @brief Create repeatable instance
 *
 * @param form Form type for repeatable
 * @return Pointer to new instance, or NULL on failure
 *
 * @notthreadsafe
 *
 * @note Caller must free with ak_repeatable_instance_free()
 */
ak_repeatable_instance_t *ak_repeatable_instance_new(ak_form_t *form);

/**
 * @brief Free repeatable instance
 *
 * @param inst Instance to free (NULL is safe)
 *
 * @notthreadsafe
 */
void ak_repeatable_instance_free(ak_repeatable_instance_t *inst);

/**
 * @brief Create list instance
 *
 * @param form Form type for list
 * @return Pointer to new instance, or NULL on failure
 *
 * @notthreadsafe
 *
 * @note Caller must free with ak_list_instance_free()
 */
ak_list_instance_t *ak_list_instance_new(ak_form_t *form);

/**
 * @brief Free list instance
 *
 * @param inst Instance to free (NULL is safe)
 *
 * @notthreadsafe
 */
void ak_list_instance_free(ak_list_instance_t *inst);

/**
 * @brief Create map instance
 *
 * @param form Form type for map
 * @return Pointer to new instance, or NULL on failure
 *
 * @notthreadsafe
 *
 * @note Caller must free with ak_map_instance_free()
 */
ak_map_instance_t *ak_map_instance_new(ak_form_t *form);

/**
 * @brief Free map instance
 *
 * @param inst Instance to free (NULL is safe)
 *
 * @notthreadsafe
 */
void ak_map_instance_free(ak_map_instance_t *inst);

#endif
