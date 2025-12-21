#include "kernel/application.h"
#include "kernel/log/include/log.h"
#include "systems/viewer/include/atom_view.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

/**
 * @brief Edge connection between two atoms
 */
typedef struct {
  size_t atom_a; /**< Index of first atom */
  size_t atom_b; /**< Index of second atom */
} atom_edge_t;

/**
 * @brief Draw a line between two points using Bresenham's algorithm
 */
void draw_line(ak_atom_view_t *view, int x0, int y0, int x1, int y1,
               const char *color) {
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1;
  int sy = y0 < y1 ? 1 : -1;
  int err = dx - dy;

  while (1) {
    // Only draw if within bounds and pixel is empty
    if (x0 >= 0 && x0 < (int)view->width && y0 >= 0 && y0 < (int)view->height) {
      if (!view->pixels[y0][x0]) {
        // Mark as edge (we'll use a special indicator)
        view->depths[y0][x0] = -1.0; // Negative depth indicates edge
      }
    }

    if (x0 == x1 && y0 == y1)
      break;

    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
}

APP_ON_SHUTDOWN(on_shutdown) {
  time_t uptime = time(NULL) - ctx->shutdown_info->start_time;
  AK24_LOG_INFO("Shutting down after %ld seconds", uptime);

#if AK24_BUILD_DEBUG_MEMORY
  AK24_LOG_INFO("=== Memory Statistics ===");
  AK24_LOG_INFO("Total allocations: %zu",
                ctx->shutdown_info->memory_stats.total_allocations);
  AK24_LOG_INFO("Total frees: %zu",
                ctx->shutdown_info->memory_stats.total_frees);
  AK24_LOG_INFO("Total reallocs: %zu",
                ctx->shutdown_info->memory_stats.total_reallocs);
  AK24_LOG_INFO("Bytes allocated: %zu",
                ctx->shutdown_info->memory_stats.bytes_allocated);
  AK24_LOG_INFO("Current bytes: %zu",
                ctx->shutdown_info->memory_stats.current_bytes);
  AK24_LOG_INFO("Peak bytes: %zu", ctx->shutdown_info->memory_stats.peak_bytes);
  AK24_LOG_INFO("=========================");
#endif
}

/**
 * @brief Atom Viewer Demo - N-Dimensional Atom Visualization
 *
 * Demonstrates the atom viewer system with a 4D hypercube (tesseract).
 * The viewer projects N-dimensional atoms onto a 2D view with rotation support.
 */

APP_MAIN(atom_viewer_demo) {
  AK24_LOG_INFO("=== AK24 Atom Viewer Demo (N-Dimensional) ===");

  // Parse command-line arguments for dimensionality and size
  size_t dimensionality = 4;      // Default: 4D hypercube
  int size = 2;                   // Default: 2x2x2x2 tesseract
  double duration_seconds = 30.0; // Default: 30 seconds
  double target_fps = 30.0;       // Default: 30 FPS

  // Arguments after the program name (ctx->args has log flags removed)
  size_t arg_count = list_count(&ctx->args);

  if (arg_count > 1) {
    // First arg is program name, skip it
    char **arg = list_get(&ctx->args, 1);
    if (arg && *arg) {
      dimensionality = (size_t)atoi(*arg);
      if (dimensionality < 2) {
        AK24_LOG_WARN("Dimensionality must be >= 2, using default (4)");
        dimensionality = 4;
      }
    }
  }

  if (arg_count > 2) {
    char **arg = list_get(&ctx->args, 2);
    if (arg && *arg) {
      size = atoi(*arg);
      if (size < 1) {
        AK24_LOG_WARN("Size must be >= 1, using default (2)");
        size = 2;
      }
    }
  }

  if (arg_count > 3) {
    char **arg = list_get(&ctx->args, 3);
    if (arg && *arg) {
      duration_seconds = atof(*arg);
      if (duration_seconds <= 0.0) {
        AK24_LOG_WARN("Duration must be > 0, using default (30)");
        duration_seconds = 30.0;
      }
    }
  }

  if (arg_count > 4) {
    char **arg = list_get(&ctx->args, 4);
    if (arg && *arg) {
      target_fps = atof(*arg);
      if (target_fps <= 0.0 || target_fps > 120.0) {
        AK24_LOG_WARN("FPS must be between 0 and 120, using default (30)");
        target_fps = 30.0;
      }
    }
  }

  AK24_LOG_INFO("Creating %zuD hypercube with size %d...", dimensionality,
                size);

  // Calculate total number of atoms
  size_t total_atoms = 1;
  for (size_t d = 0; d < dimensionality; d++) {
    total_atoms *= size;
  }

  AK24_LOG_INFO("Total atoms: %zu", total_atoms);

  // Step 1 - Create atoms with arena allocator
  ak_arena_t *atom_arena = ak_arena_new(AK_ARENA_DEFAULT_BLOCK_SIZE);
  if (!atom_arena) {
    AK24_LOG_ERROR("Failed to create atom arena");
    return 1;
  }

  ak_atom_positioned_t *atoms =
      ak_arena_alloc(atom_arena, sizeof(ak_atom_positioned_t) * total_atoms);
  if (!atoms) {
    AK24_LOG_ERROR("Failed to allocate positioned atoms");
    ak_arena_free(atom_arena);
    return 1;
  }

  // Position atoms in N-D grid
  double spacing = 2.0;
  for (size_t i = 0; i < total_atoms; i++) {
    atoms[i].atom =
        ak_atom_new(AK24_ATOM_I32, (ak_atom_value_u){.i32 = (int)i}, 4);
    atoms[i].dimensionality = dimensionality;
    atoms[i].coords =
        ak_arena_alloc(atom_arena, sizeof(double) * dimensionality);

    if (!atoms[i].coords) {
      AK24_LOG_ERROR("Failed to allocate coordinates for atom %zu", i);
      ak_arena_free(atom_arena);
      return 1;
    }

    // Calculate N-D grid position
    size_t pos = i;
    for (size_t d = 0; d < dimensionality; d++) {
      size_t coord = pos % size;
      pos /= size;
      // Center the grid around origin
      atoms[i].coords[d] = ((double)coord - (double)(size - 1) / 2.0) * spacing;
    }
  }

  AK24_LOG_INFO("Atoms positioned in %zuD grid", dimensionality);

  // Step 1.5 - Create edges between neighboring atoms in the hypercube
  // Each atom connects to neighbors that differ by 1 in exactly one dimension
  size_t max_edges = total_atoms * dimensionality; // Upper bound
  atom_edge_t *edges =
      ak_arena_alloc(atom_arena, sizeof(atom_edge_t) * max_edges);
  size_t edge_count = 0;

  if (edges) {
    for (size_t i = 0; i < total_atoms; i++) {
      // For each atom, find neighbors in each dimension
      size_t pos = i;
      size_t coords[16]; // Max 16 dimensions should be enough

      // Get grid coordinates
      for (size_t d = 0; d < dimensionality; d++) {
        coords[d] = pos % size;
        pos /= size;
      }

      // Check each dimension for a neighbor
      for (size_t d = 0; d < dimensionality; d++) {
        if (coords[d] < size - 1) {
          // Calculate neighbor index by incrementing this dimension
          size_t neighbor_idx = 0;
          size_t multiplier = 1;
          for (size_t dd = 0; dd < dimensionality; dd++) {
            size_t c = (dd == d) ? (coords[dd] + 1) : coords[dd];
            neighbor_idx += c * multiplier;
            multiplier *= size;
          }

          if (neighbor_idx < total_atoms && edge_count < max_edges) {
            edges[edge_count].atom_a = i;
            edges[edge_count].atom_b = neighbor_idx;
            edge_count++;
          }
        }
      }
    }
    AK24_LOG_INFO("Created %zu edges connecting the hypercube", edge_count);
  }

  // Step 2 - Create view configuration
  ak_atom_view_config_t *config = ak_atom_view_config_new(dimensionality);
  if (!config) {
    AK24_LOG_ERROR("Failed to create view configuration");
    ak_arena_free(atom_arena);
    return 1;
  }

  ak_atom_view_set_axes(config, 0, 1);  // Project X-Y plane
  ak_atom_view_set_scale(config, 10.0); // 10 pixels per unit

  AK24_LOG_INFO("View configured: projecting dimensions %zu and %zu",
                config->axis_x, config->axis_y);
  AK24_LOG_INFO("Target FPS: %.1f, Duration: %.1f seconds", target_fps,
                duration_seconds);

  // Step 3 - Animation loop with FPS control
  ak_arena_t *frame_arena = ak_arena_new(256 * 1024);
  if (!frame_arena) {
    AK24_LOG_ERROR("Failed to create frame arena");
    ak_atom_view_config_free(config);
    ak_arena_free(atom_arena);
    return 1;
  }

  // FPS timing variables
  struct timeval start_time, current_time, frame_start, frame_end;
  gettimeofday(&start_time, NULL);

  double frame_duration = 1.0 / target_fps;
  long frame_duration_us = (long)(frame_duration * 1000000.0);

  double rotation_xz = 0.0;
  double rotation_yw = 0.0;
  double rotation_speed = 30.0; // degrees per second
  double view_rotation_speed =
      10.0; // Slow cycle through viewing angles (seconds per full cycle)

  int frame_count = 0;
  double elapsed = 0.0;

  AK24_LOG_INFO("Starting animation (%.1fs at %.1f FPS)...", duration_seconds,
                target_fps);

  while (elapsed < duration_seconds) {
    gettimeofday(&frame_start, NULL);

    // Calculate elapsed time
    elapsed = (frame_start.tv_sec - start_time.tv_sec) +
              (frame_start.tv_usec - start_time.tv_usec) / 1000000.0;

    // Slowly cycle through different viewing angles
    // This rotates which dimensions we're looking at
    if (dimensionality >= 3) {
      double cycle_progress = fmod(elapsed / view_rotation_speed, 1.0);

      // For 3D: cycle between XY, XZ, YZ views
      // For 4D+: cycle through more dimension pairs
      if (dimensionality == 3) {
        if (cycle_progress < 0.33) {
          ak_atom_view_set_axes(config, 0, 1); // XY view
        } else if (cycle_progress < 0.66) {
          ak_atom_view_set_axes(config, 0, 2); // XZ view
        } else {
          ak_atom_view_set_axes(config, 1, 2); // YZ view
        }
      } else {
        // For 4D+, cycle through multiple axis pairs
        int total_pairs = (int)(dimensionality * (dimensionality - 1) / 2);
        int current_pair = (int)(cycle_progress * total_pairs) % total_pairs;

        // Map pair index to axis combinations
        int pair_count = 0;
        for (size_t i = 0; i < dimensionality && pair_count <= current_pair;
             i++) {
          for (size_t j = i + 1;
               j < dimensionality && pair_count <= current_pair; j++) {
            if (pair_count == current_pair) {
              ak_atom_view_set_axes(config, i, j);
              break;
            }
            pair_count++;
          }
        }
      }
    }

    // Update rotations based on elapsed time for smooth animation
    rotation_xz = rotation_speed * elapsed;
    rotation_yw = rotation_speed * elapsed * 0.75;

    if (dimensionality >= 3) {
      ak_atom_view_rotate(config, 0, 2, rotation_xz); // Rotate in X-Z plane
    }
    if (dimensionality >= 4) {
      ak_atom_view_rotate(config, 1, 3, rotation_yw); // Rotate in Y-W plane
    }

    // Reset frame arena and render
    ak_arena_reset(frame_arena);
    ak_atom_view_t *view = ak_atom_project_view_arena(
        frame_arena, atoms, total_atoms, config, 80, 40);

    if (!view) {
      AK24_LOG_ERROR("Failed to project view for frame %d", frame_count);
      break;
    }

    // Project and draw edges
    if (edges && edge_count > 0) {
      for (size_t e = 0; e < edge_count; e++) {
        size_t idx_a = edges[e].atom_a;
        size_t idx_b = edges[e].atom_b;

        if (idx_a >= total_atoms || idx_b >= total_atoms)
          continue;

        // Get rotated coordinates for both endpoints
        double coords_a[16], coords_b[16];
        memcpy(coords_a, atoms[idx_a].coords, sizeof(double) * dimensionality);
        memcpy(coords_b, atoms[idx_b].coords, sizeof(double) * dimensionality);

        // Apply rotations
        for (size_t r = 0; r < config->num_rotation_pairs; r++) {
          double angle_rad = config->rotations[r].angle * M_PI / 180.0;
          double cos_a = cos(angle_rad);
          double sin_a = sin(angle_rad);
          size_t i = config->rotations[r].axis_i;
          size_t j = config->rotations[r].axis_j;

          double old_ai = coords_a[i];
          double old_aj = coords_a[j];
          coords_a[i] = old_ai * cos_a - old_aj * sin_a;
          coords_a[j] = old_ai * sin_a + old_aj * cos_a;

          double old_bi = coords_b[i];
          double old_bj = coords_b[j];
          coords_b[i] = old_bi * cos_a - old_bj * sin_a;
          coords_b[j] = old_bi * sin_a + old_bj * cos_a;
        }

        // Project to screen coordinates
        double half_width = (double)view->width / 2.0;
        double half_height = (double)view->height / 2.0;

        int x0 = (int)(coords_a[config->axis_x] * config->scale + half_width);
        int y0 = (int)(coords_a[config->axis_y] * config->scale + half_height);
        int x1 = (int)(coords_b[config->axis_x] * config->scale + half_width);
        int y1 = (int)(coords_b[config->axis_y] * config->scale + half_height);

        // Draw line
        draw_line(view, x0, y0, x1, y1, "\033[2;90m");
      }
    }

    // Clear screen and display
    printf("\033[2J\033[H"); // ANSI escape codes

    // Header with title
    printf("\033[1;"
           "36m╔═══════════════════════════════════════════════════════════════"
           "═════════╗\033[0m\n");
    printf("\033[1;36m║\033[0m          \033[1;33m%zuD HYPERCUBE VIEWER\033[0m "
           "- Rotating Through Space & Time        \033[1;36m║\033[0m\n",
           dimensionality);
    printf("\033[1;"
           "36m╚═══════════════════════════════════════════════════════════════"
           "═════════╝\033[0m\n\n");

    // What you're seeing
    printf("\033[1;37m🔍 What you're looking at:\033[0m\n");
    printf("   You're seeing a \033[1;32m%d×%d", size, size);
    for (size_t d = 2; d < dimensionality; d++)
      printf("×%d", size);
    printf("\033[0m grid of \033[1;33m%zu\033[0m points\n", total_atoms);
    printf("   projected from \033[1;35m%zuD space\033[0m down to your "
           "\033[1;34m2D screen\033[0m\n\n",
           dimensionality);

    // Current view info
    printf("\033[1;37m📐 Current projection:\033[0m\n");
    printf("   Viewing dimension \033[1;32m%zu\033[0m (horizontal) × dimension "
           "\033[1;32m%zu\033[0m (vertical)\n",
           config->axis_x, config->axis_y);
    if (config->num_rotation_pairs > 0) {
      printf("   \033[1;33m⟲ Rotating:\033[0m ");
      for (size_t r = 0; r < config->num_rotation_pairs; r++) {
        printf("plane(%zu,%zu)=\033[1;36m%.0f°\033[0m ",
               config->rotations[r].axis_i, config->rotations[r].axis_j,
               fmod(config->rotations[r].angle, 360.0));
        if (r < config->num_rotation_pairs - 1)
          printf("• ");
      }
      printf("\n");
    }
    printf("\n");

    // Progress bar
    double progress = elapsed / duration_seconds;
    int bar_width = 50;
    int filled = (int)(progress * bar_width);
    printf("\033[1;37m⏱  Progress:\033[0m [");
    for (int i = 0; i < bar_width; i++) {
      if (i < filled)
        printf("\033[1;32m█\033[0m");
      else
        printf("\033[2;90m░\033[0m");
    }
    printf("] %.1f/%.1fs\n", elapsed, duration_seconds);
    printf("   \033[1;37m🎬 Frame:\033[0m %d  \033[1;37m⚡ FPS:\033[0m %.1f  "
           "\033[1;37m🎯 Target:\033[0m %.1f\n\n",
           frame_count, frame_count / (elapsed > 0 ? elapsed : 1), target_fps);

    // Legend
    printf("\033[1;37m🎨 Legend:\033[0m  ");
    const char *colors[] = {"\033[1;31m●\033[0m", "\033[1;32m●\033[0m",
                            "\033[1;33m●\033[0m", "\033[1;34m●\033[0m",
                            "\033[1;35m●\033[0m", "\033[1;36m●\033[0m",
                            "\033[1;91m●\033[0m", "\033[1;92m●\033[0m"};
    for (size_t i = 0; i < (total_atoms < 8 ? total_atoms : 8); i++) {
      printf("%s=%zu ", colors[i], i);
    }
    if (total_atoms > 8)
      printf("... (showing %zu points)", total_atoms);
    printf("\n\n");

    printf("───────────────────────────────────────────────────────────────────"
           "──────────\n");

    // Render the view with colorful dots
    for (size_t y = 0; y < view->height; y++) {
      for (size_t x = 0; x < view->width; x++) {
        if (view->pixels[y][x]) {
          ak_atom_value_u val = ak_atom_get_value(view->pixels[y][x]);
          int idx = val.i32 % 16;

          // Use different colors and characters based on depth
          double depth = view->depths[y][x];
          const char *color_codes[] = {
              "\033[1;31m", "\033[1;32m", "\033[1;33m", "\033[1;34m",
              "\033[1;35m", "\033[1;36m", "\033[1;91m", "\033[1;92m",
              "\033[1;93m", "\033[1;94m", "\033[1;95m", "\033[1;96m",
              "\033[0;31m", "\033[0;32m", "\033[0;33m", "\033[0;34m"};

          // Closer objects are brighter/bigger
          const char *symbol = (depth < 2.0) ? "●" : (depth < 4.0) ? "●" : "·";
          printf("%s%s\033[0m", color_codes[idx], symbol);
        } else if (view->depths[y][x] == -1.0) {
          // It's an edge
          printf("\033[2;90m·\033[0m");
        } else {
          printf(" ");
        }
      }
      printf("\n");
    }

    printf("───────────────────────────────────────────────────────────────────"
           "──────────\n");
    printf("\033[2;90m💡 Tip: Each colored dot is a point in %zuD space, "
           "depth-sorted for realism\033[0m\n",
           dimensionality);

    frame_count++;

    // Calculate frame time and sleep to maintain target FPS
    gettimeofday(&frame_end, NULL);
    long frame_time_us = (frame_end.tv_sec - frame_start.tv_sec) * 1000000 +
                         (frame_end.tv_usec - frame_start.tv_usec);
    long sleep_time_us = frame_duration_us - frame_time_us;

    if (sleep_time_us > 0) {
      usleep(sleep_time_us);
    }
  }

  // Calculate final stats
  gettimeofday(&current_time, NULL);
  double total_time = (current_time.tv_sec - start_time.tv_sec) +
                      (current_time.tv_usec - start_time.tv_usec) / 1000000.0;
  double actual_fps = frame_count / total_time;

  AK24_LOG_INFO("Animation complete!");
  AK24_LOG_INFO("Frames rendered: %d", frame_count);
  AK24_LOG_INFO("Total time: %.2f seconds", total_time);
  AK24_LOG_INFO("Average FPS: %.2f (target was %.1f)", actual_fps, target_fps);

  // Step 4 - Cleanup
  ak_arena_free(frame_arena);
  ak_atom_view_config_free(config);
  ak_arena_free(atom_arena);

  AK24_LOG_INFO("Demo complete. Try different parameters:");
  AK24_LOG_INFO(
      "  ./atom-viewer-demo [dims] [size] [duration_sec] [target_fps]");
  AK24_LOG_INFO("  Example: ./atom-viewer-demo 4 2 30 60");
  AK24_LOG_INFO("  Example: ./atom-viewer-demo 3 3 10 30");

  return 0;
}

AK24_APPLICATION(atom_viewer_demo, on_shutdown)
