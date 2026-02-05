#include <core/sync/Fence.h>
#include <core/system/Device.h>

#include <gfx/gfx.h>

#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

namespace gfx {

class FenceImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc{
            .backend = backend,
            .applicationName = "FenceImplTest"
        };
        ASSERT_EQ(gfxCreateInstance(&instanceDesc, &instance), GFX_RESULT_SUCCESS);

        GfxAdapterDescriptor adapterDesc{
            .sType = GFX_STRUCTURE_TYPE_ADAPTER_DESCRIPTOR,
            .pNext = nullptr,
            .adapterIndex = 0
        };
        ASSERT_EQ(gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter), GFX_RESULT_SUCCESS);

        GfxDeviceDescriptor deviceDesc{
            .sType = GFX_STRUCTURE_TYPE_DEVICE_DESCRIPTOR,
            .pNext = nullptr,
            .label = nullptr,
            .queueRequests = nullptr,
            .queueRequestCount = 0,
            .enabledExtensions = nullptr,
            .enabledExtensionCount = 0
        };
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

TEST_P(FenceImplTest, CreateAndDestroy)
{
    DeviceImpl deviceWrapper(device);

    FenceDescriptor fenceDesc{
        .label = "Test Fence"
    };

    auto fence = deviceWrapper.createFence(fenceDesc);
    EXPECT_NE(fence, nullptr);
}

TEST_P(FenceImplTest, InitialStatusUnsignaled)
{
    DeviceImpl deviceWrapper(device);

    FenceDescriptor fenceDesc{
        .signaled = false
    };

    auto fence = deviceWrapper.createFence(fenceDesc);
    ASSERT_NE(fence, nullptr);

    FenceStatus status = fence->getStatus();
    EXPECT_EQ(status, FenceStatus::Unsignaled);
}

TEST_P(FenceImplTest, InitialStatusSignaled)
{
    DeviceImpl deviceWrapper(device);

    FenceDescriptor fenceDesc{
        .signaled = true
    };

    auto fence = deviceWrapper.createFence(fenceDesc);
    ASSERT_NE(fence, nullptr);

    FenceStatus status = fence->getStatus();
    EXPECT_EQ(status, FenceStatus::Signaled);
}

TEST_P(FenceImplTest, WaitOnSignaledFence)
{
    DeviceImpl deviceWrapper(device);

    FenceDescriptor fenceDesc{
        .signaled = true
    };

    auto fence = deviceWrapper.createFence(fenceDesc);
    ASSERT_NE(fence, nullptr);

    // Should return immediately since fence is already signaled
    auto result = fence->wait(0);
    EXPECT_TRUE(gfx::isSuccess(result));
}

TEST_P(FenceImplTest, ResetSignaledFence)
{
    DeviceImpl deviceWrapper(device);

    FenceDescriptor fenceDesc{
        .signaled = true
    };

    auto fence = deviceWrapper.createFence(fenceDesc);
    ASSERT_NE(fence, nullptr);

    FenceStatus status = fence->getStatus();
    EXPECT_EQ(status, FenceStatus::Signaled);

    // Reset the fence
    fence->reset();

    // Check status again
    status = fence->getStatus();
    EXPECT_EQ(status, FenceStatus::Unsignaled);
}

TEST_P(FenceImplTest, MultipleFences_IndependentHandles)
{
    DeviceImpl deviceWrapper(device);

    FenceDescriptor fenceDesc{
        .signaled = false
    };

    auto fence1 = deviceWrapper.createFence(fenceDesc);
    auto fence2 = deviceWrapper.createFence(fenceDesc);

    EXPECT_NE(fence1, nullptr);
    EXPECT_NE(fence2, nullptr);
    EXPECT_NE(fence1, fence2);
}

#if defined(GFX_ENABLE_VULKAN) && defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(AllBackends, FenceImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU));
#elif defined(GFX_ENABLE_VULKAN)
INSTANTIATE_TEST_SUITE_P(VulkanOnly, FenceImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN));
#elif defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(WebGPUOnly, FenceImplTest,
    ::testing::Values(GFX_BACKEND_WEBGPU));
#endif

} // namespace gfx
