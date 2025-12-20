#include "modules.h"
#include "kernel.h"
#include <dlfcn.h>
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
 * @brief Module allocator - uses kernel memory
 */
static void *module_alloc(size_t size) { return AK24_ALLOC(size); }

static void *module_realloc(void *ptr, size_t size) {
  return AK24_REALLOC(ptr, size);
}

static void module_free(void *ptr) { AK24_FREE(ptr); }

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
static void destroy_manager(module_manager_t *mgr) {
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

  // Load required module functions
  int (*version_fn)(void) = dlsym(dl_handle, "ak_module_version");
  ak_module_result_e (*init_fn)(void **, ak_module_allocator_t *,
                                const char **) =
      dlsym(dl_handle, "ak_module_init");
  void (*deinit_fn)(void *) = dlsym(dl_handle, "ak_module_deinit");
  const char *(*info_fn)(const char *) = dlsym(dl_handle, "ak_module_info");
  void *(*get_fn)(void *, const char *) =
      dlsym(dl_handle, "ak_module_get_function");

  if (!version_fn || !init_fn || !deinit_fn || !info_fn) {
    if (error)
      *error = "Module missing required functions";
    dlclose(dl_handle);
    return NULL;
  }

  // Check API version
  if (version_fn() != AK24_MODULE_API_VERSION) {
    if (error)
      *error = "Module API version mismatch";
    dlclose(dl_handle);
    return NULL;
  }

  // Allocate instance
  module_instance_t *instance =
      (module_instance_t *)AK24_ALLOC(sizeof(module_instance_t));
  if (!instance) {
    dlclose(dl_handle);
    if (error)
      *error = "Failed to allocate module instance";
    return NULL;
  }

  // Initialize instance
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

  // Store vtable
  instance->vtable.ak_module_version = version_fn;
  instance->vtable.ak_module_init = init_fn;
  instance->vtable.ak_module_deinit = deinit_fn;
  instance->vtable.ak_module_info = info_fn;
  instance->vtable.ak_module_get_function = get_fn;

  // Create per-module mutex if thread-safe
  if (options->thread_safe) {
    instance->access_mutex =
        (pthread_mutex_t *)AK24_ALLOC(sizeof(pthread_mutex_t));
    if (!instance->access_mutex) {
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

  return instance;
}

/**
 * @brief Destroy a module instance
 */
static void destroy_module_instance(module_instance_t *instance) {
  if (!instance) {
    return;
  }

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
