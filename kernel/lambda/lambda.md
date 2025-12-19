# Lambda Library

The lambda library provides a flexible callback mechanism with support for both captured context (persistent data) and invocation arguments (runtime data).

## Core Concept

A lambda consists of:
- A function pointer with signature `void (*fn)(void *captured_ctx, void *invoke_args)`
- Optional captured context (set at creation, persists across invocations)
- Optional cleanup function for the captured context

## Usage

```c
typedef struct {
  int multiplier;
} my_context_t;

typedef struct {
  int value;
} my_args_t;

void my_lambda_fn(void *captured_ctx, void *invoke_args) {
  my_context_t *ctx = (my_context_t *)captured_ctx;
  my_args_t *args = (my_args_t *)invoke_args;

  int result = ctx->multiplier * args->value;
  printf("Result: %d\n", result);
}

my_context_t *ctx = AK24_ALLOC(sizeof(my_context_t));
ctx->multiplier = 5;

ak_lambda_t *lambda = ak_lambda_new(my_lambda_fn, ctx, free_ctx_fn);

my_args_t args1 = {10};
ak_lambda_invoke(lambda, &args1);

my_args_t args2 = {20};
ak_lambda_invoke(lambda, &args2);

ak_lambda_free(lambda);
```

## Key Features

- **Captured Context**: Persistent data bound to the lambda at creation time
- **Invocation Arguments**: Runtime data passed during each invocation
- **Memory Management**: Optional cleanup function for automatic context deallocation
- **Cloning**: Create shallow copies of lambdas that share the same captured context

## API

- `ak_lambda_new(fn, captured_ctx, ctx_free)` - Create a new lambda
- `ak_lambda_invoke(lambda, invoke_args)` - Execute the lambda with runtime arguments
- `ak_lambda_free(lambda)` - Free the lambda and optionally its captured context
- `ak_lambda_clone(lambda)` - Create a shallow copy (does not own captured context)
- `ak_lambda_get_context(lambda)` - Access the captured context
- `ak_lambda_set_context(lambda, ctx, ctx_free)` - Update the captured context
