# Atom Cube Demo

A visual demonstration of the AK24 N-dimensional atom system, rendering a rotating 3D cube of atomically mutable data structures in ASCII art.

## What This Is

This demo showcases the `atoms` module - a fundamental building block in AK24 that implements N-dimensional graph structures with atomic mutability. Think of atoms as nodes that can bond with other atoms in any number of dimensions, forming lattices, grids, cubes, or arbitrary N-dimensional structures.

The visualization creates a 3D lattice of atoms, bonds them together to form a cube, and then renders it as a rotating orthographic projection on your terminal. Each atom's value pulsates in waves from the center to the edges, demonstrating thread-safe atomic updates in real-time.

## What It Does

1. **Creates an N×N×N Cube**: Dynamically allocates and initializes a configurable 3D lattice of atoms
2. **Bonds Atoms**: Connects each atom to its neighbors in all three dimensions (x, y, z)
3. **Renders in 3D**: Projects the cube onto a 2D ASCII screen with depth sorting and rotation
4. **Animates Continuously**: Rotates the cube on two axes while running indefinitely
5. **Pulsates Values**: Updates atom values in waves based on distance from center, showing atomic mutability
6. **Configurable Size**: Accepts command-line arguments to control cube dimensions (2×2×2 up to 10×10×10)

## Building

From the project root:

```bash
cmake -S . -B build -DAK24_GC_ENABLED=ON
cmake --build build --target atom_cube_demo
```

## Running

```bash
./build/bin/atom-cube-demo [size]
```

Where `[size]` is optional (default: 2) and must be between 2 and 10.

Examples:
```bash
./build/bin/atom-cube-demo 2
./build/bin/atom-cube-demo 4
./build/bin/atom-cube-demo 8
```

## Visualization

The demo renders an ASCII orthographic projection of the 3D cube:
- `[##]` - Atom nodes with their integer values
- `=` - Horizontal bonds between atoms
- `|` - Vertical bonds between atoms
- `.` - Depth bonds (z-axis connections)

The cube rotates continuously, and atom values pulse in waves radiating from the center outward. The rendering uses depth sorting to ensure proper occlusion - closer elements appear on top of farther ones.

## Key Concepts Demonstrated

1. **N-Dimensional Structures**: Atoms form true multi-dimensional graphs, not just linked lists
2. **Atomic Mutability**: All atom values and neighbor pointers use C11 atomics for lock-free thread safety
3. **Dynamic Bonding**: Atoms can bond/unbond with any other atom at runtime
4. **Scalar Distance Queries**: Query atoms at specific "distances" (number of bonds away)
5. **Real-time Updates**: Values change atomically while the structure is being read/rendered
6. **Configurable Dimensionality**: Same code works for any size cube via dynamic allocation

## Code Structure

The example follows the AK24 application pattern:
- `APP_MAIN(app_main)`: Parses args, allocates cube, bonds atoms, starts animation
- `APP_ON_SHUTDOWN(on_shutdown)`: Cleanup and statistics
- `AK24_APPLICATION(app_id, app_main, on_shutdown)`: Application macro

Uses only the kernel's `atoms` API - no additional kernel code required.
