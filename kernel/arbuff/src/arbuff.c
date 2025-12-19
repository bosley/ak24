#include "arbuff.h"
#include "kernel.h"
#include <string.h>

static size_t next_power_of_2(size_t n) {
  if (n == 0)
    return 1;
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n |= n >> 32;
  return n + 1;
}

ak_arbuff_t *ak_arbuff_new(size_t capacity) {
  if (capacity == 0) {
    return NULL;
  }

  capacity = next_power_of_2(capacity);

  ak_arbuff_t *arbuff = AK24_ALLOC(sizeof(ak_arbuff_t));
  if (!arbuff) {
    return NULL;
  }

  arbuff->slots = AK24_ALLOC(sizeof(ak_arbuff_slot_t) * capacity);
  if (!arbuff->slots) {
    AK24_FREE(arbuff);
    return NULL;
  }

  arbuff->capacity = capacity;
  arbuff->mask = capacity - 1;
  atomic_init(&arbuff->head, 0);
  atomic_init(&arbuff->tail, 0);

  for (size_t i = 0; i < capacity; i++) {
    atomic_init(&arbuff->slots[i].value, NULL);
    atomic_init(&arbuff->slots[i].sequence, i);
  }

  return arbuff;
}

void ak_arbuff_free(ak_arbuff_t *arbuff) {
  if (!arbuff) {
    return;
  }

  if (arbuff->slots) {
    AK24_FREE(arbuff->slots);
  }

  AK24_FREE(arbuff);
}

int ak_arbuff_push(ak_arbuff_t *arbuff, void *item) {
  if (!arbuff) {
    return -1;
  }

  size_t head;
  while (1) {
    head = atomic_load_explicit(&arbuff->head, memory_order_relaxed);
    ak_arbuff_slot_t *slot = &arbuff->slots[head & arbuff->mask];
    size_t seq = atomic_load_explicit(&slot->sequence, memory_order_acquire);
    intptr_t diff = (intptr_t)seq - (intptr_t)head;

    if (diff == 0) {
      if (atomic_compare_exchange_weak_explicit(&arbuff->head, &head, head + 1,
                                                memory_order_relaxed,
                                                memory_order_relaxed)) {
        break;
      }
    } else if (diff < 0) {
      return -1;
    }
  }

  ak_arbuff_slot_t *slot = &arbuff->slots[head & arbuff->mask];
  atomic_store_explicit(&slot->value, item, memory_order_release);
  atomic_store_explicit(&slot->sequence, head + 1, memory_order_release);

  return 0;
}

void *ak_arbuff_pop(ak_arbuff_t *arbuff) {
  if (!arbuff) {
    return NULL;
  }

  size_t tail;
  while (1) {
    tail = atomic_load_explicit(&arbuff->tail, memory_order_relaxed);
    ak_arbuff_slot_t *slot = &arbuff->slots[tail & arbuff->mask];
    size_t seq = atomic_load_explicit(&slot->sequence, memory_order_acquire);
    intptr_t diff = (intptr_t)seq - (intptr_t)(tail + 1);

    if (diff == 0) {
      if (atomic_compare_exchange_weak_explicit(&arbuff->tail, &tail, tail + 1,
                                                memory_order_relaxed,
                                                memory_order_relaxed)) {
        break;
      }
    } else if (diff < 0) {
      return NULL;
    }
  }

  ak_arbuff_slot_t *slot = &arbuff->slots[tail & arbuff->mask];
  void *item = atomic_load_explicit(&slot->value, memory_order_acquire);
  atomic_store_explicit(&slot->sequence, tail + arbuff->mask + 1,
                        memory_order_release);

  return item;
}

void *ak_arbuff_peek(ak_arbuff_t *arbuff) {
  if (!arbuff) {
    return NULL;
  }

  size_t tail = atomic_load_explicit(&arbuff->tail, memory_order_acquire);
  ak_arbuff_slot_t *slot = &arbuff->slots[tail & arbuff->mask];
  size_t seq = atomic_load_explicit(&slot->sequence, memory_order_acquire);

  if ((intptr_t)seq - (intptr_t)(tail + 1) == 0) {
    return atomic_load_explicit(&slot->value, memory_order_acquire);
  }

  return NULL;
}

size_t ak_arbuff_count(ak_arbuff_t *arbuff) {
  if (!arbuff) {
    return 0;
  }

  size_t head = atomic_load_explicit(&arbuff->head, memory_order_acquire);
  size_t tail = atomic_load_explicit(&arbuff->tail, memory_order_acquire);

  if (head >= tail) {
    return head - tail;
  }

  return 0;
}

size_t ak_arbuff_capacity(ak_arbuff_t *arbuff) {
  if (!arbuff) {
    return 0;
  }

  return arbuff->capacity;
}

int ak_arbuff_is_empty(ak_arbuff_t *arbuff) {
  if (!arbuff) {
    return 1;
  }

  size_t head = atomic_load_explicit(&arbuff->head, memory_order_acquire);
  size_t tail = atomic_load_explicit(&arbuff->tail, memory_order_acquire);

  return head == tail;
}

int ak_arbuff_is_full(ak_arbuff_t *arbuff) {
  if (!arbuff) {
    return 0;
  }

  size_t head = atomic_load_explicit(&arbuff->head, memory_order_acquire);
  size_t tail = atomic_load_explicit(&arbuff->tail, memory_order_acquire);

  return (head - tail) >= arbuff->capacity;
}

void ak_arbuff_clear(ak_arbuff_t *arbuff) {
  if (!arbuff) {
    return;
  }

  size_t head = atomic_load_explicit(&arbuff->head, memory_order_acquire);
  atomic_store_explicit(&arbuff->tail, head, memory_order_release);

  for (size_t i = 0; i < arbuff->capacity; i++) {
    atomic_store_explicit(&arbuff->slots[i].value, NULL, memory_order_release);
    atomic_store_explicit(&arbuff->slots[i].sequence, head + i,
                          memory_order_release);
  }
}
