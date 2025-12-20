/**
 * @file forms.h
 * @brief Type system with structural patterns and behavioral affordances
 *
 * Provides a sophisticated type system supporting primitive types, compound
 * structures, optional and repeatable patterns, and behavioral affordances
 * (affects) that can be attached to forms. Forms can be registered in contexts
 * for name resolution and pattern matching.
 *
 * Key features:
 * - Primitive and compound type definitions
 * - Optional and repeatable type modifiers
 * - Struct, list, and map collection types
 * - Named type aliases
 * - Behavioral affordances (affects) with lambdas
 * - Pattern matching and compatibility checking
 * - Context-based type registration and lookup
 *
 * @note All operations are NOT thread-safe
 */

#ifndef AK24_FORMS_H
#define AK24_FORMS_H

#include "context.h"
#include "lambda.h"
#include "list.h"

/**
 * @def AK24_FORMS_VERSION
 * @brief Forms module version string
 */
#define FORMS_VERSION "0.0.1-dev"

/**
 * @brief Primitive type enumeration
 *
 * Basic atomic types that can be used as form primitives.
 */
typedef enum {
  AK_FORM_PRIMITIVE_BYTE, /**< Unsigned byte */
  AK_FORM_PRIMITIVE_U8,   /**< Unsigned 8-bit integer */
  AK_FORM_PRIMITIVE_U16,  /**< Unsigned 16-bit integer */
  AK_FORM_PRIMITIVE_U32,  /**< Unsigned 32-bit integer */
  AK_FORM_PRIMITIVE_U64,  /**< Unsigned 64-bit integer */
  AK_FORM_PRIMITIVE_I8,   /**< Signed 8-bit integer */
  AK_FORM_PRIMITIVE_I16,  /**< Signed 16-bit integer */
  AK_FORM_PRIMITIVE_I32,  /**< Signed 32-bit integer */
  AK_FORM_PRIMITIVE_I64,  /**< Signed 64-bit integer */
  AK_FORM_PRIMITIVE_F32,  /**< 32-bit floating point */
  AK_FORM_PRIMITIVE_F64,  /**< 64-bit floating point */
  AK_FORM_PRIMITIVE_CHAR, /**< Character */
} ak_form_primitive_type_e;

/**
 * @brief Form kind enumeration
 *
 * Categorizes the structural pattern of a form.
 */
typedef enum {
  AK_FORM_PRIMITIVE,  /**< Single primitive type */
  AK_FORM_COMPOUND,   /**< Multiple forms combined */
  AK_FORM_OPTIONAL,   /**< Form that may be absent */
  AK_FORM_REPEATABLE, /**< Form that can repeat */
  AK_FORM_STRUCT,     /**< Structured data with named fields */
  AK_FORM_LIST,       /**< Homogeneous list of elements */
  AK_FORM_MAP,        /**< Key-value mapping */
  AK_FORM_NAMED       /**< Named alias for another form */
} ak_form_kind_e;

typedef struct ak_form_s ak_form_t;
typedef struct ak_affect_s ak_affect_t;

/**
 * @brief Behavioral affordance attached to a form
 *
 * Represents a behavior (affect) that can be performed on instances of
 * a form, with associated parameter and return types.
 */
struct ak_affect_s {
  char *name;                  /**< Name of the affordance */
  ak_lambda_t *lambda;         /**< Implementation function */
  list_void_t parameter_forms; /**< Parameter type forms */
  ak_form_t *return_form;      /**< Return type form */
  list_str_t also_includes;    /**< Additional affordances included */
};

/**
 * @brief Form type definition
 *
 * Represents a type with structural pattern and optional behavioral
 * affordances.
 */
