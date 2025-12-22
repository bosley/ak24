/**
 * @file main.c
 * @brief SDL2 example demonstrating AK24 cell grid library
 *
 * This example demonstrates how to use the AK24 cell grid library
 * for creating an interactive cell-based application.
 *
 * Usage:
 *   ./sdl-example
 */

#include "cellgrid.h"
#include "kernel/application.h"
#include <stdio.h>

// ============================================================================
// Application State
// ============================================================================

static volatile int keep_running = 1;
static ak_cellgrid_t *grid = NULL;

// ============================================================================
// Cell Grid Callbacks
// ============================================================================

/**
 * @brief Handle cell clicks
 *
 * Toggle cell color based on click state. Each region's clicks
 * are processed by a dedicated thread.
 */
static void on_cell_click(ak_cell_ctx_t *cell, void *user_data) {
  (void)user_data;

  if (cell->clicked) {
    /* Reset to default color */
    ak_cell_fill(cell, 60, 60, 60, 255);
    ak_cell_set_clicked(cell, false);
  } else {
    /* Set highlight color based on region */
    int region = cell->region_id;
    ak_cell_fill(cell, (uint8_t)(100 + (region * 37) % 155),
                 (uint8_t)(100 + (region * 73) % 155),
                 (uint8_t)(100 + (region * 111) % 155), 255);
    ak_cell_set_clicked(cell, true);
  }

  AK24_LOG_INFO("Cell [%d,%d] clicked in region %d", cell->x, cell->y,
                cell->region_id);
}

/**
 * @brief Grid initialization callback
 */
static void on_grid_init(ak_cellgrid_t *g, void *user_data) {
  (void)user_data;

  int cells_x, cells_y, regions_x, regions_y;
  ak_cellgrid_get_dimensions(g, &cells_x, &cells_y, &regions_x, &regions_y);

  printf("Grid initialized:\n");
  printf("  Cells:   %dx%d (%d total)\n", cells_x, cells_y,
         ak_cellgrid_cell_count(g));
  printf("  Regions: %dx%d (%d total)\n", regions_x, regions_y,
         ak_cellgrid_region_count(g));
}

// ============================================================================
// Signal Handlers
// ============================================================================

APP_ON_SIGNAL(handle_sigint, SIGINT) {
  (void)captured;
  (void)args;
  printf("\n[SIGINT] Caught Ctrl+C, shutting down...\n");
  keep_running = 0;
  if (grid) {
    ak_cellgrid_stop(grid);
  }
}

APP_ON_SIGNAL(handle_sigterm, SIGTERM) {
  (void)captured;
  (void)args;
  printf("\n[SIGTERM] Received termination signal\n");
  keep_running = 0;
  if (grid) {
    ak_cellgrid_stop(grid);
  }
}

// ============================================================================
// Application Lifecycle
// ============================================================================

APP_ON_SHUTDOWN(on_shutdown) {
  (void)ctx;

  if (grid) {
    ak_cellgrid_free(grid);
    grid = NULL;
  }

  printf("Cleanup complete\n");
}

APP_MAIN(app_main) {
  (void)ctx;

  ak_log_set_level(AK24_LOG_LEVEL_INFO);
  ak_log_set_color(true);
  ak_log_set_path_format(AK24_LOG_PATH_ABBREV);

  printf("=== AK24 Cell Grid Example ===\n\n");

  AK24_REGISTER_SIGNAL_HANDLER(handle_sigint);
  AK24_REGISTER_SIGNAL_HANDLER(handle_sigterm);

  /* Configure the cell grid */
  ak_cellgrid_opts_t opts = ak_cellgrid_opts_default();
  opts.screen_width = 800;
  opts.screen_height = 600;
  opts.cell_size = 4;
  opts.region_size = 16;
  opts.cell_wrap = true;
  opts.title = "AK24 Cell Grid - Click cells!";
  opts.draw_region_borders = false;
  opts.bg_color = (ak_color_t){0, 0, 0, 0};
  opts.cell_color = (ak_color_t){0, 0, 0, 255};

  /* Set callbacks */
  opts.on_click = on_cell_click;
  opts.on_init = on_grid_init;
  opts.user_data = NULL;

  /* Print configuration */
  printf("Configuration:\n");
  printf("  Screen:  %dx%d pixels\n", opts.screen_width, opts.screen_height);
  printf("  Cell:    %dx%d pixels\n", opts.cell_size, opts.cell_size);
  printf("  Region:  %dx%d cells\n", opts.region_size, opts.region_size);
  printf("  Wrap:    %s\n\n", opts.cell_wrap ? "enabled" : "disabled");

  /* Create and run the grid */
  grid = ak_cellgrid_new(&opts);
  if (!grid) {
    fprintf(stderr, "Failed to create cell grid\n");
    return 1;
  }

  AK24_LOG_INFO("Cell grid created successfully");

  /* Run the event loop (blocking) */
  int result = ak_cellgrid_run(grid);

  printf("\nExiting cell grid example\n");
  return result;
}

AK24_APPLICATION("sdl-example", app_main, on_shutdown)
