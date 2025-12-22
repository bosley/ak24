/**
 * @file entity.c
 * @brief Spatial entity grid system implementation
 *
 * Implements a 2D grid of entities with directional spatial mappings
 * and lambda-based iteration primitives.
 */

#include "entity.h"
#include "kernel/kernel.h"
#include <stdlib.h>

/* ============================================================================
 * Internal Helpers
 * ============================================================================ */

/**
 * @brief Wrap coordinate to grid bounds
 */
static int wrap_coord(int coord, int max, bool wrap) {
  if (!wrap) {
    return coord;
  }
  while (coord < 0) {
    coord += max;
  }
  return coord % max;
}

/**
 * @brief Get direction offsets
 */
static void get_direction_offset(entity_direction_e dir, int *dx, int *dy) {
  *dx = 0;
  *dy = 0;

  switch (dir) {
  case ENTITY_DIRECTION_NORTH:
    *dy = -1;
    break;
  case ENTITY_DIRECTION_NORTH_EAST:
    *dx = 1;
    *dy = -1;
    break;
  case ENTITY_DIRECTION_EAST:
    *dx = 1;
    break;
  case ENTITY_DIRECTION_SOUTH_EAST:
    *dx = 1;
    *dy = 1;
    break;
  case ENTITY_DIRECTION_SOUTH:
    *dy = 1;
    break;
  case ENTITY_DIRECTION_SOUTH_WEST:
    *dx = -1;
    *dy = 1;
    break;
  case ENTITY_DIRECTION_WEST:
    *dx = -1;
    break;
  case ENTITY_DIRECTION_NORTH_WEST:
    *dx = -1;
    *dy = -1;
    break;
  default:
    break;
  }
}

/**
 * @brief Integer absolute value
 */
static int iabs(int x) { return x < 0 ? -x : x; }

/* ============================================================================
 * Grid Lifecycle
 * ============================================================================ */

entity_grid_t *entity_grid_new(int width, int height, bool wrap) {
  if (width <= 0 || height <= 0) {
    return NULL;
  }

  entity_grid_t *grid = (entity_grid_t *)AK24_ALLOC(sizeof(entity_grid_t));
  if (!grid) {
    return NULL;
  }

  grid->width = width;
  grid->height = height;
  grid->wrap = wrap;
  grid->next_id = 0;

  size_t total = (size_t)width * (size_t)height;
  grid->entities = (entity_t **)AK24_ALLOC(sizeof(entity_t *) * total);
  if (!grid->entities) {
    AK24_FREE(grid);
    return NULL;
  }

  /* Allocate all entities */
  for (size_t i = 0; i < total; i++) {
    entity_t *e = (entity_t *)AK24_ALLOC(sizeof(entity_t));
    if (!e) {
      /* Cleanup on failure */
      for (size_t j = 0; j < i; j++) {
        AK24_FREE(grid->entities[j]);
      }
      AK24_FREE(grid->entities);
      AK24_FREE(grid);
      return NULL;
    }

    e->unique_id = grid->next_id++;
    e->x = (int)(i % (size_t)width);
    e->y = (int)(i / (size_t)width);
    e->data = NULL;

    /* Initialize spatial mappings to NULL */
    for (int d = 0; d < ENTITY_DIRECTION_COUNT; d++) {
      e->spatial_mapping[d] = NULL;
    }

    grid->entities[i] = e;
  }

  /* Establish spatial mappings */
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      entity_t *e = grid->entities[y * width + x];

      /* Self references for IN direction */
      e->spatial_mapping[ENTITY_DIRECTION_IN] = e;

      /* Cardinal and diagonal directions */
      for (entity_direction_e dir = ENTITY_DIRECTION_NORTH;
           dir <= ENTITY_DIRECTION_NORTH_WEST; dir++) {
        int dx, dy;
        get_direction_offset(dir, &dx, &dy);

        int nx = x + dx;
        int ny = y + dy;

        if (wrap) {
          nx = wrap_coord(nx, width, true);
          ny = wrap_coord(ny, height, true);
          e->spatial_mapping[dir] = grid->entities[ny * width + nx];
        } else {
          if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
            e->spatial_mapping[dir] = grid->entities[ny * width + nx];
          } else {
            e->spatial_mapping[dir] = NULL;
          }
        }
      }

      /* OUT direction - user can set this for layered/depth access */
      e->spatial_mapping[ENTITY_DIRECTION_OUT] = NULL;
    }
  }

  return grid;
}