struct ak_form_s {
  ak_form_kind_e kind;                  /**< Kind of form */
  char *name;                           /**< Optional name */
  union {                               /**< Form-specific data based on kind */
    ak_form_primitive_type_e primitive; /**< Primitive type */
    list_void_t compound_parts;         /**< Compound form parts */
    struct {
      ak_form_t *inner; /**< Inner optional form */
    } optional;
    struct {
      ak_form_t *inner; /**< Inner repeatable form */
    } repeatable;
    struct {
      ak_form_t *pattern;     /**< Struct pattern */
      list_str_t field_names; /**< Field names */
    } struct_form;
    struct {
      ak_form_t *element_type; /**< List element type */
    } list_form;
    struct {
      ak_form_primitive_type_e key_type; /**< Map key type */
      ak_form_t *value_type;             /**< Map value type */
    } map_form;
    struct {
      ak_form_t *actual_form; /**< Aliased form */
    } named;
  } data;
  list_void_t affects; /**< Behavioral affordances */
};

/**
 * @brief Create a primitive form
 *
 * @param type Primitive type
 * @return Pointer to new form, or NULL on failure
 *
 * @notthreadsafe
 *
 * @note Caller must free with ak_form_free()
 */
ak_form_t *ak_form_new_primitive(ak_form_primitive_type_e type);

/**
 * @brief Create a compound form
 *
 * Combines multiple forms into a compound structure.
 *
 * @param parts List of component forms
 * @return Pointer to new form, or NULL on failure
 *
 * @notthreadsafe
 */
ak_form_t *ak_form_new_compound(list_void_t *parts);

/**
 * @brief Create an optional form
 *
 * Wraps a form to indicate it may be absent.
 *
 * @param inner Form that is optional
 * @return Pointer to new form, or NULL on failure
 *
 * @notthreadsafe
 */
ak_form_t *ak_form_new_optional(ak_form_t *inner);

/**
 * @brief Create a repeatable form
 *
 * Wraps a form to indicate it can repeat zero or more times.
 *
 * @param inner Form that can repeat
 * @return Pointer to new form, or NULL on failure
 *
 * @notthreadsafe
 */
ak_form_t *ak_form_new_repeatable(ak_form_t *inner);

/**
 * @brief Create a struct form
 *
 * Creates a structured form with named fields.
 *
 * @param pattern Pattern defining structure
 * @param field_names Names for fields
 * @return Pointer to new form, or NULL on failure
 *
 * @notthreadsafe
 */
ak_form_t *ak_form_new_struct(ak_form_t *pattern, list_str_t *field_names);

/**
 * @brief Create a list form
 *
 * Creates a homogeneous list type.
 *
 * @param element_type Type of list elements
 * @return Pointer to new form, or NULL on failure
 *
 * @notthreadsafe
 */
ak_form_t *ak_form_new_list(ak_form_t *element_type);

/**
 * @brief Create a map form
 *
 * Creates a key-value mapping type.
 *
 * @param key_type Type of keys (must be primitive)
 * @param value_type Type of values
 * @return Pointer to new form, or NULL on failure
 *
 * @notthreadsafe
 */
ak_form_t *ak_form_new_map(ak_form_primitive_type_e key_type,
                           ak_form_t *value_type);

/**
 * @brief Create a named form alias
 *
 * Creates a named reference to another form.
 *
 * @param name Name for the form
 * @param form Form to alias
 * @return Pointer to new form, or NULL on failure
 *
 * @notthreadsafe
 */
ak_form_t *ak_form_new_named(const char *name, ak_form_t *form);

/**
 * @brief Free a form
 *
 * Releases form and all nested forms.
 *
 * @param form Form to free (NULL is safe)
 *
 * @notthreadsafe
 */
void ak_form_free(ak_form_t *form);

/**
 * @brief Register form in context
 *
 * Associates a name with a form in the given context.
 *
 * @param ctx Context for registration
 * @param name Name to register
 * @param form Form to register
 * @return 0 on success, -1 on failure
 *
 * @notthreadsafe
 */
int ak_form_register(ak_context_t *ctx, const char *name, ak_form_t *form);

