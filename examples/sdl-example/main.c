/**
 * @file main.c
 * @brief SDL2 example demonstrating AK24 application framework with graphics
 *
 * This example demonstrates how to use the AK24 application framework with
 * SDL2 for creating a basic windowed application with signal handling.
 *
 * Usage:
 *   ./sdl-example
 */

#include "kernel/application.h"
#include "kernel/lambda/include/lambda.h"
#include "kernel/threads/include/threads.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// ============================================================================
// Configuration Macros
// ============================================================================

/** @brief Screen width in pixels */
#define SCREEN_WIDTH 800

/** @brief Screen height in pixels */
#define SCREEN_HEIGHT 600

/** @brief Cell size in pixels (cells are squares) */
#define CELL_SIZE 50

/**
 * @brief Cell wrapping behavior
 * - true:  Wrap cells to next row if they would extend past screen edge
 * - false: Truncate (don't extend cells past screen edge)
 */
#define CELL_WRAP true

/**
 * @brief Region size (NxN cells per region)
 * Each region is processed by a single thread in the thread pool.
 * One thread per region handles all callbacks for cells within that region.
 */
#define REGION_SIZE 2

// ============================================================================
// Derived Constants
// ============================================================================

/** @brief Number of cells that fit horizontally */
#define CELLS_X (CELL_WRAP ? (SCREEN_WIDTH / CELL_SIZE) : ((SCREEN_WIDTH + CELL_SIZE - 1) / CELL_SIZE))

/** @brief Number of cells that fit vertically */
#define CELLS_Y (CELL_WRAP ? (SCREEN_HEIGHT / CELL_SIZE) : ((SCREEN_HEIGHT + CELL_SIZE - 1) / CELL_SIZE))

/** @brief Total number of cells */
#define TOTAL_CELLS (CELLS_X * CELLS_Y)

/** @brief Number of regions horizontally */
#define REGIONS_X ((CELLS_X + REGION_SIZE - 1) / REGION_SIZE)

/** @brief Number of regions vertically */
#define REGIONS_Y ((CELLS_Y + REGION_SIZE - 1) / REGION_SIZE)

/** @brief Total number of regions (one thread per region) */
#define TOTAL_REGIONS (REGIONS_X * REGIONS_Y)

// ============================================================================
// Cell and Region Structures
// ============================================================================

/**
 * @brief Cell state structure
 */
typedef struct {
  int x;          /**< Cell X index */
  int y;          /**< Cell Y index */
  int region_id;  /**< Region this cell belongs to */
  SDL_Rect rect;  /**< Screen rectangle for this cell */
  SDL_Color color; /**< Current cell color */
  int clicked;    /**< Click state flag */
} cell_t;

/**
 * @brief Click event data passed to cell callback
 */
typedef struct {
  int cell_x;     /**< Cell X index that was clicked */
  int cell_y;     /**< Cell Y index that was clicked */
  int mouse_x;    /**< Mouse X position */
  int mouse_y;    /**< Mouse Y position */
} click_event_t;

// ============================================================================
// Global State
// ============================================================================

static volatile int keep_running = 1;
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static cell_t *cells = NULL;
static ak_thread_pool_t *region_pools[TOTAL_REGIONS] = {0};
static SDL_mutex *render_mutex = NULL;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Get cell index from grid coordinates
 */
static inline int cell_index(int x, int y) { return y * CELLS_X + x; }

/**
 * @brief Get region ID from cell coordinates
 */
static inline int get_region_id(int cell_x, int cell_y) {
  int region_x = cell_x / REGION_SIZE;
  int region_y = cell_y / REGION_SIZE;
  return region_y * REGIONS_X + region_x;
}

/**
 * @brief Get cell from pixel coordinates
 * @return Cell pointer or NULL if outside grid
 */
static cell_t *get_cell_at_pixel(int px, int py) {
  int cx = px / CELL_SIZE;
  int cy = py / CELL_SIZE;
  if (cx >= 0 && cx < CELLS_X && cy >= 0 && cy < CELLS_Y) {
    return &cells[cell_index(cx, cy)];
  }
  return NULL;
}

/**
 * @brief Cell click handler - executed in the region's thread
 */
