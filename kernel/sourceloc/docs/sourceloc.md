# Source Location Tracking Module

## Overview

The source location tracking module provides comprehensive source code position tracking with line/column numbers for error messages, diagnostics, debugging information, and tooling support. This is essential infrastructure for compiler development where every AST node, token, and diagnostic needs associated source location information.

## Key Features

- **Precise Position Tracking**: Track source positions with line, column, and byte offset
- **UTF-8 Aware**: Column numbers count UTF-8 code points, not bytes
- **Fast Lookups**: Cached line start positions enable O(log n) offset-to-line/column conversion
- **Source Ranges**: Track spans of code for highlighting and diagnostics
- **Text Extraction**: Extract source text from ranges for error context
- **Reference Counting**: Automatic lifetime management for source files
- **String Interning**: Filenames are interned for memory efficiency
- **Thread-Safe**: Safe concurrent access for read operations after file load
- **GC Compatible**: Works with or without Boehm GC enabled

## Use Cases

- Compiler error messages with line/column information
- Syntax highlighting in diagnostics
- IDE features: jump-to-definition, find references
- Stack traces in compiled language
- Debug information generation (DWARF, PDB)
- Source code transformation tracking
- Macro expansion tracking

## API Reference

### System Initialization

```c
void ak_sourceloc_init(void);
void ak_sourceloc_shutdown(void);
```

Initialize and shutdown the source location system. Must be called before/after all other sourceloc functions. Typically called from `ak_kernel_init()` and `ak_kernel_deinit()`.

### Source File Management

```c
ak_source_file_t *ak_source_file_new(const char *filename,
                                     const char *contents,
                                     size_t len);
ak_source_file_t *ak_source_file_from_path(const char *path);
```

Create source file objects from memory or disk. Files are reference counted and filenames are automatically interned. Line start positions are computed and cached for fast lookups.

**From memory:**
```c
const char *code = "int main() {\n  return 0;\n}\n";
ak_source_file_t *file = ak_source_file_new("main.c", code, strlen(code));
```

**From disk:**
```c
ak_source_file_t *file = ak_source_file_from_path("src/parser.c");
if (!file) {
    fprintf(stderr, "Failed to load file\n");
}
```

### Reference Counting

```c
ak_source_file_t *ak_source_file_retain(ak_source_file_t *file);
void ak_source_file_release(ak_source_file_t *file);
```

Manage file lifetime through reference counting. Always call `release()` when done with a file. When reference count reaches zero, the file is automatically freed.

```c
ak_source_file_t *file = ak_source_file_from_path("test.c");

// Share the file
ak_source_file_t *copy = ak_source_file_retain(file);

// Both holders release
ak_source_file_release(file);
ak_source_file_release(copy);  // Now freed
```

### File Accessors

```c
const char *ak_source_file_contents(ak_source_file_t *file);
size_t ak_source_file_length(ak_source_file_t *file);
const char *ak_source_file_name(ak_source_file_t *file);
```

Access file properties. The filename is an interned string (permanent pointer).

### Creating Locations

```c
ak_source_loc_t ak_source_loc_new(ak_source_file_t *file,
                                  uint32_t line,
                                  uint32_t column,
                                  uint32_t offset);
ak_source_loc_t ak_source_loc_from_offset(ak_source_file_t *file,
                                          uint32_t offset);
```

Create source locations with explicit values or compute line/column from byte offset.

**Explicit location:**
```c
ak_source_loc_t loc = ak_source_loc_new(file, 10, 5, 142);
```

**Computed from offset (recommended):**
```c
ak_source_loc_t loc = ak_source_loc_from_offset(file, 150);
printf("Position: line %u, column %u\n", loc.line, loc.column);
```

### Source Ranges

```c
ak_source_range_t ak_source_range_new(ak_source_loc_t start,
                                      ak_source_loc_t end);
bool ak_source_range_contains(ak_source_range_t *range,
                              ak_source_loc_t loc);
bool ak_source_range_overlaps(ak_source_range_t *a,
                              ak_source_range_t *b);
char *ak_source_extract(ak_source_range_t range);
```

Work with ranges of source code. Ranges are half-open intervals [start, end).

**Creating ranges:**
```c
ak_source_loc_t start = ak_source_loc_from_offset(file, 10);
ak_source_loc_t end = ak_source_loc_from_offset(file, 25);
ak_source_range_t range = ak_source_range_new(start, end);
```

**Testing containment:**
```c
ak_source_loc_t loc = ak_source_loc_from_offset(file, 15);
if (ak_source_range_contains(&range, loc)) {
    printf("Location is in range\n");
}
```

**Extracting text:**
```c
char *text = ak_source_extract(range);
if (text) {
    printf("Range contains: %s\n", text);
    AK24_FREE(text);
}
```

### Line Access

```c
const char *ak_source_get_line(ak_source_file_t *file, uint32_t line);
size_t ak_source_get_line_len(ak_source_file_t *file, uint32_t line);
```

Get direct access to line text for display in diagnostics. The returned pointer is into the file's content buffer (not a copy) and is NOT null-terminated.

```c
const char *line_text = ak_source_get_line(file, 10);
size_t line_len = ak_source_get_line_len(file, 10);

if (line_text) {
    printf("%4d | %.*s\n", 10, (int)line_len, line_text);
}
```

### Formatting

```c
int ak_source_loc_format(ak_source_loc_t loc, char *buf, size_t size);
int ak_source_range_format(ak_source_range_t range, char *buf, size_t size);
```

Format locations and ranges as strings for error messages. Uses standard "file:line:col" format.

