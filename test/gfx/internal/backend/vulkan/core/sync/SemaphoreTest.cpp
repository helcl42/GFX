#include <backend/vulkan/core/sync/Semaphore.h>
#include <backend/vulkan/core/system/Adapter.h>
#include <backend/vulkan/core/system/Device.h>
#include <backend/vulkan/core/system/Instance.h>

#include <gtest/gtest.h>

#include <chrono>

// Test Vulkan core Semaphore class
// These tests verify the internal semaphore implementation, not the public API

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class VulkanSemaphoreTest : public testing::Test {
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
            deviceInfo.enabledExtensions = { "gfx_timeline_semaphore" };
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
// Binary Semaphore Creation Tests
// ============================================================================

TEST_F(VulkanSemaphoreTest, CreateBinarySemaphore_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Binary;
    createInfo.initialValue = 0;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    EXPECT_NE(semaphore.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(semaphore.getType(), gfx::backend::vulkan::core::SemaphoreType::Binary);
}

TEST_F(VulkanSemaphoreTest, CreateMultipleBinarySemaphores_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Binary;
    createInfo.initialValue = 0;

    gfx::backend::vulkan::core::Semaphore sem1(device.get(), createInfo);
    gfx::backend::vulkan::core::Semaphore sem2(device.get(), createInfo);
    gfx::backend::vulkan::core::Semaphore sem3(device.get(), createInfo);

    EXPECT_NE(sem1.handle(), VK_NULL_HANDLE);
    EXPECT_NE(sem2.handle(), VK_NULL_HANDLE);
    EXPECT_NE(sem3.handle(), VK_NULL_HANDLE);
    EXPECT_NE(sem1.handle(), sem2.handle());
    EXPECT_NE(sem2.handle(), sem3.handle());
    EXPECT_NE(sem1.handle(), sem3.handle());
}

// ============================================================================
// Timeline Semaphore Creation Tests
// ============================================================================

TEST_F(VulkanSemaphoreTest, CreateTimelineSemaphoreZeroValue_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 0;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    EXPECT_NE(semaphore.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(semaphore.getType(), gfx::backend::vulkan::core::SemaphoreType::Timeline);
}

TEST_F(VulkanSemaphoreTest, CreateTimelineSemaphoreNonZeroValue_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 100;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    EXPECT_NE(semaphore.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(semaphore.getType(), gfx::backend::vulkan::core::SemaphoreType::Timeline);
}

TEST_F(VulkanSemaphoreTest, CreateMultipleTimelineSemaphores_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo1{};
    createInfo1.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo1.initialValue = 0;

    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo2{};
    createInfo2.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo2.initialValue = 50;

    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo3{};
    createInfo3.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo3.initialValue = 1000;

    gfx::backend::vulkan::core::Semaphore sem1(device.get(), createInfo1);
    gfx::backend::vulkan::core::Semaphore sem2(device.get(), createInfo2);
    gfx::backend::vulkan::core::Semaphore sem3(device.get(), createInfo3);

    EXPECT_NE(sem1.handle(), VK_NULL_HANDLE);
    EXPECT_NE(sem2.handle(), VK_NULL_HANDLE);
    EXPECT_NE(sem3.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Timeline Semaphore Value Tests
// ============================================================================

TEST_F(VulkanSemaphoreTest, GetValueInitial_ReturnsInitialValue)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 42;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    EXPECT_EQ(semaphore.getValue(), 42u);
}

TEST_F(VulkanSemaphoreTest, GetValueZero_ReturnsZero)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 0;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    EXPECT_EQ(semaphore.getValue(), 0u);
}

TEST_F(VulkanSemaphoreTest, GetValueLarge_ReturnsLargeValue)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 1000000;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    EXPECT_EQ(semaphore.getValue(), 1000000u);
}

// ============================================================================
// Timeline Semaphore Signal Tests
// ============================================================================

TEST_F(VulkanSemaphoreTest, SignalTimeline_IncreasesValue)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 0;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    EXPECT_EQ(semaphore.getValue(), 0u);

    VkResult result = semaphore.signal(1);
    EXPECT_EQ(result, VK_SUCCESS);

    EXPECT_EQ(semaphore.getValue(), 1u);
}

TEST_F(VulkanSemaphoreTest, SignalTimelineMultipleTimes_IncreasesValue)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 0;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    semaphore.signal(1);
    EXPECT_EQ(semaphore.getValue(), 1u);

    semaphore.signal(5);
    EXPECT_EQ(semaphore.getValue(), 5u);

    semaphore.signal(10);
    EXPECT_EQ(semaphore.getValue(), 10u);
}

TEST_F(VulkanSemaphoreTest, SignalTimelineLargeValue_WorksCorrectly)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 0;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    VkResult result = semaphore.signal(1000000);
    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_EQ(semaphore.getValue(), 1000000u);
}

