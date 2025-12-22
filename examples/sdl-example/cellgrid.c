/**
 * @file cellgrid.c
 * @brief SDL2 Cell Grid implementation
 *
 * Implements the cell grid abstraction library with SDL2 backend
 * and region-based thread pooling for concurrent click handling.
 */

#include "cellgrid.h"
#include "kernel/kernel.h"
#include "kernel/lambda/include/lambda.h"
#include "kernel/threads/include/threads.h"
#include <SDL2/SDL.h>
#include <string.h>

// ============================================================================
// Internal Structures
// ============================================================================

/**
 * @brief Internal cell data (mirrors ak_cell_ctx_t but with SDL types)
 */
typedef struct {
  int x;
  int y;
  int region_id;
  bool clicked;
  ak_color_t color;
  SDL_Rect rect;
  ak_pixel_matrix_t pixels;
} cell_internal_t;

/**
 * @brief Internal grid structure
 */
struct ak_cellgrid_t {
  /* Configuration (copied from opts) */
  int screen_width;
  int screen_height;
  int cell_size;
  bool cell_wrap;
  int region_size;
  ak_color_t bg_color;
  ak_color_t cell_color;
  ak_color_t grid_color;
  bool draw_region_borders;

  /* Callbacks */
  ak_cell_click_fn on_click;
  ak_frame_update_fn on_frame;
  ak_grid_init_fn on_init;
  void *user_data;

  /* Computed dimensions */
  int cells_x;
  int cells_y;
  int total_cells;
  int regions_x;
  int regions_y;
  int total_regions;

  /* SDL resources */
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_mutex *render_mutex;

  /* Cell grid */
  cell_internal_t *cells;

  /* Region thread pools (one per region) */
  ak_thread_pool_t **region_pools;

  /* State */
  volatile int running;
};

/**
 * @brief Click task context (passed to region thread)
 */
typedef struct {
  ak_cellgrid_t *grid;
  int cell_x;
  int cell_y;
  int mouse_x;
  int mouse_y;
} click_task_t;

// ============================================================================
// Helper Functions
// ============================================================================

static inline int cell_index(const ak_cellgrid_t *grid, int x, int y) {
  return y * grid->cells_x + x;
}

static inline int get_region_id(const ak_cellgrid_t *grid, int cell_x,
                                int cell_y) {
  int region_x = cell_x / grid->region_size;
  int region_y = cell_y / grid->region_size;
  return region_y * grid->regions_x + region_x;
}

static cell_internal_t *get_cell_at_pixel(ak_cellgrid_t *grid, int px, int py) {
  int cx = px / grid->cell_size;
  int cy = py / grid->cell_size;
  if (cx >= 0 && cx < grid->cells_x && cy >= 0 && cy < grid->cells_y) {
    return &grid->cells[cell_index(grid, cx, cy)];
  }
  return NULL;
}

// ============================================================================
// Pixel Matrix Functions
// ============================================================================

static ak_pixel_matrix_t *pixel_matrix_new(int size, ak_color_t initial) {
  ak_pixel_matrix_t *pm = AK24_ALLOC(sizeof(ak_pixel_matrix_t));
  if (!pm)
    return NULL;

  pm->width = size;
  pm->height = size;
  pm->modified = false;
  pm->data = AK24_ALLOC(size * size * sizeof(ak_color_t));
  if (!pm->data) {
    AK24_FREE(pm);
    return NULL;
  }

  /* Fill with initial color */
  for (int i = 0; i < size * size; i++) {
    pm->data[i] = initial;
  }

  return pm;
}

static void pixel_matrix_free(ak_pixel_matrix_t *pm) {
  if (pm) {
    if (pm->data) {
      AK24_FREE(pm->data);
    }
    AK24_FREE(pm);
  }
}

static void pixel_matrix_fill(ak_pixel_matrix_t *pm, ak_color_t color) {
  for (int i = 0; i < pm->width * pm->height; i++) {
    pm->data[i] = color;
  }
  pm->modified = true;
}

