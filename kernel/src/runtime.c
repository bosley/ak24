#include "kernel.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef AK24_PLATFORM_WINDOWS
#include <direct.h>
#include <process.h>
#include <windows.h>
#define mkdir(path, mode) _mkdir(path)
#define getpid _getpid
typedef DWORD pid_t;
#else
#include <signal.h>
#include <unistd.h>
#endif

char *ak24_kernel_lock_file_path = NULL;

// FNV-1a hash constants
#define FNV_OFFSET_BASIS 14695981039346656037ULL
#define FNV_PRIME 1099511628211ULL

/**
 * @brief Compute FNV-1a hash of string
 */
static uint64_t hash_string(const char *str) {
  uint64_t hash = FNV_OFFSET_BASIS;
  size_t len = strlen(str);
  for (size_t i = 0; i < len; i++) {
    hash ^= (uint64_t)(unsigned char)str[i];
    hash *= FNV_PRIME;
  }
  return hash;
}

/**
 * @brief Check if a process is running
 * @param pid Process ID to check
 * @return 1 if running, 0 if not running
 */
static int is_process_running(pid_t pid) {
#ifdef AK24_PLATFORM_WINDOWS
  HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  if (process == NULL) {
    return 0; // Process doesn't exist or no access
  }
  DWORD exit_code;
  BOOL result = GetExitCodeProcess(process, &exit_code);
  CloseHandle(process);
  return (result && exit_code == STILL_ACTIVE) ? 1 : 0;
#else
  // On POSIX, use kill with signal 0 to check if process exists
  // Returns 0 if process exists, -1 with errno=ESRCH if not
  if (kill(pid, 0) == 0) {
    return 1; // Process exists
  }
  return (errno != ESRCH) ? 1 : 0; // Return 1 if error is permission denied
#endif
}

/**
 * @brief Create directory recursively
 * @return 0 on success, -1 on failure
 */
static int mkdir_recursive(const char *path) {
  char tmp[4096];
  char *p = NULL;
  size_t len;

  snprintf(tmp, sizeof(tmp), "%s", path);
  len = strlen(tmp);
  if (tmp[len - 1] == '/' || tmp[len - 1] == '\\')
    tmp[len - 1] = 0;

  for (p = tmp + 1; *p; p++) {
    if (*p == '/' || *p == '\\') {
      *p = 0;
      // Skip drive letters on Windows (e.g., "C:")
      if (strlen(tmp) > 0 && !(strlen(tmp) == 2 && tmp[1] == ':')) {
        mkdir(tmp, 0755);
      }
      *p = '/';
    }
  }

  return mkdir(tmp, 0755);
}

/**
 * @brief Initialize application runtime directory and lock file
 * @return 0 on success, exits on failure
 */
