/**
 * @file thread_platform.c
 * @brief Platform-agnostic threading primitives implementation
 *
 * Implements the platform abstraction layer by wrapping kernel-level
 * threading primitives. Since kernel.h handles all platform and GC
 * concerns, this implementation works identically on all platforms.
 */

#include "thread_platform.h"

int ak_mutex_init(ak_mutex_t *mutex) {
  if (!mutex) {
    return -1;
  }
  return AK24_mutex_init(&mutex->mutex);
}

int ak_mutex_destroy(ak_mutex_t *mutex) {
  if (!mutex) {
    return -1;
  }
  return AK24_mutex_destroy(&mutex->mutex);
}

int ak_mutex_lock(ak_mutex_t *mutex) {
  if (!mutex) {
    return -1;
  }
  return AK24_mutex_lock(&mutex->mutex);
}

int ak_mutex_unlock(ak_mutex_t *mutex) {
  if (!mutex) {
    return -1;
  }
  return AK24_mutex_unlock(&mutex->mutex);
}

int ak_cond_init(ak_cond_t *cond) {
  if (!cond) {
    return -1;
  }
  return AK24_cond_init(&cond->cond);
}

int ak_cond_destroy(ak_cond_t *cond) {
  if (!cond) {
    return -1;
  }
  return AK24_cond_destroy(&cond->cond);
}

int ak_cond_wait(ak_cond_t *cond, ak_mutex_t *mutex) {
  if (!cond || !mutex) {
    return -1;
  }
  return AK24_cond_wait(&cond->cond, &mutex->mutex);
}

int ak_cond_signal(ak_cond_t *cond) {
  if (!cond) {
    return -1;
  }
  return AK24_cond_signal(&cond->cond);
}

int ak_cond_broadcast(ak_cond_t *cond) {
  if (!cond) {
    return -1;
  }
  return AK24_cond_broadcast(&cond->cond);
}

int ak_thread_create(ak_thread_t *thread, ak_thread_start_fn start_routine,
                     void *arg) {
  if (!thread || !start_routine) {
    return -1;
  }
  return AK24_THREAD_CREATE(&thread->thread, start_routine, arg);
}

int ak_thread_join(ak_thread_t thread) {
  return AK24_THREAD_JOIN(thread.thread);
}
