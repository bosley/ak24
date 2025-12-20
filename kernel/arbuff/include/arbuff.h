/**
 * @file arbuff.h
 * @brief Lock-free atomic ring buffer for multi-producer multi-consumer
 * scenarios
 *
 * This module provides a thread-safe, lock-free MPMC (multi-producer
 * multi-consumer) circular buffer implementation using C11 atomics. The
 * implementation is based on Dmitry Vyukov's bounded MPMC queue algorithm with
 * sequence-based synchronization.
 *
 * Key features:
 * - Lock-free atomic operations for concurrent access
 * - Wait-free bounded operations under contention
 * - ABA problem resistant through sequence numbers
 * - Power-of-2 capacity for efficient modulo operations
 * - Proper memory ordering (acquire/release semantics)
 *
 * @note All functions are thread-safe unless explicitly noted otherwise
 * @see
 * https://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
 */

#ifndef AK24_ARBUFF_H
#define AK24_ARBUFF_H

#include <stdatomic.h>
#include <stddef.h>

#define AK24_ARBUFF_VERSION "0.1.0"

/**
 * @brief Internal slot structure for the ring buffer
 *
 * Each slot contains a value pointer and a sequence number used for
 * synchronization between producers and consumers.
 */
typedef struct {
  _Atomic(void *) value;   /**< Atomic pointer to stored data */
  _Atomic size_t sequence; /**< Sequence number for synchronization */
} ak_arbuff_slot_t;

/**
 * @brief Atomic ring buffer structure
 *
 * A fixed-size circular buffer that supports lock-free concurrent access
 * from multiple producers and consumers. The capacity is automatically
 * rounded up to the next power of 2 for efficient indexing.
 */
typedef struct ak_arbuff_t {
  ak_arbuff_slot_t *slots; /**< Array of buffer slots */
  size_t capacity;         /**< Total capacity (power of 2) */
  size_t mask;             /**< Bitmask for fast modulo (capacity - 1) */
  _Atomic size_t head;     /**< Producer position */
  _Atomic size_t tail;     /**< Consumer position */
} ak_arbuff_t;

/**
 * @brief Create a new atomic ring buffer
 *
 * Allocates and initializes a new ring buffer with the specified capacity.
 * The actual capacity will be rounded up to the next power of 2.
 *
 * @param capacity Desired capacity (must be > 0)
 * @return Pointer to new buffer, or NULL on allocation failure or invalid
 * capacity
 *
 * @threadsafe
 * @lockfree
 *
 * @par Example:
 * @code
 * ak_arbuff_t *buf = ak_arbuff_new(100);
 * if (!buf) {
 *   fprintf(stderr, "Failed to create buffer\n");
 *   return -1;
 * }
 * @endcode
 */
ak_arbuff_t *ak_arbuff_new(size_t capacity);

/**
 * @brief Free an atomic ring buffer
 *
 * Releases all memory associated with the buffer. Does not free the items
 * stored in the buffer - caller is responsible for managing item lifetimes.
 *
 * @param arbuff Buffer to free (NULL is safe)
 *
 * @warning Ensure no concurrent operations are in progress before calling
 * @notthreadsafe
 */
void ak_arbuff_free(ak_arbuff_t *arbuff);

/**
 * @brief Push an item onto the buffer
 *
 * Attempts to add an item to the buffer in a thread-safe manner. If the
 * buffer is full, the operation fails immediately without blocking.
 *
 * @param arbuff Buffer to push to
 * @param item Pointer to item to store (must not be NULL)
 * @return 0 on success, -1 if buffer is NULL or full
 *
 * @threadsafe
 * @lockfree
 * @waitfree
 *
 * @par Example:
 * @code
 * int *data = malloc(sizeof(int));
 * *data = 42;
 * if (ak_arbuff_push(buf, data) != 0) {
 *   fprintf(stderr, "Buffer full\n");
 *   free(data);
 * }
 * @endcode
 */
int ak_arbuff_push(ak_arbuff_t *arbuff, void *item);

/**
 * @brief Pop an item from the buffer
 *
 * Removes and returns the oldest item from the buffer in a thread-safe manner.
 * If the buffer is empty, returns NULL immediately without blocking.
 *
 * @param arbuff Buffer to pop from
 * @return Pointer to item, or NULL if buffer is NULL or empty
 *
 * @threadsafe
 * @lockfree
 * @waitfree
 *
 * @par Example:
 * @code
 * int *data = (int *)ak_arbuff_pop(buf);
 * if (data) {
 *   printf("Got: %d\n", *data);
 *   free(data);
 * }
 * @endcode
 */
void *ak_arbuff_pop(ak_arbuff_t *arbuff);

/**
 * @brief Peek at the next item without removing it
 *
 * Returns the oldest item in the buffer without removing it. The item
 * may be consumed by another thread immediately after this call returns.
 *
 * @param arbuff Buffer to peek into
 * @return Pointer to item, or NULL if buffer is NULL or empty
 *
 * @threadsafe
 * @lockfree
 *
 * @note The returned pointer may become invalid if another thread pops the item
 */
void *ak_arbuff_peek(ak_arbuff_t *arbuff);

/**
 * @brief Get approximate count of items in buffer
 *
 * Returns a snapshot of the current item count. Due to concurrent operations,
 * this value may be stale by the time it's used.
 *
 * @param arbuff Buffer to query
 * @return Approximate number of items, or 0 if buffer is NULL
 *
 * @threadsafe
 * @lockfree
 *
 * @note Result may be stale in concurrent scenarios
 */
size_t ak_arbuff_count(ak_arbuff_t *arbuff);

/**
 * @brief Get the total capacity of the buffer
 *
 * Returns the maximum number of items the buffer can hold. This value
 * is constant after creation and may be larger than requested due to
 * power-of-2 rounding.
 *
 * @param arbuff Buffer to query
 * @return Buffer capacity, or 0 if buffer is NULL
 *
 * @threadsafe
 */
size_t ak_arbuff_capacity(ak_arbuff_t *arbuff);

/**
 * @brief Check if buffer is empty
 *
 * Returns a snapshot indicating whether the buffer is empty. Due to
 * concurrent operations, this may be stale by the time it's used.
 *
 * @param arbuff Buffer to check
 * @return 1 if empty or NULL, 0 otherwise
 *
 * @threadsafe
 * @lockfree
 *
 * @note Result may be stale in concurrent scenarios
 */
int ak_arbuff_is_empty(ak_arbuff_t *arbuff);

/**
 * @brief Check if buffer is full
 *
 * Returns a snapshot indicating whether the buffer is full. Due to
 * concurrent operations, this may be stale by the time it's used.
 *
 * @param arbuff Buffer to check
 * @return 1 if full, 0 if not full or NULL
 *
 * @threadsafe
 * @lockfree
 *
 * @note Result may be stale in concurrent scenarios
 */
int ak_arbuff_is_full(ak_arbuff_t *arbuff);

/**
 * @brief Clear all items from the buffer
 *
 * Resets the buffer to empty state by synchronizing head and tail positions.
 * Does not free the items - caller is responsible for item cleanup.
 *
 * @param arbuff Buffer to clear
 *
 * @warning Not safe to call during concurrent push/pop operations
 * @notthreadsafe
 */
void ak_arbuff_clear(ak_arbuff_t *arbuff);

#endif
