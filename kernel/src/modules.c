#include "modules.h"
#include "kernel.h"
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations
static module_manager_t *get_or_create_manager(void);
static void destroy_manager(module_manager_t *mgr);
static module_instance_t *
create_module_instance(ak_module_load_options_t *options, const char **error);
static void destroy_module_instance(module_instance_t *instance);
static ak_module_handle_t *load_module_impl(ak_module_load_options_t *options,
                                            const char **error);
static bool unload_module_impl(ak_module_handle_t *handle, const char **error);
static bool verify_function_in_module(void *dl_handle, void *fn_ptr,
                                      const char *expected_path);
static bool validate_module_ctx(void *ctx);

// Module allocator functions
static void *module_alloc(size_t size);
static void *module_realloc(void *ptr, size_t size);
static void module_free(void *ptr);

// Singleton module manager instance
static module_manager_t *g_module_manager = NULL;
static pthread_mutex_t g_manager_init_mutex = PTHREAD_MUTEX_INITIALIZER;

// Global allocator provided to modules
static ak_module_allocator_t g_module_allocator = {
    .alloc = module_alloc, .realloc = module_realloc, .free = module_free};

/**
 * @brief Module allocator - uses regular malloc/free, NOT GC
 *
 * IMPORTANT: Modules must use non-GC allocation because:
 * 1. Module code is unloaded via dlclose() before GC collection
 * 2. GC might try to scan/finalize memory after code is unmapped
 * 3. This causes segfaults or undefined behavior
 *
 * By using malloc/free, we ensure all module memory is explicitly
 * freed before dlclose(), avoiding GC/dlclose race conditions.
 *
 * NOTE: In future, we could implement a custom allocator that limits
 * memory usage per module, tracks allocations, etc.
 */
static void *module_alloc(size_t size) { return malloc(size); }

static void *module_realloc(void *ptr, size_t size) {
  return realloc(ptr, size);
}

static void module_free(void *ptr) { free(ptr); }

/**
 * @brief Verify function pointer is from the expected module
 *
 * Uses dladdr() to verify that a function pointer actually belongs to
 * the loaded module, preventing pointer hijacking or confusion.
 *
 * @param dl_handle Module's dlopen handle
 * @param fn_ptr Function pointer to verify
 * @param expected_path Expected module file path
 * @return true if function is from expected module
 */
static bool verify_function_in_module(void *dl_handle, void *fn_ptr,
                                      const char *expected_path) {
  (void)dl_handle; // Currently unused but keep for future, and (void) to avoid
                   // warnings

  if (!fn_ptr || !expected_path) {
    return false;
  }

  Dl_info info;
  if (dladdr(fn_ptr, &info) == 0 || !info.dli_fname) {
    return false; // dladdr failed
  }

  return strcmp(info.dli_fname, expected_path) == 0;
}

/**
 * @brief Validate module context pointer
 *
 * Performs basic sanity checks on the module context pointer returned
 * from module init to detect obvious corruption or invalid values.
 *
 * @param ctx Module context pointer
 * @return true if context appears valid
 */
static bool validate_module_ctx(void *ctx) {
  if (!ctx) {
    return false;
  }

  /*
    * Basic sanity checks:
    * - Pointer is not NULL
    * - Pointer is aligned to pointer size

    I don't really know if this is sufficient, but it's better than nothing.
  */
  if (((uintptr_t)ctx & (sizeof(void *) - 1)) != 0) {
    return false;
  }

  // region ???
  // bounds and canarys etc

  return true;
}

/**
 * @brief Stub for per-module resource tracking
 *
 * TODO: Implement resource tracking including:
 * - Memory allocation accounting per module
 * - File descriptor tracking
 * - Thread/CPU time limits
 * - Network connection monitoring
 *
 * This would integrate with the module allocator to track all resources
 * acquired by a module, enabling:
 * - Resource limits and quotas
 * - Leak detection on module unload
 * - Resource usage reporting
 *
 * @param instance Module instance to track resources for
 */
static void track_module_resources(module_instance_t *instance) {
  (void)instance;

  // I dont want to do this until the module system is fully tested
  // and has proven itself under at least a litte load. Probalbly implement
  // this when we first start having things fall apart under load and
  // need to limit the resource so im keeping this in
}

/**
 * @brief Get or create the singleton module manager
 */
