# Atom Visualization: First 3 Dimensions

This document visualizes how atoms bond and form structures in 1D, 2D, and 3D space.

## 1D: Linear Chain

In 1 dimension, atoms form a simple chain where each atom can bond with at most 2 neighbors (left and right).

```
Dimensionality: 1

atom0 ←→ atom1 ←→ atom2 ←→ atom3 ←→ atom4

Neighbor queries from atom2:
  Distance 0: [atom2]
  Distance 1: [atom1, atom3]
  Distance 2: [atom0, atom4]
  Distance 3: []
```

### Code Example

```c
ak_atom_t *chain[5];
for (int i = 0; i < 5; i++) {
    chain[i] = ak_atom_new(AK24_ATOM_I32, (ak_atom_value_u){.i32 = i}, 1);
}

for (int i = 0; i < 4; i++) {
    ak_atom_bond(chain[i], chain[i+1]);
}
```

## 2D: Planar Grid

In 2 dimensions, atoms form a grid where each atom can bond with up to 4 neighbors (up, down, left, right).

```
Dimensionality: 2

        [0,0] ←→ [0,1] ←→ [0,2]
          ↕        ↕        ↕
        [1,0] ←→ [1,1] ←→ [1,2]
          ↕        ↕        ↕
        [2,0] ←→ [2,1] ←→ [2,2]

Neighbor queries from [1,1] (center):
  Distance 0: [[1,1]]
  Distance 1: [[0,1], [2,1], [1,0], [1,2]]  (4 neighbors)
  Distance 2: [[0,0], [0,2], [2,0], [2,2]]  (4 corners)
```

### Visualization with Distance Layers

```
From center atom [1,1]:

Distance 0:          Distance 1:          Distance 2:
    · · ·                · X ·                X · X
    · ■ ·                X ■ X                · · ·
    · · ·                · X ·                X · X

    ■ = query atom
    X = atoms at this distance
    · = other atoms
```

### Code Example

```c
ak_atom_t *grid[3][3];
for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
        grid[i][j] = ak_atom_new(AK24_ATOM_I32,
                                 (ak_atom_value_u){.i32 = i*3 + j}, 2);
    }
}

for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
        if (i < 2) ak_atom_bond(grid[i][j], grid[i+1][j]);
        if (j < 2) ak_atom_bond(grid[i][j], grid[i][j+1]);
    }
}
```

## 3D: Spatial Cube

In 3 dimensions, atoms form a cube where each atom can bond with up to 6 neighbors (up, down, left, right, forward, back).

```
Dimensionality: 3

A 2×2×2 cube:

        z=1 layer              z=0 layer
    [0,0,1]─[0,1,1]        [0,0,0]─[0,1,0]
       │  ╲    │  ╲           │  ╲    │  ╲
       │   [1,0,1]─[1,1,1]    │   [1,0,0]─[1,1,0]
       │    │    │    │        │    │    │    │
    [0,0,1]─│─[0,1,1] │     [0,0,0]─│─[0,1,0] │
        ╲   │     ╲   │         ╲   │     ╲   │
         [1,0,1]───[1,1,1]       [1,0,0]───[1,1,0]

Bonds between layers (z-axis):
    [0,0,0] ←→ [0,0,1]
    [0,1,0] ←→ [0,1,1]
    [1,0,0] ←→ [1,0,1]
    [1,1,0] ←→ [1,1,1]
```

### Corner Atom Neighbors

```
From corner [0,0,0]:

Distance 1 neighbors (3 atoms):
    [1,0,0]  (x-direction)
    [0,1,0]  (y-direction)
    [0,0,1]  (z-direction)

         [0,0,1]
            │
            │
    [0,1,0]─[0,0,0]─[1,0,0]
```

### Center of 3×3×3 Cube

```
From center [1,1,1] in a 3×3×3 cube:

Distance 1 neighbors (6 atoms):
    [0,1,1]  (left)
    [2,1,1]  (right)
    [1,0,1]  (down)
    [1,2,1]  (up)
    [1,1,0]  (back)
    [1,1,2]  (forward)

Visualization (cross-section at y=1):
              [1,1,2]
                 │
    [0,1,1] ─ [1,1,1] ─ [2,1,1]
                 │
              [1,1,0]
```

### Code Example

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

## Neighbor Count by Dimension

| Position Type | 1D | 2D Grid | 3D Cube |
|--------------|----|---------|---------|
| Corner       | 1  | 2       | 3       |
| Edge         | 2  | 3       | 4       |
| Face         | -  | 4       | 5       |
| Center       | 2  | 4       | 6       |

## Distance Query Examples

### 1D Chain (5 atoms)

```
Query from atom2:

atom0 ←→ atom1 ←→ atom2 ←→ atom3 ←→ atom4
 d=2      d=1      d=0      d=1      d=2
```

### 2D Grid (3×3)

```
Query from center [1,1]:

Distance 0:     Distance 1:     Distance 2:
  · · ·           · 1 ·           2 · 2
  · 0 ·           1 0 1           · · ·
  · · ·           · 1 ·           2 · 2
```

### 3D Cube (3×3×3)

```
Query from center [1,1,1]:

Distance 1: 6 atoms (faces of inner cube)
Distance 2: 12 atoms (edges of outer shell)
Distance 3: 8 atoms (corners of outer shell)
```

## Key Concepts

1. **Dimensionality** determines maximum neighbors:
   - 1D: max 2 neighbors
   - 2D: max 4 neighbors
   - 3D: max 6 neighbors
   - ND: max 2N neighbors

2. **Distance queries** return all atoms at exactly that distance:
   - Forms concentric "shells" or "layers"
   - Like peeling an onion outward from the query atom

3. **Bonds are bidirectional**:
   - If A bonds to B, then B bonds to A
   - Both atoms track the connection

4. **No directional filtering yet**:
   - Queries return entire shells at a distance
   - Cannot yet query "only atoms in the +x direction"
   - Future enhancement: dimensional slicing
