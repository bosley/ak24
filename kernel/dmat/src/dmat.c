/**
 * @file dmat.c
 * @brief Implementation of disk-backed matrix library with LRU caching and
 * reader-writer locks
 */

#include "dmat.h"
#include "kernel.h"
#include <stdio.h>
#include <string.h>

/** Magic number for file format validation: "DMAT" */
#define DMAT_MAGIC 0x54414D44 /* "DMAT" in little-endian */

/** Default cache size in entries */
#define DMAT_DEFAULT_CACHE_SIZE 256

/** Default number of row stripes for concurrent writers */
#define DMAT_DEFAULT_ROW_STRIPES 16

/** File header structure (16 bytes total) */
typedef struct {
  uint32_t magic;        /**< Magic number for validation */
  uint32_t width;        /**< Matrix width in elements */
  uint32_t height;       /**< Matrix height in elements */
  uint32_t element_size; /**< Size of each element in bytes */
} dmat_header_t;

/** Cache entry for LRU cache */
typedef struct dmat_cache_entry_s {
  uint32_t x;                           /**< X coordinate */
  uint32_t y;                           /**< Y coordinate */
  void *data;                           /**< Cached element data */
  int dirty;                            /**< Whether entry needs write-back */
  struct dmat_cache_entry_s *prev;      /**< Previous in LRU list */
  struct dmat_cache_entry_s *next;      /**< Next in LRU list */
  struct dmat_cache_entry_s *hash_next; /**< Next in hash chain */
} dmat_cache_entry_t;

/** Cache structure with LRU eviction */
typedef struct {
  dmat_cache_entry_t **hash_table; /**< Hash table for O(1) lookup */
  dmat_cache_entry_t *lru_head;    /**< Most recently used */
  dmat_cache_entry_t *lru_tail;    /**< Least recently used */
  uint32_t capacity;               /**< Max cache entries */
  uint32_t size;                   /**< Current cache entries */
  uint32_t hash_size;              /**< Hash table size */
} dmat_cache_t;

/** Disk-backed matrix context - opaque to users */
struct dmat_ctx_s {
  FILE *file;             /**< File handle for matrix data */
  uint32_t width;         /**< Matrix width in elements */
  uint32_t height;        /**< Matrix height in elements */
  uint32_t element_size;  /**< Size of each element in bytes */
  size_t data_offset;     /**< Offset to start of matrix data */
  dmat_cache_t *cache;    /**< LRU cache */
  AK24_MUTEX cache_mutex; /**< Mutex for cache operations */
  AK24_COND cache_cond;   /**< Condition variable for cache */
  int active_readers;     /**< Number of active readers */
  int waiting_writers;    /**< Number of waiting writers */
  uint32_t num_stripes;   /**< Number of row stripes */
  AK24_MUTEX *row_locks;  /**< Array of mutexes, one per row stripe */
};

/**
 * @brief Calculate file offset for a given (x, y) position
 */
static inline size_t dmat_calculate_offset(const dmat_ctx_t *ctx, uint32_t x,
                                           uint32_t y) {
  return ctx->data_offset + ((size_t)y * ctx->width + x) * ctx->element_size;
}

/**
 * @brief Hash function for cache lookup
 */
static inline uint32_t dmat_hash_coords(uint32_t x, uint32_t y,
                                        uint32_t hash_size) {
  uint32_t hash = x * 2654435761U + y * 2246822519U;
  return hash % hash_size;
}

/**
 * @brief Calculate which stripe a row belongs to
 */
static inline uint32_t dmat_row_to_stripe(const dmat_ctx_t *ctx, uint32_t y) {
  return y % ctx->num_stripes;
}

/**
 * @brief Acquire cache read lock (for cache lookups)
 */
static void dmat_cache_read_lock(dmat_ctx_t *ctx) {
  AK24_MUTEX_LOCK(&ctx->cache_mutex);
  while (ctx->waiting_writers > 0) {
    AK24_COND_WAIT(&ctx->cache_cond, &ctx->cache_mutex);
  }
  ctx->active_readers++;
  AK24_MUTEX_UNLOCK(&ctx->cache_mutex);
}

