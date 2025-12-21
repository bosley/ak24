#include "scanner.h"
#include "kernel.h"
#include "utf8.h"

ak_scanner_t *ak_scanner_new(ak_buffer_t *buffer, size_t position) {
  if (!buffer) {
    return NULL;
  }

  if (position > buffer->count) {
    return NULL;
  }

  ak_scanner_t *scanner = AK24_ALLOC(sizeof(ak_scanner_t));
  if (!scanner) {
    return NULL;
  }

  scanner->buffer = buffer;
  scanner->position = position;

  return scanner;
}

void ak_scanner_free(ak_scanner_t *scanner) {
  if (!scanner) {
    return;
  }

  AK24_FREE(scanner);
}

// UTF-8 aware helper: check if current position is whitespace
static bool is_whitespace_at(ak_buffer_t *buf, size_t pos) {
  if (pos >= buf->count) {
    return false;
  }

  size_t remaining = buf->count - pos;
  ak_utf8_decode_result_t result = ak_utf8_decode(buf->data + pos, remaining);

  if (!result.valid) {
    // For invalid sequences, fall back to ASCII check
    uint8_t c = buf->data[pos];
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
  }

  return ak_utf8_is_whitespace(result.codepoint);
}

// UTF-8 aware helper: check if current position is a digit
static bool is_digit_at(ak_buffer_t *buf, size_t pos) {
  if (pos >= buf->count) {
    return false;
  }

  size_t remaining = buf->count - pos;
  ak_utf8_decode_result_t result = ak_utf8_decode(buf->data + pos, remaining);

  if (!result.valid) {
    // For invalid sequences, fall back to ASCII check
    uint8_t c = buf->data[pos];
    return c >= '0' && c <= '9';
  }

  return ak_utf8_is_digit(result.codepoint);
}

// Get byte length of character at position (UTF-8 aware)
static size_t char_byte_len_at(ak_buffer_t *buf, size_t pos) {
  if (pos >= buf->count) {
    return 0;
  }
  return ak_utf8_char_len(buf->data + pos, buf->count - pos);
}

static bool is_stop_symbol(uint8_t c, ak_scanner_stop_symbols_t *stop_symbols) {
  if (!stop_symbols || !stop_symbols->symbols) {
    return false;
  }
  for (size_t i = 0; i < stop_symbols->count; i++) {
    if (stop_symbols->symbols[i] == c) {
      return true;
    }
  }
  return false;
}

/*
============================================================================================================
This is the static type parser that yeets off of the buffer
it follows a simple state machine and only parses the most primitive types
"static base types" that represent some "thing" that does not have an "inner"
(conceptually) For instance: an integer absolutly has "bits" but if we consider
it from the mindset of "physics" these types would be like atoms. The bits are
there sure, but thats a different "scale" In classic lisps these are called
atoms, but we aren't necessarily parsing a lisp here so i wanted to stay away
from the terminology. Esepcially since AI have been doing docs and tests for me
- its just easier
============================================================================================================
*/

