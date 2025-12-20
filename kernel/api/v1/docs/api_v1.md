# AK24 Module API v1

This directory contains versioned API helpers for AK24 module development.

## Structure

```
kernel/api/v1/include/
└── ak24_module_macros.h    # V1 module development macros
```

## Purpose

The API directory provides stable, versioned interfaces for module developers:

- **Versioned**: Changes to the API can be introduced in v2, v3, etc. without breaking existing modules
- **Installed**: Headers are installed system-wide with the AK24 library
- **Documented**: Each API version has comprehensive documentation

## Version 1 (Current)

### ak24_module_macros.h

Provides helper macros to reduce boilerplate in AK24 module implementations:

- **Module declaration** - Auto-generates version() and info() functions
- **Initialization helpers** - Handles parameter validation and state allocation
- **Memory management** - Allocator wrapper macros
- **Function registration** - Auto-generates function lookup tables
- **Signature metadata** - Simplifies signature metadata generation

**Benefits:**
- 57% less code compared to manual implementation
- Type-safe with compile-time checks
- Consistent error handling
- Easy to maintain

### Usage

```c
#include <ak24_module_macros.h>

// Define your module state
typedef struct {
  ak_module_allocator_t *allocator;
  int call_count;
  // ... your fields
} my_module_state_t;

// Declare module metadata
AK24_MODULE_DECLARE_V1("my_module", "1.0.0",
                       "My awesome module",
                       my_module_state_t)

// Initialize
AK24_MODULE_INIT_BEGIN_V1(my_module_state_t)
  state->call_count = 0;
  // ... your init code
AK24_MODULE_INIT_END_V1()

// Cleanup
AK24_MODULE_DEINIT_BEGIN_V1(my_module_state_t)
  // ... your cleanup code
AK24_MODULE_DEINIT_END_V1()

// Register functions
AK24_MODULE_FUNCTION_TABLE_BEGIN_V1()
  AK24_MODULE_REGISTER_FUNCTION_V1("my_func", my_func)
AK24_MODULE_FUNCTION_TABLE_END_V1()
```

See [test integration example](/Users/bosley/workspace/ak24/tests/module_macrod) for complete usage.

## Installation

The API headers are installed as part of the AK24 library:

```bash
cd build
sudo make install
```

Headers are installed to: `${AK24_HOME}/include/ak24/ak24_module_macros.h`

## Future Versions

When the module API evolves, new versions will be added:

```
kernel/api/
├── v1/include/  # Current stable API
├── v2/include/  # Future enhancements
└── v3/include/  # Breaking changes
```

Each version remains available, allowing modules to choose their target API level.

## Macro Naming Convention

All macros are suffixed with `_V1` to indicate the API version:

- `AK24_MODULE_DECLARE_V1`
- `AK24_MODULE_INIT_BEGIN_V1`
- `AK24_MODULE_ALLOC_V1`

When v2 is introduced, it will use `_V2` suffixes.