/**
 * @brief Look up form by name in context chain
 *
 * Searches current context and parents for named form.
 *
 * @param ctx Context to search
 * @param name Name to look up
 * @return Pointer to form, or NULL if not found
 *
 * @notthreadsafe
 */
ak_form_t *ak_form_lookup(ak_context_t *ctx, const char *name);

/**
 * @brief Look up form by name in local context only
 *
 * Searches only the given context without checking parents.
 *
 * @param ctx Context to search
 * @param name Name to look up
 * @return Pointer to form, or NULL if not found locally
 *
 * @notthreadsafe
 */
ak_form_t *ak_form_lookup_local(ak_context_t *ctx, const char *name);

/**
 * @brief Check if form matches pattern
 *
 * Tests whether a form conforms to a structural pattern.
 *
 * @param form Form to test
 * @param pattern Pattern to match against
 * @return 1 if matches, 0 otherwise
 *
 * @notthreadsafe
 */
int ak_form_matches_pattern(ak_form_t *form, ak_form_t *pattern);

/**
 * @brief Check if two forms are compatible
 *
 * Tests whether two forms can be used interchangeably.
 *
 * @param form1 First form
 * @param form2 Second form
 * @return 1 if compatible, 0 otherwise
 *
 * @notthreadsafe
 */
int ak_form_is_compatible(ak_form_t *form1, ak_form_t *form2);

/**
 * @brief Create a new affordance
 *
 * Creates a behavioral affordance with implementation and type signature.
 *
 * @param name Name of affordance
 * @param lambda Implementation function
 * @param params Parameter type forms
 * @param return_type Return type form
 * @return Pointer to new affordance, or NULL on failure
 *
 * @notthreadsafe
 *
 * @note Caller must free with ak_affect_free()
 */
ak_affect_t *ak_affect_new(const char *name, ak_lambda_t *lambda,
                           list_void_t *params, ak_form_t *return_type);

/**
 * @brief Free an affordance
 *
 * @param affect Affordance to free (NULL is safe)
 *
 * @notthreadsafe
 */
void ak_affect_free(ak_affect_t *affect);

/**
 * @brief Add affordance to form
 *
 * Attaches a behavioral affordance to a form.
 *
 * @param form Form to modify
 * @param affect Affordance to add
 * @return 0 on success, -1 on failure
 *
 * @notthreadsafe
 */
int ak_form_add_affect(ak_form_t *form, ak_affect_t *affect);

/**
 * @brief Get affordance from form by name
 *
 * @param form Form to query
 * @param name Name of affordance
 * @return Pointer to affordance, or NULL if not found
 *
 * @notthreadsafe
 */
ak_affect_t *ak_form_get_affect(ak_form_t *form, const char *name);

/**
 * @brief Check if form has affordance
 *
 * @param form Form to query
 * @param name Name of affordance
 * @return 1 if present, 0 otherwise
 *
 * @notthreadsafe
 */
int ak_form_has_affect(ak_form_t *form, const char *name);

/**
 * @brief Add also-includes relationship to affordance
 *
 * Indicates that an affordance also provides another affordance.
 *
 * @param affect Affordance to modify
 * @param other_name Name of included affordance
 * @return 0 on success, -1 on failure
 *
 * @notthreadsafe
 */
int ak_affect_add_also(ak_affect_t *affect, const char *other_name);

/**
 * @brief Check if form has affordance (including also-includes)
 *
 * Checks for affordance directly or via also-includes relationships.
 *
 * @param form Form to query
 * @param affordance_name Name of affordance
 * @return 1 if present, 0 otherwise
 *
 * @notthreadsafe
 */
int ak_form_has_affordance(ak_form_t *form, const char *affordance_name);

/**
 * @brief Get all affordance names from form
 *
 * Returns list of all affordances including also-includes.
 *
 * @param form Form to query
 * @return List of affordance names
 *
 * @notthreadsafe
 *
 * @note Caller must free returned list with list_deinit()
 */
list_str_t ak_form_get_affordances(ak_form_t *form);

#endif
