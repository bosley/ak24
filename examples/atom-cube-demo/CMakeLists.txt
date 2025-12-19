add_executable(atom_cube_demo main.c)

target_include_directories(atom_cube_demo PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(atom_cube_demo PRIVATE
    ak24_kernel
)

set_target_properties(atom_cube_demo PROPERTIES
    OUTPUT_NAME "atom-cube-demo"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
