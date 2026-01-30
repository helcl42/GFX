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
    api/RenderPassTest.cpp
    api/FramebufferTest.cpp
    api/RenderPipelineTest.cpp
    api/ComputePipelineTest.cpp
    api/QueueTest.cpp
    api/CommandEncoderTest.cpp
    api/RenderPassEncoderTest.cpp
    api/ComputePassEncoderTest.cpp
    api/FenceTest.cpp
    api/SemaphoreTest.cpp
    api/QuerySetTest.cpp
    api/UtilTest.cpp
    api/SurfaceTest.cpp
    api/SwapchainTest.cpp
)

# Internal tests - tests for Dispatcher, converters, backend internals
set(INTERNAL_TEST_SOURCES
    internal/GfxImplTest.cpp
    internal/common/LoggerTest.cpp
    internal/backend/vulkan/converter/ConversionsTest.cpp
    internal/backend/vulkan/core/resource/BindGroupTest.cpp
    internal/backend/vulkan/core/resource/BindGroupLayoutTest.cpp
    internal/backend/vulkan/core/resource/BufferTest.cpp
    internal/backend/vulkan/core/resource/SamplerTest.cpp
    internal/backend/vulkan/core/resource/ShaderTest.cpp
    internal/backend/vulkan/core/resource/TextureTest.cpp
    internal/backend/vulkan/core/resource/TextureViewTest.cpp
)

add_executable(gfx_test
    ${PUBLIC_TEST_SOURCES}
    ${INTERNAL_TEST_SOURCES}
)

target_link_libraries(gfx_test
    gfx
    GTest::gtest_main
    GTest::gmock
)

# Add include directory for internal tests to access private headers
target_include_directories(gfx_test PRIVATE
    ${CMAKE_SOURCE_DIR}/gfx/src
)

# Discover tests (skip when ASAN is enabled due to runtime initialization issues)
if(NOT ENABLE_ASAN)
    gtest_discover_tests(gfx_test)
endif()
