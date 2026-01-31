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
    internal/backend/vulkan/core/sync/FenceTest.cpp
    internal/backend/vulkan/core/sync/SemaphoreTest.cpp
    internal/backend/vulkan/core/system/AdapterTest.cpp
    internal/backend/vulkan/core/system/DeviceTest.cpp
    internal/backend/vulkan/core/system/InstanceTest.cpp
    internal/backend/vulkan/core/system/QueueTest.cpp
    internal/backend/vulkan/core/util/UtilsTest.cpp
    internal/backend/vulkan/core/render/FramebufferTest.cpp
    internal/backend/vulkan/core/render/RenderPassTest.cpp
    internal/backend/vulkan/core/render/RenderPipelineTest.cpp
    internal/backend/vulkan/core/query/QuerySetTest.cpp
    internal/backend/vulkan/core/presentation/SurfaceTest.cpp
    internal/backend/vulkan/core/presentation/SwapchainTest.cpp
    internal/backend/vulkan/core/compute/ComputePipelineTest.cpp
    internal/backend/vulkan/core/command/CommandEncoderTest.cpp
    internal/backend/vulkan/core/command/ComputePassEncoderTest.cpp
    internal/backend/vulkan/core/command/RenderPassEncoderTest.cpp
    internal/backend/webgpu/core/system/InstanceTest.cpp
    internal/backend/webgpu/core/system/AdapterTest.cpp
    internal/backend/webgpu/core/system/DeviceTest.cpp
    internal/backend/webgpu/core/system/QueueTest.cpp
    internal/backend/webgpu/core/resource/BufferTest.cpp
    internal/backend/webgpu/core/resource/TextureTest.cpp
    internal/backend/webgpu/core/resource/TextureViewTest.cpp
    internal/backend/webgpu/core/resource/SamplerTest.cpp
    internal/backend/webgpu/core/resource/ShaderTest.cpp
    internal/backend/webgpu/core/resource/BindGroupLayoutTest.cpp
    internal/backend/webgpu/core/resource/BindGroupTest.cpp
    internal/backend/webgpu/core/sync/FenceTest.cpp
    internal/backend/webgpu/core/sync/SemaphoreTest.cpp
    internal/backend/webgpu/core/render/RenderPassTest.cpp
    internal/backend/webgpu/core/render/FramebufferTest.cpp
    internal/backend/webgpu/core/render/RenderPipelineTest.cpp
    internal/backend/webgpu/core/compute/ComputePipelineTest.cpp
    internal/backend/webgpu/core/query/QuerySetTest.cpp
    internal/backend/webgpu/core/presentation/SurfaceTest.cpp
    internal/backend/webgpu/core/presentation/SwapchainTest.cpp
    internal/backend/webgpu/core/command/CommandEncoderTest.cpp
    internal/backend/webgpu/core/command/ComputePassEncoderTest.cpp
    internal/backend/webgpu/core/command/RenderPassEncoderTest.cpp
    internal/backend/webgpu/core/util/BlitTest.cpp
    internal/backend/webgpu/core/util/UtilsTest.cpp
    internal/backend/webgpu/converter/ConversionsTest.cpp
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
