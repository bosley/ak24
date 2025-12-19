#include "scanner.h"
#include "test/assert.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test_large_valid_buffer_mixed_types(void) {
  ak_buffer_t *buffer = ak_buffer_new(2048);
  AK24_TEST_ASSERT(buffer != NULL);

  uint8_t data[] = "alpha 42 beta -17 3.14 gamma +99 delta -2.5 epsilon 0 zeta "
                   "100 eta 0.001 theta -999 iota 42.42 kappa +1 lambda -1 "
                   "mu 3.14159 nu +0 xi -0 omicron 1.0 pi 2.0 rho 3.0 "
                   "sigma 4.0 tau 5.0 upsilon 6.0 phi 7.0 chi 8.0 psi 9.0 "
                   "omega 10.0 var1 11 var2 12 var3 13 var4 14 var5 15";

  ak_buffer_copy_to(buffer, data, strlen((char *)data));
  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  typedef struct {
    ak_static_base_e expected_type;
    const char *expected_value;
  } expected_token_t;

  expected_token_t expected[] = {
      {AK24_STATIC_BASE_SYMBOL, "alpha"}, {AK24_STATIC_BASE_INTEGER, "42"},
      {AK24_STATIC_BASE_SYMBOL, "beta"},  {AK24_STATIC_BASE_INTEGER, "-17"},
      {AK24_STATIC_BASE_REAL, "3.14"},    {AK24_STATIC_BASE_SYMBOL, "gamma"},
      {AK24_STATIC_BASE_INTEGER, "+99"},  {AK24_STATIC_BASE_SYMBOL, "delta"},
      {AK24_STATIC_BASE_REAL, "-2.5"},    {AK24_STATIC_BASE_SYMBOL, "epsilon"},
      {AK24_STATIC_BASE_INTEGER, "0"},    {AK24_STATIC_BASE_SYMBOL, "zeta"},
      {AK24_STATIC_BASE_INTEGER, "100"},  {AK24_STATIC_BASE_SYMBOL, "eta"},
      {AK24_STATIC_BASE_REAL, "0.001"},   {AK24_STATIC_BASE_SYMBOL, "theta"},
      {AK24_STATIC_BASE_INTEGER, "-999"}, {AK24_STATIC_BASE_SYMBOL, "iota"},
      {AK24_STATIC_BASE_REAL, "42.42"},   {AK24_STATIC_BASE_SYMBOL, "kappa"},
      {AK24_STATIC_BASE_INTEGER, "+1"},   {AK24_STATIC_BASE_SYMBOL, "lambda"},
      {AK24_STATIC_BASE_INTEGER, "-1"},   {AK24_STATIC_BASE_SYMBOL, "mu"},
      {AK24_STATIC_BASE_REAL, "3.14159"}, {AK24_STATIC_BASE_SYMBOL, "nu"},
      {AK24_STATIC_BASE_INTEGER, "+0"},   {AK24_STATIC_BASE_SYMBOL, "xi"},
      {AK24_STATIC_BASE_INTEGER, "-0"},   {AK24_STATIC_BASE_SYMBOL, "omicron"},
      {AK24_STATIC_BASE_REAL, "1.0"},     {AK24_STATIC_BASE_SYMBOL, "pi"},
      {AK24_STATIC_BASE_REAL, "2.0"},     {AK24_STATIC_BASE_SYMBOL, "rho"},
      {AK24_STATIC_BASE_REAL, "3.0"},     {AK24_STATIC_BASE_SYMBOL, "sigma"},
      {AK24_STATIC_BASE_REAL, "4.0"},     {AK24_STATIC_BASE_SYMBOL, "tau"},
      {AK24_STATIC_BASE_REAL, "5.0"},     {AK24_STATIC_BASE_SYMBOL, "upsilon"},
      {AK24_STATIC_BASE_REAL, "6.0"},     {AK24_STATIC_BASE_SYMBOL, "phi"},
      {AK24_STATIC_BASE_REAL, "7.0"},     {AK24_STATIC_BASE_SYMBOL, "chi"},
      {AK24_STATIC_BASE_REAL, "8.0"},     {AK24_STATIC_BASE_SYMBOL, "psi"},
      {AK24_STATIC_BASE_REAL, "9.0"},     {AK24_STATIC_BASE_SYMBOL, "omega"},
      {AK24_STATIC_BASE_REAL, "10.0"},    {AK24_STATIC_BASE_SYMBOL, "var1"},
      {AK24_STATIC_BASE_INTEGER, "11"},   {AK24_STATIC_BASE_SYMBOL, "var2"},
      {AK24_STATIC_BASE_INTEGER, "12"},   {AK24_STATIC_BASE_SYMBOL, "var3"},
      {AK24_STATIC_BASE_INTEGER, "13"},   {AK24_STATIC_BASE_SYMBOL, "var4"},
      {AK24_STATIC_BASE_INTEGER, "14"},   {AK24_STATIC_BASE_SYMBOL, "var5"},
      {AK24_STATIC_BASE_INTEGER, "15"}};

  size_t num_expected = sizeof(expected) / sizeof(expected[0]);

  for (size_t i = 0; i < num_expected; i++) {
    ak_scanner_static_type_result_t result =
        ak_scanner_read_static_base_type(scanner, NULL);

    AK24_TEST_ASSERT(result.success);
    AK24_TEST_ASSERT(result.data.base == expected[i].expected_type);

    size_t expected_len = strlen(expected[i].expected_value);
    AK24_TEST_ASSERT(result.data.byte_length == expected_len);
    AK24_TEST_ASSERT(memcmp(result.data.data, expected[i].expected_value,
                            expected_len) == 0);
  }

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_large_buffer_with_whitespace_variations(void) {
  ak_buffer_t *buffer = ak_buffer_new(2048);
  AK24_TEST_ASSERT(buffer != NULL);

  uint8_t data[] = "  \t\n  a1   \t  42  \n\n  b2\t\t-17\n   3.14   \t\n"
                   "c3    +99     d4\t\t-2.5\n\ne5\t0\tf6\n100\tg7\r\n"
                   "0.001  \t  h8    -999\ni9\t\t42.42   j10\t+1\n\n"
                   "k11  -1  l12\t3.14159\tm13\n+0\tn14  -0  o15\t1.0";

  ak_buffer_copy_to(buffer, data, strlen((char *)data));
  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  typedef struct {
    ak_static_base_e expected_type;
    const char *expected_value;
  } expected_token_t;

  expected_token_t expected[] = {
      {AK24_STATIC_BASE_SYMBOL, "a1"},    {AK24_STATIC_BASE_INTEGER, "42"},
      {AK24_STATIC_BASE_SYMBOL, "b2"},    {AK24_STATIC_BASE_INTEGER, "-17"},
      {AK24_STATIC_BASE_REAL, "3.14"},    {AK24_STATIC_BASE_SYMBOL, "c3"},
      {AK24_STATIC_BASE_INTEGER, "+99"},  {AK24_STATIC_BASE_SYMBOL, "d4"},
      {AK24_STATIC_BASE_REAL, "-2.5"},    {AK24_STATIC_BASE_SYMBOL, "e5"},
      {AK24_STATIC_BASE_INTEGER, "0"},    {AK24_STATIC_BASE_SYMBOL, "f6"},
      {AK24_STATIC_BASE_INTEGER, "100"},  {AK24_STATIC_BASE_SYMBOL, "g7"},
      {AK24_STATIC_BASE_REAL, "0.001"},   {AK24_STATIC_BASE_SYMBOL, "h8"},
      {AK24_STATIC_BASE_INTEGER, "-999"}, {AK24_STATIC_BASE_SYMBOL, "i9"},
      {AK24_STATIC_BASE_REAL, "42.42"},   {AK24_STATIC_BASE_SYMBOL, "j10"},
      {AK24_STATIC_BASE_INTEGER, "+1"},   {AK24_STATIC_BASE_SYMBOL, "k11"},
      {AK24_STATIC_BASE_INTEGER, "-1"},   {AK24_STATIC_BASE_SYMBOL, "l12"},
      {AK24_STATIC_BASE_REAL, "3.14159"}, {AK24_STATIC_BASE_SYMBOL, "m13"},
      {AK24_STATIC_BASE_INTEGER, "+0"},   {AK24_STATIC_BASE_SYMBOL, "n14"},
      {AK24_STATIC_BASE_INTEGER, "-0"},   {AK24_STATIC_BASE_SYMBOL, "o15"},
      {AK24_STATIC_BASE_REAL, "1.0"}};

  size_t num_expected = sizeof(expected) / sizeof(expected[0]);

  for (size_t i = 0; i < num_expected; i++) {
    ak_scanner_static_type_result_t result =
        ak_scanner_read_static_base_type(scanner, NULL);

    AK24_TEST_ASSERT(result.success);
    AK24_TEST_ASSERT(result.data.base == expected[i].expected_type);

    size_t expected_len = strlen(expected[i].expected_value);
    AK24_TEST_ASSERT(result.data.byte_length == expected_len);
    AK24_TEST_ASSERT(memcmp(result.data.data, expected[i].expected_value,
                            expected_len) == 0);
  }

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_large_buffer_error_at_start(void) {
  ak_buffer_t *buffer = ak_buffer_new(2048);
  AK24_TEST_ASSERT(buffer != NULL);

  uint8_t data[] = "123abc alpha 42 beta gamma";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(!result.success);
  AK24_TEST_ASSERT(result.start_position == 0);
  AK24_TEST_ASSERT(result.error_position == 3);
  AK24_TEST_ASSERT(scanner->position == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_double_period_in_real(void) {
  ak_buffer_t *buffer = ak_buffer_new(64);
  AK24_TEST_ASSERT(buffer != NULL);

  uint8_t data[] = "1.2.3";
  ak_buffer_copy_to(buffer, data, strlen((char *)data));

  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  ak_scanner_static_type_result_t result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(!result.success);
  AK24_TEST_ASSERT(result.start_position == 0);
  AK24_TEST_ASSERT(result.error_position == 3);
  AK24_TEST_ASSERT(scanner->position == 0);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_large_buffer_error_in_middle(void) {
  ak_buffer_t *buffer = ak_buffer_new(2048);
  AK24_TEST_ASSERT(buffer != NULL);

  uint8_t data[] = "alpha 42 beta -17 3.14 gamma +99 delta -2.5 epsilon 0 zeta "
                   "100 eta 0.001 theta 123x iota 42.42 kappa +1 lambda";

  ak_buffer_copy_to(buffer, data, strlen((char *)data));
  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  for (int i = 0; i < 16; i++) {
    ak_scanner_static_type_result_t result =
        ak_scanner_read_static_base_type(scanner, NULL);
    AK24_TEST_ASSERT(result.success);
  }

  size_t pos_before_error = scanner->position;

  ak_scanner_static_type_result_t error_result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(!error_result.success);
  AK24_TEST_ASSERT(error_result.start_position == pos_before_error);
  AK24_TEST_ASSERT(error_result.error_position > pos_before_error);
  AK24_TEST_ASSERT(scanner->position == pos_before_error);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_large_buffer_error_at_end(void) {
  ak_buffer_t *buffer = ak_buffer_new(2048);
  AK24_TEST_ASSERT(buffer != NULL);

  uint8_t data[] = "alpha 42 beta -17 3.14 gamma +99 delta -2.5 epsilon 0 zeta "
                   "100 eta 0.001 theta -999 iota 42.42 kappa 5.5x";

  ak_buffer_copy_to(buffer, data, strlen((char *)data));
  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  for (int i = 0; i < 20; i++) {
    ak_scanner_static_type_result_t result =
        ak_scanner_read_static_base_type(scanner, NULL);
    AK24_TEST_ASSERT(result.success);
  }

  size_t pos_before_error = scanner->position;

  ak_scanner_static_type_result_t error_result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(!error_result.success);
  AK24_TEST_ASSERT(error_result.start_position == pos_before_error);
  AK24_TEST_ASSERT(error_result.error_position > pos_before_error);
  AK24_TEST_ASSERT(scanner->position == pos_before_error);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_large_buffer_invalid_integer_in_sequence(void) {
  ak_buffer_t *buffer = ak_buffer_new(2048);
  AK24_TEST_ASSERT(buffer != NULL);

  uint8_t data[] = "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15x 16 17 18 19 20";

  ak_buffer_copy_to(buffer, data, strlen((char *)data));
  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  for (int i = 0; i < 14; i++) {
    ak_scanner_static_type_result_t result =
        ak_scanner_read_static_base_type(scanner, NULL);
    AK24_TEST_ASSERT(result.success);
    AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_INTEGER);
  }

  size_t pos_before_error = scanner->position;

  ak_scanner_static_type_result_t error_result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(!error_result.success);
  AK24_TEST_ASSERT(error_result.start_position == pos_before_error);

  size_t error_offset =
      error_result.error_position - error_result.start_position;
  AK24_TEST_ASSERT(error_offset == 3);

  AK24_TEST_ASSERT(scanner->position == pos_before_error);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_large_buffer_invalid_real_in_sequence(void) {
  ak_buffer_t *buffer = ak_buffer_new(2048);
  AK24_TEST_ASSERT(buffer != NULL);

  uint8_t data[] = "1.1 2.2 3.3 4.4 5.5 6.6 7.7 8.8 9.9 10.10 11.1.1 12.12";

  ak_buffer_copy_to(buffer, data, strlen((char *)data));
  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  for (int i = 0; i < 10; i++) {
    ak_scanner_static_type_result_t result =
        ak_scanner_read_static_base_type(scanner, NULL);
    AK24_TEST_ASSERT(result.success);
    AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_REAL);
  }

  size_t pos_before_error = scanner->position;

  ak_scanner_static_type_result_t error_result =
      ak_scanner_read_static_base_type(scanner, NULL);

  AK24_TEST_ASSERT(!error_result.success);
  AK24_TEST_ASSERT(error_result.start_position == pos_before_error);
  AK24_TEST_ASSERT(error_result.error_position > pos_before_error);
  AK24_TEST_ASSERT(scanner->position == pos_before_error);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_large_buffer_complex_symbols(void) {
  ak_buffer_t *buffer = ak_buffer_new(2048);
  AK24_TEST_ASSERT(buffer != NULL);

  uint8_t data[] = "foo-bar baz_qux test123 abc-def-ghi jkl_mno_pqr "
                   "var1 var2 var3 var4 var5 var6 var7 var8 var9 var10 "
                   "alpha-1 beta-2 gamma-3 delta-4 epsilon-5 "
                   "test_a test_b test_c test_d test_e "
                   "sym1! sym2@ sym3# sym4$ sym5% "
                   "a-b-c d-e-f g-h-i j-k-l m-n-o";

  ak_buffer_copy_to(buffer, data, strlen((char *)data));
  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  int token_count = 0;
  while (scanner->position < buffer->count) {
    ak_scanner_static_type_result_t result =
        ak_scanner_read_static_base_type(scanner, NULL);

    if (!result.success) {
      break;
    }

    AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_SYMBOL);
    AK24_TEST_ASSERT(result.data.byte_length > 0);
    token_count++;
  }

  AK24_TEST_ASSERT(token_count == 35);

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_large_buffer_all_integers(void) {
  ak_buffer_t *buffer = ak_buffer_new(2048);
  AK24_TEST_ASSERT(buffer != NULL);

  uint8_t data[] =
      "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 "
      "20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 "
      "40 41 42 43 44 45 46 47 48 49";

  ak_buffer_copy_to(buffer, data, strlen((char *)data));
  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  for (int i = 0; i < 50; i++) {
    ak_scanner_static_type_result_t result =
        ak_scanner_read_static_base_type(scanner, NULL);

    AK24_TEST_ASSERT(result.success);
    AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_INTEGER);
  }

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

static int test_large_buffer_all_reals(void) {
  ak_buffer_t *buffer = ak_buffer_new(2048);
  AK24_TEST_ASSERT(buffer != NULL);

  uint8_t data[] = "0.0 1.1 2.2 3.3 4.4 5.5 6.6 7.7 8.8 9.9 "
                   "10.0 11.1 12.2 13.3 14.4 15.5 16.6 17.7 18.8 19.9 "
                   "20.0 21.1 22.2 23.3 24.4 25.5 26.6 27.7 28.8 29.9 "
                   "30.0 31.1 32.2 33.3 34.4 35.5 36.6 37.7 38.8 39.9";

  ak_buffer_copy_to(buffer, data, strlen((char *)data));
  ak_scanner_t *scanner = ak_scanner_new(buffer, 0);
  AK24_TEST_ASSERT(scanner != NULL);

  for (int i = 0; i < 40; i++) {
    ak_scanner_static_type_result_t result =
        ak_scanner_read_static_base_type(scanner, NULL);

    AK24_TEST_ASSERT(result.success);
    AK24_TEST_ASSERT(result.data.base == AK24_STATIC_BASE_REAL);
  }

  ak_scanner_free(scanner);
  ak_buffer_free(buffer);
  AK24_TEST_PASS();
}

int run_scanner_stress_tests(void) {
  AK24_TEST_RUN(test_large_valid_buffer_mixed_types);
  AK24_TEST_RUN(test_large_buffer_with_whitespace_variations);
  AK24_TEST_RUN(test_large_buffer_error_at_start);
  AK24_TEST_RUN(test_double_period_in_real);
  AK24_TEST_RUN(test_large_buffer_error_in_middle);
  AK24_TEST_RUN(test_large_buffer_error_at_end);
  AK24_TEST_RUN(test_large_buffer_invalid_integer_in_sequence);
  AK24_TEST_RUN(test_large_buffer_invalid_real_in_sequence);
  AK24_TEST_RUN(test_large_buffer_complex_symbols);
  AK24_TEST_RUN(test_large_buffer_all_integers);
  AK24_TEST_RUN(test_large_buffer_all_reals);

  return 0;
}
