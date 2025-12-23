/**
 * @file box.c
 * @brief Thread-safe container implementation
 */

#include "box.h"
#include "kernel.h"

/**
 * @brief Internal box structure
 */
struct ak_box_t {
  void *data;       /**< Boxed data pointer (not owned) */
  AK24_MUTEX mutex; /**< Mutex for thread-safe operations */
};

ak_box_t *ak_box_new(void *data) {
  ak_box_t *box = AK24_ALLOC(sizeof(ak_box_t));
  if (!box) {
    return NULL;
  }

  box->data = data;

  if (AK24_MUTEX_INIT(&box->mutex) != 0) {
    AK24_FREE(box);
    return NULL;
  }

  return box;
}

void ak_box_free(ak_box_t *box) {
  if (!box) {
    return;
  }

  AK24_MUTEX_DESTROY(&box->mutex);
  AK24_FREE(box);
}

void *ak_box_swap(ak_box_t *box, void *new_data) {
  if (!box) {
    return NULL;
  }

  AK24_MUTEX_LOCK(&box->mutex);

  void *old_data = box->data;
  box->data = new_data;

  AK24_MUTEX_UNLOCK(&box->mutex);

  return old_data;
}

void *ak_box_get(ak_box_t *box) {
  if (!box) {
    return NULL;
  }

  AK24_MUTEX_LOCK(&box->mutex);
  void *data = box->data;
  AK24_MUTEX_UNLOCK(&box->mutex);

  return data;
}

int ak_box_visit(ak_box_t *box, ak_lambda_t *lambda) {
  if (!box || !lambda) {
    return -1;
  }

  AK24_MUTEX_LOCK(&box->mutex);

  ak_lambda_invoke(lambda, box->data);

  AK24_MUTEX_UNLOCK(&box->mutex);

  return 0;
}
