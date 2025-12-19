#ifndef AK24_APPLICATION_H
#define AK24_APPLICATION_H

#include "kernel.h"
#include <string.h>

typedef struct {
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
    ak_kernel_init();                                                          \
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