/**
 * @brief Release cache read lock
 */
static void dmat_cache_read_unlock(dmat_ctx_t *ctx) {
  AK24_MUTEX_LOCK(&ctx->cache_mutex);
  ctx->active_readers--;
  if (ctx->active_readers == 0) {
    AK24_COND_BROADCAST(&ctx->cache_cond);
  }
  AK24_MUTEX_UNLOCK(&ctx->cache_mutex);
}

/**
 * @brief Acquire cache write lock (for cache modifications)
 * Keeps the cache_mutex locked - caller must call dmat_cache_write_unlock
 */
static void dmat_cache_write_lock(dmat_ctx_t *ctx) {
  AK24_MUTEX_LOCK(&ctx->cache_mutex);
  ctx->waiting_writers++;
  while (ctx->active_readers > 0) {
    AK24_COND_WAIT(&ctx->cache_cond, &ctx->cache_mutex);
  }
  ctx->waiting_writers--;
  // Keep mutex locked - caller must unlock
}

/**
 * @brief Release cache write lock
 */
static void dmat_cache_write_unlock(dmat_ctx_t *ctx) {
  AK24_COND_BROADCAST(&ctx->cache_cond);
  AK24_MUTEX_UNLOCK(&ctx->cache_mutex);
}

/**
 * @brief Acquire row stripe lock (for writing to specific rows)
 */
static void dmat_row_lock(dmat_ctx_t *ctx, uint32_t y) {
  uint32_t stripe = dmat_row_to_stripe(ctx, y);
  AK24_MUTEX_LOCK(&ctx->row_locks[stripe]);
}

/**
 * @brief Release row stripe lock
 */
static void dmat_row_unlock(dmat_ctx_t *ctx, uint32_t y) {
  uint32_t stripe = dmat_row_to_stripe(ctx, y);
  AK24_MUTEX_UNLOCK(&ctx->row_locks[stripe]);
}

/**
 * @brief Remove entry from LRU list
 */
static void dmat_cache_remove_from_lru(dmat_cache_t *cache,
                                       dmat_cache_entry_t *entry) {
  if (entry->prev) {
    entry->prev->next = entry->next;
  } else {
    cache->lru_head = entry->next;
  }

  if (entry->next) {
    entry->next->prev = entry->prev;
  } else {
    cache->lru_tail = entry->prev;
  }

  entry->prev = NULL;
  entry->next = NULL;
}

/**
 * @brief Add entry to front of LRU list (most recently used)
 */
static void dmat_cache_add_to_lru_front(dmat_cache_t *cache,
                                        dmat_cache_entry_t *entry) {
  entry->prev = NULL;
  entry->next = cache->lru_head;

  if (cache->lru_head) {
    cache->lru_head->prev = entry;
  } else {
    cache->lru_tail = entry;
  }

  cache->lru_head = entry;
}

/**
 * @brief Move entry to front of LRU list
 */
static void dmat_cache_move_to_front(dmat_cache_t *cache,
                                     dmat_cache_entry_t *entry) {
  if (cache->lru_head == entry) {
    return; // Already at front
  }

  dmat_cache_remove_from_lru(cache, entry);
  dmat_cache_add_to_lru_front(cache, entry);
}

/**
 * @brief Write dirty cache entry to disk
 */
static int dmat_cache_writeback(dmat_ctx_t *ctx, dmat_cache_entry_t *entry) {
  if (!entry->dirty) {
    return 0;
  }

  size_t offset = dmat_calculate_offset(ctx, entry->x, entry->y);

  if (fseek(ctx->file, offset, SEEK_SET) != 0) {
    return -1;
  }

  if (fwrite(entry->data, ctx->element_size, 1, ctx->file) != 1) {
    return -1;
  }

  entry->dirty = 0;
  return 0;
}

/**
 * @brief Find cache entry by coordinates
 */
