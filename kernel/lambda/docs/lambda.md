# Lambda

The lambda provides a closure-like structure for storing function pointers with captured context. It supports optional context cleanup and cloning.

## Core Concept

A lambda is a function with captured state that:
- Stores function pointer and optional captured context
- Supports optional context cleanup function
- Can be invoked with runtime arguments
- Supports cloning (shallow copy of context)
- Context can be replaced or retrieved

## Usage

```c
void my_fn(void *captured, void *args) {
  int *value = (int *)captured;
  printf("captured: %d\n", *value);
}

int captured_value = 42;
ak_lambda_t *lambda = ak_lambda_new(my_fn, &captured_value, NULL);

ak_lambda_invoke(lambda, NULL);

ak_lambda_free(lambda);
```

## With Context Cleanup

```c
void cleanup(void *ctx) {
  AK24_FREE(ctx);
}

void my_fn(void *captured, void *args) {
  int *data = (int *)captured;
  printf("data: %d\n", *data);
}

int *data = malloc(sizeof(int));
*data = 100;

ak_lambda_t *lambda = ak_lambda_new(my_fn, data, cleanup);

ak_lambda_invoke(lambda, NULL);

ak_lambda_free(lambda);
```

## With Invoke Arguments

```c
void add_fn(void *captured, void *args) {
  int *base = (int *)captured;
  int *addend = (int *)args;
  printf("result: %d\n", *base + *addend);
}

int base = 10;
ak_lambda_t *lambda = ak_lambda_new(add_fn, &base, NULL);

int addend = 5;
ak_lambda_invoke(lambda, &addend);

ak_lambda_free(lambda);
```

## Cloning

```c
ak_lambda_t *original = ak_lambda_new(my_fn, &value, NULL);

ak_lambda_t *clone = ak_lambda_clone(original);

ak_lambda_free(clone);
ak_lambda_free(original);
```

## Context Management

```c
ak_lambda_t *lambda = ak_lambda_new(my_fn, NULL, NULL);

if (ak_lambda_has_context(lambda)) {
  void *ctx = ak_lambda_get_context(lambda);
}

int new_value = 99;
ak_lambda_set_context(lambda, &new_value, NULL);

ak_lambda_free(lambda);
```

## API

- `ak_lambda_new(fn, captured_ctx, ctx_free)` - Create lambda
- `ak_lambda_invoke(lambda, invoke_args)` - Invoke lambda with args
- `ak_lambda_free(lambda)` - Free lambda and context (if cleanup provided)
- `ak_lambda_clone(lambda)` - Shallow clone (shares context, no cleanup)
- `ak_lambda_has_context(lambda)` - Check if lambda has captured context
- `ak_lambda_get_context(lambda)` - Get captured context pointer
- `ak_lambda_set_context(lambda, ctx, ctx_free)` - Replace context (cleans up old)
