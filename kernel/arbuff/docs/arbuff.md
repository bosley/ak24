# Atomic Ring Buffer (arbuff)

The atomic ring buffer provides a thread-safe, lock-free MPMC (multi-producer multi-consumer) circular buffer for storing `void*` pointers. It uses a sequence-based algorithm with C11 atomics and proper memory ordering for safe concurrent access from multiple threads.

## Core Concept

An atomic ring buffer is a fixed-size circular queue that:
- Stores `void*` pointers to user data
- Uses lock-free atomic operations for thread-safe concurrent access
- Supports multiple producers and consumers simultaneously
- Capacity automatically rounded to next power of 2
- Returns errors when full (no overwrite)
- Uses sequence numbers per slot to track state

## Usage

```c
ak_arbuff_t *arbuff = ak_arbuff_new(100);

int value = 42;
if (ak_arbuff_push(arbuff, &value) == 0) {
  printf("Pushed successfully\n");
}

int *retrieved = (int *)ak_arbuff_pop(arbuff);
if (retrieved) {
  printf("Value: %d\n", *retrieved);
}

ak_arbuff_free(arbuff);
```

## Thread Safety

The arbuff uses a sequence-based lock-free algorithm with proper memory barriers (`memory_order_acquire`/`memory_order_release`), making it safe for concurrent access from multiple producers and consumers without locks:

```c
void *producer_thread(void *arg) {
  ak_arbuff_t *arbuff = (ak_arbuff_t *)arg;
  int *data = malloc(sizeof(int));
  *data = 42;

  while (ak_arbuff_push(arbuff, data) != 0) {
    sched_yield();
  }
  return NULL;
}

void *consumer_thread(void *arg) {
  ak_arbuff_t *arbuff = (ak_arbuff_t *)arg;
  void *item = ak_arbuff_pop(arbuff);
  if (item) {
    process(item);
    free(item);
  }
  return NULL;
}
```

**Key Features:**
- Lock-free: No mutexes or blocking operations
- Wait-free bounded: Operations complete in bounded time under contention
- Memory safe: Proper acquire/release semantics prevent reordering issues
- ABA problem resistant: Sequence numbers prevent slot reuse issues

## Implementation Details

The arbuff uses a modified version of Dmitry Vyukov's bounded MPMC queue algorithm:
- Each slot contains both a value pointer and a sequence number
- Sequence numbers track whether a slot is ready for read/write
- CAS operations on head/tail ensure only one thread claims each slot
- Power-of-2 capacity enables efficient bitwise modulo operations
- Memory barriers ensure visibility across threads

## API

- `ak_arbuff_new(capacity)` - Create buffer (capacity rounded to power of 2)
- `ak_arbuff_push(arbuff, item)` - Thread-safe push (returns -1 if full)
- `ak_arbuff_pop(arbuff)` - Thread-safe pop (returns NULL if empty)
- `ak_arbuff_peek(arbuff)` - Thread-safe peek without removing
- `ak_arbuff_count(arbuff)` - Approximate count (may be stale)
- `ak_arbuff_is_empty(arbuff)` - Check if empty (may be stale)
- `ak_arbuff_is_full(arbuff)` - Check if full (may be stale)
- `ak_arbuff_clear(arbuff)` - Clear buffer (not thread-safe during concurrent ops)
- `ak_arbuff_free(arbuff)` - Free the buffer (ensure no concurrent access)
