# C++ Wrapper Tests
add_executable(gfx_cpp_test
    basic_test.cpp
    instance_test.cpp
    result_test.cpp
)

target_link_libraries(gfx_cpp_test
    gfx_cpp
    GTest::gtest_main
)

# Discover tests
gtest_discover_tests(gfx_cpp_test)
