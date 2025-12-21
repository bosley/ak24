#!/usr/bin/env bash
#
# setup.sh - Build the test module as a shared library
#
# This script compiles the test module (lib/lib.c) into a dynamic library
# that can be loaded by the AK24 module system.
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIB_DIR="${SCRIPT_DIR}/lib"
BUILD_DIR="${SCRIPT_DIR}/build"

echo "=== Building Test Module ==="

# Create build directory
mkdir -p "${BUILD_DIR}"

# Determine platform
if [[ "$OSTYPE" == "darwin"* ]]; then
    LIB_EXT="dylib"
    SHARED_FLAG="-dynamiclib"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    LIB_EXT="so"
    SHARED_FLAG="-shared"
else
    echo "Unsupported platform: $OSTYPE"
    exit 1
fi

OUTPUT="${BUILD_DIR}/libtest_module.${LIB_EXT}"

echo "Platform: ${OSTYPE}"
echo "Output: ${OUTPUT}"

# Get AK24 installation directory
AK24_HOME="${AK24_HOME:-${HOME}/.ak24}"

if [[ ! -f "${AK24_HOME}/include/ak24/interfaces.h" ]]; then
    echo "Error: AK24 not found at ${AK24_HOME}"
    echo "Please install AK24 first or set AK24_HOME"
    exit 1
fi

echo "Using AK24 from: ${AK24_HOME}"

# Check if AK24 was built with ASAN
ASAN_FLAGS=""
if [[ -f "${AK24_HOME}/lib/ak24-config.mk" ]]; then
    # Source the config to get ASAN settings
    source <(grep -E '^(AK24_ASAN_ENABLED|AK24_ASAN_FLAGS)=' "${AK24_HOME}/lib/ak24-config.mk")
    if [[ "${AK24_ASAN_ENABLED}" == "1" ]]; then
        ASAN_FLAGS="${AK24_ASAN_FLAGS}"
        echo "Detected ASAN build, adding flags: ${ASAN_FLAGS}"
    fi
fi

# Compile the shared library
echo "Compiling lib.c..."
cc -std=c11 -Wall -Wextra -O2 \
    ${SHARED_FLAG} \
    -fPIC \
    ${ASAN_FLAGS} \
    -I"${LIB_DIR}" \
    -I"${AK24_HOME}/include/ak24" \
    -o "${OUTPUT}" \
    "${LIB_DIR}/lib.c"

if [[ -f "${OUTPUT}" ]]; then
    echo "✓ Test module built successfully: ${OUTPUT}"
    ls -lh "${OUTPUT}"
else
    echo "✗ Failed to build test module"
    exit 1
fi

echo "=== Build Complete ==="