// ============================================================================
// Cell Modification Functions
// ============================================================================

void ak_cell_fill(ak_cell_ctx_t *cell, uint8_t r, uint8_t g, uint8_t b,
                  uint8_t a) {
  ak_cell_fill_color(cell, ak_color(r, g, b, a));
}

void ak_cell_fill_color(ak_cell_ctx_t *cell, ak_color_t color) {
  if (!cell)
    return;
  cell->color = color;
  if (cell->pixels) {
    pixel_matrix_fill(cell->pixels, color);
  }
}

void ak_cell_set_pixel(ak_cell_ctx_t *cell, int px, int py, ak_color_t color) {
  if (!cell || !cell->pixels)
    return;
  if (px < 0 || px >= cell->pixels->width || py < 0 ||
      py >= cell->pixels->height)
    return;

  cell->pixels->data[py * cell->pixels->width + px] = color;
  cell->pixels->modified = true;
}

ak_color_t ak_cell_get_pixel(ak_cell_ctx_t *cell, int px, int py) {
  if (!cell || !cell->pixels)
    return ak_color(0, 0, 0, 0);
  if (px < 0 || px >= cell->pixels->width || py < 0 ||
      py >= cell->pixels->height)
    return ak_color(0, 0, 0, 0);

  return cell->pixels->data[py * cell->pixels->width + px];
}

void ak_cell_set_clicked(ak_cell_ctx_t *cell, bool clicked) {
  if (cell) {
    cell->clicked = clicked;
  }
}

// ============================================================================
// Click Handler (runs in region thread)
// ============================================================================

static void click_handler(void *captured_ctx, void *invoke_args) {
  (void)invoke_args;
  click_task_t *task = (click_task_t *)captured_ctx;
  ak_cellgrid_t *grid = task->grid;

  int idx = cell_index(grid, task->cell_x, task->cell_y);
  cell_internal_t *internal = &grid->cells[idx];

  /* Build cell context for callback */
  ak_cell_ctx_t ctx = {
      .x = internal->x,
      .y = internal->y,
      .region_id = internal->region_id,
      .clicked = internal->clicked,
      .color = internal->color,
      .mouse_x = task->mouse_x,
      .mouse_y = task->mouse_y,
      .pixels = &internal->pixels,
      ._internal = internal,
  };

  /* Call user's click handler */
  if (grid->on_click) {
    grid->on_click(&ctx, grid->user_data);
  }

  /* Apply changes back to internal cell (thread-safe via region isolation) */
  SDL_LockMutex(grid->render_mutex);
  internal->clicked = ctx.clicked;
  internal->color = ctx.color;
  /* Pixel matrix changes are already in internal->pixels */
  SDL_UnlockMutex(grid->render_mutex);
}

static void free_click_task(void *ptr) { AK24_FREE(ptr); }

static void dispatch_click(ak_cellgrid_t *grid, int mouse_x, int mouse_y) {
  cell_internal_t *cell = get_cell_at_pixel(grid, mouse_x, mouse_y);
  if (!cell)
    return;

  click_task_t *task = AK24_ALLOC(sizeof(click_task_t));
  if (!task)
    return;

  task->grid = grid;
  task->cell_x = cell->x;
  task->cell_y = cell->y;
  task->mouse_x = mouse_x;
  task->mouse_y = mouse_y;

  ak_lambda_t *lambda = ak_lambda_new(click_handler, task, free_click_task);
  if (!lambda) {
    AK24_FREE(task);
    return;
  }

  int region = cell->region_id;
  ak_thread_pool_enqueue(grid->region_pools[region], lambda, NULL);
}

// ============================================================================
// Rendering
// ============================================================================

