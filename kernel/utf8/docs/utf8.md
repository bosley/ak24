# UTF-8

The UTF-8 module provides utilities for working with UTF-8 encoded text, including character boundary detection, codepoint decoding, character classification, and safe iteration over multi-byte sequences.

## Core Concept

UTF-8 is a variable-length character encoding where:
- ASCII characters (0x00-0x7F) use 1 byte
- Extended Latin and common symbols use 2 bytes
- Most other scripts (CJK, Arabic, etc.) use 3 bytes
- Emoji and rare characters use 4 bytes

This module handles UTF-8 sequences correctly while remaining ASCII-compatible. Invalid sequences are treated as single bytes.

## Character Structure

```
1-byte: 0xxxxxxx                    (ASCII: U+0000 to U+007F)
2-byte: 110xxxxx 10xxxxxx           (U+0080 to U+07FF)
3-byte: 1110xxxx 10xxxxxx 10xxxxxx  (U+0800 to U+FFFF)
4-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx (U+10000 to U+10FFFF)
```

## Basic Usage

```c
#include "kernel.h"

int main(int argc, char **argv) {
    ak_kernel_init("my-app");

    const uint8_t *text = (uint8_t *)"Hello 世界 café 🎉";
    size_t byte_len = strlen((char *)text);

    // Count characters (not bytes)
    size_t char_count = ak_utf8_char_count(text, byte_len);
    printf("Characters: %zu\n", char_count); // 15

    // Validate UTF-8
    if (ak_utf8_validate(text, byte_len)) {
        printf("Valid UTF-8\n");
    }

    ak_kernel_deinit();
    return 0;
}
```

## Character Length

```c
const uint8_t *text = (uint8_t *)"café";
size_t pos = 0;

// Get length of each character
size_t len1 = ak_utf8_char_len(text, 5);      // 'c' = 1 byte
size_t len2 = ak_utf8_char_len(text + 3, 2);  // 'é' = 2 bytes
```

## Decoding Codepoints

```c
const uint8_t *text = (uint8_t *)"日本語";
size_t pos = 0;
size_t byte_len = strlen((char *)text);

while (pos < byte_len) {
    ak_utf8_decode_result_t result = ak_utf8_decode(text + pos, byte_len - pos);

    if (result.valid) {
        printf("U+%04X (%zu bytes)\n", result.codepoint, result.bytes);
        pos += result.bytes;
    } else {
        printf("Invalid byte at position %zu\n", pos);
        pos++;
    }
}
```

## Character Classification

```c
ak_utf8_decode_result_t result = ak_utf8_decode(text, byte_len);

if (result.valid) {
    if (ak_utf8_is_whitespace(result.codepoint)) {
        printf("Whitespace\n");
    }
    if (ak_utf8_is_digit(result.codepoint)) {
        printf("Digit (any script)\n");
    }
    if (ak_utf8_is_alpha(result.codepoint)) {
        printf("Letter (any script)\n");
    }
    if (ak_utf8_is_alnum(result.codepoint)) {
        printf("Alphanumeric\n");
    }
}
```

## Unicode Digits

```c
// Recognizes digits from any script
ak_utf8_is_digit(0x30);    // '0' (ASCII) → true
ak_utf8_is_digit(0x0660);  // ٠ (Arabic-Indic) → true
ak_utf8_is_digit(0x0966);  // ० (Devanagari) → true
ak_utf8_is_digit(0x09E6);  // ০ (Bengali) → true
ak_utf8_is_digit(0x0E50);  // ๐ (Thai) → true
ak_utf8_is_digit(0xFF10);  // ０ (Fullwidth) → true
```

## Unicode Letters

```c
// Recognizes letters from any script
ak_utf8_is_alpha(0x41);    // 'A' (ASCII) → true
ak_utf8_is_alpha(0x00E9);  // é (Latin Extended) → true
ak_utf8_is_alpha(0x0391);  // Α (Greek) → true
ak_utf8_is_alpha(0x0410);  // А (Cyrillic) → true
ak_utf8_is_alpha(0x05D0);  // א (Hebrew) → true
ak_utf8_is_alpha(0x0627);  // ا (Arabic) → true
ak_utf8_is_alpha(0x4E00);  // 一 (CJK) → true
ak_utf8_is_alpha(0x3042);  // あ (Hiragana) → true
ak_utf8_is_alpha(0x30A2);  // ア (Katakana) → true
ak_utf8_is_alpha(0xAC00);  // 가 (Hangul) → true
```

## Iteration

```c
const uint8_t *text = (uint8_t *)"Hello 世界";
size_t byte_len = strlen((char *)text);
size_t pos = 0;

// Forward iteration
while (pos < byte_len) {
    size_t char_len = ak_utf8_char_len(text + pos, byte_len - pos);
    // Process character at text[pos] with length char_len
    pos += char_len;
}

// Backward iteration
pos = byte_len;
while (pos > 0) {
    pos = ak_utf8_prev_char(text, pos);
    // Process character starting at text[pos]
}
```

