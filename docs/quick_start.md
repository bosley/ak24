# AK24 Kernel - Quick Reference

**Purpose**: This document is to help a new developer using the library get up and running by explaining the overall concepts that the library provides.ß

---

## Core Principle: NEVER USE STANDARD MALLOC/FREE

```c
// ❌ NEVER DO THIS
void* ptr = malloc(size);
free(ptr);

// ✅ ALWAYS DO THIS
void* ptr = AK24_ALLOC(size);
AK24_FREE(ptr);  // No-op with GC, actual free without GC
```

---

## Application Structure

All applications use the **application.h** framework:

```c
#include "kernel/application.h"

APP_MAIN(my_app) {
    // Access args via ctx->args (list_str_t)
    // Kernel is already initialized
    return 0;
}

APP_ON_SHUTDOWN(cleanup) {
    // ctx->shutdown_info contains runtime stats
    // Called automatically before kernel deinit
}

AK24_APPLICATION("com.mycompany.app", my_app, cleanup)
```

**Key points:**
- Use `APP_MAIN` for entry point (not `int main`)
- Kernel init/deinit is automatic
- Application ID should be unique (reverse domain notation)
- Arguments are pre-parsed into `ctx->args`

---

## Memory Management Rules

### Standard Allocation
```c
AK24_ALLOC(size)           // GC_MALLOC or malloc
AK24_ALLOC_ATOMIC(size)    // GC_MALLOC_ATOMIC (for non-pointer data) or malloc
AK24_REALLOC(ptr, size)    // GC_REALLOC or realloc
AK24_FREE(ptr)             // No-op with GC, free() without GC
```

### Arena Allocation (Bulk/Temporary)
Use arenas for:
- AST construction
- Parser temporary buffers
- Per-request web server data
- Batch allocations that die together

```c
ak_arena_t *arena = ak_arena_new_default();  // 64KB blocks
void *ptr = ak_arena_alloc(arena, size);     // Fast bump allocation
char *str = ak_arena_strdup(arena, "text");  // String duplication
ak_arena_reset(arena);                       // Reuse arena, free all
ak_arena_free(arena);                        // Free everything at once
```

**Critical**: Arena allocations are NOT individually freeable. Free the entire arena when done.

---

## Essential Data Structures

### Lists (Generic Dynamic Arrays)
```c
list_t(int) numbers;
list_init(&numbers);
list_push(&numbers, 42);
int *val = list_get(&numbers, 0);  // Returns pointer
list_deinit(&numbers);

// Iteration
list_iter_t iter = list_iter(&numbers);
int *item;
while ((item = list_next(&numbers, &iter))) {
    // Use *item
}
```

### Maps (Hash Tables)
```c
map_t(int) ages;
map_init(&ages);
map_set(&ages, "alice", 30);
int *age = map_get(&ages, "alice");  // Returns pointer or NULL
map_deinit(&ages);

// Iteration
map_iter_t iter = map_iter(&ages);
const char *key;
int *value;
while (map_next(&ages, &iter, &key, &value)) {
    // Use key and *value
}
```

### Buffers (Dynamic Byte Arrays)
```c
ak_buffer_t buf;
ak_buffer_init(&buf);
ak_buffer_append_cstr(&buf, "Hello");
ak_buffer_append_byte(&buf, '\n');
// buf.data is uint8_t*, buf.size is length
ak_buffer_deinit(&buf);
```

---

## Thread Management

### Thread Creation
```c
typedef void* (*thread_func_t)(void*);

AK24_THREAD thread;
AK24_THREAD_CREATE(&thread, NULL, my_func, arg);
AK24_THREAD_JOIN(thread, NULL);
```

### Thread Pool (Recommended)
```c
ak_thread_pool_config_t config = ak_thread_pool_config_default();
config.max_workers = 4;
ak_thread_pool_t *pool = ak_thread_pool_new(&config);

// Create task with lambda
void worker(void *ctx, void *args) {
    int *num = (int*)ctx;
    printf("Processing %d\n", *num);
}

void cleanup(void *ctx) {
    AK24_FREE(ctx);
}

int *data = AK24_ALLOC(sizeof(int));
*data = 42;
ak_lambda_t *task = ak_lambda_new(worker, data, cleanup);
ak_thread_pool_enqueue(pool, task, NULL);

ak_thread_pool_wait(pool);  // Wait for all tasks
ak_thread_pool_free(pool);
```

