#ifndef GFX_VULKAN_INSTANCE_H
#define GFX_VULKAN_INSTANCE_H

#include "../CoreTypes.h"

#include <memory>
#include <vector>

namespace gfx::backend::vulkan::core {

class Adapter;

class Instance {
public:
    // Prevent copying
    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    Instance(const InstanceCreateInfo& createInfo);
    ~Instance();

    VkInstance handle() const;
    std::vector<VkPhysicalDevice> enumeratePhysicalDevices() const;

    // Adapter access - returns cached adapters
    Adapter* requestAdapter(const AdapterCreateInfo& createInfo) const;
    const std::vector<std::unique_ptr<Adapter>>& getAdapters() const;

    static std::vector<const char*> enumerateSupportedExtensions();

private:
    static std::vector<VkExtensionProperties> enumerateAvailableExtensions();

    void setupDebugMessenger();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    bool m_validationEnabled = false;
    mutable std::vector<std::unique_ptr<Adapter>> m_adapters; // Owned adapters, cached on creation
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_INSTANCE_H