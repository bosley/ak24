# AK24 Systems
# Optional higher-level systems that build on top of the kernel
# Each system is a separate library that can be included independently

cmake_minimum_required(VERSION 3.20)

# Option to enable/disable viewer system
option(AK24_BUILD_VIEWER "Build the atom viewer system" ON)

# Add viewer system if enabled
if(AK24_BUILD_VIEWER)
    message(STATUS "Atom Viewer System: ENABLED")
    add_subdirectory(viewer)
else()
    message(STATUS "Atom Viewer System: DISABLED")
endif()

# Future systems can be added here with similar options:
# option(AK24_BUILD_PHYSICS "Build the physics system" ON)
# if(AK24_BUILD_PHYSICS)
#     add_subdirectory(physics)
# endif()
