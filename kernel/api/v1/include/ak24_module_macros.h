/**
 * @file ak24_module_macros.h
 * @brief Versioned helper macros for AK24 module development
 *
 * Provides macro-based helpers to reduce boilerplate when implementing
 * AK24 modules. All macros are versioned (_V1) to allow API evolution
 * while maintaining backward compatibility.
 *
 * Usage:
 * 1. Declare module with AK24_MODULE_DECLARE_V1
 * 2. Implement init with AK24_MODULE_INIT_BEGIN_V1/END_V1
 * 3. Implement deinit with AK24_MODULE_DEINIT_BEGIN_V1/END_V1
 * 4. Register functions with AK24_MODULE_FUNCTION_TABLE_*_V1 macros
 */

#ifndef AK24_MODULE_MACROS_H
#define AK24_MODULE_MACROS_H

#include <interfaces.h>
#include <stdio.h>
#include <string.h>

//=============================================================================
// Core Module Declaration (V1)
//=============================================================================

/**
 * @brief Declare module metadata and boilerplate
 *
 * Generates:
 * - Static allocator pointer
 * - ak_module_version() implementation
 * - Module info strings
 * - ak_module_info() implementation
 *
 * @param _name Module name string
 * @param _version Module version string
 * @param _desc Module description string
 * @param _state_type The type of your module state struct
 */
#define AK24_MODULE_DECLARE_V1(_name, _version, _desc, _state_type)            \
  static ak_module_allocator_t *g_allocator = NULL;                            \
  typedef _state_type ak24_module_state_t;                                     \
                                                                               \
  int ak_module_version(void) { return AK24_MODULE_API_VERSION; }              \
                                                                               \
  const char *ak_module_info(const char *key) {                                \
    if (!key) {                                                                \
      return _name;                                                            \
    }                                                                          \
    if (strcmp(key, "name") == 0) {                                            \
      return _name;                                                            \
    } else if (strcmp(key, "version") == 0) {                                  \
      return _version;                                                         \
    } else if (strcmp(key, "description") == 0) {                              \
      return _desc;                                                            \
    }                                                                          \
    return NULL;                                                               \
  }

//=============================================================================
// Module Initialization Helpers (V1)
//=============================================================================

/**
 * @brief Begin module initialization function
 *
 * Generates ak_module_init signature and handles:
 * - Parameter validation
 * - State allocation
 * - Allocator storage
 *
 * User code has access to:
 * - state: pointer to allocated state struct
 * - allocator: the module allocator
 *
 * @param _state_type The type of your module state struct
 */
#define AK24_MODULE_INIT_BEGIN_V1(_state_type)                                 \
  ak_module_result_e ak_module_init(void **module_ctx,                         \
                                    ak_module_allocator_t *allocator,          \
                                    const char **error) {                      \
    if (!module_ctx || !allocator) {                                           \
      if (error)                                                               \
        *error = "Invalid init parameters";                                    \
      return AK_MODULE_ERROR_INIT;                                             \
    }                                                                          \
    g_allocator = allocator;                                                   \
    _state_type *state = (_state_type *)allocator->alloc(sizeof(_state_type)); \
    if (!state) {                                                              \
      if (error)                                                               \
        *error = "Failed to allocate module state";                            \
      return AK_MODULE_ERROR_INIT;                                             \
    }                                                                          \
    state->allocator = allocator;                                              \
    do {

/**
 * @brief End module initialization function
 *
 * Completes the init function and sets module_ctx on success.
 */
#define AK24_MODULE_INIT_END_V1()                                              \
  }                                                                            \
  while (0)                                                                    \
    ;                                                                          \
  *module_ctx = state;                                                         \
  return AK_MODULE_OK;                                                         \
  }

/**
 * @brief Return an error from init function
 *
 * @param _error_msg Error message string
 * @param _code Result code (AK_MODULE_ERROR_*)
 */
