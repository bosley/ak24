include(FetchContent)

if(AK24_GC_ENABLED)
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
        target_compile_definitions(gc PRIVATE LARGE_CONFIG=1)
        target_compile_options(gc PRIVATE
            -Wno-deprecated-declarations
            -Wno-strict-prototypes
            -Wno-macro-redefined
            -Wno-string-concatenation
        )
    endif()
endif()

find_library(TCC_LIBRARY NAMES tcc libtcc)
find_path(TCC_INCLUDE_DIR NAMES libtcc.h)

if(TCC_LIBRARY AND TCC_INCLUDE_DIR)
    message(STATUS "TCC: FOUND (system)")
    message(STATUS "  - Library: ${TCC_LIBRARY}")
    message(STATUS "  - Include: ${TCC_INCLUDE_DIR}")
else()
    message(STATUS "TCC: NOT FOUND (fetching from source)")

    FetchContent_Declare(
        tcc
        GIT_REPOSITORY https://github.com/AK24org/tcc.git
        GIT_TAG main
    )

    FetchContent_GetProperties(tcc)
    if(NOT tcc_POPULATED)
        FetchContent_Populate(tcc)

        set(TCC_SOURCE_DIR ${tcc_SOURCE_DIR})
        set(TCC_BINARY_DIR ${tcc_BINARY_DIR})

        if(UNIX OR APPLE)
            execute_process(
                COMMAND ${TCC_SOURCE_DIR}/configure
                    --prefix=${TCC_BINARY_DIR}/install
                    --enable-static
                WORKING_DIRECTORY ${TCC_BINARY_DIR}
                RESULT_VARIABLE TCC_CONFIGURE_RESULT
            )

            if(TCC_CONFIGURE_RESULT EQUAL 0)
                execute_process(
                    COMMAND make -j4
                    WORKING_DIRECTORY ${TCC_BINARY_DIR}
                    RESULT_VARIABLE TCC_BUILD_RESULT
                )

                if(TCC_BUILD_RESULT EQUAL 0)
                    set(TCC_LIBRARY ${TCC_BINARY_DIR}/libtcc.a)
                    set(TCC_INCLUDE_DIR ${TCC_SOURCE_DIR})
                    message(STATUS "TCC: BUILT from source")
                    message(STATUS "  - Library: ${TCC_LIBRARY}")
                    message(STATUS "  - Include: ${TCC_INCLUDE_DIR}")
                else()
                    message(FATAL_ERROR "TCC: build failed - CJIT requires TCC")
                endif()
            else()
                message(FATAL_ERROR "TCC: configure failed - CJIT requires TCC")
            endif()
        else()
            message(FATAL_ERROR "TCC: Windows build from source not yet supported - CJIT requires TCC")
        endif()
    endif()
endif()

find_package(OpenSSL QUIET)
if(OPENSSL_FOUND)
    set(AK24_TLS_ENABLED ON CACHE BOOL "TLS support enabled" FORCE)
    message(STATUS "OpenSSL: FOUND (version ${OPENSSL_VERSION})")
    message(STATUS "  - TLS/SSL support: ENABLED")
else()
    set(AK24_TLS_ENABLED OFF CACHE BOOL "TLS support disabled" FORCE)
    message(STATUS "OpenSSL: NOT FOUND")
    message(STATUS "  - TLS/SSL support: DISABLED")
    message(STATUS "  - Install OpenSSL to enable TLS (e.g., brew install openssl)")
endif()

