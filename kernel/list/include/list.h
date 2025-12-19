#ifndef AK24_LIST_H
#define AK24_LIST_H

#include <string.h>

#define AK24_LIST_VERSION "0.1.0"

typedef struct {
  void **items;
  unsigned capacity;
  unsigned count;
  int item_size;
} list_base_t;

typedef struct {
  unsigned index;
  int valid;
} list_iter_t;

#define list_t(T)                                                              \
  struct {                                                                     \
    list_base_t base;                                                          \
    T *ref;                                                                    \
    T tmp;                                                                     \
  }

#define list_init(l)                                                           \
  do {                                                                         \
    memset(l, 0, sizeof(*(l)));                                                \
    (l)->base.item_size = sizeof((l)->tmp);                                    \
  } while (0)

#define list_deinit(l) list_deinit_(&(l)->base)

#define list_push(l, value)                                                    \
  ((l)->tmp = (value), list_push_(&(l)->base, &(l)->tmp, sizeof((l)->tmp)))

#define list_pop(l) ((l)->ref = list_pop_(&(l)->base))

#define list_get(l, index) ((l)->ref = list_get_(&(l)->base, index))

#define list_set(l, index, value)                                              \
  ((l)->tmp = (value),                                                         \
   list_set_(&(l)->base, index, &(l)->tmp, sizeof((l)->tmp)))

#define list_insert(l, index, value)                                           \
  ((l)->tmp = (value),                                                         \
   list_insert_(&(l)->base, index, &(l)->tmp, sizeof((l)->tmp)))

#define list_remove(l, index) list_remove_(&(l)->base, index)

#define list_count(l) ((l)->base.count)

#define list_capacity(l) ((l)->base.capacity)

#define list_clear(l) list_clear_(&(l)->base)

#define list_rotate_left(l, n) list_rotate_left_(&(l)->base, n)

#define list_rotate_right(l, n) list_rotate_right_(&(l)->base, n)

#define list_iter(l) list_iter_()

#define list_next(l, iter) ((l)->ref = list_next_(&(l)->base, iter))

void list_deinit_(list_base_t *l);
int list_push_(list_base_t *l, void *value, int vsize);
void *list_pop_(list_base_t *l);
void *list_get_(list_base_t *l, unsigned index);
int list_set_(list_base_t *l, unsigned index, void *value, int vsize);
int list_insert_(list_base_t *l, unsigned index, void *value, int vsize);
int list_remove_(list_base_t *l, unsigned index);
void list_clear_(list_base_t *l);
int list_rotate_left_(list_base_t *l, unsigned n);
int list_rotate_right_(list_base_t *l, unsigned n);
list_iter_t list_iter_(void);
void *list_next_(list_base_t *l, list_iter_t *iter);

typedef list_t(void *) list_void_t;
typedef list_t(int) list_int_t;
typedef list_t(char *) list_str_t;
typedef list_t(float) list_float_t;
typedef list_t(double) list_double_t;

#endif
