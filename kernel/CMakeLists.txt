if(AK24_GC_ENABLED)
    include(FetchContent)

    FetchContent_Declare(
        bdwgc
        GIT_REPOSITORY https://github.com/ivmai/bdwgc.git
        GIT_TAG v8.2.6
    )

    set(enable_threads ON CACHE BOOL "" FORCE)
    set(enable_cplusplus OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(disable_handle_fork ON CACHE BOOL "" FORCE)
    set(install_headers OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(bdwgc)

    if(TARGET gc)
        target_compile_options(gc PRIVATE
            -Wno-deprecated-declarations
            -Wno-strict-prototypes
            -Wno-macro-redefined
            -Wno-string-concatenation
        )
    endif()
endif()

file(GLOB_RECURSE KERNEL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/map/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/list/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/context/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lambda/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arbuff/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/atoms/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/scanner/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/forms/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/threads/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/log/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/intern/src/*.c
)

# Add base kernel sources (non-platform/GC specific)
list(APPEND KERNEL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel.c)
list(APPEND KERNEL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/modules.c)

# Platform-specific threading sources
if(WIN32)
    file(GLOB THREADS_PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/threads/src/win/*.c
    )
    # Add Windows-specific kernel sources
    list(APPEND KERNEL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/win.c)
else()
    file(GLOB THREADS_PLATFORM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/threads/src/nix/*.c
    )
    # Add POSIX-specific kernel sources
    list(APPEND KERNEL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/posix.c)
endif()

# Exclude platform-specific directories from main glob
list(FILTER KERNEL_SOURCES EXCLUDE REGEX ".*/src/(nix|win)/.*")

# Add platform-specific sources
list(APPEND KERNEL_SOURCES ${THREADS_PLATFORM_SOURCES})

# Add GC or non-GC specific sources
if(AK24_GC_ENABLED)
    list(APPEND KERNEL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/gc.c)
else()
    list(APPEND KERNEL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/nogc.c)
endif()

file(GLOB_RECURSE KERNEL_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/map/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/list/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/context/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lambda/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arbuff/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/atoms/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/scanner/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/forms/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/threads/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/intern/test/*.c
)

file(GLOB_RECURSE KERNEL_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/map/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/list/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/context/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lambda/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/arbuff/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/atoms/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scanner/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/threads/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/forms/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/log/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/intern/include/*.h
)

if(AK24_BUILD_SHARED_KERNEL)
    add_library(ak24_kernel SHARED ${KERNEL_SOURCES})
    set_target_properties(ak24_kernel PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        POSITION_INDEPENDENT_CODE ON
    )
else()
    add_library(ak24_kernel STATIC ${KERNEL_SOURCES})
endif()

target_include_directories(ak24_kernel PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/map/include
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer/include
    ${CMAKE_CURRENT_SOURCE_DIR}/list/include
    ${CMAKE_CURRENT_SOURCE_DIR}/context/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lambda/include
    ${CMAKE_CURRENT_SOURCE_DIR}/arbuff/include
    ${CMAKE_CURRENT_SOURCE_DIR}/atoms/include
    ${CMAKE_CURRENT_SOURCE_DIR}/threads/include
    ${CMAKE_CURRENT_SOURCE_DIR}/scanner/include
    ${CMAKE_CURRENT_SOURCE_DIR}/forms/include
    ${CMAKE_CURRENT_SOURCE_DIR}/log/include
    ${CMAKE_CURRENT_SOURCE_DIR}/intern/include
    ${CMAKE_CURRENT_SOURCE_DIR}/api/v1/include
)

if(AK24_GC_ENABLED)
    target_link_libraries(ak24_kernel PUBLIC gc)
    if(APPLE)
        target_link_options(ak24_kernel PUBLIC
            -Wl,-no_warn_duplicate_libraries
            -Wl,-w
        )
    endif()
endif()

# Platform-specific threading libraries
if(UNIX OR APPLE)
    target_link_libraries(ak24_kernel PUBLIC pthread)
    message(STATUS "Threading: POSIX pthreads")
elseif(WIN32)
    # Windows threading is built into kernel32.lib (linked by default on Windows)
    # Explicitly link if needed for MinGW
    if(NOT MSVC)
        target_link_libraries(ak24_kernel PUBLIC kernel32)
    endif()
    message(STATUS "Threading: Windows native (kernel32)")
endif()

set_target_properties(ak24_kernel PROPERTIES
    OUTPUT_NAME "ak24_kernel"
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

file(GLOB_RECURSE ALL_COMPILE_TIME_TESTS
    ${CMAKE_CURRENT_SOURCE_DIR}/arbuff/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lambda/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/list/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/map/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/context/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/atoms/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/threads/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/scanner/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/forms/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/intern/test/*.c
)

file(GLOB BUFFER_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer/test/buffer_basic_operations_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer/test/buffer_rotate_trim_copy_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer/test/buffer_split_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer/test/buffer_sub_buffer_tests.c
)

set(TEST_TARGET "ak24_buffer_test_runner")
set(TEST_BINARY "${CMAKE_BINARY_DIR}/test/compile_time/${TEST_TARGET}")
add_executable(${TEST_TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer/test/buffer_test_runner.c
    ${BUFFER_TEST_SOURCES}
)
target_link_libraries(${TEST_TARGET} PRIVATE ak24_kernel)
target_include_directories(${TEST_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer/include
    ${CMAKE_CURRENT_SOURCE_DIR}/test
)
set_target_properties(${TEST_TARGET} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/compile_time
    EXCLUDE_FROM_ALL FALSE
)
add_custom_command(TARGET ${TEST_TARGET} POST_BUILD
    COMMAND ${TEST_BINARY}
    COMMENT "Running compile-time test: ${TEST_TARGET}"
    VERBATIM
)

file(GLOB SCANNER_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/scanner/test/scanner_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/scanner/test/scanner_find_group_tests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/scanner/test/scanner_stress_test.c
)

set(TEST_TARGET "ak24_scanner_test_runner")
set(TEST_BINARY "${CMAKE_BINARY_DIR}/test/compile_time/${TEST_TARGET}")
add_executable(${TEST_TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/scanner/test/scanner_test_runner.c
    ${SCANNER_TEST_SOURCES}
)
target_link_libraries(${TEST_TARGET} PRIVATE ak24_kernel)
target_include_directories(${TEST_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/scanner/include
    ${CMAKE_CURRENT_SOURCE_DIR}/test
)
set_target_properties(${TEST_TARGET} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/compile_time
    EXCLUDE_FROM_ALL FALSE
)
add_custom_command(TARGET ${TEST_TARGET} POST_BUILD
    COMMAND ${TEST_BINARY}
    COMMENT "Running compile-time test: ${TEST_TARGET}"
    VERBATIM
)

list(REMOVE_ITEM ALL_COMPILE_TIME_TESTS
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer/test/buffer_test_runner.c
    ${BUFFER_TEST_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/scanner/test/scanner_test_runner.c
    ${SCANNER_TEST_SOURCES}
)

foreach(TEST_FILE ${ALL_COMPILE_TIME_TESTS})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    get_filename_component(TEST_DIR ${TEST_FILE} DIRECTORY)
    get_filename_component(MODULE_NAME ${TEST_DIR} DIRECTORY)
    get_filename_component(MODULE_NAME ${MODULE_NAME} NAME)

    set(TEST_TARGET "ak24_${MODULE_NAME}_${TEST_NAME}")
    set(TEST_BINARY "${CMAKE_BINARY_DIR}/test/compile_time/${TEST_TARGET}")

    add_executable(${TEST_TARGET} ${TEST_FILE})
    target_link_libraries(${TEST_TARGET} PRIVATE ak24_kernel)
    target_include_directories(${TEST_TARGET} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_NAME}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/test
    )

    set_target_properties(${TEST_TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/compile_time
        EXCLUDE_FROM_ALL FALSE
    )

    add_custom_command(TARGET ${TEST_TARGET} POST_BUILD
        COMMAND ${TEST_BINARY}
        COMMENT "Running compile-time test: ${TEST_TARGET}"
        VERBATIM
    )
endforeach()

# Install garbage collector library
if(AK24_GC_ENABLED AND TARGET gc)
    install(TARGETS gc
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        COMPONENT gc
    )
endif()

# Install kernel library
install(TARGETS ak24_kernel
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install kernel headers
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ak_debug.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ak_nix.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ak_win.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ak_gc.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ak_nogc.h
    ${CMAKE_CURRENT_SOURCE_DIR}/application.h
    ${CMAKE_CURRENT_SOURCE_DIR}/interfaces.h
    ${CMAKE_CURRENT_SOURCE_DIR}/modules.h
    DESTINATION include/ak24
)

install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/map/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/list/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/context/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/lambda/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/arbuff/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/threads/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/atoms/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/scanner/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/forms/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/log/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/intern/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/api/v1/include/
    DESTINATION include/ak24
    FILES_MATCHING PATTERN "*.h"
)


