#include "buffer.h"
#include "kernel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN_BUFFER_SIZE 16

ak_buffer_t *ak_buffer_new(size_t initial_size) {
  ak_buffer_t *buffer = AK24_ALLOC(sizeof(ak_buffer_t));
  if (!buffer) {
    return NULL;
  }

  if (initial_size < MIN_BUFFER_SIZE) {
    initial_size = MIN_BUFFER_SIZE;
  }

  buffer->data = AK24_ALLOC(initial_size);
  if (!buffer->data) {
    AK24_FREE(buffer);
    return NULL;
  }

  buffer->capacity = initial_size;
  buffer->count = 0;
  buffer->origin_offset = 0;

  return buffer;
}

ak_buffer_t *ak_buffer_from_file(const char *filepath) {
  if (!filepath) {
    return NULL;
  }

  FILE *file = fopen(filepath, "rb");
  if (!file) {
    return NULL;
  }

  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return NULL;
  }

  long file_size = ftell(file);
  if (file_size < 0) {
    fclose(file);
    return NULL;
  }

  if (fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return NULL;
  }

  ak_buffer_t *buffer = ak_buffer_new((size_t)file_size);
  if (!buffer) {
    fclose(file);
    return NULL;
  }

  if (file_size > 0) {
    size_t bytes_read = fread(buffer->data, 1, (size_t)file_size, file);
    if (ferror(file)) {
      ak_buffer_free(buffer);
      fclose(file);
      return NULL;
    }
    if (bytes_read != (size_t)file_size) {
      ak_buffer_free(buffer);
      fclose(file);
      return NULL;
    }
    buffer->count = bytes_read;
  }

  if (fclose(file) != 0) {
    ak_buffer_free(buffer);
    return NULL;
  }

  return buffer;
}

void ak_buffer_free(ak_buffer_t *buffer) {
  if (!buffer) {
    return;
  }

  if (buffer->data) {
    AK24_FREE(buffer->data);
  }

  AK24_FREE(buffer);
}

int ak_buffer_copy_to(ak_buffer_t *buffer, uint8_t *src, size_t len) {
  if (!buffer || !src) {
    return -1;
  }

  if (len == 0) {
    return 0;
  }

  size_t required = buffer->count + len;

  if (required > buffer->capacity) {
    size_t new_capacity = buffer->capacity;
    while (new_capacity < required) {
      new_capacity *= 2;
    }

    uint8_t *new_data = AK24_REALLOC(buffer->data, new_capacity);
    if (!new_data) {
      return -1;
    }

    buffer->data = new_data;
    buffer->capacity = new_capacity;
  }

  memcpy(buffer->data + buffer->count, src, len);
  buffer->count += len;

  return 0;
}

size_t ak_buffer_count(ak_buffer_t *buffer) {
  if (!buffer) {
    return 0;
  }
  return buffer->count;
}

uint8_t *ak_buffer_data(ak_buffer_t *buffer) {
  if (!buffer) {
    return NULL;
  }
  return buffer->data;
}

void ak_buffer_clear(ak_buffer_t *buffer) {
  if (!buffer) {
    return;
  }
  buffer->count = 0;
}

int ak_buffer_shrink_to_fit(ak_buffer_t *buffer) {
  if (!buffer) {
    return -1;
  }

  if (buffer->count == 0) {
    return 0;
  }

  if (buffer->count >= buffer->capacity) {
    return 0;
  }

  uint8_t *new_data = AK24_REALLOC(buffer->data, buffer->count);
  if (!new_data) {
    return -1;
  }

  buffer->data = new_data;
  buffer->capacity = buffer->count;

  return 0;
}

void ak_buffer_for_each(ak_buffer_t *buffer, ak_buffer_iterator_fn fn,
                        void *callback_data) {
  if (!buffer || !fn) {
    return;
  }

  size_t idx = 0;
  while (idx < buffer->count) {
    int result = fn(&buffer->data[idx], idx, callback_data);

    if (result == 0) {
      break;
    } else if (result == 1) {
      idx++;
    } else {
      idx += result;
    }
  }
}

ak_buffer_t *ak_buffer_sub_buffer(ak_buffer_t *buffer, size_t offset,
                                  size_t length, int *bytes_copied) {
  if (!buffer) {
    if (bytes_copied) {
      *bytes_copied = 0;
    }
    return NULL;
  }

  if (offset >= buffer->count) {
    if (bytes_copied) {
      *bytes_copied = 0;
    }
    return NULL;
  }

  size_t available = buffer->count - offset;
  size_t actual_length = (length > available) ? available : length;

  if (actual_length == 0) {
    if (bytes_copied) {
      *bytes_copied = 0;
    }
    ak_buffer_t *sub_buffer = ak_buffer_new(MIN_BUFFER_SIZE);
    if (sub_buffer) {
      sub_buffer->origin_offset = buffer->origin_offset + offset;
    }
    return sub_buffer;
  }

  ak_buffer_t *sub_buffer = ak_buffer_new(actual_length);
  if (!sub_buffer) {
    if (bytes_copied) {
      *bytes_copied = 0;
    }
    return NULL;
  }

  memcpy(sub_buffer->data, buffer->data + offset, actual_length);
  sub_buffer->count = actual_length;
  sub_buffer->origin_offset = buffer->origin_offset + offset;

  if (bytes_copied) {
    *bytes_copied = (int)actual_length;
  }

  return sub_buffer;
}

