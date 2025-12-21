# Testing Guide

This document describes the AK24 testing strategy, build modes, and how to ensure your code works correctly across all supported configurations.

## Overview

AK24 supports multiple build configurations to accommodate different use cases:
- **Garbage Collection (GC)** - Memory is automatically managed by Boehm GC
- **AddressSanitizer (ASAN)** - Runtime memory error detection for debugging
- **Manual Memory** - Standard `malloc`/`free` without GC or ASAN

These configurations are **mutually exclusive** for important technical reasons.

## Build Modes

### GC Mode (Default)

```bash
AK24_BUILD_MODE=gc ./ak24.sh install
# or simply:
./ak24.sh install
```

**Characteristics:**
- Boehm GC enabled for automatic memory management
- ASAN disabled (incompatible with GC)
- Best for production use and applications that benefit from automatic memory management
- No need to explicitly free memory in most cases

### ASAN Mode (Debugging)

```bash
AK24_BUILD_MODE=asan ./ak24.sh install
```

**Characteristics:**
- AddressSanitizer enabled for comprehensive memory error detection
- GC automatically disabled (ASAN and GC are incompatible)
- Detects: buffer overflows, use-after-free, memory leaks, double-free
- Adds runtime overhead (~2x slower, ~3x memory usage)
- Best for development and debugging memory issues

### Manual Memory Mode (Advanced)

While not exposed as a build mode in `ak24.sh`, users can build AK24 without GC or ASAN by configuring CMake directly:

```bash
mkdir -p build && cd build
cmake -DCMAKE_INSTALL_PREFIX=~/.ak24 -DAK24_GC_ENABLED=OFF -DAK24_BUILD_ASAN=OFF ..
make install
```

**Characteristics:**
- Uses standard `malloc`/`free`
- No GC, no ASAN
- Maximum performance, minimal overhead
- Requires manual memory management
- Useful for resource-constrained environments or when integrating with other memory managers

## Why ASAN and GC Are Mutually Exclusive

**Technical Reason:** AddressSanitizer tracks all memory allocations to detect errors. Boehm GC performs memory scanning and collection which appears as suspicious memory access patterns to ASAN, triggering false positives for "leaks" and "use-after-free" errors.

**Solution:** When ASAN is enabled, AK24 automatically disables GC and uses standard memory allocation. This allows ASAN to properly track and report real memory errors without interference from the garbage collector.

## Test Suite Integration

The AK24 build system automatically propagates build configuration to all tests:

### Automatic Configuration Propagation

When you install AK24, a build configuration file is created:
```
~/.ak24/lib/ak24-config.mk
```

This file contains:
```makefile
AK24_VERSION=0.0.1
AK24_ASAN_ENABLED=1     # or 0
AK24_ASAN_FLAGS=-fsanitize=address -fno-omit-frame-pointer -g
AK24_GC_ENABLED=0       # or 1
```

### Test Behavior

All tests automatically detect and match the build configuration:

```bash
# Install with ASAN
AK24_BUILD_MODE=asan ./ak24.sh install

# Run tests - automatically uses ASAN flags
./ak24.sh test
```

**What happens:**
1. Compile-time tests build with ASAN flags
2. Integration tests detect ASAN configuration from `ak24-config.mk`
3. Test modules compile with ASAN flags
4. Test binaries link with ASAN flags
5. All code runs under ASAN supervision

If ASAN flags are not matched between AK24 and test code, you'll see linker errors about undefined ASAN symbols.

## Recommended Full-System Testing Strategy

For comprehensive validation of AK24 and external modules, you should test all three configurations:

### 1. GC Tests (Production Configuration)

```bash
AK24_BUILD_MODE=gc ./ak24.sh test
```

**Validates:**
- Normal production behavior
- GC integration works correctly
- Memory is managed automatically
- No explicit frees are required

### 2. No GC + ASAN Tests (Memory Debugging)

```bash
AK24_BUILD_MODE=asan ./ak24.sh test
```

