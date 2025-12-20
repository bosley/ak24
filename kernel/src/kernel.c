#include "kernel.h"
#include "thread_platform.h"
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#if AK24_GC_ENABLED
#include <sched.h>
#include <unistd.h>
#define GC_THREADS 1
#include <gc.h>
#endif

#ifdef AK24_PLATFORM_POSIX
#include <pthread.h>
#elif defined(AK24_PLATFORM_WINDOWS)
#include <windows.h>
#endif

/**
 * @brief Print warning message in bright red (Windows console)
 *
 * Prints a formatted warning message in bright red on Windows console.
 * On non-Windows platforms, prints without color.
 *
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
static void ak_print_warning(const char *format, ...) {
#ifdef AK24_PLATFORM_WINDOWS
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
  WORD saved_attributes;

  // Save current attributes
  if (GetConsoleScreenBufferInfo(hConsole, &consoleInfo)) {
    saved_attributes = consoleInfo.wAttributes;
  } else {
    saved_attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
  }

  // Set bright red color (FOREGROUND_INTENSITY makes it bright)
  SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);

  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);

  // Restore original attributes
  SetConsoleTextAttribute(hConsole, saved_attributes);
#else
  // Non-Windows: just print without color
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
#endif
}

static list_void_t shutdown_lambdas;
static int shutdown_lambdas_initialized = 0;
static time_t kernel_start_time;

// Signal handling infrastructure
typedef struct {
  int signum;
  ak_lambda_t *handler;
  struct sigaction old_action;
} ak_signal_handler_entry_t;

static list_void_t signal_handlers;
static int signal_handlers_initialized = 0;
static AK_MUTEX signal_mutex = AK_MUTEX_INITIALIZER;

// Forward declarations for signal handling
static void ak_signal_handlers_init(void);
static void ak_signal_handlers_deinit(void);
static void ak_signal_dispatch(int signum);

#if AK24_BUILD_DEBUG_MEMORY

static AK_MUTEX mem_stats_mutex = AK_MUTEX_INITIALIZER;
static ak_memory_stats_t mem_stats = {0};

#if AK24_GC_ENABLED

void *ak_mem_alloc_tracked(size_t size, const char *file, int line) {
  (void)file;
  (void)line;
  void *ptr = GC_MALLOC(size);
  if (ptr) {
    AK_MUTEX_LOCK(&mem_stats_mutex);
    mem_stats.total_allocations++;
    mem_stats.bytes_allocated += size;
    mem_stats.current_bytes += size;
    if (mem_stats.current_bytes > mem_stats.peak_bytes) {
      mem_stats.peak_bytes = mem_stats.current_bytes;
    }
    AK_MUTEX_UNLOCK(&mem_stats_mutex);
  }
  return ptr;
}

void *ak_mem_alloc_atomic_tracked(size_t size, const char *file, int line) {
  (void)file;
  (void)line;
  void *ptr = GC_MALLOC_ATOMIC(size);
  if (ptr) {
    AK_MUTEX_LOCK(&mem_stats_mutex);
    mem_stats.total_allocations++;
    mem_stats.bytes_allocated += size;
    mem_stats.current_bytes += size;
    if (mem_stats.current_bytes > mem_stats.peak_bytes) {
      mem_stats.peak_bytes = mem_stats.current_bytes;
    }
    AK_MUTEX_UNLOCK(&mem_stats_mutex);
  }
  return ptr;
}

void *ak_mem_realloc_tracked(void *ptr, size_t size, const char *file,
                             int line) {
  (void)file;
  (void)line;
  void *new_ptr = GC_REALLOC(ptr, size);
  if (new_ptr) {
    AK_MUTEX_LOCK(&mem_stats_mutex);
    mem_stats.total_reallocs++;
    mem_stats.bytes_allocated += size;
    mem_stats.current_bytes += size;
    if (mem_stats.current_bytes > mem_stats.peak_bytes) {
      mem_stats.peak_bytes = mem_stats.current_bytes;
    }
    AK_MUTEX_UNLOCK(&mem_stats_mutex);
  }
  return new_ptr;
}

void ak_mem_free_tracked(void *ptr, const char *file, int line) {
  (void)file;
  (void)line;
  if (ptr) {
    AK_MUTEX_LOCK(&mem_stats_mutex);
    mem_stats.total_frees++;
    AK_MUTEX_UNLOCK(&mem_stats_mutex);
    GC_FREE(ptr);
  }
}

#else

void *ak_mem_alloc_tracked(size_t size, const char *file, int line) {
  (void)file;
  (void)line;
  void *ptr = malloc(size);
  if (ptr) {
    AK_MUTEX_LOCK(&mem_stats_mutex);
    mem_stats.total_allocations++;
    mem_stats.bytes_allocated += size;
    mem_stats.current_bytes += size;
    if (mem_stats.current_bytes > mem_stats.peak_bytes) {
      mem_stats.peak_bytes = mem_stats.current_bytes;
    }
    AK_MUTEX_UNLOCK(&mem_stats_mutex);
  }
  return ptr;
}

void *ak_mem_alloc_atomic_tracked(size_t size, const char *file, int line) {
  (void)file;
  (void)line;
  void *ptr = malloc(size);
  if (ptr) {
    AK_MUTEX_LOCK(&mem_stats_mutex);
    mem_stats.total_allocations++;
    mem_stats.bytes_allocated += size;
    mem_stats.current_bytes += size;
    if (mem_stats.current_bytes > mem_stats.peak_bytes) {
      mem_stats.peak_bytes = mem_stats.current_bytes;
    }
    AK_MUTEX_UNLOCK(&mem_stats_mutex);
  }
  return ptr;
}

void *ak_mem_realloc_tracked(void *ptr, size_t size, const char *file,
                             int line) {
  (void)file;
  (void)line;
  void *new_ptr = realloc(ptr, size);
  if (new_ptr) {
    AK_MUTEX_LOCK(&mem_stats_mutex);
    mem_stats.total_reallocs++;
    mem_stats.bytes_allocated += size;
    mem_stats.current_bytes += size;
    if (mem_stats.current_bytes > mem_stats.peak_bytes) {
      mem_stats.peak_bytes = mem_stats.current_bytes;
    }
    AK_MUTEX_UNLOCK(&mem_stats_mutex);
  }
  return new_ptr;
}

void ak_mem_free_tracked(void *ptr, const char *file, int line) {
  (void)file;
  (void)line;
  if (ptr) {
    AK_MUTEX_LOCK(&mem_stats_mutex);
    mem_stats.total_frees++;
    AK_MUTEX_UNLOCK(&mem_stats_mutex);
    free(ptr);
  }
}

#endif

ak_memory_stats_t ak_mem_get_stats(void) {
  AK_MUTEX_LOCK(&mem_stats_mutex);
  ak_memory_stats_t stats = mem_stats;
  AK_MUTEX_UNLOCK(&mem_stats_mutex);
  return stats;
}

void ak_mem_print_stats(void) {
  ak_memory_stats_t stats = ak_mem_get_stats();
  printf("\n=== Memory Statistics ===\n");
  printf("Total allocations: %zu\n", stats.total_allocations);
  printf("Total frees: %zu\n", stats.total_frees);
  printf("Total reallocs: %zu\n", stats.total_reallocs);
  printf("Bytes allocated: %zu\n", stats.bytes_allocated);
  printf("Bytes freed: %zu\n", stats.bytes_freed);
  printf("Current bytes: %zu\n", stats.current_bytes);
  printf("Peak bytes: %zu\n", stats.peak_bytes);
  printf("=========================\n\n");
}

#endif

#if AK24_GC_ENABLED

void ak_kernel_init(void) {
  kernel_start_time = time(NULL);
#ifdef AK24_PLATFORM_WINDOWS
  ak_print_warning("\n*** WARNING ***\n");
  ak_print_warning("AK24 Windows support is UNTESTED CODE\n");
  ak_print_warning("Please report any issues to the development team\n");
  ak_print_warning("***************\n\n");
#else
  // Silence unused function warning on non-Windows platforms
  (void)ak_print_warning;
#endif
  GC_set_warn_proc(GC_ignore_warn_proc);
  GC_INIT();
  list_init(&shutdown_lambdas);
  shutdown_lambdas_initialized = 1;
  ak_signal_handlers_init();
}

void ak_kernel_deinit(void) {
  if (shutdown_lambdas_initialized) {
    kernel_shutdown_info_t info;
    info.start_time = kernel_start_time;
#if AK24_BUILD_DEBUG_MEMORY
    info.memory_stats = ak_mem_get_stats();
#endif

    list_iter_t iter = list_iter(&shutdown_lambdas);
    void **lambda_ptr;
    while ((lambda_ptr = list_next(&shutdown_lambdas, &iter))) {
      ak_lambda_t *lambda = (ak_lambda_t *)*lambda_ptr;
      if (lambda) {
        ak_lambda_invoke(lambda, &info);
      }
    }
    list_deinit(&shutdown_lambdas);
    shutdown_lambdas_initialized = 0;
  }
  ak_signal_handlers_deinit();
  sched_yield();
  sched_yield();
}

#else

void ak_kernel_init(void) {
  kernel_start_time = time(NULL);
#ifdef AK24_PLATFORM_WINDOWS
  ak_print_warning("\n*** WARNING ***\n");
  ak_print_warning("AK24 Windows support is UNTESTED CODE\n");
  ak_print_warning("Please report any issues to the development team\n");
  ak_print_warning("***************\n\n");
#else
  // Silence unused function warning on non-Windows platforms
  (void)ak_print_warning;
#endif
  list_init(&shutdown_lambdas);
  shutdown_lambdas_initialized = 1;
  ak_signal_handlers_init();
}

void ak_kernel_deinit(void) {
  if (shutdown_lambdas_initialized) {
    kernel_shutdown_info_t info;
    info.start_time = kernel_start_time;
#if AK24_BUILD_DEBUG_MEMORY
    info.memory_stats = ak_mem_get_stats();
#endif

    list_iter_t iter = list_iter(&shutdown_lambdas);
    void **lambda_ptr;
    while ((lambda_ptr = list_next(&shutdown_lambdas, &iter))) {
      ak_lambda_t *lambda = (ak_lambda_t *)*lambda_ptr;
      if (lambda) {
        ak_lambda_invoke(lambda, &info);
      }
    }
    list_deinit(&shutdown_lambdas);
    shutdown_lambdas_initialized = 0;
  }
  ak_signal_handlers_deinit();
}

#endif

void ak_on_shutdown(ak_lambda_t *lambda) {
  if (!lambda || !shutdown_lambdas_initialized) {
    return;
  }
  list_push(&shutdown_lambdas, lambda);
}

list_str_t ak_args_to_list(int argc, char **argv) {
  list_str_t args;
  list_init(&args);
  for (int i = 0; i < argc; i++) {
    list_push(&args, argv[i]);
  }
  return args;
}

// Signal handling implementation

static void ak_signal_dispatch(int signum) {
  AK_MUTEX_LOCK(&signal_mutex);

  if (!signal_handlers_initialized) {
    AK_MUTEX_UNLOCK(&signal_mutex);
    return;
  }

  list_iter_t iter = list_iter(&signal_handlers);
  void **entry_ptr;
  while ((entry_ptr = list_next(&signal_handlers, &iter))) {
    ak_signal_handler_entry_t *entry = (ak_signal_handler_entry_t *)*entry_ptr;
    if (entry && entry->signum == signum && entry->handler) {
      // Create signal info structure to pass to handler
      int *signum_ptr = AK24_ALLOC(sizeof(int));
      if (signum_ptr) {
        *signum_ptr = signum;
        ak_lambda_invoke(entry->handler, signum_ptr);
      }
    }
  }

  AK_MUTEX_UNLOCK(&signal_mutex);
}

void ak_register_signal_handler(int signum, ak_lambda_t *handler) {
  if (!handler || !signal_handlers_initialized) {
    return;
  }

  AK_MUTEX_LOCK(&signal_mutex);

  // Check if handler already exists for this signal
  list_iter_t iter = list_iter(&signal_handlers);
  void **entry_ptr;
  while ((entry_ptr = list_next(&signal_handlers, &iter))) {
    ak_signal_handler_entry_t *entry = (ak_signal_handler_entry_t *)*entry_ptr;
    if (entry && entry->signum == signum) {
      // Update existing handler
      entry->handler = handler;
      AK_MUTEX_UNLOCK(&signal_mutex);
      return;
    }
  }

  // Create new handler entry
  ak_signal_handler_entry_t *entry =
      AK24_ALLOC(sizeof(ak_signal_handler_entry_t));
  if (!entry) {
    AK_MUTEX_UNLOCK(&signal_mutex);
    return;
  }

  entry->signum = signum;
  entry->handler = handler;

  // Install signal handler
  struct sigaction sa;
  sa.sa_handler = ak_signal_dispatch;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART; // Restart interrupted system calls

  if (sigaction(signum, &sa, &entry->old_action) == 0) {
    list_push(&signal_handlers, entry);
  }

  AK_MUTEX_UNLOCK(&signal_mutex);
}

void ak_unregister_signal_handler(int signum) {
  AK_MUTEX_LOCK(&signal_mutex);

  if (!signal_handlers_initialized) {
    AK_MUTEX_UNLOCK(&signal_mutex);
    return;
  }

  list_iter_t iter = list_iter(&signal_handlers);
  void **entry_ptr;
  size_t index = 0;
  int found = 0;

  while ((entry_ptr = list_next(&signal_handlers, &iter))) {
    ak_signal_handler_entry_t *entry = (ak_signal_handler_entry_t *)*entry_ptr;
    if (entry && entry->signum == signum) {
      // Restore old signal handler
      sigaction(signum, &entry->old_action, NULL);
      found = 1;
      break;
    }
    index++;
  }

  if (found) {
    list_remove_(&signal_handlers.base, index);
  }

  AK_MUTEX_UNLOCK(&signal_mutex);
}

static void ak_signal_handlers_init(void) {
  list_init(&signal_handlers);
  signal_handlers_initialized = 1;
}

static void ak_signal_handlers_deinit(void) {
  if (!signal_handlers_initialized) {
    return;
  }

  AK_MUTEX_LOCK(&signal_mutex);

  // Restore all signal handlers
  list_iter_t iter = list_iter(&signal_handlers);
  void **entry_ptr;
  while ((entry_ptr = list_next(&signal_handlers, &iter))) {
    ak_signal_handler_entry_t *entry = (ak_signal_handler_entry_t *)*entry_ptr;
    if (entry) {
      sigaction(entry->signum, &entry->old_action, NULL);
    }
  }

  list_deinit(&signal_handlers);
  signal_handlers_initialized = 0;

  AK_MUTEX_UNLOCK(&signal_mutex);
}

// Mutex implementations

int AK_mutex_init(AK_MUTEX *mutex) {
  if (!mutex) {
    return -1;
  }
#ifdef AK24_PLATFORM_POSIX
  return pthread_mutex_init(&mutex->handle, NULL);
#elif defined(AK24_PLATFORM_WINDOWS)
  InitializeCriticalSection(&mutex->handle);
  return 0;
#else
#error "Unsupported platform"
#endif
}

int AK_mutex_destroy(AK_MUTEX *mutex) {
  if (!mutex) {
    return -1;
  }
#ifdef AK24_PLATFORM_POSIX
  return pthread_mutex_destroy(&mutex->handle);
#elif defined(AK24_PLATFORM_WINDOWS)
  DeleteCriticalSection(&mutex->handle);
  return 0;
#else
#error "Unsupported platform"
#endif
}

int AK_mutex_lock(AK_MUTEX *mutex) {
  if (!mutex) {
    return -1;
  }
#ifdef AK24_PLATFORM_POSIX
  return pthread_mutex_lock(&mutex->handle);
#elif defined(AK24_PLATFORM_WINDOWS)
  EnterCriticalSection(&mutex->handle);
  return 0;
#else
#error "Unsupported platform"
#endif
}

int AK_mutex_unlock(AK_MUTEX *mutex) {
  if (!mutex) {
    return -1;
  }
#ifdef AK24_PLATFORM_POSIX
  return pthread_mutex_unlock(&mutex->handle);
#elif defined(AK24_PLATFORM_WINDOWS)
  LeaveCriticalSection(&mutex->handle);
  return 0;
#else
#error "Unsupported platform"
#endif
}

// Condition variable implementations

int AK_cond_init(AK_COND *cond) {
  if (!cond) {
    return -1;
  }
#ifdef AK24_PLATFORM_POSIX
  return pthread_cond_init(&cond->handle, NULL);
#elif defined(AK24_PLATFORM_WINDOWS)
  InitializeConditionVariable(&cond->handle);
  return 0;
#else
#error "Unsupported platform"
#endif
}

int AK_cond_destroy(AK_COND *cond) {
  if (!cond) {
    return -1;
  }
#ifdef AK24_PLATFORM_POSIX
  return pthread_cond_destroy(&cond->handle);
#elif defined(AK24_PLATFORM_WINDOWS)
  // Windows condition variables don't need explicit destruction
  return 0;
#else
#error "Unsupported platform"
#endif
}

int AK_cond_wait(AK_COND *cond, AK_MUTEX *mutex) {
  if (!cond || !mutex) {
    return -1;
  }
#ifdef AK24_PLATFORM_POSIX
  return pthread_cond_wait(&cond->handle, &mutex->handle);
#elif defined(AK24_PLATFORM_WINDOWS)
  return SleepConditionVariableCS(&cond->handle, &mutex->handle, INFINITE) ? 0
                                                                           : -1;
#else
#error "Unsupported platform"
#endif
}

int AK_cond_signal(AK_COND *cond) {
  if (!cond) {
    return -1;
  }
#ifdef AK24_PLATFORM_POSIX
  return pthread_cond_signal(&cond->handle);
#elif defined(AK24_PLATFORM_WINDOWS)
  WakeConditionVariable(&cond->handle);
  return 0;
#else
#error "Unsupported platform"
#endif
}

int AK_cond_broadcast(AK_COND *cond) {
  if (!cond) {
    return -1;
  }
#ifdef AK24_PLATFORM_POSIX
  return pthread_cond_broadcast(&cond->handle);
#elif defined(AK24_PLATFORM_WINDOWS)
  WakeAllConditionVariable(&cond->handle);
  return 0;
#else
#error "Unsupported platform"
#endif
}

// Thread abstraction implementations
// These provide platform-agnostic threading primitives that handle
// both GC/non-GC builds and POSIX/Windows platforms

// Thread wrapper structure for Windows to adapt POSIX-style thread functions
#ifdef AK24_PLATFORM_WINDOWS
typedef struct {
  void *(*start_routine)(void *);
  void *arg;
} ak_win_thread_adapter_t;

// Windows thread adapter that converts Windows thread API to POSIX style
static DWORD WINAPI ak_win_thread_wrapper(LPVOID param) {
  ak_win_thread_adapter_t *adapter = (ak_win_thread_adapter_t *)param;
  void *(*start_routine)(void *) = adapter->start_routine;
  void *arg = adapter->arg;

  // Free the adapter structure
  AK24_FREE(adapter);

  // Call the actual thread function
  void *result = start_routine(arg);

  // Windows threads return DWORD, we cast the result
  return (DWORD)(uintptr_t)result;
}
#endif

int AK_THREAD_CREATE(AK_THREAD *thread, void *(*start_routine)(void *),
                     void *arg) {
  if (!thread || !start_routine) {
    return -1;
  }

#ifdef AK24_PLATFORM_POSIX
#if AK24_GC_ENABLED
  // GC-aware thread creation on POSIX
  return GC_pthread_create(&thread->handle, NULL, start_routine, arg);
#else
  // Standard pthread creation on POSIX
  return pthread_create(&thread->handle, NULL, start_routine, arg);
#endif
#elif defined(AK24_PLATFORM_WINDOWS)
  // Windows thread creation with adapter for POSIX-style functions
  ak_print_warning(
      "WARNING: Windows threading implementation is UNTESTED CODE\n");

  // Allocate adapter structure to pass both function and arg
  ak_win_thread_adapter_t *adapter =
      (ak_win_thread_adapter_t *)AK24_ALLOC(sizeof(ak_win_thread_adapter_t));
  if (!adapter) {
    return -1;
  }

  adapter->start_routine = start_routine;
  adapter->arg = arg;

  // Create Windows thread
  DWORD thread_id;
  thread->handle =
      CreateThread(NULL,                  // Default security attributes
                   0,                     // Default stack size
                   ak_win_thread_wrapper, // Thread function wrapper
                   adapter,               // Parameter to thread function
                   0,                     // Default creation flags
                   &thread_id             // Receive thread identifier
      );

  if (thread->handle == NULL) {
    AK24_FREE(adapter);
    return -1;
  }

  return 0;
#else
#error "Unsupported platform"
#endif
}

int AK_THREAD_JOIN(AK_THREAD thread) {
#ifdef AK24_PLATFORM_POSIX
  return pthread_join(thread.handle, NULL);
#elif defined(AK24_PLATFORM_WINDOWS)
  // Wait for Windows thread to complete
  if (thread.handle == NULL) {
    return -1;
  }

  DWORD result = WaitForSingleObject(thread.handle, INFINITE);

  if (result == WAIT_OBJECT_0) {
    // Thread completed successfully, close the handle
    CloseHandle(thread.handle);
    return 0;
  }

  // Wait failed
  return -1;
#else
#error "Unsupported platform"
#endif
}

int AK_THREAD_DETACH(AK_THREAD thread) {
#ifdef AK24_PLATFORM_POSIX
  return pthread_detach(thread.handle);
#elif defined(AK24_PLATFORM_WINDOWS)
  // On Windows, detaching means closing the handle immediately
  // This allows the thread to clean up automatically when it exits
  if (thread.handle == NULL) {
    return -1;
  }

  if (CloseHandle(thread.handle)) {
    return 0;
  }

  return -1;
#else
#error "Unsupported platform"
#endif
}