static void cell_click_handler(void *captured_ctx, void *invoke_args) {
  (void)invoke_args;
  click_event_t *event = (click_event_t *)captured_ctx;

  int idx = cell_index(event->cell_x, event->cell_y);
  cell_t *cell = &cells[idx];

  // Toggle cell color on click (thread-safe via region isolation)
  SDL_LockMutex(render_mutex);
  if (cell->clicked) {
    // Reset to default color
    cell->color = (SDL_Color){60, 60, 60, 255};
    cell->clicked = 0;
  } else {
    // Set to highlight color based on region
    int region = cell->region_id;
    cell->color = (SDL_Color){
        (Uint8)(100 + (region * 37) % 155),
        (Uint8)(100 + (region * 73) % 155),
        (Uint8)(100 + (region * 111) % 155),
        255};
    cell->clicked = 1;
  }
  SDL_UnlockMutex(render_mutex);

  AK24_LOG_INFO("Cell [%d,%d] clicked in region %d (thread processing)",
                event->cell_x, event->cell_y, cell->region_id);
}

/**
 * @brief Initialize cell grid
 */
static int init_cells(void) {
  cells = calloc(TOTAL_CELLS, sizeof(cell_t));
  if (!cells) {
    fprintf(stderr, "Failed to allocate cell grid\n");
    return -1;
  }

  for (int y = 0; y < CELLS_Y; y++) {
    for (int x = 0; x < CELLS_X; x++) {
      int idx = cell_index(x, y);
      cells[idx].x = x;
      cells[idx].y = y;
      cells[idx].region_id = get_region_id(x, y);
      cells[idx].rect = (SDL_Rect){
          .x = x * CELL_SIZE,
          .y = y * CELL_SIZE,
          .w = CELL_SIZE - 1, // -1 for grid line
          .h = CELL_SIZE - 1};
      cells[idx].color = (SDL_Color){60, 60, 60, 255};
      cells[idx].clicked = 0;
    }
  }

  AK24_LOG_INFO("Initialized %d cells (%dx%d) in %d regions (%dx%d)", TOTAL_CELLS,
                CELLS_X, CELLS_Y, TOTAL_REGIONS, REGIONS_X, REGIONS_Y);
  return 0;
}

/**
 * @brief Initialize thread pools (one per region)
 */
static int init_region_pools(void) {
  ak_thread_pool_config_t config = ak_thread_pool_config_default();
  config.max_workers = 1; // Each region gets exactly one worker thread

  for (int i = 0; i < TOTAL_REGIONS; i++) {
    region_pools[i] = ak_thread_pool_new(&config);
    if (!region_pools[i]) {
      fprintf(stderr, "Failed to create thread pool for region %d\n", i);
      return -1;
    }
  }

  AK24_LOG_INFO("Created %d region thread pools (1 worker each)", TOTAL_REGIONS);
  return 0;
}

/**
 * @brief Handle mouse click - dispatch to appropriate region thread
 */
static void handle_click(int mouse_x, int mouse_y) {
  cell_t *cell = get_cell_at_pixel(mouse_x, mouse_y);
  if (!cell)
    return;

  // Create click event data
  click_event_t *event = malloc(sizeof(click_event_t));
  if (!event)
    return;

  event->cell_x = cell->x;
  event->cell_y = cell->y;
  event->mouse_x = mouse_x;
  event->mouse_y = mouse_y;

  // Create lambda with click event as captured context
  ak_lambda_t *task = ak_lambda_new(cell_click_handler, event, free);
  if (!task) {
    free(event);
    return;
  }

  // Dispatch to the region's thread pool
  int region = cell->region_id;
  ak_thread_pool_enqueue(region_pools[region], task, NULL);
}

/**
 * @brief Render all cells
 */
