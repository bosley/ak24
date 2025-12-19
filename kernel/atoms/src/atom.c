#include "atom.h"
#include "kernel.h"
#include <string.h>

ak_atom_t *ak_atom_new(ak_atom_type_e type, ak_atom_value_u value,
                       size_t dimensionality) {
  ak_atom_t *atom = AK24_ALLOC(sizeof(ak_atom_t));
  if (!atom) {
    return NULL;
  }

  atomic_init(&atom->type, type);
  atomic_init(&atom->dimensionality, dimensionality);
  atomic_init(&atom->neighbors, NULL);

  switch (type) {
  case AK24_ATOM_BYTE:
    atomic_init(&atom->value.byte, value.byte);
    break;
  case AK24_ATOM_U8:
    atomic_init(&atom->value.u8, value.u8);
    break;
  case AK24_ATOM_U16:
    atomic_init(&atom->value.u16, value.u16);
    break;
  case AK24_ATOM_U32:
    atomic_init(&atom->value.u32, value.u32);
    break;
  case AK24_ATOM_U64:
    atomic_init(&atom->value.u64, value.u64);
    break;
  case AK24_ATOM_I8:
    atomic_init(&atom->value.i8, value.i8);
    break;
  case AK24_ATOM_I16:
    atomic_init(&atom->value.i16, value.i16);
    break;
  case AK24_ATOM_I32:
    atomic_init(&atom->value.i32, value.i32);
    break;
  case AK24_ATOM_I64:
    atomic_init(&atom->value.i64, value.i64);
    break;
  case AK24_ATOM_F32:
    atomic_init(&atom->value.f32, value.f32);
    break;
  case AK24_ATOM_F64:
    atomic_init(&atom->value.f64, value.f64);
    break;
  case AK24_ATOM_CHAR:
    atomic_init(&atom->value.c, value.c);
    break;
  case AK24_ATOM_CLUSTER:
    atom->value.cluster = value.cluster;
    break;
  }

  return atom;
}

void ak_atom_free(ak_atom_t *atom) {
  if (!atom) {
    return;
  }

  ak_atom_neighbor_t *neighbor =
      atomic_load_explicit(&atom->neighbors, memory_order_acquire);
  while (neighbor) {
    ak_atom_neighbor_t *next =
        atomic_load_explicit(&neighbor->next, memory_order_acquire);
    AK24_FREE(neighbor);
    neighbor = next;
  }

  AK24_FREE(atom);
}

ak_atom_value_u ak_atom_get_value(ak_atom_t *atom) {
  ak_atom_value_u result = {0};
  if (!atom) {
    return result;
  }

  ak_atom_type_e type = atomic_load_explicit(&atom->type, memory_order_acquire);

  switch (type) {
  case AK24_ATOM_BYTE:
    result.byte = atomic_load_explicit(&atom->value.byte, memory_order_acquire);
    break;
  case AK24_ATOM_U8:
    result.u8 = atomic_load_explicit(&atom->value.u8, memory_order_acquire);
    break;
  case AK24_ATOM_U16:
    result.u16 = atomic_load_explicit(&atom->value.u16, memory_order_acquire);
    break;
  case AK24_ATOM_U32:
    result.u32 = atomic_load_explicit(&atom->value.u32, memory_order_acquire);
    break;
  case AK24_ATOM_U64:
    result.u64 = atomic_load_explicit(&atom->value.u64, memory_order_acquire);
    break;
  case AK24_ATOM_I8:
    result.i8 = atomic_load_explicit(&atom->value.i8, memory_order_acquire);
    break;
  case AK24_ATOM_I16:
    result.i16 = atomic_load_explicit(&atom->value.i16, memory_order_acquire);
    break;
  case AK24_ATOM_I32:
    result.i32 = atomic_load_explicit(&atom->value.i32, memory_order_acquire);
    break;
  case AK24_ATOM_I64:
    result.i64 = atomic_load_explicit(&atom->value.i64, memory_order_acquire);
    break;
  case AK24_ATOM_F32:
    result.f32 = atomic_load_explicit(&atom->value.f32, memory_order_acquire);
    break;
  case AK24_ATOM_F64:
    result.f64 = atomic_load_explicit(&atom->value.f64, memory_order_acquire);
    break;
  case AK24_ATOM_CHAR:
    result.c = atomic_load_explicit(&atom->value.c, memory_order_acquire);
    break;
  case AK24_ATOM_CLUSTER:
    result.cluster = atom->value.cluster;
    break;
  }

  return result;
}

