# Kernel

The kernel provides core abstractions for memory management, threading, and testing. It includes all kernel data structures (arbuff, buffer, context, lambda, list, map) and provides a unified interface with optional Boehm GC support.

## Core Concept

The kernel is the foundation layer that:
- Abstracts memory allocation (GC or standard malloc)
- Abstracts thread creation (GC-aware or standard pthreads)
- Provides initialization and cleanup hooks
- Includes all kernel data structures
- Supports compile-time GC enable/disable
- Provides testing macros for assertions

## Memory Management

When GC is enabled (AK24_GC_ENABLED=1):
- AK24_ALLOC -> GC_MALLOC
- AK24_ALLOC_ATOMIC -> GC_MALLOC_ATOMIC (for non-pointer data)
- AK24_REALLOC -> GC_REALLOC
- AK24_FREE -> GC_FREE (no-op in GC)

When GC is disabled:
- AK24_ALLOC -> malloc
- AK24_ALLOC_ATOMIC -> malloc
- AK24_REALLOC -> realloc
- AK24_FREE -> free

## Memory Debugging

When AK24_BUILD_DEBUG_MEMORY is enabled, all memory operations are tracked:

```bash
cmake -B build -DAK24_BUILD_DEBUG_MEMORY=ON
```

Tracked statistics:
- Total allocations
- Total frees
- Total reallocs
- Bytes allocated
- Current bytes in use
- Peak memory usage

Access stats via `ak_mem_get_stats()` or `ak_mem_print_stats()`. Stats are automatically included in `kernel_shutdown_info_t` during deinit.

## Threading

When GC is enabled:
- AK24_THREAD_CREATE -> GC_pthread_create (registers thread with GC)
- AK24_THREAD_JOIN -> pthread_join
- AK24_THREAD_DETACH -> pthread_detach

When GC is disabled:
- AK24_THREAD_CREATE -> pthread_create
- AK24_THREAD_JOIN -> pthread_join
- AK24_THREAD_DETACH -> pthread_detach

## Initialization

REQUIRED: Always call ak_kernel_init() at program start and ak_kernel_deinit() before exit, regardless of GC configuration.

### Manual Initialization

```c
#include "kernel.h"

int main(void) {
  ak_kernel_init();

  ak_kernel_deinit();
  return 0;
}
```

### Application Framework (Recommended)

The application framework abstracts away boilerplate initialization, argument parsing, and shutdown handling:

```c
#include "kernel/application.h"
#include <stdio.h>

APP_ON_SHUTDOWN(on_shutdown) {
  time_t uptime = time(NULL) - ctx->shutdown_info->start_time;
  printf("Shutting down after %ld seconds\n", uptime);
}

APP_MAIN(my_app) {
  printf("Log level: %s\n", ak_log_level_string(ctx->log_level));
  printf("Arguments (%u):\n", list_count(&ctx->args));

  list_iter_t iter = list_iter(&ctx->args);
  char **arg;
  while ((arg = list_next(&ctx->args, &iter))) {
    printf("  %s\n", *arg);
  }

  return 0;
}

AK24_APPLICATION(my_app, on_shutdown)
```

The application framework automatically:
- Initializes and deinitializes the kernel
- Parses `-l` and `--log-level` flags (trace, debug, info, warn, error, fatal)
- Removes log level flags from the argument list
- Sets the log level before calling your main function
- Registers and invokes shutdown handlers
- Cleans up all resources

Available context in `ak_app_context_t`:
- `ctx->log_level` - Parsed log level (defaults to INFO)
- `ctx->args` - List of arguments (list_str_t) with log flags removed
- `ctx->shutdown_info` - Shutdown information (only available in APP_ON_SHUTDOWN)

## Using Kernel Data Structures

```c
#include "kernel.h"

int main(void) {
  ak_kernel_init();

  ak_buffer_t *buf = ak_buffer_new(100);
  list_int_t numbers;
  list_init(&numbers);
  map_int_t scores;
  map_init_generic(&scores, sizeof(char *), map_hash_str, map_cmp_str);
  ak_context_t *ctx = ak_context_new();
  ak_lambda_t *lambda = ak_lambda_new(my_fn, NULL, NULL);
  ak_arbuff_t *arbuff = ak_arbuff_new(64);

  ak_buffer_free(buf);
  list_deinit(&numbers);
  map_deinit(&scores);
  ak_context_free(ctx);
  ak_lambda_free(lambda);
  ak_arbuff_free(arbuff);

  ak_kernel_deinit();
  return 0;
}
```

