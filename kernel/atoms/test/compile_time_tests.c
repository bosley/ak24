#include "atom.h"
#include "kernel.h"
#include "test/assert.h"

static int test_atom_compile_time_basic(void) {
  ak_atom_value_u value = {.i32 = 42};
  ak_atom_t *atom = ak_atom_new(AK24_ATOM_I32, value, 3);
  AK24_TEST_ASSERT_NOT_NULL(atom);
  ak_atom_free(atom);
  AK24_TEST_PASS();
}

int run_atom_compile_time_tests(void) {
  AK24_TEST_RUN(test_atom_compile_time_basic);
  return 0;
}

int main(void) {
  ak_kernel_init();
  int result = run_atom_compile_time_tests();
  ak_kernel_deinit();
  return result;
}
