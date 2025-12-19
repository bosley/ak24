#include "arbuff.h"
#include <stddef.h>

int main(void) {
  ak_arbuff_t *arbuff = ak_arbuff_new(10);
  ak_arbuff_push(arbuff, (void *)0x1234);
  void *item = ak_arbuff_pop(arbuff);
  (void)item;
  ak_arbuff_free(arbuff);
  return 0;
}
