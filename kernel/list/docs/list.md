# List

The list provides a generic dynamic array with type-safe macros. It automatically grows as needed and supports common operations like push, pop, insert, remove, and rotation.

## Core Concept

A list is a growable array that:
- Stores any type via generic macros
- Automatically resizes when capacity is exceeded (doubles capacity)
- Initial capacity of 8 items
- Type-safe access via macro system
- Supports iteration with iterator pattern
- Items are heap-allocated copies

## Usage

```c
list_int_t numbers;
list_init(&numbers);

list_push(&numbers, 10);
list_push(&numbers, 20);
list_push(&numbers, 30);

int *val = list_get(&numbers, 1);
printf("numbers[1] = %d\n", *val);

list_deinit(&numbers);
```

## Type Definitions

```c
list_void_t
list_int_t
list_str_t
list_float_t
list_double_t

list_t(MyType) custom_list;
```

## Push and Pop

```c
list_int_t stack;
list_init(&stack);

list_push(&stack, 1);
list_push(&stack, 2);
list_push(&stack, 3);

int *top = list_pop(&stack);
printf("popped: %d\n", *top);

list_deinit(&stack);
```

## Insert and Remove

```c
list_int_t items;
list_init(&items);

list_push(&items, 1);
list_push(&items, 3);
list_insert(&items, 1, 2);

list_remove(&items, 0);

list_deinit(&items);
```

## Iteration

```c
list_int_t numbers;
list_init(&numbers);
list_push(&numbers, 10);
list_push(&numbers, 20);
list_push(&numbers, 30);

list_iter_t iter = list_iter(&numbers);
int *val;
while ((val = list_next(&numbers, &iter))) {
  printf("%d\n", *val);
}

list_deinit(&numbers);
```

## Rotation

```c
list_int_t items;
list_init(&items);
list_push(&items, 1);
list_push(&items, 2);
list_push(&items, 3);

list_rotate_left(&items, 1);

list_rotate_right(&items, 2);

list_deinit(&items);
```

## Set and Get

```c
list_int_t items;
list_init(&items);
list_push(&items, 10);
list_push(&items, 20);

list_set(&items, 0, 100);

int *val = list_get(&items, 0);

list_deinit(&items);
```

## API

- `list_init(l)` - Initialize list
- `list_deinit(l)` - Free list and all items
- `list_push(l, value)` - Append value to end
- `list_pop(l)` - Remove and return last item
- `list_get(l, index)` - Get item at index
- `list_set(l, index, value)` - Set item at index
- `list_insert(l, index, value)` - Insert value at index
- `list_remove(l, index)` - Remove item at index
- `list_count(l)` - Get number of items
- `list_capacity(l)` - Get current capacity
- `list_clear(l)` - Remove all items (keeps capacity)
- `list_rotate_left(l, n)` - Rotate items left by n positions
- `list_rotate_right(l, n)` - Rotate items right by n positions
- `list_iter(l)` - Create iterator
- `list_next(l, iter)` - Get next item in iteration