void entity_grid_free(entity_grid_t *grid) {
  if (!grid) {
    return;
  }

  if (grid->entities) {
    size_t total = (size_t)grid->width * (size_t)grid->height;
    for (size_t i = 0; i < total; i++) {
      if (grid->entities[i]) {
        /* NOTE: We do NOT free entity->data - that's the user's responsibility
         */
        AK24_FREE(grid->entities[i]);
      }
    }
    AK24_FREE(grid->entities);
  }

  AK24_FREE(grid);
}

/* ============================================================================
 * Entity Access
 * ============================================================================ */

entity_t *entity_grid_get(entity_grid_t *grid, int x, int y) {
  if (!grid) {
    return NULL;
  }

  if (grid->wrap) {
    x = wrap_coord(x, grid->width, true);
    y = wrap_coord(y, grid->height, true);
  } else {
    if (x < 0 || x >= grid->width || y < 0 || y >= grid->height) {
      return NULL;
    }
  }

  return grid->entities[y * grid->width + x];
}

entity_t *entity_get_neighbor(entity_t *entity, entity_direction_e dir) {
  if (!entity || dir < 0 || dir >= ENTITY_DIRECTION_COUNT) {
    return NULL;
  }
  return entity->spatial_mapping[dir];
}

/* ============================================================================
 * Direction Utilities
 * ============================================================================ */

entity_direction_e entity_direction_opposite(entity_direction_e dir) {
  switch (dir) {
  case ENTITY_DIRECTION_NORTH:
    return ENTITY_DIRECTION_SOUTH;
  case ENTITY_DIRECTION_NORTH_EAST:
    return ENTITY_DIRECTION_SOUTH_WEST;
  case ENTITY_DIRECTION_EAST:
    return ENTITY_DIRECTION_WEST;
  case ENTITY_DIRECTION_SOUTH_EAST:
    return ENTITY_DIRECTION_NORTH_WEST;
  case ENTITY_DIRECTION_SOUTH:
    return ENTITY_DIRECTION_NORTH;
  case ENTITY_DIRECTION_SOUTH_WEST:
    return ENTITY_DIRECTION_NORTH_EAST;
  case ENTITY_DIRECTION_WEST:
    return ENTITY_DIRECTION_EAST;
  case ENTITY_DIRECTION_NORTH_WEST:
    return ENTITY_DIRECTION_SOUTH_EAST;
  case ENTITY_DIRECTION_IN:
    return ENTITY_DIRECTION_OUT;
  case ENTITY_DIRECTION_OUT:
    return ENTITY_DIRECTION_IN;
  default:
    return dir;
  }
}

const char *entity_direction_name(entity_direction_e dir) {
  switch (dir) {
  case ENTITY_DIRECTION_IN:
    return "IN";
  case ENTITY_DIRECTION_NORTH:
    return "NORTH";
  case ENTITY_DIRECTION_NORTH_EAST:
    return "NORTH_EAST";
  case ENTITY_DIRECTION_EAST:
    return "EAST";
  case ENTITY_DIRECTION_SOUTH_EAST:
    return "SOUTH_EAST";
  case ENTITY_DIRECTION_SOUTH:
    return "SOUTH";
  case ENTITY_DIRECTION_SOUTH_WEST:
    return "SOUTH_WEST";
  case ENTITY_DIRECTION_WEST:
    return "WEST";
  case ENTITY_DIRECTION_NORTH_WEST:
    return "NORTH_WEST";
  case ENTITY_DIRECTION_OUT:
    return "OUT";
  default:
    return "UNKNOWN";
  }
}

/* ============================================================================
 * Iteration Functions
 * ============================================================================ */

void entity_iterate_line(entity_grid_t *grid, int x0, int y0, int x1, int y1,
                         ak_lambda_t *lambda) {
  if (!grid || !lambda) {
    return;
  }

  /* Bresenham's line algorithm */
  int dx = iabs(x1 - x0);
  int dy = -iabs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1;
  int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;

  int x = x0;
  int y = y0;
  int step = 0;

  entity_iter_ctx_t ctx = {0};

  while (1) {
    entity_t *e = entity_grid_get(grid, x, y);
    if (e) {
      ctx.entity = e;
      ctx.x = x;
      ctx.y = y;
      ctx.from_dir = ENTITY_DIRECTION_IN;
      ctx.distance = 0;
      ctx.step = step++;
      ctx.stop = false;

      ak_lambda_invoke(lambda, &ctx);

      if (ctx.stop) {
        return;
      }
    }

    if (x == x1 && y == y1) {
      break;
    }

    int e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y += sy;
    }
  }
}

