#include "atom.h"
#include "kernel.h"
#include "test/assert.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

static int test_atom_new_free(void) {
  ak_atom_value_u value = {.i32 = 42};
  ak_atom_t *atom = ak_atom_new(AK24_ATOM_I32, value, 3);
  AK24_TEST_ASSERT_NOT_NULL(atom);
  AK24_TEST_ASSERT_EQ(ak_atom_get_dimensionality(atom), 3);
  ak_atom_free(atom);
  AK24_TEST_PASS();
}

static int test_atom_get_set_value(void) {
  ak_atom_value_u value = {.i32 = -42};
  ak_atom_t *atom = ak_atom_new(AK24_ATOM_I32, value, 1);
  AK24_TEST_ASSERT_NOT_NULL(atom);

  ak_atom_value_u retrieved = ak_atom_get_value(atom);
  AK24_TEST_ASSERT_EQ(retrieved.i32, -42);

  ak_atom_value_u new_value = {.i32 = 100};
  ak_atom_set_value(atom, new_value);

  retrieved = ak_atom_get_value(atom);
  AK24_TEST_ASSERT_EQ(retrieved.i32, 100);

  ak_atom_free(atom);
  AK24_TEST_PASS();
}

static int test_atom_bond_unbond(void) {
  ak_atom_value_u v1 = {.i32 = 1};
  ak_atom_value_u v2 = {.i32 = 2};

  ak_atom_t *atom1 = ak_atom_new(AK24_ATOM_I32, v1, 3);
  ak_atom_t *atom2 = ak_atom_new(AK24_ATOM_I32, v2, 3);

  AK24_TEST_ASSERT_NOT_NULL(atom1);
  AK24_TEST_ASSERT_NOT_NULL(atom2);

  int result = ak_atom_bond(atom1, atom2);
  AK24_TEST_ASSERT_EQ(result, 0);

  ak_atom_cluster_t *neighbors = ak_atom_neighbors(atom1, 1);
  AK24_TEST_ASSERT_NOT_NULL(neighbors);
  AK24_TEST_ASSERT_EQ(ak_atom_cluster_count(neighbors), 1);
  AK24_TEST_ASSERT_EQ(ak_atom_cluster_get(neighbors, 0), atom2);
  ak_atom_cluster_free(neighbors);

  result = ak_atom_unbond(atom1, atom2);
  AK24_TEST_ASSERT_EQ(result, 0);

  neighbors = ak_atom_neighbors(atom1, 1);
  AK24_TEST_ASSERT_NOT_NULL(neighbors);
  AK24_TEST_ASSERT_EQ(ak_atom_cluster_count(neighbors), 0);
  ak_atom_cluster_free(neighbors);

  ak_atom_free(atom1);
  ak_atom_free(atom2);
  AK24_TEST_PASS();
}

static int test_atom_1d_chain(void) {
  ak_atom_value_u v1 = {.i32 = 10};
  ak_atom_value_u v2 = {.i32 = 20};
  ak_atom_value_u v3 = {.i32 = 30};

  ak_atom_t *atom1 = ak_atom_new(AK24_ATOM_I32, v1, 1);
  ak_atom_t *atom2 = ak_atom_new(AK24_ATOM_I32, v2, 1);
  ak_atom_t *atom3 = ak_atom_new(AK24_ATOM_I32, v3, 1);

  ak_atom_bond(atom1, atom2);
  ak_atom_bond(atom2, atom3);

  ak_atom_cluster_t *neighbors1 = ak_atom_neighbors(atom1, 1);
  AK24_TEST_ASSERT_EQ(ak_atom_cluster_count(neighbors1), 1);
  ak_atom_cluster_free(neighbors1);

  ak_atom_cluster_t *neighbors2 = ak_atom_neighbors(atom2, 1);
  AK24_TEST_ASSERT_EQ(ak_atom_cluster_count(neighbors2), 2);
  ak_atom_cluster_free(neighbors2);

  ak_atom_free(atom1);
  ak_atom_free(atom2);
  ak_atom_free(atom3);
  AK24_TEST_PASS();
}

