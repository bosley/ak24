#include "sourceloc.h"
#include "intern.h"
#include "kernel.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Internal structure for source file
 *
 * Contains the file contents, cached line start positions for fast
 * offset-to-line conversion, and reference counting for lifetime management.
 */
struct ak_source_file_s {
  const char *filename;  /**< Interned filename */
  char *contents;        /**< File contents (owned) */
  size_t length;         /**< Length in bytes */
  uint32_t *line_starts; /**< Cached byte offsets of line starts */
  uint32_t line_count;   /**< Number of lines in file */
  uint32_t ref_count;    /**< Reference count */
  AK24_MUTEX lock;       /**< Lock for reference counting */
};

/**
 * @brief Global source location system state
 */
typedef struct {
  size_t file_count;  /**< Number of currently loaded files */
  size_t total_bytes; /**< Total bytes in all files */
  AK24_MUTEX lock;    /**< Lock for statistics */
  bool initialized;   /**< Whether system is initialized */
} sourceloc_system_t;

static sourceloc_system_t g_sourceloc_system = {0};

/**
 * @brief Count UTF-8 code points in a byte sequence
 *
 * Counts the number of UTF-8 code points (characters) in the given byte
 * sequence. Handles multi-byte UTF-8 sequences correctly.
 */
static size_t utf8_strlen(const char *str, size_t byte_len) {
  size_t count = 0;
  const unsigned char *s = (const unsigned char *)str;
  const unsigned char *end = s + byte_len;

  while (s < end) {
    unsigned char c = *s;

    if (c == 0) {
      break;
    }

    count++;

    // Skip continuation bytes
    if ((c & 0x80) == 0) {
      // Single-byte character (0xxxxxxx)
      s++;
    } else if ((c & 0xE0) == 0xC0) {
      // Two-byte character (110xxxxx)
      s += 2;
    } else if ((c & 0xF0) == 0xE0) {
      // Three-byte character (1110xxxx)
      s += 3;
    } else if ((c & 0xF8) == 0xF0) {
      // Four-byte character (11110xxx)
      s += 4;
    } else {
      // Invalid UTF-8 sequence, treat as single byte
      s++;
    }
  }

  return count;
}

/**
 * @brief Compute line start positions for a source file
 *
 * Scans through the file contents and records the byte offset of each
 * line start. This enables fast binary search for offset-to-line conversion.
 */
static bool compute_line_starts(ak_source_file_t *file) {
  // First pass: count lines
  uint32_t line_count = 1; // At least one line (even if empty)
  for (size_t i = 0; i < file->length; i++) {
    if (file->contents[i] == '\n' && i + 1 < file->length) {
      line_count++;
    }
  }

  file->line_starts = AK24_ALLOC_ATOMIC(line_count * sizeof(uint32_t));
  if (!file->line_starts) {
    return false;
  }

  file->line_count = line_count;

  uint32_t line_idx = 0;
  file->line_starts[line_idx++] = 0;

  for (size_t i = 0; i < file->length; i++) {
    if (file->contents[i] == '\n' && i + 1 < file->length) {
      file->line_starts[line_idx++] = (uint32_t)(i + 1);
    }
  }

  return true;
}

/**
 * @brief Read entire file into memory
 */
static char *read_file(const char *path, size_t *out_len) {
  FILE *fp = fopen(path, "rb");
  if (!fp) {
    return NULL;
  }

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  if (size < 0) {
    fclose(fp);
    return NULL;
  }

  char *buffer = AK24_ALLOC_ATOMIC((size_t)size + 1);
  if (!buffer) {
    fclose(fp);
    return NULL;
  }

  size_t bytes_read = fread(buffer, 1, (size_t)size, fp);
  fclose(fp);

  buffer[bytes_read] = '\0';
  *out_len = bytes_read;

  return buffer;
}

void ak_sourceloc_init(void) {
  if (g_sourceloc_system.initialized) {
    return;
  }

  AK24_MUTEX_INIT(&g_sourceloc_system.lock);

  g_sourceloc_system.file_count = 0;
  g_sourceloc_system.total_bytes = 0;
  g_sourceloc_system.initialized = true;
}

void ak_sourceloc_shutdown(void) {
  if (!g_sourceloc_system.initialized) {
    return;
  }

  AK24_MUTEX_LOCK(&g_sourceloc_system.lock);

  // Note: Individual files are freed via reference counting
  // This just cleans up the global state

  g_sourceloc_system.file_count = 0;
  g_sourceloc_system.total_bytes = 0;
  g_sourceloc_system.initialized = false;

  AK24_MUTEX_UNLOCK(&g_sourceloc_system.lock);
  AK24_MUTEX_DESTROY(&g_sourceloc_system.lock);
}

