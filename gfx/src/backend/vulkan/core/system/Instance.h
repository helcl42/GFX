#ifndef GFX_VULKAN_INSTANCE_H
#define GFX_VULKAN_INSTANCE_H

#include "../CoreTypes.h"

#include "../../common/Common.h"

namespace gfx::backend::vulkan::core {

using DebugCallbackFunc = void (*)(DebugMessageSeverity severity, DebugMessageType type, const char* message, void* userData);

// Callback data wrapper for debug callbacks
struct CallbackData {
    DebugCallbackFunc callback;
    void* userData;
};

class Instance {
public:
    // Prevent copying
    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    Instance(const InstanceCreateInfo& createInfo);
    ~Instance();

    void setDebugCallback(DebugCallbackFunc callback, void* userData);

    VkInstance handle() const;

private:
    void setupDebugMessenger();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    bool m_validationEnabled = false;
    DebugCallbackFunc m_userCallback = nullptr;
    void* m_userCallbackData = nullptr; // Owned pointer, will be deleted in destructor
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_INSTANCE_H