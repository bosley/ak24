#include "kernel/application.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

APP_ON_SHUTDOWN(on_shutdown) {
  time_t uptime = time(NULL) - ctx->shutdown_info->start_time;
  printf("\nShutting down after %ld seconds\n", uptime);

#if AK24_BUILD_DEBUG_MEMORY
  printf("\n=== Memory Statistics ===\n");
  printf("Total allocations: %zu\n",
         ctx->shutdown_info->memory_stats.total_allocations);
  printf("Total frees: %zu\n", ctx->shutdown_info->memory_stats.total_frees);
  printf("Total reallocs: %zu\n",
         ctx->shutdown_info->memory_stats.total_reallocs);
  printf("Bytes allocated: %zu\n",
         ctx->shutdown_info->memory_stats.bytes_allocated);
  printf("Current bytes: %zu\n",
         ctx->shutdown_info->memory_stats.current_bytes);
  printf("Peak bytes: %zu\n", ctx->shutdown_info->memory_stats.peak_bytes);
  printf("=========================\n");
#endif
}

static void print_cube_layer(ak_atom_t *cube[3][3][3], int z) {
  printf("\n  Layer z=%d:\n", z);
  printf("  ┌─────────────┐\n");
  for (int y = 0; y < 3; y++) {
    printf("  │ ");
    for (int x = 0; x < 3; x++) {
      ak_atom_value_u val = ak_atom_get_value(cube[x][y][z]);
      printf("%3d ", val.i32);
    }
    printf("│\n");
  }
  printf("  └─────────────┘\n");
}

typedef struct {
  float x, y, z;
} vec3_t;

static void rotate_point(vec3_t *p, float angle_x, float angle_y) {
  float cos_x = cosf(angle_x);
  float sin_x = sinf(angle_x);
  float cos_y = cosf(angle_y);
  float sin_y = sinf(angle_y);

  float y = p->y * cos_x - p->z * sin_x;
  float z = p->y * sin_x + p->z * cos_x;
  p->y = y;
  p->z = z;

  float x = p->x * cos_y + p->z * sin_y;
  z = -p->x * sin_y + p->z * cos_y;
  p->x = x;
  p->z = z;
}

static void project_point(vec3_t p, int *screen_x, int *screen_y, int scale,
                          int offset_x, int offset_y) {
  *screen_x = (int)(p.x * scale) + offset_x;
  *screen_y = (int)(p.y * scale) + offset_y;
}

