cmake_minimum_required(VERSION 3.10)

# Signals Example Executable
add_executable(signals-example main.c)

# Include kernel headers
target_include_directories(signals-example PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# Link with AK24 kernel
target_link_libraries(signals-example PRIVATE ak24_kernel)

# Set output directory
set_target_properties(signals-example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Installation rules
install(TARGETS signals-example DESTINATION bin)
