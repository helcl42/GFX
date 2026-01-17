#include "Device.h"

#include "Adapter.h"
#include "Queue.h"

namespace gfx::backend::vulkan::core {

Device::Device(Adapter* adapter, const DeviceCreateInfo& createInfo)
    : m_adapter(adapter)
{
    // Queue create info
    float queuePriority = createInfo.queuePriority;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = m_adapter->getGraphicsQueueFamily();
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    auto isFeatureEnabled = [&](DeviceFeatureType feature) {
        for (const auto& enabledFeature : createInfo.enabledFeatures) {
            if (enabledFeature == feature) {
                return true;
            }
        }
        return false;
    };

    // Device features
    VkPhysicalDeviceFeatures deviceFeatures{};

    // Device extensions
    std::vector<const char*> extensions;
#ifndef GFX_HEADLESS_BUILD
    if (isFeatureEnabled(DeviceFeatureType::Swapchain)) {
        extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }
#endif // GFX_HEADLESS_BUILD

    VkDeviceCreateInfo vkCreateInfo{};
    vkCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    vkCreateInfo.queueCreateInfoCount = 1;
    vkCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    vkCreateInfo.pEnabledFeatures = &deviceFeatures;
    vkCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    vkCreateInfo.ppEnabledExtensionNames = extensions.data();

    VkResult result = vkCreateDevice(m_adapter->handle(), &vkCreateInfo, nullptr, &m_device);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan device");
    }

    m_queue = std::make_unique<Queue>(this, m_adapter->getGraphicsQueueFamily());
}

Device::~Device()
{
    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
    }
}

void Device::waitIdle()
{
    vkDeviceWaitIdle(m_device);
}

VkDevice Device::handle() const
{
    return m_device;
}

Queue* Device::getQueue()
{
    return m_queue.get();
}

Adapter* Device::getAdapter()
{
    return m_adapter;
}

const VkPhysicalDeviceProperties& Device::getProperties() const
{
    return m_adapter->getProperties();
}

} // namespace gfx::backend::vulkan::core