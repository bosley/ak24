#!/usr/bin/env bash
#
# test_macros.sh - Integration test for module macro system
#
# This script tests the V1 macro system by building and running
# the module_macrod test.
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "========================================="
echo "AK24 Module Macro System - Integration Test"
echo "========================================="
echo ""

# Check if AK24 is installed
AK24_HOME="${AK24_HOME:-${HOME}/.ak24}"

if [[ ! -f "${AK24_HOME}/include/ak24/interfaces.h" ]]; then
    echo "❌ Error: AK24 not installed at ${AK24_HOME}"
    echo ""
    echo "Please install AK24 first:"
    echo "  cd /Users/bosley/workspace/ak24/build"
    echo "  sudo make install"
    echo ""
    exit 1
fi

echo "✓ AK24 found at: ${AK24_HOME}"
echo ""

# Build the module
echo "Step 1: Building test module..."
cd "${SCRIPT_DIR}" || exit 1
echo "Working directory: $(pwd)"
./setup.sh || exit 1
echo ""

# Build the test binary
echo "Step 2: Building test binary..."
cd "${SCRIPT_DIR}" || exit 1
rm -f module_test
make || exit 1
echo ""

# Run the test
echo "Step 3: Running integration test..."
cd "${SCRIPT_DIR}" || exit 1
echo "Working directory: $(pwd)"
echo "Module should be at: $(pwd)/build/libtest_module.dylib"
if [[ ! -f "build/libtest_module.dylib" ]]; then
    echo "❌ Error: Module not found at build/libtest_module.dylib"
    ls -la build/ || echo "build/ directory does not exist"
    exit 1
fi
echo "========================================="
./module_test
echo "========================================="
echo ""
