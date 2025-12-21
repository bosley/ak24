/**
 * @file sourceloc_demo.c
 * @brief Demonstration of source location tracking usage patterns
 */

#include "kernel.h"
#include <stdio.h>

// Simple token type for demonstration
typedef struct {
  const char *text;
  ak_source_range_t range;
} token_t;

void print_error(ak_source_range_t range, const char *message) {
  char location_buf[256];
  ak_source_range_format(range, location_buf, sizeof(location_buf));

  // Extract the problematic code
  char *code = ak_source_extract(range);

  // Get the line for context
  const char *line = ak_source_get_line(range.start.file, range.start.line);
  size_t line_len = ak_source_get_line_len(range.start.file, range.start.line);

  fprintf(stderr, "\033[1;31mError\033[0m at %s: %s\n", location_buf, message);
  fprintf(stderr, "%4u | %.*s\n", range.start.line, (int)line_len, line);
  fprintf(stderr, "     | ");

  // Print arrows under the error
  for (uint32_t i = 1; i < range.start.column; i++) {
    fprintf(stderr, " ");
  }
  if (range.start.line == range.end.line) {
    uint32_t error_len = range.end.column - range.start.column;
    for (uint32_t i = 0; i < error_len; i++) {
      fprintf(stderr, "^");
    }
  } else {
    fprintf(stderr, "^");
  }
  fprintf(stderr, "\n");

  if (code) {
    AK24_FREE(code);
  }
}

void demo_basic_usage(void) {
  printf("\n=== Basic Source Location Usage ===\n");

  // Create a source file from memory
  const char *code = "int main() {\n  return 0;\n}\n";
  ak_source_file_t *file = ak_source_file_new("example.c", code, strlen(code));

  printf("Loaded file: %s (%zu bytes)\n", ak_source_file_name(file),
         ak_source_file_length(file));

  // Get location from byte offset
  ak_source_loc_t loc = ak_source_loc_from_offset(file, 15);
  printf("Offset 15 is at line %u, column %u\n", loc.line, loc.column);

  // Format location for display
  char buf[256];
  ak_source_loc_format(loc, buf, sizeof(buf));
  printf("Formatted: %s\n", buf);

  ak_source_file_release(file);
}

void demo_error_reporting(void) {
  printf("\n=== Error Reporting with Source Locations ===\n");

  const char *code = "int x = 10;\n"
                     "int y = z + 5;\n"
                     "return x + y;\n";

  ak_source_file_t *file = ak_source_file_new("test.c", code, strlen(code));

  // Simulate finding an undefined variable 'z' at position in line 2
  // "int y = z + 5;" - 'z' is at offset 20 (length 1)
  ak_source_loc_t start = ak_source_loc_from_offset(file, 20);
  ak_source_loc_t end = ak_source_loc_from_offset(file, 21);
  ak_source_range_t error_range = ak_source_range_new(start, end);

  print_error(error_range, "undefined variable 'z'");

  ak_source_file_release(file);
}

void demo_utf8_handling(void) {
  printf("\n=== UTF-8 Support ===\n");

  // Source with UTF-8 characters
  const char *code = "// Comment: Hello 世界\n"
                     "int x = 42;\n";

  ak_source_file_t *file = ak_source_file_new("utf8.c", code, strlen(code));

  // The '世' character starts at byte offset 18
  ak_source_loc_t loc = ak_source_loc_from_offset(file, 18);

  printf("Source: %s", code);
  printf("Byte offset 18 (start of '世') is at:\n");
  printf("  Line: %u, Column: %u\n", loc.line, loc.column);
  printf("  (Column counts UTF-8 code points, not bytes)\n");

  ak_source_file_release(file);
}

void demo_range_operations(void) {
  printf("\n=== Range Operations ===\n");

  const char *code = "function add(a, b) {\n"
                     "  return a + b;\n"
                     "}\n";

  ak_source_file_t *file = ak_source_file_new("func.js", code, strlen(code));

  // Extract function name "add"
  ak_source_loc_t start = ak_source_loc_from_offset(file, 9); // 'a' in "add"
  ak_source_loc_t end = ak_source_loc_from_offset(file, 12);  // after "add"
  ak_source_range_t func_name_range = ak_source_range_new(start, end);

  char *func_name = ak_source_extract(func_name_range);
  printf("Function name: '%s'\n", func_name);
  AK24_FREE(func_name);

  // Check if a location is within the function body
  ak_source_loc_t body_start = ak_source_loc_from_offset(file, 20); // '{'
  ak_source_loc_t body_end = ak_source_loc_from_offset(file, 41);   // '}'
  ak_source_range_t body_range = ak_source_range_new(body_start, body_end);

  ak_source_loc_t test_loc = ak_source_loc_from_offset(file, 30); // inside body
  printf("Location at offset 30 is %s function body\n",
         ak_source_range_contains(&body_range, test_loc) ? "inside"
                                                         : "outside");

  ak_source_file_release(file);
}

