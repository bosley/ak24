#!/usr/bin/env bash
#
# ak24.sh - Project management script for AK24
#
# Commands:
#   install - Clean, build docs, and install AK24
#   status  - Check if AK24 is installed
#   test    - Run full test suite (compile-time + integration tests)
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
AK24_HOME="${AK24_HOME:-${HOME}/.ak24}"
AK24_BUILD_MODE="${AK24_BUILD_MODE:-gc}"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_usage() {
    echo "AK24 Project Management Script"
    echo ""
    echo "Usage: $0 <command>"
    echo ""
    echo "Commands:"
    echo "  preflight  Check and install required dependencies"
    echo "  install    Clean, build documentation, and install AK24"
    echo "  uninstall  Remove AK24 installation completely"
    echo "  status     Check if AK24 is installed and show version info"
    echo "  test       Run full test suite (compile-time + integration tests)"
    echo "  ci         Run complete CI test suite (GC + ASAN + Manual modes)"
    echo ""
    echo "Environment:"
    echo "  AK24_HOME        Installation directory (default: ~/.ak24)"
    echo "                   Current: ${AK24_HOME}"
    echo "  AK24_BUILD_MODE  Build mode: 'gc' or 'asan' (default: gc)"
    echo "                   Current: ${AK24_BUILD_MODE}"
    echo "                   - gc:   Build with Boehm GC (garbage collection)"
    echo "                   - asan: Build with AddressSanitizer (memory debugging)"
    echo ""
}

check_installed() {
    if [[ -f "${AK24_HOME}/include/ak24/kernel.h" ]] && \
       [[ -f "${AK24_HOME}/lib/libak24_kernel.a" ]]; then
        return 0
    else
        return 1
    fi
}

configure_build() {
    echo -e "${BLUE}Configuring build (mode: ${AK24_BUILD_MODE})...${NC}"

    # Validate build mode
    if [[ "${AK24_BUILD_MODE}" != "gc" ]] && [[ "${AK24_BUILD_MODE}" != "asan" ]]; then
        echo -e "${RED}✗ Invalid AK24_BUILD_MODE: ${AK24_BUILD_MODE}${NC}"
        echo "Valid options: gc, asan"
        exit 1
    fi

    mkdir -p "${SCRIPT_DIR}/build"
    cd "${SCRIPT_DIR}/build"

    # Set CMake flags based on build mode
    CMAKE_FLAGS="-DCMAKE_INSTALL_PREFIX=${AK24_HOME}"

    if [[ "${AK24_BUILD_MODE}" == "asan" ]]; then
        CMAKE_FLAGS="${CMAKE_FLAGS} -DAK24_BUILD_ASAN=ON"
        echo -e "  ${YELLOW}→ AddressSanitizer enabled (GC auto-disabled)${NC}"
    else
        CMAKE_FLAGS="${CMAKE_FLAGS} -DAK24_BUILD_ASAN=OFF"
        echo -e "  ${GREEN}→ Boehm GC enabled${NC}"
    fi

    cmake ${CMAKE_FLAGS} ..
    echo ""
}

