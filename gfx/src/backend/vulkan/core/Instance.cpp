#include "Instance.h"

#include "../converter/Conversions.h"

#include <cstring>
#include <stdexcept>
#include <vector>

namespace gfx::backend::vulkan {

Instance::Instance(const InstanceCreateInfo& createInfo)
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = createInfo.applicationName;
    appInfo.applicationVersion = createInfo.applicationVersion;
    appInfo.pEngineName = "GfxWrapper";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo vkCreateInfo{};
    vkCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vkCreateInfo.pApplicationInfo = &appInfo;

    auto isFeatureEnabled = [&](InstanceFeatureType feature) {
        for (const auto& enabledFeature : createInfo.enabledFeatures) {
            if (enabledFeature == feature) {
                return true;
            }
        }
        return false;
    };

    // Extensions
    std::vector<const char*> extensions = {};
#ifndef GFX_HEADLESS_BUILD
    if (isFeatureEnabled(InstanceFeatureType::Surface)) {
        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef GFX_HAS_WIN32
        extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
#ifdef GFX_HAS_ANDROID
        extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#endif
#ifdef GFX_HAS_X11
        extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#endif
#ifdef GFX_HAS_XCB
        extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
#ifdef GFX_HAS_WAYLAND
        extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif
#if defined(GFX_HAS_COCOA) || defined(GFX_HAS_UIKIT)
        extensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
    }
#endif // GFX_HEADLESS_BUILD

    m_validationEnabled = createInfo.enableValidation;
    if (m_validationEnabled) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Check if all requested extensions are available
    uint32_t availableExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data());

    for (const char* requestedExt : extensions) {
        bool found = false;
        for (const auto& availableExt : availableExtensions) {
            if (strcmp(requestedExt, availableExt.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            std::string errorMsg = "Required Vulkan extension not available: ";
            errorMsg += requestedExt;
            throw std::runtime_error(errorMsg);
        }
    }

    vkCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    vkCreateInfo.ppEnabledExtensionNames = extensions.data();

    // Validation layers
    std::vector<const char*> layers;
    if (m_validationEnabled) {
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }
    vkCreateInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
    vkCreateInfo.ppEnabledLayerNames = layers.data();

    VkResult result = vkCreateInstance(&vkCreateInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance: " + std::string(converter::vkResultToString(result)));
    }

    // Setup debug messenger if validation enabled
    if (m_validationEnabled) {
        setupDebugMessenger();
    }
}

Instance::~Instance()
{
    // Clean up callback data if allocated
    if (m_userCallbackData) {
        delete static_cast<CallbackData*>(m_userCallbackData);
        m_userCallbackData = nullptr;
    }

    if (m_debugMessenger != VK_NULL_HANDLE) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            m_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func) {
            func(m_instance, m_debugMessenger, nullptr);
        }
    }
    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
    }
}

void Instance::setDebugCallback(DebugCallbackFunc callback, void* userData)
{
    // Clean up previously allocated data if any
    if (m_userCallbackData) {
        delete static_cast<CallbackData*>(m_userCallbackData);
    }

    m_userCallback = callback;
    m_userCallbackData = userData; // Take ownership, will delete in destructor
}

VkInstance Instance::handle() const { return m_instance; }

void Instance::setupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = this;

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        m_instance, "vkCreateDebugUtilsMessengerEXT");
    if (func) {
        func(m_instance, &createInfo, nullptr, &m_debugMessenger);
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL Instance::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    auto* instance = static_cast<Instance*>(pUserData);
    if (instance && instance->m_userCallback) {
        // Convert Vulkan types to internal enums
        DebugMessageSeverity severity = converter::convertVkDebugSeverity(messageSeverity);
        DebugMessageType type = converter::convertVkDebugType(messageType);

        instance->m_userCallback(severity, type, pCallbackData->pMessage, instance->m_userCallbackData);
    }
    return VK_FALSE;
}

} // namespace gfx::backend::vulkan