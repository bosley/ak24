# Buffer

The buffer provides a dynamic byte array for storing `uint8_t` data. It automatically grows as needed and provides utilities for manipulation, iteration, and file I/O.

## Core Concept

A buffer is a growable byte array that:
- Stores `uint8_t` bytes
- Automatically resizes when capacity is exceeded (doubles capacity)
- Minimum capacity of 16 bytes
- Supports rotation, trimming, splitting, and sub-buffer operations
- Tracks origin offset for sub-buffers
- Can load data directly from files

## Usage

```c
ak_buffer_t *buffer = ak_buffer_new(100);

uint8_t data[] = {1, 2, 3, 4, 5};
ak_buffer_copy_to(buffer, data, 5);

printf("Buffer has %zu bytes\n", ak_buffer_count(buffer));

ak_buffer_free(buffer);
```

## File Loading

```c
ak_buffer_t *buffer = ak_buffer_from_file("data.bin");
if (buffer) {
  printf("Loaded %zu bytes\n", ak_buffer_count(buffer));
  ak_buffer_free(buffer);
}
```

## Rotation

```c
ak_buffer_t *buffer = ak_buffer_new(16);
uint8_t data[] = {1, 2, 3, 4, 5};
ak_buffer_copy_to(buffer, data, 5);

ak_buffer_rotate_left(buffer, 2);

ak_buffer_rotate_right(buffer, 1);
```

## Trimming

```c
ak_buffer_t *buffer = ak_buffer_new(16);
uint8_t data[] = {0, 0, 1, 2, 3, 0, 0};
ak_buffer_copy_to(buffer, data, 7);

ak_buffer_trim_left(buffer, 0);

ak_buffer_trim_right(buffer, 0);
```

## Sub-buffers

```c
ak_buffer_t *buffer = ak_buffer_new(16);
uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8};
ak_buffer_copy_to(buffer, data, 8);

int copied;
ak_buffer_t *sub = ak_buffer_sub_buffer(buffer, 2, 4, &copied);

ak_buffer_free(sub);
```

## Splitting

```c
ak_buffer_t *buffer = ak_buffer_new(16);
uint8_t data[] = {1, 2, 3, 4, 5};
ak_buffer_copy_to(buffer, data, 5);

split_buffer_t split = ak_buffer_split(buffer, 2, 16, 16);

ak_split_buffer_free(&split);
```

## Iteration

```c
int print_byte(uint8_t *byte, size_t idx, void *data) {
  printf("byte[%zu] = %u\n", idx, *byte);
  return 1;
}

ak_buffer_for_each(buffer, print_byte, NULL);
```

Iterator return values:
- 0: Stop iteration
- 1: Continue to next byte
- N > 1: Skip N bytes forward

## API

- `ak_buffer_new(initial_size)` - Create buffer with initial capacity
- `ak_buffer_from_file(filepath)` - Load buffer from file
- `ak_buffer_free(buffer)` - Free the buffer
- `ak_buffer_copy_to(buffer, src, len)` - Append bytes (auto-grows)
- `ak_buffer_count(buffer)` - Get number of bytes
- `ak_buffer_data(buffer)` - Get raw data pointer
- `ak_buffer_clear(buffer)` - Clear buffer (keeps capacity)
- `ak_buffer_shrink_to_fit(buffer)` - Reduce capacity to match count
- `ak_buffer_for_each(buffer, fn, data)` - Iterate over bytes
- `ak_buffer_sub_buffer(buffer, offset, length, copied)` - Create sub-buffer copy
- `ak_buffer_rotate_left(buffer, n)` - Rotate bytes left
- `ak_buffer_rotate_right(buffer, n)` - Rotate bytes right
- `ak_buffer_trim_left(buffer, byte)` - Remove byte from left
- `ak_buffer_trim_right(buffer, byte)` - Remove byte from right
- `ak_buffer_copy(buffer)` - Deep copy buffer
- `ak_buffer_split(buffer, index, l, r)` - Split at index into two buffers
- `ak_split_buffer_free(split)` - Free split buffer result