int init_runtime_directory(const char *app_id) {
  if (!app_id || strlen(app_id) == 0) {
    fprintf(stderr, "AK24 Error: Application ID cannot be empty\n");
    exit(1);
  }

  // Get application data directory (platform-specific)
  // macOS: ~/Library/Application Support
  // Linux: ~/.local/share
  // Windows: %LOCALAPPDATA%
  ak_buffer_t *data_dir = ak_filepath_data();
  if (!data_dir) {
    fprintf(stderr, "AK24 Error: Failed to get application data directory\n");
    exit(1);
  }

  // Ensure data_dir is null-terminated for use as C string
  size_t data_dir_len = ak_buffer_count(data_dir);
  uint8_t *data_dir_data = ak_buffer_data(data_dir);
  char *data_dir_str = AK24_ALLOC_ATOMIC(data_dir_len + 1);
  if (!data_dir_str) {
    fprintf(stderr, "AK24 Error: Memory allocation failed\n");
    ak_buffer_free(data_dir);
    exit(1);
  }
  memcpy(data_dir_str, data_dir_data, data_dir_len);
  data_dir_str[data_dir_len] = '\0';

  ak_buffer_t *runtime_dir =
      ak_filepath_join(3, data_dir_str, "ak24", "runtime");
  AK24_FREE(data_dir_str);
  ak_buffer_free(data_dir);

  if (!runtime_dir) {
    fprintf(stderr, "AK24 Error: Failed to construct runtime directory path\n");
    exit(1);
  }

  // Ensure runtime_dir is null-terminated for use as C string
  size_t runtime_dir_len = ak_buffer_count(runtime_dir);
  uint8_t *runtime_dir_data = ak_buffer_data(runtime_dir);
  char *dir_path = AK24_ALLOC_ATOMIC(runtime_dir_len + 1);
  if (!dir_path) {
    fprintf(stderr, "AK24 Error: Memory allocation failed\n");
    ak_buffer_free(runtime_dir);
    exit(1);
  }
  memcpy(dir_path, runtime_dir_data, runtime_dir_len);
  dir_path[runtime_dir_len] = '\0';

  if (mkdir_recursive(dir_path) != 0 && errno != EEXIST) {
    fprintf(stderr, "AK24 Error: Failed to create runtime directory '%s': %s\n",
            dir_path, strerror(errno));
    AK24_FREE(dir_path);
    ak_buffer_free(runtime_dir);
    exit(1);
  }

  uint64_t hash = hash_string(app_id);
  char hash_filename[32];
  snprintf(hash_filename, sizeof(hash_filename), "%016llx.ak24",
           (unsigned long long)hash);

  ak_buffer_t *lock_path = ak_filepath_join(2, dir_path, hash_filename);
  AK24_FREE(dir_path);
  ak_buffer_free(runtime_dir);

  if (!lock_path) {
    fprintf(stderr, "AK24 Error: Failed to construct lock file path\n");
    exit(1);
  }

  // Ensure lock_path is null-terminated for use as C string
  size_t lock_path_len = ak_buffer_count(lock_path);
  uint8_t *lock_path_data = ak_buffer_data(lock_path);
  char *lock_file = AK24_ALLOC_ATOMIC(lock_path_len + 1);
  if (!lock_file) {
    fprintf(stderr, "AK24 Error: Memory allocation failed\n");
    ak_buffer_free(lock_path);
    exit(1);
  }
  memcpy(lock_file, lock_path_data, lock_path_len);
  lock_file[lock_path_len] = '\0';

  int skip_lock_check = (strcmp(app_id, "ak24-test") == 0);

  if (!skip_lock_check) {
    FILE *check = fopen(lock_file, "r");
    if (check) {
      char line1[256], line2[64];
      pid_t existing_pid = 0;

      // Read app_id and PID
      if (fgets(line1, sizeof(line1), check)) {
        line1[strcspn(line1, "\n")] = 0;
        if (fgets(line2, sizeof(line2), check)) {
          existing_pid = (pid_t)atol(line2);
        }
      }
      fclose(check);

      // Check if the process is still running
      if (existing_pid > 0 && is_process_running(existing_pid)) {
        fprintf(stderr,
                "AK24 Error: Application '%s' is already running (PID: %ld)\n",
                line1, (long)existing_pid);
        fprintf(stderr, "AK24 Error: Lock file: '%s'\n", lock_file);
        AK24_FREE(lock_file);
        ak_buffer_free(lock_path);
        exit(1);
      }

      if (existing_pid > 0) {
        fprintf(stderr,
                "AK24 Info: Removing stale lock file (PID %ld not running)\n",
                (long)existing_pid);
      }
    }
  }

  FILE *f = fopen(lock_file, "w");
  if (!f) {
    fprintf(stderr, "AK24 Error: Failed to create lock file '%s': %s\n",
            lock_file, strerror(errno));
    AK24_FREE(lock_file);
    ak_buffer_free(lock_path);
    exit(1);
  }

  pid_t current_pid = getpid();
  fprintf(f, "%s\n%ld\n", app_id, (long)current_pid);
  fclose(f);

  ak24_kernel_lock_file_path = strdup(lock_file);
  AK24_FREE(lock_file);
  ak_buffer_free(lock_path);

  return 0;
}
