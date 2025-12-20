# AK24 Module API v1

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
