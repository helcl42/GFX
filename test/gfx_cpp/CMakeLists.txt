# C++ Wrapper Tests

# =============================================================================
# Public API Tests - Always built, backend-agnostic
# =============================================================================
add_executable(gfx_cpp_api_test
    api/AdapterTest.cpp
    api/BackendTest.cpp
    api/BindGroupLayoutTest.cpp
    api/BindGroupTest.cpp
    api/BufferTest.cpp
    api/CommandEncoderTest.cpp
    api/ComputePassEncoderTest.cpp
    api/ComputePipelineTest.cpp
    api/ConstantTest.cpp
    api/DeviceTest.cpp
    api/FenceTest.cpp
    api/FramebufferTest.cpp
    api/InstanceTest.cpp
    api/QuerySetTest.cpp
    api/QueueTest.cpp
    api/RenderPassEncoderTest.cpp
    api/RenderPassTest.cpp
    api/RenderPipelineTest.cpp
    api/SamplerTest.cpp
    api/SemaphoreTest.cpp
    api/ShaderTest.cpp
    api/SurfaceTest.cpp
    api/SwapchainTest.cpp
    api/TextureTest.cpp
    api/TextureViewTest.cpp
    api/UtilTest.cpp
)

target_link_libraries(gfx_cpp_api_test
    gfx_cpp
    GTest::gtest_main
)

if(NOT ENABLE_ASAN)
    gtest_discover_tests(gfx_cpp_api_test)
endif()

# =============================================================================
# Internal Tests - Always built
# =============================================================================
add_executable(gfx_cpp_internal_test
    internal/GfxImplTest.cpp
)

target_link_libraries(gfx_cpp_internal_test
    gfx_cpp
    GTest::gtest_main
)

if(NOT ENABLE_ASAN)
    gtest_discover_tests(gfx_cpp_internal_test)
endif()
