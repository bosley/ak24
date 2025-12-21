/**
 * @file arena.h
 * @brief Arena allocator for bulk allocation and deallocation
 *
 * Provides an arena (region-based) memory allocator that allocates memory in
 * large blocks and allows freeing everything at once. Critical for compiler
 * phases that create thousands of temporary objects like AST nodes, type
 * checking structures, and IR generation nodes.
 *
 * Key features:
 * - O(1) allocation in common case
 * - Bulk deallocation via reset or free
 * - Automatic block growth when space exhausted
 * - Snapshot/restore for nested scopes
 * - Statistics tracking for memory usage
 * - Helper functions for string duplication
 * - Configurable alignment support
 *
 * Usage pattern:
 * @code
 * ak_arena_t *arena = ak_arena_new(65536);  // 64KB blocks
 *
 * // Allocate many objects
 * MyStruct *obj1 = ak_arena_alloc(arena, sizeof(MyStruct));
 * MyStruct *obj2 = ak_arena_alloc(arena, sizeof(MyStruct));
 *
 * // Free everything at once
 * ak_arena_free(arena);
 * @endcode
 *
 * @note NOT thread-safe - each thread should have its own arena
 * @note Individual allocations cannot be freed (only bulk via reset/free)
 * @note Works with or without GC enabled
 */

#ifndef AK24_ARENA_H
#define AK24_ARENA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Default block size for arena allocations (64KB)
 */
#define AK_ARENA_DEFAULT_BLOCK_SIZE (64 * 1024)

/**
 * @brief Default alignment for arena allocations (8 bytes)
 */
#define AK_ARENA_DEFAULT_ALIGNMENT 8

/**
 * @brief Opaque arena allocator structure
 *
 * Manages a linked list of memory blocks with bump pointer allocation.
 */
typedef struct ak_arena_s ak_arena_t;

/**
 * @brief Snapshot marker for arena state
 *
 * Captures the current position in the arena to enable restoration
 * to that point later, freeing all allocations made after the snapshot.
 */
typedef struct ak_arena_mark_s {
  void *block;      /**< Block pointer at snapshot time */
  size_t offset;    /**< Offset within block at snapshot time */
  size_t block_num; /**< Block number for validation */
} ak_arena_mark_t;

/**
 * @brief Create a new arena with specified block size
 *
 * Allocates an arena allocator that will use the specified block size
 * for each memory block. When a block fills up, a new block of the same
 * size is allocated.
 *
 * @param block_size Size of each memory block in bytes
 * @return New arena allocator, or NULL on allocation failure
 *
 * @note block_size should be large enough to reduce block allocations
 * @note Typical block sizes: 4KB-1MB depending on use case
 *
 * Example:
 * @code
 * ak_arena_t *arena = ak_arena_new(65536);  // 64KB blocks
 * @endcode
 */
ak_arena_t *ak_arena_new(size_t block_size);

/**
 * @brief Create a new arena with default block size
 *
 * Convenience function that creates an arena with the default block size
 * of 64KB, suitable for most use cases.
 *
 * @return New arena allocator, or NULL on allocation failure
 *
 * Example:
 * @code
 * ak_arena_t *arena = ak_arena_new_default();
 * @endcode
 */
ak_arena_t *ak_arena_new_default(void);

/**
 * @brief Free arena and all allocated memory
 *
 * Frees the arena structure and all memory blocks allocated within it.
 * After this call, all pointers returned by arena allocation functions
 * become invalid.
 *
 * @param arena Arena to free
 *
 * @warning Invalidates all pointers allocated from this arena
 *
 * Example:
 * @code
 * ak_arena_free(arena);
 * @endcode
 */
void ak_arena_free(ak_arena_t *arena);

/**
 * @brief Allocate memory from arena
 *
 * Fast O(1) allocation that bumps the current block pointer. If the
 * current block doesn't have enough space, allocates a new block.
 * Memory is NOT zero-initialized.
 *
 * @param arena Arena to allocate from
 * @param size Number of bytes to allocate
 * @return Pointer to allocated memory, or NULL if arena is NULL or size is 0
 *
 * @note Memory is aligned to AK_ARENA_DEFAULT_ALIGNMENT (8 bytes)
 * @note Individual allocations cannot be freed
 * @note NOT zero-initialized - use ak_arena_calloc for zeroed memory
 *
 * Example:
 * @code
 * MyStruct *obj = ak_arena_alloc(arena, sizeof(MyStruct));
 * @endcode
 */
void *ak_arena_alloc(ak_arena_t *arena, size_t size);

/**
 * @brief Allocate aligned memory from arena
 *
 * Like ak_arena_alloc but ensures the returned pointer is aligned to
 * the specified alignment boundary. Alignment must be a power of 2.
 *
 * @param arena Arena to allocate from
 * @param size Number of bytes to allocate
 * @param align Alignment requirement (must be power of 2)
 * @return Aligned pointer to allocated memory, or NULL on failure
 *
 * @note Alignment must be a power of 2
 * @note Wastes up to (align - 1) bytes per allocation
 *
 * Example:
 * @code
 * // Allocate 16-byte aligned memory for SIMD
 * float *vec = ak_arena_alloc_aligned(arena, 16 * sizeof(float), 16);
 * @endcode
 */