### Synchronization
```c
AK24_MUTEX mutex;
AK24_MUTEX_INIT(&mutex);
AK24_MUTEX_LOCK(&mutex);
// Critical section
AK24_MUTEX_UNLOCK(&mutex);
AK24_MUTEX_DESTROY(&mutex);
```

---

## Lambda Pattern (Closures)

Lambdas capture context and manage cleanup:

```c
void task_func(void *ctx, void *args) {
    my_data_t *data = (my_data_t*)ctx;
    // Use data
}

void task_cleanup(void *ctx) {
    AK24_FREE(ctx);
}

my_data_t *data = AK24_ALLOC(sizeof(my_data_t));
ak_lambda_t *lambda = ak_lambda_new(task_func, data, task_cleanup);
// Pass lambda to thread pool or other consumers
ak_lambda_free(lambda);  // Calls cleanup automatically
```

---

## Logging

```c
ak_log_set_level(AK24_LOG_LEVEL_DEBUG);
ak_log_set_color(true);

AK24_LOG_TRACE("Very verbose: %d", value);
AK24_LOG_DEBUG("Debug info: %s", str);
AK24_LOG_INFO("Normal operation");
AK24_LOG_WARN("Warning condition");
AK24_LOG_ERROR("Error occurred: %s", error);
```

---

## Context System (Hierarchical Scopes)

Use for symbol tables, variable scopes, configuration:

```c
ak_context_t *ctx = ak_context_new();
ak_context_push(ctx);  // New scope

ak_context_set_int(ctx, "x", 42);
ak_context_set_str(ctx, "name", "alice");

int x = ak_context_get_int(ctx, "x", -1);  // -1 is default

ak_context_pop(ctx, false);  // false = don't hoist to parent
ak_context_free(ctx);
```

---

## Utility Modules

### UTF-8 Text Handling
UTF-8 aware string operations for multi-byte characters:

```c
const char *text = "Hello 世界 🎉";
size_t char_count = ak_utf8_char_count((uint8_t*)text, strlen(text));
size_t byte_len = ak_utf8_char_size((uint8_t*)text);  // Bytes in first char

if (ak_utf8_validate((uint8_t*)text, strlen(text))) {
    // Text is valid UTF-8
}

// Iterate codepoints
const uint8_t *ptr = (uint8_t*)text;
while (*ptr) {
    uint32_t codepoint = ak_utf8_decode(ptr, &ptr);  // Advances ptr
    // Use codepoint
}
```

### String Interning
O(1) string equality via pointer comparison:

```c
const char *name1 = ak_intern("variable");
const char *name2 = ak_intern("variable");
assert(name1 == name2);  // Same pointer!

// Great for symbol tables and identifiers
map_t(symbol_t) symbols;
map_init(&symbols);
map_set(&symbols, ak_intern("x"), my_symbol);
```

### File Paths
Cross-platform path manipulation:

```c
// Join paths (uses correct separator for platform)
ak_buffer_t *path = ak_filepath_join(3, "/home", "user", "file.txt");
// Result: "/home/user/file.txt" on POSIX, "\\home\\user\\file.txt" on Windows

// Get system directories
ak_buffer_t *home = ak_filepath_home();     // User home dir
ak_buffer_t *cache = ak_filepath_cache();   // Cache dir
ak_buffer_t *config = ak_filepath_config(); // Config dir

// Extract components
const char *base = ak_filepath_basename("/path/to/file.txt");  // "file.txt"
const char *ext = ak_filepath_extension("file.txt");           // ".txt"

ak_buffer_free(path);
```

### Source Location Tracking
For compilers/interpreters - track code positions:

```c
// Load source file
ak_source_file_t *file = ak_source_file_from_path("input.c");

// Create location at byte offset
ak_source_loc_t loc = ak_source_loc_from_offset(file, 42);
printf("Line %zu, column %zu\n", loc.line, loc.column);

// Create range
ak_source_range_t range = ak_source_range_new(file, 10, 50);

// Extract source text from range
const char *text = ak_source_range_text(&range);
printf("Source: %s\n", text);

ak_source_file_release(file);
```

