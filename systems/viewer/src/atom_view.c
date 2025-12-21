/**
 * @file atom_view.c
 * @brief Implementation of N-Dimensional Atom Viewer System
 */

#include "atom_view.h"
#include "kernel/kernel.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ========================================================================== */
/*                           Helper Functions                                 */
/* ========================================================================== */

/**
 * @brief Apply rotation to a coordinate in a plane
 */
static void apply_rotation(double *coords, size_t axis_i, size_t axis_j,
                           double angle_radians) {
  double cos_a = cos(angle_radians);
  double sin_a = sin(angle_radians);

  double old_i = coords[axis_i];
  double old_j = coords[axis_j];

  coords[axis_i] = old_i * cos_a - old_j * sin_a;
  coords[axis_j] = old_i * sin_a + old_j * cos_a;
}

/**
 * @brief Calculate depth from non-displayed dimensions
 */
static double calculate_depth(double *coords, size_t dimensionality,
                              size_t axis_x, size_t axis_y) {
  double depth = 0.0;
  for (size_t i = 0; i < dimensionality; i++) {
    if (i != axis_x && i != axis_y) {
      depth += coords[i] * coords[i];
    }
  }
  return sqrt(depth);
}

/**
 * @brief Allocate and initialize view structure
 */
static ak_atom_view_t *allocate_view_malloc(size_t width, size_t height) {
  ak_atom_view_t *view = AK24_ALLOC(sizeof(ak_atom_view_t));
  if (!view)
    return NULL;

  view->width = width;
  view->height = height;

  // Allocate pixel array
  view->pixels = AK24_ALLOC(sizeof(ak_atom_t **) * height);
  if (!view->pixels) {
    AK24_FREE(view);
    return NULL;
  }

  for (size_t y = 0; y < height; y++) {
    view->pixels[y] = AK24_ALLOC_ATOMIC(sizeof(ak_atom_t *) * width);
    if (!view->pixels[y]) {
      for (size_t i = 0; i < y; i++) {
        AK24_FREE(view->pixels[i]);
      }
      AK24_FREE(view->pixels);
      AK24_FREE(view);
      return NULL;
    }
    memset(view->pixels[y], 0, sizeof(ak_atom_t *) * width);
  }

  // Allocate depth array
  view->depths = AK24_ALLOC(sizeof(double *) * height);
  if (!view->depths) {
    for (size_t y = 0; y < height; y++) {
      AK24_FREE(view->pixels[y]);
    }
    AK24_FREE(view->pixels);
    AK24_FREE(view);
    return NULL;
  }

  for (size_t y = 0; y < height; y++) {
    view->depths[y] = AK24_ALLOC_ATOMIC(sizeof(double) * width);
    if (!view->depths[y]) {
      for (size_t i = 0; i < y; i++) {
        AK24_FREE(view->depths[i]);
      }
      AK24_FREE(view->depths);
      for (size_t i = 0; i < height; i++) {
        AK24_FREE(view->pixels[i]);
      }
      AK24_FREE(view->pixels);
      AK24_FREE(view);
      return NULL;
    }
    // Initialize depth to infinity
    for (size_t x = 0; x < width; x++) {
      view->depths[y][x] = INFINITY;
    }
  }

  return view;
}

/**
 * @brief Allocate and initialize view structure from arena
 */
static ak_atom_view_t *allocate_view_arena(ak_arena_t *arena, size_t width,
                                           size_t height) {
  ak_atom_view_t *view = ak_arena_alloc(arena, sizeof(ak_atom_view_t));
  if (!view)
    return NULL;

  view->width = width;
  view->height = height;

  // Allocate pixel array
  view->pixels = ak_arena_alloc(arena, sizeof(ak_atom_t **) * height);
  if (!view->pixels)
    return NULL;

  for (size_t y = 0; y < height; y++) {
    view->pixels[y] = ak_arena_alloc(arena, sizeof(ak_atom_t *) * width);
    if (!view->pixels[y])
      return NULL;
    memset(view->pixels[y], 0, sizeof(ak_atom_t *) * width);
  }

  // Allocate depth array
  view->depths = ak_arena_alloc(arena, sizeof(double *) * height);
  if (!view->depths)
    return NULL;

  for (size_t y = 0; y < height; y++) {
    view->depths[y] = ak_arena_alloc(arena, sizeof(double) * width);
    if (!view->depths[y])
      return NULL;
    // Initialize depth to infinity
    for (size_t x = 0; x < width; x++) {
      view->depths[y][x] = INFINITY;
    }
  }

  return view;
}

