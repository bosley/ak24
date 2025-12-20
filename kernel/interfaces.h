#ifndef AK24_KERNEL_INTERFACES_H
#define AK24_KERNEL_INTERFACES_H

#include <stdbool.h>
#include <stddef.h>

/*

     The kernel itslef is a library and small application framework.
     The kernel offers functionalities and whatnot, but in order to extend the
   application we will make "modules" that are external to the kernel itself.
   These modules will have to provide a set of interfaces on registration in
   order to be able to interact with the kernel

     The application layer can decide what to add or not to based on their
   needs/demands.

     The first thing we need to define is the interface that the kernel will use
   to present itself to the application for the sake of registering modules. We
   will create the module system such-that the modules can be arbitrarily
     loaded/unloaded at runtime through atomic thread-safe operations.

     1. User does calls ak_kernel_get_module_ctx(); to get a handle to the
   interface That permits adding/removing modules. The kernel itself will have a
   singleton that is guarded through this interface.

     2. The user provides the full path of the module to load, along with a
   ak_lambda_t + ctx for callback when that instance of the module is UNLOADED

        The user will provide to us, through an enum of C types on a struct with
   meta information (pointer depth, etc) the expected functions that the module
   should provide along with their expected signatures (return type + param
   types).

        The user will provide if they want the module interactions to be
   "thread-safe" or not.

     3. If the function is able to be found, loaded, and all expected functions
   are present as they are expected to be present, the kernel will make a struct
   with lambdas on as a return result so the user can then call those functions
   via the lambdas as they see fit

        if they wanted thread-safe operations into the module, then the lambdas
   constructed will have internal locking mechanisms to ensure thread-safety.

        no matter the case, both variants will ensure that intance of the module
   is still loaded and callable (small overhead that is worht it for safety)

        In the event that there is a failure, the funtion will return NULL and
   set the error ref param (in) stating there was an error

*/

typedef struct ak_lambda_t ak_lambda_t;

/**
 * @brief C type enumeration for function parameter metadata
 *
 * Describes the base C type of a parameter for dynamic dispatch.
 */
typedef enum {
  AK_PARAM_TYPE_VOID = 0,
  AK_PARAM_TYPE_BOOL,
  AK_PARAM_TYPE_CHAR,
  AK_PARAM_TYPE_SHORT,
  AK_PARAM_TYPE_INT,
  AK_PARAM_TYPE_LONG,
  AK_PARAM_TYPE_LONG_LONG,
  AK_PARAM_TYPE_UCHAR,
  AK_PARAM_TYPE_USHORT,
  AK_PARAM_TYPE_UINT,
  AK_PARAM_TYPE_ULONG,
  AK_PARAM_TYPE_ULONG_LONG,
  AK_PARAM_TYPE_FLOAT,
  AK_PARAM_TYPE_DOUBLE,
  AK_PARAM_TYPE_SIZE_T,
  AK_PARAM_TYPE_SSIZE_T,
  AK_PARAM_TYPE_INTPTR_T,
  AK_PARAM_TYPE_UINTPTR_T,
  AK_PARAM_TYPE_CUSTOM /**< Custom/opaque type (use type_name field) */
} ak_param_type_e;

/**
 * @brief Parameter metadata for dynamic function dispatch
 *
 * Encodes complete type information for a function parameter, including
 * the base type, pointer depth, const qualifiers, and optional type name.
 */
typedef struct {
  ak_param_type_e base_type; /**< Base C type */
  const char *type_name;     /**< Type name for custom types (e.g., "FILE") */
  size_t ptr_depth;          /**< 0=value, 1=*, 2=**, etc. */
  bool is_const;             /**< True if const-qualified */
  bool is_array;             /**< True if array parameter */
} ak_param_metadata_t;

/**
 * @brief Function signature metadata for dynamic dispatch
 *
 * Describes the complete signature of a module function including
 * return type and all parameters.
 */
typedef struct {
  ak_param_metadata_t return_type; /**< Return type metadata */
  ak_param_metadata_t *params;     /**< Array of parameter metadata */
  size_t param_count;              /**< Number of parameters */
  const char *function_name;       /**< Function name */
  void *function_ptr;              /**< Actual function pointer */
} ak_function_signature_t;

