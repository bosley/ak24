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
 */
typedef struct {
  int (*ak_module_version)(void);
  ak_module_result_e (*ak_module_init)(void **module_ctx,
                                       ak_module_allocator_t *allocator,
                                       const char **error);
  void (*ak_module_deinit)(void *module_ctx);
  const char *(*ak_module_info)(const char *key);
  void *(*ak_module_get_function)(void *module_ctx, const char *name);
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

#endif
