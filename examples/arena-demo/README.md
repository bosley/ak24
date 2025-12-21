# Arena Allocator Demo

This example demonstrates the arena allocator's key features and usage patterns.

## Building and Running

```bash
make
./build/bin/arena-demo
```

## Features Demonstrated

### 1. Basic Usage
- Creating an arena with default block size
- Allocating zero-initialized arrays with `ak_arena_calloc`
- Getting memory statistics
- Bulk deallocation with `ak_arena_free`

### 2. AST Construction
- Building an expression tree using arena allocation
- Demonstrates O(1) node allocation
- Shows bulk cleanup of entire tree structure
- Practical compiler use case

### 3. Reset and Reuse
- Processing multiple "files" with the same arena
- Using `ak_arena_reset` to clear allocations between iterations
- Reusing memory blocks without reallocation
- Efficient for batch processing

### 4. Snapshot and Restore
- Saving arena state with `ak_arena_snapshot`
- Making temporary allocations
- Rolling back to saved state with `ak_arena_restore`
- Useful for nested scopes and error recovery

### 5. String Operations
- Duplicating strings with `ak_arena_strdup`
- Extracting substrings with `ak_arena_strndup`
- Building paths and concatenating strings
- Common text processing patterns

## Key Takeaways

- **O(1) allocation**: Fast bump-pointer allocation
- **Bulk deallocation**: Free entire arena at once
- **Memory reuse**: Reset arena without freeing blocks
- **Nested scopes**: Snapshot/restore for temporary allocations
- **GC compatible**: Works with or without garbage collection

## When to Use Arena Allocator

**Good for:**
- Compiler AST nodes
- Parser temporary structures
- Request-scoped allocations
- Batch processing temporary data
- Any scenario with bulk lifetime management

**Not ideal for:**
- Long-lived objects with different lifetimes
- Objects requiring individual deallocation
- Shared data across arena boundaries
