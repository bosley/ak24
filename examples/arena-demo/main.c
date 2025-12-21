/**
 * @file arena_demo.c
 * @brief Demonstration of arena allocator usage patterns
 */

#include "kernel.h"
#include <stdio.h>

// Example AST node for a simple expression tree
typedef struct ast_node_s {
  enum { NODE_NUMBER, NODE_ADD, NODE_MUL } type;
  union {
    int number;
    struct {
      struct ast_node_s *left;
      struct ast_node_s *right;
    } binary;
  } data;
} ast_node_t;

// Create AST nodes using arena
ast_node_t *make_number(ak_arena_t *arena, int value) {
  ast_node_t *node = ak_arena_alloc(arena, sizeof(ast_node_t));
  node->type = NODE_NUMBER;
  node->data.number = value;
  return node;
}

ast_node_t *make_binary(ak_arena_t *arena, int type, ast_node_t *left,
                        ast_node_t *right) {
  ast_node_t *node = ak_arena_alloc(arena, sizeof(ast_node_t));
  node->type = type;
  node->data.binary.left = left;
  node->data.binary.right = right;
  return node;
}

// Evaluate AST
int eval(ast_node_t *node) {
  switch (node->type) {
  case NODE_NUMBER:
    return node->data.number;
  case NODE_ADD:
    return eval(node->data.binary.left) + eval(node->data.binary.right);
  case NODE_MUL:
    return eval(node->data.binary.left) * eval(node->data.binary.right);
  }
  return 0;
}

void demo_basic_usage(void) {
  printf("\n=== Basic Arena Usage ===\n");

  // Create arena with default block size
  ak_arena_t *arena = ak_arena_new_default();

  // Allocate some objects
  int *nums = ak_arena_calloc(arena, 10, sizeof(int));
  for (int i = 0; i < 10; i++) {
    nums[i] = i * i;
  }

  printf("Squares: ");
  for (int i = 0; i < 10; i++) {
    printf("%d ", nums[i]);
  }
  printf("\n");

  // Get statistics
  size_t allocated, used;
  ak_arena_stats(arena, &allocated, &used);
  printf("Memory: %zu bytes used of %zu allocated (%.1f%% efficiency)\n", used,
         allocated, (double)used / allocated * 100);

  // Free everything at once
  ak_arena_free(arena);
  printf("Arena freed (all allocations released)\n");
}

void demo_ast_construction(void) {
  printf("\n=== AST Construction with Arena ===\n");

  ak_arena_t *arena = ak_arena_new_default();

  // Build AST for: (3 + 5) * (10 + 2)
  ast_node_t *ast =
      make_binary(arena, NODE_MUL,
                  make_binary(arena, NODE_ADD, make_number(arena, 3),
                              make_number(arena, 5)),
                  make_binary(arena, NODE_ADD, make_number(arena, 10),
                              make_number(arena, 2)));

  int result = eval(ast);
  printf("Expression: (3 + 5) * (10 + 2) = %d\n", result);

  size_t allocated, used;
  ak_arena_stats(arena, &allocated, &used);
  printf("AST used %zu bytes for 7 nodes\n", used);

  // Free entire AST with one call
  ak_arena_free(arena);
  printf("Entire AST freed at once\n");
}

void demo_reset_reuse(void) {
  printf("\n=== Reset and Reuse ===\n");

  ak_arena_t *arena = ak_arena_new(4096);

  // Process multiple "files"
  const char *files[] = {"file1.txt", "file2.txt", "file3.txt"};

  for (int i = 0; i < 3; i++) {
    // Simulate parsing a file
    char *filename = ak_arena_strdup(arena, files[i]);
    char *content = ak_arena_calloc(arena, 100, 1);
    snprintf(content, 100, "Content of %s", filename);

    printf("Processing %s: %s\n", filename, content);

    size_t _, used;
    ak_arena_stats(arena, &_, &used);
    printf("  Used %zu bytes\n", used);

    // Reset for next file (keeps memory blocks)
    ak_arena_reset(arena);
  }

  printf("Processed all files with same arena\n");
  ak_arena_free(arena);
}

void demo_snapshot_restore(void) {
  printf("\n=== Snapshot and Restore ===\n");

  ak_arena_t *arena = ak_arena_new_default();

  // Outer scope allocation
  char *outer = ak_arena_strdup(arena, "outer scope");
  printf("Created: %s\n", outer);

  // Save snapshot
  ak_arena_mark_t mark = ak_arena_snapshot(arena);
  printf("Snapshot saved\n");

  // Inner scope allocations
  char *inner1 = ak_arena_strdup(arena, "inner scope 1");
  char *inner2 = ak_arena_strdup(arena, "inner scope 2");
  printf("Created: %s\n", inner1);
  printf("Created: %s\n", inner2);

  size_t _, used_before;
  ak_arena_stats(arena, &_, &used_before);
  printf("Used before restore: %zu bytes\n", used_before);

  // Restore (frees inner allocations)
  ak_arena_restore(arena, mark);
  printf("Restored to snapshot\n");

  size_t used_after;
  ak_arena_stats(arena, &_, &used_after);
  printf("Used after restore: %zu bytes (freed %zu bytes)\n", used_after,
         used_before - used_after);

  // outer is still valid, inner1/inner2 are not
  printf("%s is still valid\n", outer);

  ak_arena_free(arena);
}

void demo_string_operations(void) {
  printf("\n=== String Operations ===\n");

  ak_arena_t *arena = ak_arena_new_default();

  // String duplication
  const char *src = "Hello, Arena!";
  char *copy = ak_arena_strdup(arena, src);
  printf("Duplicated: '%s'\n", copy);

  // Substring duplication
  const char *token = "identifier+123";
  char *id = ak_arena_strndup(arena, token, 10);
  printf("Substring: '%s'\n", id);

  // Build path in arena
  char *dir = ak_arena_strdup(arena, "/usr/local");
  char *file = ak_arena_strdup(arena, "bin");
  // Could concatenate using arena_alloc + sprintf
  size_t path_len = strlen(dir) + 1 + strlen(file) + 1;
  char *path = ak_arena_alloc(arena, path_len);
  snprintf(path, path_len, "%s/%s", dir, file);
  printf("Path: %s\n", path);

  ak_arena_free(arena);
}

int main(void) {
  ak_kernel_init("arena-demo");

  printf("╔════════════════════════════════════════╗\n");
  printf("║   Arena Allocator Demo                 ║\n");
  printf("╚════════════════════════════════════════╝\n");

  demo_basic_usage();
  demo_ast_construction();
  demo_reset_reuse();
  demo_snapshot_restore();
  demo_string_operations();

  printf("\n✓ All demos completed successfully!\n");

  ak_kernel_deinit();
  return 0;
}
