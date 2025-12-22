#include "cjit.h"
#include "filepath.h"
#include "test/assert.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int test_cjit_available(void) {
  AK24_TEST_ASSERT(ak_cjit_available());
  AK24_TEST_ASSERT_STR_EQ(ak_cjit_backend_name(), "tcc");
  AK24_TEST_PASS();
}

int test_cjit_config_default(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  AK24_TEST_ASSERT_EQ(config.include_path_count, 0);
  AK24_TEST_ASSERT_EQ(config.library_path_count, 0);
  AK24_TEST_ASSERT_EQ(config.library_count, 0);
  AK24_TEST_ASSERT_EQ(config.define_count, 0);
  AK24_TEST_PASS();
}

int test_cjit_error_strings(void) {
  AK24_TEST_ASSERT_STR_EQ(ak_cjit_error_string(AK_CJIT_OK), "Success");
  AK24_TEST_ASSERT_STR_EQ(ak_cjit_error_string(AK_CJIT_ERROR_ALLOC),
                          "Memory allocation failed");
  AK24_TEST_ASSERT_STR_EQ(ak_cjit_error_string(AK_CJIT_ERROR_COMPILE),
                          "Compilation failed");
  AK24_TEST_ASSERT_STR_EQ(ak_cjit_error_string(AK_CJIT_ERROR_RELOCATE),
                          "Relocation failed");
  AK24_TEST_ASSERT_STR_EQ(ak_cjit_error_string(AK_CJIT_ERROR_SYMBOL_NOT_FOUND),
                          "Symbol not found");
  AK24_TEST_ASSERT_STR_EQ(ak_cjit_error_string(AK_CJIT_ERROR_INVALID_STATE),
                          "Invalid state for operation");
  AK24_TEST_ASSERT_STR_EQ(ak_cjit_error_string(AK_CJIT_ERROR_NOT_AVAILABLE),
                          "CJIT not available");
  AK24_TEST_ASSERT_STR_EQ(ak_cjit_error_string((ak_cjit_error_e)999),
                          "Unknown error");
  AK24_TEST_PASS();
}

static char last_error_message[256] = {0};
static void *last_error_ctx = NULL;
static int error_callback_count = 0;

static void test_error_callback(void *ctx, const char *message) {
  last_error_ctx = ctx;
  error_callback_count++;
  strncpy(last_error_message, message, sizeof(last_error_message) - 1);
  last_error_message[sizeof(last_error_message) - 1] = '\0';
}

static void reset_error_state(void) {
  memset(last_error_message, 0, sizeof(last_error_message));
  last_error_ctx = NULL;
  error_callback_count = 0;
}

int test_cjit_unit_lifecycle(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);
  AK24_TEST_ASSERT_EQ(ak_cjit_unit_state(unit), AK_CJIT_STATE_INIT);
  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_cjit_unit_free_null(void) {
  ak_cjit_unit_free(NULL);
  AK24_TEST_PASS();
}

int test_cjit_unit_state_null(void) {
  ak_cjit_state_e state = ak_cjit_unit_state(NULL);
  AK24_TEST_ASSERT_EQ(state, AK_CJIT_STATE_ERROR);
  AK24_TEST_PASS();
}

