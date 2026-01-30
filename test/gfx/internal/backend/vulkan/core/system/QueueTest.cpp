#include <backend/vulkan/core/system/Adapter.h>
#include <backend/vulkan/core/system/Device.h>
#include <backend/vulkan/core/system/Instance.h>
#include <backend/vulkan/core/system/Queue.h>


#include <gtest/gtest.h>

// Test Vulkan core Queue class
// These tests verify the internal queue implementation, not the public API

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class VulkanQueueTest : public testing::Test {
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

            queue = device->getQueue();
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up Vulkan: " << e.what();
        }
    }

    std::unique_ptr<gfx::backend::vulkan::core::Instance> instance;
    gfx::backend::vulkan::core::Adapter* adapter = nullptr;
    std::unique_ptr<gfx::backend::vulkan::core::Device> device;
    gfx::backend::vulkan::core::Queue* queue = nullptr;
};

// ============================================================================
// Basic Handle Tests
// ============================================================================

TEST_F(VulkanQueueTest, GetHandle_ReturnsValidHandle)
{
    EXPECT_NE(queue->handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanQueueTest, GetHandleMultipleTimes_ReturnsSame)
{
    VkQueue handle1 = queue->handle();
    VkQueue handle2 = queue->handle();

    EXPECT_EQ(handle1, handle2);
}

// ============================================================================
// Device Access Tests
// ============================================================================

TEST_F(VulkanQueueTest, GetDevice_ReturnsDeviceHandle)
{
    VkDevice dev = queue->device();

    EXPECT_NE(dev, VK_NULL_HANDLE);
    EXPECT_EQ(dev, device->handle());
}

TEST_F(VulkanQueueTest, GetPhysicalDevice_ReturnsPhysicalDeviceHandle)
{
    VkPhysicalDevice physDev = queue->physicalDevice();

    EXPECT_NE(physDev, VK_NULL_HANDLE);
    EXPECT_EQ(physDev, adapter->handle());
}

// ============================================================================
// Queue Family Tests
// ============================================================================

TEST_F(VulkanQueueTest, GetFamily_ReturnsValidFamily)
{
    uint32_t family = queue->family();

    EXPECT_NE(family, UINT32_MAX);
}

TEST_F(VulkanQueueTest, GetFamily_MatchesGraphicsFamily)
{
    uint32_t queueFamily = queue->family();
    uint32_t graphicsFamily = adapter->getGraphicsQueueFamily();

    EXPECT_EQ(queueFamily, graphicsFamily);
}

// ============================================================================
// Wait Idle Tests
// ============================================================================

TEST_F(VulkanQueueTest, WaitIdle_CompletesSuccessfully)
{
    // Should not throw or hang
    queue->waitIdle();
}

TEST_F(VulkanQueueTest, WaitIdleMultipleTimes_WorksCorrectly)
{
    queue->waitIdle();
    queue->waitIdle();
    queue->waitIdle();
}

// ============================================================================
// Multiple Queue Tests
// ============================================================================

TEST_F(VulkanQueueTest, GetSameQueueTwice_ReturnsSameQueue)
{
    auto* queue1 = device->getQueue();
    auto* queue2 = device->getQueue();

    EXPECT_EQ(queue1, queue2);
    EXPECT_EQ(queue1->handle(), queue2->handle());
}

TEST_F(VulkanQueueTest, GetQueueByIndex_ReturnsSameAsDefault)
{
    uint32_t graphicsFamily = adapter->getGraphicsQueueFamily();
    auto* queueByIndex = device->getQueueByIndex(graphicsFamily, 0);
    auto* defaultQueue = device->getQueue();

    // They should be the same queue
    EXPECT_EQ(queueByIndex, defaultQueue);
}

// ============================================================================
// Queue Properties Tests
// ============================================================================

TEST_F(VulkanQueueTest, QueueFamilySupportsGraphics_IsTrue)
{
    uint32_t queueFamily = queue->family();
    auto queueFamilies = adapter->getQueueFamilyProperties();

    EXPECT_LT(queueFamily, queueFamilies.size());
    EXPECT_TRUE(queueFamilies[queueFamily].queueFlags & VK_QUEUE_GRAPHICS_BIT);
}

// ============================================================================
// Relationship Tests
// ============================================================================

TEST_F(VulkanQueueTest, QueueDeviceMatchesParentDevice_IsTrue)
{
    VkDevice queueDevice = queue->device();
    VkDevice parentDevice = device->handle();

    EXPECT_EQ(queueDevice, parentDevice);
}

TEST_F(VulkanQueueTest, QueuePhysicalDeviceMatchesAdapter_IsTrue)
{
    VkPhysicalDevice queuePhysDev = queue->physicalDevice();
    VkPhysicalDevice adapterPhysDev = adapter->handle();

    EXPECT_EQ(queuePhysDev, adapterPhysDev);
}

// ============================================================================
// Lifecycle Tests
// ============================================================================

TEST_F(VulkanQueueTest, QueueValidAfterDeviceWaitIdle_WorksCorrectly)
{
    device->waitIdle();

    EXPECT_NE(queue->handle(), VK_NULL_HANDLE);

    queue->waitIdle();
}

TEST_F(VulkanQueueTest, QueueOperationsSequence_WorksCorrectly)
{
    // Get queue info
    uint32_t family = queue->family();
    EXPECT_NE(family, UINT32_MAX);

    // Wait on queue
    queue->waitIdle();

    // Wait on device
    device->waitIdle();

    // Queue should still be valid
    EXPECT_NE(queue->handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Use Case Tests
// ============================================================================

TEST_F(VulkanQueueTest, TypicalQueueUsage_WorksCorrectly)
{
    // Get queue
    auto* q = device->getQueue();
    EXPECT_NE(q, nullptr);

    // Check properties
    EXPECT_NE(q->handle(), VK_NULL_HANDLE);
    EXPECT_NE(q->family(), UINT32_MAX);

    // Synchronize
    q->waitIdle();
}

TEST_F(VulkanQueueTest, MultipleQueueOperations_WorkCorrectly)
{
    auto* q = device->getQueue();

    for (int i = 0; i < 5; ++i) {
        EXPECT_NE(q->handle(), VK_NULL_HANDLE);
        q->waitIdle();
    }
}

TEST_F(VulkanQueueTest, QueueFromDifferentDevices_HaveDifferentHandles)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
    auto device2 = std::make_unique<gfx::backend::vulkan::core::Device>(adapter, createInfo);

    auto* queue1 = device->getQueue();
    auto* queue2 = device2->getQueue();

    // Different devices, different queues
    EXPECT_NE(queue1->handle(), queue2->handle());
}

TEST_F(VulkanQueueTest, AccessQueueAfterMultipleWaits_RemainsValid)
{
    queue->waitIdle();
    device->waitIdle();
    queue->waitIdle();
    device->waitIdle();

    EXPECT_NE(queue->handle(), VK_NULL_HANDLE);
    EXPECT_NE(queue->family(), UINT32_MAX);
}

} // namespace
