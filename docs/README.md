# AK24 Documentation

This directory contains documentation for the AK24 kernel project.

## Quick Start

### Generate API Documentation

```bash
make docs
```

Then open `docs/api/html/index.html` in your browser.

### Check Documentation Coverage

```bash
./docs/check_docs.sh
```

## Documentation Files

### Setup and Configuration
- **`DOCUMENTATION_SETUP.md`** - Overview of the documentation system and what's been configured
- **`Doxyfile`** (in project root) - Doxygen configuration file

### Writing Documentation
- **`DOXYGEN_GUIDE.md`** - Complete guide to documenting AK24 modules
- **`DOXYGEN_QUICK_REF.md`** - Quick reference card for Doxygen commands
- **`DOXYGEN_TEMPLATE.h`** - Template file for new modules

### Tools
- **`check_docs.sh`** - Script to check documentation coverage

## Documentation Style

We use modern Javadoc-style Doxygen comments:

```c
/**
 * @file module.h
 * @brief Brief description of the module
 *
 * Detailed description here.
 */

/**
 * @brief Create a new object
 * @param size Initial size
 * @return New object or NULL on failure
 * @threadsafe
 */
object_t *object_new(size_t size);
```

## Example

See `kernel/arbuff/include/arbuff.h` for a fully documented module example.

## Custom Aliases

Special markers for concurrent programming:

- `@threadsafe` - Function is thread-safe
- `@notthreadsafe` - Requires external synchronization
- `@lockfree` - Uses lock-free atomic operations
- `@waitfree` - Wait-free with bounded time complexity

## Getting Help

1. Read `DOXYGEN_GUIDE.md` for comprehensive documentation guidelines
2. Check `DOXYGEN_QUICK_REF.md` for syntax reference
3. Copy patterns from `DOXYGEN_TEMPLATE.h`
4. Study the example in `kernel/arbuff/include/arbuff.h`

## Installing Doxygen

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

## Output

Generated documentation is placed in `docs/api/` (excluded from git).

Structure:
```
docs/api/
└── html/
    ├── index.html          # Main entry point
    ├── files.html          # File listing
    ├── globals.html        # All functions
    └── annotated.html      # All structures
```

## Current Status

Run `./docs/check_docs.sh` to see current documentation coverage.

As of initial setup:
- ✅ `kernel/arbuff/include/arbuff.h` - Fully documented
- ⏳ Other modules - Need documentation

## Contributing Documentation

When adding or modifying public APIs:

1. Document all public functions with `@brief`, `@param`, `@return`
2. Add thread safety markers (`@threadsafe` or `@notthreadsafe`)
3. Include usage examples for non-trivial functions
4. Document memory ownership (who allocates, who frees)
5. Run `make docs` to verify the output looks correct
6. Run `./docs/check_docs.sh` to check coverage

## Resources

- [Doxygen Manual](https://www.doxygen.nl/manual/)
- [Markdown Support](https://www.doxygen.nl/manual/markdown.html)
- [C Documentation Guide](https://www.doxygen.nl/manual/docblocks.html)