void entity_iterate_immediate_neighbors(entity_grid_t *grid, int x, int y,
                                        ak_lambda_t *lambda) {
  if (!grid || !lambda) {
    return;
  }

  entity_t *center = entity_grid_get(grid, x, y);
  if (!center) {
    return;
  }

  /* Only 4 cardinal directions: N, E, S, W */
  entity_direction_e dirs[] = {ENTITY_DIRECTION_NORTH, ENTITY_DIRECTION_EAST,
                               ENTITY_DIRECTION_SOUTH, ENTITY_DIRECTION_WEST};

  entity_iter_ctx_t ctx = {0};
  int step = 0;

  for (int i = 0; i < 4; i++) {
    entity_t *neighbor = entity_get_neighbor(center, dirs[i]);
    if (neighbor) {
      ctx.entity = neighbor;
      ctx.x = neighbor->x;
      ctx.y = neighbor->y;
      ctx.from_dir = entity_direction_opposite(dirs[i]);
      ctx.distance = 1;
      ctx.step = step++;
      ctx.stop = false;

      ak_lambda_invoke(lambda, &ctx);

      if (ctx.stop) {
        return;
      }
    }
  }
}

void entity_iterate_for_self(entity_grid_t *grid, int x, int y,
                             ak_lambda_t *lambda) {
  if (!grid || !lambda) {
    return;
  }

  entity_t *self = entity_grid_get(grid, x, y);
  if (!self) {
    return;
  }

  entity_iter_ctx_t ctx = {0};
  int step = 0;

  /* IN (depth inward) */
  entity_t *in_entity = entity_get_neighbor(self, ENTITY_DIRECTION_IN);
  if (in_entity) {
    ctx.entity = in_entity;
    ctx.x = x;
    ctx.y = y;
    ctx.from_dir = ENTITY_DIRECTION_OUT;
    ctx.distance = 0;
    ctx.step = step++;
    ctx.stop = false;

    ak_lambda_invoke(lambda, &ctx);
    if (ctx.stop) {
      return;
    }
  }

  /* SELF (the entity itself, different from IN which may point elsewhere) */
  ctx.entity = self;
  ctx.x = x;
  ctx.y = y;
  ctx.from_dir = ENTITY_DIRECTION_IN;
  ctx.distance = 0;
  ctx.step = step++;
  ctx.stop = false;

  ak_lambda_invoke(lambda, &ctx);
  if (ctx.stop) {
    return;
  }

  /* OUT (depth outward) */
  entity_t *out_entity = entity_get_neighbor(self, ENTITY_DIRECTION_OUT);
  if (out_entity) {
    ctx.entity = out_entity;
    ctx.x = x;
    ctx.y = y;
    ctx.from_dir = ENTITY_DIRECTION_IN;
    ctx.distance = 0;
    ctx.step = step++;
    ctx.stop = false;

    ak_lambda_invoke(lambda, &ctx);
  }
}

/**
 * @brief Iterate a single ring at given distance in specified order
 *
 * @param grid Entity grid
 * @param cx Center x
 * @param cy Center y
 * @param distance Ring distance
 * @param clockwise Direction of iteration
 * @param lambda Lambda to invoke
 * @param step_counter Current step counter (updated)
 * @return true if should continue, false if stopped
 */
