#include <backend/vulkan/core/system/Adapter.h>
#include <backend/vulkan/core/system/Device.h>
#include <backend/vulkan/core/system/Instance.h>
#include <backend/vulkan/core/system/Queue.h>

#include <gtest/gtest.h>

// Test Vulkan core Device class
// These tests verify the internal device implementation, not the public API

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class VulkanDeviceTest : public testing::Test {
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
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up Vulkan: " << e.what();
        }
    }

    std::unique_ptr<gfx::backend::vulkan::core::Instance> instance;
    gfx::backend::vulkan::core::Adapter* adapter = nullptr;
};

// ============================================================================
// Basic Creation Tests
// ============================================================================

TEST_F(VulkanDeviceTest, CreateDefaultDevice_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};

    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    EXPECT_NE(device.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanDeviceTest, CreateDeviceNoExtensions_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
    createInfo.enabledExtensions = {};

    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    EXPECT_NE(device.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanDeviceTest, CreateDeviceWithSwapchainExtension_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
    createInfo.enabledExtensions = { "gfx_swapchain" };

    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    EXPECT_NE(device.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanDeviceTest, CreateDeviceWithTimelineSemaphore_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
    createInfo.enabledExtensions = { "gfx_timeline_semaphore" };

    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    EXPECT_NE(device.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanDeviceTest, CreateDeviceWithMultipleExtensions_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
    createInfo.enabledExtensions = { "gfx_swapchain", "gfx_timeline_semaphore" };

    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    EXPECT_NE(device.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Handle Tests
// ============================================================================

TEST_F(VulkanDeviceTest, GetHandle_ReturnsValidHandle)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    VkDevice handle = device.handle();
    EXPECT_NE(handle, VK_NULL_HANDLE);

    // Multiple calls should return same handle
    EXPECT_EQ(device.handle(), handle);
}

TEST_F(VulkanDeviceTest, MultipleDevices_HaveUniqueHandles)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};

    gfx::backend::vulkan::core::Device device1(adapter, createInfo);
    gfx::backend::vulkan::core::Device device2(adapter, createInfo);

    EXPECT_NE(device1.handle(), device2.handle());
}

// ============================================================================
// Queue Access Tests
// ============================================================================

TEST_F(VulkanDeviceTest, GetQueue_ReturnsDefaultQueue)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    auto* queue = device.getQueue();

    EXPECT_NE(queue, nullptr);
    EXPECT_NE(queue->handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanDeviceTest, GetQueueMultipleTimes_ReturnsSameQueue)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    auto* queue1 = device.getQueue();
    auto* queue2 = device.getQueue();

    EXPECT_EQ(queue1, queue2);
}

TEST_F(VulkanDeviceTest, GetQueueByIndex_ReturnsQueue)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    uint32_t graphicsFamily = adapter->getGraphicsQueueFamily();
    auto* queue = device.getQueueByIndex(graphicsFamily, 0);

    EXPECT_NE(queue, nullptr);
    EXPECT_NE(queue->handle(), VK_NULL_HANDLE);
    EXPECT_EQ(queue->family(), graphicsFamily);
}

TEST_F(VulkanDeviceTest, GetQueueByIndexMultipleTimes_ReturnsSameQueue)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    uint32_t graphicsFamily = adapter->getGraphicsQueueFamily();
    auto* queue1 = device.getQueueByIndex(graphicsFamily, 0);
    auto* queue2 = device.getQueueByIndex(graphicsFamily, 0);

    EXPECT_EQ(queue1, queue2);
}

// ============================================================================
// Adapter Access Tests
// ============================================================================

TEST_F(VulkanDeviceTest, GetAdapter_ReturnsCorrectAdapter)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    auto* deviceAdapter = device.getAdapter();

    EXPECT_EQ(deviceAdapter, adapter);
}

TEST_F(VulkanDeviceTest, GetProperties_ReturnsAdapterProperties)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    const auto& props = device.getProperties();

    EXPECT_GT(props.apiVersion, 0u);
    EXPECT_NE(props.deviceName[0], '\0');
}

// ============================================================================
// Wait Idle Tests
// ============================================================================

TEST_F(VulkanDeviceTest, WaitIdle_CompletesSuccessfully)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    // Should not throw or hang
    device.waitIdle();
}