/**
 * @brief Core projection implementation
 */
static void project_atoms_to_view(ak_atom_view_t *view,
                                  ak_atom_positioned_t *atoms, size_t num_atoms,
                                  ak_atom_view_config_t *config) {
  if (!view || !atoms || !config)
    return;

  size_t dimensionality = config->dimensionality;
  double half_width = (double)view->width / 2.0;
  double half_height = (double)view->height / 2.0;

  // Process each atom
  for (size_t i = 0; i < num_atoms; i++) {
    ak_atom_positioned_t *atom = &atoms[i];

    if (atom->dimensionality != dimensionality)
      continue;

    // Copy coordinates for rotation
    double *rotated_coords = AK24_ALLOC_ATOMIC(sizeof(double) * dimensionality);
    if (!rotated_coords)
      continue;

    memcpy(rotated_coords, atom->coords, sizeof(double) * dimensionality);

    // Apply all rotations
    for (size_t r = 0; r < config->num_rotation_pairs; r++) {
      ak_atom_view_rotation_t *rotation = &config->rotations[r];
      double angle_rad = rotation->angle * M_PI / 180.0;
      apply_rotation(rotated_coords, rotation->axis_i, rotation->axis_j,
                     angle_rad);
    }

    // Project to 2D
    double proj_x = rotated_coords[config->axis_x] * config->scale;
    double proj_y = rotated_coords[config->axis_y] * config->scale;

    // Calculate depth
    double depth = calculate_depth(rotated_coords, dimensionality,
                                   config->axis_x, config->axis_y);

    AK24_FREE(rotated_coords);

    // Convert to screen coordinates
    int screen_x = (int)(proj_x + half_width);
    int screen_y = (int)(proj_y + half_height);

    // Check bounds
    if (screen_x < 0 || screen_x >= (int)view->width || screen_y < 0 ||
        screen_y >= (int)view->height)
      continue;

    // Depth test
    if (depth < view->depths[screen_y][screen_x]) {
      view->depths[screen_y][screen_x] = depth;
      view->pixels[screen_y][screen_x] = atom->atom;
    }
  }
}

/* ========================================================================== */
/*                           View Creation and Rendering                      */
/* ========================================================================== */

ak_atom_view_t *ak_atom_project_view(ak_atom_positioned_t *atoms,
                                     size_t num_atoms,
                                     ak_atom_view_config_t *config,
                                     size_t width, size_t height) {
  if (!atoms || !config || width == 0 || height == 0)
    return NULL;

  ak_atom_view_t *view = allocate_view_malloc(width, height);
  if (!view)
    return NULL;

  project_atoms_to_view(view, atoms, num_atoms, config);

  return view;
}

ak_atom_view_t *ak_atom_project_view_arena(ak_arena_t *arena,
                                           ak_atom_positioned_t *atoms,
                                           size_t num_atoms,
                                           ak_atom_view_config_t *config,
                                           size_t width, size_t height) {
  if (!arena || !atoms || !config || width == 0 || height == 0)
    return NULL;

  ak_atom_view_t *view = allocate_view_arena(arena, width, height);
  if (!view)
    return NULL;

  project_atoms_to_view(view, atoms, num_atoms, config);

  return view;
}

ak_atom_view_t *ak_atom_project_view_parallel(ak_thread_pool_t *pool,
                                              ak_arena_t *arena,
                                              ak_atom_positioned_t *atoms,
                                              size_t num_atoms,
                                              ak_atom_view_config_t *config,
                                              size_t width, size_t height) {
  // For now, fall back to single-threaded arena version
  // TODO: Implement parallel version with thread pool
  (void)pool;
  return ak_atom_project_view_arena(arena, atoms, num_atoms, config, width,
                                    height);
}

void ak_atom_view_free(ak_atom_view_t *view) {
  if (!view)
    return;

  // Free depth arrays
  if (view->depths) {
    for (size_t y = 0; y < view->height; y++) {
      if (view->depths[y])
        AK24_FREE(view->depths[y]);
    }
    AK24_FREE(view->depths);
  }

  // Free pixel arrays
  if (view->pixels) {
    for (size_t y = 0; y < view->height; y++) {
      if (view->pixels[y])
        AK24_FREE(view->pixels[y]);
    }
    AK24_FREE(view->pixels);
  }

  AK24_FREE(view);
}

/* ========================================================================== */
/*                         View Configuration Helpers                         */
/* ========================================================================== */

