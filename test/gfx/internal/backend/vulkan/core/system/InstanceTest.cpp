#include <backend/vulkan/core/system/Adapter.h>
#include <backend/vulkan/core/system/Instance.h>


#include <gtest/gtest.h>

// Test Vulkan core Instance class
// These tests verify the internal instance implementation, not the public API

namespace {

// ============================================================================
// Basic Creation Tests
// ============================================================================

TEST(VulkanInstanceTest, CreateDefaultInstance_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::InstanceCreateInfo createInfo{};
    createInfo.enabledExtensions = {};

    gfx::backend::vulkan::core::Instance instance(createInfo);

    EXPECT_NE(instance.handle(), VK_NULL_HANDLE);
}

TEST(VulkanInstanceTest, CreateInstanceWithApplicationName_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::InstanceCreateInfo createInfo{};
    createInfo.applicationName = "Test Application";
    createInfo.applicationVersion = 1;
    createInfo.enabledExtensions = {};

    gfx::backend::vulkan::core::Instance instance(createInfo);

    EXPECT_NE(instance.handle(), VK_NULL_HANDLE);
}

TEST(VulkanInstanceTest, CreateInstanceWithVersion_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::InstanceCreateInfo createInfo{};
    createInfo.applicationName = "Versioned App";
    createInfo.applicationVersion = 12345;
    createInfo.enabledExtensions = {};

    gfx::backend::vulkan::core::Instance instance(createInfo);

    EXPECT_NE(instance.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Extension Tests
// ============================================================================

TEST(VulkanInstanceTest, CreateInstanceNoExtensions_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::InstanceCreateInfo createInfo{};
    createInfo.enabledExtensions = {};

    gfx::backend::vulkan::core::Instance instance(createInfo);

    EXPECT_NE(instance.handle(), VK_NULL_HANDLE);
}

TEST(VulkanInstanceTest, CreateInstanceDebugExtension_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::InstanceCreateInfo createInfo{};
    createInfo.enabledExtensions = { "gfx_debug" };

    gfx::backend::vulkan::core::Instance instance(createInfo);

    EXPECT_NE(instance.handle(), VK_NULL_HANDLE);
}

TEST(VulkanInstanceTest, EnumerateSupportedExtensions_ReturnsExtensions)
{
    auto extensions = gfx::backend::vulkan::core::Instance::enumerateSupportedExtensions();

    // Should have at least some standard extensions
    EXPECT_GT(extensions.size(), 0u);
}

// ============================================================================
// Physical Device Enumeration Tests
// ============================================================================

TEST(VulkanInstanceTest, EnumeratePhysicalDevices_ReturnsDevices)
{
    gfx::backend::vulkan::core::InstanceCreateInfo createInfo{};
    createInfo.enabledExtensions = {};

    gfx::backend::vulkan::core::Instance instance(createInfo);

    auto devices = instance.enumeratePhysicalDevices();

    // Should find at least one physical device (assuming test environment has Vulkan)
    EXPECT_GT(devices.size(), 0u);
    for (auto device : devices) {
        EXPECT_NE(device, VK_NULL_HANDLE);
    }
}

TEST(VulkanInstanceTest, EnumeratePhysicalDevicesMultipleTimes_ReturnsConsistent)
{
    gfx::backend::vulkan::core::InstanceCreateInfo createInfo{};
    createInfo.enabledExtensions = {};

    gfx::backend::vulkan::core::Instance instance(createInfo);

    auto devices1 = instance.enumeratePhysicalDevices();
    auto devices2 = instance.enumeratePhysicalDevices();

    EXPECT_EQ(devices1.size(), devices2.size());
}

// ============================================================================
// Adapter Tests
// ============================================================================

TEST(VulkanInstanceTest, RequestAdapterByIndex_ReturnsAdapter)
{
    gfx::backend::vulkan::core::InstanceCreateInfo createInfo{};
    createInfo.enabledExtensions = {};

    gfx::backend::vulkan::core::Instance instance(createInfo);

    gfx::backend::vulkan::core::AdapterCreateInfo adapterInfo{};
    adapterInfo.adapterIndex = 0;

    auto* adapter = instance.requestAdapter(adapterInfo);

    EXPECT_NE(adapter, nullptr);
    EXPECT_NE(adapter->handle(), VK_NULL_HANDLE);
}

TEST(VulkanInstanceTest, RequestAdapterHighPerformance_ReturnsAdapter)
{
    gfx::backend::vulkan::core::InstanceCreateInfo createInfo{};
    createInfo.enabledExtensions = {};

    gfx::backend::vulkan::core::Instance instance(createInfo);

    gfx::backend::vulkan::core::AdapterCreateInfo adapterInfo{};
    adapterInfo.adapterIndex = UINT32_MAX;
    adapterInfo.devicePreference = gfx::backend::vulkan::core::DeviceTypePreference::HighPerformance;

    auto* adapter = instance.requestAdapter(adapterInfo);

    EXPECT_NE(adapter, nullptr);
    EXPECT_NE(adapter->handle(), VK_NULL_HANDLE);
}

TEST(VulkanInstanceTest, RequestAdapterLowPower_ReturnsAdapter)
{
    gfx::backend::vulkan::core::InstanceCreateInfo createInfo{};
    createInfo.enabledExtensions = {};

    gfx::backend::vulkan::core::Instance instance(createInfo);

    gfx::backend::vulkan::core::AdapterCreateInfo adapterInfo{};
    adapterInfo.adapterIndex = UINT32_MAX;
    adapterInfo.devicePreference = gfx::backend::vulkan::core::DeviceTypePreference::LowPower;

    auto* adapter = instance.requestAdapter(adapterInfo);

    EXPECT_NE(adapter, nullptr);
    EXPECT_NE(adapter->handle(), VK_NULL_HANDLE);
}

TEST(VulkanInstanceTest, RequestSameAdapterTwice_ReturnsSameAdapter)
{
    gfx::backend::vulkan::core::InstanceCreateInfo createInfo{};
    createInfo.enabledExtensions = {};

    gfx::backend::vulkan::core::Instance instance(createInfo);

    gfx::backend::vulkan::core::AdapterCreateInfo adapterInfo{};
    adapterInfo.adapterIndex = 0;

    auto* adapter1 = instance.requestAdapter(adapterInfo);
    auto* adapter2 = instance.requestAdapter(adapterInfo);

    EXPECT_EQ(adapter1, adapter2);
}

TEST(VulkanInstanceTest, GetAdapters_ReturnsAdapters)
{
    gfx::backend::vulkan::core::InstanceCreateInfo createInfo{};
    createInfo.enabledExtensions = {};

    gfx::backend::vulkan::core::Instance instance(createInfo);

    gfx::backend::vulkan::core::AdapterCreateInfo adapterInfo{};
    adapterInfo.adapterIndex = 0;
    instance.requestAdapter(adapterInfo);

    const auto& adapters = instance.getAdapters();

    EXPECT_GT(adapters.size(), 0u);
}

// ============================================================================
// Handle Tests
// ============================================================================

TEST(VulkanInstanceTest, GetHandle_ReturnsValidHandle)
{
    gfx::backend::vulkan::core::InstanceCreateInfo createInfo{};
    createInfo.enabledExtensions = {};

    gfx::backend::vulkan::core::Instance instance(createInfo);

    VkInstance handle = instance.handle();
    EXPECT_NE(handle, VK_NULL_HANDLE);

    // Multiple calls should return same handle
    EXPECT_EQ(instance.handle(), handle);
}

TEST(VulkanInstanceTest, MultipleInstances_HaveUniqueHandles)
{
    gfx::backend::vulkan::core::InstanceCreateInfo createInfo{};
    createInfo.enabledExtensions = {};

    gfx::backend::vulkan::core::Instance instance1(createInfo);
    gfx::backend::vulkan::core::Instance instance2(createInfo);

    EXPECT_NE(instance1.handle(), instance2.handle());
}

// ============================================================================
// Use Case Tests
// ============================================================================

TEST(VulkanInstanceTest, CreateMultipleAdapters_AllValid)
{
    gfx::backend::vulkan::core::InstanceCreateInfo createInfo{};
    createInfo.enabledExtensions = {};

    gfx::backend::vulkan::core::Instance instance(createInfo);

    auto devices = instance.enumeratePhysicalDevices();

    for (size_t i = 0; i < devices.size(); ++i) {
        gfx::backend::vulkan::core::AdapterCreateInfo adapterInfo{};
        adapterInfo.adapterIndex = static_cast<uint32_t>(i);

        auto* adapter = instance.requestAdapter(adapterInfo);
        EXPECT_NE(adapter, nullptr);
        EXPECT_NE(adapter->handle(), VK_NULL_HANDLE);
    }
}

TEST(VulkanInstanceTest, InstanceLifecycle_WorksCorrectly)
{
    // Create instance
    gfx::backend::vulkan::core::InstanceCreateInfo createInfo{};
    createInfo.enabledExtensions = {};

    auto instance = std::make_unique<gfx::backend::vulkan::core::Instance>(createInfo);

    EXPECT_NE(instance->handle(), VK_NULL_HANDLE);

    // Get adapter
    gfx::backend::vulkan::core::AdapterCreateInfo adapterInfo{};
    adapterInfo.adapterIndex = 0;
    auto* adapter = instance->requestAdapter(adapterInfo);
    EXPECT_NE(adapter, nullptr);

    // Destroy instance (adapter should be destroyed too)
    instance.reset();
}

} // namespace