static void render_cells(ak_cellgrid_t *grid) {
  SDL_LockMutex(grid->render_mutex);

  for (int i = 0; i < grid->total_cells; i++) {
    cell_internal_t *cell = &grid->cells[i];

    if (cell->pixels.modified) {
      /* Render per-pixel (advanced mode) */
      for (int py = 0; py < cell->pixels.height; py++) {
        for (int px = 0; px < cell->pixels.width; px++) {
          ak_color_t c = cell->pixels.data[py * cell->pixels.width + px];
          SDL_SetRenderDrawColor(grid->renderer, c.r, c.g, c.b, c.a);
          SDL_RenderDrawPoint(grid->renderer, cell->rect.x + px,
                              cell->rect.y + py);
        }
      }
    } else {
      /* Render solid color (simple mode) */
      SDL_SetRenderDrawColor(grid->renderer, cell->color.r, cell->color.g,
                             cell->color.b, cell->color.a);
      SDL_RenderFillRect(grid->renderer, &cell->rect);
    }
  }

  SDL_UnlockMutex(grid->render_mutex);

  /* Draw region boundaries */
  if (grid->draw_region_borders) {
    SDL_SetRenderDrawColor(grid->renderer, grid->grid_color.r,
                           grid->grid_color.g, grid->grid_color.b,
                           grid->grid_color.a);

    for (int ry = 0; ry <= grid->regions_y; ry++) {
      int y = ry * grid->region_size * grid->cell_size;
      if (y <= grid->screen_height) {
        SDL_RenderDrawLine(grid->renderer, 0, y, grid->screen_width, y);
      }
    }
    for (int rx = 0; rx <= grid->regions_x; rx++) {
      int x = rx * grid->region_size * grid->cell_size;
      if (x <= grid->screen_width) {
        SDL_RenderDrawLine(grid->renderer, x, 0, x, grid->screen_height);
      }
    }
  }
}

// ============================================================================
// Configuration
// ============================================================================

ak_cellgrid_opts_t ak_cellgrid_opts_default(void) {
  return (ak_cellgrid_opts_t){
      .screen_width = 800,
      .screen_height = 600,
      .title = "Cell Grid",
      .cell_size = 5,
      .cell_wrap = true,
      .region_size = 16,
      .bg_color = {30, 30, 30, 255},
      .cell_color = {60, 60, 60, 255},
      .grid_color = {200, 200, 200, 255},
      .draw_region_borders = true,
      .on_click = NULL,
      .on_frame = NULL,
      .on_init = NULL,
      .user_data = NULL,
  };
}

// ============================================================================
// Grid Lifecycle
// ============================================================================

