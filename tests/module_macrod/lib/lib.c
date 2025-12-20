/**
 * @file lib.c
 * @brief AK24 test module implementation using V1 macros
 *
 * Demonstrates the AK24 module macro system to reduce boilerplate
 * and ensure proper ABI implementation.
 */

#include "lib.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

//=============================================================================
// Module State Definition
//=============================================================================

typedef struct {
  ak_module_allocator_t *allocator;
  char *allocated_string; // Test allocator usage
  int call_count;
} module_state_t;

//=============================================================================
// Module Declaration (auto-generates version, info functions)
//=============================================================================

AK24_MODULE_DECLARE_V1("test_module_macrod", "1.0.0",
                       "AK24 integration test module using V1 macros",
                       module_state_t)

//=============================================================================
// Module Initialization (with macro helpers)
//=============================================================================

AK24_MODULE_INIT_BEGIN_V1(module_state_t)
// Initialize call count
state->call_count = 0;

// Allocate test string using macro helper
const char *test_msg = "Allocated by module using kernel allocator (V1 macros)";
state->allocated_string = AK24_MODULE_STRDUP_V1(test_msg);
if (!state->allocated_string) {
  AK24_MODULE_INIT_ERROR_V1("Failed to allocate test string",
                            AK_MODULE_ERROR_INIT);
}

printf("[TEST_MODULE_MACROD] Initialized with V1 macros\n");
printf("[TEST_MODULE_MACROD] Test allocation: %s\n", state->allocated_string);
AK24_MODULE_INIT_END_V1()

//=============================================================================
// Module Deinitialization (with macro helpers)
//=============================================================================

AK24_MODULE_DEINIT_BEGIN_V1(module_state_t)
printf("[TEST_MODULE_MACROD] Deinitializing (calls: %d)\n", state->call_count);

// Free allocated string using macro helper
AK24_MODULE_FREE_V1(state->allocated_string);
AK24_MODULE_DEINIT_END_V1()

//=============================================================================
// Module Functions
//=============================================================================

static void test_process(void *args) {
  (void)args;
  printf("[TEST_MODULE_MACROD] Processing data\n");
}

static void test_sleep(void *args) {
  if (args) {
    int ms = *(int *)args;
    printf("[TEST_MODULE_MACROD] Sleeping for %d milliseconds\n", ms);
    usleep(ms * 1000);
    printf("[TEST_MODULE_MACROD] Woke up\n");
  }
}

static void test_allocate(void *args) {
  (void)args;

  // Demonstrate allocator usage with macro
  char *temp = AK24_MODULE_CALLOC_V1(char, 64);
  if (temp) {
    snprintf(temp, 64, "Dynamically allocated at runtime (V1 macros)");
    printf("[TEST_MODULE_MACROD] Allocated: %s\n", temp);
    AK24_MODULE_FREE_V1(temp);
  }
}

//=============================================================================
// Function Registration (auto-generates ak_module_get_function)
//=============================================================================

AK24_MODULE_FUNCTION_TABLE_BEGIN_V1()
AK24_MODULE_REGISTER_FUNCTION_V1("process", test_process)
AK24_MODULE_REGISTER_FUNCTION_V1("sleep", test_sleep)
AK24_MODULE_REGISTER_FUNCTION_V1("allocate", test_allocate)
AK24_MODULE_FUNCTION_TABLE_END_V1()

//=============================================================================
// Function Signatures (auto-generates ak_module_get_function_signature)
//=============================================================================

AK24_MODULE_SIGNATURE_TABLE_BEGIN_V1()
AK24_MODULE_SIGNATURE_VOID_PTR_V1("process", test_process)
AK24_MODULE_SIGNATURE_VOID_PTR_V1("sleep", test_sleep)
AK24_MODULE_SIGNATURE_VOID_PTR_V1("allocate", test_allocate)
AK24_MODULE_SIGNATURE_TABLE_END_V1()