ak_source_file_t *ak_source_file_new(const char *filename, const char *contents,
                                     size_t len) {
  if (!filename || !contents) {
    return NULL;
  }

  ak_source_file_t *file = AK24_ALLOC(sizeof(ak_source_file_t));
  if (!file) {
    return NULL;
  }

  if (AK24_MUTEX_INIT(&file->lock) != 0) {
    AK24_FREE(file);
    return NULL;
  }

  file->filename = ak_intern(filename);
  if (!file->filename) {
    AK24_MUTEX_DESTROY(&file->lock);
    AK24_FREE(file);
    return NULL;
  }

  file->contents = AK24_ALLOC_ATOMIC(len + 1);
  if (!file->contents) {
    AK24_MUTEX_DESTROY(&file->lock);
    AK24_FREE(file);
    return NULL;
  }
  memcpy(file->contents, contents, len);
  file->contents[len] = '\0';
  file->length = len;

  if (!compute_line_starts(file)) {
    AK24_FREE(file->contents);
    AK24_MUTEX_DESTROY(&file->lock);
    AK24_FREE(file);
    return NULL;
  }

  file->ref_count = 1;

  if (g_sourceloc_system.initialized) {
    AK24_MUTEX_LOCK(&g_sourceloc_system.lock);
    g_sourceloc_system.file_count++;
    g_sourceloc_system.total_bytes += len;
    AK24_MUTEX_UNLOCK(&g_sourceloc_system.lock);
  }

  return file;
}

ak_source_file_t *ak_source_file_from_path(const char *path) {
  if (!path) {
    return NULL;
  }

  size_t len;
  char *contents = read_file(path, &len);
  if (!contents) {
    return NULL;
  }

  ak_source_file_t *file = ak_source_file_new(path, contents, len);
  AK24_FREE(contents);

  return file;
}

ak_source_file_t *ak_source_file_retain(ak_source_file_t *file) {
  if (!file) {
    return NULL;
  }

  AK24_MUTEX_LOCK(&file->lock);
  file->ref_count++;
  AK24_MUTEX_UNLOCK(&file->lock);

  return file;
}

void ak_source_file_release(ak_source_file_t *file) {
  if (!file) {
    return;
  }

  AK24_MUTEX_LOCK(&file->lock);
  file->ref_count--;
  uint32_t ref_count = file->ref_count;
  AK24_MUTEX_UNLOCK(&file->lock);

  if (ref_count == 0) {
    if (g_sourceloc_system.initialized) {
      AK24_MUTEX_LOCK(&g_sourceloc_system.lock);
      g_sourceloc_system.file_count--;
      g_sourceloc_system.total_bytes -= file->length;
      AK24_MUTEX_UNLOCK(&g_sourceloc_system.lock);
    }

    AK24_FREE(file->line_starts);
    AK24_FREE(file->contents);
    AK24_MUTEX_DESTROY(&file->lock);
    AK24_FREE(file);
  }
}

const char *ak_source_file_contents(ak_source_file_t *file) {
  return file ? file->contents : NULL;
}

size_t ak_source_file_length(ak_source_file_t *file) {
  return file ? file->length : 0;
}

const char *ak_source_file_name(ak_source_file_t *file) {
  return file ? file->filename : NULL;
}

ak_source_loc_t ak_source_loc_new(ak_source_file_t *file, uint32_t line,
                                  uint32_t column, uint32_t offset) {
  ak_source_loc_t loc;
  loc.file = file;
  loc.line = line;
  loc.column = column;
  loc.offset = offset;
  return loc;
}

ak_source_loc_t ak_source_loc_from_offset(ak_source_file_t *file,
                                          uint32_t offset) {
  ak_source_loc_t loc = {0};

  if (!file || offset > file->length) {
    return loc;
  }

  loc.file = file;
  loc.offset = offset;

  uint32_t left = 0;
  uint32_t right = file->line_count - 1;
  uint32_t line_idx = 0;

  while (left <= right) {
    uint32_t mid = left + (right - left) / 2;

    if (file->line_starts[mid] <= offset) {
      line_idx = mid;
      if (mid == right) {
        break;
      }
      left = mid + 1;
    } else {
      if (mid == 0) {
        break;
      }
      right = mid - 1;
    }
  }

  loc.line = line_idx + 1;

  // Calculate column by counting UTF-8 code points from line start
  uint32_t line_start = file->line_starts[line_idx];
  uint32_t byte_offset_in_line = offset - line_start;

  loc.column = (uint32_t)utf8_strlen(file->contents + line_start,
                                     byte_offset_in_line) +
               1; // 1-based

  return loc;
}

