# TCP Module - Platform-abstracted TCP server with thread pool
#
# Note: TCP sources are compiled as part of ak24_kernel library.
# This CMakeLists.txt is only for tests.

# Runtime tests
add_executable(ak24_tcp_runtime_tests
    test/runtime_tests.c
    test/hardening_tests.c
)
target_include_directories(ak24_tcp_runtime_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/kernel
)
target_link_libraries(ak24_tcp_runtime_tests PRIVATE ak24_kernel)

# Link winsock on Windows
if(WIN32)
    target_link_libraries(ak24_tcp_runtime_tests PRIVATE ws2_32)
endif()

set_target_properties(ak24_tcp_runtime_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/compile_time
    EXCLUDE_FROM_ALL FALSE
)

add_custom_command(TARGET ak24_tcp_runtime_tests POST_BUILD
    COMMAND ${CMAKE_BINARY_DIR}/test/compile_time/ak24_tcp_runtime_tests
    COMMENT "Running compile-time test: ak24_tcp_runtime_tests"
    VERBATIM
)