static dmat_cache_entry_t *dmat_cache_find(dmat_cache_t *cache, uint32_t x,
                                           uint32_t y) {
  uint32_t hash = dmat_hash_coords(x, y, cache->hash_size);
  dmat_cache_entry_t *entry = cache->hash_table[hash];

  while (entry) {
    if (entry->x == x && entry->y == y) {
      return entry;
    }
    entry = entry->hash_next;
  }

  return NULL;
}

/**
 * @brief Remove entry from hash table
 */
static void dmat_cache_remove_from_hash(dmat_cache_t *cache,
                                        dmat_cache_entry_t *entry) {
  uint32_t hash = dmat_hash_coords(entry->x, entry->y, cache->hash_size);
  dmat_cache_entry_t **ptr = &cache->hash_table[hash];

  while (*ptr) {
    if (*ptr == entry) {
      *ptr = entry->hash_next;
      entry->hash_next = NULL;
      return;
    }
    ptr = &(*ptr)->hash_next;
  }
}

/**
 * @brief Add entry to hash table
 */
static void dmat_cache_add_to_hash(dmat_cache_t *cache,
                                   dmat_cache_entry_t *entry) {
  uint32_t hash = dmat_hash_coords(entry->x, entry->y, cache->hash_size);
  entry->hash_next = cache->hash_table[hash];
  cache->hash_table[hash] = entry;
}

/**
 * @brief Evict least recently used entry
 */
static int dmat_cache_evict_lru(dmat_ctx_t *ctx) {
  dmat_cache_t *cache = ctx->cache;

  if (!cache->lru_tail) {
    return -1; // Cache is empty
  }

  dmat_cache_entry_t *entry = cache->lru_tail;

  // Write back if dirty
  if (dmat_cache_writeback(ctx, entry) != 0) {
    return -1;
  }

  // Remove from LRU list and hash table
  dmat_cache_remove_from_lru(cache, entry);
  dmat_cache_remove_from_hash(cache, entry);

  // Free entry
  AK24_FREE(entry->data);
  AK24_FREE(entry);

  cache->size--;
  return 0;
}

/**
 * @brief Create a new cache entry
 */
static dmat_cache_entry_t *dmat_cache_create_entry(dmat_ctx_t *ctx, uint32_t x,
                                                   uint32_t y,
                                                   const void *data) {
  dmat_cache_entry_t *entry = AK24_ALLOC(sizeof(dmat_cache_entry_t));
  if (!entry) {
    return NULL;
  }

  entry->data = AK24_ALLOC_ATOMIC(ctx->element_size);
  if (!entry->data) {
    AK24_FREE(entry);
    return NULL;
  }

  entry->x = x;
  entry->y = y;
  entry->dirty = 0;
  entry->prev = NULL;
  entry->next = NULL;
  entry->hash_next = NULL;

  if (data) {
    memcpy(entry->data, data, ctx->element_size);
  }

  return entry;
}

/**
 * @brief Insert or update cache entry
 */
static int dmat_cache_put(dmat_ctx_t *ctx, uint32_t x, uint32_t y,
                          const void *data, int dirty) {
  dmat_cache_t *cache = ctx->cache;

  // Check if entry already exists
  dmat_cache_entry_t *entry = dmat_cache_find(cache, x, y);

  if (entry) {
    // Update existing entry
    memcpy(entry->data, data, ctx->element_size);
    entry->dirty = dirty;
    dmat_cache_move_to_front(cache, entry);
    return 0;
  }

  // Evict LRU if cache is full
  if (cache->size >= cache->capacity) {
    if (dmat_cache_evict_lru(ctx) != 0) {
      return -1;
    }
  }

  // Create new entry
  entry = dmat_cache_create_entry(ctx, x, y, data);
  if (!entry) {
    return -1;
  }

  entry->dirty = dirty;

  // Add to LRU front and hash table
  dmat_cache_add_to_lru_front(cache, entry);
  dmat_cache_add_to_hash(cache, entry);

  cache->size++;
  return 0;
}

/**
 * @brief Initialize cache
 */
