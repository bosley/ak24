# Source Location Tracking Demo

This example demonstrates the source location tracking module's key features and usage patterns for compiler development.

## Building and Running

```bash
make
./build/bin/sourceloc-demo
```

## Features Demonstrated

### 1. Basic Usage
- Creating source files from memory with `ak_source_file_new`
- Converting byte offsets to line/column numbers
- Formatting locations for display
- Essential operations for any compiler

### 2. Error Reporting
- Building rich error messages with source context
- Extracting source lines for display
- Highlighting error locations with visual markers
- Production-quality diagnostic output

### 3. UTF-8 Support
- Handling UTF-8 encoded source files
- Column counting with multi-byte characters
- Demonstrates that columns count code points, not bytes
- Essential for international character support

### 4. Range Operations
- Creating source ranges for code spans
- Extracting text from ranges
- Testing containment and overlap
- Used for highlighting and code transformations

### 5. File Loading
- Loading source files from disk with `ak_source_file_from_path`
- Accessing individual lines by number
- Real-world file I/O patterns

### 6. Reference Counting
- Sharing source files across multiple owners
- Automatic memory management via retain/release
- Prevents premature deallocation
- Safe for multi-threaded compilation

### 7. Statistics Tracking
- Monitoring loaded files and memory usage
- Useful for compiler diagnostics
- Resource management insights

### 8. Compiler Pipeline
- Simulating tokenization with location tracking
- Attaching source positions to tokens
- Error reporting in semantic analysis
- End-to-end compiler workflow

## Key Takeaways

- **Precise Tracking**: Every source position has line, column, and byte offset
- **UTF-8 Aware**: Correctly handles multi-byte characters
- **Fast Lookups**: O(log n) offset-to-line conversion via binary search
- **Rich Diagnostics**: Extract context and format error messages
- **Reference Counted**: Automatic lifetime management
- **String Interned**: Filenames are deduplicated for efficiency

## When to Use Source Location Tracking

**Essential for:**
- Compiler error messages
- Syntax highlighting
- IDE features (go-to-definition, find-references)
- Debug information generation
- Source code transformation tools
- Code analysis and linting

**Attach to:**
- Tokens during lexical analysis
- AST nodes during parsing
- Type information during semantic analysis
- IR instructions during code generation
- Error and warning diagnostics

## Example Error Output

```
Error at test.c:2:9: undefined variable 'z'
   2 | int y = z + 5;
     |         ^
```

This format is recognized by most IDEs and editors, enabling clickable error navigation.

## Integration with Other Modules

- **String Interning**: Filenames are automatically interned for memory efficiency
- **Arena Allocator**: Can allocate source files in arena for batch processing
- **Kernel**: Automatically initialized via `ak_kernel_init()`

## Performance Notes

- File loading: O(n) to scan for line starts (done once)
- Offset lookup: O(log L) where L is line count (binary search)
- Line access: O(1) using cached line starts
- Memory overhead: ~4 bytes per line for cache

For typical source files (< 10,000 lines), performance is excellent.