void ak_atom_set_value(ak_atom_t *atom, ak_atom_value_u value) {
  if (!atom) {
    return;
  }

  ak_atom_type_e type = atomic_load_explicit(&atom->type, memory_order_acquire);

  switch (type) {
  case AK24_ATOM_BYTE:
    atomic_store_explicit(&atom->value.byte, value.byte, memory_order_release);
    break;
  case AK24_ATOM_U8:
    atomic_store_explicit(&atom->value.u8, value.u8, memory_order_release);
    break;
  case AK24_ATOM_U16:
    atomic_store_explicit(&atom->value.u16, value.u16, memory_order_release);
    break;
  case AK24_ATOM_U32:
    atomic_store_explicit(&atom->value.u32, value.u32, memory_order_release);
    break;
  case AK24_ATOM_U64:
    atomic_store_explicit(&atom->value.u64, value.u64, memory_order_release);
    break;
  case AK24_ATOM_I8:
    atomic_store_explicit(&atom->value.i8, value.i8, memory_order_release);
    break;
  case AK24_ATOM_I16:
    atomic_store_explicit(&atom->value.i16, value.i16, memory_order_release);
    break;
  case AK24_ATOM_I32:
    atomic_store_explicit(&atom->value.i32, value.i32, memory_order_release);
    break;
  case AK24_ATOM_I64:
    atomic_store_explicit(&atom->value.i64, value.i64, memory_order_release);
    break;
  case AK24_ATOM_F32:
    atomic_store_explicit(&atom->value.f32, value.f32, memory_order_release);
    break;
  case AK24_ATOM_F64:
    atomic_store_explicit(&atom->value.f64, value.f64, memory_order_release);
    break;
  case AK24_ATOM_CHAR:
    atomic_store_explicit(&atom->value.c, value.c, memory_order_release);
    break;
  case AK24_ATOM_CLUSTER:
    atom->value.cluster = value.cluster;
    break;
  }
}

ak_atom_type_e ak_atom_get_type(ak_atom_t *atom) {
  if (!atom) {
    return AK24_ATOM_BYTE;
  }
  return atomic_load_explicit(&atom->type, memory_order_acquire);
}

size_t ak_atom_get_dimensionality(ak_atom_t *atom) {
  if (!atom) {
    return 0;
  }
  return atomic_load_explicit(&atom->dimensionality, memory_order_acquire);
}

int ak_atom_bond(ak_atom_t *atom1, ak_atom_t *atom2) {
  if (!atom1 || !atom2) {
    return -1;
  }

  ak_atom_neighbor_t *neighbor1 = AK24_ALLOC(sizeof(ak_atom_neighbor_t));
  ak_atom_neighbor_t *neighbor2 = AK24_ALLOC(sizeof(ak_atom_neighbor_t));

  if (!neighbor1 || !neighbor2) {
    if (neighbor1)
      AK24_FREE(neighbor1);
    if (neighbor2)
      AK24_FREE(neighbor2);
    return -1;
  }

  atomic_init(&neighbor1->atom, atom2);
  atomic_init(&neighbor2->atom, atom1);

  ak_atom_neighbor_t *old_neighbors1 =
      atomic_load_explicit(&atom1->neighbors, memory_order_acquire);
  atomic_init(&neighbor1->next, old_neighbors1);
  atomic_store_explicit(&atom1->neighbors, neighbor1, memory_order_release);

  ak_atom_neighbor_t *old_neighbors2 =
      atomic_load_explicit(&atom2->neighbors, memory_order_acquire);
  atomic_init(&neighbor2->next, old_neighbors2);
  atomic_store_explicit(&atom2->neighbors, neighbor2, memory_order_release);

  return 0;
}

int ak_atom_unbond(ak_atom_t *atom1, ak_atom_t *atom2) {
  if (!atom1 || !atom2) {
    return -1;
  }

  ak_atom_neighbor_t *prev = NULL;
  ak_atom_neighbor_t *current =
      atomic_load_explicit(&atom1->neighbors, memory_order_acquire);

  while (current) {
    ak_atom_t *neighbor_atom =
        atomic_load_explicit(&current->atom, memory_order_acquire);
    if (neighbor_atom == atom2) {
      ak_atom_neighbor_t *next =
          atomic_load_explicit(&current->next, memory_order_acquire);
      if (prev) {
        atomic_store_explicit(&prev->next, next, memory_order_release);
      } else {
        atomic_store_explicit(&atom1->neighbors, next, memory_order_release);
      }
      AK24_FREE(current);
      break;
    }
    prev = current;
    current = atomic_load_explicit(&current->next, memory_order_acquire);
  }

  prev = NULL;
  current = atomic_load_explicit(&atom2->neighbors, memory_order_acquire);

  while (current) {
    ak_atom_t *neighbor_atom =
        atomic_load_explicit(&current->atom, memory_order_acquire);
    if (neighbor_atom == atom1) {
      ak_atom_neighbor_t *next =
          atomic_load_explicit(&current->next, memory_order_acquire);
      if (prev) {
        atomic_store_explicit(&prev->next, next, memory_order_release);
      } else {
        atomic_store_explicit(&atom2->neighbors, next, memory_order_release);
      }
      AK24_FREE(current);
      break;
    }
    prev = current;
    current = atomic_load_explicit(&current->next, memory_order_acquire);
  }

  return 0;
}

