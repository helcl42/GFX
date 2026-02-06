#include "Instance.h"

#include "Adapter.h"

#include "../util/Utils.h"

#include "../../../../common/Logger.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace gfx::backend::vulkan::core {

namespace {

    const char* vkMessageSeverityToString(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity)
    {
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            return "Error";
        } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            return "Warning";
        } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            return "Info";
        } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
            return "Verbose";
        } else {
            return "Unknown";
        }
    }

    const char* vkMessageTypeToString(VkDebugUtilsMessageTypeFlagsEXT messageType)
    {
        const char* typeStr = "";
        switch (messageType) {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            typeStr = "Validation";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            typeStr = "Performance";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
        default:
            typeStr = "General";
            break;
        }
        return typeStr;
    }

    bool isExtensionEnabled(const std::vector<std::string>& enabledExtensions, const char* extension)
    {
        for (const auto& enabledExt : enabledExtensions) {
            if (enabledExt == extension) {
                return true;
            }
        }
        return false;
    }

} // namespace

Instance::Instance(const InstanceCreateInfo& createInfo)
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = createInfo.applicationName;
    appInfo.applicationVersion = createInfo.applicationVersion;
    appInfo.pEngineName = "Gfx";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo vkCreateInfo{};
    vkCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vkCreateInfo.pApplicationInfo = &appInfo;

    // Extensions
    std::vector<const char*> extensions = {};
#ifndef GFX_HEADLESS_BUILD
    if (isExtensionEnabled(createInfo.enabledExtensions, extensions::SURFACE)) {
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

    m_validationEnabled = isExtensionEnabled(createInfo.enabledExtensions, extensions::DEBUG);
    if (m_validationEnabled) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Check if all requested extensions are availables
    const auto availableExtensions = enumerateAvailableExtensions();
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
        throw std::runtime_error("Failed to create Vulkan instance: " + std::string(vkResultToString(result)));
    }

    // Setup debug messenger if validation enabled
    if (m_validationEnabled) {
        setupDebugMessenger();
    }

    // Enumerate and cache all adapters
    auto physicalDevices = enumeratePhysicalDevices();
    m_adapters.reserve(physicalDevices.size());
    for (auto physicalDevice : physicalDevices) {
        m_adapters.push_back(std::make_unique<Adapter>(physicalDevice, this));
    }
}

Instance::~Instance()
{
    // Adapters are automatically cleaned up by unique_ptr
    m_adapters.clear();

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

VkInstance Instance::handle() const
{
    return m_instance;
}

std::vector<VkPhysicalDevice> Instance::enumeratePhysicalDevices() const
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        return {};
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
    return devices;
}

std::vector<VkExtensionProperties> Instance::enumerateAvailableExtensions()
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    return extensions;
}

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
    (void)pUserData;

    const char* severityStr = vkMessageSeverityToString(messageSeverity);
    const char* typeStr = vkMessageTypeToString(messageType);

    // Map Vulkan severity to Logger calls
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        gfx::common::Logger::instance().logError("Vulkan [{}|{}]: {}", severityStr, typeStr, pCallbackData->pMessage);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        gfx::common::Logger::instance().logWarning("Vulkan [{}|{}]: {}", severityStr, typeStr, pCallbackData->pMessage);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        gfx::common::Logger::instance().logInfo("Vulkan [{}|{}]: {}", severityStr, typeStr, pCallbackData->pMessage);
    } else {
        gfx::common::Logger::instance().logDebug("Vulkan [{}|{}]: {}", severityStr, typeStr, pCallbackData->pMessage);
    }
    return VK_FALSE;
}

std::vector<const char*> Instance::enumerateSupportedExtensions()
{
    // Map our internal extension names to actual Vulkan extension names
    struct ExtensionMapping {
        const char* internalName;
        const char* vkName;
    };

    static const ExtensionMapping knownExtensions[] = {
        { extensions::SURFACE, VK_KHR_SURFACE_EXTENSION_NAME },
        { extensions::DEBUG, VK_EXT_DEBUG_UTILS_EXTENSION_NAME }
    };

    const auto availableExtensions = enumerateAvailableExtensions();

    // Build the intersection: extensions we care about that are available
    std::vector<const char*> supportedExtensions;

    for (const auto& mapping : knownExtensions) {
        bool isAvailable = std::any_of(availableExtensions.begin(), availableExtensions.end(),
            [&mapping](const VkExtensionProperties& props) {
                return strcmp(props.extensionName, mapping.vkName) == 0;
            });

        if (isAvailable) {
            supportedExtensions.push_back(mapping.internalName);
        }
    }

    return supportedExtensions;
}

Adapter* Instance::requestAdapter(const AdapterCreateInfo& createInfo) const
{
    if (m_adapters.empty()) {
        throw std::runtime_error("No adapters available");
    }

    // If specific adapter index requested, return that adapter
    if (createInfo.adapterIndex != UINT32_MAX) {
        if (createInfo.adapterIndex >= m_adapters.size()) {
            throw std::runtime_error("Adapter index out of range");
        }
        return m_adapters[createInfo.adapterIndex].get();
    }

    // Map preference to Vulkan device type
    VkPhysicalDeviceType preferredType;
    bool allowFallback = true;

    switch (createInfo.devicePreference) {
    case DeviceTypePreference::HighPerformance:
        preferredType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        break;
    case DeviceTypePreference::LowPower:
        preferredType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
        break;
    case DeviceTypePreference::SoftwareRenderer:
        preferredType = VK_PHYSICAL_DEVICE_TYPE_CPU;
        allowFallback = false;
        break;
    }

    // Search for preferred device type
    for (const auto& adapter : m_adapters) {
        if (adapter->getProperties().deviceType == preferredType) {
            return adapter.get();
        }
    }

    // Fallback to first available (except for software renderer)
    if (!allowFallback) {
        throw std::runtime_error("Software renderer not available");
    }
    return m_adapters[0].get();
}

const std::vector<std::unique_ptr<Adapter>>& Instance::getAdapters() const
{
    return m_adapters;
}

} // namespace gfx::backend::vulkan::core