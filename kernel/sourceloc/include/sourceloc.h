/**
 * @file sourceloc.h
 * @brief Source location tracking for error reporting and debugging
 *
 * Provides source code position tracking with line/column numbers for error
 * messages, diagnostics, debugging information, and tooling support. Essential
 * for compiler development where every AST node, token, and diagnostic needs
 * associated source location information.
 *
 * Key features:
 * - Track source file positions with line/column/offset
 * - Source ranges for spans of code
 * - UTF-8 aware column counting
 * - Fast offset-to-line/column conversion via cached line starts
 * - Extract source text from ranges
 * - Format locations for display in diagnostics
 * - Thread-safe read operations after file load
 * - Reference counting for file lifetime management
 * - Integration with string interning for filenames
 *
 * Usage pattern:
 * @code
 * ak_sourceloc_init();
 *
 * // Load a source file
 * ak_source_file_t *file = ak_source_file_from_path("test.c");
 *
 * // Create a location
 * ak_source_loc_t loc = ak_source_loc_new(file, 10, 5, 142);
 *
 * // Format for error message
 * char buf[256];
 * ak_source_loc_format(loc, buf, sizeof(buf));
 * printf("Error at %s\n", buf);  // "test.c:10:5"
 *
 * ak_source_file_release(file);
 * ak_sourceloc_shutdown();
 * @endcode
 *
 * @note All source files persist until released via reference counting
 * @note Line and column numbers are 1-based (user-facing)
 * @note Offsets are 0-based byte indices
 * @note UTF-8 code points count as single columns
 */

#ifndef AK24_SOURCELOC_H
#define AK24_SOURCELOC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Opaque source file structure
 *
 * Represents a loaded source file with cached line start positions
 * for fast line/column lookups. Reference counted for lifetime management.
 */
typedef struct ak_source_file_s ak_source_file_t;

/**
 * @brief Source code location (single point)
 *
 * Represents a specific position in a source file with line, column,
 * and byte offset information.
 */
typedef struct {
  ak_source_file_t *file; /**< Source file (NULL for invalid location) */
  uint32_t line;          /**< Line number (1-based) */
  uint32_t column;        /**< Column number (1-based, UTF-8 code points) */
  uint32_t offset;        /**< Byte offset from start of file (0-based) */
} ak_source_loc_t;

/**
 * @brief Source code range (span)
 *
 * Represents a range of source code from a start location to an end location.
 * Both locations must be in the same file.
 */
typedef struct {
  ak_source_loc_t start; /**< Start of range (inclusive) */
  ak_source_loc_t end;   /**< End of range (exclusive) */
} ak_source_range_t;

/**
 * @brief Initialize the source location system
 *
 * Must be called before any other sourceloc functions. Sets up internal
 * data structures for file tracking.
 *
 * @threadsafe Yes - but only call once at program start
 *
 * @note Typically called from ak_kernel_init()
 */
void ak_sourceloc_init(void);

/**
 * @brief Shutdown and cleanup the source location system
 *
 * Frees all remaining source files and associated memory. Should be called
 * at program exit after all source files have been released.
 *
 * @threadsafe Yes - but only call once at program end
 *
 * @note Typically called from ak_kernel_deinit()
 * @warning Any remaining source file pointers become invalid
 */
void ak_sourceloc_shutdown(void);

/**
 * @brief Create a new source file from memory
 *
 * Creates a source file object from in-memory content. Takes ownership
 * of the content (makes a copy). The filename is interned for efficiency.
 *
 * @param filename Name/path of source file (will be interned)
 * @param contents Source file contents (will be copied)
 * @param len Length of contents in bytes
 * @return New source file with ref count 1, or NULL on failure
 *
 * @threadsafe Yes
 *
 * @note Filename is interned using ak_intern()
 * @note Line start positions are computed and cached
 * @note Caller must eventually call ak_source_file_release()
 *
 * Example:
 * @code
 * const char *code = "int main() {\n  return 0;\n}\n";
 * ak_source_file_t *file = ak_source_file_new("test.c", code, strlen(code));
 * @endcode
 */
ak_source_file_t *ak_source_file_new(const char *filename, const char *contents,
                                     size_t len);

/**
 * @brief Load a source file from disk
 *
 * Reads a file from the filesystem and creates a source file object.
 * The filename/path is interned for efficiency.
 *
 * @param path Path to source file to load
 * @return New source file with ref count 1, or NULL on failure
 *
 * @threadsafe Yes
 *
 * @note Path is interned using ak_intern()
 * @note File is read entirely into memory
 * @note Caller must eventually call ak_source_file_release()
 *
 * Example:
 * @code
 * ak_source_file_t *file = ak_source_file_from_path("src/main.c");
 * if (!file) {
 *     fprintf(stderr, "Failed to load file\n");
 * }
 * @endcode
 */
ak_source_file_t *ak_source_file_from_path(const char *path);

