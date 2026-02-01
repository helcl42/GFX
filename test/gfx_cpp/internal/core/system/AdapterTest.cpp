#include <core/system/Adapter.h>

#include <gfx/gfx.h>

#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

namespace gfx {

class AdapterImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc = {};
        instanceDesc.backend = backend;
        instanceDesc.applicationName = "AdapterImplTest";
        ASSERT_EQ(gfxCreateInstance(&instanceDesc, &instance), GFX_RESULT_SUCCESS);

        GfxAdapterDescriptor adapterDesc = {};
        ASSERT_EQ(gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter), GFX_RESULT_SUCCESS);
    }

    void TearDown() override
    {
        if (instance) {
            gfxInstanceDestroy(instance);
        }
        gfxUnloadBackend(backend);
    }

    GfxBackend backend;
    GfxInstance instance = nullptr;
    GfxAdapter adapter = nullptr;
};

TEST_P(AdapterImplTest, CreateWrapper)
{
    AdapterImpl wrapper(adapter);
    // Wrapper created successfully
}

TEST_P(AdapterImplTest, GetInfo)
{
    AdapterImpl wrapper(adapter);

    auto info = wrapper.getInfo();

    // Verify some basic info is returned
    EXPECT_GT(info.deviceID, 0u);
    EXPECT_NE(info.adapterType, AdapterType::Unknown);
}

TEST_P(AdapterImplTest, GetLimits)
{
    AdapterImpl wrapper(adapter);

    auto limits = wrapper.getLimits();

    // Verify some reasonable limits are returned
    EXPECT_GT(limits.maxTextureDimension2D, 0u);
    EXPECT_GT(limits.maxBufferSize, 0u);
}

TEST_P(AdapterImplTest, EnumerateQueueFamilies)
{
    AdapterImpl wrapper(adapter);

    auto queueFamilies = wrapper.enumerateQueueFamilies();

    // Should have at least one queue family
    EXPECT_GT(queueFamilies.size(), 0u);

    // First queue family should have some queues
    if (!queueFamilies.empty()) {
        EXPECT_GT(queueFamilies[0].queueCount, 0u);
    }
}

TEST_P(AdapterImplTest, EnumerateExtensions)
{
    AdapterImpl wrapper(adapter);

    auto extensions = wrapper.enumerateExtensions();

    // Extensions list may be empty or contain extensions
    // Just verify it doesn't crash
    EXPECT_GE(extensions.size(), 0u);
}

TEST_P(AdapterImplTest, CreateDevice)
{
    AdapterImpl wrapper(adapter);

    DeviceDescriptor desc = {};
    auto device = wrapper.createDevice(desc);

    EXPECT_NE(device, nullptr);
}

// Instantiate tests for available backends
#if defined(GFX_ENABLE_VULKAN) && defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(AllBackends, AdapterImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU));
#elif defined(GFX_ENABLE_VULKAN)
INSTANTIATE_TEST_SUITE_P(VulkanOnly, AdapterImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN));
#elif defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(WebGPUOnly, AdapterImplTest,
    ::testing::Values(GFX_BACKEND_WEBGPU));
#endif

} // namespace gfx
