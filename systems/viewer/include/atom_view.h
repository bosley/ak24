/**
 * @file atom_view.h
 * @brief N-Dimensional Atom Viewer System
 *
 * This system provides a way to visualize N-dimensional atom clusters by
 * projecting them onto a 2D view. Users can rotate, scale, and inspect atom
 * clusters from any angle.
 *
 * Key Features:
 * - Project N-dimensional atoms onto 2D views
 * - Support for rotation in N-dimensional space (plane-based)
 * - Depth-aware rendering with occlusion
 * - Configurable axis projection and scaling
 * - Arena allocator support for fast cleanup
 * - Thread pool support for parallel rendering
 *
 * @see ATOM_VIEW_PLAN.md for detailed design documentation
 */

#ifndef AK24_ATOM_VIEW_H
#define AK24_ATOM_VIEW_H

#include "kernel/arena/include/arena.h"
#include "kernel/atoms/include/atom.h"
#include "kernel/threads/include/threads.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Rotation specification for a plane in N-dimensional space
 */
typedef struct {
  size_t axis_i; /**< First axis of rotation plane */
  size_t axis_j; /**< Second axis of rotation plane */
  double angle;  /**< Rotation angle in degrees */
} ak_atom_view_rotation_t;

/**
 * @brief Associates an atom with N-dimensional spatial coordinates
 *
 * The atom itself is read-only; this structure just adds position information
 * for visualization purposes.
 */
typedef struct {
  ak_atom_t *atom;       /**< Pointer to actual atom (read-only) */
  double *coords;        /**< N-dimensional coordinates array */
  size_t dimensionality; /**< Number of dimensions (e.g., 3, 4, 5...) */
} ak_atom_positioned_t;

/**
 * @brief 2D view output buffer with depth information
 *
 * This is the result of projecting N-dimensional atoms onto a 2D plane.
 * Each pixel can contain a pointer to an atom and its depth for occlusion.
 */
typedef struct {
  ak_atom_t ***pixels; /**< [height][width] -> atom pointer or NULL */
  double **depths;     /**< [height][width] -> depth value */
  size_t width;        /**< View width in pixels */
  size_t height;       /**< View height in pixels */
} ak_atom_view_t;

/**
 * @brief Configuration for N-dimensional projection onto 2D
 *
 * Stores which axes to project and how to rotate the view.
 */
typedef struct {
  size_t axis_x;                      /**< Which dimension maps to screen X */
  size_t axis_y;                      /**< Which dimension maps to screen Y */
  double scale;                       /**< Pixels per unit distance */
  ak_atom_view_rotation_t *rotations; /**< Array of rotation specifications */
  size_t num_rotation_pairs;          /**< Number of rotation pairs */
  size_t dimensionality;              /**< Total number of dimensions */
} ak_atom_view_config_t;

/* ========================================================================== */
/*                           View Creation and Rendering                      */
/* ========================================================================== */

/**
 * @brief Project N-dimensional atoms onto 2D view (basic version)
 *
 * Allocates view using standard malloc. You must call ak_atom_view_free() when
 * done.
 *
 * @param atoms Array of positioned atoms to render
 * @param num_atoms Number of atoms in array
 * @param config View configuration (axes, scale, rotations)
 * @param width Output width in pixels
 * @param height Output height in pixels
 * @return Newly allocated view, or NULL on failure
 */
ak_atom_view_t *ak_atom_project_view(ak_atom_positioned_t *atoms,
                                     size_t num_atoms,
                                     ak_atom_view_config_t *config,
                                     size_t width, size_t height);

/**
 * @brief Project view using arena allocator (recommended)
 *
 * Allocates all view data from the arena for fast cleanup. Perfect for
 * animation where you render many frames.
 *
 * @param arena Arena to allocate from
 * @param atoms Array of positioned atoms to render
 * @param num_atoms Number of atoms in array
 * @param config View configuration (axes, scale, rotations)
 * @param width Output width in pixels
 * @param height Output height in pixels
 * @return View allocated from arena, or NULL on failure
 */
