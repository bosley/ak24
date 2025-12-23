# Box

The box provides a thread-safe container for atomic operations on any pointer type. It uses mutex-protected access to ensure safe concurrent usage while maintaining non-owning semantics.

## Core Concept

A box is a container that:
- Wraps any pointer with thread-safe access
- Supports atomic swap operations
- Enables lambda-based visitor pattern with automatic locking
- Does NOT own the contained data (caller manages lifetime)

## Usage

### Basic Box Creation

```c
int value = 42;
ak_box_t *box = ak_box_new(&value);

// ... use box ...

ak_box_free(box);
// Note: caller must free 'value' if dynamically allocated
```

### Atomic Swap

```c
int value1 = 10;
int value2 = 20;

ak_box_t *box = ak_box_new(&value1);

// Atomically swap the value
int *old = (int *)ak_box_swap(box, &value2);
// old == &value1, box now contains &value2

ak_box_free(box);
```

### Thread-Safe Visitor Pattern

```c
void increment_fn(void *captured, void *args) {
  (void)captured;
  int *value = (int *)args;
  (*value)++;
}

int counter = 0;
ak_box_t *box = ak_box_new(&counter);

ak_lambda_t *lambda = ak_lambda_new(increment_fn, NULL, NULL);

// This operation is atomic - the mutex is held during execution
ak_box_visit(box, lambda);

ak_lambda_free(lambda);
ak_box_free(box);
// counter is now 1
```

### With Captured Context

```c
void set_from_captured(void *captured, void *args) {
  int *target = (int *)args;
  int *source = (int *)captured;
  *target = *source;
}

int target = 0;
int source = 42;

ak_box_t *box = ak_box_new(&target);
ak_lambda_t *lambda = ak_lambda_new(set_from_captured, &source, NULL);

ak_box_visit(box, lambda);
// target is now 42

ak_lambda_free(lambda);
ak_box_free(box);
```

### Concurrent Access

```c
void *worker_thread(void *arg) {
  ak_box_t *box = (ak_box_t *)arg;
  ak_lambda_t *lambda = ak_lambda_new(increment_fn, NULL, NULL);

  for (int i = 0; i < 1000; i++) {
    ak_box_visit(box, lambda);  // Thread-safe increment
  }

  ak_lambda_free(lambda);
  return NULL;
}

int counter = 0;
ak_box_t *box = ak_box_new(&counter);

// Start multiple threads that safely increment the counter
AK24_THREAD threads[4];
for (int i = 0; i < 4; i++) {
  AK24_THREAD_CREATE(&threads[i], worker_thread, box);
}

for (int i = 0; i < 4; i++) {
  AK24_THREAD_JOIN(threads[i]);
}

// counter is now 4000
ak_box_free(box);
```

## API

- `ak_box_new(data)` - Create a new box (does NOT take ownership)
- `ak_box_free(box)` - Free the box (does NOT free contained data)
- `ak_box_swap(box, new_data)` - Atomically swap and return old data
- `ak_box_get(box)` - Get current data pointer (read-only, may be stale)
- `ak_box_visit(box, lambda)` - Execute lambda with locked access to data

## Thread Safety

All box operations are thread-safe:
- `ak_box_swap()` uses mutex for atomic exchange
- `ak_box_visit()` holds mutex during lambda execution
- `ak_box_get()` is thread-safe but returned pointer may become stale

## Memory Ownership

The box explicitly does NOT own the contained data:
- `ak_box_new()` stores the pointer without copying
- `ak_box_free()` frees only the box structure, not the data
- Caller is responsible for data lifetime management
- Old data returned by `ak_box_swap()` must be freed by caller if needed