static void render_cube_frame_generic(ak_atom_t ****cube, int size,
                                      float angle_x, float angle_y,
                                      int num_atoms) {
  int width = 100;
  int height = 50;
  char screen[50][101];
  float depth[50][101];

  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      screen[i][j] = ' ';
      depth[i][j] = -999.0f;
    }
    screen[i][width] = '\0';
  }

  int scale = 20 / size;
  if (scale < 8)
    scale = 8;
  int offset_x = width / 2;
  int offset_y = height / 2;

  float center = (size - 1) / 2.0f;

  for (int pass = 0; pass < 2; pass++) {
    for (int x = 0; x < size; x++) {
      for (int y = 0; y < size; y++) {
        for (int z = 0; z < size; z++) {
          vec3_t p1 = {(float)(x - center), (float)(y - center),
                       (float)(z - center)};
          rotate_point(&p1, angle_x, angle_y);

          int sx1, sy1;
          project_point(p1, &sx1, &sy1, scale, offset_x, offset_y);

          if (pass == 0) {
            if (x < size - 1) {
              vec3_t p2 = {(float)(x + 1 - center), (float)(y - center),
                           (float)(z - center)};
              rotate_point(&p2, angle_x, angle_y);
              int sx2, sy2;
              project_point(p2, &sx2, &sy2, scale, offset_x, offset_y);

              int steps = abs(sx2 - sx1) > abs(sy2 - sy1) ? abs(sx2 - sx1)
                                                          : abs(sy2 - sy1);
              for (int i = 0; i <= steps; i++) {
                int ix = sx1 + (sx2 - sx1) * i / steps;
                int iy = sy1 + (sy2 - sy1) * i / steps;
                float iz = p1.z + (p2.z - p1.z) * i / steps;
                if (ix >= 0 && ix < width && iy >= 0 && iy < height) {
                  if (iz > depth[iy][ix]) {
                    screen[iy][ix] = '=';
                    depth[iy][ix] = iz;
                  }
                }
              }
            }

            if (y < size - 1) {
              vec3_t p2 = {(float)(x - center), (float)(y + 1 - center),
                           (float)(z - center)};
              rotate_point(&p2, angle_x, angle_y);
              int sx2, sy2;
              project_point(p2, &sx2, &sy2, scale, offset_x, offset_y);

              int steps = abs(sx2 - sx1) > abs(sy2 - sy1) ? abs(sx2 - sx1)
                                                          : abs(sy2 - sy1);
              for (int i = 0; i <= steps; i++) {
                int ix = sx1 + (sx2 - sx1) * i / steps;
                int iy = sy1 + (sy2 - sy1) * i / steps;
                float iz = p1.z + (p2.z - p1.z) * i / steps;
                if (ix >= 0 && ix < width && iy >= 0 && iy < height) {
                  if (iz > depth[iy][ix]) {
                    screen[iy][ix] = '|';
                    depth[iy][ix] = iz;
                  }
                }
              }
            }

            if (z < size - 1) {
              vec3_t p2 = {(float)(x - center), (float)(y - center),
                           (float)(z + 1 - center)};
              rotate_point(&p2, angle_x, angle_y);
              int sx2, sy2;
              project_point(p2, &sx2, &sy2, scale, offset_x, offset_y);

              int steps = abs(sx2 - sx1) > abs(sy2 - sy1) ? abs(sx2 - sx1)
                                                          : abs(sy2 - sy1);
              for (int i = 0; i <= steps; i++) {
                int ix = sx1 + (sx2 - sx1) * i / steps;
                int iy = sy1 + (sy2 - sy1) * i / steps;
                float iz = p1.z + (p2.z - p1.z) * i / steps;
                if (ix >= 0 && ix < width && iy >= 0 && iy < height) {
                  if (iz > depth[iy][ix]) {
                    screen[iy][ix] = '.';
                    depth[iy][ix] = iz;
                  }
                }
              }
            }
          } else {
            if (sx1 >= 2 && sx1 < width - 2 && sy1 >= 0 && sy1 < height) {
              ak_atom_t *atom = cube[x][y][z];
              ak_atom_value_u val = ak_atom_get_value(atom);
              char num[4];
              snprintf(num, sizeof(num), "%02d", val.i32);
              screen[sy1][sx1 - 2] = '[';
              screen[sy1][sx1 - 1] = num[0];
              screen[sy1][sx1] = num[1];
              screen[sy1][sx1 + 1] = ']';
              depth[sy1][sx1 - 2] = p1.z + 1.0f;
              depth[sy1][sx1 - 1] = p1.z + 1.0f;
              depth[sy1][sx1] = p1.z + 1.0f;
              depth[sy1][sx1 + 1] = p1.z + 1.0f;
            }
          }
        }
      }
    }
  }

  printf("\033[2J\033[H");
  printf("╔════════════════════════════════════════════════════════════════════"
         "══════════════════════════════╗\n");
  printf("║                                   ROTATING 3D ATOM CUBE            "
         "                              ║\n");
  printf("╚════════════════════════════════════════════════════════════════════"
         "══════════════════════════════╝\n\n");

  for (int i = 0; i < height; i++) {
    printf("  %s\n", screen[i]);
  }

  printf("\n  Atoms: %d  Dimensionality: 3  Angle X: %.2f  Angle Y: %.2f\n",
         num_atoms, angle_x, angle_y);
  printf("  [##] = atoms with values  = horizontal  | vertical  . depth\n");
}