/**
 * @brief Increment reference count on source file
 *
 * Increases the reference count. Use when storing a source file pointer
 * in multiple places that need independent lifetime management.
 *
 * @param file Source file to retain
 * @return Same file pointer for convenience
 *
 * @threadsafe Yes
 *
 * Example:
 * @code
 * ak_source_file_t *copy = ak_source_file_retain(original);
 * @endcode
 */
ak_source_file_t *ak_source_file_retain(ak_source_file_t *file);

/**
 * @brief Decrement reference count and free if zero
 *
 * Decreases the reference count. When count reaches zero, frees the
 * source file and all associated memory.
 *
 * @param file Source file to release (NULL-safe)
 *
 * @threadsafe Yes
 *
 * @note Always call this when done with a source file
 * @note After release, pointer may become invalid
 *
 * Example:
 * @code
 * ak_source_file_release(file);
 * file = NULL;  // Good practice
 * @endcode
 */
void ak_source_file_release(ak_source_file_t *file);

/**
 * @brief Get source file contents
 *
 * Returns a pointer to the file's content buffer. The pointer remains
 * valid as long as the file is retained.
 *
 * @param file Source file
 * @return Pointer to file contents, or NULL if file is NULL
 *
 * @threadsafe Yes (read-only)
 *
 * Example:
 * @code
 * const char *text = ak_source_file_contents(file);
 * @endcode
 */
const char *ak_source_file_contents(ak_source_file_t *file);

/**
 * @brief Get source file length in bytes
 *
 * @param file Source file
 * @return Length of contents in bytes, or 0 if file is NULL
 *
 * @threadsafe Yes (read-only)
 */
size_t ak_source_file_length(ak_source_file_t *file);

/**
 * @brief Get source file name
 *
 * Returns the interned filename/path that was provided when the file
 * was created.
 *
 * @param file Source file
 * @return Interned filename string, or NULL if file is NULL
 *
 * @threadsafe Yes (read-only)
 *
 * @note Returned pointer is an interned string (permanent)
 */
const char *ak_source_file_name(ak_source_file_t *file);

/**
 * @brief Create a source location
 *
 * Creates a location with explicit line, column, and offset values.
 * All values are taken as-is without validation.
 *
 * @param file Source file
 * @param line Line number (1-based)
 * @param column Column number (1-based, UTF-8 code points)
 * @param offset Byte offset from start of file (0-based)
 * @return Source location
 *
 * @threadsafe Yes (read-only on file)
 *
 * Example:
 * @code
 * ak_source_loc_t loc = ak_source_loc_new(file, 10, 5, 142);
 * @endcode
 */
ak_source_loc_t ak_source_loc_new(ak_source_file_t *file, uint32_t line,
                                  uint32_t column, uint32_t offset);

/**
 * @brief Compute location from byte offset
 *
 * Converts a byte offset into line and column numbers using the cached
 * line start positions. This is an O(log n) operation due to binary search.
 *
 * @param file Source file
 * @param offset Byte offset from start of file (0-based)
 * @return Source location with computed line/column
 *
 * @threadsafe Yes (read-only)
 *
 * @note Line and column are computed; offset is stored as-is
 * @note UTF-8 code points are counted for column position
 * @note Returns location with line=0, column=0 if offset is out of bounds
 *
 * Example:
 * @code
 * ak_source_loc_t loc = ak_source_loc_from_offset(file, 150);
 * printf("Offset 150 is at line %u, column %u\n", loc.line, loc.column);
 * @endcode
 */
ak_source_loc_t ak_source_loc_from_offset(ak_source_file_t *file,
                                          uint32_t offset);

/**
 * @brief Create a source range
 *
 * Creates a range spanning from start to end location. Both locations
 * should be in the same file for meaningful results.
 *
 * @param start Start location (inclusive)
 * @param end End location (exclusive)
 * @return Source range
 *
 * @threadsafe Yes
 *
 * Example:
 * @code
 * ak_source_loc_t start = ak_source_loc_from_offset(file, 10);
 * ak_source_loc_t end = ak_source_loc_from_offset(file, 25);
 * ak_source_range_t range = ak_source_range_new(start, end);
 * @endcode
 */
ak_source_range_t ak_source_range_new(ak_source_loc_t start,
                                      ak_source_loc_t end);

/**
 * @brief Check if location is within range
 *
 * Tests if a location falls within the given range (inclusive of start,
 * exclusive of end).
 *
 * @param range Source range to check
 * @param loc Location to test
 * @return true if loc is in range, false otherwise
 *
 * @threadsafe Yes
 *
 * @note Compares using byte offsets
 * @note Returns false if locations are in different files
 *
 * Example:
 * @code
 * if (ak_source_range_contains(&range, loc)) {
 *     printf("Location is within range\n");
 * }
 * @endcode
 */
bool ak_source_range_contains(ak_source_range_t *range, ak_source_loc_t loc);