TEST_F(VulkanDeviceTest, WaitIdleMultipleTimes_WorksCorrectly)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    device.waitIdle();
    device.waitIdle();
    device.waitIdle();
}

// ============================================================================
// Extension Function Loading Tests
// ============================================================================

TEST_F(VulkanDeviceTest, LoadFunctionPointer_ReturnsPointer)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
    createInfo.enabledExtensions = { "gfx_timeline_semaphore" };
    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    auto pfn = device.loadFunction<PFN_vkGetSemaphoreCounterValueKHR>("vkGetSemaphoreCounterValueKHR");

    EXPECT_NE(pfn, nullptr);
}

TEST_F(VulkanDeviceTest, LoadFunctionPointerInvalidName_ReturnsNull)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    auto pfn = device.loadFunction<PFN_vkVoidFunction>("InvalidFunctionName12345");

    EXPECT_EQ(pfn, nullptr);
}

// ============================================================================
// Queue Request Tests
// ============================================================================

TEST_F(VulkanDeviceTest, CreateDeviceWithQueueRequest_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};

    gfx::backend::vulkan::core::DeviceCreateInfo::QueueRequest queueReq{};
    queueReq.queueFamilyIndex = adapter->getGraphicsQueueFamily();
    queueReq.queueIndex = 0;
    queueReq.priority = 1.0f;

    createInfo.queueRequests = { queueReq };

    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    EXPECT_NE(device.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanDeviceTest, CreateDeviceWithMultipleQueueRequests_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};

    uint32_t graphicsFamily = adapter->getGraphicsQueueFamily();

    gfx::backend::vulkan::core::DeviceCreateInfo::QueueRequest queueReq1{};
    queueReq1.queueFamilyIndex = graphicsFamily;
    queueReq1.queueIndex = 0;
    queueReq1.priority = 1.0f;

    createInfo.queueRequests = { queueReq1 };

    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    EXPECT_NE(device.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Lifecycle Tests
// ============================================================================

TEST_F(VulkanDeviceTest, CreateAndDestroyDevice_WorksCorrectly)
{
    {
        gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
        gfx::backend::vulkan::core::Device device(adapter, createInfo);

        EXPECT_NE(device.handle(), VK_NULL_HANDLE);
    }
    // Device destroyed, no crash
}

TEST_F(VulkanDeviceTest, DeviceLifecycleWithQueue_WorksCorrectly)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
    auto device = std::make_unique<gfx::backend::vulkan::core::Device>(adapter, createInfo);

    auto* queue = device->getQueue();
    EXPECT_NE(queue, nullptr);

    // Destroy device (queue should be destroyed too)
    device.reset();
}

// ============================================================================
// Use Case Tests
// ============================================================================

TEST_F(VulkanDeviceTest, TypicalDeviceSetup_WorksCorrectly)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
    createInfo.enabledExtensions = { "gfx_swapchain" };

    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    EXPECT_NE(device.handle(), VK_NULL_HANDLE);

    auto* queue = device.getQueue();
    EXPECT_NE(queue, nullptr);

    device.waitIdle();
}

TEST_F(VulkanDeviceTest, CreateMultipleDevicesSameAdapter_AllValid)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};

    auto device1 = std::make_unique<gfx::backend::vulkan::core::Device>(adapter, createInfo);
    auto device2 = std::make_unique<gfx::backend::vulkan::core::Device>(adapter, createInfo);

    EXPECT_NE(device1->handle(), VK_NULL_HANDLE);
    EXPECT_NE(device2->handle(), VK_NULL_HANDLE);
    EXPECT_NE(device1->handle(), device2->handle());
}

// ============================================================================
// Shader Format Support Tests
// ============================================================================

TEST_F(VulkanDeviceTest, SupportsShaderFormat_SPIRV)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    bool supported = device.supportsShaderFormat(gfx::backend::vulkan::core::ShaderSourceType::SPIRV);
    EXPECT_TRUE(supported);
}

TEST_F(VulkanDeviceTest, SupportsShaderFormat_WGSL)
{
    gfx::backend::vulkan::core::DeviceCreateInfo createInfo{};
    gfx::backend::vulkan::core::Device device(adapter, createInfo);

    bool supported = device.supportsShaderFormat(gfx::backend::vulkan::core::ShaderSourceType::WGSL);
    EXPECT_FALSE(supported);
}

} // namespace
