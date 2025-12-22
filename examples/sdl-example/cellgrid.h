/**
 * @file cellgrid.h
 * @brief SDL2 Cell Grid abstraction library
 *
 * Provides a high-level API for creating interactive cell grids without
 * requiring direct SDL2 knowledge. Cells are organized into regions,
 * with each region processed by a dedicated thread for click handling.
 *
 * Key features:
 * - Configurable grid sizing (screen, cell, region dimensions)
 * - Lambda-based click callbacks with cell context
 * - Pixel matrix interface for cell modifications
 * - Region-based thread pooling for concurrent click processing
 * - Automatic SDL2 lifecycle management
 *
 * @par Example:
 * @code
 * void on_click(ak_cell_ctx_t *cell, void *user_data) {
 *   // Toggle between two colors
 *   if (cell->clicked) {
 *     ak_cell_fill(cell, 60, 60, 60, 255);
 *   } else {
 *     ak_cell_fill(cell, 200, 100, 50, 255);
 *   }
 * }
 *
 * int main() {
 *   ak_cellgrid_opts_t opts = ak_cellgrid_opts_default();
 *   opts.screen_width = 800;
 *   opts.screen_height = 600;
 *   opts.on_click = on_click;
 *
 *   ak_cellgrid_t *grid = ak_cellgrid_new(&opts);
 *   ak_cellgrid_run(grid);  // Blocking event loop
 *   ak_cellgrid_free(grid);
 * }
 * @endcode
 */

#ifndef AK24_CELLGRID_H
#define AK24_CELLGRID_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @def CELLGRID_VERSION
 * @brief Cell grid library version string
 */
#define CELLGRID_VERSION "0.0.1-dev"

// ============================================================================
// Forward Declarations
// ============================================================================

typedef struct ak_cellgrid_t ak_cellgrid_t;
typedef struct ak_cell_ctx_t ak_cell_ctx_t;
typedef struct ak_pixel_matrix_t ak_pixel_matrix_t;

// ============================================================================
// Color Type
// ============================================================================

/**
 * @brief RGBA color structure
 */
typedef struct {
  uint8_t r; /**< Red component (0-255) */
  uint8_t g; /**< Green component (0-255) */
  uint8_t b; /**< Blue component (0-255) */
  uint8_t a; /**< Alpha component (0-255, 255 = opaque) */
} ak_color_t;

/**
 * @brief Create a color from RGBA values
 */
static inline ak_color_t ak_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  return (ak_color_t){r, g, b, a};
}

/**
 * @brief Create an opaque color from RGB values
 */
static inline ak_color_t ak_color_rgb(uint8_t r, uint8_t g, uint8_t b) {
  return (ak_color_t){r, g, b, 255};
}

// ============================================================================
// Callback Types
// ============================================================================

/**
 * @brief Cell click callback signature
 *
 * Called when a cell is clicked. The callback executes in the region's
 * dedicated thread, ensuring thread isolation per region.
 *
 * @param cell Cell context with position, state, and modification methods
 * @param user_data User-provided context from opts
 */
typedef void (*ak_cell_click_fn)(ak_cell_ctx_t *cell, void *user_data);

/**
 * @brief Frame update callback signature (optional)
 *
 * Called once per frame before rendering. Executes in the main thread.
 *
 * @param grid The cell grid instance
 * @param user_data User-provided context from opts
 * @param delta_ms Milliseconds since last frame
 */
typedef void (*ak_frame_update_fn)(ak_cellgrid_t *grid, void *user_data,
                                   uint32_t delta_ms);

/**
 * @brief Grid initialization callback signature (optional)
 *
 * Called after grid is created but before event loop starts.
 *
 * @param grid The cell grid instance
 * @param user_data User-provided context from opts
 */
typedef void (*ak_grid_init_fn)(ak_cellgrid_t *grid, void *user_data);

// ============================================================================
// Configuration
// ============================================================================

