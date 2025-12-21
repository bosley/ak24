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

The kernel provides platform-agnostic threading abstractions:

**Thread Types:**
- `AK24_THREAD` - Platform-agnostic thread handle
- `AK24_MUTEX` - Platform-agnostic mutex
- `AK24_COND` - Platform-agnostic condition variable

**Thread Operations:**
- `AK24_THREAD_CREATE` - Create thread (GC-aware when GC enabled)
- `AK24_THREAD_JOIN` - Wait for thread completion
- `AK24_THREAD_DETACH` - Detach thread

**Mutex Operations:**
- `AK24_MUTEX_INIT` - Initialize mutex
- `AK24_MUTEX_DESTROY` - Destroy mutex
- `AK24_MUTEX_LOCK` - Lock mutex
- `AK24_MUTEX_UNLOCK` - Unlock mutex

**Condition Variable Operations:**
- `AK24_COND_INIT` - Initialize condition variable
- `AK24_COND_DESTROY` - Destroy condition variable
- `AK24_COND_WAIT` - Wait on condition variable
- `AK24_COND_SIGNAL` - Signal one waiting thread
- `AK24_COND_BROADCAST` - Signal all waiting threads

These abstractions work on both POSIX (Linux, macOS, BSD) and Windows platforms. When GC is enabled, thread creation automatically registers threads with the garbage collector.

## Initialization

REQUIRED: Always call `ak_kernel_init(app_id)` at program start and `ak_kernel_deinit()` before exit, regardless of GC configuration.

The `app_id` parameter is an application identity string that:
- Must be unique per application (recommended: use reverse domain notation like "com.mycompany.myapp")
- Is hashed to create application-specific runtime directories
- Creates platform-specific runtime directories:
  - **macOS**: `~/Library/Application Support/ak24/runtime/<hash>.ak24`
  - **Linux**: `~/.local/share/ak24/runtime/<hash>.ak24`
  - **Windows**: `%LOCALAPPDATA%\ak24\runtime\<hash>.ak24`
- Prevents conflicts between different applications using AK24
- Program will exit with error message if runtime directory cannot be created

### Process-Based Locking

The kernel implements intelligent PID-based locking to prevent multiple instances of the same application:

**Lock File Contents:**
- Line 1: Application ID string
- Line 2: Process ID (PID) of the running instance

**Behavior on Initialization:**
1. Checks if lock file exists for the application ID
2. If exists, reads the stored PID
3. Verifies if the process is still running:
   - **POSIX**: Uses `kill(pid, 0)` to check process existence
   - **Windows**: Uses `OpenProcess()` and `GetExitCodeProcess()`
4. If process is running: Exits with error showing app ID and PID
5. If process is NOT running: Automatically removes stale lock and continues
6. Creates new lock file with current PID

**Behavior on Cleanup:**
- `ak_kernel_deinit()` automatically removes the lock file
- Lock is cleaned up even on normal exit, allowing immediate restart

**Special Cases:**
- Applications with ID `"ak24-test"` skip lock checking entirely
- This allows parallel test execution without conflicts
- Production applications should use unique, meaningful IDs

**Error Messages:**
```
AK24 Error: Application 'com.example.myapp' is already running (PID: 12345)
AK24 Error: Lock file: '/path/to/runtime/hash.ak24'
```

**Stale Lock Cleanup:**
```
AK24 Info: Removing stale lock file (PID 12345 not running)
```

### Manual Initialization

```c
#include "kernel.h"

int main(void) {
  ak_kernel_init("com.example.myapp");

  // Your application code here

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

AK24_APPLICATION("com.example.myapp", my_app, on_shutdown)
```

The application framework automatically:
- Initializes and deinitializes the kernel with the provided app ID
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
  ak_kernel_init("my-app");

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
  ak_kernel_init("my-app");

  AK24_THREAD thread;
  AK24_THREAD_CREATE(&thread, worker, NULL);

  AK24_THREAD_JOIN(thread);

  ak_kernel_deinit();
  return 0;
}
```

## Signal Handling

Register lambdas to handle OS signals (SIGINT, SIGTERM, etc.):

```c
void handle_interrupt(void *captured, void *args) {
  int *signum = (int *)args;
  printf("Caught signal %d\n", *signum);
}

int main(void) {
  ak_kernel_init("my-app");

  ak_lambda_t *handler = ak_lambda_new(handle_interrupt, NULL, NULL);
  ak_register_signal_handler(SIGINT, handler);

  // Your application code

  ak_unregister_signal_handler(SIGINT);
  ak_kernel_deinit();
  return 0;
}
```

The application framework provides a simpler interface:

```c
APP_ON_SIGNAL(handle_sigint, SIGINT) {
  printf("Interrupted!\n");
}

APP_MAIN(my_app) {
  AK24_REGISTER_SIGNAL_HANDLER(handle_sigint);
  // Your application code
  return 0;
}

