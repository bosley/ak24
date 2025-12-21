# Buffer UTF-8 Handling

## Overview

The AK24 buffer module (`ak_buffer_t`) operates at the byte level and is **fully compatible** with UTF-8 encoded text. This document explains how buffers handle UTF-8 data and best practices for working with multi-byte character encodings.

## UTF-8 Compatibility

### Storage
Buffers store data as raw bytes (`uint8_t *`). Since UTF-8 is a byte-oriented encoding, buffers can store UTF-8 text without any special handling:

```c
ak_buffer_t *buf = ak_buffer_new(128);
const uint8_t *utf8_text = (uint8_t *)"Hello 世界 café 🎉";
ak_buffer_copy_to(buf, (uint8_t *)utf8_text, strlen((char *)utf8_text));
```

### Operations

All buffer operations work at the byte level. This means:

- **Safe operations**: `ak_buffer_copy_to`, `ak_buffer_data`, `ak_buffer_count`, `ak_buffer_clear`
- **Character-aware operations needed**: Operations that split or extract data at specific positions

## Important Considerations

### 1. Byte vs. Character Offsets

Buffers work with **byte offsets**, not character offsets. For UTF-8:
- ASCII characters: 1 byte per character
- Extended Latin (é, ñ, etc.): 2 bytes per character
- CJK characters (日本語): 3 bytes per character
- Emoji (🎉, 🎊): 4 bytes per character

**Example:**
```c
// "café" is 5 bytes but 4 characters
// c=1 byte, a=1 byte, f=1 byte, é=2 bytes
const uint8_t *text = (uint8_t *)"café";
size_t byte_length = 5;    // What buffer tracks
size_t char_count = 4;     // Logical character count
```

### 2. Character Boundary Safety

When extracting sub-buffers or splitting at specific positions, ensure you don't split multi-byte UTF-8 characters:

```c
// UNSAFE: May split UTF-8 character
ak_buffer_t *sub = ak_buffer_sub_buffer(buf, 0, 3, NULL);

// SAFE: Use UTF-8 utilities to find character boundaries
#include "utf8.h"
size_t pos = 0;
for (int i = 0; i < 3; i++) {
    size_t char_len = ak_utf8_char_len(buf->data + pos, buf->count - pos);
    pos += char_len;
}
ak_buffer_t *sub = ak_buffer_sub_buffer(buf, 0, pos, NULL);
```

### 3. Buffer Iteration

The `ak_buffer_for_each` function iterates byte-by-byte. For UTF-8 text, consider using the UTF-8 module for character-aware iteration:

```c
#include "utf8.h"

// Iterate by bytes (may be mid-character)
ak_buffer_for_each(buf, byte_callback, data);

// Iterate by UTF-8 characters
size_t pos = 0;
while (pos < buf->count) {
    ak_utf8_decode_result_t result =
        ak_utf8_decode(buf->data + pos, buf->count - pos);

    if (!result.valid || result.bytes == 0) {
        break;
    }

    // Process result.codepoint

    pos += result.bytes;
}
```

## Best Practices

### 1. Use UTF-8 Module for Text Processing

When working with text data in buffers, use the `utf8.h` utilities:

```c
#include "utf8.h"
#include "buffer.h"

// Count characters, not bytes
size_t char_count = ak_utf8_char_count(buf->data, buf->count);

// Check if specific position is whitespace
bool is_ws = ak_utf8_is_whitespace_at(buf, pos);

// Find character boundaries
size_t char_len = ak_utf8_char_len(buf->data + pos, buf->count - pos);
```

### 2. Validate UTF-8 Input

When loading external data, validate it's proper UTF-8:

```c
#include "utf8.h"

ak_buffer_t *buf = ak_buffer_from_file("input.txt");
if (buf) {
    if (!ak_utf8_validate(buf->data, buf->count)) {
        fprintf(stderr, "Invalid UTF-8 in file\n");
        ak_buffer_free(buf);
        return -1;
    }
    // Safe to process as UTF-8
}
```

### 3. Binary vs. Text Buffers

Buffers can hold both binary and text data:

- **Binary data**: No UTF-8 considerations needed
- **Text data**: Use UTF-8 utilities for character operations

Make the distinction clear in your code:

```c
// Binary buffer - byte operations OK
ak_buffer_t *binary_buf = load_image_file("image.png");

// Text buffer - use UTF-8 aware operations
ak_buffer_t *text_buf = ak_buffer_from_file("source.txt");
```

## Integration with Scanner

The scanner module (`scanner.h`) is UTF-8 aware and works correctly with UTF-8 text in buffers:

```c
#include "scanner.h"
#include "buffer.h"

// Create buffer with UTF-8 content
ak_buffer_t *buf = ak_buffer_new(128);
ak_buffer_copy_to(buf, (uint8_t *)"hello café 123", 14);

// Scanner handles UTF-8 correctly
ak_scanner_t *scanner = ak_scanner_new(buf, 0);
ak_scanner_static_type_result_t result =
    ak_scanner_read_static_base_type(scanner, NULL);

// "hello" symbol parsed correctly
// Scanner automatically handles UTF-8 whitespace and boundaries
```

## Summary

- ✅ Buffers store UTF-8 safely as raw bytes
- ✅ All basic buffer operations (copy, clear, count) work correctly
- ⚠️ Use UTF-8 utilities for character-aware operations (splitting, positioning)
- ⚠️ Buffer offsets are in bytes, not characters
- ✅ Scanner module is UTF-8 aware and handles multi-byte characters correctly
- ✅ ASCII data works exactly as before (UTF-8 is ASCII-compatible)

## See Also

- `utf8.h` - UTF-8 character utilities
- `scanner.h` - UTF-8 aware text scanner
- Buffer API documentation in `buffer.h`