ak_source_range_t ak_source_range_new(ak_source_loc_t start,
                                      ak_source_loc_t end) {
  ak_source_range_t range;
  range.start = start;
  range.end = end;
  return range;
}

bool ak_source_range_contains(ak_source_range_t *range, ak_source_loc_t loc) {
  if (!range || !range->start.file || !loc.file) {
    return false;
  }

  if (range->start.file != loc.file) {
    return false;
  }

  return loc.offset >= range->start.offset && loc.offset < range->end.offset;
}

bool ak_source_range_overlaps(ak_source_range_t *a, ak_source_range_t *b) {
  if (!a || !b || !a->start.file || !b->start.file) {
    return false;
  }

  if (a->start.file != b->start.file) {
    return false;
  }

  return a->start.offset < b->end.offset && b->start.offset < a->end.offset;
}

char *ak_source_extract(ak_source_range_t range) {
  if (!range.start.file || !range.end.file) {
    return NULL;
  }

  if (range.start.file != range.end.file) {
    return NULL;
  }

  ak_source_file_t *file = range.start.file;

  if (range.start.offset > file->length || range.end.offset > file->length) {
    return NULL;
  }

  if (range.start.offset > range.end.offset) {
    return NULL;
  }

  size_t len = range.end.offset - range.start.offset;
  char *text = AK24_ALLOC_ATOMIC(len + 1);
  if (!text) {
    return NULL;
  }

  memcpy(text, file->contents + range.start.offset, len);
  text[len] = '\0';

  return text;
}

const char *ak_source_get_line(ak_source_file_t *file, uint32_t line) {
  if (!file || line == 0 || line > file->line_count) {
    return NULL;
  }

  uint32_t line_idx = line - 1; // Convert to 0-based index
  return file->contents + file->line_starts[line_idx];
}

size_t ak_source_get_line_len(ak_source_file_t *file, uint32_t line) {
  if (!file || line == 0 || line > file->line_count) {
    return 0;
  }

  uint32_t line_idx = line - 1; // Convert to 0-based index
  uint32_t line_start = file->line_starts[line_idx];
  uint32_t line_end;

  if (line_idx + 1 < file->line_count) {
    line_end = file->line_starts[line_idx + 1];
    if (line_end > 0 && file->contents[line_end - 1] == '\n') {
      line_end--;
      if (line_end > 0 && file->contents[line_end - 1] == '\r') {
        line_end--;
      }
    }
  } else {
    line_end = (uint32_t)file->length;
    if (line_end > line_start && file->contents[line_end - 1] == '\n') {
      line_end--;
      if (line_end > line_start && file->contents[line_end - 1] == '\r') {
        line_end--;
      }
    }
  }

  return line_end > line_start ? (line_end - line_start) : 0;
}

int ak_source_loc_format(ak_source_loc_t loc, char *buf, size_t size) {
  if (!loc.file || !buf || size == 0) {
    return 0;
  }
  return snprintf(buf, size, "%s:%u:%u", loc.file->filename, loc.line,
                  loc.column);
}

int ak_source_range_format(ak_source_range_t range, char *buf, size_t size) {
  if (!range.start.file || !buf || size == 0) {
    return 0;
  }
  if (range.start.line == range.end.line) {
    return snprintf(buf, size, "%s:%u:%u-%u", range.start.file->filename,
                    range.start.line, range.start.column, range.end.column);
  }
  return snprintf(buf, size, "%s:%u:%u-%u:%u", range.start.file->filename,
                  range.start.line, range.start.column, range.end.line,
                  range.end.column);
}

void ak_sourceloc_stats(size_t *file_count, size_t *total_bytes) {
  if (!g_sourceloc_system.initialized) {
    if (file_count) {
      *file_count = 0;
    }
    if (total_bytes) {
      *total_bytes = 0;
    }
    return;
  }

  AK24_MUTEX_LOCK(&g_sourceloc_system.lock);

  if (file_count) {
    *file_count = g_sourceloc_system.file_count;
  }
  if (total_bytes) {
    *total_bytes = g_sourceloc_system.total_bytes;
  }
  AK24_MUTEX_UNLOCK(&g_sourceloc_system.lock);
}
