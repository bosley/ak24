# Map

The map provides a generic hash table with type-safe macros and support for various key types. It uses chaining for collision resolution and automatically resizes when load factor exceeds 1.0.

## Core Concept

A map is a hash table that:
- Stores key-value pairs with generic types
- Supports multiple key types (string, int, float, etc)
- Custom hash and comparison functions
- Automatic resizing (doubles buckets when full)
- Chaining for collision resolution
- Type-safe access via macro system

## Usage

```c
map_int_t ages;
map_init_generic(&ages, sizeof(char *), map_hash_str, map_cmp_str);

const char *name = "Alice";
map_set_generic(&ages, &name, 30);

int *age = map_get_generic(&ages, &name);
printf("%s is %d years old\n", name, *age);

map_deinit(&ages);
```

## Type Definitions

```c
map_void_t
map_str_t
map_int_t
map_char_t
map_float_t
map_double_t

map_t(MyType) custom_map;
```

## String Keys

```c
map_int_t scores;
map_init_generic(&scores, sizeof(char *), map_hash_str, map_cmp_str);

const char *player = "player1";
map_set_generic(&scores, &player, 100);

int *score = map_get_generic(&scores, &player);

map_deinit(&scores);
```

## Integer Keys

```c
map_str_t names;
map_init_generic(&names, sizeof(int), map_hash_i32, map_cmp_mem);

int id = 42;
char *name = "Bob";
map_set_generic(&names, &id, name);

char **retrieved = map_get_generic(&names, &id);

map_deinit(&names);
```

## Iteration

```c
map_int_t data;
map_init_generic(&data, sizeof(char *), map_hash_str, map_cmp_str);

const char *k1 = "a", *k2 = "b";
map_set_generic(&data, &k1, 1);
map_set_generic(&data, &k2, 2);

map_iter_t iter = map_iter(&data);
const char **key;
while ((key = map_next_generic(&data, &iter))) {
  int *val = map_get_generic(&data, key);
  printf("%s = %d\n", *key, *val);
}

map_deinit(&data);
```

## Remove

```c
map_int_t data;
map_init_generic(&data, sizeof(char *), map_hash_str, map_cmp_str);

const char *key = "remove_me";
map_set_generic(&data, &key, 42);

map_remove_generic(&data, &key);

map_deinit(&data);
```

## Hash Functions

- `map_hash_str` - String keys
- `map_hash_i8` - Signed 8-bit int keys
- `map_hash_i16` - Signed 16-bit int keys
- `map_hash_i32` - Signed 32-bit int keys
- `map_hash_i64` - Signed 64-bit int keys
- `map_hash_u8` - Unsigned 8-bit int keys
- `map_hash_u16` - Unsigned 16-bit int keys
- `map_hash_u32` - Unsigned 32-bit int keys
- `map_hash_u64` - Unsigned 64-bit int keys
- `map_hash_f32` - Float keys
- `map_hash_f64` - Double keys
- `map_hash_bool` - Boolean keys

## Comparison Functions

- `map_cmp_str` - String comparison
- `map_cmp_mem` - Memory comparison (for numeric types)

## API

- `map_init_generic(m, keysize, hashfn, cmpfn)` - Initialize map
- `map_deinit(m)` - Free map and all entries
- `map_get_generic(m, key)` - Get value for key
- `map_set_generic(m, key, value)` - Set key-value pair
- `map_remove_generic(m, key)` - Remove key-value pair
- `map_iter(m)` - Create iterator
- `map_next_generic(m, iter)` - Get next key in iteration
