# AK24 Documentation Setup

This document describes the modern Doxygen documentation system configured for the AK24 kernel project.

## Overview

The AK24 project now has a comprehensive Doxygen documentation system configured with modern best practices for C projects. The system is optimized for documenting concurrent, lock-free data structures and kernel components.

## What's Been Set Up

### 1. Doxygen Configuration (`Doxyfile`)

A modern Doxygen configuration file with:

- **C-optimized settings**: `OPTIMIZE_OUTPUT_FOR_C = YES`
- **Markdown support**: Full markdown in comments and separate `.md` files
- **Modern HTML output**: Clean, responsive HTML with tree view navigation
- **Source browser**: Syntax-highlighted source code with cross-references
- **Custom aliases**: Thread safety markers (`@threadsafe`, `@lockfree`, etc.)
- **Smart filtering**: Excludes test files and build artifacts
- **Javadoc style**: `JAVADOC_AUTOBRIEF = YES` for concise documentation

### 2. Build Integration

#### Makefile Target
```bash
make docs          # Generate documentation
make docs-clean    # Remove generated docs
```

#### CMake Integration
```bash
cmake -DAK24_BUILD_DOCS=ON ..
make docs
```

### 3. Custom Documentation Aliases

Special markers for concurrent programming:

- `@threadsafe` - Function is safe for concurrent access
- `@notthreadsafe` - Requires external synchronization
- `@lockfree` - Uses lock-free atomic operations
- `@waitfree` - Bounded time complexity (wait-free)

### 4. Example Documentation

The `kernel/arbuff/include/arbuff.h` file has been fully documented as a reference example showing:

- File-level documentation with overview
- Structure documentation with field descriptions
- Function documentation with parameters, returns, examples
- Thread safety annotations
- Memory ownership clarification
- Usage examples with code blocks

### 5. Documentation Guides

Three comprehensive guides have been created:

1. **`DOXYGEN_GUIDE.md`** - Complete guide to documenting AK24 modules
2. **`DOXYGEN_QUICK_REF.md`** - Quick reference card for common commands
3. **`DOXYGEN_TEMPLATE.h`** - Template file for new modules

## Documentation Style

### Modern Javadoc-Style Comments

We use `/** */` style comments with Doxygen commands:

```c
/**
 * @brief Create a new buffer
 *
 * Allocates and initializes a buffer with the specified capacity.
 * The actual capacity may be rounded up to the next power of 2.
 *
 * @param capacity Desired capacity (must be > 0)
 * @return Pointer to new buffer, or NULL on failure
 *
 * @threadsafe
 * @lockfree
 *
 * @par Example:
 * @code
 * buffer_t *buf = buffer_new(100);
 * if (!buf) {
 *   fprintf(stderr, "Allocation failed\n");
 * }
 * @endcode
 */
buffer_t *buffer_new(size_t capacity);
```

### Key Features

1. **Brief + Detailed**: Start with `@brief` one-liner, follow with details
2. **Complete Parameter Docs**: Document every parameter with `@param`
3. **Clear Return Values**: Use `@return` and optionally `@retval`
4. **Thread Safety**: Always mark with `@threadsafe` or `@notthreadsafe`
5. **Usage Examples**: Include `@code` blocks for non-trivial functions
6. **Memory Ownership**: Clarify who allocates and who frees
7. **Cross-References**: Use `@see` to link related functions

## Output Structure

Documentation is generated in `docs/api/`:

```
docs/api/
├── html/
│   ├── index.html          # Main entry point
│   ├── files.html          # File listing
│   ├── globals.html        # All functions
│   ├── annotated.html      # All structures
│   └── ...
└── ...
```

## Viewing Documentation

After running `make docs`, open `docs/api/html/index.html` in your browser.

The documentation includes:

- **Files**: Browse by directory structure
- **Data Structures**: All types, structs, and typedefs
- **Functions**: Complete API reference with search
- **Source Code**: Syntax-highlighted with cross-links
- **Search**: Full-text search across all documentation

## Next Steps

### Documenting Other Modules

To document other kernel modules, follow this process:

1. **Read the guides**: Start with `DOXYGEN_GUIDE.md`
2. **Use the template**: Copy patterns from `DOXYGEN_TEMPLATE.h`
3. **Reference arbuff**: See `kernel/arbuff/include/arbuff.h` for a complete example
4. **Check quick ref**: Use `DOXYGEN_QUICK_REF.md` for syntax

### Priority Modules to Document

Based on the kernel structure, these modules should be documented next:

1. **atoms** - Atom/symbol system
2. **buffer** - Buffer management
3. **context** - Execution context
4. **forms** - Forms/data structures
5. **lambda** - Lambda functions
6. **list** - List data structure
7. **map** - Map/dictionary
8. **scanner** - Scanner/lexer

### Documentation Checklist

For each module:

- [ ] File header with `@file` and `@brief`
- [ ] Overview of module purpose and design
- [ ] All structures documented with field descriptions
- [ ] All public functions documented with:
  - [ ] `@brief` description
  - [ ] `@param` for all parameters
  - [ ] `@return` for return values
  - [ ] Thread safety markers
  - [ ] Usage examples for complex functions
- [ ] Memory ownership documented
- [ ] Edge cases and error conditions explained

## Configuration Details

### Input Files

- Recursively scans `kernel/` directory
- Includes `.c`, `.h`, and `.md` files
- Excludes `test/` directories and `*_test.c` files

### Output Format

- HTML with tree view navigation
- Syntax-highlighted source code
- Cross-referenced symbols
- Search functionality
- Mobile-friendly responsive design

### Preprocessing

- Macro expansion enabled for documentation
- Predefined macros: `AK24_GC_ENABLED=1`, `__STDC_VERSION__=201112L`
- Include paths: `kernel/`

## Best Practices

1. **Document as you code**: Add docs when writing new functions
2. **Keep docs updated**: Update docs when changing APIs
3. **Be consistent**: Follow the established patterns
4. **Test your docs**: Run `make docs` and review the output
5. **Use examples**: Show how to use non-trivial APIs
6. **Explain thread safety**: Critical for concurrent code
7. **Document ownership**: Clarify memory management
8. **Link related items**: Use `@see` for discoverability

## Troubleshooting

### Doxygen Not Found

Install Doxygen:
- macOS: `brew install doxygen`
- Ubuntu/Debian: `sudo apt-get install doxygen`
- Fedora/RHEL: `sudo dnf install doxygen`

### Documentation Not Generating

1. Check that `Doxyfile` exists in project root
2. Verify input paths in `Doxyfile` are correct
3. Run `doxygen Doxyfile` directly to see errors
4. Check that header files have proper `/** */` comments

### Broken Links or Missing Items

1. Ensure functions/types are declared in header files (not just `.c`)
2. Check that files aren't excluded by `EXCLUDE_PATTERNS`
3. Verify `@see` references use correct function names
4. Make sure struct fields use `/**< */` inline style

## Resources

- **Doxygen Manual**: https://www.doxygen.nl/manual/
- **Markdown Support**: https://www.doxygen.nl/manual/markdown.html
- **Example Project**: `kernel/arbuff/include/arbuff.h`
- **Quick Reference**: `docs/DOXYGEN_QUICK_REF.md`
- **Full Guide**: `docs/DOXYGEN_GUIDE.md`
- **Template**: `docs/DOXYGEN_TEMPLATE.h`
