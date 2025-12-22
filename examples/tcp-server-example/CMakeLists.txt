cmake_minimum_required(VERSION 3.10)

add_executable(tcp-server-example main.c)

target_include_directories(tcp-server-example PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(tcp-server-example PRIVATE ak24_kernel)

set_target_properties(tcp-server-example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS tcp-server-example DESTINATION bin)
