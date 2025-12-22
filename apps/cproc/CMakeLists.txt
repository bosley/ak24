add_executable(cproc main.c)

target_link_libraries(cproc PRIVATE ak24_kernel)

target_include_directories(cproc PRIVATE
    ${CMAKE_SOURCE_DIR}
)

set_target_properties(cproc PROPERTIES
    OUTPUT_NAME "cproc"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install cproc executable
install(TARGETS cproc
    RUNTIME DESTINATION bin
)
