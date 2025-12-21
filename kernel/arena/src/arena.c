#include "arena.h"
#include "kernel.h"
#include <string.h>

/**
 * @brief Memory block in the arena
 */
typedef struct arena_block_s {
  uint8_t *data;              /**< Block data */
  size_t size;                /**< Block size */
  size_t used;                /**< Bytes used in this block */
  size_t block_num;           /**< Block number for snapshot validation */
  struct arena_block_s *next; /**< Next block in chain */
} arena_block_t;

/**
 * @brief Arena allocator structure
 */
struct ak_arena_s {
  arena_block_t *first_block;   /**< First block in chain */
  arena_block_t *current_block; /**< Current block for allocation */
  size_t block_size;            /**< Size for new blocks */
  size_t total_allocated;       /**< Total bytes in all blocks */
  size_t total_used;            /**< Total bytes used across all blocks */
  size_t next_block_num;        /**< Counter for block numbers */
};

/**
 * @brief Align a value up to the specified alignment
 */
static inline size_t align_up(size_t value, size_t align) {
  return (value + align - 1) & ~(align - 1);
}

/**
 * @brief Allocate a new memory block
 */
static arena_block_t *allocate_block(size_t size, size_t block_num) {
  arena_block_t *block = AK24_ALLOC(sizeof(arena_block_t));
  if (!block) {
    return NULL;
  }

  block->data = AK24_ALLOC_ATOMIC(size);
  if (!block->data) {
    AK24_FREE(block);
    return NULL;
  }

  block->size = size;
  block->used = 0;
  block->block_num = block_num;
  block->next = NULL;

  return block;
}

/**
 * @brief Free a block and all blocks in its chain
 */
static void free_block_chain(arena_block_t *block) {
  while (block) {
    arena_block_t *next = block->next;
    AK24_FREE(block->data);
    AK24_FREE(block);
    block = next;
  }
}

ak_arena_t *ak_arena_new(size_t block_size) {
  if (block_size == 0) {
    return NULL;
  }

  ak_arena_t *arena = AK24_ALLOC(sizeof(ak_arena_t));
  if (!arena) {
    return NULL;
  }

  arena_block_t *block = allocate_block(block_size, 0);
  if (!block) {
    AK24_FREE(arena);
    return NULL;
  }

  arena->first_block = block;
  arena->current_block = block;
  arena->block_size = block_size;
  arena->total_allocated = block_size;
  arena->total_used = 0;
  arena->next_block_num = 1;

  return arena;
}

ak_arena_t *ak_arena_new_default(void) {
  return ak_arena_new(AK_ARENA_DEFAULT_BLOCK_SIZE);
}

void ak_arena_free(ak_arena_t *arena) {
  if (!arena) {
    return;
  }

  free_block_chain(arena->first_block);
  AK24_FREE(arena);
}

void *ak_arena_alloc(ak_arena_t *arena, size_t size) {
  return ak_arena_alloc_aligned(arena, size, AK_ARENA_DEFAULT_ALIGNMENT);
}

void *ak_arena_alloc_aligned(ak_arena_t *arena, size_t size, size_t align) {
  if (!arena || size == 0) {
    return NULL;
  }

  // Validate alignment is power of 2
  if (align == 0 || (align & (align - 1)) != 0) {
    return NULL;
  }

  arena_block_t *block = arena->current_block;

  // Calculate aligned offset in current block
  size_t aligned_offset = align_up(block->used, align);
  size_t needed = aligned_offset - block->used + size;

  // Check if current block has enough space
  if (block->used + needed > block->size) {
    // Need a new block
    size_t new_block_size = arena->block_size;

    // If allocation is larger than default block size, use larger block
    if (size > new_block_size) {
      new_block_size = align_up(size, arena->block_size);
    }

    arena_block_t *new_block =
        allocate_block(new_block_size, arena->next_block_num++);
    if (!new_block) {
      return NULL;
    }

    block->next = new_block;
    arena->current_block = new_block;
    arena->total_allocated += new_block_size;

    // New block - starts alogned ofc
    block = new_block;
    aligned_offset = 0;
    needed = size;
  }

  // allocate from current block
  void *ptr = block->data + aligned_offset;
  block->used = aligned_offset + size;
  arena->total_used += needed;

  return ptr;
}

