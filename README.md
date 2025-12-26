<div align="center">
  <img src="assets/ak24-logo.svg" alt="AK24 Logo" width="300"/>
</div>

# AK24

AK24 (Application Kernel 24) is a C library that provides data structures, memory management primitives, and utilities for building applications. The kernel (K) is the foundation, offering containers, lambda abstractions, context management, and optional Boehm GC integration. All components are tested at compile-time. The library can be embedded as a base layer in larger systems.

## Platform Support

The AK24 kernel currently targets nix (Unix-like) systems. Windows support is planned and actively targeted; several compatibility hurdles are in progress. The final major hurdle is the `modules` subsystem, which is POSIX-only right now. This work is backburnered until the broader system stabilizes.

The `kernel` core already routes all thread functions correctly. Once the kernel supports Windows, AK24 applications will be able to run on Windows without changes.

## Kernel

The kernel is the core of AK24. It provides memory management abstractions, garbage collection integration, initialization/shutdown, and thread creation utilities. Supports both GC and non-GC builds with optional memory tracking for debugging.

### Kernel Modules

| Module | Description |
|--------|-------------|
| [arbuff](#arbuff) | Lock-free atomic ring buffer for concurrent MPMC queue operations |
| [atoms](#atoms) | Thread-safe N-dimensional data structures with atomic value updates |
| [buffer](#buffer) | Dynamic byte array with automatic growth and file I/O |
| [context](#context) | Hierarchical scoped key-value store with parent-child relationships |
| [forms](#forms) | Structural type system for compiler construction |
| [lambda](#lambda) | Closure-like structure for function pointers with captured context |
| [list](#list) | Generic dynamic array with type-safe macros |
| [log](#log) | Thread-safe logging system with multiple outputs and severity levels |
| [map](#map) | Generic hash table with type-safe macros and multiple key types |
| [scanner](#scanner) | Lexical scanner for parsing basic types from buffers |

### Building

Build the library:
```bash
make
```

Install the library:
```bash
make install
```

Run tests:
```bash
./ak24.sh test
```

Run complete CI test suite (all configurations):
```bash
./ak24.sh ci
```

Build with GC disabled:
```bash
AK24_GC=OFF make
```

### Using AK24 in Your Projects

After installation, link against AK24 using one of these methods:

**Using pkg-config:**
```bash
gcc myprogram.c $(pkg-config --cflags --libs ak24) -o myprogram
```

**Using ak24-config:**
```bash
gcc myprogram.c $(ak24-config --cflags --libs) -o myprogram
```

**Manual linking:**
```bash
gcc myprogram.c -I$AK24_HOME/include/ak24 -L$AK24_HOME/lib -lak24_kernel -lpthread -lssl -lcrypto -o myprogram
```

The static library bundles TCC and Boehm GC internally, so you only need to link system libraries (pthread, OpenSSL). For shared library builds, all dependencies are embedded.

### Documentation

Generate API documentation with Doxygen:
```bash
make docs
```

View the generated documentation at `docs/api/html/index.html`. The Doxygen documentation is the source of truth and will be the most up to date reference for the API.

---

<details>
<summary><h3>arbuff</h3></summary>

Thread-safe lock-free MPMC circular buffer for `void*` pointers. Uses sequence-based algorithm with C11 atomics for safe concurrent access. Capacity is automatically rounded to next power of 2. Returns errors when full rather than overwriting data.

**Documentation**: [kernel/arbuff/docs/arbuff.md](kernel/arbuff/docs/arbuff.md)

</details>


<details>
<summary><h3>atoms</h3></summary>

Thread-safe atomically mutable building blocks for N-dimensional data structures. Atoms store typed primitive values and can bond with multiple neighbors to form 1D chains, 2D grids, 3D cubes, or N-D lattices. Uses C11 atomics for lock-free concurrent value updates.

**Documentation**: [kernel/atoms/docs/atoms.md](kernel/atoms/docs/atoms.md)

</details>

<details>
<summary><h3>buffer</h3></summary>

Dynamic byte array for storing `uint8_t` data. Automatically resizes when capacity is exceeded (doubles capacity). Minimum capacity of 16 bytes. Supports rotation, trimming, splitting, sub-buffer operations, and file loading.

**Documentation**: [kernel/buffer/docs/buffer.md](kernel/buffer/docs/buffer.md)

</details>

<details>
<summary><h3>context</h3></summary>

Hierarchical scoped key-value store with parent-child relationships. Supports variable shadowing where child values override parent values. Can hoist values to parent scope on pop. Uses map for storage and list for hoist queue.

**Documentation**: [kernel/context/docs/context.md](kernel/context/docs/context.md)

</details>

<details>
<summary><h3>forms</h3></summary>

Structural type system infrastructure for compiler construction. Provides structural types (data layout without nominal identity), affordances (operations available on types), affects (method definitions with composition), and pattern matching for structural compatibility checking.

**Documentation**: [kernel/forms/docs/forms.md](kernel/forms/docs/forms.md)

</details>

<details>
<summary><h3>lambda</h3></summary>

Closure-like structure for storing function pointers with captured context. Supports optional context cleanup function and cloning (shallow copy). Context can be replaced or retrieved at runtime.

**Documentation**: [kernel/lambda/docs/lambda.md](kernel/lambda/docs/lambda.md)

</details>

<details>
<summary><h3>list</h3></summary>

Generic dynamic array with type-safe macros. Automatically resizes when capacity is exceeded (doubles capacity). Initial capacity of 8 items. Supports push, pop, insert, remove, rotation, and iterator pattern. Items are heap-allocated copies.

**Documentation**: [kernel/list/docs/list.md](kernel/list/docs/list.md)

</details>

<details>
<summary><h3>log</h3></summary>

Thread-safe logging system with six severity levels (TRACE to FATAL). Supports multiple simultaneous output targets, custom callbacks, file output, and optional color output for terminals. Thread safety requires user-provided lock function. Based on rxi's log.c library.

**Documentation**: See [kernel/log/include/log.h](kernel/log/include/log.h)

</details>

<details>
<summary><h3>map</h3></summary>

Generic hash table with type-safe macros. Supports multiple key types (string, int, float, etc) with custom hash and comparison functions. Automatic resizing doubles buckets when load factor exceeds 1.0. Uses chaining for collision resolution.

**Documentation**: [kernel/map/docs/map.md](kernel/map/docs/map.md)

</details>

<details>
<summary><h3>scanner</h3></summary>

Lexical scanner for parsing integers, reals, and symbols from byte buffers. State-machine-based with configurable stop symbols and whitespace handling. Supports delimited group extraction with escape sequences. Non-owning buffer references for zero-copy parsing with position tracking.

**Documentation**: See [kernel/scanner/include/scanner.h](kernel/scanner/include/scanner.h)

</details>

---

## Development

For developers looking to contribute to or extend AK24, please refer to the following documentation:

- **[docs/kernel.md](docs/kernel.md)** - Kernel initialization, memory management, threading abstractions, and application framework
- **[docs/platform.md](docs/platform.md)** - Platform abstraction layer architecture showing how AK24 maps to POSIX/Windows implementations
- **[docs/testing.md](docs/testing.md)** - Testing strategy for GC, ASAN, and manual memory configurations

