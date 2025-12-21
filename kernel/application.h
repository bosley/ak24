/**
 * @file application.h
 * @brief Application framework with automatic kernel lifecycle management
 *
 * Provides macros for creating AK24 applications with automatic kernel
 * initialization, shutdown handling, signal handling, and argument processing.
 * The framework handles boilerplate setup code and provides a clean application
 * entry point.
 *
 * Key features:
 * - Automatic kernel init/deinit
 * - Command-line argument processing
 * - Shutdown callback registration
 * - Signal handler registration
 * - Application context with runtime information
 * - Macro-based application definition
 *
 * @note Use AK24_APPLICATION macro to define application entry point
 *
 * @par Example:
 * @code
 * APP_ON_SIGNAL(handle_interrupt, SIGINT) {
 *   printf("Caught Ctrl+C\n");
 * }
 *
 * APP_MAIN(my_app) {
 *   AK24_REGISTER_SIGNAL_HANDLER(handle_interrupt);
 *   printf("Args: %u\n", list_count(&ctx->args));
 *   return 0;
 * }
 *
 * APP_ON_SHUTDOWN(my_shutdown) {
 *   printf("Shutting down\n");
 * }
 *
 * AK24_APPLICATION(my_app, my_shutdown)
 * @endcode
 */

#ifndef AK24_APPLICATION_H
#define AK24_APPLICATION_H

#include "kernel.h"
#include <signal.h>
#include <string.h>

/**
 * @brief Application context
 *
 * Provides access to command-line arguments and shutdown information.
 */
typedef struct {
  list_str_t args; /**< Command-line arguments */
  kernel_shutdown_info_t
      *shutdown_info; /**< Shutdown info (NULL until shutdown) */
} ak_app_context_t;

/**
 * @def APP_ON_SHUTDOWN
 * @brief Define application shutdown handler
 *
 * Creates a function signature for shutdown callback. The function receives
 * the application context with shutdown information populated.
 *
 * @param name Name of shutdown function
 *
 * @par Example:
 * @code
 * APP_ON_SHUTDOWN(cleanup) {
 *   printf("Cleaning up resources\n");
 *   list_deinit(&ctx->args);
 * }
 * @endcode
 */
#define APP_ON_SHUTDOWN(name) void name(ak_app_context_t *ctx)

/**
 * @def APP_MAIN
 * @brief Define application main function
 *
 * Creates a function signature for application entry point. The function
 * receives the application context with parsed arguments.
 *
 * @param name Name of main function
 * @return Exit code (0 for success)
 *
 * @par Example:
 * @code
 * APP_MAIN(my_app) {
 *   if (list_count(&ctx->args) < 2) {
 *     printf("Usage: %s <file>\n", *list_get(&ctx->args, 0));
 *     return 1;
 *   }
 *   return 0;
 * }
 * @endcode
 */
#define APP_MAIN(name) int name(ak_app_context_t *ctx)

/**
 * @def APP_ON_SIGNAL
 * @brief Define signal handler function
 *
 * Creates a function signature for a signal handler. The function receives
 * the signal number. Use the global app context if needed.
 *
 * @param name Name of signal handler function
 * @param signum Signal number to handle (SIGINT, SIGTERM, SIGUSR1, etc.)
 *
 * @par Example:
 * @code
 * APP_ON_SIGNAL(handle_sigint, SIGINT) {
 *   printf("Caught SIGINT (Ctrl+C)\n");
 * }
 * @endcode
 *
 * @note Must be registered with AK24_REGISTER_SIGNAL_HANDLER() in APP_MAIN
 */
#define APP_ON_SIGNAL(name, signum)                                            \
  static const int __ak_signal_##name = signum;                                \
  static void name(void *captured, void *args);                                \
  static void __ak_signal_wrapper_##name(void *captured, void *args) {         \
    (void)captured;                                                            \
    name(captured, args);                                                      \
  }                                                                            \
  static void name(void *captured, void *args)

