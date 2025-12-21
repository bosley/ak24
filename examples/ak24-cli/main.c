#include "kernel/application.h"
#include <stdio.h>

APP_ON_SHUTDOWN(on_shutdown) {
  time_t uptime = time(NULL) - ctx->shutdown_info->start_time;
  printf("Shutting down after %ld seconds\n", uptime);

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
  ak_log_set_level(AK24_LOG_LEVEL_TRACE);
  ak_log_set_color(true);
  ak_log_set_path_format(AK24_LOG_PATH_ABBREV);

  printf("ak24 v1.0.0\n");
  printf("Arguments (%u):\n", list_count(&ctx->args));

  list_iter_t iter = list_iter(&ctx->args);
  char **arg;
  while ((arg = list_next(&ctx->args, &iter))) {
    printf("  %s\n", *arg);
  }

  AK24_LOG_TRACE("This is a trace message");
  AK24_LOG_DEBUG("This is a debug message");
  AK24_LOG_INFO("This is an info message");
  AK24_LOG_WARN("This is a warning message");
  AK24_LOG_ERROR("This is an error message");

  return 0;
}

AK24_APPLICATION("ak24-cli", app_main, on_shutdown)