ak_cellgrid_t *ak_cellgrid_new(const ak_cellgrid_opts_t *opts) {
  ak_cellgrid_opts_t config;
  if (opts) {
    config = *opts;
  } else {
    config = ak_cellgrid_opts_default();
  }

  ak_cellgrid_t *grid = AK24_ALLOC(sizeof(ak_cellgrid_t));
  if (!grid)
    return NULL;
  memset(grid, 0, sizeof(ak_cellgrid_t));

  /* Copy configuration */
  grid->screen_width = config.screen_width;
  grid->screen_height = config.screen_height;
  grid->cell_size = config.cell_size;
  grid->cell_wrap = config.cell_wrap;
  grid->region_size = config.region_size;
  grid->bg_color = config.bg_color;
  grid->cell_color = config.cell_color;
  grid->grid_color = config.grid_color;
  grid->draw_region_borders = config.draw_region_borders;
  grid->on_click = config.on_click;
  grid->on_frame = config.on_frame;
  grid->on_init = config.on_init;
  grid->user_data = config.user_data;

  /* Compute dimensions */
  if (grid->cell_wrap) {
    grid->cells_x = grid->screen_width / grid->cell_size;
    grid->cells_y = grid->screen_height / grid->cell_size;
  } else {
    grid->cells_x =
        (grid->screen_width + grid->cell_size - 1) / grid->cell_size;
    grid->cells_y =
        (grid->screen_height + grid->cell_size - 1) / grid->cell_size;
  }
  grid->total_cells = grid->cells_x * grid->cells_y;
  grid->regions_x = (grid->cells_x + grid->region_size - 1) / grid->region_size;
  grid->regions_y = (grid->cells_y + grid->region_size - 1) / grid->region_size;
  grid->total_regions = grid->regions_x * grid->regions_y;

  /* Initialize SDL */
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    AK24_LOG_ERROR("SDL_Init failed: %s", SDL_GetError());
    AK24_FREE(grid);
    return NULL;
  }

  grid->window = SDL_CreateWindow(config.title ? config.title : "Cell Grid",
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, grid->screen_width,
                                  grid->screen_height, SDL_WINDOW_SHOWN);
  if (!grid->window) {
    AK24_LOG_ERROR("SDL_CreateWindow failed: %s", SDL_GetError());
    SDL_Quit();
    AK24_FREE(grid);
    return NULL;
  }

  grid->renderer =
      SDL_CreateRenderer(grid->window, -1, SDL_RENDERER_ACCELERATED);
  if (!grid->renderer) {
    AK24_LOG_ERROR("SDL_CreateRenderer failed: %s", SDL_GetError());
    SDL_DestroyWindow(grid->window);
    SDL_Quit();
    AK24_FREE(grid);
    return NULL;
  }

  grid->render_mutex = SDL_CreateMutex();
  if (!grid->render_mutex) {
    AK24_LOG_ERROR("SDL_CreateMutex failed: %s", SDL_GetError());
    SDL_DestroyRenderer(grid->renderer);
    SDL_DestroyWindow(grid->window);
    SDL_Quit();
    AK24_FREE(grid);
    return NULL;
  }

  /* Allocate cells */
  grid->cells = AK24_ALLOC(grid->total_cells * sizeof(cell_internal_t));
  if (!grid->cells) {
    AK24_LOG_ERROR("Failed to allocate cell grid");
    SDL_DestroyMutex(grid->render_mutex);
    SDL_DestroyRenderer(grid->renderer);
    SDL_DestroyWindow(grid->window);
    SDL_Quit();
    AK24_FREE(grid);
    return NULL;
  }
  memset(grid->cells, 0, grid->total_cells * sizeof(cell_internal_t));

  /* Initialize cells */
  for (int y = 0; y < grid->cells_y; y++) {
    for (int x = 0; x < grid->cells_x; x++) {
      int idx = cell_index(grid, x, y);
      cell_internal_t *cell = &grid->cells[idx];
      cell->x = x;
      cell->y = y;
      cell->region_id = get_region_id(grid, x, y);
      cell->clicked = false;
      cell->color = grid->cell_color;
      cell->rect = (SDL_Rect){
          .x = x * grid->cell_size,
          .y = y * grid->cell_size,
          .w = grid->cell_size - 1,
          .h = grid->cell_size - 1,
      };
      /* Initialize pixel matrix */
      cell->pixels.width = grid->cell_size;
      cell->pixels.height = grid->cell_size;
      cell->pixels.modified = false;
      cell->pixels.data =
          AK24_ALLOC(grid->cell_size * grid->cell_size * sizeof(ak_color_t));
      if (cell->pixels.data) {
        for (int i = 0; i < grid->cell_size * grid->cell_size; i++) {
          cell->pixels.data[i] = grid->cell_color;
        }
      }
    }
  }

  /* Create region thread pools */
  grid->region_pools =
      AK24_ALLOC(grid->total_regions * sizeof(ak_thread_pool_t *));
  if (!grid->region_pools) {
    AK24_LOG_ERROR("Failed to allocate region pools array");
    /* Cleanup cells */
    for (int i = 0; i < grid->total_cells; i++) {
      if (grid->cells[i].pixels.data) {
        AK24_FREE(grid->cells[i].pixels.data);
      }
    }
    AK24_FREE(grid->cells);
    SDL_DestroyMutex(grid->render_mutex);
    SDL_DestroyRenderer(grid->renderer);
    SDL_DestroyWindow(grid->window);
    SDL_Quit();
    AK24_FREE(grid);
    return NULL;
  }
  memset(grid->region_pools, 0,
         grid->total_regions * sizeof(ak_thread_pool_t *));

  ak_thread_pool_config_t pool_config = ak_thread_pool_config_default();
  pool_config.max_workers = 1; /* One worker per region */

  for (int i = 0; i < grid->total_regions; i++) {
    grid->region_pools[i] = ak_thread_pool_new(&pool_config);
    if (!grid->region_pools[i]) {
      AK24_LOG_ERROR("Failed to create thread pool for region %d", i);
      /* Cleanup already created pools */
      for (int j = 0; j < i; j++) {
        ak_thread_pool_free(grid->region_pools[j]);
      }
      AK24_FREE(grid->region_pools);
      for (int j = 0; j < grid->total_cells; j++) {
        if (grid->cells[j].pixels.data) {
          AK24_FREE(grid->cells[j].pixels.data);
        }
      }
      AK24_FREE(grid->cells);
      SDL_DestroyMutex(grid->render_mutex);
      SDL_DestroyRenderer(grid->renderer);
      SDL_DestroyWindow(grid->window);
      SDL_Quit();
      AK24_FREE(grid);
      return NULL;
    }
  }

  grid->running = 0;

  AK24_LOG_INFO("Cell grid created: %dx%d cells in %dx%d regions",
                grid->cells_x, grid->cells_y, grid->regions_x, grid->regions_y);

  return grid;
}