AK24_APPLICATION("my-app", my_app, NULL)
```

## Logging

Built-in logging system with six severity levels:

```c
AK24_LOG_TRACE("Detailed trace: %d", value);
AK24_LOG_DEBUG("Debug info: %s", str);
AK24_LOG_INFO("Application started");
AK24_LOG_WARN("Warning: %s", message);
AK24_LOG_ERROR("Error: %d", error_code);
AK24_LOG_FATAL("Fatal error!");
```

Configure logging behavior:

```c
ak_log_set_level(AK24_LOG_LEVEL_DEBUG);  // Filter below DEBUG
ak_log_set_quiet(true);                   // Disable console output
ak_log_set_colors(false);                 // Disable color output
ak_log_add_fp(file_ptr, AK24_LOG_LEVEL_INFO);  // Log to file
```

For thread-safe logging, provide a lock function:

```c
AK24_MUTEX log_mutex = AK24_MUTEX_INITIALIZER;

void log_lock(bool lock, void *udata) {
  if (lock) AK24_MUTEX_LOCK((AK24_MUTEX *)udata);
  else AK24_MUTEX_UNLOCK((AK24_MUTEX *)udata);
}

ak_log_set_lock(log_lock, &log_mutex);
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
  ak_kernel_init("my-app");
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

## Module System

Load and manage dynamic modules at runtime:

```c
ak_module_ctx_t *mod_ctx = ak_module_get_system_ctx();

ak_module_load_options_t opts = {
  .module_path = "./my_module.so",
  .thread_safe = true,
  .unload_callback = NULL
};

const char *error = NULL;
ak_module_handle_t *handle = mod_ctx->load_module(&opts, &error);

if (handle) {
  void *fn = ak_handle_get_function(handle, "my_function");
  const char *name = ak_handle_get_info(handle, "name");

  mod_ctx->unload_module(handle, &error);
}

ak_module_free_system_ctx(mod_ctx);
```

## Utility Modules

### UTF-8 Text Handling

The UTF-8 module provides utilities for working with UTF-8 encoded text:

```c
const uint8_t *text = (uint8_t *)"Hello 世界 café 🎉";
size_t byte_len = strlen((char *)text);

// Count characters (not bytes)
size_t char_count = ak_utf8_char_count(text, byte_len);

// Validate UTF-8 encoding
if (ak_utf8_validate(text, byte_len)) {
  printf("Valid UTF-8\n");
}

// Get byte length of first character
size_t first_char_bytes = ak_utf8_char_size(text);

// Decode codepoints
const uint8_t *ptr = text;
while (*ptr) {
  uint32_t codepoint = ak_utf8_decode(ptr, &ptr);
  // Process codepoint
}

// Character classification
if (ak_utf8_is_whitespace(codepoint)) { }
if (ak_utf8_is_digit(codepoint)) { }
if (ak_utf8_is_alpha(codepoint)) { }
```

**Documentation**: [kernel/utf8/docs/utf8.md](kernel/utf8/docs/utf8.md)

### String Interning

String interning ensures only one copy of each unique string exists, enabling O(1) equality checks:

```c
// Intern strings
const char *s1 = ak_intern("identifier");
const char *s2 = ak_intern("identifier");
assert(s1 == s2);  // Same pointer!

// Intern substring
const char *source = "identifier+123";
const char *id = ak_intern_n(source, 10);  // "identifier"

// Check if already interned (doesn't create new entry)
const char *existing = ak_intern_lookup("identifier");

// Statistics
ak_intern_stats_t stats = ak_intern_get_stats();
printf("Interned: %zu strings, %zu bytes\n",
       stats.string_count, stats.total_bytes);
```

**Use cases**: Symbol table keys, identifiers, type names, configuration keys

**Documentation**: [kernel/intern/docs/intern.md](kernel/intern/docs/intern.md)

### File Path Manipulation

Cross-platform file path handling:

```c
// Join paths with correct separator
ak_buffer_t *path = ak_filepath_join(3, "/home", "user", "file.txt");
// POSIX: "/home/user/file.txt"
// Windows: "\\home\\user\\file.txt"

// Get system directories
ak_buffer_t *home = ak_filepath_home();       // User home
ak_buffer_t *cache = ak_filepath_cache();     // Cache directory
ak_buffer_t *config = ak_filepath_config();   // Config directory
ak_buffer_t *data = ak_filepath_data();       // Data directory
ak_buffer_t *temp = ak_filepath_temp();       // Temp directory

// Extract path components
const char *base = ak_filepath_basename("/path/to/file.txt");  // "file.txt"
const char *dir = ak_filepath_dirname("/path/to/file.txt");    // "/path/to"
const char *ext = ak_filepath_extension("file.txt");           // ".txt"

// Normalize paths (resolve . and ..)
ak_buffer_t *normalized = ak_filepath_normalize("/path/./to/../file.txt");
// Result: "/path/file.txt"

// Check path properties
if (ak_filepath_is_absolute("/path/to/file")) { }
if (ak_filepath_is_relative("../file")) { }

// Platform separators
char sep = ak_filepath_separator();       // '/' or '\\'
char list_sep = ak_filepath_list_separator();  // ':' or ';'

ak_buffer_free(path);
ak_buffer_free(normalized);
```

