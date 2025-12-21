#include "kernel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if AK24_GC_ENABLED
#include <sched.h>
#define GC_THREADS 1
#include <gc.h>
#endif

// Forward declarations from platform-specific files
void ak_signal_handlers_init(void);
void ak_signal_handlers_deinit(void);

// Forward declaration from runtime.c
int init_runtime_directory(const char *app_id);

extern char *ak24_kernel_lock_file_path;

#ifdef AK24_PLATFORM_WINDOWS
void ak_print_warning(const char *format, ...);
#endif

static list_void_t shutdown_lambdas;
static int shutdown_lambdas_initialized = 0;
static time_t kernel_start_time;

#if AK24_GC_ENABLED

void ak_kernel_init(const char *app_id) {
  kernel_start_time = time(NULL);
#ifdef AK24_PLATFORM_WINDOWS
  ak_print_warning("\n*** WARNING ***\n");
  ak_print_warning("AK24 Windows support is UNTESTED CODE\n");
  ak_print_warning("Please report any issues to the development team\n");
  ak_print_warning("***************\n\n");
#endif
  GC_set_warn_proc(GC_ignore_warn_proc);
  GC_INIT();
  ak_intern_init();
  ak_sourceloc_init();
  ak_filepath_init();
  init_runtime_directory(app_id);
  list_init(&shutdown_lambdas);
  shutdown_lambdas_initialized = 1;
  ak_signal_handlers_init();
}

void ak_kernel_deinit(void) {
  if (shutdown_lambdas_initialized) {
    kernel_shutdown_info_t info;
    info.start_time = kernel_start_time;
#if AK24_BUILD_DEBUG_MEMORY
    info.memory_stats = ak_mem_get_stats();
#endif

    list_iter_t iter = list_iter(&shutdown_lambdas);
    void **lambda_ptr;
    while ((lambda_ptr = list_next(&shutdown_lambdas, &iter))) {
      ak_lambda_t *lambda = (ak_lambda_t *)*lambda_ptr;
      if (lambda) {
        ak_lambda_invoke(lambda, &info);
      }
    }
    list_deinit(&shutdown_lambdas);
    shutdown_lambdas_initialized = 0;
  }

  // Remove lock file with sanity check (before filepath shutdown)
  if (ak24_kernel_lock_file_path) {
    // Verify the lock file is in the expected runtime directory
    ak_buffer_t *data_dir = ak_filepath_data();
    if (data_dir) {
      ak_buffer_t *expected_runtime_dir = ak_filepath_join(
          3, (const char *)ak_buffer_data(data_dir), "ak24", "runtime");
      ak_buffer_free(data_dir);

      if (expected_runtime_dir) {
        const char *expected_path =
            (const char *)ak_buffer_data(expected_runtime_dir);
        size_t expected_len = strlen(expected_path);

        // Check if lock file path starts with expected runtime directory
        if (strncmp(ak24_kernel_lock_file_path, expected_path, expected_len) ==
            0) {
          remove(ak24_kernel_lock_file_path);
        } else {
          fprintf(stderr, "AK24 Warning: Lock file path not in expected "
                          "location, skipping removal\n");
          fprintf(stderr, "  Expected prefix: %s\n", expected_path);
          fprintf(stderr, "  Actual path: %s\n", ak24_kernel_lock_file_path);
        }
        ak_buffer_free(expected_runtime_dir);
      }
    }
    free(ak24_kernel_lock_file_path);
    ak24_kernel_lock_file_path = NULL;
  }

  ak_signal_handlers_deinit();
  ak_filepath_shutdown();
  ak_sourceloc_shutdown();
  ak_intern_shutdown();

  sched_yield();
  sched_yield();
}

#else

void ak_kernel_init(const char *app_id) {
  kernel_start_time = time(NULL);
#ifdef AK24_PLATFORM_WINDOWS
  ak_print_warning("\n*** WARNING ***\n");
  ak_print_warning("AK24 Windows support is UNTESTED CODE\n");
  ak_print_warning("Please report any issues to the development team\n");
  ak_print_warning("***************\n\n");
#endif
  ak_intern_init();
  ak_sourceloc_init();
  ak_filepath_init();
  init_runtime_directory(app_id);
  list_init(&shutdown_lambdas);
  shutdown_lambdas_initialized = 1;
  ak_signal_handlers_init();
}

void ak_kernel_deinit(void) {
  if (shutdown_lambdas_initialized) {
    kernel_shutdown_info_t info;
    info.start_time = kernel_start_time;
#if AK24_BUILD_DEBUG_MEMORY
    info.memory_stats = ak_mem_get_stats();
#endif

    list_iter_t iter = list_iter(&shutdown_lambdas);
    void **lambda_ptr;
    while ((lambda_ptr = list_next(&shutdown_lambdas, &iter))) {
      ak_lambda_t *lambda = (ak_lambda_t *)*lambda_ptr;
      if (lambda) {
        ak_lambda_invoke(lambda, &info);
      }
    }
    list_deinit(&shutdown_lambdas);
    shutdown_lambdas_initialized = 0;
  }

  // Remove lock file with sanity check (before filepath shutdown)
  if (ak24_kernel_lock_file_path) {
    // Verify the lock file is in the expected runtime directory
    ak_buffer_t *data_dir = ak_filepath_data();
    if (data_dir) {
      ak_buffer_t *expected_runtime_dir = ak_filepath_join(
          3, (const char *)ak_buffer_data(data_dir), "ak24", "runtime");
      ak_buffer_free(data_dir);

      if (expected_runtime_dir) {
        const char *expected_path =
            (const char *)ak_buffer_data(expected_runtime_dir);
        size_t expected_len = strlen(expected_path);

        // Check if lock file path starts with expected runtime directory
        if (strncmp(ak24_kernel_lock_file_path, expected_path, expected_len) ==
            0) {
          remove(ak24_kernel_lock_file_path);
        } else {
          fprintf(stderr, "AK24 Warning: Lock file path not in expected "
                          "location, skipping removal\n");
          fprintf(stderr, "  Expected prefix: %s\n", expected_path);
          fprintf(stderr, "  Actual path: %s\n", ak24_kernel_lock_file_path);
        }
        ak_buffer_free(expected_runtime_dir);
      }
    }
    free(ak24_kernel_lock_file_path);
    ak24_kernel_lock_file_path = NULL;
  }

  ak_signal_handlers_deinit();
  ak_filepath_shutdown();
  ak_sourceloc_shutdown();
  ak_intern_shutdown();
}

#endif

void ak_on_shutdown(ak_lambda_t *lambda) {
  if (!lambda || !shutdown_lambdas_initialized) {
    return;
  }
  list_push(&shutdown_lambdas, lambda);
}

list_str_t ak_args_to_list(int argc, char **argv) {
  list_str_t args;
  list_init(&args);
  for (int i = 0; i < argc; i++) {
    list_push(&args, argv[i]);
  }
  return args;
}
