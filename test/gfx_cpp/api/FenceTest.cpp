#include "CommonTest.h"

#include <memory>

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxCppFenceTest : public testing::TestWithParam<gfx::Backend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        try {
            gfx::InstanceDescriptor instDesc{
                .backend = backend,
                .enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG }
            };

            instance = gfx::createInstance(instDesc);

            gfx::AdapterDescriptor adapterDesc{
                .adapterIndex = 0
            };

            adapter = instance->requestAdapter(adapterDesc);

            gfx::DeviceDescriptor deviceDesc{
                .label = "Test Device"
            };

            device = adapter->createDevice(deviceDesc);
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up: " << e.what();
        }
    }

    void TearDown() override
    {
        device.reset();
        adapter.reset();
        instance.reset();
    }

    gfx::Backend backend = gfx::Backend::Vulkan;
    std::shared_ptr<gfx::Instance> instance;
    std::shared_ptr<gfx::Adapter> adapter;
    std::shared_ptr<gfx::Device> device;
};

// Functional tests
TEST_P(GfxCppFenceTest, CreateAndDestroy)
{
    ASSERT_NE(device, nullptr);

    gfx::FenceDescriptor fenceDesc{
        .label = "Test Fence"
    };

    auto fence = device->createFence(fenceDesc);
    EXPECT_NE(fence, nullptr);
}

TEST_P(GfxCppFenceTest, CreateWithDefaultDescriptor)
{
    ASSERT_NE(device, nullptr);

    auto fence = device->createFence();
    EXPECT_NE(fence, nullptr);
}

TEST_P(GfxCppFenceTest, InitialStatusUnsignaled)
{
    ASSERT_NE(device, nullptr);

    gfx::FenceDescriptor fenceDesc{
        .signaled = false
    };

    auto fence = device->createFence(fenceDesc);
    ASSERT_NE(fence, nullptr);

    auto status = fence->getStatus();
    EXPECT_EQ(status, gfx::FenceStatus::Unsignaled);
}

TEST_P(GfxCppFenceTest, InitialStatusSignaled)
{
    ASSERT_NE(device, nullptr);

    gfx::FenceDescriptor fenceDesc{
        .signaled = true
    };

    auto fence = device->createFence(fenceDesc);
    ASSERT_NE(fence, nullptr);

    auto status = fence->getStatus();
    EXPECT_EQ(status, gfx::FenceStatus::Signaled);
}

TEST_P(GfxCppFenceTest, WaitOnSignaledFence)
{
    ASSERT_NE(device, nullptr);

    gfx::FenceDescriptor fenceDesc{
        .signaled = true
    };

    auto fence = device->createFence(fenceDesc);
    ASSERT_NE(fence, nullptr);

    // Should return immediately since fence is already signaled
    auto result = fence->wait(0);
    EXPECT_TRUE(gfx::isSuccess(result));
}

TEST_P(GfxCppFenceTest, WaitOnUnsignaledFenceWithZeroTimeout)
{
    ASSERT_NE(device, nullptr);

    gfx::FenceDescriptor fenceDesc{
        .signaled = false
    };

    auto fence = device->createFence(fenceDesc);
    ASSERT_NE(fence, nullptr);

    // Should return with timeout since fence is not signaled
    auto result = fence->wait(0);
    EXPECT_EQ(result, gfx::Result::Timeout);
}

TEST_P(GfxCppFenceTest, ResetSignaledFence)
{
    ASSERT_NE(device, nullptr);

    gfx::FenceDescriptor fenceDesc{
        .signaled = true
    };

    auto fence = device->createFence(fenceDesc);
    ASSERT_NE(fence, nullptr);

    auto status = fence->getStatus();
    EXPECT_EQ(status, gfx::FenceStatus::Signaled);

    fence->reset();

    status = fence->getStatus();
    EXPECT_EQ(status, gfx::FenceStatus::Unsignaled);
}

TEST_P(GfxCppFenceTest, ResetUnsignaledFence)
{
    ASSERT_NE(device, nullptr);

    gfx::FenceDescriptor fenceDesc{
        .signaled = false
    };

    auto fence = device->createFence(fenceDesc);
    ASSERT_NE(fence, nullptr);

    auto status = fence->getStatus();
    EXPECT_EQ(status, gfx::FenceStatus::Unsignaled);

    // Reset should be safe even if already unsignaled
    EXPECT_NO_THROW(fence->reset());

    status = fence->getStatus();
    EXPECT_EQ(status, gfx::FenceStatus::Unsignaled);
}

TEST_P(GfxCppFenceTest, CreateWithEmptyLabel)
{
    ASSERT_NE(device, nullptr);

    gfx::FenceDescriptor fenceDesc{
        .label = "",
        .signaled = false
    };

    auto fence = device->createFence(fenceDesc);
    EXPECT_NE(fence, nullptr);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppFenceTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

} // namespace
