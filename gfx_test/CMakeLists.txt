# C API Tests (using C++ for GoogleTest)
add_executable(gfx_test
    basic_test.cpp
    instance_test.cpp
)

target_link_libraries(gfx_test
    gfx
    GTest::gtest_main
)

# Discover tests
gtest_discover_tests(gfx_test)
