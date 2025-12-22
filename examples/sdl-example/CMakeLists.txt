find_package(SDL2 QUIET)

if(NOT SDL2_FOUND)
    message(STATUS "SDL2 not found - skipping sdl-example")
    return()
endif()

message(STATUS "SDL2 found - building sdl-example")

# Cell grid library (can be used by other examples)
add_library(cellgrid STATIC
    cellgrid.c
    cellgrid.h
)

add_library(entity STATIC
    entity.c
    entity.h
)

target_include_directories(cellgrid PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(cellgrid PUBLIC
    ak24_kernel
    ${SDL2_LIBRARIES}
)

# SDL example executable
add_executable(sdl_example main.c)

target_include_directories(sdl_example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(sdl_example PRIVATE
    cellgrid
    entity
)

set_target_properties(sdl_example PROPERTIES
    OUTPUT_NAME "sdl-example"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

install(TARGETS sdl_example DESTINATION bin)
