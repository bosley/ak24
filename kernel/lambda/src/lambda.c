#include "lambda.h"
#include "kernel.h"

ak_lambda_t *ak_lambda_new(ak_lambda_fn fn, void *captured_ctx,
                           ak_lambda_ctx_free_fn ctx_free) {
  if (!fn) {
    return NULL;
  }

  ak_lambda_t *lambda = AK24_ALLOC(sizeof(ak_lambda_t));
  if (!lambda) {
    return NULL;
  }

  lambda->fn = fn;
  lambda->captured_ctx = captured_ctx;
  lambda->ctx_free = ctx_free;

  return lambda;
}

void ak_lambda_invoke(ak_lambda_t *lambda, void *invoke_args) {
  if (!lambda || !lambda->fn) {
    return;
  }

  lambda->fn(lambda->captured_ctx, invoke_args);
}

void ak_lambda_free(ak_lambda_t *lambda) {
  if (!lambda) {
    return;
  }

  if (lambda->captured_ctx && lambda->ctx_free) {
    lambda->ctx_free(lambda->captured_ctx);
  }

  AK24_FREE(lambda);
}

ak_lambda_t *ak_lambda_clone(ak_lambda_t *lambda) {
  if (!lambda) {
    return NULL;
  }

  ak_lambda_t *clone = AK24_ALLOC(sizeof(ak_lambda_t));
  if (!clone) {
    return NULL;
  }

  clone->fn = lambda->fn;
  clone->captured_ctx = lambda->captured_ctx;
  clone->ctx_free = NULL;

  return clone;
}

int ak_lambda_has_context(ak_lambda_t *lambda) {
  if (!lambda) {
    return 0;
  }

  return lambda->captured_ctx != NULL;
}

void *ak_lambda_get_context(ak_lambda_t *lambda) {
  if (!lambda) {
    return NULL;
  }

  return lambda->captured_ctx;
}

int ak_lambda_set_context(ak_lambda_t *lambda, void *captured_ctx,
                          ak_lambda_ctx_free_fn ctx_free) {
  if (!lambda) {
    return -1;
  }

  if (lambda->captured_ctx && lambda->ctx_free) {
    lambda->ctx_free(lambda->captured_ctx);
  }

  lambda->captured_ctx = captured_ctx;
  lambda->ctx_free = ctx_free;

  return 0;
}
