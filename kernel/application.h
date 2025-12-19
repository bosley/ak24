#ifndef AK24_APPLICATION_H
#define AK24_APPLICATION_H

#include "kernel.h"
#include <string.h>

typedef struct {
  ak_log_level_t log_level;
  list_str_t args;
  kernel_shutdown_info_t *shutdown_info;
} ak_app_context_t;

#define APP_ON_SHUTDOWN(name) void name(ak_app_context_t *ctx)

#define APP_MAIN(name) int name(ak_app_context_t *ctx)

#define AK24_APPLICATION(app_main_fn, app_shutdown_fn)                         \
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
  static ak_log_level_t __ak_parse_log_level(const char *level_str) {          \
    if (strcmp(level_str, "trace") == 0)                                       \
      return AK24_LOG_LEVEL_TRACE;                                             \
    if (strcmp(level_str, "debug") == 0)                                       \
      return AK24_LOG_LEVEL_DEBUG;                                             \
    if (strcmp(level_str, "info") == 0)                                        \
      return AK24_LOG_LEVEL_INFO;                                              \
    if (strcmp(level_str, "warn") == 0)                                        \
      return AK24_LOG_LEVEL_WARN;                                              \
    if (strcmp(level_str, "error") == 0)                                       \
      return AK24_LOG_LEVEL_ERROR;                                             \
    if (strcmp(level_str, "fatal") == 0)                                       \
      return AK24_LOG_LEVEL_FATAL;                                             \
    return AK24_LOG_LEVEL_INFO;                                                \
  }                                                                            \
                                                                               \
  static list_str_t __ak_process_args(int argc, char **argv,                   \
                                      ak_log_level_t *log_level) {             \
    list_str_t processed_args;                                                 \
    list_init(&processed_args);                                                \
                                                                               \
    for (int i = 0; i < argc; i++) {                                           \
      if ((strcmp(argv[i], "-l") == 0 ||                                       \
           strcmp(argv[i], "--log-level") == 0) &&                             \
          i + 1 < argc) {                                                      \
        *log_level = __ak_parse_log_level(argv[i + 1]);                        \
        i++;                                                                   \
        continue;                                                              \
      }                                                                        \
      list_push(&processed_args, argv[i]);                                     \
    }                                                                          \
                                                                               \
    return processed_args;                                                     \
  }                                                                            \
                                                                               \
  int main(int argc, char **argv) {                                            \
    ak_kernel_init();                                                          \
                                                                               \
    __ak_app_ctx.log_level = AK24_LOG_LEVEL_INFO;                              \
    __ak_app_ctx.args =                                                        \
        __ak_process_args(argc, argv, &__ak_app_ctx.log_level);                \
    __ak_app_ctx.shutdown_info = NULL;                                         \
                                                                               \
    ak_log_set_level(__ak_app_ctx.log_level);                                  \
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