static dmat_cache_t *dmat_cache_init(uint32_t capacity) {
  dmat_cache_t *cache = AK24_ALLOC(sizeof(dmat_cache_t));
  if (!cache) {
    return NULL;
  }

  // Use prime number for hash table size
  cache->hash_size = capacity * 2 + 1;
  cache->hash_table =
      AK24_ALLOC_ATOMIC(sizeof(dmat_cache_entry_t *) * cache->hash_size);
  if (!cache->hash_table) {
    AK24_FREE(cache);
    return NULL;
  }

  memset(cache->hash_table, 0, sizeof(dmat_cache_entry_t *) * cache->hash_size);

  cache->capacity = capacity;
  cache->size = 0;
  cache->lru_head = NULL;
  cache->lru_tail = NULL;

  return cache;
}

/**
 * @brief Flush all dirty cache entries
 */
static int dmat_cache_flush_all(dmat_ctx_t *ctx) {
  dmat_cache_t *cache = ctx->cache;
  dmat_cache_entry_t *entry = cache->lru_head;

  while (entry) {
    if (dmat_cache_writeback(ctx, entry) != 0) {
      return -1;
    }
    entry = entry->next;
  }

  return 0;
}

/**
 * @brief Destroy cache and free all entries
 */
static void dmat_cache_destroy(dmat_cache_t *cache) {
  if (!cache) {
    return;
  }

  // Free all entries
  dmat_cache_entry_t *entry = cache->lru_head;
  while (entry) {
    dmat_cache_entry_t *next = entry->next;
    AK24_FREE(entry->data);
    AK24_FREE(entry);
    entry = next;
  }

  AK24_FREE(cache->hash_table);
  AK24_FREE(cache);
}

/**
 * @brief Write header to a new matrix file
 */
static int dmat_write_header(FILE *file, uint32_t width, uint32_t height,
                             uint32_t element_size) {
  dmat_header_t header = {.magic = DMAT_MAGIC,
                          .width = width,
                          .height = height,
                          .element_size = element_size};

  if (fseek(file, 0, SEEK_SET) != 0) {
    return -1;
  }

  if (fwrite(&header, sizeof(header), 1, file) != 1) {
    return -1;
  }

  if (fflush(file) != 0) {
    return -1;
  }

  return 0;
}

/**
 * @brief Read and validate header from an existing matrix file
 */
static int dmat_read_header(FILE *file, uint32_t expected_width,
                            uint32_t expected_height,
                            uint32_t expected_element_size) {
  dmat_header_t header;

  if (fseek(file, 0, SEEK_SET) != 0) {
    return -1;
  }

  if (fread(&header, sizeof(header), 1, file) != 1) {
    return -1;
  }

  if (header.magic != DMAT_MAGIC) {
    return -1;
  }

  if (header.width != expected_width || header.height != expected_height ||
      header.element_size != expected_element_size) {
    return -1;
  }

  return 0;
}

/**
 * @brief Initialize matrix data area with zeros for new files
 */
static int dmat_initialize_data(FILE *file, uint32_t width, uint32_t height,
                                uint32_t element_size) {
  size_t data_size = (size_t)width * height * element_size;
  size_t chunk_size = 4096;
  char *zero_buffer = AK24_ALLOC_ATOMIC(chunk_size);

  if (!zero_buffer) {
    return -1;
  }
  if (fseek(file, sizeof(dmat_header_t), SEEK_SET) != 0) {
    AK24_FREE(zero_buffer);
    return -1;
  }

  size_t remaining = data_size;
  while (remaining > 0) {
    size_t to_write = remaining < chunk_size ? remaining : chunk_size;
    if (fwrite(zero_buffer, 1, to_write, file) != to_write) {
      AK24_FREE(zero_buffer);
      return -1;
    }
    remaining -= to_write;
  }

  AK24_FREE(zero_buffer);

  if (fflush(file) != 0) {
    return -1;
  }

  return 0;
}

