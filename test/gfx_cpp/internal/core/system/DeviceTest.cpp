#include <core/system/Device.h>

#include <gfx/gfx.h>

#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

namespace gfx {

class DeviceImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc = {};
        instanceDesc.backend = backend;
        instanceDesc.applicationName = "DeviceImplTest";
        ASSERT_EQ(gfxCreateInstance(&instanceDesc, &instance), GFX_RESULT_SUCCESS);

        GfxAdapterDescriptor adapterDesc = {};
        adapterDesc.preference = GFX_ADAPTER_PREFERENCE_HIGH_PERFORMANCE;
        ASSERT_EQ(gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter), GFX_RESULT_SUCCESS);

        GfxDeviceDescriptor deviceDesc = {};
        ASSERT_EQ(gfxAdapterCreateDevice(adapter, &deviceDesc, &device), GFX_RESULT_SUCCESS);
    }

    void TearDown() override
    {
        if (device) {
            gfxDeviceDestroy(device);
        }
        if (instance) {
            gfxInstanceDestroy(instance);
        }
        gfxUnloadBackend(backend);
    }

    GfxBackend backend;
    GfxInstance instance = nullptr;
    GfxAdapter adapter = nullptr;
    GfxDevice device = nullptr;
};

TEST_P(DeviceImplTest, CreateWrapper)
{
    DeviceImpl wrapper(device);
    // Wrapper created successfully
}

TEST_P(DeviceImplTest, GetQueue)
{
    DeviceImpl wrapper(device);
    auto queue = wrapper.getQueue();
    EXPECT_NE(queue, nullptr);
}

TEST_P(DeviceImplTest, GetQueueByIndex)
{
    DeviceImpl wrapper(device);
    auto queue = wrapper.getQueueByIndex(0, 0);
    EXPECT_NE(queue, nullptr);
}

TEST_P(DeviceImplTest, WaitIdle)
{
    DeviceImpl wrapper(device);
    // Should not crash
    wrapper.waitIdle();
}

TEST_P(DeviceImplTest, GetLimits)
{
    DeviceImpl wrapper(device);
    auto limits = wrapper.getLimits();

    // Verify some reasonable limits are returned
    EXPECT_GT(limits.maxTextureDimension2D, 0u);
    EXPECT_GT(limits.maxBufferSize, 0u);
}

TEST_P(DeviceImplTest, SupportsShaderFormat_SPIRV)
{
    DeviceImpl wrapper(device);
    bool supported = wrapper.supportsShaderFormat(ShaderSourceType::SPIRV);

    // Both Vulkan and WebGPU support SPIR-V (except Emscripten)
    EXPECT_TRUE(supported);
}

TEST_P(DeviceImplTest, SupportsShaderFormat_WGSL)
{
    DeviceImpl wrapper(device);
    bool supported = wrapper.supportsShaderFormat(ShaderSourceType::WGSL);

    // Vulkan doesn't support WGSL, WebGPU does
    if (backend == GFX_BACKEND_VULKAN) {
        EXPECT_FALSE(supported);
    } else {
        EXPECT_TRUE(supported);
    }
}

// Instantiate tests for available backends
#if defined(GFX_ENABLE_VULKAN) && defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(AllBackends, DeviceImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU));
#elif defined(GFX_ENABLE_VULKAN)
INSTANTIATE_TEST_SUITE_P(VulkanOnly, DeviceImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN));
#elif defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(WebGPUOnly, DeviceImplTest,
    ::testing::Values(GFX_BACKEND_WEBGPU));
#endif

} // namespace gfx