static int test_atom_2d_grid(void) {
  ak_atom_value_u val = {.i32 = 0};

  ak_atom_t *grid[3][3];
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      val.i32 = i * 3 + j;
      grid[i][j] = ak_atom_new(AK24_ATOM_I32, val, 2);
    }
  }

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (i < 2)
        ak_atom_bond(grid[i][j], grid[i + 1][j]);
      if (j < 2)
        ak_atom_bond(grid[i][j], grid[i][j + 1]);
    }
  }

  ak_atom_cluster_t *center_neighbors = ak_atom_neighbors(grid[1][1], 1);
  AK24_TEST_ASSERT_EQ(ak_atom_cluster_count(center_neighbors), 4);
  ak_atom_cluster_free(center_neighbors);

  ak_atom_cluster_t *corner_neighbors = ak_atom_neighbors(grid[0][0], 1);
  AK24_TEST_ASSERT_EQ(ak_atom_cluster_count(corner_neighbors), 2);
  ak_atom_cluster_free(corner_neighbors);

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      ak_atom_free(grid[i][j]);
    }
  }
  AK24_TEST_PASS();
}

static int test_atom_3d_cube(void) {
  ak_atom_value_u val = {.i32 = 0};

  ak_atom_t *cube[2][2][2];
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 2; k++) {
        val.i32 = i * 4 + j * 2 + k;
        cube[i][j][k] = ak_atom_new(AK24_ATOM_I32, val, 3);
      }
    }
  }

  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 2; k++) {
        if (i < 1)
          ak_atom_bond(cube[i][j][k], cube[i + 1][j][k]);
        if (j < 1)
          ak_atom_bond(cube[i][j][k], cube[i][j + 1][k]);
        if (k < 1)
          ak_atom_bond(cube[i][j][k], cube[i][j][k + 1]);
      }
    }
  }

  ak_atom_cluster_t *neighbors = ak_atom_neighbors(cube[0][0][0], 1);
  AK24_TEST_ASSERT_EQ(ak_atom_cluster_count(neighbors), 3);
  ak_atom_cluster_free(neighbors);

  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 2; k++) {
        ak_atom_free(cube[i][j][k]);
      }
    }
  }
  AK24_TEST_PASS();
}

static int test_atom_distance_queries(void) {
  ak_atom_value_u val = {.i32 = 0};

  ak_atom_t *atoms[5];
  for (int i = 0; i < 5; i++) {
    val.i32 = i;
    atoms[i] = ak_atom_new(AK24_ATOM_I32, val, 1);
  }

  for (int i = 0; i < 4; i++) {
    ak_atom_bond(atoms[i], atoms[i + 1]);
  }

  ak_atom_cluster_t *dist0 = ak_atom_neighbors(atoms[2], 0);
  AK24_TEST_ASSERT_EQ(ak_atom_cluster_count(dist0), 1);
  ak_atom_cluster_free(dist0);

  ak_atom_cluster_t *dist1 = ak_atom_neighbors(atoms[2], 1);
  AK24_TEST_ASSERT_EQ(ak_atom_cluster_count(dist1), 2);
  ak_atom_cluster_free(dist1);

  ak_atom_cluster_t *dist2 = ak_atom_neighbors(atoms[2], 2);
  AK24_TEST_ASSERT_EQ(ak_atom_cluster_count(dist2), 2);
  ak_atom_cluster_free(dist2);

  for (int i = 0; i < 5; i++) {
    ak_atom_free(atoms[i]);
  }
  AK24_TEST_PASS();
}

static int test_atom_cluster_operations(void) {
  ak_atom_cluster_t *cluster = ak_atom_cluster_new(2);
  AK24_TEST_ASSERT_NOT_NULL(cluster);
  AK24_TEST_ASSERT_EQ(ak_atom_cluster_count(cluster), 0);

  ak_atom_value_u val = {.i32 = 42};
  ak_atom_t *atom1 = ak_atom_new(AK24_ATOM_I32, val, 1);
  ak_atom_t *atom2 = ak_atom_new(AK24_ATOM_I32, val, 1);
  ak_atom_t *atom3 = ak_atom_new(AK24_ATOM_I32, val, 1);

  AK24_TEST_ASSERT_EQ(ak_atom_cluster_add(cluster, atom1), 0);
  AK24_TEST_ASSERT_EQ(ak_atom_cluster_add(cluster, atom2), 0);
  AK24_TEST_ASSERT_EQ(ak_atom_cluster_add(cluster, atom3), 0);

  AK24_TEST_ASSERT_EQ(ak_atom_cluster_count(cluster), 3);
  AK24_TEST_ASSERT_EQ(ak_atom_cluster_get(cluster, 0), atom1);
  AK24_TEST_ASSERT_EQ(ak_atom_cluster_get(cluster, 1), atom2);
  AK24_TEST_ASSERT_EQ(ak_atom_cluster_get(cluster, 2), atom3);

  ak_atom_free(atom1);
  ak_atom_free(atom2);
  ak_atom_free(atom3);
  ak_atom_cluster_free(cluster);
  AK24_TEST_PASS();
}

