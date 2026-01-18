#ifndef GFX_VULKAN_INSTANCE_H
#define GFX_VULKAN_INSTANCE_H

#include "../CoreTypes.h"

namespace gfx::backend::vulkan::core {

class Instance {
public:
    // Prevent copying
    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    Instance(const InstanceCreateInfo& createInfo);
    ~Instance();

    VkInstance handle() const;

private:
    void setupDebugMessenger();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    bool m_validationEnabled = false;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_INSTANCE_H