/**
 * @def AK24_REGISTER_SIGNAL_HANDLER
 * @brief Register a signal handler defined with APP_ON_SIGNAL
 *
 * Registers a signal handler in the application's main function. Must be
 * called after APP_ON_SIGNAL macro is used to define the handler.
 *
 * @param handler_name Name of the signal handler function
 *
 * @par Example:
 * @code
 * APP_ON_SIGNAL(handle_interrupt, SIGINT) {
 *   printf("Interrupted!\n");
 * }
 *
 * APP_MAIN(my_app) {
 *   AK24_REGISTER_SIGNAL_HANDLER(handle_interrupt);
 *   // ... rest of application
 * }
 * @endcode
 */
#define AK24_REGISTER_SIGNAL_HANDLER(handler_name)                             \
  do {                                                                         \
    ak_lambda_t *__signal_lambda_##handler_name =                              \
        ak_lambda_new(__ak_signal_wrapper_##handler_name, NULL, NULL);         \
    ak_register_signal_handler(__ak_signal_##handler_name,                     \
                               __signal_lambda_##handler_name);                \
  } while (0)

/**
 * @def AK24_APPLICATION
 * @brief Define complete AK24 application
 *
 * Generates main() function with automatic kernel initialization, argument
 * processing, shutdown callback registration, and cleanup. Both parameters
 * should be function names defined with APP_MAIN and APP_ON_SHUTDOWN.
 *
 * @param app_id_str Application identity string (used for runtime directory
 * isolation)
 * @param app_main_fn Main application function (can be NULL)
 * @param app_shutdown_fn Shutdown handler function (can be NULL)
 *
 * The generated main() function:
 * 1. Initializes kernel with ak_kernel_init(app_id_str)
 * 2. Processes command-line arguments into list
 * 3. Registers shutdown callback
 * 4. Invokes application main function
 * 5. Deinitializes kernel (triggers shutdown callbacks)
 * 6. Returns application exit code
 *
 * @par Example:
 * @code
 * APP_MAIN(my_app) {
 *   AK24_LOG_INFO("Application started");
 *   return 0;
 * }
 *
 * APP_ON_SHUTDOWN(my_cleanup) {
 *   AK24_LOG_INFO("Application ending");
 * }
 *
 * AK24_APPLICATION("my-app-v1", my_app, my_cleanup)
 * @endcode
 */
#define AK24_APPLICATION(app_id_str, app_main_fn, app_shutdown_fn)             \
  static ak_app_context_t __ak_app_ctx;                                        \
                                                                               \
  static void __ak_internal_shutdown_handler(void *captured, void *args) {     \
    ak_app_context_t *ctx = (ak_app_context_t *)captured;                      \
    ctx->shutdown_info = (kernel_shutdown_info_t *)args;                       \
    if (app_shutdown_fn) {                                                     \
      app_shutdown_fn(ctx);                                                    \
    }                                                                          \
    list_deinit(&ctx->args);                                                   \
  }                                                                            \
                                                                               \
  static list_str_t __ak_process_args(int argc, char **argv) {                 \
    list_str_t processed_args;                                                 \
    list_init(&processed_args);                                                \
                                                                               \
    for (int i = 0; i < argc; i++) {                                           \
      list_push(&processed_args, argv[i]);                                     \
    }                                                                          \
                                                                               \
    return processed_args;                                                     \
  }                                                                            \
                                                                               \
  int main(int argc, char **argv) {                                            \
    ak_kernel_init(app_id_str);                                                \
                                                                               \
    __ak_app_ctx.args = __ak_process_args(argc, argv);                         \
    __ak_app_ctx.shutdown_info = NULL;                                         \
                                                                               \
    ak_lambda_t *shutdown_lambda =                                             \
        ak_lambda_new(__ak_internal_shutdown_handler, &__ak_app_ctx, NULL);    \
    ak_on_shutdown(shutdown_lambda);                                           \
                                                                               \
    int result = 0;                                                            \
    if (app_main_fn) {                                                         \
      result = app_main_fn(&__ak_app_ctx);                                     \
    }                                                                          \
                                                                               \
    ak_kernel_deinit();                                                        \
    return result;                                                             \
  }

#endif
