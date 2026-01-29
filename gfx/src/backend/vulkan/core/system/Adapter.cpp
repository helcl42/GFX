#include "Adapter.h"

#include "Instance.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace gfx::backend::vulkan::core {

Adapter::Adapter(VkPhysicalDevice physicalDevice, Instance* instance)
    : m_physicalDevice(physicalDevice)
    , m_instance(instance)
{
    initializeAdapterInfo();
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

std::vector<VkExtensionProperties> Adapter::enumerateDeviceExtensionProperties() const
{
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &count, nullptr);

    std::vector<VkExtensionProperties> extensions(count);
    vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &count, extensions.data());

    return extensions;
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
    auto queueFamilies = getQueueFamilyProperties();

    for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilies.size()); ++i) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            m_graphicsQueueFamily = i;
            break;
        }
    }

    if (m_graphicsQueueFamily == UINT32_MAX) {
        throw std::runtime_error("Failed to find graphics queue family for adapter");
    }
}

std::vector<const char*> Adapter::enumerateSupportedExtensions() const
{
    // Map our internal extension names to actual Vulkan extension names
    struct ExtensionMapping {
        const char* internalName;
        const char* vkName;
    };

    static const ExtensionMapping knownExtensions[] = {
        { extensions::SWAPCHAIN, VK_KHR_SWAPCHAIN_EXTENSION_NAME },
        { extensions::TIMELINE_SEMAPHORE, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME },
        { extensions::MULTIVIEW, VK_KHR_MULTIVIEW_EXTENSION_NAME }
    };

    // Query what this physical device actually supports
    auto availableExtensions = enumerateDeviceExtensionProperties();

    // Build the intersection: extensions we care about that this device supports
    std::vector<const char*> supportedExtensions;

    for (const auto& mapping : knownExtensions) {
        bool deviceSupportsIt = std::any_of(availableExtensions.begin(), availableExtensions.end(),
            [&mapping](const VkExtensionProperties& props) {
                return strcmp(props.extensionName, mapping.vkName) == 0;
            });

        if (deviceSupportsIt) {
            supportedExtensions.push_back(mapping.internalName);
        }
    }

    return supportedExtensions;
}

} // namespace gfx::backend::vulkan::core