ak_scanner_static_type_result_t
ak_scanner_read_static_base_type(ak_scanner_t *scanner,
                                 ak_scanner_stop_symbols_t *stop_symbols) {
  if (!scanner) {
    return (ak_scanner_static_type_result_t){
        .success = false,
        .start_position = 0,
        .error_position = 0,
        .data = {
            .base = AK24_STATIC_BASE_NONE, .data = NULL, .byte_length = 0}};
  }

  size_t start_pos = scanner->position;
  size_t pos = start_pos;
  ak_buffer_t *buf = scanner->buffer;

  // Skip leading whitespace (UTF-8 aware)
  while (pos < buf->count && is_whitespace_at(buf, pos)) {
    size_t char_len = char_byte_len_at(buf, pos);
    if (char_len == 0)
      break;
    pos += char_len;
  }

  if (pos >= buf->count) {
    return (ak_scanner_static_type_result_t){
        .success = false,
        .start_position = start_pos,
        .error_position = pos,
        .data = {
            .base = AK24_STATIC_BASE_NONE, .data = NULL, .byte_length = 0}};
  }

  if (is_stop_symbol(buf->data[pos], stop_symbols)) {
    return (ak_scanner_static_type_result_t){
        .success = false,
        .start_position = start_pos,
        .error_position = pos,
        .data = {
            .base = AK24_STATIC_BASE_NONE, .data = NULL, .byte_length = 0}};
  }

  size_t token_start = pos;
  uint8_t first_char = buf->data[pos];

  typedef enum {
    STATE_START,
    STATE_SIGN,
    STATE_INTEGER,
    STATE_REAL,
    STATE_SYMBOL,
    STATE_ERROR
  } parse_state_t;

  parse_state_t state = STATE_START;
  bool has_sign = false;
  bool has_period = false;

  if (first_char == '+' || first_char == '-') {
    has_sign = true;
    (void)has_sign;
    pos++;

    if (pos >= buf->count) {
      state = STATE_SYMBOL;
      pos = token_start + 1;
    } else if (is_digit_at(buf, pos)) {
      state = STATE_INTEGER;
    } else if (is_whitespace_at(buf, pos)) {
      state = STATE_SYMBOL;
      pos = token_start + 1;
    } else {
      state = STATE_SYMBOL;
    }
  } else if (is_digit_at(buf, pos)) {
    state = STATE_INTEGER;
  } else {
    state = STATE_SYMBOL;
  }

  while (pos < buf->count && state != STATE_ERROR) {
    uint8_t c = buf->data[pos];

    if (is_whitespace_at(buf, pos)) {
      break;
    }

    if (is_stop_symbol(c, stop_symbols)) {
      break;
    }

    switch (state) {
    case STATE_INTEGER:
      if (is_digit_at(buf, pos)) {
        pos++;
      } else if (c == '.') {
        has_period = true;
        (void)has_period;
        state = STATE_REAL;
        pos++;
      } else {
        state = STATE_ERROR;
      }
      break;

    case STATE_REAL:
      if (is_digit_at(buf, pos)) {
        pos++;
      } else if (c == '.') {
        state = STATE_ERROR;
      } else {
        state = STATE_ERROR;
      }
      break;

    case STATE_SYMBOL:
      // For symbols, advance by full UTF-8 character length
      {
        size_t char_len = char_byte_len_at(buf, pos);
        if (char_len == 0) {
          state = STATE_ERROR;
        } else {
          pos += char_len;
        }
      }
      break;

    default:
      state = STATE_ERROR;
      break;
    }
  }

  if (state == STATE_ERROR) {
    return (ak_scanner_static_type_result_t){
        .success = false,
        .start_position = start_pos,
        .error_position = pos,
        .data = {
            .base = AK24_STATIC_BASE_NONE, .data = NULL, .byte_length = 0}};
  }

  if (pos == token_start) {
    return (ak_scanner_static_type_result_t){
        .success = false,
        .start_position = start_pos,
        .error_position = pos,
        .data = {
            .base = AK24_STATIC_BASE_NONE, .data = NULL, .byte_length = 0}};
  }

  ak_static_base_e base_type;
  if (state == STATE_INTEGER) {
    base_type = AK24_STATIC_BASE_INTEGER;
  } else if (state == STATE_REAL) {
    base_type = AK24_STATIC_BASE_REAL;
  } else {
    base_type = AK24_STATIC_BASE_SYMBOL;
  }

  size_t token_length = pos - token_start;
  scanner->position = pos;

  return (ak_scanner_static_type_result_t){
      .success = true,
      .start_position = start_pos,
      .error_position = 0,
      .data = {.base = base_type,
               .data = &buf->data[token_start],
               .byte_length = token_length}};
}

/*
============================================================================================================
This scans the buffer from the current start position until some given end
symbol We offer the option of "escaping" briefly from detecting the end byte
============================================================================================================
*/

