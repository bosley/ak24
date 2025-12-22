/**
 * @file entity.h
 * @brief Spatial entity grid system with directional mappings
 *
 * Provides a 2D grid of entities with directional spatial mappings.
 * Each entity has a 3D aspect: the main plane (fabric) plus IN/OUT
 * directions for layered/depth traversal. Think of it like a towel:
 * the main grid is the fabric, IN/OUT are the threads on surfaces.
 *
 * Key features:
 * - 2D grid of entities with void* user data
 * - Directional spatial mappings (8 cardinal + IN/OUT)
 * - Bresenham line iteration between points
 * - Neighbor iteration (immediate, scalar rings, fills)
 * - Lambda-based iteration callbacks
 *
 * @note Entity system does NOT allocate or free user data.
 *       It only provides mappings and iteration primitives.
 * @note Uses AK24_ALLOC/AK24_FREE for memory management.
 */

#ifndef SDL_EXAMPLE_ENTITY_H
#define SDL_EXAMPLE_ENTITY_H

#include "kernel/kernel.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Entity direction enumeration
 *
 * IN = self/depth inward, OUT = visual expression/depth outward
 * Cardinal directions for spatial neighbor mappings.
 */
typedef enum {
  ENTITY_DIRECTION_IN = 0,
  ENTITY_DIRECTION_NORTH,
  ENTITY_DIRECTION_NORTH_EAST,
  ENTITY_DIRECTION_EAST,
  ENTITY_DIRECTION_SOUTH_EAST,
  ENTITY_DIRECTION_SOUTH,
  ENTITY_DIRECTION_SOUTH_WEST,
  ENTITY_DIRECTION_WEST,
  ENTITY_DIRECTION_NORTH_WEST,
  ENTITY_DIRECTION_OUT,
  ENTITY_DIRECTION_COUNT
} entity_direction_e;

/**
 * @brief Entity structure with spatial mappings
 *
 * Each entity has a unique ID, spatial neighbor mappings,
 * and a user-defined data pointer. The grid system does NOT
 * manage the data pointer - that is the user's responsibility.
 */
typedef struct entity_t {
  size_t unique_id;                              /**< Unique entity identifier */
  int x;                                         /**< Grid x coordinate */
  int y;                                         /**< Grid y coordinate */
  struct entity_t *spatial_mapping[ENTITY_DIRECTION_COUNT]; /**< Neighbor mappings */
  void *data;                                    /**< User-defined data (NOT managed by entity system) */
} entity_t;

/**
 * @brief Entity grid structure (the "fabric")
 *
 * 2D array of entity pointers representing the main traversable plane.
 */
typedef struct {
  entity_t **entities;  /**< Flat array of entity pointers [y * width + x] */
  int width;            /**< Grid width in entities */
  int height;           /**< Grid height in entities */
  size_t next_id;       /**< Next unique ID to assign */
  bool wrap;            /**< Whether grid wraps at edges */
} entity_grid_t;

/**
 * @brief Iteration context passed to lambdas
 *
 * Contains the entity being visited and iteration metadata.
 */
typedef struct {
  entity_t *entity;           /**< Current entity */
  int x;                      /**< Entity x coordinate */
  int y;                      /**< Entity y coordinate */
  entity_direction_e from_dir; /**< Direction we came from (for neighbor iterations) */
  int distance;               /**< Distance from origin (for scalar iterations) */
  int step;                   /**< Current step in iteration */
  bool stop;                  /**< Set to true to stop iteration early */
} entity_iter_ctx_t;

/* ============================================================================
 * Grid Lifecycle
 * ============================================================================ */

/**
 * @brief Create a new entity grid
 *
 * Allocates grid and all entities. Entity data pointers are initialized to NULL.
 * Spatial mappings are automatically established based on grid position.
 *
 * @param width Grid width in entities
 * @param height Grid height in entities
 * @param wrap Whether to wrap at grid edges (toroidal topology)
 * @return New entity grid, or NULL on allocation failure
 *
 * @note Uses AK24_ALLOC for memory allocation
 */
entity_grid_t *entity_grid_new(int width, int height, bool wrap);

/**
 * @brief Free an entity grid
 *
 * Frees grid and all entities. Does NOT free entity data pointers.
 *
 * @param grid Grid to free (NULL is safe)
 *
 * @note Uses AK24_FREE for memory deallocation
 * @warning User must free entity->data before calling this
 */
void entity_grid_free(entity_grid_t *grid);