void demo_file_loading(void) {
  printf("\n=== Loading from File ===\n");

  // Create a temporary test file
  const char *test_content = "// Test file\nint main() {\n  return 0;\n}\n";
  FILE *fp = fopen("/tmp/test_sourceloc.c", "w");
  if (fp) {
    fwrite(test_content, 1, strlen(test_content), fp);
    fclose(fp);

    // Load from disk
    ak_source_file_t *file = ak_source_file_from_path("/tmp/test_sourceloc.c");
    if (file) {
      printf("Loaded file: %s\n", ak_source_file_name(file));
      printf("Size: %zu bytes\n", ak_source_file_length(file));

      // Get first line
      const char *line1 = ak_source_get_line(file, 1);
      size_t len1 = ak_source_get_line_len(file, 1);
      printf("Line 1: %.*s\n", (int)len1, line1);

      ak_source_file_release(file);
    } else {
      fprintf(stderr, "Failed to load file\n");
    }

    // Clean up
    remove("/tmp/test_sourceloc.c");
  }
}

void demo_reference_counting(void) {
  printf("\n=== Reference Counting ===\n");

  const char *code = "shared code";
  ak_source_file_t *file = ak_source_file_new("shared.c", code, strlen(code));

  printf("Created file (ref count = 1)\n");

  // Retain for another owner
  ak_source_file_t *file2 = ak_source_file_retain(file);
  printf("Retained file (ref count = 2)\n");

  // Both pointers are valid and point to same file
  printf("file1 name: %s\n", ak_source_file_name(file));
  printf("file2 name: %s\n", ak_source_file_name(file2));
  printf("Same file: %s\n", file == file2 ? "yes" : "no");

  // Release first reference
  ak_source_file_release(file);
  printf("Released file1 (ref count = 1)\n");

  // Second reference still valid
  printf("file2 still valid: %s\n", ak_source_file_name(file2));

  // Release second reference - now freed
  ak_source_file_release(file2);
  printf("Released file2 (ref count = 0, freed)\n");
}

void demo_statistics(void) {
  printf("\n=== Statistics ===\n");

  size_t count, bytes;
  ak_sourceloc_stats(&count, &bytes);
  printf("Initial: %zu files, %zu bytes\n", count, bytes);

  ak_source_file_t *file1 = ak_source_file_new("file1.c", "content1", 8);
  ak_sourceloc_stats(&count, &bytes);
  printf("After file1: %zu files, %zu bytes\n", count, bytes);

  ak_source_file_t *file2 = ak_source_file_new("file2.c", "longer content", 14);
  ak_sourceloc_stats(&count, &bytes);
  printf("After file2: %zu files, %zu bytes\n", count, bytes);

  ak_source_file_release(file1);
  ak_sourceloc_stats(&count, &bytes);
  printf("After release file1: %zu files, %zu bytes\n", count, bytes);

  ak_source_file_release(file2);
  ak_sourceloc_stats(&count, &bytes);
  printf("After release file2: %zu files, %zu bytes\n", count, bytes);
}

void demo_compiler_pipeline(void) {
  printf("\n=== Simulated Compiler Pipeline ===\n");

  const char *code = "function parse(input) {\n"
                     "  if (input === undefined) {\n"
                     "    throw 'Missing input';\n"
                     "  }\n"
                     "  return process(input);\n"
                     "}\n";

  ak_source_file_t *file = ak_source_file_new("parser.js", code, strlen(code));

  printf("Parsing: %s\n", ak_source_file_name(file));

  // Simulate tokenization - create tokens with locations
  token_t tokens[] = {
      {"function",
       {ak_source_loc_from_offset(file, 0),
        ak_source_loc_from_offset(file, 8)}},
      {"parse",
       {ak_source_loc_from_offset(file, 9),
        ak_source_loc_from_offset(file, 14)}},
      {"undefined",
       {ak_source_loc_from_offset(file, 45),
        ak_source_loc_from_offset(file, 54)}},
  };

  printf("\nTokens:\n");
  for (size_t i = 0; i < 3; i++) {
    char buf[256];
    ak_source_range_format(tokens[i].range, buf, sizeof(buf));
    printf("  '%s' at %s\n", tokens[i].text, buf);
  }

  // Simulate finding an error during semantic analysis
  printf("\nSemantic Analysis:\n");
  ak_source_loc_t error_start = ak_source_loc_from_offset(file, 71);
  ak_source_loc_t error_end = ak_source_loc_from_offset(file, 78);
  ak_source_range_t error_range = ak_source_range_new(error_start, error_end);

  print_error(error_range, "undefined function 'process'");

  ak_source_file_release(file);
}

int main(void) {
  ak_kernel_init();

  printf("╔════════════════════════════════════════════╗\n");
  printf("║   Source Location Tracking Demo           ║\n");
  printf("╚════════════════════════════════════════════╝\n");

  demo_basic_usage();
  demo_error_reporting();
  demo_utf8_handling();
  demo_range_operations();
  demo_file_loading();
  demo_reference_counting();
  demo_statistics();
  demo_compiler_pipeline();

  printf("\n╔════════════════════════════════════════════╗\n");
  printf("║   Demo Complete!                           ║\n");
  printf("╚════════════════════════════════════════════╝\n");

  ak_kernel_deinit();
  return 0;
}
