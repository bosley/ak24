/**
 * @file main.c
 * @brief AK24 Module System Integration Test
 *
 * This integration test demonstrates:
 * - Loading the installed AK24 library
 * - Loading a dynamic module with allocator interface
 * - Calling module functions via get_function
 * - Module lifecycle management (load/use/unload)
 * - Clean shutdown with statistics
 */

#include <application.h>
#include <interfaces.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

APP_ON_SHUTDOWN(on_shutdown) {
  time_t uptime = time(NULL) - ctx->shutdown_info->start_time;
  printf("\n=== Shutting Down ===\n");
  printf("Uptime: %ld seconds\n", uptime);

#if AK24_BUILD_DEBUG_MEMORY
  printf("\n=== Memory Statistics ===\n");
  printf("Total allocations: %zu\n",
         ctx->shutdown_info->memory_stats.total_allocations);
  printf("Total frees: %zu\n", ctx->shutdown_info->memory_stats.total_frees);
  printf("Total reallocs: %zu\n",
         ctx->shutdown_info->memory_stats.total_reallocs);
  printf("Bytes allocated: %zu\n",
         ctx->shutdown_info->memory_stats.bytes_allocated);
  printf("Current bytes: %zu\n",
         ctx->shutdown_info->memory_stats.current_bytes);
  printf("Peak bytes: %zu\n", ctx->shutdown_info->memory_stats.peak_bytes);
  printf("=========================\n");
#endif
}

APP_MAIN(app_main) {
  (void)ctx; // Suppress unused parameter warning

  printf("=== AK24 Module System Integration Test ===\n\n");

  // Get the current working directory to locate the module
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    printf("✗ Failed to get current directory\n");
    return 1;
  }

  // Construct module path
  char module_path[1536];
  snprintf(module_path, sizeof(module_path), "%s/build/libtest_module.dylib",
           cwd);

  printf("Module path: %s\n\n", module_path);

  // Get module system context
  printf("Step 1: Getting module system context...\n");
  ak_module_ctx_t *mod_ctx = ak_module_get_system_ctx();
  if (!mod_ctx) {
    printf("✗ Failed to get module system context\n");
    return 1;
  }
  printf("✓ Module system initialized\n\n");

  // Load module options
  ak_module_load_options_t load_opts = {.module_path = module_path,
                                        .unload_callback = NULL,
                                        .unload_callback_ctx = NULL,
                                        .thread_safe = false};

  // Load the module
  printf("Step 2: Loading module...\n");
  printf("  DEBUG: Calling load_module with path: %s\n", module_path);
  const char *error = NULL;
  ak_module_handle_t *module = mod_ctx->load_module(&load_opts, &error);
  if (!module) {
    printf("✗ Failed to load module: %s\n", error ? error : "unknown error");
    ak_module_free_system_ctx(mod_ctx);
    return 1;
  }
  printf("✓ Module loaded successfully\n\n");

  // Get module info
  printf("Step 3: Querying module info...\n");
  const char *name = module->vtable.ak_module_info("name");
  const char *version = module->vtable.ak_module_info("version");
  const char *description = module->vtable.ak_module_info("description");
  printf("  Name: %s\n", name ? name : "N/A");
  printf("  Version: %s\n", version ? version : "N/A");
  printf("  Description: %s\n", description ? description : "N/A");
  printf("\n");

  // Get and call module functions
  printf("Step 4: Calling module functions...\n\n");

  // Get process function
  typedef void (*module_fn_t)(void *);
  module_fn_t process_fn = (module_fn_t)module->vtable.ak_module_get_function(
      module->module_ctx, "process");
  if (process_fn) {
    printf("Calling process()...\n");
    process_fn(NULL);
    printf("\n");
  }

  // Get and call allocate function
  module_fn_t allocate_fn = (module_fn_t)module->vtable.ak_module_get_function(
      module->module_ctx, "allocate");
  if (allocate_fn) {
    printf("Calling allocate() to test allocator...\n");
    allocate_fn(NULL);
    printf("\n");
  }

  // Get and call sleep function
  module_fn_t sleep_fn = (module_fn_t)module->vtable.ak_module_get_function(
      module->module_ctx, "sleep");
  if (sleep_fn) {
    printf("Calling sleep(500ms)...\n");
    int sleep_ms = 500;
    sleep_fn(&sleep_ms);
    printf("\n");
  }

  // Unload the module
  printf("Step 5: Unloading module...\n");
  if (!mod_ctx->unload_module(module, &error)) {
    printf("✗ Failed to unload module: %s\n", error ? error : "unknown error");
    ak_module_free_system_ctx(mod_ctx);
    return 1;
  }
  printf("✓ Module unloaded successfully\n\n");

  // Cleanup
  ak_module_free_system_ctx(mod_ctx);

  printf("=== Test Complete ===\n");
  return 0;
}

AK24_APPLICATION(app_main, on_shutdown)