ak_atom_view_t *ak_atom_project_view_arena(ak_arena_t *arena,
                                           ak_atom_positioned_t *atoms,
                                           size_t num_atoms,
                                           ak_atom_view_config_t *config,
                                           size_t width, size_t height);

/**
 * @brief Project view using thread pool for parallel processing
 *
 * Best for large atom counts (>1000). Splits atoms into chunks and processes
 * them in parallel, then merges results with proper depth comparison.
 *
 * @param pool Thread pool to use for parallel processing
 * @param arena Arena to allocate results from
 * @param atoms Array of positioned atoms to render
 * @param num_atoms Number of atoms in array
 * @param config View configuration (axes, scale, rotations)
 * @param width Output width in pixels
 * @param height Output height in pixels
 * @return View allocated from arena, or NULL on failure
 */
ak_atom_view_t *ak_atom_project_view_parallel(ak_thread_pool_t *pool,
                                              ak_arena_t *arena,
                                              ak_atom_positioned_t *atoms,
                                              size_t num_atoms,
                                              ak_atom_view_config_t *config,
                                              size_t width, size_t height);

/**
 * @brief Free a view structure (only for non-arena views)
 *
 * Do NOT call this for views allocated with arena - just free the arena
 * instead.
 *
 * @param view View to free
 */
void ak_atom_view_free(ak_atom_view_t *view);

/* ========================================================================== */
/*                         View Configuration Helpers                         */
/* ========================================================================== */

/**
 * @brief Create default view configuration
 *
 * Creates config with:
 * - axis_x = 0, axis_y = 1 (project first two dimensions)
 * - scale = 10.0 (10 pixels per unit)
 * - No rotations
 *
 * @param dimensionality Number of dimensions in the space
 * @return Newly allocated config, or NULL on failure
 */
ak_atom_view_config_t *ak_atom_view_config_new(size_t dimensionality);

/**
 * @brief Free view configuration
 *
 * @param config Config to free
 */
void ak_atom_view_config_free(ak_atom_view_config_t *config);

/**
 * @brief Rotate view around a plane
 *
 * Adds or updates a rotation in the specified plane. Rotations are applied
 * in the order they were added.
 *
 * @param config Config to modify
 * @param axis_i First axis of rotation plane
 * @param axis_j Second axis of rotation plane
 * @param degrees Rotation angle in degrees (positive = counterclockwise)
 */
void ak_atom_view_rotate(ak_atom_view_config_t *config, size_t axis_i,
                         size_t axis_j, double degrees);

/**
 * @brief Set which axes project to screen X and Y
 *
 * @param config Config to modify
 * @param axis_x Which dimension maps to screen X
 * @param axis_y Which dimension maps to screen Y
 */
void ak_atom_view_set_axes(ak_atom_view_config_t *config, size_t axis_x,
                           size_t axis_y);

/**
 * @brief Set the scale (pixels per unit)
 *
 * @param config Config to modify
 * @param scale Pixels per unit distance
 */
void ak_atom_view_set_scale(ak_atom_view_config_t *config, double scale);

/* ========================================================================== */
/*                              Utility Functions                             */
/* ========================================================================== */

/**
 * @brief Create positioned atoms from cluster at regular spacing
 *
 * Helper function that positions atoms from a cluster in a regular grid.
 * Useful for creating simple test visualizations.
 *
 * @param cluster Atom cluster to position
 * @param dimensionality Number of dimensions for positioning
 * @param spacing Distance between atoms in grid
 * @return Array of positioned atoms (caller must free)
 */
ak_atom_positioned_t *ak_atom_cluster_to_positioned(ak_atom_cluster_t *cluster,
                                                    size_t dimensionality,
                                                    double spacing);

/**
 * @brief Free array of positioned atoms (but not the atoms themselves)
 *
 * @param positioned Array to free
 * @param count Number of atoms in array
 */
void ak_atom_positioned_free(ak_atom_positioned_t *positioned, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* AK24_ATOM_VIEW_H */