void *ak_arena_alloc_aligned(ak_arena_t *arena, size_t size, size_t align);

/**
 * @brief Allocate zero-initialized memory from arena
 *
 * Like ak_arena_alloc but zeros the allocated memory before returning.
 *
 * @param arena Arena to allocate from
 * @param nmemb Number of elements
 * @param size Size of each element
 * @return Pointer to zero-initialized memory, or NULL on failure
 *
 * Example:
 * @code
 * int *array = ak_arena_calloc(arena, 100, sizeof(int));
 * @endcode
 */
void *ak_arena_calloc(ak_arena_t *arena, size_t nmemb, size_t size);

/**
 * @brief Reallocate memory in arena
 *
 * Attempts to grow or shrink an allocation. If the new size is larger
 * and there's room in the current block, grows in place. Otherwise,
 * allocates new memory and copies the old data.
 *
 * @param arena Arena to allocate from
 * @param ptr Previous allocation (must be most recent allocation)
 * @param old_size Previous allocation size
 * @param new_size Desired new size
 * @return Pointer to resized memory, or NULL on failure
 *
 * @note May return a different pointer if relocation is needed
 * @note Only efficient if ptr is the most recent allocation
 * @note Consider using ak_arena_alloc for new allocations instead
 *
 * Example:
 * @code
 * char *buf = ak_arena_alloc(arena, 100);
 * buf = ak_arena_realloc(arena, buf, 100, 200);
 * @endcode
 */
void *ak_arena_realloc(ak_arena_t *arena, void *ptr, size_t old_size,
                       size_t new_size);

/**
 * @brief Reset arena and reuse memory
 *
 * Frees all allocations but keeps the memory blocks for reuse. This is
 * much faster than freeing and reallocating the arena. Invalidates all
 * pointers returned by previous allocations.
 *
 * @param arena Arena to reset
 *
 * @warning Invalidates all pointers allocated from this arena
 *
 * Example:
 * @code
 * // Parse multiple files with the same arena
 * for (each file) {
 *   ak_arena_reset(arena);
 *   parse_file(arena, file);
 * }
 * @endcode
 */
void ak_arena_reset(ak_arena_t *arena);

/**
 * @brief Save current arena position
 *
 * Creates a snapshot of the current arena state that can be restored
 * later. Useful for nested scopes where you want to free temporary
 * allocations without affecting outer scope allocations.
 *
 * @param arena Arena to snapshot
 * @return Snapshot marker
 *
 * Example:
 * @code
 * ak_arena_mark_t mark = ak_arena_snapshot(arena);
 * // ... temporary allocations ...
 * ak_arena_restore(arena, mark);  // Free temporary allocations
 * @endcode
 */
ak_arena_mark_t ak_arena_snapshot(ak_arena_t *arena);

/**
 * @brief Restore arena to snapshot position
 *
 * Frees all allocations made after the snapshot was taken. Does not
 * free memory blocks, just resets the allocation position.
 *
 * @param arena Arena to restore
 * @param mark Snapshot marker from ak_arena_snapshot
 *
 * @warning Invalidates all pointers allocated after the snapshot
 *
 * Example:
 * @code
 * ak_arena_mark_t mark = ak_arena_snapshot(arena);
 * MyStruct *temp = ak_arena_alloc(arena, sizeof(MyStruct));
 * ak_arena_restore(arena, mark);  // temp is now invalid
 * @endcode
 */
void ak_arena_restore(ak_arena_t *arena, ak_arena_mark_t mark);

/**
 * @brief Get arena memory usage statistics
 *
 * Returns statistics about memory usage in the arena, including total
 * allocated capacity and current usage.
 *
 * @param arena Arena to query
 * @param allocated Output: total bytes allocated in blocks (may be NULL)
 * @param used Output: bytes currently used for allocations (may be NULL)
 *
 * Example:
 * @code
 * size_t allocated, used;
 * ak_arena_stats(arena, &allocated, &used);
 * printf("Arena: %zu / %zu bytes used\n", used, allocated);
 * @endcode
 */
void ak_arena_stats(ak_arena_t *arena, size_t *allocated, size_t *used);

/**
 * @brief Duplicate string into arena
 *
 * Allocates space in the arena and copies the null-terminated string.
 * Convenient for copying strings without manual length calculation.
 *
 * @param arena Arena to allocate from
 * @param str String to duplicate
 * @return Pointer to duplicated string in arena, or NULL if str is NULL
 *
 * Example:
 * @code
 * const char *name = ak_arena_strdup(arena, "variable_name");
 * @endcode
 */
char *ak_arena_strdup(ak_arena_t *arena, const char *str);

/**
 * @brief Duplicate substring into arena
 *
 * Allocates space in the arena and copies the first n characters of the
 * string, adding a null terminator.
 *
 * @param arena Arena to allocate from
 * @param str String to duplicate
 * @param n Number of characters to copy
 * @return Pointer to duplicated string in arena, or NULL if str is NULL
 *
 * Example:
 * @code
 * const char *token = ak_arena_strndup(arena, "identifier+123", 10);
 * // token = "identifier"
 * @endcode
 */
char *ak_arena_strndup(ak_arena_t *arena, const char *str, size_t n);

#endif /* AK24_ARENA_H */