static void pulsate_values_generic(ak_atom_t ****cube, int size,
                                   int wave_position) {
  int center = size / 2;
  for (int x = 0; x < size; x++) {
    for (int y = 0; y < size; y++) {
      for (int z = 0; z < size; z++) {
        int dist_from_center =
            abs(x - center) + abs(y - center) + abs(z - center);

        int base_value = x + y * size + z * size * size;
        int pulse = 0;

        if (dist_from_center == (wave_position % (size + 1))) {
          pulse = 50;
        }

        ak_atom_t *atom = cube[x][y][z];
        ak_atom_value_u new_val = {.i32 = base_value + pulse};
        ak_atom_set_value(atom, new_val);
      }
    }
  }
}

static void animate_rotating_cube(ak_atom_t ****cube, int size) {
  printf("\n╔═══════════════════════════════════════╗\n");
  printf("║      Rotating 3D Cube Animation      ║\n");
  printf("╚═══════════════════════════════════════╝\n\n");
  printf("Starting infinite animation...\n");
  printf("Press Ctrl+C to stop\n\n");
  sleep(2);

  float angle_x = 0.3f;
  float angle_y = 0.3f;
  int frame = 0;

  int num_atoms = size * size * size;

  while (1) {
    pulsate_values_generic(cube, size, frame / 10);

    render_cube_frame_generic(cube, size, angle_x, angle_y, num_atoms);

    angle_x += 0.03f;
    angle_y += 0.02f;

    frame++;
    usleep(50000);
  }
}

static void demonstrate_neighbors(ak_atom_t *cube[3][3][3]) {
  printf("\n╔═══════════════════════════════════════╗\n");
  printf("║      Neighbor Query Demonstrations    ║\n");
  printf("╚═══════════════════════════════════════╝\n");

  printf("\n1. Corner atom [0,0,0] neighbors:\n");
  ak_atom_cluster_t *corner_neighbors = ak_atom_neighbors(cube[0][0][0], 1);
  printf("   Distance 1: %zu neighbors\n",
         ak_atom_cluster_count(corner_neighbors));
  for (size_t i = 0; i < ak_atom_cluster_count(corner_neighbors); i++) {
    ak_atom_t *neighbor = ak_atom_cluster_get(corner_neighbors, i);
    ak_atom_value_u val = ak_atom_get_value(neighbor);
    printf("     - Atom with value: %d\n", val.i32);
  }
  ak_atom_cluster_free(corner_neighbors);

  printf("\n2. Center atom [1,1,1] neighbors:\n");
  ak_atom_cluster_t *center_neighbors = ak_atom_neighbors(cube[1][1][1], 1);
  printf("   Distance 1: %zu neighbors (6 faces)\n",
         ak_atom_cluster_count(center_neighbors));
  for (size_t i = 0; i < ak_atom_cluster_count(center_neighbors); i++) {
    ak_atom_t *neighbor = ak_atom_cluster_get(center_neighbors, i);
    ak_atom_value_u val = ak_atom_get_value(neighbor);
    printf("     - Atom with value: %d\n", val.i32);
  }
  ak_atom_cluster_free(center_neighbors);

  printf("\n3. Edge atom [1,0,0] neighbors:\n");
  ak_atom_cluster_t *edge_neighbors = ak_atom_neighbors(cube[1][0][0], 1);
  printf("   Distance 1: %zu neighbors\n",
         ak_atom_cluster_count(edge_neighbors));
  for (size_t i = 0; i < ak_atom_cluster_count(edge_neighbors); i++) {
    ak_atom_t *neighbor = ak_atom_cluster_get(edge_neighbors, i);
    ak_atom_value_u val = ak_atom_get_value(neighbor);
    printf("     - Atom with value: %d\n", val.i32);
  }
  ak_atom_cluster_free(edge_neighbors);

  printf("\n4. Distance queries from center [1,1,1]:\n");
  for (int dist = 0; dist <= 3; dist++) {
    ak_atom_cluster_t *dist_neighbors = ak_atom_neighbors(cube[1][1][1], dist);
    printf("   Distance %d: %zu atoms\n", dist,
           ak_atom_cluster_count(dist_neighbors));
    ak_atom_cluster_free(dist_neighbors);
  }
}