int test_cjit_compile_simple_function(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, test_error_callback, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source = "int add(int a, int b) { return a + b; }";

  ak_cjit_error_e err = ak_cjit_add_source(unit, source, "test.c");
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_unit_state(unit), AK_CJIT_STATE_COMPILED);

  err = ak_cjit_relocate(unit);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_unit_state(unit), AK_CJIT_STATE_RELOCATED);

  typedef int (*add_fn)(int, int);
  add_fn add = (add_fn)ak_cjit_get_symbol(unit, "add");
  AK24_TEST_ASSERT_NOT_NULL(add);
  AK24_TEST_ASSERT_EQ(add(2, 3), 5);
  AK24_TEST_ASSERT_EQ(add(-1, 1), 0);
  AK24_TEST_ASSERT_EQ(add(100, 200), 300);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_cjit_add_source_null_checks(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  ak_cjit_error_e err = ak_cjit_add_source(NULL, "int x;", NULL);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_add_source(unit, NULL, NULL);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_cjit_compile_error(void) {
  reset_error_state();
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, test_error_callback, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *bad_source = "int broken( { return; }";
  ak_cjit_error_e err = ak_cjit_add_source(unit, bad_source, "broken.c");
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_COMPILE);
  AK24_TEST_ASSERT_EQ(ak_cjit_unit_state(unit), AK_CJIT_STATE_ERROR);
  AK24_TEST_ASSERT(strlen(last_error_message) > 0);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_cjit_get_symbol_not_relocated(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source = "int foo(void) { return 42; }";
  ak_cjit_error_e err = ak_cjit_add_source(unit, source, NULL);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  void *sym = ak_cjit_get_symbol(unit, "foo");
  AK24_TEST_ASSERT_NULL(sym);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_cjit_get_symbol_null_checks(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  void *sym = ak_cjit_get_symbol(NULL, "foo");
  AK24_TEST_ASSERT_NULL(sym);

  sym = ak_cjit_get_symbol(unit, NULL);
  AK24_TEST_ASSERT_NULL(sym);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_cjit_get_symbol_not_found(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source = "int foo(void) { return 42; }";
  ak_cjit_error_e err = ak_cjit_add_source(unit, source, NULL);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  err = ak_cjit_relocate(unit);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  void *sym = ak_cjit_get_symbol(unit, "nonexistent");
  AK24_TEST_ASSERT_NULL(sym);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_cjit_relocate_state_checks(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  ak_cjit_error_e err = ak_cjit_relocate(unit);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_relocate(NULL);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

static int host_multiply(int a, int b) { return a * b; }

int test_cjit_add_host_symbol(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, test_error_callback, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  ak_cjit_error_e err =
      ak_cjit_add_symbol(unit, "host_multiply", (void *)host_multiply);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  const char *source = "extern int host_multiply(int, int);\n"
                       "int use_host(int x, int y) {\n"
                       "  return host_multiply(x, y) + 1;\n"
                       "}\n";

  err = ak_cjit_add_source(unit, source, "host_test.c");
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  err = ak_cjit_relocate(unit);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  typedef int (*use_host_fn)(int, int);
  use_host_fn use_host = (use_host_fn)ak_cjit_get_symbol(unit, "use_host");
  AK24_TEST_ASSERT_NOT_NULL(use_host);
  AK24_TEST_ASSERT_EQ(use_host(3, 4), 13);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_cjit_add_symbol_null_checks(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  ak_cjit_error_e err = ak_cjit_add_symbol(NULL, "foo", (void *)host_multiply);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_add_symbol(unit, NULL, (void *)host_multiply);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_add_symbol(unit, "foo", NULL);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_cjit_define(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, test_error_callback, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  ak_cjit_error_e err = ak_cjit_define(unit, "MY_VALUE", "42");
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  err = ak_cjit_define(unit, "FEATURE_ENABLED", NULL);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  const char *source = "#ifdef FEATURE_ENABLED\n"
                       "int get_value(void) { return MY_VALUE; }\n"
                       "#else\n"
                       "int get_value(void) { return 0; }\n"
                       "#endif\n";

  err = ak_cjit_add_source(unit, source, "define_test.c");
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  err = ak_cjit_relocate(unit);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  typedef int (*get_value_fn)(void);
  get_value_fn get_value = (get_value_fn)ak_cjit_get_symbol(unit, "get_value");
  AK24_TEST_ASSERT_NOT_NULL(get_value);
  AK24_TEST_ASSERT_EQ(get_value(), 42);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_cjit_define_null_checks(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  ak_cjit_error_e err = ak_cjit_define(NULL, "FOO", "1");
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_define(unit, NULL, "1");
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_cjit_path_functions_null_checks(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  ak_cjit_error_e err = ak_cjit_add_include_path(NULL, "/some/path");
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_add_include_path(unit, NULL);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_add_library_path(NULL, "/some/path");
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_add_library_path(unit, NULL);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_add_library(NULL, "m");
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_add_library(unit, NULL);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_cjit_add_include_path(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  ak_buffer_t *temp = ak_filepath_temp();
  AK24_TEST_ASSERT_NOT_NULL(temp);

  ak_cjit_error_e err =
      ak_cjit_add_include_path(unit, (const char *)ak_buffer_data(temp));
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  ak_buffer_free(temp);
  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_cjit_add_library_path(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  ak_buffer_t *temp = ak_filepath_temp();
  AK24_TEST_ASSERT_NOT_NULL(temp);

  ak_cjit_error_e err =
      ak_cjit_add_library_path(unit, (const char *)ak_buffer_data(temp));
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  ak_buffer_free(temp);
  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_cjit_add_library(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  ak_cjit_error_e err = ak_cjit_add_library(unit, "m");
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_cjit_add_file_null_checks(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  ak_cjit_error_e err = ak_cjit_add_file(NULL, "/some/file.c");
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_add_file(unit, NULL);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_cjit_operations_after_relocate(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source = "int foo(void) { return 1; }";
  ak_cjit_error_e err = ak_cjit_add_source(unit, source, NULL);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  err = ak_cjit_relocate(unit);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  err = ak_cjit_add_source(unit, "int bar(void) { return 2; }", NULL);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_add_symbol(unit, "sym", (void *)host_multiply);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_add_include_path(unit, "/tmp");
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_add_library_path(unit, "/tmp");
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_add_library(unit, "m");
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_define(unit, "FOO", "1");
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_cjit_config_with_defines(void) {
  const char *defines[] = {"VALUE=100", "FLAG"};

  ak_cjit_config_t config = ak_cjit_config_default();
  config.defines = defines;
  config.define_count = 2;

  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, test_error_callback, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source = "#ifdef FLAG\n"
                       "int get_value(void) { return VALUE; }\n"
                       "#else\n"
                       "int get_value(void) { return 0; }\n"
                       "#endif\n";

  ak_cjit_error_e err = ak_cjit_add_source(unit, source, NULL);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  err = ak_cjit_relocate(unit);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  typedef int (*get_value_fn)(void);
  get_value_fn get_value = (get_value_fn)ak_cjit_get_symbol(unit, "get_value");
  AK24_TEST_ASSERT_NOT_NULL(get_value);
  AK24_TEST_ASSERT_EQ(get_value(), 100);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_cjit_multiple_functions(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, test_error_callback, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source = "int square(int x) { return x * x; }\n"
                       "int cube(int x) { return x * x * x; }\n"
                       "int add_one(int x) { return x + 1; }\n";

  ak_cjit_error_e err = ak_cjit_add_source(unit, source, NULL);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  err = ak_cjit_relocate(unit);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  typedef int (*int_fn)(int);
  int_fn square = (int_fn)ak_cjit_get_symbol(unit, "square");
  int_fn cube = (int_fn)ak_cjit_get_symbol(unit, "cube");
  int_fn add_one = (int_fn)ak_cjit_get_symbol(unit, "add_one");

  AK24_TEST_ASSERT_NOT_NULL(square);
  AK24_TEST_ASSERT_NOT_NULL(cube);
  AK24_TEST_ASSERT_NOT_NULL(add_one);

  AK24_TEST_ASSERT_EQ(square(5), 25);
  AK24_TEST_ASSERT_EQ(cube(3), 27);
  AK24_TEST_ASSERT_EQ(add_one(99), 100);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_cjit_global_variables(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, test_error_callback, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source = "int counter = 0;\n"
                       "void increment(void) { counter++; }\n"
                       "int get_counter(void) { return counter; }\n";

  ak_cjit_error_e err = ak_cjit_add_source(unit, source, NULL);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  err = ak_cjit_relocate(unit);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_OK);

  typedef void (*void_fn)(void);
  typedef int (*int_fn)(void);

  void_fn increment = (void_fn)ak_cjit_get_symbol(unit, "increment");
  int_fn get_counter = (int_fn)ak_cjit_get_symbol(unit, "get_counter");

  AK24_TEST_ASSERT_NOT_NULL(increment);
  AK24_TEST_ASSERT_NOT_NULL(get_counter);

  AK24_TEST_ASSERT_EQ(get_counter(), 0);
  increment();
  AK24_TEST_ASSERT_EQ(get_counter(), 1);
  increment();
  increment();
  AK24_TEST_ASSERT_EQ(get_counter(), 3);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

static int host_add(int a, int b) { return a + b; }
static int host_sub(int a, int b) { return a - b; }
static int host_mul(int a, int b) { return a * b; }

int test_error_callback_context(void) {
  reset_error_state();
  int ctx_value = 0xDEADBEEF;

  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit =
      ak_cjit_unit_new(&config, test_error_callback, &ctx_value);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  ak_cjit_add_source(unit, "int broken(", "bad.c");
  AK24_TEST_ASSERT_EQ(last_error_ctx, &ctx_value);
  AK24_TEST_ASSERT(error_callback_count > 0);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_operations_after_error_state(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  ak_cjit_add_source(unit, "int broken(", NULL);
  AK24_TEST_ASSERT_EQ(ak_cjit_unit_state(unit), AK_CJIT_STATE_ERROR);

  ak_cjit_error_e err =
      ak_cjit_add_source(unit, "int foo(void) { return 1; }", NULL);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_add_symbol(unit, "sym", (void *)host_add);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_add_include_path(unit, "/tmp");
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_add_library_path(unit, "/tmp");
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_add_library(unit, "m");
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  err = ak_cjit_define(unit, "FOO", "1");
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_multiple_host_symbols(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  AK24_TEST_ASSERT_EQ(ak_cjit_add_symbol(unit, "host_add", (void *)host_add),
                      AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_add_symbol(unit, "host_sub", (void *)host_sub),
                      AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_add_symbol(unit, "host_mul", (void *)host_mul),
                      AK_CJIT_OK);

  const char *source = "extern int host_add(int, int);\n"
                       "extern int host_sub(int, int);\n"
                       "extern int host_mul(int, int);\n"
                       "int compute(int a, int b) {\n"
                       "  return host_mul(host_add(a, b), host_sub(a, b));\n"
                       "}\n";

  AK24_TEST_ASSERT_EQ(ak_cjit_add_source(unit, source, NULL), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  typedef int (*compute_fn)(int, int);
  compute_fn compute = (compute_fn)ak_cjit_get_symbol(unit, "compute");
  AK24_TEST_ASSERT_NOT_NULL(compute);
  AK24_TEST_ASSERT_EQ(compute(5, 3), (5 + 3) * (5 - 3));

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_recursive_function(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source = "int factorial(int n) {\n"
                       "  if (n <= 1) return 1;\n"
                       "  return n * factorial(n - 1);\n"
                       "}\n";

  AK24_TEST_ASSERT_EQ(ak_cjit_add_source(unit, source, NULL), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  typedef int (*fact_fn)(int);
  fact_fn factorial = (fact_fn)ak_cjit_get_symbol(unit, "factorial");
  AK24_TEST_ASSERT_NOT_NULL(factorial);
  AK24_TEST_ASSERT_EQ(factorial(0), 1);
  AK24_TEST_ASSERT_EQ(factorial(1), 1);
  AK24_TEST_ASSERT_EQ(factorial(5), 120);
  AK24_TEST_ASSERT_EQ(factorial(10), 3628800);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_function_pointers(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source =
      "typedef int (*binop_fn)(int, int);\n"
      "int add(int a, int b) { return a + b; }\n"
      "int sub(int a, int b) { return a - b; }\n"
      "int apply(binop_fn fn, int a, int b) { return fn(a, b); }\n"
      "int test_apply(int which, int a, int b) {\n"
      "  binop_fn fn = which ? sub : add;\n"
      "  return apply(fn, a, b);\n"
      "}\n";

  AK24_TEST_ASSERT_EQ(ak_cjit_add_source(unit, source, NULL), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  typedef int (*test_fn)(int, int, int);
  test_fn test_apply = (test_fn)ak_cjit_get_symbol(unit, "test_apply");
  AK24_TEST_ASSERT_NOT_NULL(test_apply);
  AK24_TEST_ASSERT_EQ(test_apply(0, 10, 3), 13);
  AK24_TEST_ASSERT_EQ(test_apply(1, 10, 3), 7);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_static_functions_and_vars(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source =
      "static int hidden_value = 100;\n"
      "static int hidden_helper(int x) { return x * 2; }\n"
      "int public_func(int x) { return hidden_helper(x) + hidden_value; }\n";

  AK24_TEST_ASSERT_EQ(ak_cjit_add_source(unit, source, NULL), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  typedef int (*pub_fn)(int);
  pub_fn public_func = (pub_fn)ak_cjit_get_symbol(unit, "public_func");
  AK24_TEST_ASSERT_NOT_NULL(public_func);
  AK24_TEST_ASSERT_EQ(public_func(5), 110);

  void *hidden = ak_cjit_get_symbol(unit, "hidden_helper");
  AK24_TEST_ASSERT_NULL(hidden);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_struct_handling(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source =
      "typedef struct { int x; int y; } point_t;\n"
      "point_t make_point(int x, int y) { point_t p; p.x = x; p.y = y; return "
      "p; }\n"
      "int point_sum(point_t p) { return p.x + p.y; }\n"
      "int test_struct(int x, int y) { return point_sum(make_point(x, y)); }\n";

  AK24_TEST_ASSERT_EQ(ak_cjit_add_source(unit, source, NULL), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  typedef int (*test_fn)(int, int);
  test_fn test_struct = (test_fn)ak_cjit_get_symbol(unit, "test_struct");
  AK24_TEST_ASSERT_NOT_NULL(test_struct);
  AK24_TEST_ASSERT_EQ(test_struct(10, 20), 30);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_pointer_manipulation(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source =
      "void swap(int *a, int *b) { int t = *a; *a = *b; *b = t; }\n"
      "int sum_array(int *arr, int n) {\n"
      "  int s = 0;\n"
      "  for (int i = 0; i < n; i++) s += arr[i];\n"
      "  return s;\n"
      "}\n";

  AK24_TEST_ASSERT_EQ(ak_cjit_add_source(unit, source, NULL), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  typedef void (*swap_fn)(int *, int *);
  typedef int (*sum_fn)(int *, int);

  swap_fn swap = (swap_fn)ak_cjit_get_symbol(unit, "swap");
  sum_fn sum_array = (sum_fn)ak_cjit_get_symbol(unit, "sum_array");
  AK24_TEST_ASSERT_NOT_NULL(swap);
  AK24_TEST_ASSERT_NOT_NULL(sum_array);

  int a = 5, b = 10;
  swap(&a, &b);
  AK24_TEST_ASSERT_EQ(a, 10);
  AK24_TEST_ASSERT_EQ(b, 5);

  int arr[] = {1, 2, 3, 4, 5};
  AK24_TEST_ASSERT_EQ(sum_array(arr, 5), 15);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_string_handling(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source = "int str_len(const char *s) {\n"
                       "  int n = 0;\n"
                       "  while (s[n]) n++;\n"
                       "  return n;\n"
                       "}\n"
                       "char get_char(const char *s, int i) { return s[i]; }\n";

  AK24_TEST_ASSERT_EQ(ak_cjit_add_source(unit, source, NULL), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  typedef int (*len_fn)(const char *);
  typedef char (*getc_fn)(const char *, int);

  len_fn str_len = (len_fn)ak_cjit_get_symbol(unit, "str_len");
  getc_fn get_char = (getc_fn)ak_cjit_get_symbol(unit, "get_char");
  AK24_TEST_ASSERT_NOT_NULL(str_len);
  AK24_TEST_ASSERT_NOT_NULL(get_char);

  AK24_TEST_ASSERT_EQ(str_len("hello"), 5);
  AK24_TEST_ASSERT_EQ(str_len(""), 0);
  AK24_TEST_ASSERT_EQ(get_char("abc", 0), 'a');
  AK24_TEST_ASSERT_EQ(get_char("abc", 2), 'c');

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_multiple_defines(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  AK24_TEST_ASSERT_EQ(ak_cjit_define(unit, "A", "10"), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_define(unit, "B", "20"), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_define(unit, "C", "30"), AK_CJIT_OK);

  const char *source = "int get_sum(void) { return A + B + C; }\n";

  AK24_TEST_ASSERT_EQ(ak_cjit_add_source(unit, source, NULL), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  typedef int (*sum_fn)(void);
  sum_fn get_sum = (sum_fn)ak_cjit_get_symbol(unit, "get_sum");
  AK24_TEST_ASSERT_NOT_NULL(get_sum);
  AK24_TEST_ASSERT_EQ(get_sum(), 60);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_multiple_units_independent(void) {
  ak_cjit_config_t config = ak_cjit_config_default();

  ak_cjit_unit_t *unit1 = ak_cjit_unit_new(&config, NULL, NULL);
  ak_cjit_unit_t *unit2 = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit1);
  AK24_TEST_ASSERT_NOT_NULL(unit2);
  AK24_TEST_ASSERT_NEQ(unit1, unit2);

  AK24_TEST_ASSERT_EQ(
      ak_cjit_add_source(unit1, "int get(void) { return 1; }", NULL),
      AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(
      ak_cjit_add_source(unit2, "int get(void) { return 2; }", NULL),
      AK_CJIT_OK);

  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit1), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit2), AK_CJIT_OK);

  typedef int (*get_fn)(void);
  get_fn get1 = (get_fn)ak_cjit_get_symbol(unit1, "get");
  get_fn get2 = (get_fn)ak_cjit_get_symbol(unit2, "get");
  AK24_TEST_ASSERT_NOT_NULL(get1);
  AK24_TEST_ASSERT_NOT_NULL(get2);
  AK24_TEST_ASSERT_EQ(get1(), 1);
  AK24_TEST_ASSERT_EQ(get2(), 2);

  ak_cjit_unit_free(unit1);
  ak_cjit_unit_free(unit2);
  AK24_TEST_PASS();
}

int test_double_relocate(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  AK24_TEST_ASSERT_EQ(
      ak_cjit_add_source(unit, "int foo(void) { return 42; }", NULL),
      AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  ak_cjit_error_e err = ak_cjit_relocate(unit);
  AK24_TEST_ASSERT_EQ(err, AK_CJIT_ERROR_INVALID_STATE);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_unit_new_null_config(void) {
  ak_cjit_unit_t *unit = ak_cjit_unit_new(NULL, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);
  AK24_TEST_ASSERT_EQ(ak_cjit_unit_state(unit), AK_CJIT_STATE_INIT);

  AK24_TEST_ASSERT_EQ(
      ak_cjit_add_source(unit, "int x(void) { return 1; }", NULL), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  typedef int (*fn)(void);
  fn x = (fn)ak_cjit_get_symbol(unit, "x");
  AK24_TEST_ASSERT_NOT_NULL(x);
  AK24_TEST_ASSERT_EQ(x(), 1);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_nested_structs(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source = "typedef struct { int x, y; } vec2_t;\n"
                       "typedef struct { vec2_t min, max; } rect_t;\n"
                       "int rect_area(rect_t r) {\n"
                       "  int w = r.max.x - r.min.x;\n"
                       "  int h = r.max.y - r.min.y;\n"
                       "  return w * h;\n"
                       "}\n"
                       "int test_rect(void) {\n"
                       "  rect_t r;\n"
                       "  r.min.x = 0; r.min.y = 0;\n"
                       "  r.max.x = 10; r.max.y = 5;\n"
                       "  return rect_area(r);\n"
                       "}\n";

  AK24_TEST_ASSERT_EQ(ak_cjit_add_source(unit, source, NULL), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  typedef int (*fn)(void);
  fn test_rect = (fn)ak_cjit_get_symbol(unit, "test_rect");
  AK24_TEST_ASSERT_NOT_NULL(test_rect);
  AK24_TEST_ASSERT_EQ(test_rect(), 50);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_bitwise_operations(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source =
      "unsigned int set_bit(unsigned int v, int b) { return v | (1u << b); }\n"
      "unsigned int clear_bit(unsigned int v, int b) { return v & ~(1u << b); "
      "}\n"
      "int has_bit(unsigned int v, int b) { return (v >> b) & 1; }\n";

  AK24_TEST_ASSERT_EQ(ak_cjit_add_source(unit, source, NULL), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  typedef unsigned int (*bit_fn)(unsigned int, int);
  typedef int (*has_fn)(unsigned int, int);

  bit_fn set_bit = (bit_fn)ak_cjit_get_symbol(unit, "set_bit");
  bit_fn clear_bit = (bit_fn)ak_cjit_get_symbol(unit, "clear_bit");
  has_fn has_bit = (has_fn)ak_cjit_get_symbol(unit, "has_bit");

  AK24_TEST_ASSERT_NOT_NULL(set_bit);
  AK24_TEST_ASSERT_NOT_NULL(clear_bit);
  AK24_TEST_ASSERT_NOT_NULL(has_bit);

  AK24_TEST_ASSERT_EQ(set_bit(0, 3), 8);
  AK24_TEST_ASSERT_EQ(clear_bit(0xFF, 0), 0xFE);
  AK24_TEST_ASSERT_EQ(has_bit(8, 3), 1);
  AK24_TEST_ASSERT_EQ(has_bit(8, 2), 0);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_float_operations(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source = "float addf(float a, float b) { return a + b; }\n"
                       "double addd(double a, double b) { return a + b; }\n"
                       "int truncate(float f) { return (int)f; }\n";

  AK24_TEST_ASSERT_EQ(ak_cjit_add_source(unit, source, NULL), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  typedef float (*addf_fn)(float, float);
  typedef double (*addd_fn)(double, double);
  typedef int (*trunc_fn)(float);

  addf_fn addf = (addf_fn)ak_cjit_get_symbol(unit, "addf");
  addd_fn addd = (addd_fn)ak_cjit_get_symbol(unit, "addd");
  trunc_fn truncate_fn = (trunc_fn)ak_cjit_get_symbol(unit, "truncate");

  AK24_TEST_ASSERT_NOT_NULL(addf);
  AK24_TEST_ASSERT_NOT_NULL(addd);
  AK24_TEST_ASSERT_NOT_NULL(truncate_fn);

  float rf = addf(1.5f, 2.5f);
  AK24_TEST_ASSERT(rf > 3.9f && rf < 4.1f);

  double rd = addd(1.5, 2.5);
  AK24_TEST_ASSERT(rd > 3.9 && rd < 4.1);

  AK24_TEST_ASSERT_EQ(truncate_fn(3.9f), 3);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_variadic_like_pattern(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source =
      "int sum3(int a, int b, int c) { return a + b + c; }\n"
      "int sum5(int a, int b, int c, int d, int e) { return a+b+c+d+e; }\n";

  AK24_TEST_ASSERT_EQ(ak_cjit_add_source(unit, source, NULL), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  typedef int (*sum3_fn)(int, int, int);
  typedef int (*sum5_fn)(int, int, int, int, int);

  sum3_fn sum3 = (sum3_fn)ak_cjit_get_symbol(unit, "sum3");
  sum5_fn sum5 = (sum5_fn)ak_cjit_get_symbol(unit, "sum5");

  AK24_TEST_ASSERT_NOT_NULL(sum3);
  AK24_TEST_ASSERT_NOT_NULL(sum5);
  AK24_TEST_ASSERT_EQ(sum3(1, 2, 3), 6);
  AK24_TEST_ASSERT_EQ(sum5(1, 2, 3, 4, 5), 15);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

static int host_global_value = 999;

int test_host_global_variable(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  AK24_TEST_ASSERT_EQ(
      ak_cjit_add_symbol(unit, "host_global", &host_global_value), AK_CJIT_OK);

  const char *source = "extern int host_global;\n"
                       "int get_host_global(void) { return host_global; }\n"
                       "void set_host_global(int v) { host_global = v; }\n";

  AK24_TEST_ASSERT_EQ(ak_cjit_add_source(unit, source, NULL), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  typedef int (*get_fn)(void);
  typedef void (*set_fn)(int);

  get_fn get_host_global = (get_fn)ak_cjit_get_symbol(unit, "get_host_global");
  set_fn set_host_global = (set_fn)ak_cjit_get_symbol(unit, "set_host_global");

  AK24_TEST_ASSERT_NOT_NULL(get_host_global);
  AK24_TEST_ASSERT_NOT_NULL(set_host_global);

  AK24_TEST_ASSERT_EQ(get_host_global(), 999);

  set_host_global(1234);
  AK24_TEST_ASSERT_EQ(host_global_value, 1234);
  AK24_TEST_ASSERT_EQ(get_host_global(), 1234);

  host_global_value = 999;

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_switch_statement(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source = "int classify(int x) {\n"
                       "  switch (x) {\n"
                       "    case 0: return 100;\n"
                       "    case 1: return 200;\n"
                       "    case 2: return 300;\n"
                       "    default: return -1;\n"
                       "  }\n"
                       "}\n";

  AK24_TEST_ASSERT_EQ(ak_cjit_add_source(unit, source, NULL), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  typedef int (*classify_fn)(int);
  classify_fn classify = (classify_fn)ak_cjit_get_symbol(unit, "classify");
  AK24_TEST_ASSERT_NOT_NULL(classify);

  AK24_TEST_ASSERT_EQ(classify(0), 100);
  AK24_TEST_ASSERT_EQ(classify(1), 200);
  AK24_TEST_ASSERT_EQ(classify(2), 300);
  AK24_TEST_ASSERT_EQ(classify(99), -1);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_ternary_operator(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source = "int max(int a, int b) { return a > b ? a : b; }\n"
                       "int min(int a, int b) { return a < b ? a : b; }\n"
                       "int clamp(int v, int lo, int hi) { return v < lo ? lo "
                       ": (v > hi ? hi : v); }\n";

  AK24_TEST_ASSERT_EQ(ak_cjit_add_source(unit, source, NULL), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  typedef int (*binop)(int, int);
  typedef int (*triop)(int, int, int);

  binop max_fn = (binop)ak_cjit_get_symbol(unit, "max");
  binop min_fn = (binop)ak_cjit_get_symbol(unit, "min");
  triop clamp = (triop)ak_cjit_get_symbol(unit, "clamp");

  AK24_TEST_ASSERT_NOT_NULL(max_fn);
  AK24_TEST_ASSERT_NOT_NULL(min_fn);
  AK24_TEST_ASSERT_NOT_NULL(clamp);

  AK24_TEST_ASSERT_EQ(max_fn(5, 10), 10);
  AK24_TEST_ASSERT_EQ(min_fn(5, 10), 5);
  AK24_TEST_ASSERT_EQ(clamp(5, 0, 10), 5);
  AK24_TEST_ASSERT_EQ(clamp(-5, 0, 10), 0);
  AK24_TEST_ASSERT_EQ(clamp(15, 0, 10), 10);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_array_initialization(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source = "int lookup(int i) {\n"
                       "  int table[5] = {10, 20, 30, 40, 50};\n"
                       "  if (i < 0 || i >= 5) return -1;\n"
                       "  return table[i];\n"
                       "}\n";

  AK24_TEST_ASSERT_EQ(ak_cjit_add_source(unit, source, NULL), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  typedef int (*lookup_fn)(int);
  lookup_fn lookup = (lookup_fn)ak_cjit_get_symbol(unit, "lookup");
  AK24_TEST_ASSERT_NOT_NULL(lookup);

  AK24_TEST_ASSERT_EQ(lookup(0), 10);
  AK24_TEST_ASSERT_EQ(lookup(4), 50);
  AK24_TEST_ASSERT_EQ(lookup(-1), -1);
  AK24_TEST_ASSERT_EQ(lookup(10), -1);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_enum_handling(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source = "enum Color { RED = 1, GREEN = 2, BLUE = 4 };\n"
                       "int color_value(enum Color c) { return (int)c; }\n"
                       "int combined(void) { return RED | GREEN | BLUE; }\n";

  AK24_TEST_ASSERT_EQ(ak_cjit_add_source(unit, source, NULL), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  typedef int (*color_fn)(int);
  typedef int (*comb_fn)(void);

  color_fn color_value = (color_fn)ak_cjit_get_symbol(unit, "color_value");
  comb_fn combined = (comb_fn)ak_cjit_get_symbol(unit, "combined");

  AK24_TEST_ASSERT_NOT_NULL(color_value);
  AK24_TEST_ASSERT_NOT_NULL(combined);

  AK24_TEST_ASSERT_EQ(color_value(1), 1);
  AK24_TEST_ASSERT_EQ(color_value(2), 2);
  AK24_TEST_ASSERT_EQ(combined(), 7);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_void_return(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source = "int side_effect_value = 0;\n"
                       "void set_value(int v) { side_effect_value = v; }\n"
                       "int get_value(void) { return side_effect_value; }\n";

  AK24_TEST_ASSERT_EQ(ak_cjit_add_source(unit, source, NULL), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  typedef void (*set_fn)(int);
  typedef int (*get_fn)(void);

  set_fn set_value = (set_fn)ak_cjit_get_symbol(unit, "set_value");
  get_fn get_value = (get_fn)ak_cjit_get_symbol(unit, "get_value");

  AK24_TEST_ASSERT_NOT_NULL(set_value);
  AK24_TEST_ASSERT_NOT_NULL(get_value);

  AK24_TEST_ASSERT_EQ(get_value(), 0);
  set_value(42);
  AK24_TEST_ASSERT_EQ(get_value(), 42);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int test_sizeof_operator(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  ak_cjit_unit_t *unit = ak_cjit_unit_new(&config, NULL, NULL);
  AK24_TEST_ASSERT_NOT_NULL(unit);

  const char *source = "int size_int(void) { return (int)sizeof(int); }\n"
                       "int size_ptr(void) { return (int)sizeof(void*); }\n"
                       "int size_char(void) { return (int)sizeof(char); }\n";

  AK24_TEST_ASSERT_EQ(ak_cjit_add_source(unit, source, NULL), AK_CJIT_OK);
  AK24_TEST_ASSERT_EQ(ak_cjit_relocate(unit), AK_CJIT_OK);

  typedef int (*size_fn)(void);

  size_fn size_int = (size_fn)ak_cjit_get_symbol(unit, "size_int");
  size_fn size_ptr = (size_fn)ak_cjit_get_symbol(unit, "size_ptr");
  size_fn size_char = (size_fn)ak_cjit_get_symbol(unit, "size_char");

  AK24_TEST_ASSERT_NOT_NULL(size_int);
  AK24_TEST_ASSERT_NOT_NULL(size_ptr);
  AK24_TEST_ASSERT_NOT_NULL(size_char);

  AK24_TEST_ASSERT_EQ(size_int(), (int)sizeof(int));
  AK24_TEST_ASSERT_EQ(size_ptr(), (int)sizeof(void *));
  AK24_TEST_ASSERT_EQ(size_char(), 1);

  ak_cjit_unit_free(unit);
  AK24_TEST_PASS();
}

int main(void) {
  ak_kernel_init("ak24-cjit-test");

  AK24_TEST_RUN(test_cjit_available);
  AK24_TEST_RUN(test_cjit_config_default);
  AK24_TEST_RUN(test_cjit_error_strings);
  AK24_TEST_RUN(test_cjit_unit_lifecycle);
  AK24_TEST_RUN(test_cjit_unit_free_null);
  AK24_TEST_RUN(test_cjit_unit_state_null);
  AK24_TEST_RUN(test_cjit_compile_simple_function);
  AK24_TEST_RUN(test_cjit_add_source_null_checks);
  AK24_TEST_RUN(test_cjit_compile_error);
  AK24_TEST_RUN(test_cjit_get_symbol_not_relocated);
  AK24_TEST_RUN(test_cjit_get_symbol_null_checks);
  AK24_TEST_RUN(test_cjit_get_symbol_not_found);
  AK24_TEST_RUN(test_cjit_relocate_state_checks);
  AK24_TEST_RUN(test_cjit_add_host_symbol);
  AK24_TEST_RUN(test_cjit_add_symbol_null_checks);
  AK24_TEST_RUN(test_cjit_define);
  AK24_TEST_RUN(test_cjit_define_null_checks);
  AK24_TEST_RUN(test_cjit_path_functions_null_checks);
  AK24_TEST_RUN(test_cjit_add_include_path);
  AK24_TEST_RUN(test_cjit_add_library_path);
  AK24_TEST_RUN(test_cjit_add_library);
  AK24_TEST_RUN(test_cjit_add_file_null_checks);
  AK24_TEST_RUN(test_cjit_operations_after_relocate);
  AK24_TEST_RUN(test_cjit_config_with_defines);
  AK24_TEST_RUN(test_cjit_multiple_functions);
  AK24_TEST_RUN(test_cjit_global_variables);

  AK24_TEST_RUN(test_error_callback_context);
  AK24_TEST_RUN(test_operations_after_error_state);
  AK24_TEST_RUN(test_multiple_host_symbols);
  AK24_TEST_RUN(test_recursive_function);
  AK24_TEST_RUN(test_function_pointers);
  AK24_TEST_RUN(test_static_functions_and_vars);
  AK24_TEST_RUN(test_struct_handling);
  AK24_TEST_RUN(test_pointer_manipulation);
  AK24_TEST_RUN(test_string_handling);
  AK24_TEST_RUN(test_multiple_defines);
  AK24_TEST_RUN(test_multiple_units_independent);
  AK24_TEST_RUN(test_double_relocate);
  AK24_TEST_RUN(test_unit_new_null_config);
  AK24_TEST_RUN(test_nested_structs);
  AK24_TEST_RUN(test_bitwise_operations);
  AK24_TEST_RUN(test_float_operations);
  AK24_TEST_RUN(test_variadic_like_pattern);
  AK24_TEST_RUN(test_host_global_variable);
  AK24_TEST_RUN(test_switch_statement);
  AK24_TEST_RUN(test_ternary_operator);
  AK24_TEST_RUN(test_array_initialization);
  AK24_TEST_RUN(test_enum_handling);
  AK24_TEST_RUN(test_void_return);
  AK24_TEST_RUN(test_sizeof_operator);

  ak_kernel_deinit();
  return 0;
}