TEST_F(VulkanSemaphoreTest, SignalTimelineFromNonZero_IncreasesCorrectly)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 100;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    EXPECT_EQ(semaphore.getValue(), 100u);

    semaphore.signal(200);
    EXPECT_EQ(semaphore.getValue(), 200u);
}

// ============================================================================
// Timeline Semaphore Wait Tests
// ============================================================================

TEST_F(VulkanSemaphoreTest, WaitTimelineAlreadyReached_ReturnsImmediately)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 10;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    auto start = std::chrono::high_resolution_clock::now();
    VkResult result = semaphore.wait(5, 1000000000); // Wait for value 5, already at 10
    auto end = std::chrono::high_resolution_clock::now();

    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_LT(durationMs, 100); // Should return almost immediately
}

TEST_F(VulkanSemaphoreTest, WaitTimelineCurrentValue_ReturnsImmediately)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 42;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    auto start = std::chrono::high_resolution_clock::now();
    VkResult result = semaphore.wait(42, 1000000000); // Wait for current value
    auto end = std::chrono::high_resolution_clock::now();

    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_LT(durationMs, 100);
}

TEST_F(VulkanSemaphoreTest, WaitTimelineFutureValueZeroTimeout_ReturnsTimeout)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 0;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    VkResult result = semaphore.wait(100, 0); // Wait for future value with zero timeout

    EXPECT_EQ(result, VK_TIMEOUT);
}

TEST_F(VulkanSemaphoreTest, WaitTimelineFutureValueShortTimeout_ReturnsTimeout)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 0;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    auto start = std::chrono::high_resolution_clock::now();
    VkResult result = semaphore.wait(100, 10000000); // 10ms timeout
    auto end = std::chrono::high_resolution_clock::now();

    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_EQ(result, VK_TIMEOUT);
    EXPECT_GE(durationMs, 9);
    EXPECT_LE(durationMs, 100);
}

TEST_F(VulkanSemaphoreTest, SignalThenWait_ReturnsImmediately)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 0;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    semaphore.signal(10);

    auto start = std::chrono::high_resolution_clock::now();
    VkResult result = semaphore.wait(10, 1000000000);
    auto end = std::chrono::high_resolution_clock::now();

    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_LT(durationMs, 100);
}

// ============================================================================
// Timeline Semaphore Signal-Wait Pattern Tests
// ============================================================================

TEST_F(VulkanSemaphoreTest, SignalWaitIncrementing_WorksCorrectly)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 0;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    // Simulate incrementing timeline pattern
    for (uint64_t i = 1; i <= 10; ++i) {
        semaphore.signal(i);
        EXPECT_EQ(semaphore.getValue(), i);

        VkResult result = semaphore.wait(i, 1000000000);
        EXPECT_EQ(result, VK_SUCCESS);
    }
}

TEST_F(VulkanSemaphoreTest, SignalLargeGaps_WorksCorrectly)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 0;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    semaphore.signal(1);
    EXPECT_EQ(semaphore.wait(1, 0), VK_SUCCESS);

    semaphore.signal(100);
    EXPECT_EQ(semaphore.wait(100, 0), VK_SUCCESS);

    semaphore.signal(10000);
    EXPECT_EQ(semaphore.wait(10000, 0), VK_SUCCESS);
}

TEST_F(VulkanSemaphoreTest, MultipleWaitsSameValue_AllSucceed)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 100;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    EXPECT_EQ(semaphore.wait(50, 0), VK_SUCCESS);
    EXPECT_EQ(semaphore.wait(50, 0), VK_SUCCESS);
    EXPECT_EQ(semaphore.wait(50, 0), VK_SUCCESS);
}

// ============================================================================
// Handle Tests
// ============================================================================

TEST_F(VulkanSemaphoreTest, GetHandle_ReturnsValidHandle)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Binary;
    createInfo.initialValue = 0;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    VkSemaphore handle = semaphore.handle();
    EXPECT_NE(handle, VK_NULL_HANDLE);

    // Multiple calls should return same handle
    EXPECT_EQ(semaphore.handle(), handle);
}

TEST_F(VulkanSemaphoreTest, MultipleSemaphores_HaveUniqueHandles)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Binary;
    createInfo.initialValue = 0;

    gfx::backend::vulkan::core::Semaphore sem1(device.get(), createInfo);
    gfx::backend::vulkan::core::Semaphore sem2(device.get(), createInfo);

    EXPECT_NE(sem1.handle(), sem2.handle());
}

// ============================================================================
// Type Tests
// ============================================================================

