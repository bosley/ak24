#include "filepath.h"
#include "test/assert.h"
#include <string.h>

// Helper to get C string from buffer
static const char *buf_cstr(ak_buffer_t *buf) {
  if (!buf)
    return NULL;
  uint8_t *data = ak_buffer_data(buf);
  size_t count = ak_buffer_count(buf);
  // Ensure null termination
  if (buf->capacity > count) {
    data[count] = '\0';
  }
  return (const char *)data;
}

// Test initialization and shutdown
int test_filepath_init_shutdown(void) {
  ak_filepath_init();
  ak_filepath_shutdown();
  AK24_TEST_PASS();
}

// Test separator functions
int test_filepath_separators(void) {
  ak_filepath_init();

  char sep = ak_filepath_separator();
  char list_sep = ak_filepath_list_separator();

#ifdef AK24_PLATFORM_WINDOWS
  AK24_TEST_ASSERT(sep == '\\');
  AK24_TEST_ASSERT(list_sep == ';');
#else
  AK24_TEST_ASSERT(sep == '/');
  AK24_TEST_ASSERT(list_sep == ':');
#endif

  ak_filepath_shutdown();
  AK24_TEST_PASS();
}

// Test basic path joining
int test_filepath_join_basic(void) {
  ak_filepath_init();

  ak_buffer_t *path = ak_filepath_join(3, "home", "user", "file.txt");
  AK24_TEST_ASSERT_NOT_NULL(path);

  const char *result = buf_cstr(path);
  AK24_TEST_ASSERT_NOT_NULL(result);

#ifdef AK24_PLATFORM_WINDOWS
  AK24_TEST_ASSERT_STR_EQ(result, "home\\user\\file.txt");
#else
  AK24_TEST_ASSERT_STR_EQ(result, "home/user/file.txt");
#endif

  ak_buffer_free(path);
  ak_filepath_shutdown();
  AK24_TEST_PASS();
}

// Test joining with trailing/leading separators
int test_filepath_join_with_separators(void) {
  ak_filepath_init();

#ifdef AK24_PLATFORM_WINDOWS
  ak_buffer_t *path = ak_filepath_join(3, "C:\\", "Users\\", "file.txt");
#else
  ak_buffer_t *path = ak_filepath_join(3, "/home/", "user/", "file.txt");
#endif

  AK24_TEST_ASSERT_NOT_NULL(path);
  const char *result = buf_cstr(path);

#ifdef AK24_PLATFORM_WINDOWS
  AK24_TEST_ASSERT_STR_EQ(result, "C:\\Users\\file.txt");
#else
  AK24_TEST_ASSERT_STR_EQ(result, "/home/user/file.txt");
#endif

  ak_buffer_free(path);
  ak_filepath_shutdown();
  AK24_TEST_PASS();
}

// Test joining with NULL components
int test_filepath_join_with_nulls(void) {
  ak_filepath_init();

  ak_buffer_t *path = ak_filepath_join(4, "home", NULL, "user", "file.txt");
  AK24_TEST_ASSERT_NOT_NULL(path);

  const char *result = buf_cstr(path);

#ifdef AK24_PLATFORM_WINDOWS
  AK24_TEST_ASSERT_STR_EQ(result, "home\\user\\file.txt");
#else
  AK24_TEST_ASSERT_STR_EQ(result, "home/user/file.txt");
#endif

  ak_buffer_free(path);
  ak_filepath_shutdown();
  AK24_TEST_PASS();
}

// Test path normalization
int test_filepath_normalize_basic(void) {
  ak_filepath_init();

#ifdef AK24_PLATFORM_WINDOWS
  ak_buffer_t *norm = ak_filepath_normalize("C:\\Users\\..\\home\\.\\file.txt");
#else
  ak_buffer_t *norm = ak_filepath_normalize("/home/user/../other/./file.txt");
#endif

  AK24_TEST_ASSERT_NOT_NULL(norm);
  const char *result = buf_cstr(norm);

#ifdef AK24_PLATFORM_WINDOWS
  AK24_TEST_ASSERT_STR_EQ(result, "\\home\\file.txt");
#else
  AK24_TEST_ASSERT_STR_EQ(result, "/home/other/file.txt");
#endif

  ak_buffer_free(norm);
  ak_filepath_shutdown();
  AK24_TEST_PASS();
}