/* ============================================================================
 * Entity Access
 * ============================================================================ */

/**
 * @brief Get entity at grid position
 *
 * @param grid Entity grid
 * @param x X coordinate
 * @param y Y coordinate
 * @return Entity at position, or NULL if out of bounds (when wrap is false)
 */
entity_t *entity_grid_get(entity_grid_t *grid, int x, int y);

/**
 * @brief Get entity in direction from given entity
 *
 * @param entity Source entity
 * @param dir Direction to look
 * @return Entity in that direction, or NULL if none
 */
entity_t *entity_get_neighbor(entity_t *entity, entity_direction_e dir);

/* ============================================================================
 * Iteration Functions
 * ============================================================================
 *
 * All iteration functions invoke the lambda with entity_iter_ctx_t* as
 * the invoke_args parameter. The lambda's captured_ctx is user-defined.
 *
 * Lambda signature: void fn(void *captured_ctx, void *invoke_args)
 *   - invoke_args is entity_iter_ctx_t*
 *   - Set ctx->stop = true to halt iteration early
 */

/**
 * @brief Iterate along Bresenham line from (x0,y0) to (x1,y1)
 *
 * Walks each entity along the discrete line path and invokes lambda.
 *
 * @param grid Entity grid
 * @param x0 Start x coordinate
 * @param y0 Start y coordinate
 * @param x1 End x coordinate
 * @param y1 End y coordinate
 * @param lambda Lambda to invoke for each entity on line
 */
void entity_iterate_line(entity_grid_t *grid, int x0, int y0, int x1, int y1,
                         ak_lambda_t *lambda);

/**
 * @brief Iterate immediate neighbors (4-directional: N, E, S, W)
 *
 * @param grid Entity grid
 * @param x Center x coordinate
 * @param y Center y coordinate
 * @param lambda Lambda to invoke for each neighbor
 */
void entity_iterate_immediate_neighbors(entity_grid_t *grid, int x, int y,
                                        ak_lambda_t *lambda);

/**
 * @brief Iterate self and depth layers (IN, SELF, OUT)
 *
 * @param grid Entity grid
 * @param x Entity x coordinate
 * @param y Entity y coordinate
 * @param lambda Lambda to invoke for self and depth entities
 */
void entity_iterate_for_self(entity_grid_t *grid, int x, int y,
                             ak_lambda_t *lambda);

/**
 * @brief Iterate neighbors at distance in clockwise order
 *
 * When is_filled is false, only iterates the ring at exactly 'distance'.
 * When is_filled is true, iterates all entities from distance 1 to 'distance'.
 *
 * @param grid Entity grid
 * @param x Center x coordinate
 * @param y Center y coordinate
 * @param distance Distance (1 = immediate 8 neighbors, 2 = next ring, etc.)
 * @param is_filled If true, fill all rings from 1 to distance
 * @param lambda Lambda to invoke for each entity
 */
void entity_iterate_scalar_clockwise(entity_grid_t *grid, int x, int y,
                                     int distance, bool is_filled,
                                     ak_lambda_t *lambda);

/**
 * @brief Iterate neighbors at distance in counter-clockwise order
 *
 * When is_filled is false, only iterates the ring at exactly 'distance'.
 * When is_filled is true, iterates all entities from distance 1 to 'distance'.
 *
 * @param grid Entity grid
 * @param x Center x coordinate
 * @param y Center y coordinate
 * @param distance Distance (1 = immediate 8 neighbors, 2 = next ring, etc.)
 * @param is_filled If true, fill all rings from 1 to distance
 * @param lambda Lambda to invoke for each entity
 */
void entity_iterate_scalar_counter_clockwise(entity_grid_t *grid, int x, int y,
                                             int distance, bool is_filled,
                                             ak_lambda_t *lambda);

/**
 * @brief Iterate all entities in grid
 *
 * Iterates row by row, left to right, top to bottom.
 *
 * @param grid Entity grid
 * @param lambda Lambda to invoke for each entity
 */
void entity_iterate_all(entity_grid_t *grid, ak_lambda_t *lambda);

/**
 * @brief Get opposite direction
 *
 * @param dir Direction
 * @return Opposite direction (e.g., NORTH -> SOUTH)
 */
entity_direction_e entity_direction_opposite(entity_direction_e dir);

/**
 * @brief Get direction name as string
 *
 * @param dir Direction
 * @return Static string name of direction
 */
const char *entity_direction_name(entity_direction_e dir);

#endif
