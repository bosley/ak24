cmake_minimum_required(VERSION 3.10)

add_executable(tcp-client-stressor main.c)

target_include_directories(tcp-client-stressor PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(tcp-client-stressor PRIVATE ak24_kernel)

set_target_properties(tcp-client-stressor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS tcp-client-stressor DESTINATION bin)