/**
 * @brief Module memory allocator interface
 *
 * Modules receive this allocator during initialization and MUST use it
 * for all persistent allocations. This prevents GC/dlclose conflicts.
 */
typedef struct {
  void *(*alloc)(size_t size);              /**< Allocate memory */
  void *(*realloc)(void *ptr, size_t size); /**< Reallocate memory */
  void (*free)(void *ptr);                  /**< Free memory */
} ak_module_allocator_t;

/**
 * @brief Module initialization result codes
 */
typedef enum {
  AK_MODULE_OK = 0,
  AK_MODULE_ERROR_VERSION,
  AK_MODULE_ERROR_INIT,
  AK_MODULE_ERROR_INVALID
} ak_module_result_e;

/**
 * @brief Standard AK24 module interface
 *
 * All modules must export these functions with C linkage:
 * - ak_module_version
 * - ak_module_init
 * - ak_module_deinit
 * - ak_module_info
 * - ak_module_get_function (optional)
 * - ak_module_get_function_signature (optional)
 */
typedef struct {
  int (*ak_module_version)(void);
  ak_module_result_e (*ak_module_init)(void **module_ctx,
                                       ak_module_allocator_t *allocator,
                                       const char **error);
  void (*ak_module_deinit)(void *module_ctx);
  const char *(*ak_module_info)(const char *key);
  void *(*ak_module_get_function)(void *module_ctx, const char *name);
  ak_function_signature_t *(*ak_module_get_function_signature)(
      void *module_ctx, const char *name);
} ak_module_vtable_t;

/**
 * @brief Module loading options
 */
typedef struct {
  const char *module_path;
  ak_lambda_t *unload_callback;
  void *unload_callback_ctx;
  bool thread_safe;
} ak_module_load_options_t;

/**
 * @brief Module handle returned to users
 */
typedef struct ak_module_handle_t ak_module_handle_t;

struct ak_module_handle_t {
  void *module_handle;       /**< dlopen handle */
  void *module_ctx;          /**< Module's internal context */
  ak_module_vtable_t vtable; /**< Function pointers */
  ak_lambda_t *unload_cb;    /**< User callback on unload */
  void *unload_cb_ctx;       /**< User callback context */
  bool thread_safe;          /**< Thread-safe access requested */
  void *lock;                /**< Lock if thread_safe is true */
  void *internal_instance;   /**< Internal module instance (opaque) */
};

/**
 * @brief Module system interface
 */
typedef struct {
  ak_module_handle_t *(*load_module)(ak_module_load_options_t *options,
                                     const char **error);
  bool (*unload_module)(ak_module_handle_t *handle, const char **error);
  void *internal_ctx;
} ak_module_ctx_t;

/** Current AK24 module API version */
#define AK24_MODULE_API_VERSION 1

/**
 * @brief Get module system context
 *
 * @return Module system context, or NULL on failure
 */
ak_module_ctx_t *ak_module_get_system_ctx(void);

/**
 * @brief Free module system context
 *
 * @param ctx Module system context to free
 */
void ak_module_free_system_ctx(ak_module_ctx_t *ctx);

/**
 * @brief Get module information string
 *
 * Convenience wrapper for module->vtable.ak_module_info(key)
 *
 * @param handle Module handle
 * @param key Info key (e.g., "name", "version", "description")
 * @return Info string, or NULL if not found
 */
const char *ak_handle_get_info(ak_module_handle_t *handle, const char *key);

/**
 * @brief Get a function pointer from a module
 *
 * Convenience wrapper for module->vtable.ak_module_get_function()
 *
 * @param handle Module handle
 * @param function_name Name of function to retrieve
 * @return Function pointer, or NULL if not found
 */
void *ak_handle_get_function(ak_module_handle_t *handle,
                             const char *function_name);

/**
 * @brief Get function signature metadata from a module
 *
 * Convenience wrapper for module->vtable.ak_module_get_function_signature()
 *
 * @param handle Module handle
 * @param function_name Name of function to query
 * @return Function signature metadata, or NULL if not available
 */
ak_function_signature_t *
ak_handle_get_function_signature(ak_module_handle_t *handle,
                                 const char *function_name);

#endif