**Location formatting:**
```c
char buf[256];
ak_source_loc_format(loc, buf, sizeof(buf));
printf("Error at %s: undefined variable\n", buf);
// Output: "Error at src/main.c:42:15: undefined variable"
```

**Range formatting:**
```c
char buf[256];
ak_source_range_format(range, buf, sizeof(buf));
printf("Warning at %s: unused variable\n", buf);
// Output: "Warning at src/main.c:10:5-10:15: unused variable"
```

### Statistics

```c
void ak_sourceloc_stats(size_t *file_count, size_t *total_bytes);
```

Get statistics about currently loaded source files.

```c
size_t files, bytes;
ak_sourceloc_stats(&files, &bytes);
printf("Loaded %zu files, %zu bytes\n", files, bytes);
```

## Complete Example: Compiler Error

Here's a complete example showing how to use source locations in a compiler for error reporting:

```c
#include "kernel.h"

void report_error(ak_source_range_t range, const char *message) {
    char location_buf[256];
    ak_source_range_format(range, location_buf, sizeof(location_buf));

    // Extract the problematic code
    char *code = ak_source_extract(range);

    // Get the line for context
    const char *line = ak_source_get_line(range.start.file, range.start.line);
    size_t line_len = ak_source_get_line_len(range.start.file, range.start.line);

    fprintf(stderr, "Error at %s: %s\n", location_buf, message);
    fprintf(stderr, "%4d | %.*s\n", range.start.line, (int)line_len, line);
    fprintf(stderr, "     | ");

    // Print arrows under the error
    for (uint32_t i = 1; i < range.start.column; i++) {
        fprintf(stderr, " ");
    }
    uint32_t error_len = range.end.column - range.start.column;
    for (uint32_t i = 0; i < error_len; i++) {
        fprintf(stderr, "^");
    }
    fprintf(stderr, "\n");

    if (code) {
        AK24_FREE(code);
    }
}

int main(void) {
    ak_kernel_init();

    // Load source file
    ak_source_file_t *file = ak_source_file_from_path("test.c");
    if (!file) {
        fprintf(stderr, "Failed to load file\n");
        ak_kernel_deinit();
        return 1;
    }

    // Parse and find an error at offset 100-110
    ak_source_loc_t start = ak_source_loc_from_offset(file, 100);
    ak_source_loc_t end = ak_source_loc_from_offset(file, 110);
    ak_source_range_t error_range = ak_source_range_new(start, end);

    report_error(error_range, "undefined variable 'foo'");

    ak_source_file_release(file);
    ak_kernel_deinit();
    return 0;
}
```

Output:
```
Error at test.c:5:8-5:11: undefined variable 'foo'
   5 |     int foo = bar;
     |         ^^^
```

## UTF-8 Support

The module correctly handles UTF-8 encoded source files. Column numbers count UTF-8 code points (characters), not bytes:

```c
// Source: "Hello 世界\n"
//         H=col1, e=col2, l=col3, l=col4, o=col5, space=col6, 世=col7, 界=col8

ak_source_file_t *file = ak_source_file_new("test.c", "Hello 世界\n", 14);

// Byte offset 6 is the start of '世' (3 bytes)
ak_source_loc_t loc = ak_source_loc_from_offset(file, 6);
assert(loc.column == 7);  // Column 7, not byte 6
```

This ensures that error messages and diagnostics align correctly with the visual display of the code.

## Performance Characteristics

- **File Creation**: O(n) where n is file size (must scan for line starts)
- **Offset to Line/Column**: O(log L) where L is line count (binary search)
- **Line Access**: O(1) using cached line starts
- **Text Extraction**: O(n) where n is range length
- **Memory Overhead**: ~4 bytes per line for cached line starts

For typical source files (< 10,000 lines), performance is excellent:
- 10K offset lookups < 10ms
- Line start cache for 10K lines: ~40KB

## Design Considerations

### Why Reference Counting?

Source files may be referenced by many tokens, AST nodes, and diagnostics throughout compilation. Reference counting provides automatic lifetime management without requiring explicit ownership tracking.

### Why Intern Filenames?

In a compiler processing many files, the same filename appears in thousands of locations (error messages, AST nodes, debug info). Interning saves memory and enables O(1) string equality checks.

### Why Cache Line Starts?

Converting byte offsets to line/column numbers is a frequent operation during error reporting and IDE features. Caching line starts provides O(log n) lookups instead of O(n) scanning.

### Thread Safety

- Source file creation and release: thread-safe (internal locking)
- Read operations after creation: thread-safe (immutable data)
- Statistics access: thread-safe (internal locking)

## Integration with AK24 Kernel

The sourceloc module integrates seamlessly with other kernel features:

- **String Interning**: Filenames are interned using `ak_intern()`
- **Memory Management**: Uses `AK24_ALLOC`/`AK24_FREE` for GC compatibility
- **Threading**: Uses `AK24_MUTEX` for platform-agnostic locking
- **Kernel Lifecycle**: Initialized/shutdown via `ak_kernel_init()`/`ak_kernel_deinit()`

## Testing

The module includes comprehensive tests covering:
- Basic file creation and reference counting
- Offset-to-line/column conversion
- UTF-8 column counting
- Range operations (contains, overlaps, extract)
- Line access and formatting
- Edge cases (empty files, single line, CRLF line endings)
- Statistics tracking

Run tests:
```bash
make test
./build/bin/ak24_tests
```

## Future Enhancements

Potential future additions:
- Macro expansion tracking (virtual locations)
- Source file include tree tracking
- Incremental line start computation for streaming parsers
- Memory-mapped file support for large files
- Source file search index for LSP features
