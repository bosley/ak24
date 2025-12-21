cmake_minimum_required(VERSION 3.20)

add_executable(arena-demo main.c)
target_link_libraries(arena-demo PRIVATE ak24_kernel)
target_include_directories(arena-demo PRIVATE
    ${CMAKE_SOURCE_DIR}/kernel
)

set_target_properties(arena-demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