**Validates:**
- No memory leaks (ASAN will report any leaked allocations)
- No buffer overflows or underflows
- No use-after-free errors
- No double-free errors
- Proper manual memory management

### 3. No GC + No ASAN Tests (Minimal Configuration)

```bash
mkdir -p build && cd build
cmake -DCMAKE_INSTALL_PREFIX=~/.ak24 -DAK24_GC_ENABLED=OFF -DAK24_BUILD_ASAN=OFF ..
make install
cd ..
./tests/run.sh
```

**Validates:**
- Code works without GC
- Code works without ASAN overhead
- Manual memory management is correct
- Minimal runtime dependencies
- Maximum performance configuration

### Why Test All Three?

**External Module Compatibility:** Your AK24 modules might be used in different environments:
- Embedded systems (No GC, No ASAN)
- Desktop applications (GC enabled)
- Development/debugging (ASAN enabled)

**Memory Management Verification:** Different configurations expose different issues:
- **GC mode** hides memory leaks but may mask manual memory management bugs
- **ASAN mode** exposes memory errors but has runtime overhead
- **Manual mode** validates that memory is managed correctly without any assistance

**Full Coverage:** Testing all three ensures:
- Your code doesn't depend on GC behavior
- Your code doesn't leak memory (verified by ASAN)
- Your code works in resource-constrained environments (manual mode)
- External modules can link against any AK24 configuration

## Running Specific Test Suites

### Compile-Time Tests Only

```bash
cd build
make test
```

These tests validate template correctness, macro behavior, and compile-time features.

### Integration Tests Only

```bash
cd tests
./run.sh
```

These tests validate:
- Module loading and unloading
- Cross-module communication
- Dynamic library integration
- Real-world usage patterns

### Full Test Suite

```bash
./ak24.sh test
```

Runs both compile-time and integration tests with automatic build and installation for the current build mode.

### Complete CI Test Suite

```bash
./ak24.sh ci
```

Runs the complete continuous integration test suite across **all three configurations**:
1. GC mode (production)
2. ASAN mode (memory debugging)
3. Manual mode (no GC, no ASAN)

This command:
- Builds, installs, and tests each configuration sequentially
- Stops immediately on any failure
- Automatically cleans up and uninstalls after all tests pass
- Provides a final summary report

**Use this for comprehensive validation** before commits or releases to ensure your code works correctly in all supported build configurations.

## Continuous Integration Recommendations

For CI/CD pipelines, use the automated CI test suite:

```bash
./ak24.sh ci
```

This single command replaces the manual approach and automatically:
- Tests all three configurations (GC, ASAN, Manual)
- Fails fast on any error
- Cleans up after completion
- Provides comprehensive test coverage

**Manual approach** (if you need more control):

```bash
#!/bin/bash
set -e

# Test with GC (production)
echo "Testing with GC..."
AK24_BUILD_MODE=gc ./ak24.sh test

# Test with ASAN (debugging)
echo "Testing with ASAN..."
AK24_BUILD_MODE=asan ./ak24.sh test

# Test without GC or ASAN (minimal)
echo "Testing manual memory mode..."
make clean
mkdir -p build && cd build
cmake -DCMAKE_INSTALL_PREFIX=~/.ak24 -DAK24_GC_ENABLED=OFF -DAK24_BUILD_ASAN=OFF ..
make install
cd ..
./tests/run.sh

echo "All configurations passed!"
```

## Troubleshooting

### Linker Errors About ASAN Symbols

**Problem:**
```
Undefined symbols for architecture arm64:
  "___asan_init", referenced from: ...
```

**Cause:** AK24 was built with ASAN, but your code wasn't compiled with ASAN flags.

**Solution:** Rebuild AK24 or ensure your code uses the config from `ak24-config.mk`:

```makefile
-include $(AK24_HOME)/lib/ak24-config.mk

ifeq ($(AK24_ASAN_ENABLED),1)
    CFLAGS += $(AK24_ASAN_FLAGS)
    LDFLAGS += $(AK24_ASAN_FLAGS)
endif
```