static module_manager_t *get_or_create_manager(void) {
  pthread_mutex_lock(&g_manager_init_mutex);

  if (!g_module_manager) {
    g_module_manager = (module_manager_t *)AK24_ALLOC(sizeof(module_manager_t));
    if (!g_module_manager) {
      pthread_mutex_unlock(&g_manager_init_mutex);
      return NULL;
    }

    map_init_generic(&g_module_manager->loaded_modules, sizeof(char *),
                     map_hash_str, map_cmp_str);
    pthread_mutex_init(&g_module_manager->registry_mutex, NULL);
    g_module_manager->initialized = true;
  }

  pthread_mutex_unlock(&g_manager_init_mutex);
  return g_module_manager;
}

/**
 * @brief Destroy the module manager singleton
 */
__attribute__((unused)) static void destroy_manager(module_manager_t *mgr) {
  if (!mgr) {
    return;
  }

  pthread_mutex_lock(&mgr->registry_mutex);

  // Unload all modules
  map_iter_t iter = map_iter(&mgr->loaded_modules);
  const char **key;
  while ((key = (const char **)map_next_generic(&mgr->loaded_modules, &iter))) {
    module_instance_t **inst_ptr =
        (module_instance_t **)map_get_generic(&mgr->loaded_modules, key);
    if (inst_ptr && *inst_ptr) {
      destroy_module_instance(*inst_ptr);
    }
  }

  map_deinit(&mgr->loaded_modules);
  pthread_mutex_unlock(&mgr->registry_mutex);
  pthread_mutex_destroy(&mgr->registry_mutex);

  AK24_FREE(mgr);
}

/**
 * @brief Create a module instance from loaded dynamic library
 */
