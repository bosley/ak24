# Arena Allocator

The arena allocator provides region-based memory management optimized for bulk allocation and deallocation. It's designed for compiler phases and other scenarios where many temporary objects are created and can be freed all at once.

## Overview

An arena allocator:
- Allocates memory in large blocks (default 64KB)
- Provides O(1) allocation via bump pointer
- Supports bulk deallocation (reset or free)
- Eliminates individual free overhead
- Reduces memory fragmentation
- Can work with or without GC

## Use Cases

**Ideal for:**
- Compiler AST node allocation
- Parser temporary structures
- Type checking working memory
- IR generation intermediate nodes
- Per-compilation-unit memory pools
- Request-scoped allocations in servers
- Temporary data in batch processing

**Not ideal for:**
- Long-lived objects with different lifetimes
- Memory that needs individual deallocation
- Shared data between arena boundaries

## Basic Usage

```c
#include "kernel.h"

// Create arena with 64KB blocks
ak_arena_t *arena = ak_arena_new_default();

// Allocate objects
MyNode *node1 = ak_arena_alloc(arena, sizeof(MyNode));
MyNode *node2 = ak_arena_alloc(arena, sizeof(MyNode));
MyNode *node3 = ak_arena_alloc(arena, sizeof(MyNode));

// Use the objects...

// Free everything at once
ak_arena_free(arena);
```

## Custom Block Size

```c
// Small arena for few allocations
ak_arena_t *small = ak_arena_new(4096);  // 4KB blocks

// Large arena for many allocations
ak_arena_t *large = ak_arena_new(1024 * 1024);  // 1MB blocks

// Choose block size based on:
// - Expected total allocation size
// - Number of allocations
// - Memory constraints
```

## Reset and Reuse

For repeated operations (e.g., parsing multiple files):

```c
ak_arena_t *arena = ak_arena_new_default();

for (int i = 0; i < num_files; i++) {
    // Parse file into arena
    AST *ast = parse_file(arena, files[i]);

    // Process AST...
    process_ast(ast);

    // Free all allocations but keep blocks
    ak_arena_reset(arena);
}

ak_arena_free(arena);
```

## Snapshot and Restore

For nested scopes and temporary allocations:

```c
ak_arena_t *arena = ak_arena_new_default();

// Outer scope allocations
FunctionDef *func = ak_arena_alloc(arena, sizeof(FunctionDef));

// Save position
ak_arena_mark_t mark = ak_arena_snapshot(arena);

// Temporary allocations
TypeInfo *temp_type = ak_arena_alloc(arena, sizeof(TypeInfo));
ExprNode *temp_expr = ak_arena_alloc(arena, sizeof(ExprNode));

// ... use temporary data ...

// Free temporary allocations only
ak_arena_restore(arena, mark);

// func is still valid, temp_type and temp_expr are freed

ak_arena_free(arena);
```

## Nested Snapshots

Snapshots can be nested for multi-level scopes:

```c
ak_arena_t *arena = ak_arena_new_default();

void *outer = ak_arena_alloc(arena, 100);
ak_arena_mark_t mark1 = ak_arena_snapshot(arena);

void *middle = ak_arena_alloc(arena, 200);
ak_arena_mark_t mark2 = ak_arena_snapshot(arena);

void *inner = ak_arena_alloc(arena, 300);

// Restore inner scope
ak_arena_restore(arena, mark2);  // inner freed

// Restore middle scope
ak_arena_restore(arena, mark1);  // middle freed

// outer still valid

ak_arena_free(arena);
```

## String Duplication

Convenient helpers for string allocation:

```c
ak_arena_t *arena = ak_arena_new_default();

// Duplicate full string
const char *name = ak_arena_strdup(arena, "variable_name");

// Duplicate substring
const char *token_text = "identifier+123";
const char *identifier = ak_arena_strndup(arena, token_text, 10);
// identifier = "identifier"

ak_arena_free(arena);
```

## Alignment

Default alignment is 8 bytes. For specific requirements:

```c
ak_arena_t *arena = ak_arena_new_default();

// 16-byte alignment for SIMD
float *vec = ak_arena_alloc_aligned(arena, 16 * sizeof(float), 16);

// 32-byte alignment for cache lines
struct CacheAligned *obj =
    ak_arena_alloc_aligned(arena, sizeof(struct CacheAligned), 32);

ak_arena_free(arena);
```

## Zero-Initialized Memory

```c
ak_arena_t *arena = ak_arena_new_default();

// Allocate and zero
int *array = ak_arena_calloc(arena, 100, sizeof(int));
// All elements are 0

ak_arena_free(arena);
```

## Reallocation

```c
ak_arena_t *arena = ak_arena_new_default();

char *buf = ak_arena_alloc(arena, 100);
// ... use buffer ...

// Grow buffer (efficient if it's the last allocation)
buf = ak_arena_realloc(arena, buf, 100, 200);

ak_arena_free(arena);
```

