# Atoms

The atoms module provides thread-safe, atomically mutable building blocks for N-dimensional data structures. Atoms use C11 atomics for lock-free concurrent access, following the same patterns as arbuff.

## Core Concept

Atoms are fundamental data units that exist in N-dimensional space. Unlike traditional linked lists that connect in a single dimension, atoms can bond with multiple neighbors, forming:
- 1D chains (linked lists)
- 2D grids (planar structures)
- 3D cubes (spatial structures)
- N-D lattices (hyperdimensional structures)

Each atom:
- Stores a typed primitive value (u8, u16, u32, u64, i8, i16, i32, i64, f32, f64, char, byte)
- Has a dimensionality (1, 2, 3, ... N)
- Can bond with any number of other atoms
- Supports thread-safe atomic value updates

## Architecture

### Atom Structure

Each atom contains:
- **Type**: The data type it stores
- **Dimensionality**: How many dimensions it exists in
- **Neighbors**: Linked list of bonded atoms
- **Value**: The actual data (atomically accessible)

### Neighbor Management

Atoms track neighbors via a linked list of `ak_atom_neighbor_t` structures. This allows:
- Dynamic neighbor addition/removal
- No fixed limit on connections
- Efficient traversal

### Clusters

`ak_atom_cluster_t` is a collection of atoms, used for:
- Returning query results (neighbors at distance N)
- Grouping atoms for batch operations
- Future: slicing operations

## Usage

### Creating Atoms

```c
ak_atom_value_u value = {.i32 = 42};
ak_atom_t *atom = ak_atom_new(AK24_ATOM_I32, value, 3);
```

### Bonding Atoms

```c
ak_atom_t *atom1 = ak_atom_new(AK24_ATOM_I32, (ak_atom_value_u){.i32 = 1}, 2);
ak_atom_t *atom2 = ak_atom_new(AK24_ATOM_I32, (ak_atom_value_u){.i32 = 2}, 2);

ak_atom_bond(atom1, atom2);
```

### Querying Neighbors

```c
ak_atom_cluster_t *neighbors = ak_atom_neighbors(atom, 1);
size_t count = ak_atom_cluster_count(neighbors);

for (size_t i = 0; i < count; i++) {
    ak_atom_t *neighbor = ak_atom_cluster_get(neighbors, i);
}

ak_atom_cluster_free(neighbors);
```

### Atomic Value Operations

```c
ak_atom_value_u value = ak_atom_get_value(atom);

ak_atom_value_u new_value = {.i32 = 100};
ak_atom_set_value(atom, new_value);
```

## Examples

### 1D Chain (Linked List)

```c
ak_atom_t *a1 = ak_atom_new(AK24_ATOM_I32, (ak_atom_value_u){.i32 = 10}, 1);
ak_atom_t *a2 = ak_atom_new(AK24_ATOM_I32, (ak_atom_value_u){.i32 = 20}, 1);
ak_atom_t *a3 = ak_atom_new(AK24_ATOM_I32, (ak_atom_value_u){.i32 = 30}, 1);

ak_atom_bond(a1, a2);
ak_atom_bond(a2, a3);
```

### 2D Grid

```c
ak_atom_t *grid[3][3];
for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
        grid[i][j] = ak_atom_new(AK24_ATOM_I32,
                                 (ak_atom_value_u){.i32 = i * 3 + j}, 2);
    }
}

for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
        if (i < 2) ak_atom_bond(grid[i][j], grid[i+1][j]);
        if (j < 2) ak_atom_bond(grid[i][j], grid[i][j+1]);
    }
}
```

### 3D Cube

```c
ak_atom_t *cube[2][2][2];
for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
        for (int k = 0; k < 2; k++) {
            cube[i][j][k] = ak_atom_new(AK24_ATOM_I32,
                                        (ak_atom_value_u){.i32 = i*4 + j*2 + k}, 3);
        }
    }
}

for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
        for (int k = 0; k < 2; k++) {
            if (i < 1) ak_atom_bond(cube[i][j][k], cube[i+1][j][k]);
            if (j < 1) ak_atom_bond(cube[i][j][k], cube[i][j+1][k]);
            if (k < 1) ak_atom_bond(cube[i][j][k], cube[i][j][k+1]);
        }
    }
}
```

## Distance Queries

Get all atoms at a specific scalar distance:

```c
ak_atom_cluster_t *dist0 = ak_atom_neighbors(atom, 0);
ak_atom_cluster_t *dist1 = ak_atom_neighbors(atom, 1);
ak_atom_cluster_t *dist2 = ak_atom_neighbors(atom, 2);
```

- Distance 0: The atom itself
- Distance 1: Immediate neighbors (directly bonded)
- Distance 2: Neighbors of neighbors
- Distance N: All atoms N bonds away

## Thread Safety

All atom operations use atomic primitives with proper memory ordering:
- `memory_order_acquire` for reads
- `memory_order_release` for writes
- `memory_order_acq_rel` for atomic exchanges

Multiple threads can safely:
- Read and write atom values concurrently
- Query neighbors
- Bond/unbond atoms (with care)

## API Reference

### Atom Operations

- `ak_atom_new(type, value, dimensionality)` - Create new atom
- `ak_atom_free(atom)` - Free atom and its neighbor list
- `ak_atom_get_value(atom)` - Atomically read value
- `ak_atom_set_value(atom, value)` - Atomically write value
- `ak_atom_get_type(atom)` - Get atom type
- `ak_atom_get_dimensionality(atom)` - Get dimensionality

### Bonding Operations

- `ak_atom_bond(atom1, atom2)` - Create bidirectional bond
- `ak_atom_unbond(atom1, atom2)` - Remove bidirectional bond
- `ak_atom_neighbors(atom, distance)` - Get neighbors at distance

### Cluster Operations

- `ak_atom_cluster_new(capacity)` - Create cluster
- `ak_atom_cluster_free(cluster)` - Free cluster (not atoms)
- `ak_atom_cluster_add(cluster, atom)` - Add atom to cluster
- `ak_atom_cluster_get(cluster, index)` - Get atom by index
- `ak_atom_cluster_count(cluster)` - Get atom count