**Documentation**: [kernel/filepath/docs/filepath.md](kernel/filepath/docs/filepath.md)

### Source Location Tracking

Compiler-focused module for tracking source code positions:

```c
// Load source file
ak_source_file_t *file = ak_source_file_from_path("input.c");

// Create location from byte offset
ak_source_loc_t loc = ak_source_loc_from_offset(file, 42);
printf("Error at %s:%zu:%zu\n",
       ak_source_file_filename(file), loc.line, loc.column);

// Create source range
ak_source_range_t range = ak_source_range_new(file, 10, 50);

// Extract source text
const char *text = ak_source_range_text(&range);
printf("Source: %s\n", text);

// Get line text
const char *line = ak_source_file_get_line_text(file, 5);

// Reference counting
ak_source_file_retain(file);   // Increment refcount
ak_source_file_release(file);  // Decrement (frees at 0)
ak_source_file_release(file);
```

**Use cases**: Compiler error messages, syntax highlighting, IDE features, debug info

**Documentation**: [kernel/sourceloc/docs/sourceloc.md](kernel/sourceloc/docs/sourceloc.md)

Modules must implement the `ak_module_vtable_t` interface defined in [interfaces.h](../kernel/interfaces.h).

## Included Modules

- arbuff - Lock-free atomic ring buffer
- atoms - Interned immutable strings (atom cube)
- buffer - Dynamic byte array
- context - Hierarchical scoped key-value store
- forms - S-expression parsing and manipulation
- lambda - Function closures with captured context
- list - Generic dynamic array
- log - Thread-safe logging system
- map - Generic hash table
- scanner - Token scanning and pattern matching

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
  ak_kernel_init("my-app");

  ak_lambda_t *cleanup = ak_lambda_new(cleanup_fn, NULL, NULL);
  ak_on_shutdown(cleanup);

  ak_kernel_deinit();
  return 0;
}
```

## API

### Core Functions
- `ak_kernel_init("my-app")` - Initialize kernel (REQUIRED - always call at program start)
- `ak_kernel_deinit()` - Cleanup kernel and invoke shutdown lambdas (REQUIRED - always call before exit)
- `ak_on_shutdown(lambda)` - Register lambda to invoke during deinit
- `ak_args_to_list(argc, argv)` - Convert command-line arguments to list_str_t

### Application Framework Macros
- `APP_MAIN(name)` - Define application main entry point (receives ak_app_context_t *ctx)
- `APP_ON_SHUTDOWN(name)` - Define shutdown handler (receives ak_app_context_t *ctx)
- `AK24_APPLICATION(app_id, main_fn, shutdown_fn)` - Wire up application with automatic initialization

### Memory Management
- `AK24_ALLOC(size)` - Allocate memory
- `AK24_ALLOC_ATOMIC(size)` - Allocate non-pointer data
- `AK24_REALLOC(ptr, size)` - Reallocate memory
- `AK24_FREE(ptr)` - Free memory
- `ak_mem_get_stats()` - Get memory statistics (debug builds only)
- `ak_mem_print_stats()` - Print memory statistics (debug builds only)

### Threading
- `AK24_THREAD_CREATE(thread, fn, arg)` - Create thread
- `AK24_THREAD_JOIN(thread)` - Join thread
- `AK24_THREAD_DETACH(thread)` - Detach thread

### Signal Handling
- `ak_register_signal_handler(signum, lambda)` - Register signal handler
- `ak_unregister_signal_handler(signum)` - Unregister signal handler
- `APP_ON_SIGNAL(name, signum)` - Define signal handler (application framework)
- `AK24_REGISTER_SIGNAL_HANDLER(handler)` - Register handler in APP_MAIN

### Logging
- `AK24_LOG_TRACE/DEBUG/INFO/WARN/ERROR/FATAL(...)` - Log messages
- `ak_log_set_level(level)` - Set minimum log level
- `ak_log_set_quiet(enable)` - Enable/disable console output
- `ak_log_set_lock(fn, udata)` - Enable thread-safe logging
- `ak_log_add_fp(file, level)` - Add file output target

### Module System
- `ak_module_get_system_ctx()` - Get module system context
- `ak_module_free_system_ctx(ctx)` - Free module context
- `ak_handle_get_function(handle, name)` - Get function from module
- `ak_handle_get_info(handle, key)` - Get module metadata