void ak_buffer_rotate_left(ak_buffer_t *buffer, size_t n) {
  if (!buffer || buffer->count == 0 || n == 0) {
    return;
  }

  n = n % buffer->count;
  if (n == 0) {
    return;
  }

  uint8_t *temp = AK24_ALLOC(n);
  if (!temp) {
    return;
  }

  memcpy(temp, buffer->data, n);
  memmove(buffer->data, buffer->data + n, buffer->count - n);
  memcpy(buffer->data + (buffer->count - n), temp, n);

  AK24_FREE(temp);
}

void ak_buffer_rotate_right(ak_buffer_t *buffer, size_t n) {
  if (!buffer || buffer->count == 0 || n == 0) {
    return;
  }

  n = n % buffer->count;
  if (n == 0) {
    return;
  }

  uint8_t *temp = AK24_ALLOC(n);
  if (!temp) {
    return;
  }

  memcpy(temp, buffer->data + (buffer->count - n), n);
  memmove(buffer->data + n, buffer->data, buffer->count - n);
  memcpy(buffer->data, temp, n);

  AK24_FREE(temp);
}

int ak_buffer_trim_left(ak_buffer_t *buffer, uint8_t byte) {
  if (!buffer) {
    return -1;
  }

  if (buffer->count == 0) {
    return 0;
  }

  size_t trim_count = 0;
  while (trim_count < buffer->count && buffer->data[trim_count] == byte) {
    trim_count++;
  }

  if (trim_count == 0) {
    return 0;
  }

  if (trim_count == buffer->count) {
    buffer->count = 0;
    return ak_buffer_shrink_to_fit(buffer);
  }

  memmove(buffer->data, buffer->data + trim_count, buffer->count - trim_count);
  buffer->count -= trim_count;

  return ak_buffer_shrink_to_fit(buffer);
}

int ak_buffer_trim_right(ak_buffer_t *buffer, uint8_t byte) {
  if (!buffer) {
    return -1;
  }

  if (buffer->count == 0) {
    return 0;
  }

  size_t trim_count = 0;
  while (trim_count < buffer->count &&
         buffer->data[buffer->count - 1 - trim_count] == byte) {
    trim_count++;
  }

  if (trim_count == 0) {
    return 0;
  }

  buffer->count -= trim_count;

  return ak_buffer_shrink_to_fit(buffer);
}

ak_buffer_t *ak_buffer_copy(ak_buffer_t *buffer) {
  if (!buffer) {
    return NULL;
  }

  ak_buffer_t *copy = ak_buffer_new(buffer->capacity);
  if (!copy) {
    return NULL;
  }

  if (buffer->count > 0) {
    memcpy(copy->data, buffer->data, buffer->count);
    copy->count = buffer->count;
  }

  copy->origin_offset = buffer->origin_offset;

  return copy;
}

split_buffer_t ak_buffer_split(ak_buffer_t *buffer, size_t index, size_t l,
                               size_t r) {
  split_buffer_t result = {NULL, NULL};

  if (!buffer || index > buffer->count) {
    return result;
  }

  size_t left_content_size = index;
  size_t right_content_size = buffer->count - index;

  size_t left_capacity = (left_content_size < l) ? left_content_size : l;
  if (left_capacity < MIN_BUFFER_SIZE && left_content_size > 0) {
    left_capacity = MIN_BUFFER_SIZE;
  } else if (left_capacity == 0 && left_content_size == 0) {
    left_capacity = MIN_BUFFER_SIZE;
  }

  size_t right_capacity = (right_content_size < r) ? right_content_size : r;
  if (right_capacity < MIN_BUFFER_SIZE && right_content_size > 0) {
    right_capacity = MIN_BUFFER_SIZE;
  } else if (right_capacity == 0 && right_content_size == 0) {
    right_capacity = MIN_BUFFER_SIZE;
  }

  result.left = ak_buffer_new(left_capacity);
  if (!result.left) {
    return result;
  }

  result.right = ak_buffer_new(right_capacity);
  if (!result.right) {
    ak_buffer_free(result.left);
    result.left = NULL;
    return result;
  }

  if (left_content_size > 0) {
    memcpy(result.left->data, buffer->data, left_content_size);
    result.left->count = left_content_size;
  }

  if (right_content_size > 0) {
    memcpy(result.right->data, buffer->data + index, right_content_size);
    result.right->count = right_content_size;
  }

  return result;
}

void ak_split_buffer_free(split_buffer_t *split) {
  if (!split) {
    return;
  }

  if (split->left) {
    ak_buffer_free(split->left);
    split->left = NULL;
  }

  if (split->right) {
    ak_buffer_free(split->right);
    split->right = NULL;
  }
}
