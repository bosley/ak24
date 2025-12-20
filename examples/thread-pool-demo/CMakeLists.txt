cmake_minimum_required(VERSION 3.20)

add_executable(thread-pool-demo main.c)

target_link_libraries(thread-pool-demo PRIVATE ak24_kernel)

set_target_properties(thread-pool-demo PROPERTIES
    OUTPUT_NAME "thread-pool-demo"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
