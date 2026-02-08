#include <backend/vulkan/core/sync/Fence.h>
#include <backend/vulkan/core/system/Adapter.h>
#include <backend/vulkan/core/system/Device.h>
#include <backend/vulkan/core/system/Instance.h>

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

// Test Vulkan core Fence class
// These tests verify the internal fence implementation, not the public API

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class VulkanFenceTest : public testing::Test {
protected:
    void SetUp() override
    {
        try {
            gfx::backend::vulkan::core::InstanceCreateInfo instInfo{};
            instInfo.enabledExtensions = {};
            instance = std::make_unique<gfx::backend::vulkan::core::Instance>(instInfo);

            gfx::backend::vulkan::core::AdapterCreateInfo adapterInfo{};
            adapterInfo.adapterIndex = 0;
            adapter = instance->requestAdapter(adapterInfo);

            gfx::backend::vulkan::core::DeviceCreateInfo deviceInfo{};
            device = std::make_unique<gfx::backend::vulkan::core::Device>(adapter, deviceInfo);
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up Vulkan: " << e.what();
        }
    }

    std::unique_ptr<gfx::backend::vulkan::core::Instance> instance;
    gfx::backend::vulkan::core::Adapter* adapter = nullptr;
    std::unique_ptr<gfx::backend::vulkan::core::Device> device;
};

// ============================================================================
// Basic Creation Tests
// ============================================================================

