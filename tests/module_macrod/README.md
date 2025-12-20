# Module Macros Integration Test

This integration test demonstrates the **AK24 Module Macro System (V1)**, which provides versioned helper macros to reduce boilerplate when implementing AK24 modules.

## Overview

The V1 macro system transforms verbose module implementations into concise, maintainable code while ensuring proper ABI compliance.

### Before (without macros):
```c
// 259 lines of repetitive boilerplate
int ak_module_version(void) { return AK24_MODULE_API_VERSION; }

ak_module_result_e ak_module_init(void **module_ctx,
                                  ak_module_allocator_t *allocator,
                                  const char **error) {
  if (!module_ctx || !allocator) {
    if (error) *error = "Invalid init parameters";
    return AK_MODULE_ERROR_INIT;
  }
  // ... 20+ more lines of boilerplate
}
```

### After (with V1 macros):
```c
// 112 lines, focused on business logic
AK24_MODULE_DECLARE_V1("test_module", "1.0.0", "Description", module_state_t)

AK24_MODULE_INIT_BEGIN_V1(module_state_t)
  state->call_count = 0;
  state->allocated_string = AK24_MODULE_STRDUP_V1("Test");
AK24_MODULE_INIT_END_V1()
```

## Macro Categories

### 1. Core Declaration
- `AK24_MODULE_DECLARE_V1()` - Generates version, info, and allocator boilerplate

### 2. Initialization/Cleanup
- `AK24_MODULE_INIT_BEGIN_V1()` / `END_V1()` - Handles init signature and state allocation
- `AK24_MODULE_DEINIT_BEGIN_V1()` / `END_V1()` - Handles cleanup and state deallocation
- `AK24_MODULE_INIT_ERROR_V1()` - Standardized error handling

### 3. Memory Management
- `AK24_MODULE_ALLOC_V1(type)` - Allocate single object
- `AK24_MODULE_CALLOC_V1(type, count)` - Allocate array
- `AK24_MODULE_FREE_V1(ptr)` - Free with NULL check
- `AK24_MODULE_STRDUP_V1(str)` - Duplicate string

### 4. Function Registration
- `AK24_MODULE_FUNCTION_TABLE_BEGIN_V1()` / `END_V1()` - Auto-generate function lookup
- `AK24_MODULE_REGISTER_FUNCTION_V1(name, func)` - Register a function

### 5. Function Signatures
- `AK24_MODULE_SIGNATURE_TABLE_BEGIN_V1()` / `END_V1()` - Auto-generate signature lookup
- `AK24_MODULE_SIGNATURE_VOID_PTR_V1(name, func)` - Common void(void*) signature

## Build Instructions

1. **Install AK24** (requires sudo):
   ```bash
   cd /Users/bosley/workspace/ak24/build
   sudo make install
   ```

2. **Build the module**:
   ```bash
   cd /Users/bosley/workspace/ak24/tests/module_macrod
   ./setup.sh
   ```

3. **Build the test binary**:
   ```bash
   make
   ```

4. **Run the test**:
   ```bash
   ./module_test
   ```

## What Gets Generated

The macros automatically generate all required AK24 module exports:

✅ `int ak_module_version(void)`
✅ `ak_module_result_e ak_module_init(...)`
✅ `void ak_module_deinit(...)`
✅ `const char *ak_module_info(...)`
✅ `void *ak_module_get_function(...)`
✅ `ak_function_signature_t *ak_module_get_function_signature(...)`

## Benefits

- **57% less code** (259 → 112 lines)
- **Type safety** - Macros handle casts and state management
- **Error handling** - Built-in validation and cleanup
- **Versioned** - _V1 suffix allows API evolution
- **Readable** - Focus on business logic, not boilerplate

## File Structure

```
module_macrod/
├── README.md              # This file
├── main.c                 # Integration test driver
├── Makefile              # Builds test binary
├── setup.sh              # Builds module .dylib/.so
└── lib/
    ├── ak24_module_macros.h  # V1 macro definitions
    ├── lib.h                 # Module interface
    └── lib.c                 # Module implementation (using macros)
```

## Comparison with `modules/` Test

The `modules/` directory contains the **original verbose implementation** without macros. Compare the two to see the macro system's value:

- `modules/lib/lib.c` - 259 lines, manual boilerplate
- `module_macrod/lib/lib.c` - 112 lines, macro-assisted

Both implement the identical AK24 module ABI and functionality.