dmat_ctx_t *dmat_new(const char *filepath, uint32_t width, uint32_t height,
                     uint32_t element_size) {
  if (!filepath || width == 0 || height == 0 || element_size == 0) {
    return NULL;
  }

  size_t total_elements = (size_t)width * height;
  if (total_elements / height != width) {
    return NULL;
  }

  size_t data_size = total_elements * element_size;
  if (data_size / element_size != total_elements) {
    return NULL;
  }

  FILE *file = fopen(filepath, "r+b");
  int is_new_file = 0;

  if (!file) {
    file = fopen(filepath, "w+b");
    if (!file) {
      return NULL;
    }
    is_new_file = 1;
  }

  if (is_new_file) {
    if (dmat_write_header(file, width, height, element_size) != 0) {
      fclose(file);
      return NULL;
    }

    if (dmat_initialize_data(file, width, height, element_size) != 0) {
      fclose(file);
      return NULL;
    }
  } else {
    if (dmat_read_header(file, width, height, element_size) != 0) {
      fclose(file);
      return NULL;
    }
  }

  dmat_ctx_t *ctx = AK24_ALLOC(sizeof(dmat_ctx_t));
  if (!ctx) {
    fclose(file);
    return NULL;
  }

  ctx->file = file;
  ctx->width = width;
  ctx->height = height;
  ctx->element_size = element_size;
  ctx->data_offset = sizeof(dmat_header_t);

  // Initialize cache
  ctx->cache = dmat_cache_init(DMAT_DEFAULT_CACHE_SIZE);
  if (!ctx->cache) {
    fclose(file);
    AK24_FREE(ctx);
    return NULL;
  }

  // Initialize cache mutex and condition variable
  if (AK24_MUTEX_INIT(&ctx->cache_mutex) != 0) {
    dmat_cache_destroy(ctx->cache);
    fclose(file);
    AK24_FREE(ctx);
    return NULL;
  }

  if (AK24_COND_INIT(&ctx->cache_cond) != 0) {
    AK24_MUTEX_DESTROY(&ctx->cache_mutex);
    dmat_cache_destroy(ctx->cache);
    fclose(file);
    AK24_FREE(ctx);
    return NULL;
  }

  ctx->active_readers = 0;
  ctx->waiting_writers = 0;

  // Initialize row stripe locks
  ctx->num_stripes = DMAT_DEFAULT_ROW_STRIPES;
  ctx->row_locks = AK24_ALLOC_ATOMIC(sizeof(AK24_MUTEX) * ctx->num_stripes);
  if (!ctx->row_locks) {
    AK24_COND_DESTROY(&ctx->cache_cond);
    AK24_MUTEX_DESTROY(&ctx->cache_mutex);
    dmat_cache_destroy(ctx->cache);
    fclose(file);
    AK24_FREE(ctx);
    return NULL;
  }

  for (uint32_t i = 0; i < ctx->num_stripes; i++) {
    if (AK24_MUTEX_INIT(&ctx->row_locks[i]) != 0) {
      // Clean up already initialized mutexes
      for (uint32_t j = 0; j < i; j++) {
        AK24_MUTEX_DESTROY(&ctx->row_locks[j]);
      }
      AK24_FREE(ctx->row_locks);
      AK24_COND_DESTROY(&ctx->cache_cond);
      AK24_MUTEX_DESTROY(&ctx->cache_mutex);
      dmat_cache_destroy(ctx->cache);
      fclose(file);
      AK24_FREE(ctx);
      return NULL;
    }
  }

  return ctx;
}

void dmat_free(dmat_ctx_t *ctx) {
  if (!ctx) {
    return;
  }

  // Flush all dirty cache entries if cache exists
  if (ctx->cache) {
    dmat_cache_write_lock(ctx);
    dmat_cache_flush_all(ctx);
    dmat_cache_write_unlock(ctx);

    dmat_cache_destroy(ctx->cache);

    // Destroy cache synchronization primitives
    AK24_COND_DESTROY(&ctx->cache_cond);
    AK24_MUTEX_DESTROY(&ctx->cache_mutex);
  }

  // Destroy row stripe locks
  if (ctx->row_locks) {
    for (uint32_t i = 0; i < ctx->num_stripes; i++) {
      AK24_MUTEX_DESTROY(&ctx->row_locks[i]);
    }
    AK24_FREE(ctx->row_locks);
  }

  if (ctx->file) {
    fflush(ctx->file);
    fclose(ctx->file);
  }

  AK24_FREE(ctx);
}