#define AK24_MODULE_INIT_ERROR_V1(_error_msg, _code)                           \
  do {                                                                         \
    if (error)                                                                 \
      *error = _error_msg;                                                     \
    allocator->free(state);                                                    \
    return _code;                                                              \
  } while (0)

//=============================================================================
// Module Deinitialization Helpers (V1)
//=============================================================================

/**
 * @brief Begin module deinitialization function
 *
 * Generates ak_module_deinit signature and provides:
 * - state: pointer to module state (casted)
 * - allocator: stored allocator reference
 *
 * @param _state_type The type of your module state struct
 */
#define AK24_MODULE_DEINIT_BEGIN_V1(_state_type)                               \
  void ak_module_deinit(void *module_ctx) {                                    \
    if (!module_ctx) {                                                         \
      return;                                                                  \
    }                                                                          \
    _state_type *state = (_state_type *)module_ctx;                            \
    ak_module_allocator_t *allocator = state->allocator;                       \
    do {

/**
 * @brief End module deinitialization function
 *
 * Automatically frees the state struct.
 */
#define AK24_MODULE_DEINIT_END_V1()                                            \
  }                                                                            \
  while (0)                                                                    \
    ;                                                                          \
  allocator->free(state);                                                      \
  }

//=============================================================================
// Allocator Wrapper Macros (V1)
//=============================================================================

/**
 * @brief Allocate memory for a single object
 *
 * @param _type Type to allocate
 * @return Pointer to allocated memory, or NULL on failure
 */
#define AK24_MODULE_ALLOC_V1(_type) ((_type *)g_allocator->alloc(sizeof(_type)))

/**
 * @brief Allocate memory for an array
 *
 * @param _type Type of array elements
 * @param _count Number of elements
 * @return Pointer to allocated memory, or NULL on failure
 */
#define AK24_MODULE_CALLOC_V1(_type, _count)                                   \
  ((_type *)g_allocator->alloc(sizeof(_type) * (_count)))

/**
 * @brief Free allocated memory
 *
 * @param _ptr Pointer to free
 */
#define AK24_MODULE_FREE_V1(_ptr)                                              \
  do {                                                                         \
    if (_ptr) {                                                                \
      g_allocator->free(_ptr);                                                 \
      (_ptr) = NULL;                                                           \
    }                                                                          \
  } while (0)

/**
 * @brief Duplicate a string using module allocator
 *
 * @param _str String to duplicate
 * @return Pointer to duplicated string, or NULL on failure
 */
#define AK24_MODULE_STRDUP_V1(_str)                                            \
  ({                                                                           \
    const char *__s = (_str);                                                  \
    char *__result = NULL;                                                     \
    if (__s && g_allocator) {                                                  \
      size_t __len = strlen(__s) + 1;                                          \
      __result = (char *)g_allocator->alloc(__len);                            \
      if (__result) {                                                          \
        strcpy(__result, __s);                                                 \
      }                                                                        \
    }                                                                          \
    __result;                                                                  \
  })

//=============================================================================
// Function Registration Helpers (V1)
//=============================================================================

/**
 * @brief Begin function registration table
 *
 * Use with AK24_MODULE_REGISTER_FUNCTION_V1 and
 * AK24_MODULE_FUNCTION_TABLE_END_V1
 */
#define AK24_MODULE_FUNCTION_TABLE_BEGIN_V1()                                  \
  void *ak_module_get_function(void *module_ctx, const char *name) {           \
    if (!module_ctx || !name) {                                                \
      return NULL;                                                             \
    }                                                                          \
    ak24_module_state_t *state = (ak24_module_state_t *)module_ctx;            \
    state->call_count++;

/**
 * @brief Register a function in the lookup table
 *
 * @param _name Function name string
 * @param _func Function pointer
 */
#define AK24_MODULE_REGISTER_FUNCTION_V1(_name, _func)                         \
  if (strcmp(name, _name) == 0) {                                              \
    return (void *)(_func);                                                    \
  }

/**
 * @brief End function registration table
 */
#define AK24_MODULE_FUNCTION_TABLE_END_V1()                                    \
  return NULL;                                                                 \
  }