ak_scanner_find_group_result_t ak_scanner_find_group(ak_scanner_t *scanner,
                                                     uint8_t must_start_with,
                                                     uint8_t must_end_with,
                                                     uint8_t *can_escape_with,
                                                     bool consume_leading_ws) {
  if (!scanner) {
    return (ak_scanner_find_group_result_t){.success = false,
                                            .index_of_start_symbol = 0,
                                            .index_of_closing_symbol = 0};
  }

  ak_buffer_t *buf = scanner->buffer;
  if (!buf) {
    return (ak_scanner_find_group_result_t){.success = false,
                                            .index_of_start_symbol = 0,
                                            .index_of_closing_symbol = 0};
  }

  size_t pos = scanner->position;

  if (pos >= buf->count) {
    return (ak_scanner_find_group_result_t){.success = false,
                                            .index_of_start_symbol = 0,
                                            .index_of_closing_symbol = 0};
  }

  if (consume_leading_ws) {
    while (pos < buf->count && is_whitespace_at(buf, pos)) {
      size_t char_len = char_byte_len_at(buf, pos);
      if (char_len == 0)
        break;
      pos += char_len;
    }

    if (pos >= buf->count) {
      return (ak_scanner_find_group_result_t){.success = false,
                                              .index_of_start_symbol = 0,
                                              .index_of_closing_symbol = 0};
    }
  }

  if (buf->data[pos] != must_start_with) {
    return (ak_scanner_find_group_result_t){.success = false,
                                            .index_of_start_symbol = 0,
                                            .index_of_closing_symbol = 0};
  }

  size_t start_index = pos;
  pos++;

  bool same_delimiters = (must_start_with == must_end_with);
  int depth = 1;

  while (pos < buf->count) {
    uint8_t current = buf->data[pos];

    bool is_escaped = false;
    if (can_escape_with != NULL && pos > start_index + 1) {
      if (buf->data[pos - 1] == *can_escape_with) {
        is_escaped = true;
      }
    }

    if (!is_escaped) {
      if (same_delimiters) {
        if (current == must_end_with) {
          scanner->position = pos;
          return (ak_scanner_find_group_result_t){
              .success = true,
              .index_of_start_symbol = start_index,
              .index_of_closing_symbol = pos};
        }
      } else {
        if (current == must_start_with) {
          depth++;
        } else if (current == must_end_with) {
          depth--;
          if (depth == 0) {
            scanner->position = pos;
            return (ak_scanner_find_group_result_t){
                .success = true,
                .index_of_start_symbol = start_index,
                .index_of_closing_symbol = pos};
          }
        }
      }
    }

    pos++;
  }

  return (ak_scanner_find_group_result_t){.success = false,
                                          .index_of_start_symbol = 0,
                                          .index_of_closing_symbol = 0};
}

bool ak_scanner_goto_next_non_white(ak_scanner_t *scanner) {
  if (!scanner) {
    return false;
  }

  ak_buffer_t *buf = scanner->buffer;
  if (!buf) {
    return false;
  }

  size_t pos = scanner->position;

  // UTF-8 aware whitespace skipping
  while (pos < buf->count && is_whitespace_at(buf, pos)) {
    size_t char_len = char_byte_len_at(buf, pos);
    if (char_len == 0)
      break;
    pos += char_len;
  }

  if (pos >= buf->count) {
    return false;
  }

  scanner->position = pos;
  return true;
}

bool ak_scanner_skip_whitespace_and_comments(ak_scanner_t *scanner) {
  if (!scanner) {
    return false;
  }

  ak_buffer_t *buf = scanner->buffer;
  if (!buf) {
    return false;
  }

  size_t pos = scanner->position;

  while (pos < buf->count) {
    // UTF-8 aware whitespace check
    if (is_whitespace_at(buf, pos)) {
      size_t char_len = char_byte_len_at(buf, pos);
      if (char_len == 0)
        break;
      pos += char_len;
      continue;
    }

    if (buf->data[pos] == ';') {
      while (pos < buf->count && buf->data[pos] != '\n') {
        pos++;
      }
      if (pos < buf->count && buf->data[pos] == '\n') {
        pos++;
      }
      continue;
    }

    break;
  }

  if (pos >= buf->count) {
    return false;
  }

  scanner->position = pos;
  return true;
}

bool ak_scanner_goto_next_target(ak_scanner_t *scanner, uint8_t target_byte) {
  if (!scanner) {
    return false;
  }

  ak_buffer_t *buf = scanner->buffer;
  if (!buf) {
    return false;
  }
  size_t pos = scanner->position;

  if (pos >= buf->count) {
    return false;
  }

  if (buf->data[pos] == target_byte) {
    scanner->position = pos;
    return true;
  }

  while (pos < buf->count && buf->data[pos] != target_byte) {
    pos++;
  }

  if (pos >= buf->count) {
    return false;
  }

  scanner->position = pos;
  return true;
}
