#include "kernel.h"
#include "thread_platform.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#ifdef AK24_PLATFORM_WINDOWS
#include <windows.h>

/**
 * @brief Print warning message in bright red (Windows console)
 *
 * Prints a formatted warning message in bright red on Windows console.
 *
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
void ak_print_warning(const char *format, ...) {
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
}

// Windows stubs for signal handling
void ak_register_signal_handler(int signum, ak_lambda_t *handler) {
  // Windows does not support POSIX signals
  (void)signum;
  (void)handler;
}

void ak_unregister_signal_handler(int signum) {
  // Windows does not support POSIX signals
  (void)signum;
}

void ak_signal_handlers_init(void) {
  // No-op on Windows
}

void ak_signal_handlers_deinit(void) {
  // No-op on Windows
}

// Mutex implementations

int AK_mutex_init(AK_MUTEX *mutex) {
  if (!mutex) {
    return -1;
  }
  InitializeCriticalSection(&mutex->handle);
  return 0;
}

int AK_mutex_destroy(AK_MUTEX *mutex) {
  if (!mutex) {
    return -1;
  }
  DeleteCriticalSection(&mutex->handle);
  return 0;
}

int AK_mutex_lock(AK_MUTEX *mutex) {
  if (!mutex) {
    return -1;
  }
  EnterCriticalSection(&mutex->handle);
  return 0;
}

int AK_mutex_unlock(AK_MUTEX *mutex) {
  if (!mutex) {
    return -1;
  }
  LeaveCriticalSection(&mutex->handle);
  return 0;
}

// Condition variable implementations

int AK_cond_init(AK_COND *cond) {
  if (!cond) {
    return -1;
  }
  InitializeConditionVariable(&cond->handle);
  return 0;
}

int AK_cond_destroy(AK_COND *cond) {
  if (!cond) {
    return -1;
  }
  // Windows condition variables don't need explicit destruction
  return 0;
}

int AK_cond_wait(AK_COND *cond, AK_MUTEX *mutex) {
  if (!cond || !mutex) {
    return -1;
  }
  return SleepConditionVariableCS(&cond->handle, &mutex->handle, INFINITE) ? 0
                                                                           : -1;
}

int AK_cond_signal(AK_COND *cond) {
  if (!cond) {
    return -1;
  }
  WakeConditionVariable(&cond->handle);
  return 0;
}

int AK_cond_broadcast(AK_COND *cond) {
  if (!cond) {
    return -1;
  }
  WakeAllConditionVariable(&cond->handle);
  return 0;
}

// Thread wrapper structure for Windows to adapt POSIX-style thread functions
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

// Thread implementations

int AK_THREAD_CREATE(AK_THREAD *thread, void *(*start_routine)(void *),
                     void *arg) {
  if (!thread || !start_routine) {
    return -1;
  }

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
}

int AK_THREAD_JOIN(AK_THREAD thread) {
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
}

int AK_THREAD_DETACH(AK_THREAD thread) {
  // On Windows, detaching means closing the handle immediately
  // This allows the thread to clean up automatically when it exits
  if (thread.handle == NULL) {
    return -1;
  }

  if (CloseHandle(thread.handle)) {
    return 0;
  }

  return -1;
}

#endif // AK24_PLATFORM_WINDOWS
