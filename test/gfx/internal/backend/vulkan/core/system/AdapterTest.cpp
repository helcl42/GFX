#include <backend/vulkan/core/system/Adapter.h>
#include <backend/vulkan/core/system/Instance.h>

#include <gtest/gtest.h>

// Test Vulkan core Adapter class
// These tests verify the internal adapter implementation, not the public API

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class VulkanAdapterTest : public testing::Test {
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
// Basic Handle Tests
// ============================================================================

TEST_F(VulkanAdapterTest, GetHandle_ReturnsValidHandle)
{
    EXPECT_NE(adapter->handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanAdapterTest, GetHandleMultipleTimes_ReturnsSame)
{
    VkPhysicalDevice handle1 = adapter->handle();
    VkPhysicalDevice handle2 = adapter->handle();

    EXPECT_EQ(handle1, handle2);
}

// ============================================================================
// Properties Tests
// ============================================================================

TEST_F(VulkanAdapterTest, GetProperties_ReturnsValidProperties)
{
    const auto& props = adapter->getProperties();

    EXPECT_GT(props.apiVersion, 0u);
    EXPECT_GT(props.driverVersion, 0u);
    EXPECT_GT(props.vendorID, 0u);
    EXPECT_GT(props.deviceID, 0u);
    EXPECT_NE(props.deviceName[0], '\0');
}

TEST_F(VulkanAdapterTest, GetPropertiesDeviceName_IsNotEmpty)
{
    const auto& props = adapter->getProperties();
    std::string deviceName(props.deviceName);

    EXPECT_FALSE(deviceName.empty());
}

TEST_F(VulkanAdapterTest, GetPropertiesLimits_AreReasonable)
{
    const auto& props = adapter->getProperties();

    EXPECT_GT(props.limits.maxImageDimension2D, 1024u);
    EXPECT_GT(props.limits.maxUniformBufferRange, 16384u);
    EXPECT_GT(props.limits.maxBoundDescriptorSets, 0u);
}

// ============================================================================
// Memory Properties Tests
// ============================================================================

TEST_F(VulkanAdapterTest, GetMemoryProperties_HasMemoryTypes)
{
    const auto& memProps = adapter->getMemoryProperties();

    EXPECT_GT(memProps.memoryTypeCount, 0u);
    EXPECT_LE(memProps.memoryTypeCount, VK_MAX_MEMORY_TYPES);
}

TEST_F(VulkanAdapterTest, GetMemoryProperties_HasMemoryHeaps)
{
    const auto& memProps = adapter->getMemoryProperties();

    EXPECT_GT(memProps.memoryHeapCount, 0u);
    EXPECT_LE(memProps.memoryHeapCount, VK_MAX_MEMORY_HEAPS);
}

TEST_F(VulkanAdapterTest, GetMemoryProperties_HeapsHaveSize)
{
    const auto& memProps = adapter->getMemoryProperties();

    for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i) {
        EXPECT_GT(memProps.memoryHeaps[i].size, 0u);
    }
}

// ============================================================================
// Features Tests
// ============================================================================

TEST_F(VulkanAdapterTest, GetFeatures_ReturnsFeatures)
{
    const auto& features = adapter->getFeatures();

    // At least some features should be available (just checking structure is valid)
    // Can't assert specific features as they vary by hardware
    (void)features;
}

// ============================================================================
// Queue Family Tests
// ============================================================================

TEST_F(VulkanAdapterTest, GetGraphicsQueueFamily_ReturnsValidIndex)
{
    uint32_t queueFamily = adapter->getGraphicsQueueFamily();

    EXPECT_NE(queueFamily, UINT32_MAX);
}

TEST_F(VulkanAdapterTest, GetQueueFamilyProperties_ReturnsProperties)
{
    auto queueFamilies = adapter->getQueueFamilyProperties();

    EXPECT_GT(queueFamilies.size(), 0u);
}

TEST_F(VulkanAdapterTest, GetQueueFamilyProperties_GraphicsFamilyExists)
{
    auto queueFamilies = adapter->getQueueFamilyProperties();
    uint32_t graphicsFamily = adapter->getGraphicsQueueFamily();

    EXPECT_LT(graphicsFamily, queueFamilies.size());
    EXPECT_TRUE(queueFamilies[graphicsFamily].queueFlags & VK_QUEUE_GRAPHICS_BIT);
}

TEST_F(VulkanAdapterTest, GetQueueFamilyProperties_FamiliesHaveQueues)
{
    auto queueFamilies = adapter->getQueueFamilyProperties();

    for (const auto& family : queueFamilies) {
        EXPECT_GT(family.queueCount, 0u);
    }
}

// ============================================================================
// Extension Tests
// ============================================================================

TEST_F(VulkanAdapterTest, EnumerateExtensionProperties_ReturnsExtensions)
{
    auto extensions = adapter->enumerateExtensionProperties();

    // Should have at least some extensions
    EXPECT_GT(extensions.size(), 0u);
}

TEST_F(VulkanAdapterTest, EnumerateSupportedExtensions_ReturnsExtensions)
{
    auto extensions = adapter->enumerateSupportedExtensions();

    // Should have at least some supported extensions
    EXPECT_GT(extensions.size(), 0u);
}

// ============================================================================
// Instance Relationship Tests
// ============================================================================

TEST_F(VulkanAdapterTest, GetInstance_ReturnsParentInstance)
{
    auto* inst = adapter->getInstance();

    EXPECT_NE(inst, nullptr);
    EXPECT_EQ(inst, instance.get());
}

TEST_F(VulkanAdapterTest, GetInstance_HandleMatches)
{
    auto* inst = adapter->getInstance();

    EXPECT_EQ(inst->handle(), instance->handle());
}

// ============================================================================
// Multiple Adapters Tests
// ============================================================================

TEST_F(VulkanAdapterTest, MultipleAdapters_HaveUniqueHandles)
{
    auto devices = instance->enumeratePhysicalDevices();

    if (devices.size() > 1) {
        gfx::backend::vulkan::core::AdapterCreateInfo adapterInfo1{};
        adapterInfo1.adapterIndex = 0;
        auto* adapter1 = instance->requestAdapter(adapterInfo1);

        gfx::backend::vulkan::core::AdapterCreateInfo adapterInfo2{};
        adapterInfo2.adapterIndex = 1;
        auto* adapter2 = instance->requestAdapter(adapterInfo2);

        EXPECT_NE(adapter1->handle(), adapter2->handle());
    }
}

// ============================================================================
// Device Type Tests
// ============================================================================

TEST_F(VulkanAdapterTest, GetProperties_HasDeviceType)
{
    const auto& props = adapter->getProperties();

    EXPECT_GE(props.deviceType, VK_PHYSICAL_DEVICE_TYPE_OTHER);
    EXPECT_LE(props.deviceType, VK_PHYSICAL_DEVICE_TYPE_CPU);
}

// ============================================================================
// Use Case Tests
// ============================================================================

TEST_F(VulkanAdapterTest, InspectAllAvailableAdapters_AllValid)
{
    auto devices = instance->enumeratePhysicalDevices();

    for (size_t i = 0; i < devices.size(); ++i) {
        gfx::backend::vulkan::core::AdapterCreateInfo adapterInfo{};
        adapterInfo.adapterIndex = static_cast<uint32_t>(i);
        auto* adapterPtr = instance->requestAdapter(adapterInfo);

        EXPECT_NE(adapterPtr, nullptr);
        EXPECT_NE(adapterPtr->handle(), VK_NULL_HANDLE);

        const auto& props = adapterPtr->getProperties();
        EXPECT_NE(props.deviceName[0], '\0');

        uint32_t graphicsQueue = adapterPtr->getGraphicsQueueFamily();
        EXPECT_NE(graphicsQueue, UINT32_MAX);
    }
}

TEST_F(VulkanAdapterTest, CheckCommonExtensionSupport_ReturnsResult)
{
    auto extensions = adapter->enumerateSupportedExtensions();

    // Check if swapchain extension is supported (common extension)
    bool hasSwapchain = false;
    for (const auto* ext : extensions) {
        if (std::string(ext) == "gfx_swapchain") {
            hasSwapchain = true;
            break;
        }
    }

    // Most adapters should support swapchain
    // (but we can't assert it as it depends on platform)
    (void)hasSwapchain;
}

} // namespace