int dmat_get(dmat_ctx_t *ctx, uint32_t x, uint32_t y, void *out_buffer) {
  if (!ctx || !out_buffer) {
    return -1;
  }

  if (x >= ctx->width || y >= ctx->height) {
    return -1;
  }

  // Try cache lookup with read lock (no cache modification)
  dmat_cache_read_lock(ctx);

  dmat_cache_entry_t *entry = dmat_cache_find(ctx->cache, x, y);

  if (entry) {
    // Cache hit - copy data without modifying cache
    // (Skip LRU update to maintain true read-only access)
    memcpy(out_buffer, entry->data, ctx->element_size);
    dmat_cache_read_unlock(ctx);
    return 0;
  }

  dmat_cache_read_unlock(ctx);

  // Cache miss - need cache write lock and row lock
  dmat_cache_write_lock(ctx);
  dmat_row_lock(ctx, y);

  // Double-check cache (another thread might have loaded it)
  entry = dmat_cache_find(ctx->cache, x, y);
  if (entry) {
    memcpy(out_buffer, entry->data, ctx->element_size);
    dmat_cache_move_to_front(ctx->cache, entry);
    dmat_row_unlock(ctx, y);
    dmat_cache_write_unlock(ctx);
    return 0;
  }

  // Read from disk
  size_t offset = dmat_calculate_offset(ctx, x, y);

  if (fseek(ctx->file, offset, SEEK_SET) != 0) {
    dmat_row_unlock(ctx, y);
    dmat_cache_write_unlock(ctx);
    return -1;
  }

  if (fread(out_buffer, ctx->element_size, 1, ctx->file) != 1) {
    dmat_row_unlock(ctx, y);
    dmat_cache_write_unlock(ctx);
    return -1;
  }

  // Add to cache (not dirty since we just read it)
  dmat_cache_put(ctx, x, y, out_buffer, 0);

  dmat_row_unlock(ctx, y);
  dmat_cache_write_unlock(ctx);
  return 0;
}

int dmat_set(dmat_ctx_t *ctx, uint32_t x, uint32_t y, const void *data) {
  if (!ctx || !data) {
    return -1;
  }

  if (x >= ctx->width || y >= ctx->height) {
    return -1;
  }

  // Lock both cache (for modification) and row stripe (for this row)
  dmat_cache_write_lock(ctx);
  dmat_row_lock(ctx, y);

  // Update cache with dirty flag
  int result = dmat_cache_put(ctx, x, y, data, 1);

  dmat_row_unlock(ctx, y);
  dmat_cache_write_unlock(ctx);

  return result;
}

int dmat_flush(dmat_ctx_t *ctx) {
  if (!ctx || !ctx->file) {
    return -1;
  }

  // Acquire cache write lock and all row locks
  dmat_cache_write_lock(ctx);
  for (uint32_t i = 0; i < ctx->num_stripes; i++) {
    AK24_MUTEX_LOCK(&ctx->row_locks[i]);
  }

  // Flush all dirty cache entries
  int result = dmat_cache_flush_all(ctx);

  if (result == 0) {
    // Flush file buffer
    if (fflush(ctx->file) != 0) {
      result = -1;
    }
  }

  // Release all locks
  for (uint32_t i = 0; i < ctx->num_stripes; i++) {
    AK24_MUTEX_UNLOCK(&ctx->row_locks[i]);
  }
  dmat_cache_write_unlock(ctx);

  return result;
}

uint32_t dmat_get_width(const dmat_ctx_t *ctx) { return ctx ? ctx->width : 0; }

uint32_t dmat_get_height(const dmat_ctx_t *ctx) {
  return ctx ? ctx->height : 0;
}

uint32_t dmat_get_element_size(const dmat_ctx_t *ctx) {
  return ctx ? ctx->element_size : 0;
}