static void render_cells(void) {
  SDL_LockMutex(render_mutex);
  for (int i = 0; i < TOTAL_CELLS; i++) {
    SDL_SetRenderDrawColor(renderer, cells[i].color.r, cells[i].color.g,
                           cells[i].color.b, cells[i].color.a);
    SDL_RenderFillRect(renderer, &cells[i].rect);
  }
  SDL_UnlockMutex(render_mutex);

  // Draw region boundaries
  SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
  for (int ry = 0; ry <= REGIONS_Y; ry++) {
    int y = ry * REGION_SIZE * CELL_SIZE;
    if (y <= SCREEN_HEIGHT) {
      SDL_RenderDrawLine(renderer, 0, y, SCREEN_WIDTH, y);
    }
  }
  for (int rx = 0; rx <= REGIONS_X; rx++) {
    int x = rx * REGION_SIZE * CELL_SIZE;
    if (x <= SCREEN_WIDTH) {
      SDL_RenderDrawLine(renderer, x, 0, x, SCREEN_HEIGHT);
    }
  }
}

APP_ON_SIGNAL(handle_sigint, SIGINT) {
  (void)captured;
  (void)args;
  printf("\n[SIGINT] Caught Ctrl+C, shutting down...\n");
  keep_running = 0;
}

APP_ON_SIGNAL(handle_sigterm, SIGTERM) {
  (void)captured;
  (void)args;
  printf("\n[SIGTERM] Received termination signal\n");
  keep_running = 0;
}

APP_ON_SHUTDOWN(on_shutdown) {
  (void)ctx;

  // Shutdown all region thread pools
  for (int i = 0; i < TOTAL_REGIONS; i++) {
    if (region_pools[i]) {
      ak_thread_pool_free(region_pools[i]);
      region_pools[i] = NULL;
    }
  }

  // Free cells
  if (cells) {
    free(cells);
    cells = NULL;
  }

  // Destroy render mutex
  if (render_mutex) {
    SDL_DestroyMutex(render_mutex);
    render_mutex = NULL;
  }

  if (renderer) {
    SDL_DestroyRenderer(renderer);
    renderer = NULL;
  }
  if (window) {
    SDL_DestroyWindow(window);
    window = NULL;
  }
  SDL_Quit();
  printf("SDL cleanup complete\n");
}

APP_MAIN(app_main) {
  ak_log_set_level(AK24_LOG_LEVEL_INFO);
  ak_log_set_color(true);
  ak_log_set_path_format(AK24_LOG_PATH_ABBREV);

  (void)ctx;

  printf("=== AK24 SDL2 Cell Grid Example ===\n\n");
  printf("Configuration:\n");
  printf("  Screen:  %dx%d pixels\n", SCREEN_WIDTH, SCREEN_HEIGHT);
  printf("  Cell:    %dx%d pixels\n", CELL_SIZE, CELL_SIZE);
  printf("  Grid:    %dx%d cells (%d total)\n", CELLS_X, CELLS_Y, TOTAL_CELLS);
  printf("  Regions: %dx%d (%d total, %dx%d cells each)\n", REGIONS_X, REGIONS_Y,
         TOTAL_REGIONS, REGION_SIZE, REGION_SIZE);
  printf("  Wrap:    %s\n\n", CELL_WRAP ? "enabled" : "disabled");

  AK24_REGISTER_SIGNAL_HANDLER(handle_sigint);
  AK24_REGISTER_SIGNAL_HANDLER(handle_sigterm);

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }

  window = SDL_CreateWindow("AK24 Cell Grid - Click cells!", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT,
                            SDL_WINDOW_SHOWN);
  if (!window) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  render_mutex = SDL_CreateMutex();
  if (!render_mutex) {
    fprintf(stderr, "SDL_CreateMutex failed: %s\n", SDL_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  if (init_cells() != 0) {
    SDL_DestroyMutex(render_mutex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  if (init_region_pools() != 0) {
    free(cells);
    SDL_DestroyMutex(render_mutex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  AK24_LOG_INFO("SDL2 window and cell grid created successfully");

  SDL_Event event;
  while (keep_running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        keep_running = 0;
      } else if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_ESCAPE) {
          keep_running = 0;
        }
      } else if (event.type == SDL_MOUSEBUTTONDOWN) {
        if (event.button.button == SDL_BUTTON_LEFT) {
          handle_click(event.button.x, event.button.y);
        }
      }
    }

    // Clear screen
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    // Render cell grid
    render_cells();

    SDL_RenderPresent(renderer);
    SDL_Delay(16);
  }

  printf("\nExiting SDL example\n");
  return 0;
}

AK24_APPLICATION("sdl-example", app_main, on_shutdown)
