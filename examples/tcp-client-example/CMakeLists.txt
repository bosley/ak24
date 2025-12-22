cmake_minimum_required(VERSION 3.10)

add_executable(tcp-client-example main.c)

target_include_directories(tcp-client-example PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(tcp-client-example PRIVATE ak24_kernel)

set_target_properties(tcp-client-example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS tcp-client-example DESTINATION bin)
