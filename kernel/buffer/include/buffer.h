/**
 * @file buffer.h
 * @brief Dynamic byte buffer with operations for manipulation and iteration
 *
 * Provides a resizable byte buffer with support for common operations including
 * rotation, trimming, splitting, and sub-buffer extraction. Designed for
 * efficient byte-level data manipulation in single-threaded contexts.
 *
 * Key features:
 * - Dynamic resizing with capacity management
 * - File loading support
 * - Rotation and trimming operations
 * - Buffer splitting and copying
 * - Iterator pattern for byte-level processing
 * - Sub-buffer extraction with offset tracking
 *
 * @note All functions are NOT thread-safe and require external synchronization
 */

#ifndef AK24_BUFFER_H
#define AK24_BUFFER_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Dynamic byte buffer structure
 *
 * Manages a resizable array of bytes with capacity tracking and origin offset
 * for sub-buffer operations.
 */
typedef struct ak_buffer_s {
  uint8_t *data;        /**< Pointer to byte data */
  size_t capacity;      /**< Total allocated capacity */
  size_t count;         /**< Current number of bytes */
  size_t origin_offset; /**< Offset from original buffer (for sub-buffers) */
} ak_buffer_t;

/**
 * @brief Unowned buffer pointer type
 *
 * Indicates a pointer to a buffer that the holder does not own and
 * should not free.
 */
typedef ak_buffer_t *ak_buffer_unowned_ptr_t;

/**
 * @brief Result of buffer split operation
 *
 * Contains two newly allocated buffers from a split operation.
 */
typedef struct {
  ak_buffer_t *left;  /**< Buffer containing bytes before split point */
  ak_buffer_t *right; /**< Buffer containing bytes after split point */
} split_buffer_t;

/**
 * @brief Iterator callback function type
 *
 * @param byte Pointer to current byte
 * @param idx Index of current byte
 * @param callback_data User data passed to iterator
 * @return 0 to continue iteration, non-zero to stop
 */
typedef int (*ak_buffer_iterator_fn)(uint8_t *byte, size_t idx,
                                     void *callback_data);

/**
 * @brief Create a new buffer with specified initial capacity
 *
 * Allocates a new buffer with the given initial size. The buffer will
 * automatically resize as needed during operations.
 *
 * @param initial_size Initial capacity in bytes
 * @return Pointer to new buffer, or NULL on allocation failure
 *
 * @notthreadsafe
 *
 * @note Caller must free with ak_buffer_free()
 *
 * @par Example:
 * @code
 * ak_buffer_t *buf = ak_buffer_new(1024);
 * if (!buf) {
 *   fprintf(stderr, "Failed to create buffer\n");
 *   return -1;
 * }
 * @endcode
 */
ak_buffer_t *ak_buffer_new(size_t initial_size);

/**
 * @brief Create buffer from file contents
 *
 * Reads entire file into a newly allocated buffer.
 *
 * @param filepath Path to file to read
 * @return Pointer to new buffer with file contents, or NULL on failure
 *
 * @notthreadsafe
 *
 * @note Caller must free with ak_buffer_free()
 */
ak_buffer_t *ak_buffer_from_file(const char *filepath);

/**
 * @brief Free a buffer
 *
 * Releases all memory associated with the buffer including internal data.
 *
 * @param buffer Buffer to free (NULL is safe)
 *
 * @notthreadsafe
 */
void ak_buffer_free(ak_buffer_t *buffer);

/**
 * @brief Copy bytes into buffer
 *
 * Copies data from source into buffer, resizing if necessary. Replaces
 * existing buffer contents.
 *
 * @param buffer Target buffer
 * @param src Source data to copy
 * @param len Number of bytes to copy
 * @return 0 on success, -1 on failure
 *
 * @notthreadsafe
 */
int ak_buffer_copy_to(ak_buffer_t *buffer, uint8_t *src, size_t len);

/**
 * @brief Get number of bytes in buffer
 *
 * @param buffer Buffer to query
 * @return Number of bytes, or 0 if buffer is NULL
 *
 * @notthreadsafe
 */
size_t ak_buffer_count(ak_buffer_t *buffer);

/**
 * @brief Get pointer to buffer data
 *
 * Returns direct pointer to internal byte array. Pointer may become
 * invalid after operations that resize the buffer.
 *
 * @param buffer Buffer to query
 * @return Pointer to data, or NULL if buffer is NULL
 *
 * @notthreadsafe
 *
 * @warning Pointer may be invalidated by resize operations
 */
uint8_t *ak_buffer_data(ak_buffer_t *buffer);

