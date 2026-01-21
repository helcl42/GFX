#include "Device.h"

#include "Adapter.h"
#include "Queue.h"

namespace gfx::backend::vulkan::core {

namespace {
    inline uint64_t makeQueueKey(uint32_t queueFamilyIndex, uint32_t queueIndex)
    {
        return (static_cast<uint64_t>(queueFamilyIndex) << 16) | queueIndex;
    }
} // anonymous namespace

Device::Device(Adapter* adapter, const DeviceCreateInfo& createInfo)
    : m_adapter(adapter)
{

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

    // Determine which queues to create
    std::vector<DeviceCreateInfo::QueueRequest> queueRequests;
    if (createInfo.queueRequests.empty()) {
        // Default: create one graphics queue
        queueRequests.push_back({ m_adapter->getGraphicsQueueFamily(), 0, createInfo.queuePriority });
    } else {
        queueRequests = createInfo.queueRequests;
    }

    // Group queue requests by family and find max queue index per family
    std::unordered_map<uint32_t, uint32_t> maxQueueIndexPerFamily;
    for (const auto& req : queueRequests) {
        maxQueueIndexPerFamily[req.queueFamilyIndex] = std::max(maxQueueIndexPerFamily[req.queueFamilyIndex], req.queueIndex);
    }

    // Build VkDeviceQueueCreateInfo for each family
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::vector<std::vector<float>> priorityStorage; // Keep priorities alive

    for (const auto& [familyIndex, maxIndex] : maxQueueIndexPerFamily) {
        uint32_t queueCount = maxIndex + 1;
        std::vector<float> priorities(queueCount, 1.0f);

        // Set specified priorities
        for (const auto& req : queueRequests) {
            if (req.queueFamilyIndex == familyIndex) {
                priorities[req.queueIndex] = req.priority;
            }
        }

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = familyIndex;
        queueCreateInfo.queueCount = queueCount;
        queueCreateInfo.pQueuePriorities = priorities.data();
        queueCreateInfos.push_back(queueCreateInfo);
        priorityStorage.push_back(std::move(priorities));
    }

    VkDeviceCreateInfo vkCreateInfo{};
    vkCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    vkCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    vkCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    vkCreateInfo.pEnabledFeatures = &deviceFeatures;
    vkCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    vkCreateInfo.ppEnabledExtensionNames = extensions.data();

    VkResult result = vkCreateDevice(m_adapter->handle(), &vkCreateInfo, nullptr, &m_device);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan device");
    }

    // Create Queue wrappers for all requested queues
    for (const auto& req : queueRequests) {
        VkQueue vkQueue = VK_NULL_HANDLE;
        vkGetDeviceQueue(m_device, req.queueFamilyIndex, req.queueIndex, &vkQueue);

        uint64_t key = makeQueueKey(req.queueFamilyIndex, req.queueIndex);
        auto queue = std::make_unique<Queue>(this, vkQueue, req.queueFamilyIndex);

        // Store default queue pointer (first one created)
        if (!m_defaultQueue) {
            m_defaultQueue = queue.get();
        }

        m_queues[key] = std::move(queue);
    }
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
    return m_defaultQueue;
}

Queue* Device::getQueueByIndex(uint32_t queueFamilyIndex, uint32_t queueIndex)
{
    uint64_t key = makeQueueKey(queueFamilyIndex, queueIndex);
    auto it = m_queues.find(key);
    return (it != m_queues.end()) ? it->second.get() : nullptr;
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