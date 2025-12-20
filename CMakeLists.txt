cmake_minimum_required(VERSION 3.20)
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

set(CMAKE_C_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS_RELEASE "-O3 -Wall -Wextra -Wpedantic")

if(AK24_GC_ENABLED)
    add_compile_definitions(AK24_GC_ENABLED=1)
    message(STATUS "Boehm GC: ENABLED")
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

        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)

        message(STATUS "Doxygen: ENABLED (run 'make docs' to generate)")
    else()
        message(WARNING "Doxygen not found - documentation will not be built")
    endif()
endif()

add_subdirectory(kernel)

add_subdirectory(examples/ak24-cli)
add_subdirectory(examples/atom-cube-demo)

