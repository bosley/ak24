# CJIT - C JIT Compilation Abstraction

set(CJIT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cjit.c
)

set(CJIT_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/cjit.h
)

# Export sources and headers to parent
set(CJIT_SOURCES ${CJIT_SOURCES} PARENT_SCOPE)
set(CJIT_HEADERS ${CJIT_HEADERS} PARENT_SCOPE)
set(CJIT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)