TEST_F(VulkanFenceTest, CreateUnsignaledFence_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::FenceCreateInfo createInfo{};
    createInfo.signaled = false;

    gfx::backend::vulkan::core::Fence fence(device.get(), createInfo);

    EXPECT_NE(fence.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanFenceTest, CreateSignaledFence_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::FenceCreateInfo createInfo{};
    createInfo.signaled = true;

    gfx::backend::vulkan::core::Fence fence(device.get(), createInfo);

    EXPECT_NE(fence.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanFenceTest, CreateMultipleFences_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::FenceCreateInfo createInfo{};
    createInfo.signaled = false;

    gfx::backend::vulkan::core::Fence fence1(device.get(), createInfo);
    gfx::backend::vulkan::core::Fence fence2(device.get(), createInfo);
    gfx::backend::vulkan::core::Fence fence3(device.get(), createInfo);

    EXPECT_NE(fence1.handle(), VK_NULL_HANDLE);
    EXPECT_NE(fence2.handle(), VK_NULL_HANDLE);
    EXPECT_NE(fence3.handle(), VK_NULL_HANDLE);
    EXPECT_NE(fence1.handle(), fence2.handle());
    EXPECT_NE(fence2.handle(), fence3.handle());
    EXPECT_NE(fence1.handle(), fence3.handle());
}

// ============================================================================
// Status Tests
// ============================================================================

TEST_F(VulkanFenceTest, GetStatusUnsignaled_ReturnsUnsignaled)
{
    gfx::backend::vulkan::core::FenceCreateInfo createInfo{};
    createInfo.signaled = false;

    gfx::backend::vulkan::core::Fence fence(device.get(), createInfo);

    bool isSignaled = true;
    VkResult result = fence.getStatus(&isSignaled);

    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_FALSE(isSignaled);
}

TEST_F(VulkanFenceTest, GetStatusSignaled_ReturnsSignaled)
{
    gfx::backend::vulkan::core::FenceCreateInfo createInfo{};
    createInfo.signaled = true;

    gfx::backend::vulkan::core::Fence fence(device.get(), createInfo);

    bool isSignaled = false;
    VkResult result = fence.getStatus(&isSignaled);

    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_TRUE(isSignaled);
}

TEST_F(VulkanFenceTest, GetStatusMultipleTimes_ReturnsConsistentState)
{
    gfx::backend::vulkan::core::FenceCreateInfo createInfo{};
    createInfo.signaled = true;

    gfx::backend::vulkan::core::Fence fence(device.get(), createInfo);

    bool isSignaled1 = false;
    bool isSignaled2 = false;
    bool isSignaled3 = false;

    fence.getStatus(&isSignaled1);
    fence.getStatus(&isSignaled2);
    fence.getStatus(&isSignaled3);

    EXPECT_TRUE(isSignaled1);
    EXPECT_TRUE(isSignaled2);
    EXPECT_TRUE(isSignaled3);
}

// ============================================================================
// Wait Tests
// ============================================================================

TEST_F(VulkanFenceTest, WaitSignaledFence_ReturnsImmediately)
{
    gfx::backend::vulkan::core::FenceCreateInfo createInfo{};
    createInfo.signaled = true;

    gfx::backend::vulkan::core::Fence fence(device.get(), createInfo);

    auto start = std::chrono::high_resolution_clock::now();
    VkResult result = fence.wait(1000000000); // 1 second timeout
    auto end = std::chrono::high_resolution_clock::now();

    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_LT(durationMs, 100); // Should return almost immediately
}

TEST_F(VulkanFenceTest, WaitUnsignaledFenceZeroTimeout_ReturnsTimeout)
{
    gfx::backend::vulkan::core::FenceCreateInfo createInfo{};
    createInfo.signaled = false;

    gfx::backend::vulkan::core::Fence fence(device.get(), createInfo);

    VkResult result = fence.wait(0); // Zero timeout

    EXPECT_EQ(result, VK_TIMEOUT);
}

TEST_F(VulkanFenceTest, WaitUnsignaledFenceShortTimeout_ReturnsTimeout)
{
    gfx::backend::vulkan::core::FenceCreateInfo createInfo{};
    createInfo.signaled = false;

    gfx::backend::vulkan::core::Fence fence(device.get(), createInfo);

    auto start = std::chrono::high_resolution_clock::now();
    VkResult result = fence.wait(10000000); // 10ms timeout
    auto end = std::chrono::high_resolution_clock::now();

    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_EQ(result, VK_TIMEOUT);
    EXPECT_GE(durationMs, 9); // Should wait at least ~10ms
    EXPECT_LE(durationMs, 100); // But not much longer
}

TEST_F(VulkanFenceTest, WaitInfiniteTimeout_WaitsForSignal)
{
    gfx::backend::vulkan::core::FenceCreateInfo createInfo{};
    createInfo.signaled = true;

    gfx::backend::vulkan::core::Fence fence(device.get(), createInfo);

    VkResult result = fence.wait(UINT64_MAX); // Infinite timeout

    EXPECT_EQ(result, VK_SUCCESS);
}

// ============================================================================
// Reset Tests
// ============================================================================

TEST_F(VulkanFenceTest, ResetSignaledFence_UnsignalsFence)
{
    gfx::backend::vulkan::core::FenceCreateInfo createInfo{};
    createInfo.signaled = true;

    gfx::backend::vulkan::core::Fence fence(device.get(), createInfo);

    bool isSignaledBefore = false;
    fence.getStatus(&isSignaledBefore);
    EXPECT_TRUE(isSignaledBefore);

    fence.reset();

    bool isSignaledAfter = true;
    fence.getStatus(&isSignaledAfter);
    EXPECT_FALSE(isSignaledAfter);
}

TEST_F(VulkanFenceTest, ResetUnsignaledFence_RemainsUnsignaled)
{
    gfx::backend::vulkan::core::FenceCreateInfo createInfo{};
    createInfo.signaled = false;

    gfx::backend::vulkan::core::Fence fence(device.get(), createInfo);

    fence.reset();

    bool isSignaled = true;
    fence.getStatus(&isSignaled);
    EXPECT_FALSE(isSignaled);
}

TEST_F(VulkanFenceTest, ResetMultipleTimes_WorksCorrectly)
{
    gfx::backend::vulkan::core::FenceCreateInfo createInfo{};
    createInfo.signaled = true;

    gfx::backend::vulkan::core::Fence fence(device.get(), createInfo);

    fence.reset();
    fence.reset();
    fence.reset();

    bool isSignaled = true;
    fence.getStatus(&isSignaled);
    EXPECT_FALSE(isSignaled);
}

// ============================================================================
// Lifecycle Tests
// ============================================================================

TEST_F(VulkanFenceTest, ResetWaitSignaledCycle_WorksCorrectly)
{
    gfx::backend::vulkan::core::FenceCreateInfo createInfo{};
    createInfo.signaled = true;

    gfx::backend::vulkan::core::Fence fence(device.get(), createInfo);

    // Signaled -> wait succeeds
    VkResult result1 = fence.wait(0);
    EXPECT_EQ(result1, VK_SUCCESS);

    // Reset -> unsignaled
    fence.reset();
    bool isSignaled = true;
    fence.getStatus(&isSignaled);
    EXPECT_FALSE(isSignaled);

    // Unsignaled -> wait times out
    VkResult result2 = fence.wait(0);
    EXPECT_EQ(result2, VK_TIMEOUT);
}

TEST_F(VulkanFenceTest, MultipleWaitsSameSignaledFence_AllSucceed)
{
    gfx::backend::vulkan::core::FenceCreateInfo createInfo{};
    createInfo.signaled = true;

    gfx::backend::vulkan::core::Fence fence(device.get(), createInfo);

    VkResult result1 = fence.wait(1000000000);
    VkResult result2 = fence.wait(1000000000);
    VkResult result3 = fence.wait(1000000000);

    EXPECT_EQ(result1, VK_SUCCESS);
    EXPECT_EQ(result2, VK_SUCCESS);
    EXPECT_EQ(result3, VK_SUCCESS);
}

// ============================================================================
// Handle Tests
// ============================================================================

TEST_F(VulkanFenceTest, GetHandle_ReturnsValidHandle)
{
    gfx::backend::vulkan::core::FenceCreateInfo createInfo{};
    createInfo.signaled = false;

    gfx::backend::vulkan::core::Fence fence(device.get(), createInfo);

    VkFence handle = fence.handle();
    EXPECT_NE(handle, VK_NULL_HANDLE);

    // Multiple calls should return same handle
    EXPECT_EQ(fence.handle(), handle);
}

TEST_F(VulkanFenceTest, MultipleFences_HaveUniqueHandles)
{
    gfx::backend::vulkan::core::FenceCreateInfo createInfo{};
    createInfo.signaled = false;

    gfx::backend::vulkan::core::Fence fence1(device.get(), createInfo);
    gfx::backend::vulkan::core::Fence fence2(device.get(), createInfo);

    EXPECT_NE(fence1.handle(), fence2.handle());
}

// ============================================================================
// Use Case Tests
// ============================================================================

TEST_F(VulkanFenceTest, TypicalRenderLoopPattern_WorksCorrectly)
{
    // Typical pattern: create fence, submit work, wait, reset, repeat
    gfx::backend::vulkan::core::FenceCreateInfo createInfo{};
    createInfo.signaled = false;

    gfx::backend::vulkan::core::Fence fence(device.get(), createInfo);

    // Simulate multiple frames
    for (int i = 0; i < 3; ++i) {
        // Initially unsignaled (or reset from previous frame)
        bool isSignaled = true;
        fence.getStatus(&isSignaled);
        EXPECT_FALSE(isSignaled);

        // In real usage, work would be submitted here and fence would be signaled by GPU
        // For testing, we can't actually signal it from GPU, so we skip the wait

        // Reset for next frame
        fence.reset();
    }
}

TEST_F(VulkanFenceTest, CreateManyFences_AllWorkCorrectly)
{
    std::vector<std::unique_ptr<gfx::backend::vulkan::core::Fence>> fences;

    gfx::backend::vulkan::core::FenceCreateInfo createInfo{};
    createInfo.signaled = false;

    // Create many fences
    for (int i = 0; i < 100; ++i) {
        fences.push_back(std::make_unique<gfx::backend::vulkan::core::Fence>(device.get(), createInfo));
    }

    // Verify all are valid and unique
    for (size_t i = 0; i < fences.size(); ++i) {
        EXPECT_NE(fences[i]->handle(), VK_NULL_HANDLE);

        bool isSignaled = true;
        fences[i]->getStatus(&isSignaled);
        EXPECT_FALSE(isSignaled);

        // Check uniqueness with a few others
        if (i > 0) {
            EXPECT_NE(fences[i]->handle(), fences[i - 1]->handle());
        }
    }
}

TEST_F(VulkanFenceTest, SignaledFenceImmediateReuse_WorksCorrectly)
{
    gfx::backend::vulkan::core::FenceCreateInfo createInfo{};
    createInfo.signaled = true;

    gfx::backend::vulkan::core::Fence fence(device.get(), createInfo);

    // Wait immediately (should succeed)
    EXPECT_EQ(fence.wait(0), VK_SUCCESS);

    // Reset and check
    fence.reset();
    bool isSignaled = true;
    fence.getStatus(&isSignaled);
    EXPECT_FALSE(isSignaled);

    // Try to wait (should timeout)
    EXPECT_EQ(fence.wait(0), VK_TIMEOUT);
}

} // namespace