**Note:** Realloc is most efficient when growing the most recent allocation. For other allocations, it copies to a new location.

## Memory Statistics

Track memory usage for diagnostics:

```c
ak_arena_t *arena = ak_arena_new_default();

// Make some allocations
for (int i = 0; i < 100; i++) {
    ak_arena_alloc(arena, 64);
}

size_t allocated, used;
ak_arena_stats(arena, &allocated, &used);

printf("Arena: %zu bytes used of %zu bytes allocated\n", used, allocated);
printf("Efficiency: %.1f%%\n", (double)used / allocated * 100);

ak_arena_free(arena);
```

## Compiler Example

Complete example for a compiler phase:

```c
typedef struct {
    ak_arena_t *arena;
    // ... other parser state ...
} Parser;

Parser *parser_new(void) {
    Parser *p = malloc(sizeof(Parser));
    p->arena = ak_arena_new(1024 * 1024);  // 1MB blocks for AST
    return p;
}

ASTNode *parse_expression(Parser *p) {
    // Save position for error recovery
    ak_arena_mark_t mark = ak_arena_snapshot(p->arena);

    ASTNode *node = ak_arena_alloc(p->arena, sizeof(ASTNode));

    // Parse...
    if (error) {
        // Rollback allocations on error
        ak_arena_restore(p->arena, mark);
        return NULL;
    }

    return node;
}

void parser_free(Parser *p) {
    // Frees entire AST at once
    ak_arena_free(p->arena);
    free(p);
}

// Usage
Parser *p = parser_new();
ASTNode *ast = parse_file(p, source);
process_ast(ast);
parser_free(p);  // All AST nodes freed in one operation
```

## Performance Characteristics

| Operation | Time Complexity | Notes |
|-----------|----------------|-------|
| `ak_arena_alloc` | O(1) | Bump pointer allocation |
| `ak_arena_alloc_aligned` | O(1) | May waste up to (align-1) bytes |
| `ak_arena_calloc` | O(n) | Zero initialization |
| `ak_arena_realloc` | O(1) or O(n) | O(1) if last allocation and space available |
| `ak_arena_reset` | O(blocks) | Fast, reuses memory |
| `ak_arena_free` | O(blocks) | Frees all blocks |
| `ak_arena_snapshot` | O(1) | Just saves position |
| `ak_arena_restore` | O(1) | Resets position |

## Memory Layout

```
Arena:
  block_size: 65536

  Block 1: [████████████████░░░░░░░░] (used: 45000, size: 65536)
             ^                ^
             |                |
           data          current position

  Block 2: [████░░░░░░░░░░░░░░░░░░░░] (used: 12000, size: 65536)
             ^   ^
             |   |
           data  current position (current_block)
```

## Thread Safety

**NOT thread-safe.** Each thread should have its own arena:

```c
// Thread-local storage
_Thread_local ak_arena_t *thread_arena = NULL;

void *thread_worker(void *arg) {
    thread_arena = ak_arena_new_default();

    // Use thread_arena for allocations...

    ak_arena_free(thread_arena);
    return NULL;
}
```

## Best Practices

1. **Choose appropriate block size**: Larger blocks = fewer allocations, more waste if unused
2. **Use reset for repeated operations**: Faster than free + new
3. **Use snapshots for error recovery**: Easy rollback of partial work
4. **Profile memory usage**: Use stats to tune block size
5. **Don't mix lifetimes**: Objects with different lifetimes should use different arenas
6. **Consider GC interaction**: Arena works with GC but provides deterministic cleanup

## API Reference

See [arena.h](../include/arena.h) for complete API documentation.

### Core Functions

- `ak_arena_new(size_t block_size)` - Create arena
- `ak_arena_new_default()` - Create with default block size
- `ak_arena_free(ak_arena_t *arena)` - Free arena
- `ak_arena_alloc(ak_arena_t *arena, size_t size)` - Allocate
- `ak_arena_reset(ak_arena_t *arena)` - Reset for reuse

### Advanced Functions

- `ak_arena_alloc_aligned(arena, size, align)` - Aligned allocation
- `ak_arena_calloc(arena, nmemb, size)` - Zero-initialized allocation
- `ak_arena_realloc(arena, ptr, old_size, new_size)` - Reallocate
- `ak_arena_snapshot(arena)` - Save position
- `ak_arena_restore(arena, mark)` - Restore position
- `ak_arena_stats(arena, allocated, used)` - Get statistics

### Helper Functions

- `ak_arena_strdup(arena, str)` - Duplicate string
- `ak_arena_strndup(arena, str, n)` - Duplicate substring

## Related Modules

- **intern**: String interning for identifiers (complements arena for compiler work)
- **buffer**: Dynamic byte buffers (different use case - individual buffer growth)
- **list**: Dynamic lists (different memory model - individual element lifetime)
