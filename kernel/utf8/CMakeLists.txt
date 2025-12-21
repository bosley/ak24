# UTF-8 Utility Module
add_library(ak24_utf8 STATIC
    src/utf8.c
)

target_include_directories(ak24_utf8 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ak24_utf8 PUBLIC
    ak24_kernel
)

# UTF-8 Tests
add_executable(ak24_utf8_tests
    test/utf8_test_runner.c
)

target_link_libraries(ak24_utf8_tests PRIVATE
    ak24_utf8
    ak24_kernel
)

# Add test
add_test(NAME utf8_tests COMMAND ak24_utf8_tests)
