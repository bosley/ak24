#include "kernel.h"
#include "thread_platform.h"
#include <pthread.h>
#include <signal.h>

#ifdef AK24_PLATFORM_POSIX

// Signal handling infrastructure
typedef struct {
  int signum;
  ak_lambda_t *handler;
  struct sigaction old_action;
} ak_signal_handler_entry_t;

static list_void_t signal_handlers;
static int signal_handlers_initialized = 0;
static AK24_MUTEX signal_mutex = AK24_MUTEX_INITIALIZER;

// Forward declarations
static void ak_signal_dispatch(int signum);

static void ak_signal_dispatch(int signum) {
  AK24_MUTEX_LOCK(&signal_mutex);

  if (!signal_handlers_initialized) {
    AK24_MUTEX_UNLOCK(&signal_mutex);
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

  AK24_MUTEX_UNLOCK(&signal_mutex);
}

void ak_register_signal_handler(int signum, ak_lambda_t *handler) {
  if (!handler || !signal_handlers_initialized) {
    return;
  }

  AK24_MUTEX_LOCK(&signal_mutex);

  // Check if handler already exists for this signal
  list_iter_t iter = list_iter(&signal_handlers);
  void **entry_ptr;
  while ((entry_ptr = list_next(&signal_handlers, &iter))) {
    ak_signal_handler_entry_t *entry = (ak_signal_handler_entry_t *)*entry_ptr;
    if (entry && entry->signum == signum) {
      // Update existing handler
      entry->handler = handler;
      AK24_MUTEX_UNLOCK(&signal_mutex);
      return;
    }
  }

  // Create new handler entry
  ak_signal_handler_entry_t *entry =
      AK24_ALLOC(sizeof(ak_signal_handler_entry_t));
  if (!entry) {
    AK24_MUTEX_UNLOCK(&signal_mutex);
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

  AK24_MUTEX_UNLOCK(&signal_mutex);
}

void ak_unregister_signal_handler(int signum) {
  AK24_MUTEX_LOCK(&signal_mutex);

  if (!signal_handlers_initialized) {
    AK24_MUTEX_UNLOCK(&signal_mutex);
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

  AK24_MUTEX_UNLOCK(&signal_mutex);
}

void ak_signal_handlers_init(void) {
  list_init(&signal_handlers);
  signal_handlers_initialized = 1;
}

void ak_signal_handlers_deinit(void) {
  if (!signal_handlers_initialized) {
    return;
  }

  AK24_MUTEX_LOCK(&signal_mutex);

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

  AK24_MUTEX_UNLOCK(&signal_mutex);
}

// Mutex implementations

int AK24_mutex_init(AK24_MUTEX *mutex) {
  if (!mutex) {
    return -1;
  }
  return pthread_mutex_init(&mutex->handle, NULL);
}

int AK24_mutex_destroy(AK24_MUTEX *mutex) {
  if (!mutex) {
    return -1;
  }
  return pthread_mutex_destroy(&mutex->handle);
}

int AK24_mutex_lock(AK24_MUTEX *mutex) {
  if (!mutex) {
    return -1;
  }
  return pthread_mutex_lock(&mutex->handle);
}

int AK24_mutex_unlock(AK24_MUTEX *mutex) {
  if (!mutex) {
    return -1;
  }
  return pthread_mutex_unlock(&mutex->handle);
}

// Condition variable implementations

int AK24_cond_init(AK24_COND *cond) {
  if (!cond) {
    return -1;
  }
  return pthread_cond_init(&cond->handle, NULL);
}

int AK24_cond_destroy(AK24_COND *cond) {
  if (!cond) {
    return -1;
  }
  return pthread_cond_destroy(&cond->handle);
}

int AK24_cond_wait(AK24_COND *cond, AK24_MUTEX *mutex) {
  if (!cond || !mutex) {
    return -1;
  }
  return pthread_cond_wait(&cond->handle, &mutex->handle);
}

int AK24_cond_signal(AK24_COND *cond) {
  if (!cond) {
    return -1;
  }
  return pthread_cond_signal(&cond->handle);
}

int AK24_cond_broadcast(AK24_COND *cond) {
  if (!cond) {
    return -1;
  }
  return pthread_cond_broadcast(&cond->handle);
}

// Thread implementations

int AK24_THREAD_CREATE(AK24_THREAD *thread, void *(*start_routine)(void *),
                       void *arg) {
  if (!thread || !start_routine) {
    return -1;
  }

#if AK24_GC_ENABLED
  // GC-aware thread creation on POSIX
  return GC_pthread_create(&thread->handle, NULL, start_routine, arg);
#else
  // Standard pthread creation on POSIX
  return pthread_create(&thread->handle, NULL, start_routine, arg);
#endif
}

int AK24_THREAD_JOIN(AK24_THREAD thread) {
  return pthread_join(thread.handle, NULL);
}

int AK24_THREAD_DETACH(AK24_THREAD thread) {
  return pthread_detach(thread.handle);
}

#endif // AK24_PLATFORM_POSIX