//=============================================================================
// Function Signature Helpers (V1)
//=============================================================================

/**
 * @brief Begin function signature table
 *
 * Use with AK24_MODULE_SIGNATURE_*_V1 macros
 */
#define AK24_MODULE_SIGNATURE_TABLE_BEGIN_V1()                                 \
  ak_function_signature_t *ak_module_get_function_signature(                   \
      void *module_ctx, const char *name) {                                    \
    if (!module_ctx || !name || !g_allocator) {                                \
      return NULL;                                                             \
    }                                                                          \
    ak_function_signature_t *sig =                                             \
        AK24_MODULE_ALLOC_V1(ak_function_signature_t);                         \
    if (!sig) {                                                                \
      return NULL;                                                             \
    }

/**
 * @brief Define a simple void(void*) signature
 *
 * Common pattern: void func(void *args)
 *
 * @param _name Function name string
 * @param _func Function pointer
 */
#define AK24_MODULE_SIGNATURE_VOID_PTR_V1(_name, _func)                        \
  if (strcmp(name, _name) == 0) {                                              \
    sig->function_name = _name;                                                \
    sig->function_ptr = (void *)(_func);                                       \
    sig->return_type = (ak_param_metadata_t){.base_type = AK_PARAM_TYPE_VOID,  \
                                             .type_name = NULL,                \
                                             .ptr_depth = 0,                   \
                                             .is_const = false,                \
                                             .is_array = false};               \
    sig->param_count = 1;                                                      \
    sig->params = AK24_MODULE_CALLOC_V1(ak_param_metadata_t, 1);               \
    if (!sig->params) {                                                        \
      AK24_MODULE_FREE_V1(sig);                                                \
      return NULL;                                                             \
    }                                                                          \
    sig->params[0] = (ak_param_metadata_t){.base_type = AK_PARAM_TYPE_VOID,    \
                                           .type_name = NULL,                  \
                                           .ptr_depth = 1,                     \
                                           .is_const = false,                  \
                                           .is_array = false};                 \
    return sig;                                                                \
  }

/**
 * @brief End function signature table
 */
#define AK24_MODULE_SIGNATURE_TABLE_END_V1()                                   \
  AK24_MODULE_FREE_V1(sig);                                                    \
  return NULL;                                                                 \
  }

//=============================================================================
// Helper Macros for Signature Building (Advanced)
//=============================================================================

/**
 * @brief Create a parameter metadata struct
 *
 * @param _base_type Base type (AK_PARAM_TYPE_*)
 * @param _ptr_depth Pointer depth (0=value, 1=*, 2=**)
 * @param _is_const Is const qualified
 * @param _type_name Type name for custom types (or NULL)
 */
#define AK24_PARAM_V1(_base_type, _ptr_depth, _is_const, _type_name)           \
  (ak_param_metadata_t) {                                                      \
    .base_type = _base_type, .type_name = _type_name, .ptr_depth = _ptr_depth, \
    .is_const = _is_const, .is_array = false                                   \
  }

/**
 * @brief Common void* parameter
 */
#define AK24_PARAM_VOID_PTR_V1()                                               \
  AK24_PARAM_V1(AK_PARAM_TYPE_VOID, 1, false, NULL)

/**
 * @brief Common int parameter
 */
#define AK24_PARAM_INT_V1() AK24_PARAM_V1(AK_PARAM_TYPE_INT, 0, false, NULL)

/**
 * @brief Common const char* parameter
 */
#define AK24_PARAM_CONST_STR_V1()                                              \
  AK24_PARAM_V1(AK_PARAM_TYPE_CHAR, 1, true, NULL)

/**
 * @brief Void return type
 */
#define AK24_RETURN_VOID_V1() AK24_PARAM_V1(AK_PARAM_TYPE_VOID, 0, false, NULL)

#endif // AK24_MODULE_MACROS_H
