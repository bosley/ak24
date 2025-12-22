#include "kernel/application.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Global flags
static volatile int keep_running = 1;
static volatile int signal_count = 0;

/**
 * @brief Handle SIGINT (Ctrl+C) - graceful shutdown
 */
APP_ON_SIGNAL(handle_sigint, SIGINT) {
  (void)captured;
  (void)args;

  signal_count++;
  printf("\n[SIGINT] Interrupt received (count: %d)\n", signal_count);

  if (signal_count >= 2) {
    printf("[SIGINT] Forcing shutdown...\n");
    keep_running = 0;
  } else {
    printf("[SIGINT] Press Ctrl+C again to force exit\n");
  }
}

/**
 * @brief Handle SIGTERM - graceful termination
 */
APP_ON_SIGNAL(handle_sigterm, SIGTERM) {
  (void)captured;
  (void)args;

  printf("\n[SIGTERM] Termination signal received, shutting down...\n");
  keep_running = 0;
}

APP_ON_SHUTDOWN(on_shutdown) {
  time_t uptime = time(NULL) - ctx->shutdown_info->start_time;
  printf("\n=== Shutdown Summary ===\n");
  printf("Uptime: %ld seconds\n", uptime);
  printf("Signals received: %d\n", signal_count);

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
#endif
  printf("========================\n");
}

static void print_usage(const char *prog_name) {
  printf("Usage: %s [OPTIONS] [FILES...]\n\n", prog_name);
  printf("Options:\n");
  printf("  -h, --help           Show this help message\n");
  printf("  -v, --version        Show version information\n");
  printf("  -l, --log-level LVL  Set log level "
         "(trace|debug|info|warn|error|fatal)\n");
  printf("                       Default: info\n");
  printf(
      "\nLog levels are automatically parsed by the application framework.\n");
  printf("Use -l trace or --log-level debug to see more verbose output.\n");
}

APP_MAIN(app_main) {
  // Register signal handlers
  AK24_REGISTER_SIGNAL_HANDLER(handle_sigint);
  AK24_REGISTER_SIGNAL_HANDLER(handle_sigterm);

  // Log level already set by application framework
  ak_log_set_color(true);
  ak_log_set_path_format(AK24_LOG_PATH_ABBREV);

  AK24_LOG_DEBUG("Application initialized with log level: %s",
                 ak_log_level_string(ctx->log_level));

  // Parse remaining arguments (log flags already removed by framework)
  list_str_t *files = AK24_ALLOC(sizeof(list_str_t));
  list_init(files);

  list_iter_t iter = list_iter(&ctx->args);
  char **arg;

  while ((arg = list_next(&ctx->args, &iter))) {
    if (strcmp(*arg, "-h") == 0 || strcmp(*arg, "--help") == 0) {
      print_usage(list_count(&ctx->args) > 0 ? *list_get(&ctx->args, 0)
                                             : "cproc");
      list_deinit(files);
      AK24_FREE(files);
      return 0;
    } else if (strcmp(*arg, "-v") == 0 || strcmp(*arg, "--version") == 0) {
      printf("cproc v0.1.0 - Cell Processor\n");
      printf("Built with AK24 kernel\n");
      list_deinit(files);
      AK24_FREE(files);
      return 0;
    } else if ((*arg)[0] == '-') {
      fprintf(stderr, "Unknown option: %s\n", *arg);
      fprintf(stderr, "Use -h or --help for usage information\n");
      list_deinit(files);
      AK24_FREE(files);
      return 1;
    } else {
      // Regular file argument
      char *file_copy = AK24_ALLOC(strlen(*arg) + 1);
      strcpy(file_copy, *arg);
      list_push(files, file_copy);
    }
  }

  printf("=== cproc - Cell Processor ===\n");
  printf("Process ID: %d\n", getpid());
  printf("Log Level: %s\n", ak_log_level_string(ctx->log_level));

  if (list_count(files) > 0) {
    printf("\nInput files (%u):\n", list_count(files));
    list_iter_t file_iter = list_iter(files);
    char **file;
    while ((file = list_next(files, &file_iter))) {
      printf("  - %s\n", *file);
      AK24_LOG_DEBUG("Processing file: %s", *file);
    }
  } else {
    printf("\nNo input files specified.\n");
    AK24_LOG_INFO("Running in interactive mode");
  }

  printf("\nPress Ctrl+C to exit\n\n");

  // Main processing loop
  int iteration = 0;
  while (keep_running) {
    iteration++;
    AK24_LOG_TRACE("Main loop iteration: %d", iteration);
    printf("\r[%d] Running... (Ctrl+C to exit)", iteration);
    fflush(stdout);
    sleep(1);
  }

  printf("\n\nShutting down...\n");

  // Cleanup files list
  list_iter_t cleanup_iter = list_iter(files);
  char **file_ptr;
  while ((file_ptr = list_next(files, &cleanup_iter))) {
    AK24_FREE(*file_ptr);
  }
  list_deinit(files);
  AK24_FREE(files);

  return 0;
}

AK24_APPLICATION("cproc", app_main, on_shutdown)