int ak_cellgrid_run(ak_cellgrid_t *grid) {
  if (!grid)
    return -1;

  grid->running = 1;

  /* Call init callback */
  if (grid->on_init) {
    grid->on_init(grid, grid->user_data);
  }

  Uint32 last_frame = SDL_GetTicks();
  SDL_Event event;

  while (grid->running) {
    Uint32 now = SDL_GetTicks();
    Uint32 delta = now - last_frame;
    last_frame = now;

    /* Process events */
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        grid->running = 0;
      } else if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_ESCAPE) {
          grid->running = 0;
        }
      } else if (event.type == SDL_MOUSEBUTTONDOWN) {
        if (event.button.button == SDL_BUTTON_LEFT) {
          dispatch_click(grid, event.button.x, event.button.y);
        }
      }
    }

    /* Call frame update callback */
    if (grid->on_frame) {
      grid->on_frame(grid, grid->user_data, delta);
    }

    /* Clear and render */
    SDL_SetRenderDrawColor(grid->renderer, grid->bg_color.r, grid->bg_color.g,
                           grid->bg_color.b, grid->bg_color.a);
    SDL_RenderClear(grid->renderer);

    render_cells(grid);

    SDL_RenderPresent(grid->renderer);
    SDL_Delay(16); /* ~60 FPS */
  }

  return 0;
}

void ak_cellgrid_stop(ak_cellgrid_t *grid) {
  if (grid) {
    grid->running = 0;
  }
}

void ak_cellgrid_free(ak_cellgrid_t *grid) {
  if (!grid)
    return;

  /* Stop if still running */
  grid->running = 0;

  /* Free region thread pools */
  if (grid->region_pools) {
    for (int i = 0; i < grid->total_regions; i++) {
      if (grid->region_pools[i]) {
        ak_thread_pool_free(grid->region_pools[i]);
      }
    }
    AK24_FREE(grid->region_pools);
  }

  /* Free cells and their pixel data */
  if (grid->cells) {
    for (int i = 0; i < grid->total_cells; i++) {
      if (grid->cells[i].pixels.data) {
        AK24_FREE(grid->cells[i].pixels.data);
      }
    }
    AK24_FREE(grid->cells);
  }

  /* Cleanup SDL */
  if (grid->render_mutex) {
    SDL_DestroyMutex(grid->render_mutex);
  }
  if (grid->renderer) {
    SDL_DestroyRenderer(grid->renderer);
  }
  if (grid->window) {
    SDL_DestroyWindow(grid->window);
  }
  SDL_Quit();

  AK24_FREE(grid);

  AK24_LOG_INFO("Cell grid freed");
}

