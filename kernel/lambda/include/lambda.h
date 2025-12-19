#ifndef AK24_LAMBDA_H
#define AK24_LAMBDA_H

#define AK24_LAMBDA_VERSION "0.1.0"

typedef void (*ak_lambda_fn)(void *captured_ctx, void *invoke_args);

typedef void (*ak_lambda_ctx_free_fn)(void *ctx);

typedef struct ak_lambda_t {
  ak_lambda_fn fn;
  void *captured_ctx;
  ak_lambda_ctx_free_fn ctx_free;
} ak_lambda_t;

ak_lambda_t *ak_lambda_new(ak_lambda_fn fn, void *captured_ctx,
                           ak_lambda_ctx_free_fn ctx_free);

void ak_lambda_invoke(ak_lambda_t *lambda, void *invoke_args);

void ak_lambda_free(ak_lambda_t *lambda);

ak_lambda_t *ak_lambda_clone(ak_lambda_t *lambda);

int ak_lambda_has_context(ak_lambda_t *lambda);

void *ak_lambda_get_context(ak_lambda_t *lambda);

int ak_lambda_set_context(ak_lambda_t *lambda, void *captured_ctx,
                          ak_lambda_ctx_free_fn ctx_free);

#endif
