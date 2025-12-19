#include "scanner.h"
#include "test/assert.h"

extern int run_scanner_tests(void);
extern int run_scanner_find_group_tests(void);
extern int run_scanner_stress_tests(void);

int main(void) {
  AK24_TEST_RUN(run_scanner_tests);
  AK24_TEST_RUN(run_scanner_find_group_tests);
  AK24_TEST_RUN(run_scanner_stress_tests);
  return 0;
}
