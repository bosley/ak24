# AK24 Atom Viewer System
# N-dimensional atom visualization library

file(GLOB VIEWER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

file(GLOB VIEWER_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
)

# Create the viewer library (static by default)
add_library(ak24_viewer STATIC ${VIEWER_SOURCES})

# Include directories
target_include_directories(ak24_viewer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
)

# Link against kernel (viewer depends on kernel)
target_link_libraries(ak24_viewer PUBLIC
    ak24_kernel
)

# Set output properties
set_target_properties(ak24_viewer PROPERTIES
    OUTPUT_NAME "ak24_viewer"
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Install viewer library
install(TARGETS ak24_viewer
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

# Install viewer headers
install(FILES ${VIEWER_HEADERS}
    DESTINATION include/ak24/systems/viewer
)

message(STATUS "Configuring atom viewer system library")
