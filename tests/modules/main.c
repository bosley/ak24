/**
 * @file main.c
 * @brief AK24 Module System Integration Test
 *
 * This integration test demonstrates:
 * - Loading the installed AK24 library
 * - Loading a dynamic module with allocator interface
 * - Calling module functions via get_function
 * - Using unload callbacks
 * - Module lifecycle management (load/use/unload)
 * - Function signature metadata for dynamic dispatch
 * - Clean shutdown with statistics
 */

#include <application.h>
#include <interfaces.h>
#include <unistd.h>

// Unload callback function
static void module_unload_callback(void *captured, void *args) {
  (void)args; // Unused
  const char *module_path = (const char *)captured;
  AK24_LOG_INFO("Module unload callback triggered for: %s", module_path);
  AK24_LOG_DEBUG("Module cleanup complete");
}

APP_ON_SHUTDOWN(on_shutdown) {
  time_t uptime = time(NULL) - ctx->shutdown_info->start_time;
  AK24_LOG_INFO("=== Shutting Down ===");
  AK24_LOG_INFO("Uptime: %ld seconds", uptime);

#if AK24_BUILD_DEBUG_MEMORY
  AK24_LOG_INFO("=== Memory Statistics ===");
  AK24_LOG_INFO("Total allocations: %zu",
                ctx->shutdown_info->memory_stats.total_allocations);
  AK24_LOG_INFO("Total frees: %zu",
                ctx->shutdown_info->memory_stats.total_frees);
  AK24_LOG_INFO("Total reallocs: %zu",
                ctx->shutdown_info->memory_stats.total_reallocs);
  AK24_LOG_INFO("Bytes allocated: %zu",
                ctx->shutdown_info->memory_stats.bytes_allocated);
  AK24_LOG_INFO("Current bytes: %zu",
                ctx->shutdown_info->memory_stats.current_bytes);
  AK24_LOG_INFO("Peak bytes: %zu", ctx->shutdown_info->memory_stats.peak_bytes);
  AK24_LOG_INFO("=========================");
#endif
}

APP_MAIN(app_main) {
  (void)ctx; // Suppress unused parameter warning

  AK24_LOG_INFO("=== AK24 Module System Integration Test ===");

  // Get the current working directory to locate the module
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    AK24_LOG_ERROR("Failed to get current directory");
    return 1;
  }

  // Construct module path
  char module_path[1536];
  snprintf(module_path, sizeof(module_path), "%s/build/libtest_module.dylib",
           cwd);

  AK24_LOG_INFO("Module path: %s", module_path);

  // Get module system context
  AK24_LOG_INFO("Step 1: Getting module system context...");
  ak_module_ctx_t *mod_ctx = ak_module_get_system_ctx();
  if (!mod_ctx) {
    AK24_LOG_ERROR("Failed to get module system context");
    return 1;
  }
  AK24_LOG_INFO("✓ Module system initialized");

  // Create unload callback lambda (NULL free function since module_path doesn't
  // need freeing)
  ak_lambda_t *unload_lambda =
      ak_lambda_new(module_unload_callback, (void *)module_path, NULL);
  if (!unload_lambda) {
    AK24_LOG_ERROR("Failed to create unload callback");
    ak_module_free_system_ctx(mod_ctx);
    return 1;
  }

  // Load module options
  ak_module_load_options_t load_opts = {.module_path = module_path,
                                        .unload_callback = unload_lambda,
                                        .unload_callback_ctx = NULL,
                                        .thread_safe = false};

  // Load the module
  AK24_LOG_INFO("Step 2: Loading module...");
  AK24_LOG_DEBUG("Calling load_module with path: %s", module_path);
  const char *error = NULL;
  ak_module_handle_t *module = mod_ctx->load_module(&load_opts, &error);
  if (!module) {
    AK24_LOG_ERROR("Failed to load module: %s",
                   error ? error : "unknown error");
    ak_lambda_free(unload_lambda);
    ak_module_free_system_ctx(mod_ctx);
    return 1;
  }
  AK24_LOG_INFO("✓ Module loaded successfully");

  // Get module info
  AK24_LOG_INFO("Step 3: Querying module info...");
  const char *name = module->vtable.ak_module_info("name");
  const char *version = module->vtable.ak_module_info("version");
  const char *description = module->vtable.ak_module_info("description");
  AK24_LOG_INFO("  Name: %s", name ? name : "N/A");
  AK24_LOG_INFO("  Version: %s", version ? version : "N/A");
  AK24_LOG_INFO("  Description: %s", description ? description : "N/A");

  // Get and call module functions
  AK24_LOG_INFO("Step 4: Calling module functions...");

  // Get process function
  typedef void (*module_fn_t)(void *);
  module_fn_t process_fn = (module_fn_t)module->vtable.ak_module_get_function(
      module->module_ctx, "process");
  if (process_fn) {
    AK24_LOG_INFO("Calling process()...");
    process_fn(NULL);
  }

  // Get and call allocate function
  module_fn_t allocate_fn = (module_fn_t)module->vtable.ak_module_get_function(
      module->module_ctx, "allocate");
  if (allocate_fn) {
    AK24_LOG_INFO("Calling allocate() to test allocator...");
    allocate_fn(NULL);
  }

  // Get and call sleep function
  module_fn_t sleep_fn = (module_fn_t)module->vtable.ak_module_get_function(
      module->module_ctx, "sleep");
  if (sleep_fn) {
    AK24_LOG_INFO("Calling sleep(500ms)...");
    int sleep_ms = 500;
    sleep_fn(&sleep_ms);
  }

  // Test function signature retrieval
  AK24_LOG_INFO("Step 5: Testing function signature metadata...");
  if (module->vtable.ak_module_get_function_signature) {
    ak_function_signature_t *sig =
        module->vtable.ak_module_get_function_signature(module->module_ctx,
                                                        "process");
    if (sig) {
      AK24_LOG_INFO("  Function: %s", sig->function_name);
      AK24_LOG_INFO("  Parameters: %zu", sig->param_count);
      AK24_LOG_INFO("  Return type: %d (ptr_depth: %zu)",
                    sig->return_type.base_type, sig->return_type.ptr_depth);
      for (size_t i = 0; i < sig->param_count; i++) {
        AK24_LOG_INFO("    Param %zu: type=%d ptr_depth=%zu", i,
                      sig->params[i].base_type, sig->params[i].ptr_depth);
      }
    } else {
      AK24_LOG_WARN("Function signature not available for 'process'");
    }
  } else {
    AK24_LOG_WARN("Module does not support function signature metadata");
  }

  // Unload the module
  AK24_LOG_INFO("Step 6: Unloading module...");
  if (!mod_ctx->unload_module(module, &error)) {
    AK24_LOG_ERROR("Failed to unload module: %s",
                   error ? error : "unknown error");
    ak_lambda_free(unload_lambda);
    ak_module_free_system_ctx(mod_ctx);
    return 1;
  }
  AK24_LOG_INFO("✓ Module unloaded successfully");

  // Cleanup
  ak_lambda_free(unload_lambda);
  ak_module_free_system_ctx(mod_ctx);

  AK24_LOG_INFO("=== Test Complete ===");
  return 0;
}

AK24_APPLICATION(app_main, on_shutdown)
