add_executable(atom_viewer_demo main.c)

target_include_directories(atom_viewer_demo PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(atom_viewer_demo PRIVATE
    ak24_kernel
    ak24_viewer
)

set_target_properties(atom_viewer_demo PROPERTIES
    OUTPUT_NAME "atom-viewer-demo"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