## Threading Example

```c
#include "kernel.h"

void *worker(void *arg) {
  int *data = AK24_ALLOC_ATOMIC(sizeof(int));
  *data = 42;
  return data;
}

int main(void) {
  ak_kernel_init();

  pthread_t thread;
  AK24_THREAD_CREATE(&thread, NULL, worker, NULL);

  void *result;
  AK24_THREAD_JOIN(thread, &result);

  ak_kernel_deinit();
  return 0;
}
```

## Testing Macros

```c
#include "kernel.h"
#include "assert.h"

int test_example(void) {
  int x = 5;
  AK24_TEST_ASSERT(x == 5);
  AK24_TEST_ASSERT_EQ(x, 5);
  AK24_TEST_ASSERT_NEQ(x, 10);

  int *ptr = AK24_ALLOC(sizeof(int));
  AK24_TEST_ASSERT_NOT_NULL(ptr);

  AK24_FREE(ptr);
  ptr = NULL;
  AK24_TEST_ASSERT_NULL(ptr);

  const char *str = "hello";
  AK24_TEST_ASSERT_STR_EQ(str, "hello");
  AK24_TEST_ASSERT_STR_NEQ(str, "world");

  AK24_TEST_PASS();
}

int main(void) {
  ak_kernel_init();
  AK24_TEST_RUN(test_example);
  ak_kernel_deinit();
  return 0;
}
```

## Thread-Safe Testing

For tests with concurrent access:

```c
AK24_TEST_ASSERT_ATOMIC(condition);
AK24_TEST_ASSERT_EQ_ATOMIC(a, b);
```

## Included Modules

- arbuff - Lock-free atomic ring buffer
- buffer - Dynamic byte array
- context - Hierarchical scoped key-value store
- lambda - Function closures with captured context
- list - Generic dynamic array
- map - Generic hash table

## Shutdown Callbacks

Register lambdas to execute during kernel deinit. Lambdas receive kernel_shutdown_info_t with runtime information:

```c
void cleanup_fn(void *captured, void *args) {
  kernel_shutdown_info_t *info = (kernel_shutdown_info_t *)args;
  time_t uptime = time(NULL) - info->start_time;
  printf("Kernel uptime: %ld seconds\n", uptime);
  printf("Cleaning up resources\n");
}

int main(void) {
  ak_kernel_init();

  ak_lambda_t *cleanup = ak_lambda_new(cleanup_fn, NULL, NULL);
  ak_on_shutdown(cleanup);

  ak_kernel_deinit();
  return 0;
}
```

## API

### Core Functions
- `ak_kernel_init()` - Initialize kernel (REQUIRED - always call at program start)
- `ak_kernel_deinit()` - Cleanup kernel and invoke shutdown lambdas (REQUIRED - always call before exit)
- `ak_on_shutdown(lambda)` - Register lambda to invoke during deinit
- `ak_args_to_list(argc, argv)` - Convert command-line arguments to list_str_t

### Application Framework Macros
- `APP_MAIN(name)` - Define application main entry point (receives ak_app_context_t *ctx)
- `APP_ON_SHUTDOWN(name)` - Define shutdown handler (receives ak_app_context_t *ctx)
- `AK24_APPLICATION(main_fn, shutdown_fn)` - Wire up application with automatic initialization

### Memory Management
- `AK24_ALLOC(size)` - Allocate memory
- `AK24_ALLOC_ATOMIC(size)` - Allocate non-pointer data
- `AK24_REALLOC(ptr, size)` - Reallocate memory
- `AK24_FREE(ptr)` - Free memory
- `ak_mem_get_stats()` - Get memory statistics (debug builds only)
- `ak_mem_print_stats()` - Print memory statistics (debug builds only)

### Threading
- `AK24_THREAD_CREATE(thread, attr, fn, arg)` - Create thread
- `AK24_THREAD_JOIN(thread, retval)` - Join thread
- `AK24_THREAD_DETACH(thread)` - Detach thread