TEST_F(VulkanSemaphoreTest, GetTypeBinary_ReturnsBinary)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Binary;
    createInfo.initialValue = 0;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    EXPECT_EQ(semaphore.getType(), gfx::backend::vulkan::core::SemaphoreType::Binary);
}

TEST_F(VulkanSemaphoreTest, GetTypeTimeline_ReturnsTimeline)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 0;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    EXPECT_EQ(semaphore.getType(), gfx::backend::vulkan::core::SemaphoreType::Timeline);
}

// ============================================================================
// Use Case Tests
// ============================================================================

TEST_F(VulkanSemaphoreTest, FramePacingPattern_WorksCorrectly)
{
    // Typical frame pacing pattern with timeline semaphore
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 0;

    gfx::backend::vulkan::core::Semaphore semaphore(device.get(), createInfo);

    // Simulate 5 frames
    for (uint64_t frame = 1; frame <= 5; ++frame) {
        // Signal completion of frame
        semaphore.signal(frame);
        EXPECT_EQ(semaphore.getValue(), frame);

        // Wait for frame to complete
        EXPECT_EQ(semaphore.wait(frame, 1000000000), VK_SUCCESS);
    }
}

TEST_F(VulkanSemaphoreTest, DependencyChainPattern_WorksCorrectly)
{
    // Multiple timeline semaphores for dependency chains
    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 0;

    gfx::backend::vulkan::core::Semaphore stage1(device.get(), createInfo);
    gfx::backend::vulkan::core::Semaphore stage2(device.get(), createInfo);
    gfx::backend::vulkan::core::Semaphore stage3(device.get(), createInfo);

    // Signal stages in order
    stage1.signal(1);
    EXPECT_EQ(stage1.wait(1, 0), VK_SUCCESS);

    stage2.signal(1);
    EXPECT_EQ(stage2.wait(1, 0), VK_SUCCESS);

    stage3.signal(1);
    EXPECT_EQ(stage3.wait(1, 0), VK_SUCCESS);
}

TEST_F(VulkanSemaphoreTest, CreateManySemaphores_AllWorkCorrectly)
{
    std::vector<std::unique_ptr<gfx::backend::vulkan::core::Semaphore>> semaphores;

    gfx::backend::vulkan::core::SemaphoreCreateInfo createInfo{};
    createInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    createInfo.initialValue = 0;

    // Create many semaphores
    for (int i = 0; i < 50; ++i) {
        semaphores.push_back(std::make_unique<gfx::backend::vulkan::core::Semaphore>(device.get(), createInfo));
    }

    // Verify all are valid and unique
    for (size_t i = 0; i < semaphores.size(); ++i) {
        EXPECT_NE(semaphores[i]->handle(), VK_NULL_HANDLE);
        EXPECT_EQ(semaphores[i]->getType(), gfx::backend::vulkan::core::SemaphoreType::Timeline);
        EXPECT_EQ(semaphores[i]->getValue(), 0u);

        // Check uniqueness with a few others
        if (i > 0) {
            EXPECT_NE(semaphores[i]->handle(), semaphores[i - 1]->handle());
        }
    }
}

TEST_F(VulkanSemaphoreTest, MixedBinaryAndTimeline_BothWorkCorrectly)
{
    gfx::backend::vulkan::core::SemaphoreCreateInfo binaryInfo{};
    binaryInfo.type = gfx::backend::vulkan::core::SemaphoreType::Binary;
    binaryInfo.initialValue = 0;

    gfx::backend::vulkan::core::SemaphoreCreateInfo timelineInfo{};
    timelineInfo.type = gfx::backend::vulkan::core::SemaphoreType::Timeline;
    timelineInfo.initialValue = 0;

    gfx::backend::vulkan::core::Semaphore binary1(device.get(), binaryInfo);
    gfx::backend::vulkan::core::Semaphore timeline1(device.get(), timelineInfo);
    gfx::backend::vulkan::core::Semaphore binary2(device.get(), binaryInfo);
    gfx::backend::vulkan::core::Semaphore timeline2(device.get(), timelineInfo);

    EXPECT_EQ(binary1.getType(), gfx::backend::vulkan::core::SemaphoreType::Binary);
    EXPECT_EQ(timeline1.getType(), gfx::backend::vulkan::core::SemaphoreType::Timeline);
    EXPECT_EQ(binary2.getType(), gfx::backend::vulkan::core::SemaphoreType::Binary);
    EXPECT_EQ(timeline2.getType(), gfx::backend::vulkan::core::SemaphoreType::Timeline);

    // Timeline semaphores can be signaled
    timeline1.signal(10);
    timeline2.signal(20);
    EXPECT_EQ(timeline1.getValue(), 10u);
    EXPECT_EQ(timeline2.getValue(), 20u);
}

} // namespace
