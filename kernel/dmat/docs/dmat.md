# dmat - Disk-Backed Matrix Library

Disk-backed matrix storage for large typed matrices (e.g., RGBA image data). Random access without loading entire dataset into memory.

## Features

- Random access to any element via disk I/O
- Type-generic: any fixed-size element type
- LRU cache (256 entries default) for frequently accessed elements
- Thread-safe: reader-writer locks + row stripe locks (16 stripes default)
- Persistent binary format with validation
- Row-major layout

## File Format

**Header (16 bytes):**
- Magic: "DMAT" (0x54414D44)
- Width, Height, Element Size (uint32_t each)

**Data:** Row-major binary layout, offset = `header_size + (y * width + x) * element_size`

## API

```c
typedef struct dmat_ctx_s dmat_ctx_t;

// Create new or open existing. Returns NULL on error.
// Creates file if not exists, validates dimensions if exists.
dmat_ctx_t *dmat_new(const char *filepath, uint32_t width, uint32_t height, uint32_t element_size);

// Free context, auto-flushes. NULL-safe.
void dmat_free(dmat_ctx_t *ctx);

// Read/write element at (x, y). Returns 0 on success, -1 on error.
// Buffer must be >= element_size bytes.
int dmat_get(dmat_ctx_t *ctx, uint32_t x, uint32_t y, void *out_buffer);
int dmat_set(dmat_ctx_t *ctx, uint32_t x, uint32_t y, const void *data);

// Flush dirty cache entries to disk. Returns 0 on success, -1 on error.
int dmat_flush(dmat_ctx_t *ctx);

// Query dimensions. Returns 0 if ctx is NULL.
uint32_t dmat_get_width(const dmat_ctx_t *ctx);
uint32_t dmat_get_height(const dmat_ctx_t *ctx);
uint32_t dmat_get_element_size(const dmat_ctx_t *ctx);
```

## Examples

### RGBA Image (1920x1080)

```c
#include "dmat.h"
#include <stdint.h>

typedef struct { uint8_t r, g, b, a; } rgba_t;

dmat_ctx_t *img = dmat_new("/tmp/img.dmat", 1920, 1080, sizeof(rgba_t));
rgba_t red = {255, 0, 0, 255};
dmat_set(img, 100, 500, &red);

rgba_t pixel;
dmat_get(img, 100, 500, &pixel);
dmat_flush(img);  // Optional - auto-flushes on dmat_free
dmat_free(img);
```

### Persistence

```c
// Write
dmat_ctx_t *m = dmat_new("/data/vals.dmat", 100, 100, sizeof(double));
double val = 42.5;
dmat_set(m, 10, 20, &val);
dmat_free(m);  // Flushes to disk

// Later: Read back
m = dmat_new("/data/vals.dmat", 100, 100, sizeof(double));
dmat_get(m, 10, 20, &val);  // val == 42.5
dmat_free(m);
```

### Custom Types

```c
typedef struct { int32_t temp; uint32_t timestamp; } sensor_t;

dmat_ctx_t *s = dmat_new("/data/sensors.dmat", 64, 64, sizeof(sensor_t));
sensor_t reading = {.temp = 2500, .timestamp = time(NULL)};
dmat_set(s, 10, 20, &reading);
dmat_free(s);
```

## Performance

**Caching:** 256-entry LRU cache. Repeated access to same elements is fast.

**Threading:** Multiple readers concurrent. Writers use row stripe locks (16 stripes) - parallel writes to different row stripes don't contend.

**Flushing:** Dirty entries written on `dmat_flush()` or `dmat_free()`. Batch writes before flushing.

**Memory:** ~200 bytes overhead per context + cache entries (256 * element_size max).

## Error Handling

- `dmat_new()` returns NULL on: invalid params, I/O failure, dimension mismatch
- `dmat_get/set/flush()` return -1 on: NULL ctx, out of bounds, I/O failure
- `dmat_free()` and getters are NULL-safe

## Thread Safety

**Thread-safe.** All operations can be called from multiple threads concurrently without external synchronization.

**Implementation:** Reader-writer locks for cache access. Row stripe locks (16 stripes) for write operations enable high parallelism.