static module_instance_t *
create_module_instance(ak_module_load_options_t *options, const char **error) {
  if (!options || !options->module_path) {
    if (error)
      *error = "Invalid module load options";
    return NULL;
  }

  // Open the dynamic library
  void *dl_handle = dlopen(options->module_path, RTLD_NOW | RTLD_LOCAL);
  if (!dl_handle) {
    if (error) {
      const char *dlerr = dlerror();
      *error = dlerr ? dlerr : "Failed to load module";
    }
    return NULL;
  }

/*
  Here is where "the rubber meets the road" and we have to actually grab
  things out of the external library. So what we do is look for the api
  functions that we state we expect

  We have to ignore pedants here because undefiend C behavior promised
  by POSIX is how we get the dlsym to work with function pointers.

  When we adapt this to WIN we will need to block here, and use their loading
  mechanisms instead. - bosley
*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
  int (*version_fn)(void) =
      (int (*)(void))dlsym(dl_handle, "ak_module_version");
  ak_module_result_e (*init_fn)(void **, ak_module_allocator_t *,
                                const char **) =
      (ak_module_result_e (*)(void **, ak_module_allocator_t *,
                              const char **))dlsym(dl_handle, "ak_module_init");
  void (*deinit_fn)(void *) =
      (void (*)(void *))dlsym(dl_handle, "ak_module_deinit");
  const char *(*info_fn)(const char *) =
      (const char *(*)(const char *))dlsym(dl_handle, "ak_module_info");
  void *(*get_fn)(void *, const char *) =
      (void *(*)(void *, const char *))dlsym(dl_handle,
                                             "ak_module_get_function");
  ak_function_signature_t *(*get_sig_fn)(void *, const char *) =
      (ak_function_signature_t * (*)(void *, const char *))
          dlsym(dl_handle, "ak_module_get_function_signature");
#pragma GCC diagnostic pop

  if (!version_fn || !init_fn || !deinit_fn || !info_fn) {
    if (error)
      *error = "Module missing required functions";
    dlclose(dl_handle);
    return NULL;
  }

  /*
    straight comparison for now, but we could be a little more granular
    in the future if we want to allow for some level of backward compatibility
    or something like that
  */
  if (version_fn() != AK24_MODULE_API_VERSION) {
    if (error)
      *error = "Module API version mismatch";
    dlclose(dl_handle);
    return NULL;
  }

  // Allocate module instance that will hold state
  module_instance_t *instance =
      (module_instance_t *)AK24_ALLOC(sizeof(module_instance_t));
  if (!instance) {
    dlclose(dl_handle);
    if (error)
      *error = "Failed to allocate module instance";
    return NULL;
  }

  atomic_store_explicit((_Atomic int *)&instance->state, MODULE_STATE_LOADING,
                        memory_order_relaxed);

  instance->dl_handle = dl_handle;
  instance->path = (char *)AK24_ALLOC(strlen(options->module_path) + 1);
  if (!instance->path) {
    dlclose(dl_handle);
    AK24_FREE(instance);
    if (error)
      *error = "Failed to allocate path string";
    return NULL;
  }
  strcpy(instance->path, options->module_path);

  instance->unload_callback = options->unload_callback;
  instance->unload_callback_ctx = options->unload_callback_ctx;
  instance->thread_safe = options->thread_safe;
  atomic_store_explicit(&instance->ref_count, 0, memory_order_relaxed);

  /*
    setup a virtual table that points to the functions we loaded from the module
    so that we can call them later via the instance

    this is basically what classes are under the hood in C++ btw
  */
  instance->vtable.ak_module_version = version_fn;
  instance->vtable.ak_module_init = init_fn;
  instance->vtable.ak_module_deinit = deinit_fn;
  instance->vtable.ak_module_info = info_fn;
  instance->vtable.ak_module_get_function = get_fn;
  instance->vtable.ak_module_get_function_signature = get_sig_fn;

  /*
    Verify that the function pointers actually belong to the loaded module
    to prevent pointer hijacking or confusion. This is a itty bitty security
    measure.
  */
  if (!verify_function_in_module(dl_handle, (void *)version_fn,
                                 options->module_path)) {
    atomic_store_explicit((_Atomic int *)&instance->state, MODULE_STATE_FAILED,
                          memory_order_release);
    if (error)
      *error = "Function pointer validation failed";
    if (instance->access_mutex) {
      pthread_mutex_destroy(instance->access_mutex);
      AK24_FREE(instance->access_mutex);
    }
    AK24_FREE(instance->path);
    dlclose(dl_handle);
    AK24_FREE(instance);
    return NULL;
  }

  /*
    Initialize per-module mutex if thread-safe access requested by the caller.
    This is where when the app caller says "i want to use this module but make
    it safe for threads" we set that up here and auto-magically handle locking
    in the lambdas we create for the user later on down the line
  */
  if (options->thread_safe) {
    instance->access_mutex =
        (pthread_mutex_t *)AK24_ALLOC(sizeof(pthread_mutex_t));
    if (!instance->access_mutex) {
      atomic_store_explicit((_Atomic int *)&instance->state,
                            MODULE_STATE_FAILED, memory_order_release);
      AK24_FREE(instance->path);
      dlclose(dl_handle);
      AK24_FREE(instance);
      if (error)
        *error = "Failed to allocate mutex";
      return NULL;
    }
    pthread_mutex_init(instance->access_mutex, NULL);
  } else {
    instance->access_mutex = NULL;
  }

  // Initialize module with allocator
  void *module_ctx = NULL;
  const char *init_error = NULL;
  ak_module_result_e result =
      init_fn(&module_ctx, &g_module_allocator, &init_error);
  if (result != AK_MODULE_OK) {
    atomic_store_explicit((_Atomic int *)&instance->state, MODULE_STATE_FAILED,
                          memory_order_release);
    if (error)
      *error = init_error ? init_error : "Module init failed";
    if (instance->access_mutex) {
      pthread_mutex_destroy(instance->access_mutex);
      AK24_FREE(instance->access_mutex);
    }
    AK24_FREE(instance->path);
    dlclose(dl_handle);
    AK24_FREE(instance);
    return NULL;
  }

  instance->module_ctx = module_ctx;

  // Validate module context
  if (!validate_module_ctx(module_ctx)) {
    atomic_store_explicit((_Atomic int *)&instance->state, MODULE_STATE_FAILED,
                          memory_order_release);
    if (error)
      *error = "Module context validation failed";
    // Module init succeeded so we should deinit
    if (deinit_fn) {
      deinit_fn(module_ctx);
    }
    if (instance->access_mutex) {
      pthread_mutex_destroy(instance->access_mutex);
      AK24_FREE(instance->access_mutex);
    }
    AK24_FREE(instance->path);
    dlclose(dl_handle);
    AK24_FREE(instance);
    return NULL;
  }

  // start tracking state (later) and set to laoded

  track_module_resources(instance);

  atomic_store_explicit((_Atomic int *)&instance->state, MODULE_STATE_LOADED,
                        memory_order_release);

  return instance;
}

/**
 * @brief Destroy a module instance
 */
