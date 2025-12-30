cmake_minimum_required(VERSION 3.20)

include(FetchContent)

# Try to find SDL2 locally first
find_package(SDL2 QUIET)

if(NOT SDL2_FOUND)
    # Try using pkg-config as fallback
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2 QUIET sdl2)
    endif()
endif()

# If still not found, fetch from source
if(NOT SDL2_FOUND)
    message(STATUS "SDL2 not found locally, fetching from source...")

    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.28.5
        GIT_SHALLOW TRUE
    )

    # Disable SDL2 tests and examples
    set(SDL2_DISABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(SDL2)

    set(SDL2_FOUND TRUE)
    message(STATUS "SDL2 fetched successfully")
endif()

add_executable(sdl-example main.c)

# Include kernel headers
target_include_directories(sdl-example PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(sdl-example PRIVATE ak24_kernel)

# Link SDL2
if(TARGET SDL2::SDL2)
    target_link_libraries(sdl-example PRIVATE SDL2::SDL2)
elseif(TARGET SDL2::SDL2-static)
    target_link_libraries(sdl-example PRIVATE SDL2::SDL2-static)
elseif(TARGET SDL2-static)
    target_link_libraries(sdl-example PRIVATE SDL2-static)
else()
    # FetchContent or pkg-config found
    if(TARGET SDL2)
        target_link_libraries(sdl-example PRIVATE SDL2)
    else()
        target_include_directories(sdl-example PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(sdl-example PRIVATE ${SDL2_LIBRARIES})
    endif()
endif()

set_target_properties(sdl-example PROPERTIES
    OUTPUT_NAME "sdl-example"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

message(STATUS "SDL2 example configured successfully")