ak_atom_view_config_t *ak_atom_view_config_new(size_t dimensionality) {
  if (dimensionality < 2)
    return NULL;

  ak_atom_view_config_t *config = AK24_ALLOC(sizeof(ak_atom_view_config_t));
  if (!config)
    return NULL;

  config->dimensionality = dimensionality;
  config->axis_x = 0;
  config->axis_y = 1;
  config->scale = 10.0;
  config->rotations = NULL;
  config->num_rotation_pairs = 0;

  return config;
}

void ak_atom_view_config_free(ak_atom_view_config_t *config) {
  if (!config)
    return;

  if (config->rotations)
    AK24_FREE(config->rotations);

  AK24_FREE(config);
}

void ak_atom_view_rotate(ak_atom_view_config_t *config, size_t axis_i,
                         size_t axis_j, double degrees) {
  if (!config || axis_i >= config->dimensionality ||
      axis_j >= config->dimensionality || axis_i == axis_j)
    return;

  // Check if this rotation plane already exists
  for (size_t i = 0; i < config->num_rotation_pairs; i++) {
    if ((config->rotations[i].axis_i == axis_i &&
         config->rotations[i].axis_j == axis_j) ||
        (config->rotations[i].axis_i == axis_j &&
         config->rotations[i].axis_j == axis_i)) {
      // Update existing rotation
      config->rotations[i].angle = degrees;
      return;
    }
  }

  // Add new rotation
  size_t new_size = config->num_rotation_pairs + 1;
  ak_atom_view_rotation_t *new_rotations = AK24_REALLOC(
      config->rotations, sizeof(ak_atom_view_rotation_t) * new_size);
  if (!new_rotations)
    return;

  config->rotations = new_rotations;
  config->rotations[config->num_rotation_pairs].axis_i = axis_i;
  config->rotations[config->num_rotation_pairs].axis_j = axis_j;
  config->rotations[config->num_rotation_pairs].angle = degrees;
  config->num_rotation_pairs++;
}

void ak_atom_view_set_axes(ak_atom_view_config_t *config, size_t axis_x,
                           size_t axis_y) {
  if (!config || axis_x >= config->dimensionality ||
      axis_y >= config->dimensionality || axis_x == axis_y)
    return;

  config->axis_x = axis_x;
  config->axis_y = axis_y;
}

void ak_atom_view_set_scale(ak_atom_view_config_t *config, double scale) {
  if (!config || scale <= 0.0)
    return;

  config->scale = scale;
}

/* ========================================================================== */
/*                              Utility Functions                             */
/* ========================================================================== */

ak_atom_positioned_t *ak_atom_cluster_to_positioned(ak_atom_cluster_t *cluster,
                                                    size_t dimensionality,
                                                    double spacing) {
  if (!cluster || dimensionality == 0)
    return NULL;

  size_t count = ak_atom_cluster_count(cluster);
  if (count == 0)
    return NULL;

  ak_atom_positioned_t *positioned =
      AK24_ALLOC(sizeof(ak_atom_positioned_t) * count);
  if (!positioned)
    return NULL;

  // Calculate grid dimensions based on cube root of count
  size_t grid_size = (size_t)ceil(pow((double)count, 1.0 / dimensionality));

  size_t idx = 0;
  for (size_t i = 0; i < count && idx < count; i++) {
    ak_atom_t *atom = ak_atom_cluster_get(cluster, i);
    if (!atom)
      continue;

    positioned[idx].atom = atom;
    positioned[idx].dimensionality = dimensionality;
    positioned[idx].coords = AK24_ALLOC_ATOMIC(sizeof(double) * dimensionality);
    if (!positioned[idx].coords) {
      // Clean up on failure
      for (size_t j = 0; j < idx; j++) {
        AK24_FREE(positioned[j].coords);
      }
      AK24_FREE(positioned);
      return NULL;
    }

    // Calculate grid position for each dimension
    size_t pos = idx;
    for (size_t d = 0; d < dimensionality; d++) {
      size_t coord = pos % grid_size;
      pos /= grid_size;
      positioned[idx].coords[d] = (double)coord * spacing;
    }

    idx++;
  }

  return positioned;
}

void ak_atom_positioned_free(ak_atom_positioned_t *positioned, size_t count) {
  if (!positioned)
    return;

  for (size_t i = 0; i < count; i++) {
    if (positioned[i].coords)
      AK24_FREE(positioned[i].coords);
  }

  AK24_FREE(positioned);
}
