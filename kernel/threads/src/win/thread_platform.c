/**
 * @file thread_platform.c
 * @brief Windows implementation of threading primitives (STUB)
 *
 * Provides stub implementations of the platform abstraction layer for Windows.
 * These functions are not yet implemented and will trigger assertions if
 * called.
 *
 * TODO: Implement the following using Windows API:
 * - CRITICAL_SECTION for mutexes
 * - CONDITION_VARIABLE for condition variables
 * - CreateThread/WaitForSingleObject for threads
 *
 * References:
 * -
 * https://docs.microsoft.com/en-us/windows/win32/sync/critical-section-objects
 * - https://docs.microsoft.com/en-us/windows/win32/sync/condition-variables
 * - https://docs.microsoft.com/en-us/windows/win32/procthread/creating-threads
 */

#include "thread_platform.h"
#include <assert.h>
#include <stdio.h>

// Helper to mark functions as not implemented
#define NOT_IMPLEMENTED()                                                      \
  do {                                                                         \
    fprintf(stderr, "ERROR: %s not implemented for Windows\n", __func__);      \
    assert(0 && "Windows threading not yet implemented");                      \
    return -1;                                                                 \
  } while (0)

int ak_mutex_init(ak_mutex_t *mutex) {
  if (!mutex) {
    return -1;
  }
  // TODO: InitializeCriticalSection(&mutex->handle);
  NOT_IMPLEMENTED();
}

int ak_mutex_destroy(ak_mutex_t *mutex) {
  if (!mutex) {
    return -1;
  }
  // TODO: DeleteCriticalSection(&mutex->handle);
  NOT_IMPLEMENTED();
}

int ak_mutex_lock(ak_mutex_t *mutex) {
  if (!mutex) {
    return -1;
  }
  // TODO: EnterCriticalSection(&mutex->handle);
  NOT_IMPLEMENTED();
}

int ak_mutex_unlock(ak_mutex_t *mutex) {
  if (!mutex) {
    return -1;
  }
  // TODO: LeaveCriticalSection(&mutex->handle);
  NOT_IMPLEMENTED();
}

int ak_cond_init(ak_cond_t *cond) {
  if (!cond) {
    return -1;
  }
  // TODO: InitializeConditionVariable(&cond->handle);
  NOT_IMPLEMENTED();
}

int ak_cond_destroy(ak_cond_t *cond) {
  if (!cond) {
    return -1;
  }
  // TODO: Windows condition variables don't need explicit cleanup
  // Just return success after implementation
  NOT_IMPLEMENTED();
}

int ak_cond_wait(ak_cond_t *cond, ak_mutex_t *mutex) {
  if (!cond || !mutex) {
    return -1;
  }
  // TODO: SleepConditionVariableCS(&cond->handle, &mutex->handle, INFINITE);
  NOT_IMPLEMENTED();
}

int ak_cond_signal(ak_cond_t *cond) {
  if (!cond) {
    return -1;
  }
  // TODO: WakeConditionVariable(&cond->handle);
  NOT_IMPLEMENTED();
}

int ak_cond_broadcast(ak_cond_t *cond) {
  if (!cond) {
    return -1;
  }
  // TODO: WakeAllConditionVariable(&cond->handle);
  NOT_IMPLEMENTED();
}

/**
 * @brief Thread start data wrapper for Windows
 *
 * Windows threads use DWORD WINAPI signature, but we need to support
 * the POSIX-style void* (*)(void*) signature. This wrapper converts.
 */
typedef struct {
  ak_thread_start_fn start_routine;
  void *arg;
} win_thread_start_data_t;

/**
 * @brief Windows thread entry point wrapper
 *
 * Converts from Windows thread signature to POSIX signature.
 *
 * TODO: Implement using Windows API
 */
static DWORD WINAPI win_thread_start_wrapper(LPVOID param) {
  win_thread_start_data_t *data = (win_thread_start_data_t *)param;
  // TODO: Call data->start_routine(data->arg)
  // TODO: Free data
  // TODO: Return 0
  (void)data;
  fprintf(stderr, "ERROR: Windows thread wrapper not implemented\n");
  assert(0 && "Windows threading not yet implemented");
  return (DWORD)-1;
}

int ak_thread_create(ak_thread_t *thread, ak_thread_start_fn start_routine,
                     void *arg) {
  if (!thread || !start_routine) {
    return -1;
  }

  // TODO: Allocate win_thread_start_data_t
  // TODO: data->start_routine = start_routine;
  // TODO: data->arg = arg;
  // TODO: thread->handle = CreateThread(NULL, 0, win_thread_start_wrapper,
  // data, 0, NULL); TODO: Check for NULL handle and cleanup on failure
  (void)arg;
  NOT_IMPLEMENTED();
}

int ak_thread_join(ak_thread_t thread) {
  // TODO: WaitForSingleObject(thread.handle, INFINITE);
  // TODO: CloseHandle(thread.handle);
  (void)thread;
  NOT_IMPLEMENTED();
}