cmd_preflight() {
    echo "========================================="
    echo "AK24 Preflight Check"
    echo "========================================="
    echo ""

    # Detect platform
    UNAME_S=$(uname -s)
    if [[ "${UNAME_S}" == "Darwin" ]]; then
        PLATFORM="macOS"
        PKG_MANAGER="brew"
    elif [[ "${UNAME_S}" == "Linux" ]]; then
        PLATFORM="Linux"
        # Detect Linux package manager
        if command -v apt-get &> /dev/null; then
            PKG_MANAGER="apt"
        elif command -v dnf &> /dev/null; then
            PKG_MANAGER="dnf"
        elif command -v yum &> /dev/null; then
            PKG_MANAGER="yum"
        elif command -v pacman &> /dev/null; then
            PKG_MANAGER="pacman"
        else
            PKG_MANAGER="unknown"
        fi
    else
        echo -e "${RED}✗ Unsupported platform: ${UNAME_S}${NC}"
        echo "AK24 supports macOS and Linux only."
        exit 1
    fi

    echo "Platform: ${PLATFORM}"
    echo "Package Manager: ${PKG_MANAGER}"
    echo ""

    # Required dependencies from CMakeLists.txt
    DEPS_TO_INSTALL=()
    MISSING_DEPS=()

    # Check CMake (required, minimum 3.20)
    echo -n "Checking for CMake... "
    if command -v cmake &> /dev/null; then
        CMAKE_VERSION=$(cmake --version | head -n1 | awk '{print $3}')
        echo -e "${GREEN}✓ found (${CMAKE_VERSION})${NC}"
        # Check if version is at least 3.20
        CMAKE_MAJOR=$(echo "${CMAKE_VERSION}" | cut -d. -f1)
        CMAKE_MINOR=$(echo "${CMAKE_VERSION}" | cut -d. -f2)
        if [[ ${CMAKE_MAJOR} -lt 3 ]] || [[ ${CMAKE_MAJOR} -eq 3 && ${CMAKE_MINOR} -lt 20 ]]; then
            echo -e "  ${YELLOW}⚠ CMake version 3.20+ required, found ${CMAKE_VERSION}${NC}"
            MISSING_DEPS+=("cmake")
            DEPS_TO_INSTALL+=("cmake")
        fi
    else
        echo -e "${RED}✗ not found${NC}"
        MISSING_DEPS+=("cmake")
        DEPS_TO_INSTALL+=("cmake")
    fi

    # Check C compiler
    echo -n "Checking for C compiler... "
    if command -v cc &> /dev/null || command -v gcc &> /dev/null || command -v clang &> /dev/null; then
        if command -v cc &> /dev/null; then
            CC_VERSION=$(cc --version | head -n1)
        elif command -v gcc &> /dev/null; then
            CC_VERSION=$(gcc --version | head -n1)
        else
            CC_VERSION=$(clang --version | head -n1)
        fi
        echo -e "${GREEN}✓ found${NC}"
        echo "  ${CC_VERSION}"
    else
        echo -e "${RED}✗ not found${NC}"
        MISSING_DEPS+=("compiler")
        if [[ "${PLATFORM}" == "macOS" ]]; then
            echo -e "  ${YELLOW}Install Xcode Command Line Tools: xcode-select --install${NC}"
        elif [[ "${PKG_MANAGER}" == "apt" ]]; then
            DEPS_TO_INSTALL+=("build-essential")
        elif [[ "${PKG_MANAGER}" == "dnf" ]] || [[ "${PKG_MANAGER}" == "yum" ]]; then
            DEPS_TO_INSTALL+=("gcc" "gcc-c++" "make")
        elif [[ "${PKG_MANAGER}" == "pacman" ]]; then
            DEPS_TO_INSTALL+=("base-devel")
        fi
    fi

    # Check Git (required by FetchContent for bdwgc)
    echo -n "Checking for Git... "
    if command -v git &> /dev/null; then
        GIT_VERSION=$(git --version | awk '{print $3}')
        echo -e "${GREEN}✓ found (${GIT_VERSION})${NC}"
    else
        echo -e "${RED}✗ not found${NC}"
        MISSING_DEPS+=("git")
        DEPS_TO_INSTALL+=("git")
    fi

    # Check Make
    echo -n "Checking for Make... "
    if command -v make &> /dev/null; then
        MAKE_VERSION=$(make --version | head -n1)
        echo -e "${GREEN}✓ found${NC}"
    else
        echo -e "${RED}✗ not found${NC}"
        MISSING_DEPS+=("make")
        if [[ "${PLATFORM}" != "macOS" ]]; then
            DEPS_TO_INSTALL+=("make")
        fi
    fi

    # Check Doxygen (optional, for documentation)
    echo -n "Checking for Doxygen (optional)... "
    if command -v doxygen &> /dev/null; then
        DOXYGEN_VERSION=$(doxygen --version)
        echo -e "${GREEN}✓ found (${DOXYGEN_VERSION})${NC}"
    else
        echo -e "${YELLOW}⚠ not found (documentation will not be built)${NC}"
    fi

    echo ""

    # Summary
    if [[ ${#MISSING_DEPS[@]} -eq 0 ]]; then
        echo "========================================="
        echo -e "${GREEN}✓ All required dependencies are installed!${NC}"
        echo "========================================="
        echo ""
        echo "You can now build AK24 with:"
        echo "  ./ak24.sh install"
        echo ""
        exit 0
    fi

    echo "========================================="
    echo -e "${YELLOW}Missing Dependencies${NC}"
    echo "========================================="
    for dep in "${MISSING_DEPS[@]}"; do
        echo "  - ${dep}"
    done
    echo ""

    # Offer to install
    if [[ ${#DEPS_TO_INSTALL[@]} -gt 0 ]] && [[ "${PKG_MANAGER}" != "unknown" ]]; then
        echo "Install missing dependencies? [Y/n] "
        read -r response

        if [[ "${response}" =~ ^[Nn]$ ]]; then
            echo "Installation cancelled."
            exit 0
        fi

        echo ""
        echo -e "${BLUE}Installing dependencies...${NC}"
        echo ""

        if [[ "${PLATFORM}" == "macOS" ]]; then
            # Check if Homebrew is installed
            if ! command -v brew &> /dev/null; then
                echo "Homebrew not found. Install from: https://brew.sh"
                echo ""
                echo "Or run:"
                echo '  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"'
                exit 1
            fi

            for dep in "${DEPS_TO_INSTALL[@]}"; do
                echo "Installing ${dep}..."
                brew install "${dep}" || true
            done
        elif [[ "${PKG_MANAGER}" == "apt" ]]; then
            echo "Updating package list..."
            sudo apt-get update
            for dep in "${DEPS_TO_INSTALL[@]}"; do
                echo "Installing ${dep}..."
                sudo apt-get install -y "${dep}" || true
            done
        elif [[ "${PKG_MANAGER}" == "dnf" ]]; then
            for dep in "${DEPS_TO_INSTALL[@]}"; do
                echo "Installing ${dep}..."
                sudo dnf install -y "${dep}" || true
            done
        elif [[ "${PKG_MANAGER}" == "yum" ]]; then
            for dep in "${DEPS_TO_INSTALL[@]}"; do
                echo "Installing ${dep}..."
                sudo yum install -y "${dep}" || true
            done
        elif [[ "${PKG_MANAGER}" == "pacman" ]]; then
            for dep in "${DEPS_TO_INSTALL[@]}"; do
                echo "Installing ${dep}..."
                sudo pacman -S --noconfirm "${dep}" || true
            done
        fi

        echo ""
        echo "========================================="
        echo -e "${GREEN}✓ Dependencies installed${NC}"
        echo "========================================="
        echo ""
        echo "Run preflight again to verify:"
        echo "  ./ak24.sh preflight"
        echo ""
    else
        if [[ "${PLATFORM}" == "macOS" ]]; then
            echo "Install Xcode Command Line Tools:"
            echo "  xcode-select --install"
            echo ""
            if [[ ${#DEPS_TO_INSTALL[@]} -gt 0 ]]; then
                echo "Install other dependencies with Homebrew:"
                echo "  brew install ${DEPS_TO_INSTALL[*]}"
                echo ""
            fi
        else
            echo "Please install the missing dependencies manually."
            echo ""
        fi
    fi
}

cmd_status() {
    echo "========================================="
    echo "AK24 Installation Status"
    echo "========================================="
    echo ""
    echo "Installation directory: ${AK24_HOME}"
    echo ""

    if check_installed; then
        echo -e "${GREEN}✓ AK24 is installed${NC}"
        echo ""

        # Show version if available
        if [[ -f "${SCRIPT_DIR}/VERSION" ]]; then
            VERSION=$(cat "${SCRIPT_DIR}/VERSION")
            echo "Version: ${VERSION}"
        fi

        # Show some key files
        echo ""
        echo "Key files:"
        [[ -f "${AK24_HOME}/include/ak24/kernel.h" ]] && echo "  ✓ Headers installed"
        [[ -f "${AK24_HOME}/lib/libak24_kernel.a" ]] && echo "  ✓ Static library installed"

        if [[ -d "${AK24_HOME}/share/doc/ak24/html" ]]; then
            echo "  ✓ Documentation installed"
        fi

        echo ""
        echo "Library location: ${AK24_HOME}/lib"
        echo "Headers location: ${AK24_HOME}/include/ak24"

        if [[ -d "${AK24_HOME}/share/doc/ak24/html" ]]; then
            echo "Documentation:    ${AK24_HOME}/share/doc/ak24/html/index.html"
        fi
    else
        echo -e "${RED}✗ AK24 is not installed${NC}"
        echo ""
        echo "To install AK24, run:"
        echo "  ./ak24.sh install"
    fi

    echo ""
}

cmd_install() {
    echo "========================================="
    echo "Installing AK24"
    echo "========================================="
    echo ""
    echo "Build mode: ${AK24_BUILD_MODE}"
    echo ""

    cd "${SCRIPT_DIR}"

    # Step 1: Clean
    echo -e "${BLUE}Step 1: Cleaning previous builds...${NC}"
    make clean
    echo ""

    # Step 2: Configure build
    echo -e "${BLUE}Step 2: Configuring CMake...${NC}"
    configure_build
    echo ""

    # Step 3: Build documentation
    echo -e "${BLUE}Step 3: Building documentation...${NC}"
    if command -v doxygen &> /dev/null; then
        cd "${SCRIPT_DIR}"
        make docs
        echo -e "${GREEN}✓ Documentation built successfully${NC}"
    else
        echo -e "${YELLOW}⚠ Doxygen not found, skipping documentation${NC}"
    fi
    echo ""

    # Step 4: Install
    echo -e "${BLUE}Step 4: Building and installing AK24...${NC}"
    echo ""
    echo "This may require sudo privileges for system installation."
    echo "If AK24_HOME is set to a user directory, sudo is not needed."
    echo ""

    if [[ "${AK24_HOME}" == "${HOME}/.ak24" ]] || [[ "${AK24_HOME}" == ${HOME}/* ]]; then
        # User installation, no sudo needed
        cd "${SCRIPT_DIR}"
        make install
    else
        # System installation, may need sudo
        cd "${SCRIPT_DIR}"
        if [[ $EUID -ne 0 ]]; then
            echo "System installation detected, you may be prompted for password..."
            sudo make install
        else
            make install
        fi
    fi

    echo ""

    # Verify installation
    if check_installed; then
        echo "========================================="
        echo -e "${GREEN}✓ AK24 installed successfully!${NC}"
        echo "========================================="
        echo ""
        echo "Installation location: ${AK24_HOME}"
        echo ""
        echo "To run integration tests:"
        echo "  ./ak24.sh test"
        echo ""

        if [[ -d "${AK24_HOME}/share/doc/ak24/html" ]]; then
            echo "View documentation at:"
            echo "  open ${AK24_HOME}/share/doc/ak24/html/index.html"
            echo ""
        fi
    else
        echo "========================================="
        echo -e "${RED}✗ Installation verification failed${NC}"
        echo "========================================="
        echo ""
        echo "Expected files not found at: ${AK24_HOME}"
        exit 1
    fi
}

cmd_test() {
    echo "========================================="
    echo "Running Full AK24 Test Suite"
    echo "========================================="
    echo ""
    echo "Build mode: ${AK24_BUILD_MODE}"
    echo ""

    cd "${SCRIPT_DIR}"

    # Step 1: Clean build
    echo -e "${BLUE}Step 1: Cleaning previous builds...${NC}"
    make clean
    echo ""

    # Step 2: Configure build
    echo -e "${BLUE}Step 2: Configuring CMake...${NC}"
    configure_build
    echo ""

    # Step 3: Build and install
    echo -e "${BLUE}Step 3: Building and installing AK24...${NC}"
    echo "This will also run compile-time tests..."
    echo ""

    if [[ "${AK24_HOME}" == "${HOME}/.ak24" ]] || [[ "${AK24_HOME}" == ${HOME}/* ]]; then
        make install
    else
        if [[ $EUID -ne 0 ]]; then
            sudo make install
        else
            make install
        fi
    fi

    echo ""

    # Verify installation before running integration tests
    if ! check_installed; then
        echo -e "${RED}✗ Installation failed, cannot run integration tests${NC}"
        exit 1
    fi

    echo -e "${GREEN}✓ AK24 installed successfully${NC}"
    echo ""

    # Step 4: Run compile-time tests
    echo -e "${BLUE}Step 4: Running compile-time tests...${NC}"
    echo ""

    make test

    echo ""
    echo -e "${GREEN}✓ Compile-time tests passed${NC}"
    echo ""

    # Step 5: Run integration tests
    echo -e "${BLUE}Step 5: Running integration tests...${NC}"
    echo ""

    cd "${SCRIPT_DIR}/tests"

    if [[ -f "run.sh" ]]; then
        bash run.sh
        TEST_RESULT=$?

        echo ""

        if [[ ${TEST_RESULT} -eq 0 ]]; then
            echo "========================================="
            echo -e "${GREEN}✓ All tests passed!${NC}"
            echo "========================================="
            echo ""
            echo "Compile-time tests: ✓ Passed"
            echo "Integration tests:  ✓ Passed"
            echo ""
            return 0
        else
            echo "========================================="
            echo -e "${RED}✗ Integration tests failed${NC}"
            echo "========================================="
            return 1
        fi
    else
        echo -e "${YELLOW}⚠ Integration test runner not found: tests/run.sh${NC}"
        return 1
    fi
}

cmd_uninstall() {
    echo "========================================="
    echo "Uninstalling AK24"
    echo "========================================="
    echo ""
    echo "Installation directory: ${AK24_HOME}"
    echo ""

    if ! check_installed; then
        echo -e "${YELLOW}⚠ AK24 is not installed at ${AK24_HOME}${NC}"
        echo ""
        if [[ -d "${AK24_HOME}" ]]; then
            echo "Directory exists but does not appear to contain a valid AK24 installation."
            echo -n "Remove directory anyway? [y/N] "
            read -r response
            if [[ ! "${response}" =~ ^[Yy]$ ]]; then
                echo "Uninstall cancelled."
                exit 0
            fi
        else
            echo "Directory does not exist."
            exit 0
        fi
    else
        echo -e "${YELLOW}This will completely remove AK24 from:${NC}"
        echo "  ${AK24_HOME}"
        echo ""
        echo -n "Are you sure? [y/N] "
        read -r response

        if [[ ! "${response}" =~ ^[Yy]$ ]]; then
            echo "Uninstall cancelled."
            exit 0
        fi
    fi

    echo ""
    echo -e "${BLUE}Removing AK24 installation...${NC}"

    # Check if we need sudo
    if [[ -w "${AK24_HOME}" ]] || [[ ! -e "${AK24_HOME}" ]]; then
        # We have write permission or directory doesn't exist
        rm -rf "${AK24_HOME}"
    else
        # Need sudo
        echo "Removing system installation (may require password)..."
        sudo rm -rf "${AK24_HOME}"
    fi

    echo ""

    if [[ -d "${AK24_HOME}" ]]; then
        echo "========================================="
        echo -e "${RED}✗ Failed to remove ${AK24_HOME}${NC}"
        echo "========================================="
        exit 1
    else
        echo "========================================="
        echo -e "${GREEN}✓ AK24 uninstalled successfully${NC}"
        echo "========================================="
        echo ""
        echo "AK24 has been completely removed from your system."
        echo ""
    fi
}

cmd_ci() {
    echo "========================================="
    echo "AK24 Continuous Integration Test Suite"
    echo "========================================="
    echo ""
    echo "This will run the complete test suite across all configurations:"
    echo "  1. GC mode (production)"
    echo "  2. ASAN mode (memory debugging)"
    echo "  3. Manual mode (no GC, no ASAN)"
    echo ""

    # Store original build mode to restore later
    ORIGINAL_BUILD_MODE="${AK24_BUILD_MODE}"

    # Configuration 1: GC Mode
    echo "========================================="
    echo "Configuration 1/3: GC Mode"
    echo "========================================="
    echo ""
    export AK24_BUILD_MODE=gc

    if ! cmd_test; then
        echo ""
        echo "========================================="
        echo -e "${RED}✗ CI FAILED: GC mode tests failed${NC}"
        echo "========================================="
        exit 1
    fi

    echo ""
    echo -e "${GREEN}✓ GC mode tests passed${NC}"
    echo ""

    # Configuration 2: ASAN Mode
    echo "========================================="
    echo "Configuration 2/3: ASAN Mode"
    echo "========================================="
    echo ""
    export AK24_BUILD_MODE=asan

    if ! cmd_test; then
        echo ""
        echo "========================================="
        echo -e "${RED}✗ CI FAILED: ASAN mode tests failed${NC}"
        echo "========================================="
        exit 1
    fi

    echo ""
    echo -e "${GREEN}✓ ASAN mode tests passed${NC}"
    echo ""

    # Configuration 3: Manual Mode (No GC, No ASAN)
    echo "========================================="
    echo "Configuration 3/3: Manual Mode (No GC, No ASAN)"
    echo "========================================="
    echo ""

    cd "${SCRIPT_DIR}"

    # Clean previous build
    echo -e "${BLUE}Cleaning previous builds...${NC}"
    make clean
    echo ""

    # Configure with no GC and no ASAN
    echo -e "${BLUE}Configuring CMake (GC=OFF, ASAN=OFF)...${NC}"
    mkdir -p "${SCRIPT_DIR}/build"
    cd "${SCRIPT_DIR}/build"
    cmake -DCMAKE_INSTALL_PREFIX="${AK24_HOME}" \
          -DAK24_GC_ENABLED=OFF \
          -DAK24_BUILD_ASAN=OFF \
          ..
    echo ""

    # Build and install
    echo -e "${BLUE}Building and installing AK24...${NC}"
    cd "${SCRIPT_DIR}"

    if [[ "${AK24_HOME}" == "${HOME}/.ak24" ]] || [[ "${AK24_HOME}" == ${HOME}/* ]]; then
        make install
    else
        if [[ $EUID -ne 0 ]]; then
            sudo make install
        else
            make install
        fi
    fi

    # Verify installation
    if ! check_installed; then
        echo -e "${RED}✗ Installation failed${NC}"
        exit 1
    fi
    echo ""

    # Run compile-time tests
    echo -e "${BLUE}Running compile-time tests...${NC}"
    make test
    echo ""
    echo -e "${GREEN}✓ Compile-time tests passed${NC}"
    echo ""

    # Run integration tests
    echo -e "${BLUE}Running integration tests...${NC}"
    cd "${SCRIPT_DIR}/tests"

    if [[ ! -f "run.sh" ]]; then
        echo -e "${RED}✗ Integration test runner not found: tests/run.sh${NC}"
        exit 1
    fi

    if ! bash run.sh; then
        echo ""
        echo "========================================="
        echo -e "${RED}✗ CI FAILED: Manual mode tests failed${NC}"
        echo "========================================="
        exit 1
    fi

    echo ""
    echo -e "${GREEN}✓ Manual mode tests passed${NC}"
    echo ""

    # Clean up: uninstall
    cd "${SCRIPT_DIR}"
    echo "========================================="
    echo "Cleaning up: Uninstalling AK24"
    echo "========================================="
    echo ""

    if [[ -w "${AK24_HOME}" ]] || [[ ! -e "${AK24_HOME}" ]]; then
        rm -rf "${AK24_HOME}"
    else
        sudo rm -rf "${AK24_HOME}"
    fi

    # Final report
    echo ""
    echo "========================================="
    echo -e "${GREEN}✓ CI PASSED: All configurations successful!${NC}"
    echo "========================================="
    echo ""
    echo "Test results:"
    echo "  ✓ GC mode (production):           PASSED"
    echo "  ✓ ASAN mode (memory debugging):   PASSED"
    echo "  ✓ Manual mode (no GC, no ASAN):   PASSED"
    echo ""
    echo "All tests completed successfully across all build configurations."
    echo ""

    # Restore original build mode
    export AK24_BUILD_MODE="${ORIGINAL_BUILD_MODE}"
}

# Main command dispatcher
if [[ $# -eq 0 ]]; then
    print_usage
    exit 1
fi

COMMAND="$1"

case "${COMMAND}" in
    preflight)
        cmd_preflight
        ;;
    install)
        cmd_install
        ;;
    uninstall)
        cmd_uninstall
        ;;
    status)
        cmd_status
        ;;
    test)
        cmd_test
        exit $?
        ;;
    ci)
        cmd_ci
        ;;
    help|-h|--help)
        print_usage
        exit 0
        ;;
    *)
        echo -e "${RED}Error: Unknown command '${COMMAND}'${NC}"
        echo ""
        print_usage
        exit 1
        ;;
esac
