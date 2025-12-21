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
  // ASCII digits
  if (codepoint >= 0x0030 && codepoint <= 0x0039)
    return true;

  // Arabic-Indic digits
  if (codepoint >= 0x0660 && codepoint <= 0x0669)
    return true;

  // Extended Arabic-Indic digits
  if (codepoint >= 0x06F0 && codepoint <= 0x06F9)
    return true;

  // Devanagari digits
  if (codepoint >= 0x0966 && codepoint <= 0x096F)
    return true;

  // Bengali digits
  if (codepoint >= 0x09E6 && codepoint <= 0x09EF)
    return true;

  // Gurmukhi digits
  if (codepoint >= 0x0A66 && codepoint <= 0x0A6F)
    return true;

  // Gujarati digits
  if (codepoint >= 0x0AE6 && codepoint <= 0x0AEF)
    return true;

  // Oriya digits
  if (codepoint >= 0x0B66 && codepoint <= 0x0B6F)
    return true;

  // Tamil digits
  if (codepoint >= 0x0BE6 && codepoint <= 0x0BEF)
    return true;

  // Telugu digits
  if (codepoint >= 0x0C66 && codepoint <= 0x0C6F)
    return true;

  // Kannada digits
  if (codepoint >= 0x0CE6 && codepoint <= 0x0CEF)
    return true;

  // Malayalam digits
  if (codepoint >= 0x0D66 && codepoint <= 0x0D6F)
    return true;

  // Thai digits
  if (codepoint >= 0x0E50 && codepoint <= 0x0E59)
    return true;

  // Lao digits
  if (codepoint >= 0x0ED0 && codepoint <= 0x0ED9)
    return true;

  // Tibetan digits
  if (codepoint >= 0x0F20 && codepoint <= 0x0F29)
    return true;

  // Myanmar digits
  if (codepoint >= 0x1040 && codepoint <= 0x1049)
    return true;

  // Khmer digits
  if (codepoint >= 0x17E0 && codepoint <= 0x17E9)
    return true;

  // Mongolian digits
  if (codepoint >= 0x1810 && codepoint <= 0x1819)
    return true;

  // Fullwidth digits
  if (codepoint >= 0xFF10 && codepoint <= 0xFF19)
    return true;

  return false;
}

bool ak_utf8_is_alpha(ak_utf8_codepoint_t codepoint) {
  // ASCII letters (A-Z, a-z)
  if ((codepoint >= 0x0041 && codepoint <= 0x005A) ||
      (codepoint >= 0x0061 && codepoint <= 0x007A))
    return true;

  // Latin-1 Supplement letters (\u00c0-\u00d6, \u00d8-\u00f6, \u00f8-\u00ff)
  if ((codepoint >= 0x00C0 && codepoint <= 0x00D6) ||
      (codepoint >= 0x00D8 && codepoint <= 0x00F6) ||
      (codepoint >= 0x00F8 && codepoint <= 0x00FF))
    return true;

  // Latin Extended-A (U+0100-U+017F)
  if (codepoint >= 0x0100 && codepoint <= 0x017F)
    return true;

  // Latin Extended-B (U+0180-U+024F)
  if (codepoint >= 0x0180 && codepoint <= 0x024F)
    return true;

  // Greek and Coptic (U+0370-U+03FF)
  if (codepoint >= 0x0370 && codepoint <= 0x03FF)
    return true;

  // Cyrillic (U+0400-U+04FF)
  if (codepoint >= 0x0400 && codepoint <= 0x04FF)
    return true;

  // Cyrillic Supplement (U+0500-U+052F)
  if (codepoint >= 0x0500 && codepoint <= 0x052F)
    return true;

  // Armenian (U+0530-U+058F)
  if (codepoint >= 0x0531 && codepoint <= 0x0556)
    return true;
  if (codepoint >= 0x0561 && codepoint <= 0x0587)
    return true;

  // Hebrew (U+0590-U+05FF)
  if (codepoint >= 0x05D0 && codepoint <= 0x05EA)
    return true;
  if (codepoint >= 0x05F0 && codepoint <= 0x05F2)
    return true;

  // Arabic (U+0600-U+06FF)
  if (codepoint >= 0x0621 && codepoint <= 0x064A)
    return true;
  if (codepoint >= 0x066E && codepoint <= 0x066F)
    return true;
  if (codepoint >= 0x0671 && codepoint <= 0x06D3)
    return true;
  if (codepoint == 0x06D5)
    return true;
  if (codepoint >= 0x06E5 && codepoint <= 0x06E6)
    return true;
  if (codepoint >= 0x06EE && codepoint <= 0x06EF)
    return true;
  if (codepoint >= 0x06FA && codepoint <= 0x06FC)
    return true;
  if (codepoint == 0x06FF)
    return true;

  // Devanagari (U+0900-U+097F)
  if (codepoint >= 0x0905 && codepoint <= 0x0939)
    return true;
  if (codepoint >= 0x0958 && codepoint <= 0x0961)
    return true;
  if (codepoint >= 0x0972 && codepoint <= 0x097F)
    return true;

  // Bengali (U+0980-U+09FF)
  if (codepoint >= 0x0985 && codepoint <= 0x098C)
    return true;
  if (codepoint >= 0x098F && codepoint <= 0x0990)
    return true;
  if (codepoint >= 0x0993 && codepoint <= 0x09A8)
    return true;
  if (codepoint >= 0x09AA && codepoint <= 0x09B0)
    return true;
  if (codepoint == 0x09B2)
    return true;
  if (codepoint >= 0x09B6 && codepoint <= 0x09B9)
    return true;
  if (codepoint >= 0x09DC && codepoint <= 0x09DD)
    return true;
  if (codepoint >= 0x09DF && codepoint <= 0x09E1)
    return true;
  if (codepoint >= 0x09F0 && codepoint <= 0x09F1)
    return true;

  // Hiragana (U+3040-U+309F)
  if (codepoint >= 0x3041 && codepoint <= 0x3096)
    return true;

  // Katakana (U+30A0-U+30FF)
  if (codepoint >= 0x30A1 && codepoint <= 0x30FA)
    return true;

  // CJK Unified Ideographs (U+4E00-U+9FFF)
  if (codepoint >= 0x4E00 && codepoint <= 0x9FFF)
    return true;

  // Hangul Syllables (U+AC00-U+D7AF)
  if (codepoint >= 0xAC00 && codepoint <= 0xD7A3)
    return true;

  return false;
}

