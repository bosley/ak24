#include "utf8.h"
#include <string.h>

size_t ak_utf8_char_len(const uint8_t *data, size_t max_bytes) {
  if (!data || max_bytes == 0) {
    return 0;
  }

  uint8_t first = data[0];

  // Single-byte character (0xxxxxxx) - ASCII
  if ((first & 0x80) == 0) {
    return 1;
  }

  // Two-byte character (110xxxxx 10xxxxxx)
  if ((first & 0xE0) == 0xC0) {
    if (max_bytes >= 2 && (data[1] & 0xC0) == 0x80) {
      return 2;
    }
    return 1; // Invalid, treat as single byte
  }

  // Three-byte character (1110xxxx 10xxxxxx 10xxxxxx)
  if ((first & 0xF0) == 0xE0) {
    if (max_bytes >= 3 && (data[1] & 0xC0) == 0x80 &&
        (data[2] & 0xC0) == 0x80) {
      return 3;
    }
    return 1; // Invalid, treat as single byte
  }

  // Four-byte character (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
  if ((first & 0xF8) == 0xF0) {
    if (max_bytes >= 4 && (data[1] & 0xC0) == 0x80 &&
        (data[2] & 0xC0) == 0x80 && (data[3] & 0xC0) == 0x80) {
      return 4;
    }
    return 1; // Invalid, treat as single byte
  }

  // Invalid UTF-8 start byte or continuation byte
  return 1;
}

ak_utf8_decode_result_t ak_utf8_decode(const uint8_t *data, size_t max_bytes) {
  ak_utf8_decode_result_t result = {0, 0, false};

  if (!data || max_bytes == 0) {
    return result;
  }

  uint8_t first = data[0];

  // Single-byte character (0xxxxxxx) - ASCII
  if ((first & 0x80) == 0) {
    result.codepoint = first;
    result.bytes = 1;
    result.valid = true;
    return result;
  }

  // Two-byte character (110xxxxx 10xxxxxx)
  if ((first & 0xE0) == 0xC0) {
    if (max_bytes >= 2 && (data[1] & 0xC0) == 0x80) {
      result.codepoint = ((first & 0x1F) << 6) | (data[1] & 0x3F);
      result.bytes = 2;
      result.valid = (result.codepoint >= 0x80); // Check for overlong encoding
      return result;
    }
    // Invalid sequence
    result.codepoint = first;
    result.bytes = 1;
    result.valid = false;
    return result;
  }

  // Three-byte character (1110xxxx 10xxxxxx 10xxxxxx)
  if ((first & 0xF0) == 0xE0) {
    if (max_bytes >= 3 && (data[1] & 0xC0) == 0x80 &&
        (data[2] & 0xC0) == 0x80) {
      result.codepoint =
          ((first & 0x0F) << 12) | ((data[1] & 0x3F) << 6) | (data[2] & 0x3F);
      result.bytes = 3;
      // Check for overlong encoding and surrogate pairs
      result.valid = (result.codepoint >= 0x800) &&
                     (result.codepoint < 0xD800 || result.codepoint > 0xDFFF);
      return result;
    }
    // Invalid sequence
    result.codepoint = first;
    result.bytes = 1;
    result.valid = false;
    return result;
  }

  // Four-byte character (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
  if ((first & 0xF8) == 0xF0) {
    if (max_bytes >= 4 && (data[1] & 0xC0) == 0x80 &&
        (data[2] & 0xC0) == 0x80 && (data[3] & 0xC0) == 0x80) {
      result.codepoint = ((first & 0x07) << 18) | ((data[1] & 0x3F) << 12) |
                         ((data[2] & 0x3F) << 6) | (data[3] & 0x3F);
      result.bytes = 4;
      // Check for overlong encoding and valid range
      result.valid =
          (result.codepoint >= 0x10000) && (result.codepoint <= 0x10FFFF);
      return result;
    }
    // Invalid sequence
    result.codepoint = first;
    result.bytes = 1;
    result.valid = false;
    return result;
  }

  // Invalid UTF-8 start byte
  result.codepoint = first;
  result.bytes = 1;
  result.valid = false;
  return result;
}

size_t ak_utf8_char_count(const uint8_t *data, size_t byte_len) {
  if (!data) {
    return 0;
  }

  size_t count = 0;
  size_t pos = 0;

  while (pos < byte_len) {
    size_t char_len = ak_utf8_char_len(data + pos, byte_len - pos);
    if (char_len == 0) {
      break;
    }
    count++;
    pos += char_len;
  }

  return count;
}

bool ak_utf8_is_whitespace(ak_utf8_codepoint_t codepoint) {
  // ASCII whitespace
  if (codepoint == 0x20 || // Space
      codepoint == 0x09 || // Tab
      codepoint == 0x0A || // Line feed
      codepoint == 0x0D) { // Carriage return
    return true;
  }

  // Common Unicode whitespace
  if (codepoint == 0x00A0 ||                          // No-break space
      codepoint == 0x1680 ||                          // Ogham space mark
      (codepoint >= 0x2000 && codepoint <= 0x200A) || // Various spaces
      codepoint == 0x202F ||                          // Narrow no-break space
      codepoint == 0x205F || // Medium mathematical space
      codepoint == 0x3000) { // Ideographic space
    return true;
  }

  return false;
}

bool ak_utf8_is_digit(ak_utf8_codepoint_t codepoint) {
  // ASCII digits only (0-9)
  return codepoint >= 0x30 && codepoint <= 0x39;
}

bool ak_utf8_is_alpha(ak_utf8_codepoint_t codepoint) {
  // ASCII letters only (A-Z, a-z)
  return (codepoint >= 0x41 && codepoint <= 0x5A) || // A-Z
         (codepoint >= 0x61 && codepoint <= 0x7A);   // a-z
}

bool ak_utf8_is_alnum(ak_utf8_codepoint_t codepoint) {
  return ak_utf8_is_alpha(codepoint) || ak_utf8_is_digit(codepoint);
}

bool ak_utf8_is_start_byte(uint8_t byte) {
  // Valid start bytes:
  // - 0xxxxxxx (ASCII)
  // - 110xxxxx (2-byte)
  // - 1110xxxx (3-byte)
  // - 11110xxx (4-byte)
  // Invalid: 10xxxxxx (continuation) and 11111xxx (invalid)

  if ((byte & 0x80) == 0) {
    return true; // ASCII
  }

  if ((byte & 0xC0) == 0x80) {
    return false; // Continuation byte
  }

  if ((byte & 0xFE) == 0xFE) {
    return false; // Invalid (11111110 or 11111111)
  }

  return true;
}

bool ak_utf8_is_continuation_byte(uint8_t byte) {
  // Continuation bytes: 10xxxxxx
  return (byte & 0xC0) == 0x80;
}

size_t ak_utf8_prev_char(const uint8_t *data, size_t current_pos) {
  if (current_pos == 0) {
    return 0;
  }

  size_t pos = current_pos - 1;

  // Skip continuation bytes (max 3 for valid UTF-8)
  size_t steps = 0;
  while (pos > 0 && ak_utf8_is_continuation_byte(data[pos]) && steps < 3) {
    pos--;
    steps++;
  }

  return pos;
}

bool ak_utf8_validate(const uint8_t *data, size_t byte_len) {
  if (!data) {
    return false;
  }

  size_t pos = 0;

  while (pos < byte_len) {
    ak_utf8_decode_result_t result = ak_utf8_decode(data + pos, byte_len - pos);

    if (!result.valid || result.bytes == 0) {
      return false;
    }

    pos += result.bytes;
  }

  return true;
}