---

## Advanced: Atoms (Lock-Free N-D Structures)

Thread-safe atomic nodes that bond to form grids/cubes/lattices:

```c
// 3D cube example
ak_atom_t *atom = ak_atom_new(AK24_ATOM_I32);
ak_atom_set_i32(atom, 42);
int val = ak_atom_get_value(atom).i32;

// Bond atoms
ak_atom_bond(atom1, AK24_BOND_X_POS, atom2);
ak_atom_bond(atom1, AK24_BOND_Y_POS, atom3);
ak_atom_bond(atom1, AK24_BOND_Z_POS, atom4);

// Navigate
ak_atom_t *neighbor = ak_atom_get_bond(atom1, AK24_BOND_X_POS);
ak_atom_free(atom);
```

---

## Build Configuration

The kernel adapts to build flags:

- **AK24_GC_ENABLED=ON** (default): Uses Boehm GC, `AK24_FREE` is no-op
- **AK24_GC_ENABLED=OFF**: Uses malloc/free, must manually free
- **AK24_BUILD_DEBUG_MEMORY=ON**: Tracks all allocations, prints stats on shutdown

---

## Key Include Patterns

```c
// For standalone kernel apps
#include "kernel.h"

// For application framework (includes kernel.h)
#include "kernel/application.h"

// For specific modules (rarely needed, kernel.h includes all)
#include "kernel/list.h"
#include "kernel/map.h"
#include "kernel/lambda.h"
#include "kernel/threads.h"
```

---

## Example Demos Reference

| Demo | Purpose | Key Features |
|------|---------|--------------|
| [ak24-cli](examples/ak24-cli/main.c) | Basic app structure | Args, logging, app framework |
| [arena-demo](examples/arena-demo/main.c) | Arena allocation | AST construction, bulk alloc patterns |
| [atom-cube-demo](examples/atom-cube-demo/main.c) | 3D atomic structures | Atoms, bonding, concurrent access |
| [thread-pool-demo](examples/thread-pool-demo/main.c) | Concurrent tasks | Thread pool, lambdas, task callbacks |

---

## Common Mistakes to Avoid

1. **Using malloc/free directly** → Use `AK24_ALLOC`/`AK24_FREE`
2. **Freeing arena allocations individually** → Only free entire arena
3. **Not calling `list_deinit`/`map_deinit`** → Memory leak
4. **Using `int main()`** → Use `APP_MAIN` macro
5. **Manual kernel init** → Framework does it automatically
6. **Accessing list items by value** → Use pointers (`list_get` returns `T*`)
7. **Forgetting NULL checks** → `map_get` and `list_get` can return NULL

---

## Where to Find Documentation

- **API Reference**: Generate with `make docs` → `docs/api/html/index.html`
- **Module Docs**: `kernel/*/docs/*.md` (e.g., `kernel/arbuff/docs/arbuff.md`)
- **Quick Start**: [README.md](README.md)
- **Kernel Details**: [docs/kernel.md](docs/kernel.md)
- **Main Overview**: [docs/MAIN.md](docs/MAIN.md)

---

## Typical Application Flow

```c
#include "kernel/application.h"

APP_MAIN(my_app) {
    // 1. Kernel already initialized
    // 2. Args available in ctx->args

    // 3. Setup data structures
    list_t(task_t) tasks;
    list_init(&tasks);

    // 4. Use arena for temporary work
    ak_arena_t *arena = ak_arena_new_default();

    // 5. Process work
    ak_thread_pool_t *pool = ak_thread_pool_new(&ak_thread_pool_config_default());
    // ... enqueue tasks ...
    ak_thread_pool_wait(pool);

    // 6. Cleanup
    ak_thread_pool_free(pool);
    ak_arena_free(arena);
    list_deinit(&tasks);

    return 0;
}

APP_ON_SHUTDOWN(cleanup) {
    AK24_LOG_INFO("Uptime: %ld sec", time(NULL) - ctx->shutdown_info->start_time);
}

AK24_APPLICATION("com.example.myapp", my_app, cleanup)
```

---

**Remember**: AK24 is a memory-managed C framework. Always use kernel allocators, always clean up containers, always use the application framework. When in doubt, check the demos.