/**
 * @brief Clear all bytes from buffer
 *
 * Resets count to zero but maintains allocated capacity.
 *
 * @param buffer Buffer to clear
 *
 * @notthreadsafe
 */
void ak_buffer_clear(ak_buffer_t *buffer);

/**
 * @brief Reduce capacity to match current count
 *
 * Reallocates buffer to minimize memory usage.
 *
 * @param buffer Buffer to shrink
 * @return 0 on success, -1 on failure
 *
 * @notthreadsafe
 */
int ak_buffer_shrink_to_fit(ak_buffer_t *buffer);

/**
 * @brief Iterate over all bytes in buffer
 *
 * Calls callback function for each byte. Iteration stops if callback
 * returns non-zero.
 *
 * @param buffer Buffer to iterate
 * @param fn Callback function
 * @param callback_data User data passed to callback
 *
 * @notthreadsafe
 *
 * @par Example:
 * @code
 * int print_byte(uint8_t *byte, size_t idx, void *data) {
 *   printf("[%zu] = 0x%02x\n", idx, *byte);
 *   return 0;
 * }
 *
 * ak_buffer_for_each(buf, print_byte, NULL);
 * @endcode
 */
void ak_buffer_for_each(ak_buffer_t *buffer, ak_buffer_iterator_fn fn,
                        void *callback_data);

/**
 * @brief Extract sub-buffer from buffer
 *
 * Creates a new buffer containing a copy of bytes from the specified range.
 * The new buffer tracks its origin offset from the source.
 *
 * @param buffer Source buffer
 * @param offset Starting byte offset
 * @param length Number of bytes to extract
 * @param bytes_copied Output parameter for actual bytes copied
 * @return New buffer with extracted bytes, or NULL on failure
 *
 * @notthreadsafe
 *
 * @note Caller must free returned buffer with ak_buffer_free()
 */
ak_buffer_t *ak_buffer_sub_buffer(ak_buffer_t *buffer, size_t offset,
                                  size_t length, int *bytes_copied);

/**
 * @brief Rotate buffer contents left
 *
 * Moves first n bytes to the end of the buffer.
 *
 * @param buffer Buffer to rotate
 * @param n Number of positions to rotate
 *
 * @notthreadsafe
 */
void ak_buffer_rotate_left(ak_buffer_t *buffer, size_t n);

/**
 * @brief Rotate buffer contents right
 *
 * Moves last n bytes to the beginning of the buffer.
 *
 * @param buffer Buffer to rotate
 * @param n Number of positions to rotate
 *
 * @notthreadsafe
 */
void ak_buffer_rotate_right(ak_buffer_t *buffer, size_t n);

/**
 * @brief Trim matching bytes from left
 *
 * Removes all occurrences of the specified byte from the beginning.
 *
 * @param buffer Buffer to trim
 * @param byte Byte value to remove
 * @return Number of bytes removed
 *
 * @notthreadsafe
 */
int ak_buffer_trim_left(ak_buffer_t *buffer, uint8_t byte);

/**
 * @brief Trim matching bytes from right
 *
 * Removes all occurrences of the specified byte from the end.
 *
 * @param buffer Buffer to trim
 * @param byte Byte value to remove
 * @return Number of bytes removed
 *
 * @notthreadsafe
 */
int ak_buffer_trim_right(ak_buffer_t *buffer, uint8_t byte);

/**
 * @brief Create a deep copy of buffer
 *
 * Allocates a new buffer with identical contents.
 *
 * @param buffer Buffer to copy
 * @return New buffer with copied data, or NULL on failure
 *
 * @notthreadsafe
 *
 * @note Caller must free returned buffer with ak_buffer_free()
 */
ak_buffer_t *ak_buffer_copy(ak_buffer_t *buffer);

/**
 * @brief Split buffer into two new buffers
 *
 * Creates two new buffers by splitting at the specified index. The l and r
 * parameters control how many bytes from the split point are included in
 * each buffer.
 *
 * @param buffer Buffer to split
 * @param index Split point index
 * @param l Number of bytes before index to include in left buffer
 * @param r Number of bytes after index to include in right buffer
 * @return Structure containing left and right buffers
 *
 * @notthreadsafe
 *
 * @note Caller must free both buffers with ak_split_buffer_free()
 */
split_buffer_t ak_buffer_split(ak_buffer_t *buffer, size_t index, size_t l,
                               size_t r);

/**
 * @brief Free both buffers from split operation
 *
 * Convenience function to free both left and right buffers.
 *
 * @param split Split buffer structure to free
 *
 * @notthreadsafe
 */
void ak_split_buffer_free(split_buffer_t *split);

#endif
