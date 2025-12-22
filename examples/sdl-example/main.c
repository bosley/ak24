/**
 * @file main.c
 * @brief SDL2 example demonstrating AK24 cell grid library with entity system
 *
 * This example demonstrates how to use the AK24 cell grid library
 * combined with the entity system for creating an interactive
 * cell-based application with spatial entity relationships.
 *
 * Usage:
 *   ./sdl-example
 */

#include "cellgrid.h"
#include "entity.h"
#include "kernel/application.h"
#include <stdio.h>

// ============================================================================
// Entity Data Structure
// ============================================================================

/**
 * @brief Entity data allocated on click
 *
 * This structure holds per-entity state that is allocated when
 * an entity is first clicked. It provides the foundation for
 * effects propagation and rendering state.
 */
typedef struct {
  bool active;          /**< Whether this entity is currently active */
  bool needs_redraw;    /**< Dirty flag - entity needs to sync to cell */
  uint32_t click_count; /**< Number of times this entity was clicked */
  ak_color_t color;     /**< Current entity color for rendering */
} entity_data_t;

// ============================================================================
// Application State
// ============================================================================

static volatile int keep_running = 1;
static ak_cellgrid_t *grid = NULL;
static entity_grid_t *entities = NULL;

/* Grid dimensions (set during initialization) */
static int grid_cells_x = 0;
static int grid_cells_y = 0;

// ============================================================================
// Entity Helper Functions
// ============================================================================

/**
 * @brief Allocate and initialize entity data for first click
 *
 * @param color Initial color for the entity
 * @return Newly allocated entity data, or NULL on failure
 */
static entity_data_t *entity_data_new(ak_color_t color) {
  entity_data_t *data = (entity_data_t *)AK24_ALLOC(sizeof(entity_data_t));
  if (!data) {
    AK24_LOG_ERROR("Failed to allocate entity data");
    return NULL;
  }

  data->active = true;
  data->needs_redraw = true;
  data->click_count = 1;
  data->color = color;

  return data;
}

/**
 * @brief Free entity data
 *
 * @param data Entity data to free (NULL is safe)
 */
static void entity_data_free(entity_data_t *data) {
  if (data) {
    AK24_FREE(data);
  }
}

/**
 * @brief Get entity at cell coordinates
 *
 * @param x Cell x coordinate
 * @param y Cell y coordinate
 * @return Entity at position, or NULL if out of bounds
 */
static entity_t *get_entity_at(int x, int y) {
  if (!entities) {
    return NULL;
  }
  return entity_grid_get(entities, x, y);
}

// ============================================================================
// Ring Effect Lambda
// ============================================================================

/**
 * @brief Context for ring effect propagation
 */
typedef struct {
  ak_color_t ring_color; /**< Color for ring entities */
} ring_effect_ctx_t;

/**
 * @brief Lambda callback for ring effect - updates entity data only
 *
 * The render sync thread will pick up the color change and update the cell.
 *
 * @param captured_ctx ring_effect_ctx_t* with effect parameters
 * @param invoke_args entity_iter_ctx_t* from iteration
 */
static void ring_effect_fn(void *captured_ctx, void *invoke_args) {
  ring_effect_ctx_t *ctx = (ring_effect_ctx_t *)captured_ctx;
  entity_iter_ctx_t *iter = (entity_iter_ctx_t *)invoke_args;

  if (!iter->entity) {
    return;
  }

  entity_t *entity = iter->entity;
  entity_data_t *data = (entity_data_t *)entity->data;

  /* Allocate entity data if not present */
  if (!data) {
    data = entity_data_new(ctx->ring_color);
    if (data) {
      entity->data = data;
    }
  } else {
    /* Update existing data - mark for redraw */
    data->active = true;
    data->needs_redraw = true;
    data->click_count++;
    data->color = ctx->ring_color;
  }

  AK24_LOG_DEBUG("Ring effect: Entity %zu at [%d,%d] (distance %d, step %d)",
                 entity->unique_id, entity->x, entity->y, iter->distance,
                 iter->step);
}

// ============================================================================
// Cell Grid Callbacks
// ============================================================================

/**
 * @brief Handle cell clicks
 *
 * Associates cell clicks with entities. On first click, allocates
 * entity data. Subsequent clicks toggle the entity state.
 * Each region's clicks are processed by a dedicated thread.
 */