bool ak_utf8_is_alnum(ak_utf8_codepoint_t codepoint) {
  return ak_utf8_is_alpha(codepoint) || ak_utf8_is_digit(codepoint);
}

bool ak_utf8_is_combining_mark(ak_utf8_codepoint_t codepoint) {
  // Combining Diacritical Marks (U+0300-U+036F)
  if (codepoint >= 0x0300 && codepoint <= 0x036F)
    return true;

  // Combining Diacritical Marks Extended (U+1AB0-U+1AFF)
  if (codepoint >= 0x1AB0 && codepoint <= 0x1AFF)
    return true;

  // Combining Diacritical Marks Supplement (U+1DC0-U+1DFF)
  if (codepoint >= 0x1DC0 && codepoint <= 0x1DFF)
    return true;

  // Combining Diacritical Marks for Symbols (U+20D0-U+20FF)
  if (codepoint >= 0x20D0 && codepoint <= 0x20FF)
    return true;

  // Combining Half Marks (U+FE20-U+FE2F)
  if (codepoint >= 0xFE20 && codepoint <= 0xFE2F)
    return true;

  // Hebrew combining marks
  if (codepoint >= 0x0591 && codepoint <= 0x05BD)
    return true;
  if (codepoint == 0x05BF)
    return true;
  if (codepoint >= 0x05C1 && codepoint <= 0x05C2)
    return true;
  if (codepoint >= 0x05C4 && codepoint <= 0x05C5)
    return true;
  if (codepoint == 0x05C7)
    return true;

  // Arabic combining marks
  if (codepoint >= 0x0610 && codepoint <= 0x061A)
    return true;
  if (codepoint >= 0x064B && codepoint <= 0x065F)
    return true;
  if (codepoint == 0x0670)
    return true;
  if (codepoint >= 0x06D6 && codepoint <= 0x06DC)
    return true;
  if (codepoint >= 0x06DF && codepoint <= 0x06E4)
    return true;
  if (codepoint >= 0x06E7 && codepoint <= 0x06E8)
    return true;
  if (codepoint >= 0x06EA && codepoint <= 0x06ED)
    return true;

  // Devanagari combining marks
  if (codepoint >= 0x0900 && codepoint <= 0x0903)
    return true;
  if (codepoint >= 0x093A && codepoint <= 0x093C)
    return true;
  if (codepoint >= 0x093E && codepoint <= 0x094F)
    return true;
  if (codepoint >= 0x0951 && codepoint <= 0x0957)
    return true;
  if (codepoint >= 0x0962 && codepoint <= 0x0963)
    return true;

  return false;
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

size_t ak_utf8_grapheme_len(const uint8_t *data, size_t max_bytes) {
  if (!data || max_bytes == 0) {
    return 0;
  }

  // Get the base character
  size_t total_len = ak_utf8_char_len(data, max_bytes);
  if (total_len == 0 || total_len > max_bytes) {
    return total_len;
  }

  // Decode base character to check if it's valid
  ak_utf8_decode_result_t base = ak_utf8_decode(data, max_bytes);
  if (!base.valid) {
    return total_len;
  }

  // Continue consuming combining marks
  size_t pos = total_len;
  while (pos < max_bytes) {
    ak_utf8_decode_result_t next = ak_utf8_decode(data + pos, max_bytes - pos);

    if (!next.valid) {
      break;
    }

    if (!ak_utf8_is_combining_mark(next.codepoint)) {
      break;
    }

    total_len += next.bytes;
    pos += next.bytes;
  }

  return total_len;
}

size_t ak_utf8_grapheme_count(const uint8_t *data, size_t byte_len) {
  if (!data) {
    return 0;
  }

  size_t count = 0;
  size_t pos = 0;

  while (pos < byte_len) {
    size_t grapheme_len = ak_utf8_grapheme_len(data + pos, byte_len - pos);
    if (grapheme_len == 0) {
      break;
    }
    count++;
    pos += grapheme_len;
  }

  return count;
}