/**
 * @brief Cell grid configuration options
 *
 * Defines all sizing, behavior, and callback settings for the grid.
 */
typedef struct {
  /* Window settings */
  int screen_width;  /**< Window width in pixels (default: 800) */
  int screen_height; /**< Window height in pixels (default: 600) */
  const char *title; /**< Window title (default: "Cell Grid") */

  /* Cell settings */
  int cell_size;  /**< Cell size in pixels (cells are squares, default: 5) */
  bool cell_wrap; /**< Wrap cells to fit screen vs truncate (default: true) */

  /* Region settings (for thread pooling) */
  int region_size; /**< Region size in cells (NxN, default: 16) */

  /* Colors */
  ak_color_t bg_color;      /**< Background color (default: dark gray) */
  ak_color_t cell_color;    /**< Default cell color (default: medium gray) */
  ak_color_t grid_color;    /**< Grid line color (default: light gray) */
  bool draw_region_borders; /**< Draw region boundaries (default: true) */

  /* Callbacks */
  ak_cell_click_fn on_click;   /**< Cell click handler (required) */
  ak_frame_update_fn on_frame; /**< Per-frame update (optional) */
  ak_grid_init_fn on_init;     /**< Grid initialization (optional) */

  /* User context */
  void *user_data; /**< User data passed to all callbacks */
} ak_cellgrid_opts_t;

/**
 * @brief Create default cell grid options
 *
 * @return Options initialized with sensible defaults
 */
ak_cellgrid_opts_t ak_cellgrid_opts_default(void);

// ============================================================================
// Cell Context (passed to click callbacks)
// ============================================================================

/**
 * @brief Cell context provided to click callbacks
 *
 * Contains cell position, state, and provides methods for modification.
 * Modifications are thread-safe within the region's thread.
 */
struct ak_cell_ctx_t {
  /* Cell position */
  int x;         /**< Cell X index in grid */
  int y;         /**< Cell Y index in grid */
  int region_id; /**< Region this cell belongs to */

  /* Cell state */
  bool clicked;     /**< Current click/toggle state */
  ak_color_t color; /**< Current cell color */

  /* Click event info */
  int mouse_x; /**< Mouse X position in pixels */
  int mouse_y; /**< Mouse Y position in pixels */

  /* Pixel access (for advanced modifications) */
  ak_pixel_matrix_t
      *pixels; /**< Pixel matrix for cell (cell_size x cell_size) */

  /* Internal - do not modify */
  void *_internal;
};

// ============================================================================
// Pixel Matrix (for advanced cell modifications)
// ============================================================================

/**
 * @brief Pixel matrix for per-pixel cell modifications
 *
 * Allows setting individual pixels within a cell. Changes are applied
 * when the callback returns.
 */
struct ak_pixel_matrix_t {
  int width;        /**< Matrix width (cell_size) */
  int height;       /**< Matrix height (cell_size) */
  ak_color_t *data; /**< Pixel data (row-major, width * height) */
  bool modified;    /**< Set to true when pixels are changed */
};

// ============================================================================
// Cell Modification Functions
// ============================================================================

/**
 * @brief Fill entire cell with a solid color
 *
 * @param cell Cell context
 * @param r Red component
 * @param g Green component
 * @param b Blue component
 * @param a Alpha component
 */
void ak_cell_fill(ak_cell_ctx_t *cell, uint8_t r, uint8_t g, uint8_t b,
                  uint8_t a);

/**
 * @brief Fill entire cell with a color struct
 *
 * @param cell Cell context
 * @param color Color to fill
 */
void ak_cell_fill_color(ak_cell_ctx_t *cell, ak_color_t color);

/**
 * @brief Set a single pixel in the cell
 *
 * @param cell Cell context
 * @param px Pixel X (0 to cell_size-1)
 * @param py Pixel Y (0 to cell_size-1)
 * @param color Pixel color
 */