// Test normalizing relative paths
int test_filepath_normalize_relative(void) {
  ak_filepath_init();

  ak_buffer_t *norm = ak_filepath_normalize("./user/../file.txt");
  AK24_TEST_ASSERT_NOT_NULL(norm);

  const char *result = buf_cstr(norm);
  AK24_TEST_ASSERT_STR_EQ(result, "file.txt");

  ak_buffer_free(norm);
  ak_filepath_shutdown();
  AK24_TEST_PASS();
}

// Test is_absolute
int test_filepath_is_absolute(void) {
  ak_filepath_init();

#ifdef AK24_PLATFORM_WINDOWS
  AK24_TEST_ASSERT(ak_filepath_is_absolute("C:\\Users"));
  AK24_TEST_ASSERT(ak_filepath_is_absolute("D:/data"));
  AK24_TEST_ASSERT(ak_filepath_is_absolute("\\\\server\\share"));
  AK24_TEST_ASSERT(!ak_filepath_is_absolute("relative\\path"));
#else
  AK24_TEST_ASSERT(ak_filepath_is_absolute("/home/user"));
  AK24_TEST_ASSERT(ak_filepath_is_absolute("/"));
  AK24_TEST_ASSERT(!ak_filepath_is_absolute("relative/path"));
  AK24_TEST_ASSERT(!ak_filepath_is_absolute("./relative"));
#endif

  ak_filepath_shutdown();
  AK24_TEST_PASS();
}

// Test basename extraction
int test_filepath_basename(void) {
  ak_filepath_init();

  ak_buffer_t *base1 = ak_filepath_basename("/home/user/file.txt");
  AK24_TEST_ASSERT_NOT_NULL(base1);
  AK24_TEST_ASSERT_STR_EQ(buf_cstr(base1), "file.txt");
  ak_buffer_free(base1);

  ak_buffer_t *base2 = ak_filepath_basename("file.txt");
  AK24_TEST_ASSERT_NOT_NULL(base2);
  AK24_TEST_ASSERT_STR_EQ(buf_cstr(base2), "file.txt");
  ak_buffer_free(base2);

  ak_buffer_t *base3 = ak_filepath_basename("/home/user/");
  AK24_TEST_ASSERT_NOT_NULL(base3);
  AK24_TEST_ASSERT_STR_EQ(buf_cstr(base3), "");
  ak_buffer_free(base3);

  ak_filepath_shutdown();
  AK24_TEST_PASS();
}

// Test dirname extraction
int test_filepath_dirname(void) {
  ak_filepath_init();

  ak_buffer_t *dir1 = ak_filepath_dirname("/home/user/file.txt");
  AK24_TEST_ASSERT_NOT_NULL(dir1);
  AK24_TEST_ASSERT_STR_EQ(buf_cstr(dir1), "/home/user");
  ak_buffer_free(dir1);

  ak_buffer_t *dir2 = ak_filepath_dirname("file.txt");
  AK24_TEST_ASSERT_NOT_NULL(dir2);
  AK24_TEST_ASSERT_STR_EQ(buf_cstr(dir2), ".");
  ak_buffer_free(dir2);

  ak_buffer_t *dir3 = ak_filepath_dirname("/file.txt");
  AK24_TEST_ASSERT_NOT_NULL(dir3);
  char sep[2] = {ak_filepath_separator(), '\0'};
  AK24_TEST_ASSERT_STR_EQ(buf_cstr(dir3), sep);
  ak_buffer_free(dir3);

  ak_filepath_shutdown();
  AK24_TEST_PASS();
}

// Test extension extraction
int test_filepath_extension(void) {
  ak_filepath_init();

  ak_buffer_t *ext1 = ak_filepath_extension("file.txt");
  AK24_TEST_ASSERT_NOT_NULL(ext1);
  AK24_TEST_ASSERT_STR_EQ(buf_cstr(ext1), ".txt");
  ak_buffer_free(ext1);

  ak_buffer_t *ext2 = ak_filepath_extension("archive.tar.gz");
  AK24_TEST_ASSERT_NOT_NULL(ext2);
  AK24_TEST_ASSERT_STR_EQ(buf_cstr(ext2), ".gz");
  ak_buffer_free(ext2);

  ak_buffer_t *ext3 = ak_filepath_extension("noext");
  AK24_TEST_ASSERT_NULL(ext3);

  ak_buffer_t *ext4 = ak_filepath_extension(".bashrc");
  AK24_TEST_ASSERT_NULL(ext4);

  ak_filepath_shutdown();
  AK24_TEST_PASS();
}

