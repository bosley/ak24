# AK24 Application Kernel

<div align="center">
  <img src="ak24-logo.svg" alt="AK24 Logo" width="300"/>
</div>

**Version**: 0.0.1-dev
**Status**: Pre-alpha development

---

## Overview

AK24 (Application Kernel 24) is a lightweight C library providing core data structures and memory management primitives for building robust applications. The kernel includes generic containers (lists, maps, buffers), lambda abstractions, context management, and optional Boehm GC integration.

## Core Modules

### Data Structures
- **[Buffer](@ref buffer.h)** - Dynamic byte buffer with manipulation operations
- **[List](@ref list.h)** - Type-safe generic dynamic array using macros
- **[Map](@ref map.h)** - Type-safe generic hash map with custom hash functions
- **[Arbuff](@ref arbuff.h)** - Lock-free atomic ring buffer for MPMC scenarios

### Advanced Primitives
- **[Atoms](@ref atom.h)** - Multi-dimensional atomic data nodes with neighbor relationships
- **[Lambda](@ref lambda.h)** - Function closures with captured context
- **[Context](@ref context.h)** - Hierarchical scoped context with variable hoisting

### Processing & Type System
- **[Scanner](@ref scanner.h)** - Lexical scanner for parsing basic types from buffers
- **[Forms](@ref forms.h)** - Type system with structural patterns and affordances
- **[Forms Primitives](@ref forms_primitives.h)** - Form instances and built-in primitives

### System
- **[Kernel](@ref kernel.h)** - Main kernel header with memory management
- **[Application](@ref application.h)** - Application framework with lifecycle management
- **[Log](@ref log.h)** - Thread-safe logging system with multiple output targets

## Quick Start

### Building

```bash
mkdir build && cd build
cmake -DAK24_GC_ENABLED=ON ..
make
```

### Application Framework

AK24 provides an application framework that handles initialization, argument processing, and shutdown. Applications are defined using three key macros:

- **`APP_MAIN(name)`** - Defines the main application entry point
- **`APP_ON_SHUTDOWN(name)`** - Defines a shutdown handler
- **`AK24_APPLICATION(app_id, main_fn, shutdown_fn)`** - Binds everything together

The **application ID** is a string that identifies your application and is used for runtime directory isolation. It should be unique to your application (e.g., `"my-app"`, `"my-app-v1"`).

### Basic Usage

```c
#include <kernel.h>

APP_MAIN(my_app) {
    AK24_LOG_INFO("Application started");

    list_t(int) numbers;
    list_init(&numbers);
    list_push(&numbers, 42);

    AK24_LOG_INFO("Number: %d", *list_get(&numbers, 0));

    list_deinit(&numbers);
    return 0;
}

APP_ON_SHUTDOWN(cleanup) {
    AK24_LOG_INFO("Shutting down");
}

AK24_APPLICATION("my-app", my_app, cleanup)
```

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `AK24_GC_ENABLED` | ON | Enable Boehm GC for memory management |
| `AK24_BUILD_DEBUG_MEMORY` | OFF | Enable memory tracking and statistics |
| `AK24_BUILD_SHARED_KERNEL` | OFF | Build kernel as shared library |
| `AK24_BUILD_DOCS` | OFF | Build API documentation with Doxygen |

## Documentation

- **[Files](files.html)** - Browse by file structure
- **[Data Structures](annotated.html)** - All types and structures
- **[Functions](globals_func.html)** - Complete API reference

## Thread Safety Guide

All functions are marked with thread safety indicators:

- **@threadsafe** - Safe for concurrent access
- **@notthreadsafe** - Requires external synchronization
- **@lockfree** - Uses lock-free atomic operations
- **@waitfree** - Bounded time complexity

## Examples

See the `examples/` directory for complete working examples:
- `ak24-cli` - Command-line application template
- `atom-cube-demo` - Multi-dimensional atom demonstration

## License

See LICENSE file for details.

## Contributing

This is a pre-alpha project under active development. API is subject to change.

---

**Documentation generated with Doxygen**
