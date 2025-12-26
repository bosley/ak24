cmake_minimum_required(VERSION 3.20)

# Set install prefix from AK24_HOME if available, before project()
if(DEFINED ENV{AK24_HOME})
    set(CMAKE_INSTALL_PREFIX "$ENV{AK24_HOME}" CACHE PATH "Install prefix from AK24_HOME" FORCE)
    message(STATUS "Installing to AK24_HOME: $ENV{AK24_HOME}")
elseif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.ak24" CACHE PATH "Default install prefix" FORCE)
    message(STATUS "AK24_HOME not set, using default: ${CMAKE_INSTALL_PREFIX}")
endif()

project(ak24 VERSION 0.1.2 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.cache/fetchcontent" CACHE PATH "FetchContent cache directory")

option(AK24_BUILD_ASAN "Build with AddressSanitizer for memory error detection" OFF)
option(AK24_GC_ENABLED "Enable Boehm GC for memory management" ON)
option(AK24_BUILD_DEBUG_MEMORY "Enable memory tracking and statistics" OFF)
option(AK24_BUILD_SHARED_KERNEL "Build kernel as shared library" OFF)
option(AK24_BUILD_DOCS "Build API documentation with Doxygen" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# AddressSanitizer configuration
if(AK24_BUILD_ASAN)
    if(AK24_GC_ENABLED)
        message(WARNING "AddressSanitizer enabled - forcing GC OFF to avoid false leak reports")
        set(AK24_GC_ENABLED OFF CACHE BOOL "GC disabled for ASAN compatibility" FORCE)
    endif()

    set(ASAN_FLAGS "-fsanitize=address -fno-omit-frame-pointer -g")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ASAN_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")

    message(STATUS "AddressSanitizer: ENABLED")
    message(STATUS "  - Memory error detection: buffer overflows, use-after-free, leaks")
    message(STATUS "  - Boehm GC: DISABLED (incompatible with ASAN leak detection)")
endif()

# Platform detection
if(WIN32)
    message(STATUS "Platform: Windows")
    add_compile_definitions(AK24_PLATFORM_WINDOWS)
    # Windows-specific compiler flags (MSVC or MinGW)
    if(MSVC)
        set(CMAKE_C_FLAGS_DEBUG "/W4 /Od /Zi")
        set(CMAKE_C_FLAGS_RELEASE "/O2 /W4")
        # Disable specific MSVC warnings if needed
        add_compile_options(/wd4996)  # Disable deprecated function warnings
    else()
        # MinGW or other Windows compiler
        set(CMAKE_C_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
        set(CMAKE_C_FLAGS_RELEASE "-O3 -Wall -Wextra -Wpedantic")
    endif()
elseif(UNIX OR APPLE)
    if(APPLE)
        message(STATUS "Platform: macOS")
    else()
        message(STATUS "Platform: POSIX/Unix")
    endif()
    add_compile_definitions(AK24_PLATFORM_POSIX)
    set(CMAKE_C_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -Wall -Wextra -Wpedantic")
else()
    message(FATAL_ERROR "Unsupported platform - only POSIX and Windows are supported")
endif()

if(AK24_GC_ENABLED)
    add_compile_definitions(AK24_GC_ENABLED=1)
    message(STATUS "Boehm GC: ENABLED")
    if(WIN32)
        message(WARNING "Boehm GC on Windows may require additional configuration")
    endif()
    set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
else()
    add_compile_definitions(AK24_GC_ENABLED=0)
    message(STATUS "Boehm GC: DISABLED (using standard malloc/free)")
endif()

if(AK24_BUILD_DEBUG_MEMORY)
    add_compile_definitions(AK24_BUILD_DEBUG_MEMORY=1)
    message(STATUS "Memory Debug: ENABLED")
else()
    add_compile_definitions(AK24_BUILD_DEBUG_MEMORY=0)
    message(STATUS "Memory Debug: DISABLED")
endif()

if(AK24_BUILD_SHARED_KERNEL)
    message(STATUS "Kernel Library: SHARED")
else()
    message(STATUS "Kernel Library: STATIC")
endif()

if(AK24_BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

        add_custom_target(docs ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)

        # Install documentation
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs/api/html
            DESTINATION share/doc/ak24
            OPTIONAL)

        message(STATUS "Doxygen: ENABLED (docs will be generated during build)")
    else()
        message(WARNING "Doxygen not found - documentation will not be built")
    endif()
endif()

add_subdirectory(kernel)

add_subdirectory(examples/ak24-cli)
add_subdirectory(examples/arena-demo)
add_subdirectory(examples/atom-cube-demo)
add_subdirectory(examples/thread-pool-demo)
add_subdirectory(examples/signals-example)
add_subdirectory(examples/sourceloc-demo)
add_subdirectory(examples/tcp-server-example)
add_subdirectory(examples/tcp-client-example)
add_subdirectory(examples/tcp-client-stressor)

# Install build configuration file for tests/external projects
set(AK24_BUILD_CONFIG_FILE "${CMAKE_BINARY_DIR}/ak24-config.mk")
file(WRITE "${AK24_BUILD_CONFIG_FILE}" "# AK24 Build Configuration\n")
file(APPEND "${AK24_BUILD_CONFIG_FILE}" "# Auto-generated - do not edit\n")
file(APPEND "${AK24_BUILD_CONFIG_FILE}" "AK24_VERSION=${PROJECT_VERSION}\n")
if(AK24_BUILD_ASAN)
    file(APPEND "${AK24_BUILD_CONFIG_FILE}" "AK24_ASAN_ENABLED=1\n")
    file(APPEND "${AK24_BUILD_CONFIG_FILE}" "AK24_ASAN_FLAGS=-fsanitize=address -fno-omit-frame-pointer -g\n")
else()
    file(APPEND "${AK24_BUILD_CONFIG_FILE}" "AK24_ASAN_ENABLED=0\n")
    file(APPEND "${AK24_BUILD_CONFIG_FILE}" "AK24_ASAN_FLAGS=\n")
endif()
if(AK24_GC_ENABLED)
    file(APPEND "${AK24_BUILD_CONFIG_FILE}" "AK24_GC_ENABLED=1\n")
else()
    file(APPEND "${AK24_BUILD_CONFIG_FILE}" "AK24_GC_ENABLED=0\n")
endif()
install(FILES "${AK24_BUILD_CONFIG_FILE}" DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")

set(PKG_CONFIG_FILE "${CMAKE_BINARY_DIR}/ak24.pc")
file(WRITE "${PKG_CONFIG_FILE}" "prefix=${CMAKE_INSTALL_PREFIX}\n")
file(APPEND "${PKG_CONFIG_FILE}" "exec_prefix=\${prefix}\n")
file(APPEND "${PKG_CONFIG_FILE}" "libdir=\${prefix}/lib\n")
file(APPEND "${PKG_CONFIG_FILE}" "includedir=\${prefix}/include\n\n")
file(APPEND "${PKG_CONFIG_FILE}" "Name: ak24\n")
file(APPEND "${PKG_CONFIG_FILE}" "Description: AK24 Kernel Library\n")
file(APPEND "${PKG_CONFIG_FILE}" "Version: ${PROJECT_VERSION}\n")
file(APPEND "${PKG_CONFIG_FILE}" "Cflags: -I\${includedir}/ak24\n")
if(AK24_BUILD_SHARED_KERNEL)
    file(APPEND "${PKG_CONFIG_FILE}" "Libs: -L\${libdir} -lak24_kernel\n")
else()
    file(APPEND "${PKG_CONFIG_FILE}" "Libs: -L\${libdir} -lak24_kernel -lpthread -lssl -lcrypto\n")
endif()
install(FILES "${PKG_CONFIG_FILE}" DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig")

install(PROGRAMS "${CMAKE_SOURCE_DIR}/ak24-config" DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")

# Add uninstall target
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
        COMMENT "Uninstalling AK24...")
endif()

