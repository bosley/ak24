#!/usr/bin/env bash
#
# run.sh - Run all integration tests
#
# This script runs all integration tests in the tests/ directory.
# These tests require AK24 to be installed first.
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Array of test directories to run
TESTS=(
    "module_macrod"
    "modules"
)

echo "========================================="
echo "AK24 Integration Tests"
echo "========================================="
echo ""

# Check if AK24 is installed
AK24_HOME="${AK24_HOME:-${HOME}/.ak24}"

if [[ ! -f "${AK24_HOME}/include/ak24/interfaces.h" ]]; then
    echo "❌ Error: AK24 not installed at ${AK24_HOME}"
    echo ""
    echo "Please install AK24 first:"
    echo "  ./ak24.sh install"
    echo ""
    exit 1
fi

echo "✓ AK24 found at: ${AK24_HOME}"
echo ""

# Counter for test results
TOTAL_TESTS=${#TESTS[@]}
PASSED_TESTS=0
FAILED_TESTS=0

# Run each test
for test_dir in "${TESTS[@]}"; do
    TEST_PATH="${SCRIPT_DIR}/${test_dir}"

    if [[ ! -d "${TEST_PATH}" ]]; then
        echo "❌ Test directory not found: ${test_dir}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        continue
    fi

    echo "========================================="
    echo "Running test: ${test_dir}"
    echo "========================================="
    echo ""

    cd "${TEST_PATH}" || exit 1

    # Clean previous builds
    echo "Cleaning previous builds..."
    make clean 2>/dev/null || true
    rm -rf build 2>/dev/null || true
    echo ""

    # Run setup to build the library
    if [[ -f "setup.sh" ]]; then
        echo "Building test module..."
        if ./setup.sh; then
            echo "✓ Module built successfully"
        else
            echo "❌ Failed to build module"
            FAILED_TESTS=$((FAILED_TESTS + 1))
            continue
        fi
        echo ""
    fi

    # Build the test binary
    echo "Building test binary..."
    if make; then
        echo "✓ Test binary built successfully"
    else
        echo "❌ Failed to build test binary"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        continue
    fi
    echo ""

    # Run the test binary
    echo "Running test..."
    TEST_BINARY="module_test"
    if [[ -f "${TEST_BINARY}" ]]; then
        if ./"${TEST_BINARY}"; then
            echo ""
            echo "✓ Test passed: ${test_dir}"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo ""
            echo "❌ Test failed: ${test_dir}"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    else
        echo "❌ Test binary not found: ${TEST_BINARY}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi

    echo ""
    cd "${SCRIPT_DIR}"
done

# Print summary
echo "========================================="
echo "Test Summary"
echo "========================================="
echo "Total tests:  ${TOTAL_TESTS}"
echo "Passed:       ${PASSED_TESTS}"
echo "Failed:       ${FAILED_TESTS}"
echo "========================================="
echo ""

if [[ ${FAILED_TESTS} -eq 0 ]]; then
    echo "✓ All integration tests passed!"
    exit 0
else
    echo "❌ Some integration tests failed"
    exit 1
fi