static bool iterate_ring(entity_grid_t *grid, int cx, int cy, int distance,
                         bool clockwise, ak_lambda_t *lambda,
                         int *step_counter) {
  if (distance <= 0) {
    return true;
  }

  entity_iter_ctx_t ctx = {0};

  /*
   * Ring iteration: walk around the perimeter at given distance.
   * Starting from top-left corner of the ring.
   *
   * For distance d, the ring covers:
   * - Top edge: (cx-d, cy-d) to (cx+d, cy-d)
   * - Right edge: (cx+d, cy-d+1) to (cx+d, cy+d)
   * - Bottom edge: (cx+d-1, cy+d) to (cx-d, cy+d)
   * - Left edge: (cx-d, cy+d-1) to (cx-d, cy-d+1)
   */

  int d = distance;
  int ring_size = 8 * d; /* Perimeter of ring at distance d */

  /* Build array of positions in clockwise order */
  int *positions_x = (int *)AK24_ALLOC(sizeof(int) * (size_t)ring_size);
  int *positions_y = (int *)AK24_ALLOC(sizeof(int) * (size_t)ring_size);
  if (!positions_x || !positions_y) {
    if (positions_x)
      AK24_FREE(positions_x);
    if (positions_y)
      AK24_FREE(positions_y);
    return true;
  }

  int pos = 0;

  /* Top edge: left to right */
  for (int x = cx - d; x <= cx + d; x++) {
    positions_x[pos] = x;
    positions_y[pos] = cy - d;
    pos++;
  }

  /* Right edge: top+1 to bottom */
  for (int y = cy - d + 1; y <= cy + d; y++) {
    positions_x[pos] = cx + d;
    positions_y[pos] = y;
    pos++;
  }

  /* Bottom edge: right-1 to left */
  for (int x = cx + d - 1; x >= cx - d; x--) {
    positions_x[pos] = x;
    positions_y[pos] = cy + d;
    pos++;
  }

  /* Left edge: bottom-1 to top+1 */
  for (int y = cy + d - 1; y >= cy - d + 1; y--) {
    positions_x[pos] = cx - d;
    positions_y[pos] = y;
    pos++;
  }

  /* Iterate in specified order */
  int start = 0;
  int end = pos;
  int step_dir = 1;

  if (!clockwise) {
    start = pos - 1;
    end = -1;
    step_dir = -1;
  }

  for (int i = start; i != end; i += step_dir) {
    int px = positions_x[i];
    int py = positions_y[i];

    entity_t *e = entity_grid_get(grid, px, py);
    if (e) {
      /* Determine which direction we came from relative to center */
      int dx = px - cx;
      int dy = py - cy;
      entity_direction_e from_dir = ENTITY_DIRECTION_IN;

      if (dy < 0 && dx == 0)
        from_dir = ENTITY_DIRECTION_SOUTH;
      else if (dy < 0 && dx > 0)
        from_dir = ENTITY_DIRECTION_SOUTH_WEST;
      else if (dy == 0 && dx > 0)
        from_dir = ENTITY_DIRECTION_WEST;
      else if (dy > 0 && dx > 0)
        from_dir = ENTITY_DIRECTION_NORTH_WEST;
      else if (dy > 0 && dx == 0)
        from_dir = ENTITY_DIRECTION_NORTH;
      else if (dy > 0 && dx < 0)
        from_dir = ENTITY_DIRECTION_NORTH_EAST;
      else if (dy == 0 && dx < 0)
        from_dir = ENTITY_DIRECTION_EAST;
      else if (dy < 0 && dx < 0)
        from_dir = ENTITY_DIRECTION_SOUTH_EAST;

      ctx.entity = e;
      ctx.x = px;
      ctx.y = py;
      ctx.from_dir = from_dir;
      ctx.distance = distance;
      ctx.step = (*step_counter)++;
      ctx.stop = false;

      ak_lambda_invoke(lambda, &ctx);

      if (ctx.stop) {
        AK24_FREE(positions_x);
        AK24_FREE(positions_y);
        return false;
      }
    }
  }

  AK24_FREE(positions_x);
  AK24_FREE(positions_y);
  return true;
}

void entity_iterate_scalar_clockwise(entity_grid_t *grid, int x, int y,
                                     int distance, bool is_filled,
                                     ak_lambda_t *lambda) {
  if (!grid || !lambda || distance <= 0) {
    return;
  }

  int step = 0;

  if (is_filled) {
    /* Iterate all rings from 1 to distance */
    for (int d = 1; d <= distance; d++) {
      if (!iterate_ring(grid, x, y, d, true, lambda, &step)) {
        return;
      }
    }
  } else {
    /* Just the outer ring */
    iterate_ring(grid, x, y, distance, true, lambda, &step);
  }
}

void entity_iterate_scalar_counter_clockwise(entity_grid_t *grid, int x, int y,
                                             int distance, bool is_filled,
                                             ak_lambda_t *lambda) {
  if (!grid || !lambda || distance <= 0) {
    return;
  }

  int step = 0;

  if (is_filled) {
    /* Iterate all rings from 1 to distance */
    for (int d = 1; d <= distance; d++) {
      if (!iterate_ring(grid, x, y, d, false, lambda, &step)) {
        return;
      }
    }
  } else {
    /* Just the outer ring */
    iterate_ring(grid, x, y, distance, false, lambda, &step);
  }
}

void entity_iterate_all(entity_grid_t *grid, ak_lambda_t *lambda) {
  if (!grid || !lambda) {
    return;
  }

  entity_iter_ctx_t ctx = {0};
  int step = 0;

  for (int y = 0; y < grid->height; y++) {
    for (int x = 0; x < grid->width; x++) {
      entity_t *e = grid->entities[y * grid->width + x];
      if (e) {
        ctx.entity = e;
        ctx.x = x;
        ctx.y = y;
        ctx.from_dir = ENTITY_DIRECTION_IN;
        ctx.distance = 0;
        ctx.step = step++;
        ctx.stop = false;

        ak_lambda_invoke(lambda, &ctx);

        if (ctx.stop) {
          return;
        }
      }
    }
  }
}
