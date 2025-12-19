add_executable(ak24_cli main.c)

target_include_directories(ak24_cli PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(ak24_cli PRIVATE
    ak24_kernel
)

set_target_properties(ak24_cli PROPERTIES
    OUTPUT_NAME "ak24"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

