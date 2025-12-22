#include "cjit.h"
#include "filepath.h"
#include "test/assert.h"
#include <stdio.h>
#include <string.h>

int test_cjit_available(void) {
  AK24_TEST_ASSERT(ak_cjit_available());
  AK24_TEST_ASSERT_STR_EQ(ak_cjit_backend_name(), "tcc");
  AK24_TEST_PASS();
}

int test_cjit_config_default(void) {
  ak_cjit_config_t config = ak_cjit_config_default();
  AK24_TEST_ASSERT_EQ(config.debug_symbols, false);
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

static void test_error_callback(void *ctx, const char *message) {
  (void)ctx;
  strncpy(last_error_message, message, sizeof(last_error_message) - 1);
  last_error_message[sizeof(last_error_message) - 1] = '\0';
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
  memset(last_error_message, 0, sizeof(last_error_message));
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

  ak_kernel_deinit();
  return 0;
}