void ak_cell_set_pixel(ak_cell_ctx_t *cell, int px, int py, ak_color_t color);

/**
 * @brief Get a pixel color from the cell
 *
 * @param cell Cell context
 * @param px Pixel X (0 to cell_size-1)
 * @param py Pixel Y (0 to cell_size-1)
 * @return Pixel color
 */
ak_color_t ak_cell_get_pixel(ak_cell_ctx_t *cell, int px, int py);

/**
 * @brief Set cell's clicked state
 *
 * @param cell Cell context
 * @param clicked New clicked state
 */
void ak_cell_set_clicked(ak_cell_ctx_t *cell, bool clicked);

// ============================================================================
// Cell Grid API
// ============================================================================

/**
 * @brief Create a new cell grid
 *
 * Initializes SDL2, creates the window and renderer, allocates cells,
 * and sets up region thread pools.
 *
 * @param opts Configuration options (NULL for defaults)
 * @return Grid instance or NULL on failure
 *
 * @note Caller must free with ak_cellgrid_free()
 */
ak_cellgrid_t *ak_cellgrid_new(const ak_cellgrid_opts_t *opts);

/**
 * @brief Run the cell grid event loop
 *
 * Blocking call that runs until the window is closed, ESC is pressed,
 * or ak_cellgrid_stop() is called.
 *
 * @param grid Grid instance
 * @return Exit code (0 = normal, non-zero = error)
 */
int ak_cellgrid_run(ak_cellgrid_t *grid);

/**
 * @brief Signal the grid to stop running
 *
 * Can be called from any thread to request shutdown.
 *
 * @param grid Grid instance
 */
void ak_cellgrid_stop(ak_cellgrid_t *grid);

/**
 * @brief Free cell grid resources
 *
 * Shuts down thread pools, destroys SDL resources, and frees memory.
 *
 * @param grid Grid instance
 */
void ak_cellgrid_free(ak_cellgrid_t *grid);

// ============================================================================
// Grid Query Functions
// ============================================================================

/**
 * @brief Get grid dimensions
 *
 * @param grid Grid instance
 * @param cells_x Output: number of cells horizontally (can be NULL)
 * @param cells_y Output: number of cells vertically (can be NULL)
 * @param regions_x Output: number of regions horizontally (can be NULL)
 * @param regions_y Output: number of regions vertically (can be NULL)
 */
void ak_cellgrid_get_dimensions(const ak_cellgrid_t *grid, int *cells_x,
                                int *cells_y, int *regions_x, int *regions_y);

/**
 * @brief Get total number of cells
 *
 * @param grid Grid instance
 * @return Total cell count
 */
int ak_cellgrid_cell_count(const ak_cellgrid_t *grid);

/**
 * @brief Get total number of regions
 *
 * @param grid Grid instance
 * @return Total region count
 */
int ak_cellgrid_region_count(const ak_cellgrid_t *grid);

/**
 * @brief Get cell at grid coordinates
 *
 * @param grid Grid instance
 * @param x Cell X index
 * @param y Cell Y index
 * @return Read-only cell info, or NULL if out of bounds
 *
 * @note The returned pointer is only valid until the next click event
 *       modifies the cell. Use for querying, not storing.
 */
const ak_cell_ctx_t *ak_cellgrid_get_cell(const ak_cellgrid_t *grid, int x,
                                          int y);

/**
 * @brief Modify a cell programmatically
 *
 * Dispatches a modification to the cell's region thread. The callback
 * receives a mutable cell context.
 *
 * @param grid Grid instance
 * @param x Cell X index
 * @param y Cell Y index
 * @param modifier Callback to modify the cell
 * @param user_data Data passed to modifier callback
 */
void ak_cellgrid_modify_cell(ak_cellgrid_t *grid, int x, int y,
                             ak_cell_click_fn modifier, void *user_data);

#endif /* AK24_CELLGRID_H */