## Byte Classification

```c
uint8_t byte = text[pos];

// Check if byte starts a new character
if (ak_utf8_is_start_byte(byte)) {
    printf("Start of UTF-8 character\n");
}

// Check if byte is a continuation byte (10xxxxxx)
if (ak_utf8_is_continuation_byte(byte)) {
    printf("Continuation byte\n");
}
```

## Character Counting

```c
// Count bytes vs. characters
const uint8_t *text = (uint8_t *)"café";
size_t byte_len = strlen((char *)text);        // 5 bytes
size_t char_count = ak_utf8_char_count(text, byte_len); // 4 characters

// Mixed content
const uint8_t *mixed = (uint8_t *)"🎉 Hello 世界";
size_t chars = ak_utf8_char_count(mixed, strlen((char *)mixed));
// emoji (1) + space (1) + Hello (5) + space (1) + 世界 (2) = 10
```

## Validation

```c
// Validate before processing
const uint8_t *input = get_user_input();
size_t len = strlen((char *)input);

if (!ak_utf8_validate(input, len)) {
    fprintf(stderr, "Invalid UTF-8 input\n");
    return -1;
}

// Safe to process
size_t chars = ak_utf8_char_count(input, len);
```

## Whitespace Detection

```c
// Supports ASCII and common Unicode whitespace
ak_utf8_is_whitespace(0x20);   // space → true
ak_utf8_is_whitespace(0x09);   // tab → true
ak_utf8_is_whitespace(0x0A);   // LF → true
ak_utf8_is_whitespace(0x0D);   // CR → true
ak_utf8_is_whitespace(0x00A0); // non-breaking space → true
ak_utf8_is_whitespace(0x2003); // em space → true
ak_utf8_is_whitespace(0x3000); // ideographic space → true
```

## Grapheme Clusters

Grapheme clusters are user-perceived characters that may consist of multiple codepoints (base character + combining marks).

### Combining Marks

```c
// Check if a codepoint is a combining mark
ak_utf8_is_combining_mark(0x0301);  // Combining acute accent → true
ak_utf8_is_combining_mark(0x0308);  // Combining diaeresis → true
ak_utf8_is_combining_mark(0x0327);  // Combining cedilla → true
ak_utf8_is_combining_mark(0x0041);  // 'A' → false
```

### Grapheme Length

```c
// Precomposed character (single codepoint)
const uint8_t *precomposed = (uint8_t *)"é";  // U+00E9
size_t len = ak_utf8_grapheme_len(precomposed, 2);
// len = 2 (one grapheme cluster)

// Decomposed character (base + combining mark)
const uint8_t decomposed[] = {0x65, 0xCC, 0x81, 0x00};  // e + combining acute
len = ak_utf8_grapheme_len(decomposed, 3);
// len = 3 (one grapheme cluster: base + combining mark)

// Multiple combining marks
const uint8_t complex[] = {0x61, 0xCC, 0x80, 0xCC, 0x83, 0x00};  // a + grave + tilde
len = ak_utf8_grapheme_len(complex, 5);
// len = 5 (one grapheme cluster with multiple combining marks)
```

### Grapheme Counting

```c
// Count user-perceived characters
const uint8_t *text1 = (uint8_t *)"café";  // Precomposed é
size_t graphemes = ak_utf8_grapheme_count(text1, strlen((char *)text1));
// graphemes = 4 (c, a, f, é)

// Decomposed version
const uint8_t text2[] = {0x63, 0x61, 0x66, 0x65, 0xCC, 0x81, 0x00};  // e + combining
size_t chars = ak_utf8_char_count(text2, 6);  // 5 codepoints
graphemes = ak_utf8_grapheme_count(text2, 6);  // 4 grapheme clusters

// Hebrew with vowel points
const uint8_t hebrew[] = {0xD7, 0xA9, 0xD6, 0xB8, 0xD7, 0x81, 0x00};  // ש + vowels
chars = ak_utf8_char_count(hebrew, 6);      // 3 codepoints
graphemes = ak_utf8_grapheme_count(hebrew, 6);  // 1 grapheme cluster
```

## Thread Safety

**None of the UTF-8 functions are thread-safe.** They are designed for single-threaded use or require external synchronization. Multiple threads can safely read the same UTF-8 data concurrently, but any shared state must be protected.

## Error Handling

- Invalid UTF-8 sequences are treated as single bytes
- `ak_utf8_decode()` returns `valid=false` for invalid sequences
- `ak_utf8_validate()` returns `false` for any invalid UTF-8
- `ak_utf8_char_len()` returns `1` for invalid start bytes
- No functions return NULL or crash on invalid input

## Limitations

- Does not normalize Unicode (precomposed vs decomposed forms are treated as different)
- Does not handle complex grapheme clusters (emoji ZWJ sequences, regional indicators)
- Maximum codepoint: U+10FFFF (4-byte sequences) - per Unicode standard