void *ak_arena_calloc(ak_arena_t *arena, size_t nmemb, size_t size) {
  if (!arena || nmemb == 0 || size == 0) {
    return NULL;
  }

  // Check for overflow
  size_t total_size = nmemb * size;
  if (total_size / nmemb != size) {
    return NULL;
  }

  void *ptr = ak_arena_alloc(arena, total_size);
  if (ptr) {
    memset(ptr, 0, total_size);
  }

  return ptr;
}

void *ak_arena_realloc(ak_arena_t *arena, void *ptr, size_t old_size,
                       size_t new_size) {
  if (!arena) {
    return NULL;
  }

  if (new_size == 0) {
    return NULL;
  }

  if (!ptr) {
    return ak_arena_alloc(arena, new_size);
  }

  // If shrinking or staying same size, just return the same pointer
  if (new_size <= old_size) {
    return ptr;
  }

  arena_block_t *block = arena->current_block;

  // Check if ptr is at the end of current block (most recent allocation)
  // If so, we can grow in place if there's room
  uint8_t *ptr_bytes = (uint8_t *)ptr;
  if (ptr_bytes + old_size == block->data + block->used) {
    size_t additional = new_size - old_size;
    if (block->used + additional <= block->size) {
      // Can grow in place
      block->used += additional;
      arena->total_used += additional;
      return ptr;
    }
  }

  // Allocate new memory and copy
  void *new_ptr = ak_arena_alloc(arena, new_size);
  if (!new_ptr) {
    return NULL;
  }

  memcpy(new_ptr, ptr, old_size);
  return new_ptr;
}

void ak_arena_reset(ak_arena_t *arena) {
  if (!arena) {
    return;
  }

  arena_block_t *block = arena->first_block;
  while (block) {
    block->used = 0;
    block = block->next;
  }

  arena->current_block = arena->first_block;
  arena->total_used = 0;
}

ak_arena_mark_t ak_arena_snapshot(ak_arena_t *arena) {
  ak_arena_mark_t mark = {0};

  if (!arena) {
    return mark;
  }

  mark.block = arena->current_block;
  mark.offset = arena->current_block->used;
  mark.block_num = arena->current_block->block_num;

  return mark;
}

void ak_arena_restore(ak_arena_t *arena, ak_arena_mark_t mark) {
  if (!arena || !mark.block) {
    return;
  }

  arena_block_t *block = (arena_block_t *)mark.block;

  // Validate block is still valid (block number matches)
  if (block->block_num != mark.block_num) {
    return;
  }

  size_t reclaimed = 0;

  arena_block_t *current = arena->first_block;
  while (current && current != block) {
    current = current->next;
  }

  if (current != block) {
    return;
  }

  // Reclaim from blocks after snapshot
  arena_block_t *next = block->next;
  while (next) {
    reclaimed += next->used;
    next->used = 0;
    next = next->next;
  }

  reclaimed += block->used - mark.offset;
  block->used = mark.offset;

  arena->current_block = block;
  arena->total_used -= reclaimed;
}

void ak_arena_stats(ak_arena_t *arena, size_t *allocated, size_t *used) {
  if (!arena) {
    if (allocated)
      *allocated = 0;
    if (used)
      *used = 0;
    return;
  }

  if (allocated) {
    *allocated = arena->total_allocated;
  }

  if (used) {
    *used = arena->total_used;
  }
}

char *ak_arena_strdup(ak_arena_t *arena, const char *str) {
  if (!arena || !str) {
    return NULL;
  }

  size_t len = strlen(str);
  char *copy = ak_arena_alloc(arena, len + 1);
  if (!copy) {
    return NULL;
  }

  memcpy(copy, str, len + 1);
  return copy;
}

char *ak_arena_strndup(ak_arena_t *arena, const char *str, size_t n) {
  if (!arena || !str) {
    return NULL;
  }

  // Find actual length (might be less than n if null terminator found)
  size_t len = 0;
  while (len < n && str[len] != '\0') {
    len++;
  }

  char *copy = ak_arena_alloc(arena, len + 1);
  if (!copy) {
    return NULL;
  }

  memcpy(copy, str, len);
  copy[len] = '\0';

  return copy;
}
