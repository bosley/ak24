#ifndef AK24_BUFFER_H
#define AK24_BUFFER_H

#include <stddef.h>
#include <stdint.h>

typedef struct ak_buffer_s {
  uint8_t *data;
  size_t capacity;
  size_t count;
  size_t origin_offset;
} ak_buffer_t;

typedef ak_buffer_t *ak_buffer_unowned_ptr_t;

typedef struct {
  ak_buffer_t *left;
  ak_buffer_t *right;
} split_buffer_t;

typedef int (*ak_buffer_iterator_fn)(uint8_t *byte, size_t idx,
                                     void *callback_data);

ak_buffer_t *ak_buffer_new(size_t initial_size);

ak_buffer_t *ak_buffer_from_file(const char *filepath);

void ak_buffer_free(ak_buffer_t *buffer);

int ak_buffer_copy_to(ak_buffer_t *buffer, uint8_t *src, size_t len);

size_t ak_buffer_count(ak_buffer_t *buffer);

uint8_t *ak_buffer_data(ak_buffer_t *buffer);

void ak_buffer_clear(ak_buffer_t *buffer);

int ak_buffer_shrink_to_fit(ak_buffer_t *buffer);

void ak_buffer_for_each(ak_buffer_t *buffer, ak_buffer_iterator_fn fn,
                        void *callback_data);

ak_buffer_t *ak_buffer_sub_buffer(ak_buffer_t *buffer, size_t offset,
                                  size_t length, int *bytes_copied);

void ak_buffer_rotate_left(ak_buffer_t *buffer, size_t n);

void ak_buffer_rotate_right(ak_buffer_t *buffer, size_t n);

int ak_buffer_trim_left(ak_buffer_t *buffer, uint8_t byte);

int ak_buffer_trim_right(ak_buffer_t *buffer, uint8_t byte);

ak_buffer_t *ak_buffer_copy(ak_buffer_t *buffer);

split_buffer_t ak_buffer_split(ak_buffer_t *buffer, size_t index, size_t l,
                               size_t r);

void ak_split_buffer_free(split_buffer_t *split);

#endif
