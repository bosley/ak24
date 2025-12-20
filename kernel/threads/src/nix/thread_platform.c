/**
 * @file thread_platform.c
 * @brief POSIX implementation of threading primitives
 *
 * Implements the platform abstraction layer using POSIX threads (pthreads).
 * This implementation is used on Linux, macOS, BSD, and other POSIX-compliant
 * systems.
 */

#include "thread_platform.h"
#include "kernel.h"
#include <errno.h>

int ak_mutex_init(ak_mutex_t *mutex) {
  if (!mutex) {
    return -1;
  }
  return pthread_mutex_init(&mutex->handle, NULL);
}

int ak_mutex_destroy(ak_mutex_t *mutex) {
  if (!mutex) {
    return -1;
  }
  return pthread_mutex_destroy(&mutex->handle);
}

int ak_mutex_lock(ak_mutex_t *mutex) {
  if (!mutex) {
    return -1;
  }
  return pthread_mutex_lock(&mutex->handle);
}

int ak_mutex_unlock(ak_mutex_t *mutex) {
  if (!mutex) {
    return -1;
  }
  return pthread_mutex_unlock(&mutex->handle);
}

int ak_cond_init(ak_cond_t *cond) {
  if (!cond) {
    return -1;
  }
  return pthread_cond_init(&cond->handle, NULL);
}

int ak_cond_destroy(ak_cond_t *cond) {
  if (!cond) {
    return -1;
  }
  return pthread_cond_destroy(&cond->handle);
}

int ak_cond_wait(ak_cond_t *cond, ak_mutex_t *mutex) {
  if (!cond || !mutex) {
    return -1;
  }
  return pthread_cond_wait(&cond->handle, &mutex->handle);
}

int ak_cond_signal(ak_cond_t *cond) {
  if (!cond) {
    return -1;
  }
  return pthread_cond_signal(&cond->handle);
}

int ak_cond_broadcast(ak_cond_t *cond) {
  if (!cond) {
    return -1;
  }
  return pthread_cond_broadcast(&cond->handle);
}

int ak_thread_create(ak_thread_t *thread, ak_thread_start_fn start_routine,
                     void *arg) {
  if (!thread || !start_routine) {
    return -1;
  }
  return AK24_THREAD_CREATE(&thread->handle, NULL, start_routine, arg);
}

int ak_thread_join(ak_thread_t thread) {
  return AK24_THREAD_JOIN(thread.handle, NULL);
}