// ============================================================================
// Query Functions
// ============================================================================

void ak_cellgrid_get_dimensions(const ak_cellgrid_t *grid, int *cells_x,
                                int *cells_y, int *regions_x, int *regions_y) {
  if (!grid)
    return;
  if (cells_x)
    *cells_x = grid->cells_x;
  if (cells_y)
    *cells_y = grid->cells_y;
  if (regions_x)
    *regions_x = grid->regions_x;
  if (regions_y)
    *regions_y = grid->regions_y;
}

int ak_cellgrid_cell_count(const ak_cellgrid_t *grid) {
  return grid ? grid->total_cells : 0;
}

int ak_cellgrid_region_count(const ak_cellgrid_t *grid) {
  return grid ? grid->total_regions : 0;
}

const ak_cell_ctx_t *ak_cellgrid_get_cell(const ak_cellgrid_t *grid, int x,
                                          int y) {
  if (!grid || x < 0 || x >= grid->cells_x || y < 0 || y >= grid->cells_y)
    return NULL;

  /* Note: This returns a temporary view. For thread safety, caller should
   * use this only for reading or use ak_cellgrid_modify_cell for changes. */
  static __thread ak_cell_ctx_t temp_ctx;
  cell_internal_t *internal = &grid->cells[cell_index(grid, x, y)];

  temp_ctx.x = internal->x;
  temp_ctx.y = internal->y;
  temp_ctx.region_id = internal->region_id;
  temp_ctx.clicked = internal->clicked;
  temp_ctx.color = internal->color;
  temp_ctx.mouse_x = 0;
  temp_ctx.mouse_y = 0;
  temp_ctx.pixels = &internal->pixels;
  temp_ctx._internal = internal;

  return &temp_ctx;
}

/**
 * @brief Modify task context
 */
typedef struct {
  ak_cellgrid_t *grid;
  int cell_x;
  int cell_y;
  ak_cell_click_fn modifier;
  void *user_data;
} modify_task_t;

static void modify_handler(void *ctx, void *args) {
  (void)args;
  modify_task_t *mt = (modify_task_t *)ctx;
  ak_cellgrid_t *g = mt->grid;

  int idx = cell_index(g, mt->cell_x, mt->cell_y);
  cell_internal_t *internal = &g->cells[idx];

  ak_cell_ctx_t cell_ctx = {
      .x = internal->x,
      .y = internal->y,
      .region_id = internal->region_id,
      .clicked = internal->clicked,
      .color = internal->color,
      .mouse_x = mt->cell_x * g->cell_size + g->cell_size / 2,
      .mouse_y = mt->cell_y * g->cell_size + g->cell_size / 2,
      .pixels = &internal->pixels,
      ._internal = internal,
  };

  mt->modifier(&cell_ctx, mt->user_data);

  SDL_LockMutex(g->render_mutex);
  internal->clicked = cell_ctx.clicked;
  internal->color = cell_ctx.color;
  SDL_UnlockMutex(g->render_mutex);
}

static void free_modify_task(void *ptr) { AK24_FREE(ptr); }

void ak_cellgrid_modify_cell(ak_cellgrid_t *grid, int x, int y,
                             ak_cell_click_fn modifier, void *user_data) {
  if (!grid || !modifier || x < 0 || x >= grid->cells_x || y < 0 ||
      y >= grid->cells_y)
    return;

  modify_task_t *mtask = AK24_ALLOC(sizeof(modify_task_t));
  if (!mtask)
    return;

  mtask->grid = grid;
  mtask->cell_x = x;
  mtask->cell_y = y;
  mtask->modifier = modifier;
  mtask->user_data = user_data;

  int region = get_region_id(grid, x, y);
  ak_lambda_t *lambda = ak_lambda_new(modify_handler, mtask, free_modify_task);
  if (!lambda) {
    AK24_FREE(mtask);
    return;
  }

  ak_thread_pool_enqueue(grid->region_pools[region], lambda, NULL);
}