file(GLOB_RECURSE KERNEL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/map/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/list/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/context/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lambda/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arbuff/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/scanner/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/threads/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/log/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/intern/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arena/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sourceloc/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utf8/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/filepath/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/box/src/*.c
)

# Add base kernel sources (non-platform/GC specific)
list(APPEND KERNEL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/kernel.c)
list(APPEND KERNEL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/modules.c)
list(APPEND KERNEL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime.c)

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

# Add CJIT sources
list(APPEND KERNEL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cjit/src/cjit.c)

file(GLOB_RECURSE KERNEL_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/map/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/list/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/context/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lambda/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arbuff/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/scanner/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/threads/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/intern/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arena/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sourceloc/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utf8/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/filepath/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/box/test/*.c
)

file(GLOB_RECURSE KERNEL_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/map/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/list/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/context/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/lambda/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/arbuff/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/scanner/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/threads/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/log/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/intern/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/arena/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sourceloc/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/utf8/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/filepath/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cjit/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/box/include/*.h
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
    ${CMAKE_CURRENT_SOURCE_DIR}/threads/include
    ${CMAKE_CURRENT_SOURCE_DIR}/scanner/include
    ${CMAKE_CURRENT_SOURCE_DIR}/log/include
    ${CMAKE_CURRENT_SOURCE_DIR}/intern/include
    ${CMAKE_CURRENT_SOURCE_DIR}/arena/include
    ${CMAKE_CURRENT_SOURCE_DIR}/utf8/include
    ${CMAKE_CURRENT_SOURCE_DIR}/sourceloc/include
    ${CMAKE_CURRENT_SOURCE_DIR}/filepath/include
    ${CMAKE_CURRENT_SOURCE_DIR}/utf8/include
    ${CMAKE_CURRENT_SOURCE_DIR}/cjit/include
    ${CMAKE_CURRENT_SOURCE_DIR}/box/include
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

target_include_directories(ak24_kernel PRIVATE ${TCC_INCLUDE_DIR})
target_link_libraries(ak24_kernel PUBLIC ${TCC_LIBRARY})

if(AK24_TLS_ENABLED)
    target_compile_definitions(ak24_kernel PUBLIC AK24_TLS_ENABLED=1)
    target_link_libraries(ak24_kernel PUBLIC OpenSSL::SSL OpenSSL::Crypto)
else()
    target_compile_definitions(ak24_kernel PUBLIC AK24_TLS_ENABLED=0)
endif()

# Apply ASAN flags if enabled
if(AK24_BUILD_ASAN)
    target_compile_options(ak24_kernel PUBLIC
        -fsanitize=address
        -fno-omit-frame-pointer
        -g
    )
    target_link_options(ak24_kernel PUBLIC
        -fsanitize=address
    )
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
    ${CMAKE_CURRENT_SOURCE_DIR}/threads/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/scanner/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/intern/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arena/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sourceloc/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utf8/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/filepath/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cjit/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/box/test/*.c
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
    ${CMAKE_CURRENT_SOURCE_DIR}/cjit/test/extended_tests.c
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

if(NOT AK24_BUILD_SHARED_KERNEL AND UNIX AND AK24_GC_ENABLED AND TARGET gc)
    install(CODE "
        set(KERNEL_LIB \"${CMAKE_INSTALL_PREFIX}/lib/libak24_kernel.a\")
        set(GC_LIB \"$<TARGET_FILE:gc>\")
        set(TCC_LIB \"${TCC_LIBRARY}\")
        set(TEMP_DIR \"${CMAKE_BINARY_DIR}/bundle_temp\")

        file(MAKE_DIRECTORY \"\${TEMP_DIR}\")

        execute_process(
            COMMAND ar x \"\${KERNEL_LIB}\"
            WORKING_DIRECTORY \"\${TEMP_DIR}\"
        )

        file(MAKE_DIRECTORY \"\${TEMP_DIR}/gc\")
        execute_process(
            COMMAND ar x \"\${GC_LIB}\"
            WORKING_DIRECTORY \"\${TEMP_DIR}/gc\"
        )

        file(MAKE_DIRECTORY \"\${TEMP_DIR}/tcc\")
        execute_process(
            COMMAND ar x \"\${TCC_LIB}\"
            WORKING_DIRECTORY \"\${TEMP_DIR}/tcc\"
        )

        file(GLOB ALL_OBJS
            \"\${TEMP_DIR}/*.o\"
            \"\${TEMP_DIR}/gc/*.o\"
            \"\${TEMP_DIR}/tcc/*.o\"
        )

        execute_process(
            COMMAND ar rcs \"\${KERNEL_LIB}\" \${ALL_OBJS}
        )

        execute_process(
            COMMAND ranlib \"\${KERNEL_LIB}\"
        )

        file(REMOVE_RECURSE \"\${TEMP_DIR}\")

        message(STATUS \"Bundled gc and tcc into libak24_kernel.a\")
    ")
elseif(NOT AK24_BUILD_SHARED_KERNEL AND UNIX)
    install(CODE "
        set(KERNEL_LIB \"${CMAKE_INSTALL_PREFIX}/lib/libak24_kernel.a\")
        set(TCC_LIB \"${TCC_LIBRARY}\")
        set(TEMP_DIR \"${CMAKE_BINARY_DIR}/bundle_temp\")

        file(MAKE_DIRECTORY \"\${TEMP_DIR}\")

        execute_process(
            COMMAND ar x \"\${KERNEL_LIB}\"
            WORKING_DIRECTORY \"\${TEMP_DIR}\"
        )

        file(MAKE_DIRECTORY \"\${TEMP_DIR}/tcc\")
        execute_process(
            COMMAND ar x \"\${TCC_LIB}\"
            WORKING_DIRECTORY \"\${TEMP_DIR}/tcc\"
        )

        file(GLOB ALL_OBJS
            \"\${TEMP_DIR}/*.o\"
            \"\${TEMP_DIR}/tcc/*.o\"
        )

        execute_process(
            COMMAND ar rcs \"\${KERNEL_LIB}\" \${ALL_OBJS}
        )

        execute_process(
            COMMAND ranlib \"\${KERNEL_LIB}\"
        )

        file(REMOVE_RECURSE \"\${TEMP_DIR}\")

        message(STATUS \"Bundled tcc into libak24_kernel.a\")
    ")
endif()

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
    ${CMAKE_CURRENT_SOURCE_DIR}/scanner/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/log/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/intern/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/arena/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/sourceloc/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/filepath/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/cjit/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/api/v1/include/
    DESTINATION include/ak24
    FILES_MATCHING PATTERN "*.h"
)


