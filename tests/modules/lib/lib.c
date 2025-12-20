/**
 * @file lib.c
 * @brief AK24 test module implementation
 *
 * Demonstrates proper module ABI with allocator usage
 */

#include "lib.h"
#include <interfaces.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Module internal state
typedef struct {
  ak_module_allocator_t *allocator;
  char *allocated_string; // Test allocator usage
  int call_count;
} module_state_t;

// Static allocator reference
static ak_module_allocator_t *g_allocator = NULL;

/**
 * @brief Module API version (required export)
 */
int ak_module_version(void) { return AK24_MODULE_API_VERSION; }

/**
 * @brief Module initialization (required export)
 */
ak_module_result_e ak_module_init(void **module_ctx,
                                  ak_module_allocator_t *allocator,
                                  const char **error) {
  if (!module_ctx || !allocator) {
    if (error)
      *error = "Invalid init parameters";
    return AK_MODULE_ERROR_INIT;
  }

  // Store allocator
  g_allocator = allocator;

  // Allocate module state using provided allocator
  module_state_t *state =
      (module_state_t *)allocator->alloc(sizeof(module_state_t));
  if (!state) {
    if (error)
      *error = "Failed to allocate module state";
    return AK_MODULE_ERROR_INIT;
  }

  state->allocator = allocator;
  state->call_count = 0;

  // Allocate a test string using the allocator
  const char *test_msg = "Allocated by module using kernel allocator";
  state->allocated_string = (char *)allocator->alloc(strlen(test_msg) + 1);
  if (!state->allocated_string) {
    allocator->free(state);
    if (error)
      *error = "Failed to allocate test string";
    return AK_MODULE_ERROR_INIT;
  }
  strcpy(state->allocated_string, test_msg);

  *module_ctx = state;

  printf("[TEST_MODULE] Initialized with allocator\n");
  printf("[TEST_MODULE] Test allocation: %s\n", state->allocated_string);

  return AK_MODULE_OK;
}

/**
 * @brief Module deinitialization (required export)
 */
void ak_module_deinit(void *module_ctx) {
  if (!module_ctx) {
    return;
  }

  module_state_t *state = (module_state_t *)module_ctx;

  printf("[TEST_MODULE] Deinitializing (calls: %d)\n", state->call_count);

  // Free allocated string using module allocator
  if (state->allocated_string) {
    state->allocator->free(state->allocated_string);
  }

  // Free module state
  state->allocator->free(state);
}

/**
 * @brief Module info query (required export)
 */
const char *ak_module_info(const char *key) {
  if (!key) {
    return "test_module";
  }

  if (strcmp(key, "name") == 0) {
    return "test_module";
  } else if (strcmp(key, "version") == 0) {
    return "1.0.0";
  } else if (strcmp(key, "description") == 0) {
    return "AK24 integration test module with allocator";
  }

  return NULL;
}

// Module function implementations

static void test_process(void *args) {
  (void)args;
  printf("[TEST_MODULE] Processing data\n");
}

static void test_sleep(void *args) {
  if (args) {
    int ms = *(int *)args;
    printf("[TEST_MODULE] Sleeping for %d milliseconds\n", ms);
    usleep(ms * 1000);
    printf("[TEST_MODULE] Woke up\n");
  }
}

static void test_allocate(void *args) {
  (void)args;
  if (!g_allocator) {
    printf("[TEST_MODULE] No allocator available\n");
    return;
  }

  // Demonstrate allocator usage
  char *temp = (char *)g_allocator->alloc(64);
  if (temp) {
    snprintf(temp, 64, "Dynamically allocated at runtime");
    printf("[TEST_MODULE] Allocated: %s\n", temp);
    g_allocator->free(temp);
  }
}

/**
 * @brief Get module function by name (optional export)
 */
void *ak_module_get_function(void *module_ctx, const char *name) {
  if (!module_ctx || !name) {
    return NULL;
  }

  module_state_t *state = (module_state_t *)module_ctx;
  state->call_count++;

  if (strcmp(name, "process") == 0) {
    return (void *)test_process;
  } else if (strcmp(name, "sleep") == 0) {
    return (void *)test_sleep;
  } else if (strcmp(name, "allocate") == 0) {
    return (void *)test_allocate;
  }

  return NULL;
}

/**
 * @brief Get function signature metadata (optional export)
 */
ak_function_signature_t *ak_module_get_function_signature(void *module_ctx,
                                                          const char *name) {
  if (!module_ctx || !name) {
    return NULL;
  }

  if (!g_allocator) {
    return NULL;
  }

  // Allocate signature structure
  ak_function_signature_t *sig = (ak_function_signature_t *)g_allocator->alloc(
      sizeof(ak_function_signature_t));
  if (!sig) {
    return NULL;
  }

  if (strcmp(name, "process") == 0) {
    // void process(void *)
    sig->function_name = "process";
    sig->function_ptr = (void *)test_process;
    sig->return_type = (ak_param_metadata_t){.base_type = AK_PARAM_TYPE_VOID,
                                             .type_name = NULL,
                                             .ptr_depth = 0,
                                             .is_const = false,
                                             .is_array = false};
    sig->param_count = 1;
    sig->params =
        (ak_param_metadata_t *)g_allocator->alloc(sizeof(ak_param_metadata_t));
    if (!sig->params) {
      g_allocator->free(sig);
      return NULL;
    }
    sig->params[0] = (ak_param_metadata_t){.base_type = AK_PARAM_TYPE_VOID,
                                           .type_name = NULL,
                                           .ptr_depth = 1,
                                           .is_const = false,
                                           .is_array = false};
  } else if (strcmp(name, "sleep") == 0) {
    // void sleep(void *)
    sig->function_name = "sleep";
    sig->function_ptr = (void *)test_sleep;
    sig->return_type = (ak_param_metadata_t){.base_type = AK_PARAM_TYPE_VOID,
                                             .type_name = NULL,
                                             .ptr_depth = 0,
                                             .is_const = false,
                                             .is_array = false};
    sig->param_count = 1;
    sig->params =
        (ak_param_metadata_t *)g_allocator->alloc(sizeof(ak_param_metadata_t));
    if (!sig->params) {
      g_allocator->free(sig);
      return NULL;
    }
    sig->params[0] = (ak_param_metadata_t){.base_type = AK_PARAM_TYPE_VOID,
                                           .type_name = NULL,
                                           .ptr_depth = 1,
                                           .is_const = false,
                                           .is_array = false};
  } else if (strcmp(name, "allocate") == 0) {
    // void allocate(void *)
    sig->function_name = "allocate";
    sig->function_ptr = (void *)test_allocate;
    sig->return_type = (ak_param_metadata_t){.base_type = AK_PARAM_TYPE_VOID,
                                             .type_name = NULL,
                                             .ptr_depth = 0,
                                             .is_const = false,
                                             .is_array = false};
    sig->param_count = 1;
    sig->params =
        (ak_param_metadata_t *)g_allocator->alloc(sizeof(ak_param_metadata_t));
    if (!sig->params) {
      g_allocator->free(sig);
      return NULL;
    }
    sig->params[0] = (ak_param_metadata_t){.base_type = AK_PARAM_TYPE_VOID,
                                           .type_name = NULL,
                                           .ptr_depth = 1,
                                           .is_const = false,
                                           .is_array = false};
  } else {
    g_allocator->free(sig);
    return NULL;
  }

  return sig;
}