// Test home directory
int test_filepath_home(void) {
  ak_filepath_init();

  ak_buffer_t *home = ak_filepath_home();
  AK24_TEST_ASSERT_NOT_NULL(home);
  AK24_TEST_ASSERT(ak_buffer_count(home) > 0);

  ak_buffer_free(home);
  ak_filepath_shutdown();
  AK24_TEST_PASS();
}

// Test cache directory
int test_filepath_cache(void) {
  ak_filepath_init();

  ak_buffer_t *cache = ak_filepath_cache();
  AK24_TEST_ASSERT_NOT_NULL(cache);
  AK24_TEST_ASSERT(ak_buffer_count(cache) > 0);

  ak_buffer_free(cache);
  ak_filepath_shutdown();
  AK24_TEST_PASS();
}

// Test config directory
int test_filepath_config(void) {
  ak_filepath_init();

  ak_buffer_t *config = ak_filepath_config();
  AK24_TEST_ASSERT_NOT_NULL(config);
  AK24_TEST_ASSERT(ak_buffer_count(config) > 0);

  ak_buffer_free(config);
  ak_filepath_shutdown();
  AK24_TEST_PASS();
}

// Test data directory
int test_filepath_data(void) {
  ak_filepath_init();

  ak_buffer_t *data = ak_filepath_data();
  AK24_TEST_ASSERT_NOT_NULL(data);
  AK24_TEST_ASSERT(ak_buffer_count(data) > 0);

  ak_buffer_free(data);
  ak_filepath_shutdown();
  AK24_TEST_PASS();
}

// Test temp directory
int test_filepath_temp(void) {
  ak_filepath_init();

  ak_buffer_t *temp = ak_filepath_temp();
  AK24_TEST_ASSERT_NOT_NULL(temp);
  AK24_TEST_ASSERT(ak_buffer_count(temp) > 0);

  ak_buffer_free(temp);
  ak_filepath_shutdown();
  AK24_TEST_PASS();
}

// Test current working directory
int test_filepath_cwd(void) {
  ak_filepath_init();

  ak_buffer_t *cwd = ak_filepath_cwd();
  AK24_TEST_ASSERT_NOT_NULL(cwd);
  AK24_TEST_ASSERT(ak_buffer_count(cwd) > 0);

  ak_buffer_free(cwd);
  ak_filepath_shutdown();
  AK24_TEST_PASS();
}

// Test to_native conversion
int test_filepath_to_native(void) {
  ak_filepath_init();

#ifdef AK24_PLATFORM_WINDOWS
  ak_buffer_t *native = ak_filepath_to_native("C:/Users/name/file.txt");
  AK24_TEST_ASSERT_NOT_NULL(native);
  AK24_TEST_ASSERT_STR_EQ(buf_cstr(native), "C:\\Users\\name\\file.txt");
#else
  // On POSIX, backslashes don't have special meaning, so they stay as-is
  ak_buffer_t *native = ak_filepath_to_native("home/user/file.txt");
  AK24_TEST_ASSERT_NOT_NULL(native);
  AK24_TEST_ASSERT_STR_EQ(buf_cstr(native), "home/user/file.txt");
#endif

  ak_buffer_free(native);
  ak_filepath_shutdown();
  AK24_TEST_PASS();
}

// Test runner
int main(void) {
  ak_kernel_init("ak24-test");

  AK24_TEST_RUN(test_filepath_init_shutdown);
  AK24_TEST_RUN(test_filepath_separators);
  AK24_TEST_RUN(test_filepath_join_basic);
  AK24_TEST_RUN(test_filepath_join_with_separators);
  AK24_TEST_RUN(test_filepath_join_with_nulls);
  AK24_TEST_RUN(test_filepath_normalize_basic);
  AK24_TEST_RUN(test_filepath_normalize_relative);
  AK24_TEST_RUN(test_filepath_is_absolute);
  AK24_TEST_RUN(test_filepath_basename);
  AK24_TEST_RUN(test_filepath_dirname);
  AK24_TEST_RUN(test_filepath_extension);
  AK24_TEST_RUN(test_filepath_home);
  AK24_TEST_RUN(test_filepath_cache);
  AK24_TEST_RUN(test_filepath_config);
  AK24_TEST_RUN(test_filepath_data);
  AK24_TEST_RUN(test_filepath_temp);
  AK24_TEST_RUN(test_filepath_cwd);
  AK24_TEST_RUN(test_filepath_to_native);

  ak_kernel_deinit();
  return 0;
}
