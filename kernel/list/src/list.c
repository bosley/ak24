#include "list.h"
#include "kernel.h"
#include <string.h>

#define LIST_INITIAL_CAPACITY 8

static int list_ensure_capacity_(list_base_t *l, unsigned required) {
  if (l->capacity >= required) {
    return 0;
  }

  unsigned new_capacity =
      l->capacity == 0 ? LIST_INITIAL_CAPACITY : l->capacity;
  while (new_capacity < required) {
    new_capacity *= 2;
  }

  void **new_items = AK24_REALLOC(l->items, sizeof(void *) * new_capacity);
  if (!new_items) {
    return -1;
  }

  l->items = new_items;
  l->capacity = new_capacity;
  return 0;
}

void list_deinit_(list_base_t *l) {
  if (l->items) {
    for (unsigned i = 0; i < l->count; i++) {
      if (l->items[i]) {
        AK24_FREE(l->items[i]);
      }
    }
    AK24_FREE(l->items);
  }
  l->items = NULL;
  l->count = 0;
  l->capacity = 0;
}

int list_push_(list_base_t *l, void *value, int vsize) {
  if (list_ensure_capacity_(l, l->count + 1) != 0) {
    return -1;
  }

  void *item = AK24_ALLOC(vsize);
  if (!item) {
    return -1;
  }

  memcpy(item, value, vsize);
  l->items[l->count] = item;
  l->count++;
  return 0;
}

void *list_pop_(list_base_t *l) {
  if (l->count == 0) {
    return NULL;
  }

  l->count--;
  return l->items[l->count];
}

void *list_get_(list_base_t *l, unsigned index) {
  if (index >= l->count) {
    return NULL;
  }
  return l->items[index];
}

int list_set_(list_base_t *l, unsigned index, void *value, int vsize) {
  if (index >= l->count) {
    return -1;
  }

  memcpy(l->items[index], value, vsize);
  return 0;
}

int list_insert_(list_base_t *l, unsigned index, void *value, int vsize) {
  if (index > l->count) {
    return -1;
  }

  if (list_ensure_capacity_(l, l->count + 1) != 0) {
    return -1;
  }

  void *item = AK24_ALLOC(vsize);
  if (!item) {
    return -1;
  }

  memcpy(item, value, vsize);

  for (unsigned i = l->count; i > index; i--) {
    l->items[i] = l->items[i - 1];
  }

  l->items[index] = item;
  l->count++;
  return 0;
}

int list_remove_(list_base_t *l, unsigned index) {
  if (index >= l->count) {
    return -1;
  }

  AK24_FREE(l->items[index]);

  for (unsigned i = index; i < l->count - 1; i++) {
    l->items[i] = l->items[i + 1];
  }

  l->count--;
  return 0;
}

void list_clear_(list_base_t *l) {
  for (unsigned i = 0; i < l->count; i++) {
    if (l->items[i]) {
      AK24_FREE(l->items[i]);
    }
  }
  l->count = 0;
}

list_iter_t list_iter_(void) {
  list_iter_t iter;
  iter.index = 0;
  iter.valid = 1;
  return iter;
}

void *list_next_(list_base_t *l, list_iter_t *iter) {
  if (!iter->valid || iter->index >= l->count) {
    iter->valid = 0;
    return NULL;
  }

  void *item = l->items[iter->index];
  iter->index++;
  return item;
}

int list_rotate_left_(list_base_t *l, unsigned n) {
  if (l->count == 0 || n == 0) {
    return 0;
  }

  n = n % l->count;
  if (n == 0) {
    return 0;
  }

  void **temp = AK24_ALLOC(sizeof(void *) * n);
  if (!temp) {
    return -1;
  }

  for (unsigned i = 0; i < n; i++) {
    temp[i] = l->items[i];
  }

  for (unsigned i = 0; i < l->count - n; i++) {
    l->items[i] = l->items[i + n];
  }

  for (unsigned i = 0; i < n; i++) {
    l->items[l->count - n + i] = temp[i];
  }

  AK24_FREE(temp);
  return 0;
}

int list_rotate_right_(list_base_t *l, unsigned n) {
  if (l->count == 0 || n == 0) {
    return 0;
  }

  n = n % l->count;
  if (n == 0) {
    return 0;
  }

  void **temp = AK24_ALLOC(sizeof(void *) * n);
  if (!temp) {
    return -1;
  }

  for (unsigned i = 0; i < n; i++) {
    temp[i] = l->items[l->count - n + i];
  }

  for (unsigned i = l->count - 1; i >= n; i--) {
    l->items[i] = l->items[i - n];
  }

  for (unsigned i = 0; i < n; i++) {
    l->items[i] = temp[i];
  }

  AK24_FREE(temp);
  return 0;
}
