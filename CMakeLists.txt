cmake_minimum_required(VERSION 3.20)

# Set install prefix from AK24_HOME if available, before project()
if(DEFINED ENV{AK24_HOME})
    set(CMAKE_INSTALL_PREFIX "$ENV{AK24_HOME}" CACHE PATH "Install prefix from AK24_HOME" FORCE)
    message(STATUS "Installing to AK24_HOME: $ENV{AK24_HOME}")
elseif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.ak24" CACHE PATH "Default install prefix" FORCE)
    message(STATUS "AK24_HOME not set, using default: ${CMAKE_INSTALL_PREFIX}")
endif()

project(ak24 VERSION 0.0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.cache/fetchcontent" CACHE PATH "FetchContent cache directory")

option(AK24_GC_ENABLED "Enable Boehm GC for memory management" ON)
option(AK24_BUILD_DEBUG_MEMORY "Enable memory tracking and statistics" OFF)
option(AK24_BUILD_SHARED_KERNEL "Build kernel as shared library" OFF)
option(AK24_BUILD_DOCS "Build API documentation with Doxygen" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
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

