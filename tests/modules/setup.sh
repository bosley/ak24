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

# Compile the shared library
echo "Compiling lib.c..."
cc -std=c11 -Wall -Wextra -O2 \
    ${SHARED_FLAG} \
    -fPIC \
    -I"${LIB_DIR}" \
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