static void destroy_module_instance(module_instance_t *instance) {
  if (!instance) {
    return;
  }

  // Transition to UNLOADING state
  atomic_store_explicit((_Atomic int *)&instance->state, MODULE_STATE_UNLOADING,
                        memory_order_release);

  // Wait for all active function calls to complete
  while (atomic_load_explicit(&instance->ref_count, memory_order_acquire) > 0) {
    // Busy wait - could use condition variable for efficiency
  }

  // Invoke unload callback if present
  if (instance->unload_callback) {
    ak_lambda_invoke(instance->unload_callback, instance->unload_callback_ctx);
  }

  // Deinit module
  if (instance->vtable.ak_module_deinit && instance->module_ctx) {
    instance->vtable.ak_module_deinit(instance->module_ctx);
  }

  // Close dynamic library
  if (instance->dl_handle) {
    dlclose(instance->dl_handle);
  }

  // Destroy mutex if present
  if (instance->access_mutex) {
    pthread_mutex_destroy(instance->access_mutex);
    AK24_FREE(instance->access_mutex);
  }

  // Free path
  if (instance->path) {
    AK24_FREE(instance->path);
  }

  AK24_FREE(instance);
}

/**
 * @brief Load a module (internal implementation)
 */
static ak_module_handle_t *load_module_impl(ak_module_load_options_t *options,
                                            const char **error) {
  module_manager_t *mgr = get_or_create_manager();
  if (!mgr) {
    if (error)
      *error = "Failed to initialize module manager";
    return NULL;
  }

  pthread_mutex_lock(&mgr->registry_mutex);

  // Check if module already loaded
  module_instance_t **existing = (module_instance_t **)map_get_generic(
      &mgr->loaded_modules, &options->module_path);
  if (existing && *existing) {
    pthread_mutex_unlock(&mgr->registry_mutex);
    if (error)
      *error = "Module already loaded";
    return NULL;
  }

  // Create module instance
  module_instance_t *instance = create_module_instance(options, error);
  if (!instance) {
    pthread_mutex_unlock(&mgr->registry_mutex);
    return NULL;
  }

  // Add to registry
  map_set_generic(&mgr->loaded_modules, &options->module_path, instance);

  pthread_mutex_unlock(&mgr->registry_mutex);

  // Create user-facing handle
  ak_module_handle_t *handle =
      (ak_module_handle_t *)AK24_ALLOC(sizeof(ak_module_handle_t));
  if (!handle) {
    pthread_mutex_lock(&mgr->registry_mutex);
    map_remove_generic(&mgr->loaded_modules, &options->module_path);
    pthread_mutex_unlock(&mgr->registry_mutex);
    destroy_module_instance(instance);
    if (error)
      *error = "Failed to allocate module handle";
    return NULL;
  }

  handle->module_handle = instance->dl_handle;
  handle->module_ctx = instance->module_ctx;
  handle->vtable = instance->vtable;
  handle->unload_cb = instance->unload_callback;
  handle->unload_cb_ctx = instance->unload_callback_ctx;
  handle->thread_safe = instance->thread_safe;
  handle->lock = instance->access_mutex;
  handle->internal_instance = instance;

  return handle;
}

/**
 * @brief Unload a module (internal implementation)
 */
static bool unload_module_impl(ak_module_handle_t *handle, const char **error) {
  if (!handle) {
    if (error)
      *error = "Invalid module handle";
    return false;
  }

  module_manager_t *mgr = get_or_create_manager();
  if (!mgr) {
    if (error)
      *error = "Module manager not initialized";
    return false;
  }

  pthread_mutex_lock(&mgr->registry_mutex);

  // Find the module instance by dl_handle
  module_instance_t *found_instance = NULL;
  const char *found_path = NULL;

  map_iter_t iter = map_iter(&mgr->loaded_modules);
  const char **key;
  while ((key = (const char **)map_next_generic(&mgr->loaded_modules, &iter))) {
    module_instance_t **inst_ptr =
        (module_instance_t **)map_get_generic(&mgr->loaded_modules, key);
    if (inst_ptr && *inst_ptr &&
        (*inst_ptr)->dl_handle == handle->module_handle) {
      found_instance = *inst_ptr;
      found_path = *key;
      break;
    }
  }

  if (!found_instance || !found_path) {
    pthread_mutex_unlock(&mgr->registry_mutex);
    if (error)
      *error = "Module not found in registry";
    return false;
  }

  // Check module state - must be LOADED to unload
  int current_state = atomic_load_explicit(
      (_Atomic int *)&found_instance->state, memory_order_acquire);
  if (current_state != MODULE_STATE_LOADED) {
    pthread_mutex_unlock(&mgr->registry_mutex);
    if (error) {
      if (current_state == MODULE_STATE_UNLOADING) {
        *error = "Module is already being unloaded";
      } else if (current_state == MODULE_STATE_LOADING) {
        *error = "Module is still loading";
      } else {
        *error = "Module is in invalid state";
      }
    }
    return false;
  }

  // Check if module is still in use
  if (atomic_load_explicit(&found_instance->ref_count, memory_order_acquire) >
      0) {
    pthread_mutex_unlock(&mgr->registry_mutex);
    if (error)
      *error = "Module still in use";
    return false;
  }

  // Remove from registry
  map_remove_generic(&mgr->loaded_modules, &found_path);

  pthread_mutex_unlock(&mgr->registry_mutex);

  // Destroy instance
  destroy_module_instance(found_instance);

  // Free handle
  AK24_FREE(handle);

  return true;
}

