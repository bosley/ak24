#include "lambda.h"
#include <stddef.h>

static void dummy_fn(void *captured_ctx, void *invoke_args) {
  (void)captured_ctx;
  (void)invoke_args;
}

static void dummy_free(void *ctx) { (void)ctx; }

int main(void) {
  ak_lambda_t *lambda = ak_lambda_new(dummy_fn, NULL, NULL);
  ak_lambda_invoke(lambda, NULL);
  ak_lambda_free(lambda);

  lambda = ak_lambda_new(dummy_fn, (void *)0x1234, dummy_free);
  ak_lambda_t *clone = ak_lambda_clone(lambda);
  ak_lambda_invoke(clone, (void *)0x5678);
  ak_lambda_free(clone);
  ak_lambda_free(lambda);

  return 0;
}
