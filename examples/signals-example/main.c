/**
 * @file main.c
 * @brief Signal handling example demonstrating AK24 signal framework
 *
 * This example demonstrates how to use the AK24 application framework's
 * signal handling capabilities. It registers handlers for multiple signals
 * and runs a loop that can be interrupted with Ctrl+C or other signals.
 *
 * Usage:
 *   ./signals-example
 *
 * Then send signals:
 *   Ctrl+C (SIGINT)
 *   kill -TERM <pid>
 *   kill -USR1 <pid>
 *   kill -USR2 <pid>
 */

#include "kernel/application.h"
#include <stdio.h>
#include <unistd.h>

// Global flag to control main loop
static volatile int keep_running = 1;
static volatile int signal_count = 0;

/**
 * @brief Handle SIGINT (Ctrl+C)
 */
APP_ON_SIGNAL(handle_sigint, SIGINT) {
  (void)captured; // Unused
  int signum = *(int *)args;
  (void)signum;

  signal_count++;
  printf("\n[SIGINT] Caught Ctrl+C (signal count: %d)\n", signal_count);
  printf("[SIGINT] Press Ctrl+C again to exit, or wait for normal operation\n");

  if (signal_count >= 2) {
    printf("[SIGINT] Exiting after multiple interrupts...\n");
    keep_running = 0;
  }
}

/**
 * @brief Handle SIGTERM (termination request)
 */
APP_ON_SIGNAL(handle_sigterm, SIGTERM) {
  (void)captured;
  (void)args;

  printf("\n[SIGTERM] Received termination signal\n");
  printf("[SIGTERM] Shutting down gracefully...\n");
  keep_running = 0;
}

/**
 * @brief Handle SIGUSR1 (user-defined signal 1)
 */
APP_ON_SIGNAL(handle_sigusr1, SIGUSR1) {
  (void)captured;
  int signum = *(int *)args;

  printf("\n[SIGUSR1] Received user signal 1 (signal number: %d)\n", signum);
  printf("[SIGUSR1] This is a custom user-defined signal\n");
}

/**
 * @brief Handle SIGUSR2 (user-defined signal 2)
 */
APP_ON_SIGNAL(handle_sigusr2, SIGUSR2) {
  (void)captured;
  (void)args;

  printf("\n[SIGUSR2] Received user signal 2\n");
  printf("[SIGUSR2] Toggling operation mode...\n");
}

/**
 * @brief Handle SIGHUP (hangup)
 */
APP_ON_SIGNAL(handle_sighup, SIGHUP) {
  (void)captured;
  (void)args;

  printf("\n[SIGHUP] Received hangup signal\n");
  printf("[SIGHUP] Reloading configuration...\n");
}

/**
 * @brief Application shutdown handler
 */
APP_ON_SHUTDOWN(on_shutdown) {
  time_t uptime = time(NULL) - ctx->shutdown_info->start_time;
  printf("\n=== Shutting Down ===\n");
  printf("Uptime: %ld seconds\n", uptime);
  printf("Total signals received: %d\n", signal_count);

#if AK24_BUILD_DEBUG_MEMORY
  printf("\n=== Memory Statistics ===\n");
  printf("Total allocations: %zu\n",
         ctx->shutdown_info->memory_stats.total_allocations);
  printf("Total frees: %zu\n", ctx->shutdown_info->memory_stats.total_frees);
  printf("Current bytes: %zu\n",
         ctx->shutdown_info->memory_stats.current_bytes);
  printf("Peak bytes: %zu\n", ctx->shutdown_info->memory_stats.peak_bytes);
#endif

  printf("======================\n");
}

/**
 * @brief Application main function
 */
APP_MAIN(app_main) {
  // Configure logging
  ak_log_set_level(AK24_LOG_LEVEL_INFO);
  ak_log_set_color(true);
  ak_log_set_path_format(AK24_LOG_PATH_ABBREV);

  (void)ctx;

  // Display startup information
  printf("=== AK24 Signal Handling Example ===\n\n");
  printf("This example demonstrates signal handling in AK24 applications.\n");
  printf("Process ID: %d\n\n", getpid());

  printf("Registered signal handlers:\n");
  printf("  SIGINT  (Ctrl+C)       - Interrupt (press twice to exit)\n");
  printf("  SIGTERM (kill -TERM)   - Terminate gracefully\n");
  printf("  SIGUSR1 (kill -USR1)   - User signal 1\n");
  printf("  SIGUSR2 (kill -USR2)   - User signal 2\n");
  printf("  SIGHUP  (kill -HUP)    - Hangup signal\n");
  printf("\n");

  // Register all signal handlers
  AK24_REGISTER_SIGNAL_HANDLER(handle_sigint);
  AK24_REGISTER_SIGNAL_HANDLER(handle_sigterm);
  AK24_REGISTER_SIGNAL_HANDLER(handle_sigusr1);
  AK24_REGISTER_SIGNAL_HANDLER(handle_sigusr2);
  AK24_REGISTER_SIGNAL_HANDLER(handle_sighup);

  AK24_LOG_INFO("All signal handlers registered successfully");

  printf("\nRunning main loop... (send signals to test)\n");
  printf("Press Ctrl+C to interrupt\n\n");

  // Main application loop
  int iteration = 0;
  while (keep_running) {
    iteration++;
    printf("\r[%d] Running... (signals received: %d)", iteration, signal_count);
    fflush(stdout);
    sleep(1);
  }

  printf("\n\nMain loop exited.\n");
  return 0;
}

// Define the complete application
AK24_APPLICATION(app_main, on_shutdown)