static void on_cell_click(ak_cell_ctx_t *cell, void *user_data) {
  (void)user_data;

  /* Get the associated entity */
  entity_t *entity = get_entity_at(cell->x, cell->y);
  if (!entity) {
    AK24_LOG_WARN("No entity found at [%d,%d]", cell->x, cell->y);
    return;
  }

  entity_data_t *data = (entity_data_t *)entity->data;
  int region = cell->region_id;

  /* Toggle based on current entity state (not cell state) */
  bool currently_active = data && data->active;

  if (currently_active) {
    /* Deactivate */
    if (data) {
      data->active = false;
      data->needs_redraw = true;
      data->color = (ak_color_t){60, 60, 60, 255};
    }
  } else {
    /* Activate: Set highlight color based on region */
    ak_color_t new_color = {(uint8_t)(100 + (region * 37) % 155),
                            (uint8_t)(100 + (region * 73) % 155),
                            (uint8_t)(100 + (region * 111) % 155), 255};

    if (!data) {
      /* First click - allocate entity data */
      data = entity_data_new(new_color);
      if (data) {
        entity->data = data;
        AK24_LOG_DEBUG("Entity %zu at [%d,%d] data allocated",
                       entity->unique_id, cell->x, cell->y);
      }
    } else {
      /* Subsequent click - update existing data */
      data->active = true;
      data->needs_redraw = true;
      data->click_count++;
      data->color = new_color;
    }

    /* Trigger ring effect at distance 3 (no fill) */
    ring_effect_ctx_t ring_ctx = {
        .ring_color = {(uint8_t)(50 + (region * 53) % 200),
                       (uint8_t)(50 + (region * 97) % 200),
                       (uint8_t)(50 + (region * 131) % 200), 255}};

    ak_lambda_t *ring_lambda = ak_lambda_new(ring_effect_fn, &ring_ctx, NULL);
    if (ring_lambda) {
      entity_iterate_scalar_clockwise(
          entities, cell->x, cell->y, 3, /* distance */
          false,                         /* is_filled = no fill, just ring */
          ring_lambda);
      ak_lambda_free(ring_lambda);
    }
  }

  AK24_LOG_INFO("Entity %zu at [%d,%d] %s (clicks: %u)", entity->unique_id,
                cell->x, cell->y,
                (data && data->active) ? "activated" : "deactivated",
                data ? data->click_count : 0);
}

/**
 * @brief Grid initialization callback
 *
 * Creates the entity grid to match cell grid dimensions.
 */
static void on_grid_init(ak_cellgrid_t *g, void *user_data) {
  (void)user_data;

  int regions_x, regions_y;
  ak_cellgrid_get_dimensions(g, &grid_cells_x, &grid_cells_y, &regions_x,
                             &regions_y);

  printf("Grid initialized:\n");
  printf("  Cells:   %dx%d (%d total)\n", grid_cells_x, grid_cells_y,
         ak_cellgrid_cell_count(g));
  printf("  Regions: %dx%d (%d total)\n", regions_x, regions_y,
         ak_cellgrid_region_count(g));

  /* Create entity grid matching cell grid dimensions */
  entities = entity_grid_new(grid_cells_x, grid_cells_y, true /* wrap */);
  if (!entities) {
    AK24_LOG_ERROR("Failed to create entity grid");
    return;
  }

  printf("  Entities: %dx%d grid created\n", grid_cells_x, grid_cells_y);
  AK24_LOG_INFO("Entity grid created with %d entities",
                grid_cells_x * grid_cells_y);
}

// ============================================================================
// Render Sync - Entity to Cell Color Sync
// ============================================================================

/**
 * @brief Cell modifier that syncs entity color to cell
 */
static void sync_cell_color(ak_cell_ctx_t *cell, void *user_data) {
  entity_data_t *data = (entity_data_t *)user_data;
  if (!data)
    return;

  ak_cell_fill_color(cell, data->color);
  ak_cell_set_clicked(cell, data->active);
  data->needs_redraw = false;
}

/**
 * @brief Frame update callback - syncs dirty entities to cells
 *
 * Called every frame. Iterates all entities and syncs those with
 * needs_redraw flag set to their corresponding cells.
 */
static void on_frame_update(ak_cellgrid_t *g, void *user_data,
                            uint32_t delta_ms) {
  (void)user_data;
  (void)delta_ms;

  if (!entities || !g) {
    return;
  }

  /* Iterate all entities and sync dirty ones to cells */
  for (int y = 0; y < grid_cells_y; y++) {
    for (int x = 0; x < grid_cells_x; x++) {
      entity_t *entity = entity_grid_get(entities, x, y);
      if (!entity)
        continue;

      entity_data_t *data = (entity_data_t *)entity->data;
      if (data && data->needs_redraw) {
        ak_cellgrid_modify_cell(g, x, y, sync_cell_color, data);
      }
    }
  }
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

  /* Free all entity data before freeing the entity grid */
  if (entities) {
    for (int y = 0; y < grid_cells_y; y++) {
      for (int x = 0; x < grid_cells_x; x++) {
        entity_t *entity = entity_grid_get(entities, x, y);
        if (entity && entity->data) {
          entity_data_free((entity_data_t *)entity->data);
          entity->data = NULL;
        }
      }
    }
    entity_grid_free(entities);
    entities = NULL;
  }

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
  opts.on_frame = on_frame_update;
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