static int test_atom_null_handling(void) {
  ak_atom_value_u value = {.i32 = 42};

  ak_atom_free(NULL);

  ak_atom_value_u result = ak_atom_get_value(NULL);
  AK24_TEST_ASSERT_EQ(result.i32, 0);

  ak_atom_set_value(NULL, value);

  ak_atom_type_e type = ak_atom_get_type(NULL);
  AK24_TEST_ASSERT_EQ(type, AK24_ATOM_BYTE);

  size_t dim = ak_atom_get_dimensionality(NULL);
  AK24_TEST_ASSERT_EQ(dim, 0);

  int bond_result = ak_atom_bond(NULL, NULL);
  AK24_TEST_ASSERT_EQ(bond_result, -1);

  int unbond_result = ak_atom_unbond(NULL, NULL);
  AK24_TEST_ASSERT_EQ(unbond_result, -1);

  ak_atom_cluster_t *neighbors = ak_atom_neighbors(NULL, 1);
  AK24_TEST_ASSERT_NULL(neighbors);

  ak_atom_cluster_free(NULL);

  AK24_TEST_PASS();
}

typedef struct {
  ak_atom_t *atom;
  int thread_id;
  int iterations;
} thread_data_t;

static void *writer_thread(void *arg) {
  thread_data_t *data = (thread_data_t *)arg;

  for (int i = 0; i < data->iterations; i++) {
    ak_atom_value_u value = {.i32 = data->thread_id * 1000 + i};
    ak_atom_set_value(data->atom, value);
    usleep(1);
  }

  return NULL;
}

static void *reader_thread(void *arg) {
  thread_data_t *data = (thread_data_t *)arg;
  int reads = 0;

  while (reads < data->iterations) {
    ak_atom_value_u value = ak_atom_get_value(data->atom);
    (void)value;
    reads++;
    usleep(1);
  }

  return NULL;
}

static int test_atom_concurrent_access(void) {
  ak_atom_value_u value = {.i32 = 0};
  ak_atom_t *atom = ak_atom_new(AK24_ATOM_I32, value, 3);
  AK24_TEST_ASSERT_NOT_NULL(atom);

#define NUM_WRITERS 2
#define NUM_READERS 2
#define ITERATIONS 50

  AK_THREAD writers[NUM_WRITERS];
  AK_THREAD readers[NUM_READERS];
  thread_data_t writer_data[NUM_WRITERS];
  thread_data_t reader_data[NUM_READERS];

  for (int i = 0; i < NUM_WRITERS; i++) {
    writer_data[i].atom = atom;
    writer_data[i].thread_id = i;
    writer_data[i].iterations = ITERATIONS;
    AK24_THREAD_CREATE(&writers[i], NULL, writer_thread, &writer_data[i]);
  }

  for (int i = 0; i < NUM_READERS; i++) {
    reader_data[i].atom = atom;
    reader_data[i].thread_id = i;
    reader_data[i].iterations = ITERATIONS;
    AK24_THREAD_CREATE(&readers[i], NULL, reader_thread, &reader_data[i]);
  }

  for (int i = 0; i < NUM_WRITERS; i++) {
    AK24_THREAD_JOIN(writers[i], NULL);
  }

  for (int i = 0; i < NUM_READERS; i++) {
    AK24_THREAD_JOIN(readers[i], NULL);
  }

#undef NUM_WRITERS
#undef NUM_READERS
#undef ITERATIONS

  ak_atom_free(atom);
  AK24_TEST_PASS();
}

int run_atom_tests(void) {
  AK24_TEST_RUN(test_atom_new_free);
  AK24_TEST_RUN(test_atom_get_set_value);
  AK24_TEST_RUN(test_atom_bond_unbond);
  AK24_TEST_RUN(test_atom_1d_chain);
  AK24_TEST_RUN(test_atom_2d_grid);
  AK24_TEST_RUN(test_atom_3d_cube);
  AK24_TEST_RUN(test_atom_distance_queries);
  AK24_TEST_RUN(test_atom_cluster_operations);
  AK24_TEST_RUN(test_atom_null_handling);
  AK24_TEST_RUN(test_atom_concurrent_access);
  return 0;
}

int main(void) {
  ak_kernel_init();
  int result = run_atom_tests();
  ak_kernel_deinit();
  return result;
}
