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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/log/src/*.c
)

file(GLOB_RECURSE KERNEL_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/map/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/list/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/context/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lambda/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/arbuff/test/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/atoms/test/*.c
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
    ${CMAKE_CURRENT_SOURCE_DIR}/log/include/*.h
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
    ${CMAKE_CURRENT_SOURCE_DIR}/log/include
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

list(REMOVE_ITEM ALL_COMPILE_TIME_TESTS
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer/test/buffer_test_runner.c
    ${BUFFER_TEST_SOURCES}
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

