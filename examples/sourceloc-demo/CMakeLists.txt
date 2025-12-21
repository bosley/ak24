cmake_minimum_required(VERSION 3.20)

add_executable(sourceloc-demo main.c)
target_link_libraries(sourceloc-demo PRIVATE ak24_kernel)
target_include_directories(sourceloc-demo PRIVATE
    ${CMAKE_SOURCE_DIR}/kernel
)

set_target_properties(sourceloc-demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
