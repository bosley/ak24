# String Interning Module

## Overview

The string interning module provides a global string table that ensures only one copy of each unique string exists in memory. This enables O(1) pointer-based equality checks instead of O(n) strcmp operations, making it ideal for identifiers, symbol table keys, type names, and string literals.

## Key Features

- **Memory Optimization**: Only one copy of each unique string in memory
- **Fast Equality**: O(1) pointer comparison instead of O(n) strcmp
- **Thread-Safe**: Internal locking for concurrent access
- **Statistics**: Track interned string count and memory usage
- **Stable Pointers**: Interned strings remain valid until shutdown
- **GC Compatible**: Works with or without Boehm GC enabled

## Use Cases

- Compiler identifiers and keywords
- Symbol table keys
- Type names
- String literals in DSLs
- Configuration keys
- Any scenario requiring frequent string equality checks

## API Reference

### Initialization

```c
void ak_intern_init(void);
void ak_intern_shutdown(void);
```

Initialize and shutdown the interning system. `ak_intern_init()` must be called before any other intern functions, typically from `ak_kernel_init()`. `ak_intern_shutdown()` frees all interned strings and should be called from `ak_kernel_deinit()`.

### Interning Strings

```c
const char *ak_intern(const char *str);
const char *ak_intern_n(const char *str, size_t len);
```

Intern a string and return its canonical pointer. If the string already exists, returns the existing pointer. Otherwise, creates a new copy and adds it to the table.

`ak_intern_n()` interns a substring, useful for tokenization:

```c
const char *source = "identifier+123";
const char *id = ak_intern_n(source, 10);  // Returns "identifier"
```

### Equality and Hashing

```c
bool ak_intern_eq(const char *a, const char *b);
uint64_t ak_intern_hash(const char *str);
```

Fast equality check via pointer comparison and pointer-based hashing. Only valid for interned strings.

```c
const char *key1 = ak_intern("name");
const char *key2 = ak_intern("name");

if (ak_intern_eq(key1, key2)) {
    // Always true - same pointer!
}

uint64_t hash = ak_intern_hash(key1);  // O(1) hash
```

### Statistics and Management

```c
void ak_intern_stats(size_t *count, size_t *bytes);
void ak_intern_clear(void);
```

Get statistics about interned strings or clear all strings while keeping the system initialized.

## Usage Examples

### Basic Usage

```c
#include "kernel.h"
#include "intern.h"

int main(void) {
    ak_kernel_init();

    const char *str1 = ak_intern("hello");
    const char *str2 = ak_intern("hello");

    assert(str1 == str2);  // Same pointer!
    assert(ak_intern_eq(str1, str2));

    ak_kernel_deinit();
    return 0;
}
```

### Symbol Table with Interned Keys

```c
typedef struct {
    const char *name;  // Interned string
    int value;
} symbol_t;

map_int_t symbol_table;
map_init_generic(&symbol_table, sizeof(char *),
                 map_hash_str, map_cmp_str);

// Insert with interned key
const char *key = ak_intern("variable");
map_insert(&symbol_table, &key, 42);

// Fast lookup - intern the search key
const char *search = ak_intern("variable");
int *value = map_get(&symbol_table, &search);
```

### Tokenization

```c
typedef struct {
    const char *text;  // Interned token text
    int line;
    int column;
} token_t;

token_t tokenize(const char *source, size_t start, size_t len) {
    token_t tok;
    tok.text = ak_intern_n(source + start, len);
    tok.line = current_line;
    tok.column = current_column;
    return tok;
}
```

### Monitoring Memory Usage

```c
size_t count, bytes;
ak_intern_stats(&count, &bytes);
printf("Interned: %zu strings, %zu bytes\n", count, bytes);
```

## Implementation Details

### Hash Table

The intern table uses a hash table with chaining for collision resolution. The initial size is 1024 buckets, and strings are hashed using the FNV-1a algorithm.

### Thread Safety

All public functions are thread-safe via an internal mutex. Multiple threads can safely call `ak_intern()` concurrently.

### Memory Management

- Uses `AK24_ALLOC` for metadata structures
- Uses `AK24_ALLOC_ATOMIC` for string data (no pointers)
- Works with or without GC enabled
- Strings persist until `ak_intern_shutdown()` or `ak_intern_clear()`

### Performance Characteristics

- **Intern**: O(1) average case, O(n) worst case (hash collision)
- **Equality**: O(1) pointer comparison
- **Hash**: O(1) pointer hash
- **Memory**: One copy per unique string + hash table overhead

## Testing

Run the test suite:

```bash
make test
# or
./build/test/compile_time/ak24_intern_runtime_tests
```

Test coverage includes:
- Basic interning and deduplication
- Substring interning
- Equality and hashing
- Statistics tracking
- NULL handling
- Empty strings
- Special characters
- Stress test with 10,000 strings
- Thread safety (if AK24_THREAD_TESTS enabled)

## Integration

The intern module is automatically included when building the kernel. Just include the header:

```c
#include "intern.h"
```

No additional linking required - it's part of `libak24_kernel.a`.

## Best Practices

1. **Initialize Early**: Call `ak_intern_init()` at program start (done by `ak_kernel_init()`)
2. **Intern Once**: Cache interned pointers, don't re-intern repeatedly
3. **Use for Symbols**: Best for identifiers, keywords, and small strings
4. **Avoid Large Strings**: Not designed for large text or user content
5. **Pointer Identity**: Only use `ak_intern_eq()` with interned strings
6. **Monitor Usage**: Use `ak_intern_stats()` to track memory