static void demonstrate_value_updates(ak_atom_t *cube[3][3][3]) {
  printf("\n╔═══════════════════════════════════════╗\n");
  printf("║      Atomic Value Updates Demo        ║\n");
  printf("╚═══════════════════════════════════════╝\n");

  printf("\nOriginal center atom [1,1,1] value: %d\n",
         ak_atom_get_value(cube[1][1][1]).i32);

  printf("Updating to 999...\n");
  ak_atom_set_value(cube[1][1][1], (ak_atom_value_u){.i32 = 999});

  printf("New center atom [1,1,1] value: %d\n",
         ak_atom_get_value(cube[1][1][1]).i32);

  print_cube_layer(cube, 1);

  printf("\nRestoring original value...\n");
  ak_atom_set_value(cube[1][1][1], (ak_atom_value_u){.i32 = 13});
}

APP_MAIN(app_main) {
  int size = 2;

  if (list_count(&ctx->args) > 1) {
    char **size_arg = (char **)list_get(&ctx->args, 1);
    if (size_arg && *size_arg) {
      int parsed = atoi(*size_arg);
      if (parsed >= 2 && parsed <= 10) {
        size = parsed;
      } else {
        printf("Error: Size must be between 2 and 10\n");
        printf("Usage: %s [size]\n", *(char **)list_get(&ctx->args, 0));
        return 1;
      }
    }
  }

  printf("\n╔═══════════════════════════════════════╗\n");
  printf("║   AK24 3D Atom Cube Demonstration    ║\n");
  printf("╚═══════════════════════════════════════╝\n");

  printf("\nCreating %d×%d×%d atom cube in 3D space...\n", size, size, size);

  ak_atom_t ****cube = AK24_ALLOC(sizeof(ak_atom_t ***) * size);
  for (int x = 0; x < size; x++) {
    cube[x] = AK24_ALLOC(sizeof(ak_atom_t **) * size);
    for (int y = 0; y < size; y++) {
      cube[x][y] = AK24_ALLOC(sizeof(ak_atom_t *) * size);
    }
  }

  for (int x = 0; x < size; x++) {
    for (int y = 0; y < size; y++) {
      for (int z = 0; z < size; z++) {
        int value = x + y * size + z * size * size;
        cube[x][y][z] =
            ak_atom_new(AK24_ATOM_I32, (ak_atom_value_u){.i32 = value}, 3);
      }
    }
  }

  printf("Bonding atoms in 3D lattice...\n");

  for (int x = 0; x < size; x++) {
    for (int y = 0; y < size; y++) {
      for (int z = 0; z < size; z++) {
        if (x < size - 1)
          ak_atom_bond(cube[x][y][z], cube[x + 1][y][z]);
        if (y < size - 1)
          ak_atom_bond(cube[x][y][z], cube[x][y + 1][z]);
        if (z < size - 1)
          ak_atom_bond(cube[x][y][z], cube[x][y][z + 1]);
      }
    }
  }

  printf("\n╔═══════════════════════════════════════╗\n");
  printf("║           Cube Statistics             ║\n");
  printf("╚═══════════════════════════════════════╝\n");
  printf("\nNumber of atoms: %d\n", size * size * size);
  printf("Cube dimensions: %d×%d×%d\n", size, size, size);
  printf("Dimensionality: 3\n");

  animate_rotating_cube(cube, size);

  demonstrate_neighbors(cube);

  demonstrate_value_updates(cube);

  printf("\nCleaning up...\n");
  for (int x = 0; x < size; x++) {
    for (int y = 0; y < size; y++) {
      for (int z = 0; z < size; z++) {
        ak_atom_free(cube[x][y][z]);
      }
      AK24_FREE(cube[x][y]);
    }
    AK24_FREE(cube[x]);
  }
  AK24_FREE(cube);

  printf("Done!\n");

  return 0;
}

AK24_APPLICATION(app_main, on_shutdown)
