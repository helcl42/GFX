# C API Tests (using C++ for GoogleTest)

# Public API tests - tests that verify the C API contract
set(PUBLIC_TEST_SOURCES
    api/basic_test.cpp
    api/instance_test.cpp
)

# Internal tests - tests for Dispatcher, converters, backend internals
set(INTERNAL_TEST_SOURCES
    # Add internal tests here as needed
)

add_executable(gfx_test
    ${PUBLIC_TEST_SOURCES}
    ${INTERNAL_TEST_SOURCES}
)

target_link_libraries(gfx_test
    gfx
    GTest::gtest_main
)

# Discover tests
gtest_discover_tests(gfx_test)
