#ifndef GFX_VULKAN_ADAPTER_H
#define GFX_VULKAN_ADAPTER_H

#include "../CoreTypes.h"

namespace gfx::backend::vulkan::core {

class Instance;

class Adapter {
public:
    Adapter(const Adapter&) = delete;
    Adapter& operator=(const Adapter&) = delete;

    Adapter(VkPhysicalDevice physicalDevice, Instance* instance);
    ~Adapter() = default;

    VkPhysicalDevice handle() const;
    uint32_t getGraphicsQueueFamily() const;
    Instance* getInstance() const;
    const VkPhysicalDeviceProperties& getProperties() const;
    const VkPhysicalDeviceMemoryProperties& getMemoryProperties() const;
    const VkPhysicalDeviceFeatures& getFeatures() const;
    std::vector<VkQueueFamilyProperties> getQueueFamilyProperties() const;
    std::vector<VkExtensionProperties> enumerateExtensionProperties() const;
    bool supportsPresentation(uint32_t queueFamilyIndex, VkSurfaceKHR surface) const;

    std::vector<const char*> enumerateSupportedExtensions() const;

private:
    void initializeAdapterInfo();

    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    Instance* m_instance = nullptr; // Non-owning
    VkPhysicalDeviceProperties m_properties{};
    VkPhysicalDeviceMemoryProperties m_memoryProperties{};
    VkPhysicalDeviceFeatures m_features{};
    uint32_t m_graphicsQueueFamily = UINT32_MAX;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_ADAPTER_H