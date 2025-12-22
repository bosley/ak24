# Doxygen Quick Reference

## Common Commands

| Command | Usage | Example |
|---------|-------|---------|
| `@file` | Document a file | `@file buffer.h` |
| `@brief` | One-line description | `@brief Create a new buffer` |
| `@param` | Document parameter | `@param size Buffer size in bytes` |
| `@return` | Document return value | `@return Pointer to buffer or NULL` |
| `@retval` | Specific return value | `@retval 0 Success` |
| `@note` | Important note | `@note Thread-safe for concurrent access` |
| `@warning` | Warning about usage | `@warning Do not call after free` |
| `@see` | Cross-reference | `@see buffer_free()` |
| `@code` | Start code block | `@code` |
| `@endcode` | End code block | `@endcode` |
| `@par` | Named paragraph | `@par Example:` |

## Comment Styles

### File Header
```c
/**
 * @file myfile.h
 * @brief Brief description
 *
 * Detailed description here.
 */
```

### Function
```c
/**
 * @brief Brief description
 * @param name Parameter description
 * @return Return value description
 */
int my_function(const char *name);
```

### Struct
```c
/**
 * @brief Brief description of struct
 */
typedef struct {
  int field;    /**< Field description */
} my_struct_t;
```

### Inline Field Comment
```c
int count;      /**< Number of items */
```

## Custom Aliases (AK24 Specific)

| Alias | Usage | Expands To |
|-------|-------|------------|
| `@threadsafe` | Mark function as thread-safe | "Thread Safety: This function is thread-safe..." |
| `@notthreadsafe` | Mark function as NOT thread-safe | "Thread Safety: This function is NOT thread-safe..." |
| `@lockfree` | Mark as lock-free | "Lock-Free: This function uses lock-free atomic operations" |
| `@waitfree` | Mark as wait-free | "Wait-Free: This function is wait-free with bounded time..." |

## Example Function Documentation

```c
/**
 * @brief Push an item onto the stack
 *
 * Adds an item to the top of the stack. If the stack is full,
 * it will automatically resize to accommodate the new item.
 *
 * @param stack Stack to push onto
 * @param item Item to push (must not be NULL)
 * @return 0 on success, -1 on failure
 * @retval 0 Item successfully pushed
 * @retval -1 Stack is NULL or allocation failed
 *
 * @threadsafe
 * @lockfree
 *
 * @note The stack takes ownership of the item pointer
 * @warning Do not push NULL items
 *
 * @par Example:
 * @code
 * stack_t *s = stack_new(10);
 * int *value = malloc(sizeof(int));
 * *value = 42;
 *
 * if (stack_push(s, value) != 0) {
 *   fprintf(stderr, "Push failed\n");
 *   AK24_FREE(value);
 * }
 * @endcode
 *
 * @see stack_pop(), stack_peek()
 */
int stack_push(stack_t *stack, void *item);
```

## Markdown Support

Doxygen supports markdown in descriptions:

```c
/**
 * @brief Process data with options
 *
 * This function supports several modes:
 * - **Fast mode**: Quick processing with lower accuracy
 * - **Accurate mode**: Slower but more precise
 * - **Balanced mode**: Default, good compromise
 *
 * You can also use `inline code` and [links](https://example.com).
 *
 * @param data Input data
 * @param mode Processing mode
 */
```

## Lists

```c
/**
 * @brief Initialize the system
 *
 * Performs the following steps:
 * 1. Allocate memory
 * 2. Initialize structures
 * 3. Start background threads
 * 4. Register signal handlers
 */
```

## Code Blocks

```c
/**
 * @brief Example function
 *
 * @par Usage Example:
 * @code
 * my_struct_t *obj = my_function_new();
 * my_function_process(obj, data);
 * my_function_free(obj);
 * @endcode
 */
```

## Sections

```c
/**
 * @brief Complex function
 *
 * @par Thread Safety:
 * This function is thread-safe when called with different
 * instances, but not safe for concurrent calls on the same instance.
 *
 * @par Performance:
 * Time complexity: O(n log n)
 * Space complexity: O(n)
 *
 * @par Implementation Notes:
 * Uses quicksort algorithm with median-of-three pivot selection.
 */
```

## Common Patterns

### Constructor/Destructor Pair
```c
/**
 * @brief Create a new object
 * @param size Initial size
 * @return New object or NULL on failure
 * @note Caller must free with object_free()
 */
object_t *object_new(size_t size);

/**
 * @brief Free an object
 * @param obj Object to free (NULL is safe)
 * @warning Do not use object after calling this
 */
void object_free(object_t *obj);
```

### Getter/Setter Pair
```c
/**
 * @brief Get the current size
 * @param obj Object to query
 * @return Current size, or 0 if obj is NULL
 */
size_t object_get_size(object_t *obj);

/**
 * @brief Set the size
 * @param obj Object to modify
 * @param size New size
 * @return 0 on success, -1 on failure
 */
int object_set_size(object_t *obj, size_t size);
```

## Tips

1. **Always document public APIs** - Every function in a header file should have documentation
2. **Use `@brief` first** - Start with a one-line summary
3. **Document all parameters** - Use `@param` for each parameter
4. **Explain return values** - Use `@return` and optionally `@retval` for specific values
5. **Add examples** - Use `@code` blocks for non-trivial functions
6. **Mark thread safety** - Always indicate if functions are thread-safe
7. **Document ownership** - Clarify who owns memory and who frees it
8. **Use `@see` for related functions** - Help users discover related APIs
9. **Keep it up to date** - Update docs when changing function signatures or behavior
10. **Test your docs** - Run `make docs` and verify the output looks correct
