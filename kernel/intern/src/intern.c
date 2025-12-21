#include "intern.h"
#include "kernel.h"
#include <string.h>

// FNV-1a hash constants
#define FNV_OFFSET_BASIS 14695981039346656037ULL
#define FNV_PRIME 1099511628211ULL

// Initial hash table size (must be power of 2)
#define INITIAL_TABLE_SIZE 1024

/**
 * @brief Entry in the intern hash table
 */
typedef struct intern_entry_s {
  const char *str;             /**< Interned string */
  size_t len;                  /**< String length (not including null) */
  uint64_t hash;               /**< Hash of string contents */
  struct intern_entry_s *next; /**< Next entry in collision chain */
} intern_entry_t;

/**
 * @brief Global intern table state
 */
typedef struct {
  intern_entry_t **buckets; /**< Hash table buckets */
  size_t bucket_count;      /**< Number of buckets */
  size_t string_count;      /**< Number of unique strings */
  size_t total_bytes;       /**< Total bytes in interned strings */
  AK24_MUTEX lock;          /**< Thread safety lock */
  bool initialized;         /**< Whether system is initialized */
} intern_table_t;

static intern_table_t g_intern_table = {0};

/**
 * @brief Compute FNV-1a hash of string data
 */
static uint64_t hash_string(const char *str, size_t len) {
  uint64_t hash = FNV_OFFSET_BASIS;
  for (size_t i = 0; i < len; i++) {
    hash ^= (uint64_t)(unsigned char)str[i];
    hash *= FNV_PRIME;
  }
  return hash;
}

/**
 * @brief Find an entry in the table (must hold lock)
 */
static intern_entry_t *find_entry(const char *str, size_t len, uint64_t hash) {
  size_t bucket_idx = hash & (g_intern_table.bucket_count - 1);
  intern_entry_t *entry = g_intern_table.buckets[bucket_idx];

  while (entry) {
    if (entry->hash == hash && entry->len == len &&
        memcmp(entry->str, str, len) == 0) {
      return entry;
    }
    entry = entry->next;
  }

  return NULL;
}

/**
 * @brief Create a new entry and add to table (must hold lock)
 */
static intern_entry_t *create_entry(const char *str, size_t len,
                                    uint64_t hash) {
  // Allocate entry
  intern_entry_t *entry = AK24_ALLOC(sizeof(intern_entry_t));
  if (!entry) {
    return NULL;
  }

  // Allocate and copy string data (with null terminator)
  char *str_copy = AK24_ALLOC_ATOMIC(len + 1);
  if (!str_copy) {
    AK24_FREE(entry);
    return NULL;
  }
  memcpy(str_copy, str, len);
  str_copy[len] = '\0';

  entry->str = str_copy;
  entry->len = len;
  entry->hash = hash;
  entry->next = NULL;

  // Insert into hash table
  size_t bucket_idx = hash & (g_intern_table.bucket_count - 1);
  entry->next = g_intern_table.buckets[bucket_idx];
  g_intern_table.buckets[bucket_idx] = entry;

  // Update statistics
  g_intern_table.string_count++;
  g_intern_table.total_bytes += len + 1; // +1 for null terminator

  return entry;
}

void ak_intern_init(void) {
  if (g_intern_table.initialized) {
    return;
  }

  AK24_MUTEX_INIT(&g_intern_table.lock);

  // Allocate initial hash table
  g_intern_table.buckets =
      AK24_ALLOC(INITIAL_TABLE_SIZE * sizeof(intern_entry_t *));
  if (!g_intern_table.buckets) {
    AK24_MUTEX_DESTROY(&g_intern_table.lock);
    return;
  }

  // Zero out buckets
  memset(g_intern_table.buckets, 0,
         INITIAL_TABLE_SIZE * sizeof(intern_entry_t *));

  g_intern_table.bucket_count = INITIAL_TABLE_SIZE;
  g_intern_table.string_count = 0;
  g_intern_table.total_bytes = 0;
  g_intern_table.initialized = true;
}

void ak_intern_shutdown(void) {
  if (!g_intern_table.initialized) {
    return;
  }

  AK24_MUTEX_LOCK(&g_intern_table.lock);

  // Free all entries
  for (size_t i = 0; i < g_intern_table.bucket_count; i++) {
    intern_entry_t *entry = g_intern_table.buckets[i];
    while (entry) {
      intern_entry_t *next = entry->next;
      AK24_FREE((void *)entry->str); // Cast away const for free
      AK24_FREE(entry);
      entry = next;
    }
  }

  // Free bucket array
  AK24_FREE(g_intern_table.buckets);

  g_intern_table.buckets = NULL;
  g_intern_table.bucket_count = 0;
  g_intern_table.string_count = 0;
  g_intern_table.total_bytes = 0;

  AK24_MUTEX_UNLOCK(&g_intern_table.lock);
  AK24_MUTEX_DESTROY(&g_intern_table.lock);

  g_intern_table.initialized = false;
}

const char *ak_intern(const char *str) {
  if (!str) {
    return NULL;
  }

  return ak_intern_n(str, strlen(str));
}

const char *ak_intern_n(const char *str, size_t len) {
  if (!str) {
    return NULL;
  }

  if (!g_intern_table.initialized) {
    return NULL;
  }

  uint64_t hash = hash_string(str, len);

  AK24_MUTEX_LOCK(&g_intern_table.lock);

  // Check if string already exists
  intern_entry_t *existing = find_entry(str, len, hash);
  if (existing) {
    AK24_MUTEX_UNLOCK(&g_intern_table.lock);
    return existing->str;
  }

  // Create new entry
  intern_entry_t *new_entry = create_entry(str, len, hash);

  AK24_MUTEX_UNLOCK(&g_intern_table.lock);

  return new_entry ? new_entry->str : NULL;
}

void ak_intern_stats(size_t *count, size_t *bytes) {
  if (!g_intern_table.initialized) {
    if (count)
      *count = 0;
    if (bytes)
      *bytes = 0;
    return;
  }

  AK24_MUTEX_LOCK(&g_intern_table.lock);

  if (count) {
    *count = g_intern_table.string_count;
  }
  if (bytes) {
    *bytes = g_intern_table.total_bytes;
  }

  AK24_MUTEX_UNLOCK(&g_intern_table.lock);
}

void ak_intern_clear(void) {
  if (!g_intern_table.initialized) {
    return;
  }

  AK24_MUTEX_LOCK(&g_intern_table.lock);

  // Free all entries but keep the table structure
  for (size_t i = 0; i < g_intern_table.bucket_count; i++) {
    intern_entry_t *entry = g_intern_table.buckets[i];
    while (entry) {
      intern_entry_t *next = entry->next;
      AK24_FREE((void *)entry->str);
      AK24_FREE(entry);
      entry = next;
    }
    g_intern_table.buckets[i] = NULL;
  }

  g_intern_table.string_count = 0;
  g_intern_table.total_bytes = 0;

  AK24_MUTEX_UNLOCK(&g_intern_table.lock);
}
