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
    static std::vector<VkLayerProperties> enumerateAvailableLayers();

    VkDebugUtilsMessengerEXT createDebugMessenger();
    void destroyDebugMessenger(VkDebugUtilsMessengerEXT messenger);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

    VkDebugReportCallbackEXT createDebugReport();
    void destroyDebugReport(VkDebugReportCallbackEXT callback);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback(VkDebugReportFlagsEXT msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location, int32_t msgCode, const char* pLayerPrefix, const char* pMsg, void* pUserData);

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkDebugReportCallbackEXT m_debugReportCallback = VK_NULL_HANDLE;
    bool m_validationEnabled = false;
    std::vector<std::unique_ptr<Adapter>> m_adapters; // Owned adapters, cached on creation
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_INSTANCE_H