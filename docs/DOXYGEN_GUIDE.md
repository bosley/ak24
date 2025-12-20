# Doxygen Documentation Guide for AK24

This guide explains how to document AK24 kernel modules using modern Doxygen conventions.

## Quick Start

### Generate Documentation

```bash
make docs
```

Or with CMake:

```bash
cmake -DAK24_BUILD_DOCS=ON ..
make docs
```

View the generated documentation by opening `docs/api/html/index.html` in your browser.

### Installing Doxygen

**macOS:**
```bash
brew install doxygen
```

**Linux (Debian/Ubuntu):**
```bash
sudo apt-get install doxygen
```

**Linux (Fedora/RHEL):**
```bash
sudo dnf install doxygen
```

## Documentation Style

We use **Javadoc-style** comments with modern Doxygen commands.

### File Header

Every header file should start with a file-level comment:

```c
/**
 * @file module_name.h
 * @brief One-line description of the module
 *
 * Detailed description of what this module does, its purpose,
 * and any important implementation details or algorithms used.
 *
 * Key features:
 * - Feature 1
 * - Feature 2
 * - Feature 3
 *
 * @note Any important notes about usage
 * @see Related documentation or references
 */
```

### Structure/Type Documentation

```c
/**
 * @brief Brief description of the structure
 *
 * Detailed description of what this structure represents
 * and how it should be used.
 */
typedef struct {
  int field1;        /**< Description of field1 */
  char *field2;      /**< Description of field2 */
  _Atomic size_t count;  /**< Atomic counter for thread-safe access */
} my_struct_t;
```

### Function Documentation

```c
/**
 * @brief Brief one-line description
 *
 * Detailed description of what the function does, including
 * any important behavior, side effects, or algorithms.
 *
 * @param param1 Description of first parameter
 * @param param2 Description of second parameter
 * @return Description of return value
 *
 * @threadsafe
 * @lockfree
 *
 * @note Any important notes
 * @warning Any warnings about usage
 *
 * @par Example:
 * @code
 * my_struct_t *obj = my_function_new(42, "test");
 * if (!obj) {
 *   fprintf(stderr, "Failed to create object\n");
 * }
 * @endcode
 */
my_struct_t *my_function_new(int param1, const char *param2);
```

## Custom Aliases

We've defined several custom aliases for common patterns:

### Thread Safety Markers

- `@threadsafe` - Function is thread-safe and can be called concurrently
- `@notthreadsafe` - Function requires external synchronization
- `@lockfree` - Function uses lock-free atomic operations
- `@waitfree` - Function has bounded time complexity (wait-free)

### Usage Examples

```c
/**
 * @brief Push item to queue
 * @param queue Queue to push to
 * @param item Item to push
 * @return 0 on success, -1 on failure
 * @threadsafe
 * @lockfree
 */
int queue_push(queue_t *queue, void *item);

/**
 * @brief Clear all items from queue
 * @param queue Queue to clear
 * @notthreadsafe
 * @warning Do not call during concurrent operations
 */
void queue_clear(queue_t *queue);
```

## Documentation Checklist

For each public API function, document:

- [ ] Brief description (`@brief`)
- [ ] Detailed description (what it does, how it works)
- [ ] All parameters (`@param`)
- [ ] Return value (`@return`)
- [ ] Thread safety (`@threadsafe` or `@notthreadsafe`)
- [ ] Lock-free properties if applicable (`@lockfree`, `@waitfree`)
- [ ] Important notes (`@note`)
- [ ] Warnings about misuse (`@warning`)
- [ ] Usage example (`@par Example:` with `@code` block)

For each structure/type:

- [ ] Brief description
- [ ] Detailed description of purpose
- [ ] Documentation for each field (inline `/**< */` style)

For each file:

- [ ] File header with `@file` and `@brief`
- [ ] Overview of module functionality
- [ ] Key features list
- [ ] References to algorithms or papers if applicable

## Best Practices

1. **Be Concise but Complete**: Brief descriptions should be one line, detailed descriptions can be multiple paragraphs

2. **Document Thread Safety**: Always indicate whether functions are thread-safe, especially for concurrent data structures

3. **Provide Examples**: Include code examples for non-trivial functions

4. **Use Inline Comments for Fields**: For struct fields, use `/**< */` style on the same line

5. **Document Edge Cases**: Mention what happens with NULL pointers, empty containers, etc.

6. **Link Related Functions**: Use `@see` to reference related functions or documentation

7. **Mark Deprecated APIs**: Use `@deprecated` for functions that should no longer be used

8. **Document Memory Ownership**: Clarify who owns allocated memory and who is responsible for freeing it

## Example: Complete Module Documentation

See `kernel/arbuff/include/arbuff.h` for a complete example of a well-documented module.

## Configuration

The main Doxygen configuration is in `Doxyfile` at the project root. Key settings:

- **Input**: Recursively scans `kernel/` directory
- **Exclude**: Excludes test files and build directories
- **Output**: Generates HTML in `docs/api/html/`
- **Markdown**: Full markdown support enabled
- **C Optimization**: Optimized for C projects
- **Source Browser**: Enabled for cross-referencing

## Viewing Documentation

After running `make docs`, open `docs/api/html/index.html` in your browser.

The documentation includes:

- **Modules**: Organized by kernel component
- **Files**: Browse by file structure
- **Data Structures**: All types and structures
- **Functions**: Complete API reference
- **Source Code**: Syntax-highlighted source browser
