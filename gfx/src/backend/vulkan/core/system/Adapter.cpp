#include "Adapter.h"

#include "Instance.h"

#include <stdexcept>
#include <vector>

namespace gfx::backend::vulkan::core {

Adapter::Adapter(Instance* instance, const AdapterCreateInfo& createInfo)
    : m_instance(instance)
{
    // Enumerate physical devices
    std::vector<VkPhysicalDevice> devices = instance->enumeratePhysicalDevices();
    
    if (devices.empty()) {
        throw std::runtime_error("No Vulkan physical devices found");
    }

    uint32_t deviceCount = static_cast<uint32_t>(devices.size());

    // If adapter index is specified, use that directly
    if (createInfo.adapterIndex != UINT32_MAX) {
        if (createInfo.adapterIndex >= deviceCount) {
            throw std::runtime_error("Adapter index out of range");
        }
        m_physicalDevice = devices[createInfo.adapterIndex];
    } else {
        // Otherwise, use preference-based selection
        // Determine preferred device type based on createInfo
        VkPhysicalDeviceType preferredType;
        switch (createInfo.devicePreference) {
        case DeviceTypePreference::SoftwareRenderer:
            preferredType = VK_PHYSICAL_DEVICE_TYPE_CPU;
            break;
        case DeviceTypePreference::LowPower:
            preferredType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
            break;
        case DeviceTypePreference::HighPerformance:
        default:
            preferredType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
            break;
        }

        // First pass: try to find preferred device type
        m_physicalDevice = VK_NULL_HANDLE;
        for (auto device : devices) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);
            if (props.deviceType == preferredType) {
                m_physicalDevice = device;
                break;
            }
        }

        // Fallback: if preferred type not found, use first available device
        if (m_physicalDevice == VK_NULL_HANDLE) {
            m_physicalDevice = devices[0];
        }
    }

    initializeAdapterInfo();
}

// Constructor for wrapping a specific physical device (used by enumerate)
Adapter::Adapter(Instance* instance, VkPhysicalDevice physicalDevice)
    : m_instance(instance)
    , m_physicalDevice(physicalDevice)
{
    initializeAdapterInfo();
}

// Static method to enumerate all available adapters
// NOTE: Each adapter returned must be freed by the caller using the backend's adapterDestroy method
// (e.g., gfxAdapterDestroy() in the public API)
uint32_t Adapter::enumerate(Instance* instance, Adapter** outAdapters, uint32_t maxAdapters)
{
    if (!instance) {
        return 0;
    }

    std::vector<VkPhysicalDevice> devices = instance->enumeratePhysicalDevices();
    uint32_t deviceCount = static_cast<uint32_t>(devices.size());

    if (deviceCount == 0) {
        return 0;
    }

    // If outAdapters is NULL, just return the count
    if (!outAdapters) {
        return deviceCount;
    }

    // Create an adapter for each physical device
    uint32_t count = std::min(deviceCount, maxAdapters);
    for (uint32_t i = 0; i < count; ++i) {
        outAdapters[i] = new Adapter(instance, devices[i]);
    }

    return count;
}

VkPhysicalDevice Adapter::handle() const
{
    return m_physicalDevice;
}

uint32_t Adapter::getGraphicsQueueFamily() const
{
    return m_graphicsQueueFamily;
}

Instance* Adapter::getInstance() const
{
    return m_instance;
}

const VkPhysicalDeviceProperties& Adapter::getProperties() const
{
    return m_properties;
}

const VkPhysicalDeviceMemoryProperties& Adapter::getMemoryProperties() const
{
    return m_memoryProperties;
}

std::vector<VkQueueFamilyProperties> Adapter::getQueueFamilyProperties() const
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &count, nullptr);

    std::vector<VkQueueFamilyProperties> properties(count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &count, properties.data());

    return properties;
}

bool Adapter::supportsPresentation(uint32_t queueFamilyIndex, VkSurfaceKHR surface) const
{
    VkBool32 supported = VK_FALSE;
    VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, queueFamilyIndex, surface, &supported);

    return (result == VK_SUCCESS && supported == VK_TRUE);
}

void Adapter::initializeAdapterInfo()
{
    vkGetPhysicalDeviceProperties(m_physicalDevice, &m_properties);
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_memoryProperties);

    // Find graphics queue family
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            m_graphicsQueueFamily = i;
            break;
        }
    }

    if (m_graphicsQueueFamily == UINT32_MAX) {
        throw std::runtime_error("Failed to find graphics queue family for adapter");
    }
}

} // namespace gfx::backend::vulkan::core