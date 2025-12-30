/**
 * @file dmat.h
 * @brief Disk-backed matrix library for efficient random access to typed matrix
 * data
 *
 * The dmat module provides a disk-backed matrix storage system that allows
 * efficient random access to matrix elements without loading the entire dataset
 * into memory. It's particularly useful for large matrices such as image data
 * where elements are structured types (e.g., RGBA pixels).
 *
 * ## Features
 * - Direct disk I/O for random element access
 * - Support for any fixed-size element type
 * - Explicit flush control for performance
 * - Persistent binary format with magic number validation
 *
 * ## File Format
 * The on-disk format consists of:
 * - Header: 4-byte magic "DMAT", 4-byte width, 4-byte height, 4-byte
 * element_size
 * - Data: Row-major binary layout (width * height * element_size bytes)
 *
 * ## Thread Safety
 * @notthreadsafe Functions are not thread-safe. External synchronization
 * required for concurrent access.
 *
 * ## Example Usage
 * @code
 * // Define RGBA pixel structure
 * typedef struct {
 *     uint8_t r, g, b, a;
 * } rgba_t;
 *
 * // Create a new 1920x1080 RGBA matrix
 * dmat_ctx_t *matrix = dmat_new("/tmp/image.dmat", 1920, 1080, sizeof(rgba_t));
 *
 * // Set a pixel value
 * rgba_t pixel = {255, 128, 64, 255};
 * dmat_set(matrix, 100, 50, &pixel);
 *
 * // Read a pixel value
 * rgba_t read_pixel;
 * dmat_get(matrix, 100, 50, &read_pixel);
 *
 * // Ensure changes are written to disk
 * dmat_flush(matrix);
 *
 * // Clean up (auto-flushes)
 * dmat_free(matrix);
 * @endcode
 */

#ifndef AK24_DMAT_H
#define AK24_DMAT_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Opaque disk-backed matrix context
 *
 * This structure is intentionally opaque to prevent direct manipulation
 * of internal state. Use the provided API functions to interact with
 * the matrix.
 */
typedef struct dmat_ctx_s dmat_ctx_t;

/**
 * @brief Create a new disk-backed matrix or open an existing one
 *
 * If the file specified by filepath does not exist, creates a new matrix file
 * with the given dimensions and element size. If the file exists, opens it and
 * validates that the stored dimensions and element size match the provided
 * values.
 *
 * @param filepath Path to the matrix file as C string (non-NULL)
 * @param width Matrix width in elements (must be > 0)
 * @param height Matrix height in elements (must be > 0)
 * @param element_size Size of each matrix element in bytes (must be > 0)
 * @return New matrix context, or NULL on error (invalid params, I/O failure,
 * dimension mismatch)
 * @notthreadsafe
 */
dmat_ctx_t *dmat_new(const char *filepath, uint32_t width, uint32_t height,
                     uint32_t element_size);

/**
 * @brief Free a matrix context and close the underlying file
 *
 * Automatically flushes any pending writes before closing. Safe to call with
 * NULL.
 *
 * @param ctx Matrix context to free (may be NULL)
 * @notthreadsafe
 */
void dmat_free(dmat_ctx_t *ctx);

/**
 * @brief Read an element from the matrix
 *
 * Reads the element at position (x, y) from the matrix and copies it to
 * the provided output buffer. The buffer must be at least element_size bytes.
 *
 * @param ctx Matrix context (non-NULL)
 * @param x Column index (0-based, must be < width)
 * @param y Row index (0-based, must be < height)
 * @param out_buffer Output buffer for element data (non-NULL, >= element_size
 * bytes)
 * @return 0 on success, -1 on error (NULL params, out of bounds, I/O failure)
 * @notthreadsafe
 */
int dmat_get(dmat_ctx_t *ctx, uint32_t x, uint32_t y, void *out_buffer);

/**
 * @brief Write an element to the matrix
 *
 * Writes the provided data to position (x, y) in the matrix. The data buffer
 * must be at least element_size bytes. Changes are buffered until dmat_flush()
 * is called or the context is freed.
 *
 * @param ctx Matrix context (non-NULL)
 * @param x Column index (0-based, must be < width)
 * @param y Row index (0-based, must be < height)
 * @param data Input buffer containing element data (non-NULL, >= element_size
 * bytes)
 * @return 0 on success, -1 on error (NULL params, out of bounds, I/O failure)
 * @notthreadsafe
 */
int dmat_set(dmat_ctx_t *ctx, uint32_t x, uint32_t y, const void *data);

/**
 * @brief Flush pending writes to disk
 *
 * Forces all buffered writes to be committed to disk. This is automatically
 * called by dmat_free() but can be called explicitly for durability guarantees.
 *
 * @param ctx Matrix context (non-NULL)
 * @return 0 on success, -1 on error (NULL ctx, I/O failure)
 * @notthreadsafe
 */
int dmat_flush(dmat_ctx_t *ctx);

/**
 * @brief Get the width of the matrix
 *
 * @param ctx Matrix context (non-NULL)
 * @return Matrix width in elements, or 0 if ctx is NULL
 * @notthreadsafe
 */
uint32_t dmat_get_width(const dmat_ctx_t *ctx);

/**
 * @brief Get the height of the matrix
 *
 * @param ctx Matrix context (non-NULL)
 * @return Matrix height in elements, or 0 if ctx is NULL
 * @notthreadsafe
 */
uint32_t dmat_get_height(const dmat_ctx_t *ctx);

/**
 * @brief Get the element size of the matrix
 *
 * @param ctx Matrix context (non-NULL)
 * @return Element size in bytes, or 0 if ctx is NULL
 * @notthreadsafe
 */
uint32_t dmat_get_element_size(const dmat_ctx_t *ctx);

#endif /* AK24_DMAT_H */