/**
 * @brief Check if two ranges overlap
 *
 * Tests if two ranges have any overlapping positions.
 *
 * @param a First range
 * @param b Second range
 * @return true if ranges overlap, false otherwise
 *
 * @threadsafe Yes
 *
 * @note Returns false if ranges are in different files
 * @note Empty ranges (start == end) can still overlap
 *
 * Example:
 * @code
 * if (ak_source_range_overlaps(&range1, &range2)) {
 *     printf("Ranges overlap\n");
 * }
 * @endcode
 */
bool ak_source_range_overlaps(ak_source_range_t *a, ak_source_range_t *b);

/**
 * @brief Extract source text for a range
 *
 * Allocates and returns a null-terminated string containing the source
 * text spanned by the range. Caller must free the returned string.
 *
 * @param range Source range to extract
 * @return Newly allocated string, or NULL on failure or invalid range
 *
 * @threadsafe Yes (read-only)
 *
 * @note Returns NULL if start/end are in different files
 * @note Returns NULL if offsets are out of bounds
 * @note Caller must free returned string with AK24_FREE
 *
 * Example:
 * @code
 * char *text = ak_source_extract(range);
 * if (text) {
 *     printf("Range contains: %s\n", text);
 *     AK24_FREE(text);
 * }
 * @endcode
 */
char *ak_source_extract(ak_source_range_t range);

/**
 * @brief Get a specific line's text
 *
 * Returns a pointer to the start of the specified line within the file.
 * The returned pointer is NOT null-terminated; use ak_source_get_line_len()
 * to get the length.
 *
 * @param file Source file
 * @param line Line number (1-based)
 * @return Pointer to line start, or NULL if line number is invalid
 *
 * @threadsafe Yes (read-only)
 *
 * @note Pointer is into file's content buffer (not a copy)
 * @note Line does NOT include the newline character
 * @note Returns NULL for line numbers beyond file length
 *
 * Example:
 * @code
 * const char *line_text = ak_source_get_line(file, 10);
 * size_t line_len = ak_source_get_line_len(file, 10);
 * printf("Line 10: %.*s\n", (int)line_len, line_text);
 * @endcode
 */
const char *ak_source_get_line(ak_source_file_t *file, uint32_t line);

/**
 * @brief Get the length of a specific line
 *
 * Returns the length of the specified line in bytes, excluding the
 * newline character(s).
 *
 * @param file Source file
 * @param line Line number (1-based)
 * @return Length of line in bytes, or 0 if line number is invalid
 *
 * @threadsafe Yes (read-only)
 *
 * Example:
 * @code
 * size_t len = ak_source_get_line_len(file, 5);
 * @endcode
 */
size_t ak_source_get_line_len(ak_source_file_t *file, uint32_t line);

/**
 * @brief Format a location as "file:line:col"
 *
 * Writes a formatted location string to the provided buffer in the
 * standard format "filename:line:column".
 *
 * @param loc Location to format
 * @param buf Buffer to write to
 * @param size Size of buffer in bytes
 * @return Number of characters written (excluding null terminator),
 *         or number that would have been written if buffer was large enough
 *
 * @threadsafe Yes
 *
 * @note Returns 0 if loc.file is NULL
 * @note Uses snprintf semantics for return value
 *
 * Example:
 * @code
 * char buf[256];
 * ak_source_loc_format(loc, buf, sizeof(buf));
 * printf("Error at %s\n", buf);  // "test.c:10:5"
 * @endcode
 */
int ak_source_loc_format(ak_source_loc_t loc, char *buf, size_t size);

/**
 * @brief Format a range as "file:line:col-line:col"
 *
 * Writes a formatted range string to the provided buffer. Format is
 * "filename:line:col-line:col" for multi-line ranges, or
 * "filename:line:col-col" for single-line ranges.
 *
 * @param range Range to format
 * @param buf Buffer to write to
 * @param size Size of buffer in bytes
 * @return Number of characters written (excluding null terminator),
 *         or number that would have been written if buffer was large enough
 *
 * @threadsafe Yes
 *
 * @note Returns 0 if range.start.file is NULL
 * @note Uses snprintf semantics for return value
 *
 * Example:
 * @code
 * char buf[256];
 * ak_source_range_format(range, buf, sizeof(buf));
 * printf("Error at %s\n", buf);  // "test.c:10:5-10:15"
 * @endcode
 */
int ak_source_range_format(ak_source_range_t range, char *buf, size_t size);

/**
 * @brief Get statistics about source file system
 *
 * Returns the number of currently loaded source files and total bytes
 * they occupy in memory.
 *
 * @param file_count Output: number of loaded files (may be NULL)
 * @param total_bytes Output: total memory in bytes (may be NULL)
 *
 * @threadsafe Yes
 *
 * Example:
 * @code
 * size_t files, bytes;
 * ak_sourceloc_stats(&files, &bytes);
 * printf("Loaded %zu files, %zu bytes\n", files, bytes);
 * @endcode
 */
void ak_sourceloc_stats(size_t *file_count, size_t *total_bytes);

#endif // AK24_SOURCELOC_H
