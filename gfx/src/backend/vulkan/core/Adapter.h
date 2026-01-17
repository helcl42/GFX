#ifndef GFX_VULKAN_ADAPTER_H
#define GFX_VULKAN_ADAPTER_H

#include "CoreTypes.h"

namespace gfx::backend::vulkan::core {

class Instance;

class Adapter {
public:
    Adapter(const Adapter&) = delete;
    Adapter& operator=(const Adapter&) = delete;

    Adapter(Instance* instance, const AdapterCreateInfo& createInfo);
    Adapter(Instance* instance, VkPhysicalDevice physicalDevice);
    ~Adapter() = default;

    static uint32_t enumerate(Instance* instance, Adapter** outAdapters, uint32_t maxAdapters);

    VkPhysicalDevice handle() const;
    uint32_t getGraphicsQueueFamily() const;
    Instance* getInstance() const;
    const VkPhysicalDeviceProperties& getProperties() const;
    const VkPhysicalDeviceMemoryProperties& getMemoryProperties() const;

private:
    void initializeAdapterInfo();

    Instance* m_instance = nullptr;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties m_properties{};
    VkPhysicalDeviceMemoryProperties m_memoryProperties{};
    uint32_t m_graphicsQueueFamily = UINT32_MAX;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_ADAPTER_H