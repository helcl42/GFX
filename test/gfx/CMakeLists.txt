# C API Tests (using C++ for GoogleTest)

# Public API tests - tests that verify the C API contract
set(PUBLIC_TEST_SOURCES
    api/ConstantTest.cpp
    api/BackendTest.cpp
    api/InstanceTest.cpp
    api/AdapterTest.cpp
    api/DeviceTest.cpp
    api/BufferTest.cpp
    api/TextureTest.cpp
    api/TextureViewTest.cpp
    api/SamplerTest.cpp
    api/ShaderTest.cpp
    api/BindGroupLayoutTest.cpp
    api/BindGroupTest.cpp
    api/SurfaceTest.cpp
    api/SwapchainTest.cpp
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