/**
 * @brief Get module system context (public API)
 */
ak_module_ctx_t *ak_module_get_system_ctx(void) {
  module_manager_t *mgr = get_or_create_manager();
  if (!mgr) {
    return NULL;
  }

  ak_module_ctx_t *ctx = (ak_module_ctx_t *)AK24_ALLOC(sizeof(ak_module_ctx_t));
  if (!ctx) {
    return NULL;
  }

  ctx->load_module = load_module_impl;
  ctx->unload_module = unload_module_impl;
  ctx->internal_ctx = mgr;

  return ctx;
}

/**
 * @brief Free module system context (public API)
 */
void ak_module_free_system_ctx(ak_module_ctx_t *ctx) {
  if (!ctx) {
    return;
  }
  // Note: We don't destroy the manager here as it's a singleton
  // It will be cleaned up at program exit or explicit shutdown
  AK24_FREE(ctx);
}

/**
 * @brief Get module information string (public API)
 *
 * Thread-safe with state checking. Does not require ref counting since
 * it's a simple metadata query that doesn't involve executing module code.
 */
const char *ak_handle_get_info(ak_module_handle_t *handle, const char *key) {
  if (!handle || !key) {
    return NULL;
  }

  module_instance_t *instance = (module_instance_t *)handle->internal_instance;
  if (!instance) {
    return NULL;
  }

  // Check module state - must be LOADED
  int state = atomic_load_explicit((_Atomic int *)&instance->state,
                                   memory_order_acquire);
  if (state != MODULE_STATE_LOADED) {
    return NULL;
  }

  return handle->vtable.ak_module_info(key);
}

/**
 * @brief Get a function pointer from a module (public API)
 *
 * Thread-safe with state checking and ref counting. Increments ref count
 * to prevent unloading while the function pointer is being retrieved.
 */
void *ak_handle_get_function(ak_module_handle_t *handle,
                             const char *function_name) {
  if (!handle || !function_name) {
    return NULL;
  }

  module_instance_t *instance = (module_instance_t *)handle->internal_instance;
  if (!instance) {
    return NULL;
  }

  // Check module state - must be LOADED
  int state = atomic_load_explicit((_Atomic int *)&instance->state,
                                   memory_order_acquire);
  if (state != MODULE_STATE_LOADED) {
    return NULL;
  }

  if (!handle->vtable.ak_module_get_function) {
    return NULL;
  }

  // Increment ref count to prevent unload during this operation
  atomic_fetch_add_explicit(&instance->ref_count, 1, memory_order_acquire);

  void *result =
      handle->vtable.ak_module_get_function(handle->module_ctx, function_name);

  // Decrement ref count
  atomic_fetch_sub_explicit(&instance->ref_count, 1, memory_order_release);

  return result;
}

/**
 * @brief Get function signature metadata from a module (public API)
 *
 * Thread-safe with state checking and ref counting. Increments ref count
 * to prevent unloading while the signature is being retrieved.
 */
ak_function_signature_t *
ak_handle_get_function_signature(ak_module_handle_t *handle,
                                 const char *function_name) {
  if (!handle || !function_name) {
    return NULL;
  }

  module_instance_t *instance = (module_instance_t *)handle->internal_instance;
  if (!instance) {
    return NULL;
  }

  // Check module state - must be LOADED
  int state = atomic_load_explicit((_Atomic int *)&instance->state,
                                   memory_order_acquire);
  if (state != MODULE_STATE_LOADED) {
    return NULL;
  }

  if (!handle->vtable.ak_module_get_function_signature) {
    return NULL;
  }

  // Increment ref count to prevent unload during this operation
  atomic_fetch_add_explicit(&instance->ref_count, 1, memory_order_acquire);

  ak_function_signature_t *result =
      handle->vtable.ak_module_get_function_signature(handle->module_ctx,
                                                      function_name);

  // Decrement ref count
  atomic_fetch_sub_explicit(&instance->ref_count, 1, memory_order_release);

  return result;
}