static void ak_atom_neighbors_recursive(ak_atom_t *atom, size_t distance,
                                        size_t current_distance,
                                        ak_atom_cluster_t *cluster,
                                        ak_atom_t **visited,
                                        size_t *visited_count,
                                        size_t visited_capacity) {
  if (!atom || current_distance > distance) {
    return;
  }

  for (size_t i = 0; i < *visited_count; i++) {
    if (visited[i] == atom) {
      return;
    }
  }

  if (*visited_count < visited_capacity) {
    visited[(*visited_count)++] = atom;
  }

  if (current_distance == distance && current_distance > 0) {
    ak_atom_cluster_add(cluster, atom);
    return;
  }

  ak_atom_neighbor_t *neighbor =
      atomic_load_explicit(&atom->neighbors, memory_order_acquire);
  while (neighbor) {
    ak_atom_t *neighbor_atom =
        atomic_load_explicit(&neighbor->atom, memory_order_acquire);
    ak_atom_neighbors_recursive(neighbor_atom, distance, current_distance + 1,
                                cluster, visited, visited_count,
                                visited_capacity);
    neighbor = atomic_load_explicit(&neighbor->next, memory_order_acquire);
  }
}

ak_atom_cluster_t *ak_atom_neighbors(ak_atom_t *atom, size_t distance) {
  if (!atom) {
    return NULL;
  }

  ak_atom_cluster_t *cluster = ak_atom_cluster_new(16);
  if (!cluster) {
    return NULL;
  }

  if (distance == 0) {
    ak_atom_cluster_add(cluster, atom);
    return cluster;
  }

  size_t visited_capacity = 1024;
  ak_atom_t **visited = AK24_ALLOC(sizeof(ak_atom_t *) * visited_capacity);
  if (!visited) {
    ak_atom_cluster_free(cluster);
    return NULL;
  }

  size_t visited_count = 0;
  ak_atom_neighbors_recursive(atom, distance, 0, cluster, visited,
                              &visited_count, visited_capacity);

  AK24_FREE(visited);
  return cluster;
}

ak_atom_cluster_t *ak_atom_cluster_new(size_t initial_capacity) {
  ak_atom_cluster_t *cluster = AK24_ALLOC(sizeof(ak_atom_cluster_t));
  if (!cluster) {
    return NULL;
  }

  cluster->atoms = AK24_ALLOC(sizeof(ak_atom_t *) * initial_capacity);
  if (!cluster->atoms) {
    AK24_FREE(cluster);
    return NULL;
  }

  cluster->count = 0;
  cluster->capacity = initial_capacity;

  return cluster;
}

void ak_atom_cluster_free(ak_atom_cluster_t *cluster) {
  if (!cluster) {
    return;
  }

  if (cluster->atoms) {
    AK24_FREE(cluster->atoms);
  }

  AK24_FREE(cluster);
}

int ak_atom_cluster_add(ak_atom_cluster_t *cluster, ak_atom_t *atom) {
  if (!cluster || !atom) {
    return -1;
  }

  if (cluster->count >= cluster->capacity) {
    size_t new_capacity = cluster->capacity * 2;
    ak_atom_t **new_atoms =
        AK24_REALLOC(cluster->atoms, sizeof(ak_atom_t *) * new_capacity);
    if (!new_atoms) {
      return -1;
    }
    cluster->atoms = new_atoms;
    cluster->capacity = new_capacity;
  }

  cluster->atoms[cluster->count++] = atom;
  return 0;
}

ak_atom_t *ak_atom_cluster_get(ak_atom_cluster_t *cluster, size_t index) {
  if (!cluster || index >= cluster->count) {
    return NULL;
  }
  return cluster->atoms[index];
}

size_t ak_atom_cluster_count(ak_atom_cluster_t *cluster) {
  if (!cluster) {
    return 0;
  }
  return cluster->count;
}
