# C++ Wrapper Tests

# Public API tests - tests that verify the C++ wrapper API contract
set(PUBLIC_TEST_SOURCES
    api/BasicTest.cpp
    api/InstanceTest.cpp
    api/ResultTest.cpp
)

# Internal tests - tests for C++ wrapper implementation details
set(INTERNAL_TEST_SOURCES
    # Add internal tests here as needed
)

add_executable(gfx_cpp_test
    ${PUBLIC_TEST_SOURCES}
    ${INTERNAL_TEST_SOURCES}
)

target_link_libraries(gfx_cpp_test
    gfx_cpp
    GTest::gtest_main
)

# Discover tests
gtest_discover_tests(gfx_cpp_test)
