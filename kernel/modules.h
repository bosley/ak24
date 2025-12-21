/**
 * @file modules.h
 * @brief Internal module system types and helpers
 *
 * Private header for module system implementation. Defines internal structures
 * for tracking loaded modules, wrapping function pointers in lambdas, and
 * managing the module registry singleton.
 *
 * @note This is an internal header - public API is in interfaces.h
 */

#ifndef AK24_KERNEL_MODULES_INTERNAL_H
#define AK24_KERNEL_MODULES_INTERNAL_H

#include "interfaces.h"
#include "kernel.h"
#include "lambda.h"
#include "map.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Module lifecycle state
 *
 * Tracks the current state of a loaded module to prevent invalid operations.
 */
typedef enum {
  MODULE_STATE_LOADING = 0, /**< Module is being loaded */
  MODULE_STATE_LOADED,      /**< Module is loaded and operational */
  MODULE_STATE_UNLOADING,   /**< Module is being unloaded */
  MODULE_STATE_FAILED       /**< Module failed to load/init */
} module_state_e;

/**
 * @brief Internal module instance
 *
 * Represents a loaded dynamic library with module context and vtable.
 * Reference counted to prevent unloading while functions are executing.
 */
typedef struct module_instance_s {
  void *dl_handle;              /**< dlopen handle */
  char *path;                   /**< Module file path (owned) */
  void *module_ctx;             /**< Module's internal context */
  ak_module_vtable_t vtable;    /**< Module function pointers */
  ak_lambda_t *unload_callback; /**< User callback on unload */
  void *unload_callback_ctx;    /**< Context for unload callback */
  AK24_MUTEX *access_mutex;     /**< Per-module mutex (NULL if !thread_safe) */
  _Atomic size_t ref_count;     /**< Active function call counter */
  _Atomic int state;            /**< Current module state (module_state_e) */
  bool thread_safe;             /**< Whether functions require locking */
} module_instance_t;

/**
 * @brief Module registry singleton
 *
 * Thread-safe registry tracking all loaded modules by path.
 * Protected by mutex for concurrent load/unload operations.
 */
typedef struct {
  map_t(module_instance_t *) loaded_modules; /**< path -> instance map */
  AK24_MUTEX registry_mutex;                 /**< Protects map access */
  bool initialized;                          /**< Singleton init flag */
} module_manager_t;

#endif
