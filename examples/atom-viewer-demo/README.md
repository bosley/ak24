# Atom Viewer Demo (N-Dimensional)

A demonstration of the AK24 N-dimensional atom viewer system for visualizing and projecting arbitrary N-dimensional atom structures onto 2D views.

## Status

⚠️ **This is currently a stub/template.** The viewer system library needs to be implemented before this demo will function. See [`ATOM_VIEW_PLAN.md`](../../ATOM_VIEW_PLAN.md) for the complete design specification.

## What This Will Be

When implemented, this demo will showcase the `viewer` system - a higher-level visualization layer built on top of the kernel's `atoms` module. It will demonstrate:

1. **N-Dimensional Positioning**: Create atoms positioned in 4D, 5D, or higher dimensional spaces
2. **Flexible Projection**: Project any N-dimensional structure onto a 2D view by selecting axes
3. **N-Dimensional Rotation**: Rotate the view in multiple planes simultaneously
4. **Depth-Aware Rendering**: Proper occlusion handling via depth buffer
5. **Real-time Animation**: Continuously rotating visualization in ASCII art

Unlike the 3D-only `atom-cube-demo`, this will work with arbitrary dimensionalities and provide a general-purpose viewer API.

## Expected Features

- **Configurable Dimensionality**: View 3D, 4D, 5D, or higher dimensional structures
- **Multiple Projection Modes**: Choose which dimensions to project onto the screen
- **Compound Rotations**: Rotate in multiple planes at different rates
- **Arena-Based Rendering**: Fast per-frame allocation and cleanup
- **Thread Pool Support**: Optional parallel rendering for large atom counts

## Building (Once Implemented)

From the project root:

```bash
cmake -S . -B build -DAK24_GC_ENABLED=ON -DAK24_BUILD_VIEWER=ON
cmake --build build --target atom_viewer_demo
```

To disable the viewer system (and this demo):

```bash
cmake -S . -B build -DAK24_BUILD_VIEWER=OFF
```

## Running (Once Implemented)

```bash
./build/bin/atom-viewer-demo [dimensionality] [size]
```

Where:
- `[dimensionality]` - Number of dimensions (default: 4)
- `[size]` - Size of hypercube per dimension (default: 2)

Examples:
```bash
./build/bin/atom-viewer-demo 4 2   # 4D tesseract (2x2x2x2)
./build/bin/atom-viewer-demo 3 3   # 3D cube (3x3x3)
./build/bin/atom-viewer-demo 5 2   # 5D penteract (2x2x2x2x2)
```

## Implementation Roadmap

The viewer system needs to be implemented in phases as outlined in `ATOM_VIEW_PLAN.md`:

### Phase 1: Basic Projection (No Rotation)
- [ ] Implement `ak_atom_positioned_t` structure allocation
- [ ] Implement `ak_atom_view_t` structure allocation
- [ ] Implement basic `ak_atom_project_view()` - just project two axes
- [ ] Implement arena version `ak_atom_project_view_arena()`
- [ ] Test with simple 3D case

### Phase 2: Depth Buffer
- [ ] Add depth buffer logic during projection
- [ ] Calculate depth from non-displayed dimensions
- [ ] Test occlusion works correctly

### Phase 3: Rotation Support
- [ ] Implement `ak_atom_view_config_t` structure
- [ ] Implement plane-based rotation application
- [ ] Add `ak_atom_view_rotate()` helper
- [ ] Test rotations in 3D, then 4D

### Phase 4: Higher Dimensions
- [ ] Test with 4D, 5D, 6D cases
- [ ] Verify multiple simultaneous rotations work
- [ ] Add utility functions for common operations

### Phase 5: Parallel Processing (Optional)
- [ ] Implement `ak_atom_project_view_parallel()`
- [ ] Benchmark vs single-threaded
- [ ] Document performance characteristics

## Architecture

The demo is built on two separate libraries:

1. **Kernel** (`ak24_kernel`) - Core data structures including atoms
2. **Viewer System** (`ak24_viewer`) - Visualization layer (to be implemented)

This modular design allows applications to:
- Use only the kernel if visualization isn't needed
- Optionally include viewer or other systems
- Keep dependencies minimal and explicit

## Files to Implement

- [`systems/viewer/src/atom_view.c`](../../systems/viewer/src/atom_view.c) - All viewer functions
- [`examples/atom-viewer-demo/main.c`](main.c) - Complete the TODOs in this file

## Related Documentation

- [`ATOM_VIEW_PLAN.md`](../../ATOM_VIEW_PLAN.md) - Complete design specification
- [`kernel/atoms/docs/atoms.md`](../../kernel/atoms/docs/atoms.md) - Atom system documentation
- [`examples/atom-cube-demo/`](../atom-cube-demo/) - Simpler 3D-only predecessor

## Design Philosophy

The viewer system follows AK24 principles:

- **Simple and Direct**: No complex scene graphs or game engine abstractions
- **Read-only Views**: Views don't modify atoms, they're snapshots for display
- **Arena-Friendly**: Designed for arena allocation patterns
- **Thread-Safe**: Supports parallel rendering when beneficial
- **Zero Dependencies**: Only depends on kernel, no external graphics libs
