#ifndef GFX_VULKAN_ENTITIES_H
#define GFX_VULKAN_ENTITIES_H

#include "CreateInfo.h" // Internal CreateInfo structs

#include "../common/VulkanCommon.h"
#include "../converter/GfxVulkanConverter.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace gfx::backend::vulkan {

// Forward declarations
class Instance;
class Adapter;
class Device;
class Queue;
class Surface;
class Swapchain;
class Buffer;
class Texture;
class TextureView;
class Sampler;
class Shader;
class RenderPipeline;
class ComputePipeline;
class CommandEncoder;
class RenderPassEncoder;
class ComputePassEncoder;
class BindGroupLayout;
class BindGroup;
class RenderPass;
class Framebuffer;
class Fence;
class Semaphore;

using DebugCallbackFunc = void (*)(DebugMessageSeverity severity, DebugMessageType type, const char* message, void* userData);

// Callback data wrapper for debug callbacks
struct CallbackData {
    DebugCallbackFunc callback;
    void* userData;
};

inline VkAccessFlags getVkAccessFlagsForLayout(VkImageLayout layout)
{
    switch (layout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        return 0;
    case VK_IMAGE_LAYOUT_GENERAL:
        return VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_ACCESS_SHADER_READ_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return VK_ACCESS_TRANSFER_READ_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return VK_ACCESS_TRANSFER_WRITE_BIT;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return VK_ACCESS_MEMORY_READ_BIT;
    default:
        return 0;
    }
}

class Instance {
public:
    // Prevent copying
    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    Instance(const InstanceCreateInfo& createInfo)
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

        // Extensions
        std::vector<const char*> extensions = {};
#ifndef GFX_HEADLESS_BUILD
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

    ~Instance()
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

    void setDebugCallback(DebugCallbackFunc callback, void* userData)
    {
        // Clean up previously allocated data if any
        if (m_userCallbackData) {
            delete static_cast<CallbackData*>(m_userCallbackData);
        }

        m_userCallback = callback;
        m_userCallbackData = userData; // Take ownership, will delete in destructor
    }

    VkInstance handle() const { return m_instance; }

private:
    void setupDebugMessenger()
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

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
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

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    bool m_validationEnabled = false;
    DebugCallbackFunc m_userCallback = nullptr;
    void* m_userCallbackData = nullptr; // Owned pointer, will be deleted in destructor
};

class Adapter {
public:
    Adapter(const Adapter&) = delete;
    Adapter& operator=(const Adapter&) = delete;

    Adapter(Instance* instance, const AdapterCreateInfo& createInfo)
        : m_instance(instance)
    {
        // Enumerate physical devices
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance->handle(), &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("No Vulkan physical devices found");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance->handle(), &deviceCount, devices.data());

        // If adapter index is specified, use that directly
        if (createInfo.adapterIndex != UINT32_MAX) {
            if (createInfo.adapterIndex >= deviceCount) {
                throw std::runtime_error("Adapter index out of range");
            }
            m_physicalDevice = devices[createInfo.adapterIndex];
        } else {
            // Otherwise, use preference-based selection
            // Determine preferred device type based on createInfo
            VkPhysicalDeviceType preferredType;
            switch (createInfo.devicePreference) {
            case DeviceTypePreference::SoftwareRenderer:
                preferredType = VK_PHYSICAL_DEVICE_TYPE_CPU;
                break;
            case DeviceTypePreference::LowPower:
                preferredType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
                break;
            case DeviceTypePreference::HighPerformance:
            default:
                preferredType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
                break;
            }

            // First pass: try to find preferred device type
            m_physicalDevice = VK_NULL_HANDLE;
            for (auto device : devices) {
                VkPhysicalDeviceProperties props;
                vkGetPhysicalDeviceProperties(device, &props);
                if (props.deviceType == preferredType) {
                    m_physicalDevice = device;
                    break;
                }
            }

            // Fallback: if preferred type not found, use first available device
            if (m_physicalDevice == VK_NULL_HANDLE) {
                m_physicalDevice = devices[0];
            }
        }

        initializeAdapterInfo();
    }

    // Constructor for wrapping a specific physical device (used by enumerate)
    Adapter(Instance* instance, VkPhysicalDevice physicalDevice)
        : m_instance(instance)
        , m_physicalDevice(physicalDevice)
    {
        initializeAdapterInfo();
    }

    // Static method to enumerate all available adapters
    // NOTE: Each adapter returned must be freed by the caller using the backend's adapterDestroy method
    // (e.g., gfxAdapterDestroy() in the public API)
    static uint32_t enumerate(Instance* instance, Adapter** outAdapters, uint32_t maxAdapters)
    {
        if (!instance) {
            return 0;
        }

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance->handle(), &deviceCount, nullptr);

        if (deviceCount == 0) {
            return 0;
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance->handle(), &deviceCount, devices.data());

        // Create an adapter for each physical device
        uint32_t count = std::min(deviceCount, maxAdapters);
        if (outAdapters) {
            for (uint32_t i = 0; i < count; ++i) {
                outAdapters[i] = new Adapter(instance, devices[i]);
            }
        }

        return count;
    }

    VkPhysicalDevice handle() const { return m_physicalDevice; }
    uint32_t getGraphicsQueueFamily() const { return m_graphicsQueueFamily; }
    Instance* getInstance() const { return m_instance; }
    const VkPhysicalDeviceProperties& getProperties() const { return m_properties; }
    const VkPhysicalDeviceMemoryProperties& getMemoryProperties() const { return m_memoryProperties; }

private:
    void initializeAdapterInfo()
    {
        vkGetPhysicalDeviceProperties(m_physicalDevice, &m_properties);
        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_memoryProperties);

        // Find graphics queue family
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilyCount; ++i) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                m_graphicsQueueFamily = i;
                break;
            }
        }

        if (m_graphicsQueueFamily == UINT32_MAX) {
            throw std::runtime_error("Failed to find graphics queue family for adapter");
        }
    }

    Instance* m_instance = nullptr;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties m_properties{};
    VkPhysicalDeviceMemoryProperties m_memoryProperties{};
    uint32_t m_graphicsQueueFamily = UINT32_MAX;
};

class Queue {
public:
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    Queue(Device* device, uint32_t queueFamily);

    VkQueue handle() const { return m_queue; }
    VkDevice device() const;
    VkPhysicalDevice physicalDevice() const;
    uint32_t family() const { return m_queueFamily; }

    VkResult submit(const SubmitInfo& submitInfo);

    // Write data directly to a buffer by mapping it
    void writeBuffer(Buffer* buffer, uint64_t offset, const void* data, uint64_t size);

    // Write data directly to a texture using staging buffer
    void writeTexture(Texture* texture, const VkOffset3D& origin, uint32_t mipLevel,
        const void* data, uint64_t dataSize,
        const VkExtent3D& extent, VkImageLayout finalLayout);

    // Wait for all operations on the queue to complete
    void waitIdle();

private:
    VkQueue m_queue = VK_NULL_HANDLE;
    Device* m_device = nullptr;
    uint32_t m_queueFamily = 0;
};

class Device {
public:
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    Device(Adapter* adapter, const DeviceCreateInfo& createInfo)
        : m_adapter(adapter)
    {
        // Queue create info
        float queuePriority = createInfo.queuePriority;
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = m_adapter->getGraphicsQueueFamily();
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        // Device features
        VkPhysicalDeviceFeatures deviceFeatures{};

        // Device extensions
        std::vector<const char*> extensions;
#ifndef GFX_HEADLESS_BUILD
        extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
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

    ~Device()
    {
        if (m_device != VK_NULL_HANDLE) {
            vkDestroyDevice(m_device, nullptr);
        }
    }

    VkDevice handle() const { return m_device; }
    Queue* getQueue() { return m_queue.get(); }
    Adapter* getAdapter() { return m_adapter; }
    const VkPhysicalDeviceProperties& getProperties() const
    {
        return m_adapter->getProperties();
    }

    void waitIdle()
    {
        vkDeviceWaitIdle(m_device);
    }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    Adapter* m_adapter = nullptr; // Non-owning pointer
    std::unique_ptr<Queue> m_queue;
};

class Shader {
public:
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Device* device, const ShaderCreateInfo& createInfo)
        : m_device(device)
    {
        if (createInfo.entryPoint) {
            m_entryPoint = createInfo.entryPoint;
        } else {
            m_entryPoint = "main";
        }

        VkShaderModuleCreateInfo vkCreateInfo{};
        vkCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vkCreateInfo.codeSize = createInfo.codeSize;
        vkCreateInfo.pCode = reinterpret_cast<const uint32_t*>(createInfo.code);

        VkResult result = vkCreateShaderModule(m_device->handle(), &vkCreateInfo, nullptr, &m_shaderModule);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module");
        }
    }

    ~Shader()
    {
        if (m_shaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(m_device->handle(), m_shaderModule, nullptr);
        }
    }

    VkShaderModule handle() const { return m_shaderModule; }
    const char* entryPoint() const { return m_entryPoint.c_str(); }

private:
    VkShaderModule m_shaderModule = VK_NULL_HANDLE;
    std::string m_entryPoint;
    Device* m_device = nullptr;
};

class BindGroupLayout {
public:
    BindGroupLayout(const BindGroupLayout&) = delete;
    BindGroupLayout& operator=(const BindGroupLayout&) = delete;

    BindGroupLayout(Device* device, const BindGroupLayoutCreateInfo& createInfo)
        : m_device(device)
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        for (const auto& entry : createInfo.entries) {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = entry.binding;
            binding.descriptorCount = 1;
            binding.descriptorType = entry.descriptorType;
            binding.stageFlags = entry.stageFlags;

            bindings.push_back(binding);

            // Store binding info for later queries
            m_bindingTypes[entry.binding] = entry.descriptorType;
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        VkResult result = vkCreateDescriptorSetLayout(m_device->handle(), &layoutInfo, nullptr, &m_layout);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout");
        }
    }

    ~BindGroupLayout()
    {
        if (m_layout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_device->handle(), m_layout, nullptr);
        }
    }

    VkDescriptorSetLayout handle() const { return m_layout; }

    VkDescriptorType getBindingType(uint32_t binding) const
    {
        auto it = m_bindingTypes.find(binding);
        if (it != m_bindingTypes.end()) {
            return it->second;
        }
        return VK_DESCRIPTOR_TYPE_MAX_ENUM; // Invalid
    }

private:
    VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
    Device* m_device = nullptr;
    std::unordered_map<uint32_t, VkDescriptorType> m_bindingTypes;
};

class Surface {
public:
    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;

    Surface(Adapter* adapter, const SurfaceCreateInfo& createInfo)
        : m_adapter(adapter)
    {
#ifdef GFX_HEADLESS_BUILD
        (void)createInfo;
        throw std::runtime_error("Surface creation is not available in headless builds");
#else
        switch (createInfo.windowHandle.platform) {
#ifdef GFX_HAS_WIN32
        case PlatformWindowHandle::Platform::Win32:
            m_surface = createSurfaceWin32(m_adapter->getInstance()->handle(), createInfo.windowHandle);
            break;
#endif
#ifdef GFX_HAS_ANDROID
        case PlatformWindowHandle::Platform::Android:
            m_surface = createSurfaceAndroid(m_adapter->getInstance()->handle(), createInfo.windowHandle);
            break;
#endif
#ifdef GFX_HAS_X11
        case PlatformWindowHandle::Platform::Xlib:
            m_surface = createSurfaceXlib(m_adapter->getInstance()->handle(), createInfo.windowHandle);
            break;
#endif
#ifdef GFX_HAS_XCB
        case PlatformWindowHandle::Platform::Xcb:
            m_surface = createSurfaceXCB(m_adapter->getInstance()->handle(), createInfo.windowHandle);
            break;
#endif
#ifdef GFX_HAS_WAYLAND
        case PlatformWindowHandle::Platform::Wayland:
            m_surface = createSurfaceWayland(m_adapter->getInstance()->handle(), createInfo.windowHandle);
            break;
#endif
#if defined(GFX_HAS_COCOA) || defined(GFX_HAS_UIKIT)
        case PlatformWindowHandle::Platform::Metal:
            m_surface = createSurfaceMetal(m_adapter->getInstance()->handle(), createInfo.windowHandle);
            break;
#endif
        // Other platforms can be added here
        default:
            throw std::runtime_error("Unsupported windowing platform");
        }
#endif // GFX_HEADLESS_BUILD
    }

    ~Surface()
    {
        if (m_surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(m_adapter->getInstance()->handle(), m_surface, nullptr);
        }
    }

    VkInstance instance() const { return m_adapter->getInstance()->handle(); }
    VkPhysicalDevice physicalDevice() const { return m_adapter->handle(); }
    VkSurfaceKHR handle() const { return m_surface; }

    std::vector<VkSurfaceFormatKHR> getSupportedFormats() const
    {
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_adapter->handle(), m_surface, &formatCount, nullptr);

        if (formatCount == 0) {
            return {};
        }

        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_adapter->handle(), m_surface, &formatCount, formats.data());
        return formats;
    }

    std::vector<VkPresentModeKHR> getSupportedPresentModes() const
    {
        uint32_t modeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_adapter->handle(), m_surface, &modeCount, nullptr);

        if (modeCount == 0) {
            return {};
        }

        std::vector<VkPresentModeKHR> presentModes(modeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_adapter->handle(), m_surface, &modeCount, presentModes.data());
        return presentModes;
    }

private:
#ifndef GFX_HEADLESS_BUILD
#ifdef GFX_HAS_WIN32
    static VkSurfaceKHR createSurfaceWin32(VkInstance instance, const PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.win32.hwnd || !windowHandle.handle.win32.hinstance) {
            throw std::runtime_error("Invalid Win32 window or instance handle");
        }

        VkWin32SurfaceCreateInfoKHR vkCreateInfo{};
        vkCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        vkCreateInfo.hwnd = windowHandle.handle.win32.hwnd;
        vkCreateInfo.hinstance = windowHandle.handle.win32.hinstance;

        VkSurfaceKHR surface;
        if (vkCreateWin32SurfaceKHR(instance, &vkCreateInfo, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Win32 surface");
        }
        return surface;
    }
#endif
#ifdef GFX_HAS_ANDROID
    static VkSurfaceKHR createSurfaceAndroid(VkInstance instance, const PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.android.window) {
            throw std::runtime_error("Invalid Android window handle");
        }

        VkAndroidSurfaceCreateInfoKHR vkCreateInfo{};
        vkCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
        vkCreateInfo.window = windowHandle.handle.android.window;

        VkSurfaceKHR surface;
        if (vkCreateAndroidSurfaceKHR(instance, &vkCreateInfo, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Android surface");
        }
        return surface;
    }
#endif
#ifdef GFX_HAS_X11
    static VkSurfaceKHR createSurfaceXlib(VkInstance instance, const PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.xlib.display || !windowHandle.handle.xlib.window) {
            throw std::runtime_error("Invalid Xlib display handle or window handle");
        }

        VkXlibSurfaceCreateInfoKHR vkCreateInfo{};
        vkCreateInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
        vkCreateInfo.dpy = static_cast<Display*>(windowHandle.handle.xlib.display);
        vkCreateInfo.window = static_cast<Window>(windowHandle.handle.xlib.window);

        VkSurfaceKHR surface;
        if (vkCreateXlibSurfaceKHR(instance, &vkCreateInfo, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Xlib surface");
        }
        return surface;
    }
#endif
#ifdef GFX_HAS_XCB
    static VkSurfaceKHR createSurfaceXCB(VkInstance instance, const PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.xcb.window || !windowHandle.handle.xcb.connection) {
            throw std::runtime_error("Invalid XCB window or connection handle");
        }

        VkXcbSurfaceCreateInfoKHR vkCreateInfo{};
        vkCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        vkCreateInfo.connection = static_cast<xcb_connection_t*>(windowHandle.handle.xcb.connection);
        vkCreateInfo.window = static_cast<xcb_window_t>(windowHandle.handle.xcb.window);

        VkSurfaceKHR surface;
        if (vkCreateXcbSurfaceKHR(instance, &vkCreateInfo, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create XCB surface");
        }
        return surface;
    }
#endif
#ifdef GFX_HAS_WAYLAND
    static VkSurfaceKHR createSurfaceWayland(VkInstance instance, const PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.wayland.surface || !windowHandle.handle.wayland.display) {
            throw std::runtime_error("Invalid Wayland surface or display handle");
        }

        VkWaylandSurfaceCreateInfoKHR vkCreateInfo{};
        vkCreateInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        vkCreateInfo.display = static_cast<wl_display*>(windowHandle.handle.wayland.display);
        vkCreateInfo.surface = static_cast<wl_surface*>(windowHandle.handle.wayland.surface);

        VkSurfaceKHR surface;
        if (vkCreateWaylandSurfaceKHR(instance, &vkCreateInfo, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Wayland surface");
        }
        return surface;
    }
#endif
#if defined(GFX_HAS_COCOA) || defined(GFX_HAS_UIKIT)
    static VkSurfaceKHR createSurfaceMetal(VkInstance instance, const PlatformWindowHandle& windowHandle)
    {
        if (!windowHandle.handle.metalLayer) {
            throw std::runtime_error("Invalid Metal layer handle");
        }

        VkMetalSurfaceCreateInfoEXT metalCreateInfo{};
        metalCreateInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
        metalCreateInfo.pLayer = windowHandle.handle.metalLayer;

        VkSurfaceKHR surface;
        if (vkCreateMetalSurfaceEXT(instance, &metalCreateInfo, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Metal surface");
        }
        return surface;
    }
#endif
#endif // GFX_HEADLESS_BUILD

private:
    Adapter* m_adapter = nullptr;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
};

class Swapchain {
public:
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    Swapchain(Device* device, Surface* surface, const SwapchainCreateInfo& createInfo)
        : m_device(device)
        , m_surface(surface)
    {
        uint32_t queueFamily = device->getAdapter()->getGraphicsQueueFamily();

        // Check if queue family supports presentation
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device->getAdapter()->handle(), queueFamily, surface->handle(), &presentSupport);
        if (presentSupport != VK_TRUE) {
            throw std::runtime_error("Selected queue family does not support presentation");
        }

        // Query and choose format
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_surface->physicalDevice(), m_surface->handle(), &formatCount, nullptr);
        if (formatCount == 0) {
            throw std::runtime_error("No surface formats available for swapchain");
        }
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_surface->physicalDevice(), m_surface->handle(), &formatCount, formats.data());

        VkSurfaceFormatKHR selectedFormat = formats[0];
        for (const auto& availableFormat : formats) {
            if (availableFormat.format == createInfo.format) {
                selectedFormat = availableFormat;
                break;
            }
        }
        m_info.format = selectedFormat.format;

        // Query and choose present mode
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_surface->physicalDevice(), m_surface->handle(), &presentModeCount, nullptr);
        if (presentModeCount == 0) {
            throw std::runtime_error("No present modes available for swapchain");
        }
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_surface->physicalDevice(), m_surface->handle(), &presentModeCount, presentModes.data());

        m_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        for (const auto& availableMode : presentModes) {
            if (availableMode == createInfo.presentMode) {
                m_info.presentMode = availableMode;
                break;
            }
        }

        // Query surface capabilities
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_surface->physicalDevice(), m_surface->handle(), &capabilities);

        // Determine actual swapchain extent and store directly in m_info
        // If currentExtent is defined, we MUST use it. Otherwise, we can choose within min/max bounds.
        if (capabilities.currentExtent.width != UINT32_MAX) {
            // Window manager is telling us the size - we must use it
            m_info.width = capabilities.currentExtent.width;
            m_info.height = capabilities.currentExtent.height;
        } else {
            // We can choose the extent within bounds
            m_info.width = std::max(capabilities.minImageExtent.width,
                std::min(createInfo.width, capabilities.maxImageExtent.width));
            m_info.height = std::max(capabilities.minImageExtent.height,
                std::min(createInfo.height, capabilities.maxImageExtent.height));
        }

        // Create swapchain
        VkSwapchainCreateInfoKHR vkCreateInfo{};
        vkCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        vkCreateInfo.surface = m_surface->handle();
        vkCreateInfo.minImageCount = std::min(createInfo.imageCount, capabilities.minImageCount + 1);
        if (capabilities.maxImageCount > 0) {
            vkCreateInfo.minImageCount = std::min(vkCreateInfo.minImageCount, capabilities.maxImageCount);
        }
        vkCreateInfo.imageFormat = m_info.format;
        vkCreateInfo.imageColorSpace = selectedFormat.colorSpace;
        vkCreateInfo.imageExtent = { m_info.width, m_info.height };
        vkCreateInfo.imageArrayLayers = 1;
        vkCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        vkCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateInfo.preTransform = capabilities.currentTransform;
        vkCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        vkCreateInfo.presentMode = m_info.presentMode;
        vkCreateInfo.clipped = VK_TRUE;

        VkResult result = vkCreateSwapchainKHR(m_device->handle(), &vkCreateInfo, nullptr, &m_swapchain);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain");
        }

        // Get swapchain images
        uint32_t imageCount;
        vkGetSwapchainImagesKHR(m_device->handle(), m_swapchain, &imageCount, nullptr);
        m_images.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device->handle(), m_swapchain, &imageCount, m_images.data());

        // Update SwapchainInfo with final imageCount (width/height/format/presentMode already set)
        m_info.imageCount = imageCount;

        m_textures.reserve(imageCount);
        m_textureViews.reserve(imageCount);
        for (size_t i = 0; i < imageCount; ++i) {
            // Create non-owning Texture wrapper for swapchain image
            TextureCreateInfo textureCreateInfo{};
            textureCreateInfo.format = m_info.format;
            textureCreateInfo.size = { m_info.width, m_info.height, 1 };
            textureCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            textureCreateInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
            textureCreateInfo.mipLevelCount = 1;
            textureCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            textureCreateInfo.arrayLayers = 1;
            textureCreateInfo.flags = 0;
            m_textures.push_back(std::make_unique<Texture>(m_device, m_images[i], textureCreateInfo));

            // Create TextureView for the texture
            TextureViewCreateInfo viewCreateInfo{};
            viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewCreateInfo.format = VK_FORMAT_UNDEFINED; // Use texture's format
            viewCreateInfo.baseMipLevel = 0;
            viewCreateInfo.mipLevelCount = 1;
            viewCreateInfo.baseArrayLayer = 0;
            viewCreateInfo.arrayLayerCount = 1;
            m_textureViews.push_back(std::make_unique<TextureView>(m_textures[i].get(), viewCreateInfo));
        }

        // Get present queue (assume queue family 0)
        vkGetDeviceQueue(m_device->handle(), 0, 0, &m_presentQueue);

        // Don't pre-acquire an image - let explicit acquire handle it
        m_currentImageIndex = 0;
    }

    ~Swapchain()
    {
        // Explicitly destroy TextureViews and Textures before destroying the swapchain
        // This ensures VkImageViews are destroyed before the swapchain's VkImages
        m_textureViews.clear();
        m_textures.clear();

        if (m_swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_device->handle(), m_swapchain, nullptr);
        }
    }

    VkSwapchainKHR handle() const { return m_swapchain; }
    uint32_t getImageCount() const { return m_info.imageCount; }
    Texture* getTexture(uint32_t index) const { return m_textures[index].get(); }
    Texture* getCurrentTexture() const { return m_textures[m_currentImageIndex].get(); }
    TextureView* getTextureView(uint32_t index) const { return m_textureViews[index].get(); }
    TextureView* getCurrentTextureView() const { return m_textureViews[m_currentImageIndex].get(); }
    VkFormat getFormat() const { return m_info.format; }
    uint32_t getWidth() const { return m_info.width; }
    uint32_t getHeight() const { return m_info.height; }
    uint32_t getCurrentImageIndex() const { return m_currentImageIndex; }
    VkPresentModeKHR getPresentMode() const { return m_info.presentMode; }
    const SwapchainInfo& getInfo() const { return m_info; }

    VkResult acquireNextImage(uint64_t timeoutNs, VkSemaphore semaphore, VkFence fence, uint32_t* outImageIndex)
    {
        VkResult result = vkAcquireNextImageKHR(m_device->handle(), m_swapchain, timeoutNs, semaphore, fence, outImageIndex);
        if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR) {
            m_currentImageIndex = *outImageIndex;
        }
        return result;
    }

    VkResult present(const std::vector<VkSemaphore>& waitSemaphores)
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
        presentInfo.pWaitSemaphores = waitSemaphores.empty() ? nullptr : waitSemaphores.data();
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_swapchain;
        presentInfo.pImageIndices = &m_currentImageIndex;

        return vkQueuePresentKHR(m_presentQueue, &presentInfo);
    }

private:
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    Device* m_device = nullptr;
    Surface* m_surface = nullptr;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    std::vector<VkImage> m_images;
    std::vector<std::unique_ptr<Texture>> m_textures;
    std::vector<std::unique_ptr<TextureView>> m_textureViews;
    SwapchainInfo m_info{};
    uint32_t m_currentImageIndex = 0;
};

class Buffer {
public:
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    // Owning constructor - creates and manages VkBuffer and memory
    Buffer(Device* device, const BufferCreateInfo& createInfo)
        : m_device(device)
        , m_ownsResources(true)
        , m_memory(VK_NULL_HANDLE)
        , m_info(createBufferInfo(createInfo))
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = m_info.size;
        bufferInfo.usage = m_info.usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateBuffer(m_device->handle(), &bufferInfo, nullptr, &m_buffer);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create buffer");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_device->handle(), m_buffer, &memRequirements);

        // Find memory type
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_device->getAdapter()->handle(), &memProperties);

        uint32_t memoryTypeIndex = UINT32_MAX;
        VkMemoryPropertyFlags properties = createInfo.mapped
            ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
            if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                memoryTypeIndex = i;
                break;
            }
        }

        if (memoryTypeIndex == UINT32_MAX) {
            vkDestroyBuffer(m_device->handle(), m_buffer, nullptr);
            throw std::runtime_error("Failed to find suitable memory type");
        }

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = memoryTypeIndex;

        result = vkAllocateMemory(m_device->handle(), &allocInfo, nullptr, &m_memory);
        if (result != VK_SUCCESS) {
            vkDestroyBuffer(m_device->handle(), m_buffer, nullptr);
            throw std::runtime_error("Failed to allocate buffer memory");
        }

        vkBindBufferMemory(m_device->handle(), m_buffer, m_memory, 0);
    }

    // Non-owning constructor - wraps an existing VkBuffer
    Buffer(Device* device, VkBuffer buffer, const BufferImportInfo& importInfo)
        : m_device(device)
        , m_ownsResources(false)
        , m_buffer(buffer)
        , m_memory(VK_NULL_HANDLE)
        , m_info(createBufferInfo(importInfo))
    {
    }

    ~Buffer()
    {
        if (m_ownsResources) {
            if (m_memory != VK_NULL_HANDLE) {
                vkFreeMemory(m_device->handle(), m_memory, nullptr);
            }
            if (m_buffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(m_device->handle(), m_buffer, nullptr);
            }
        }
    }

    VkBuffer handle() const { return m_buffer; }

    void* map()
    {
        if (!m_info.mapped) {
            return nullptr;
        }

        void* data;
        vkMapMemory(m_device->handle(), m_memory, 0, m_info.size, 0, &data);
        return data;
    }

    void unmap()
    {
        if (!m_info.mapped) {
            return;
        }

        vkUnmapMemory(m_device->handle(), m_memory);
    }

    size_t size() const { return m_info.size; }
    VkBufferUsageFlags getUsage() const { return m_info.usage; }
    const BufferInfo& getInfo() const { return m_info; }

private:
    static BufferInfo createBufferInfo(const BufferCreateInfo& createInfo)
    {
        BufferInfo info{};
        info.size = createInfo.size;
        info.usage = createInfo.usage;
        info.mapped = createInfo.mapped;
        return info;
    }

    static BufferInfo createBufferInfo(const BufferImportInfo& importInfo)
    {
        BufferInfo info{};
        info.size = importInfo.size;
        info.usage = importInfo.usage;
        info.mapped = importInfo.mapped;
        return info;
    }

private:
    Device* m_device = nullptr;
    bool m_ownsResources = true;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    BufferInfo m_info{};
};

class Texture {
public:
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Owning constructor - creates and manages VkImage and memory
    Texture(Device* device, const TextureCreateInfo& createInfo)
        : m_device(device)
        , m_ownsResources(true)
        , m_info(createTextureInfo(createInfo))
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = m_info.imageType;
        imageInfo.extent = m_info.size;
        imageInfo.mipLevels = m_info.mipLevelCount;
        imageInfo.arrayLayers = m_info.arrayLayers;
        imageInfo.flags = createInfo.flags;
        imageInfo.format = m_info.format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Always create in UNDEFINED, transition explicitly
        imageInfo.usage = m_info.usage;

        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = m_info.sampleCount;

        VkResult result = vkCreateImage(m_device->handle(), &imageInfo, nullptr, &m_image);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_device->handle(), m_image, &memRequirements);

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_device->getAdapter()->handle(), &memProperties);

        uint32_t memoryTypeIndex = UINT32_MAX;
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
            if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                memoryTypeIndex = i;
                break;
            }
        }

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = memoryTypeIndex;

        result = vkAllocateMemory(m_device->handle(), &allocInfo, nullptr, &m_memory);
        if (result != VK_SUCCESS) {
            vkDestroyImage(m_device->handle(), m_image, nullptr);
            throw std::runtime_error("Failed to allocate image memory");
        }

        vkBindImageMemory(m_device->handle(), m_image, m_memory, 0);
    }

    // Non-owning constructor - wraps an existing VkImage (e.g., from swapchain)
    Texture(Device* device, VkImage image, const TextureCreateInfo& createInfo)
        : m_device(device)
        , m_ownsResources(false)
        , m_info(createTextureInfo(createInfo))
        , m_image(image)
    {
    }

    // Non-owning constructor for imported textures
    Texture(Device* device, VkImage image, const TextureImportInfo& importInfo)
        : m_device(device)
        , m_ownsResources(false)
        , m_info(createTextureInfo(importInfo))
        , m_image(image)
    {
    }

    ~Texture()
    {
        if (m_ownsResources) {
            if (m_memory != VK_NULL_HANDLE) {
                vkFreeMemory(m_device->handle(), m_memory, nullptr);
            }
            if (m_image != VK_NULL_HANDLE) {
                vkDestroyImage(m_device->handle(), m_image, nullptr);
            }
        }
    }

    VkImage handle() const { return m_image; }
    VkDevice device() const { return m_device->handle(); }
    VkImageType getImageType() const { return m_info.imageType; }
    VkExtent3D getSize() const { return m_info.size; }
    uint32_t getArrayLayers() const { return m_info.arrayLayers; }
    VkFormat getFormat() const { return m_info.format; }
    uint32_t getMipLevelCount() const { return m_info.mipLevelCount; }
    VkSampleCountFlagBits getSampleCount() const { return m_info.sampleCount; }
    VkImageUsageFlags getUsage() const { return m_info.usage; }
    const TextureInfo& getInfo() const { return m_info; }

    VkImageLayout getLayout() const { return m_currentLayout; }
    void setLayout(VkImageLayout layout) { m_currentLayout = layout; }

    void transitionLayout(CommandEncoder* encoder, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount);
    void transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount);
    void generateMipmaps(CommandEncoder* encoder);
    void generateMipmapsRange(CommandEncoder* encoder, uint32_t baseMipLevel, uint32_t levelCount);

private:
    // Static helper to create TextureInfo from TextureCreateInfo
    static TextureInfo createTextureInfo(const TextureCreateInfo& info)
    {
        return TextureInfo{
            info.imageType,
            info.size,
            info.arrayLayers,
            info.format,
            info.mipLevelCount,
            info.sampleCount,
            info.usage
        };
    }

    // Static helper to create TextureInfo from TextureImportInfo
    static TextureInfo createTextureInfo(const TextureImportInfo& info)
    {
        return TextureInfo{
            info.imageType,
            info.size,
            info.arrayLayers,
            info.format,
            info.mipLevelCount,
            info.sampleCount,
            info.usage
        };
    }

    Device* m_device = nullptr;
    bool m_ownsResources = true;
    TextureInfo m_info{};
    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkImageLayout m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};

class TextureView {
public:
    TextureView(const TextureView&) = delete;
    TextureView& operator=(const TextureView&) = delete;

    TextureView(Texture* texture, const TextureViewCreateInfo& createInfo)
        : m_device(texture->device())
        , m_texture(texture)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = texture->handle();
        viewInfo.viewType = createInfo.viewType;
        // Use texture's format if VK_FORMAT_UNDEFINED
        m_format = (createInfo.format == VK_FORMAT_UNDEFINED)
            ? texture->getFormat()
            : createInfo.format;
        viewInfo.format = m_format;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = converter::getImageAspectMask(viewInfo.format);
        viewInfo.subresourceRange.baseMipLevel = createInfo.baseMipLevel;
        viewInfo.subresourceRange.levelCount = createInfo.mipLevelCount;
        viewInfo.subresourceRange.baseArrayLayer = createInfo.baseArrayLayer;
        viewInfo.subresourceRange.layerCount = createInfo.arrayLayerCount;

        VkResult result = vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageView);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image view");
        }
    }

    ~TextureView()
    {
        if (m_imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(m_device, m_imageView, nullptr);
        }
    }

    VkImageView handle() const { return m_imageView; }
    Texture* getTexture() const { return m_texture; }
    VkFormat getFormat() const { return m_format; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    Texture* m_texture = nullptr;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkFormat m_format; // View format (may differ from texture format)
};

class Sampler {
public:
    Sampler(const Sampler&) = delete;
    Sampler& operator=(const Sampler&) = delete;

    Sampler(Device* device, const SamplerCreateInfo& createInfo)
        : m_device(device)
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        // Address modes
        samplerInfo.addressModeU = createInfo.addressModeU;
        samplerInfo.addressModeV = createInfo.addressModeV;
        samplerInfo.addressModeW = createInfo.addressModeW;

        // Filter modes
        samplerInfo.magFilter = createInfo.magFilter;
        samplerInfo.minFilter = createInfo.minFilter;
        samplerInfo.mipmapMode = createInfo.mipmapMode;

        // LOD
        samplerInfo.minLod = createInfo.lodMinClamp;
        samplerInfo.maxLod = createInfo.lodMaxClamp;

        // Anisotropy
        if (createInfo.maxAnisotropy > 1) {
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = static_cast<float>(createInfo.maxAnisotropy);
        } else {
            samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.maxAnisotropy = 1.0f;
        }

        // Compare operation for depth textures
        if (createInfo.compareOp != VK_COMPARE_OP_MAX_ENUM) {
            samplerInfo.compareEnable = VK_TRUE;
            samplerInfo.compareOp = createInfo.compareOp;
        } else {
            samplerInfo.compareEnable = VK_FALSE;
        }

        VkResult result = vkCreateSampler(m_device->handle(), &samplerInfo, nullptr, &m_sampler);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create sampler");
        }
    }

    ~Sampler()
    {
        if (m_sampler != VK_NULL_HANDLE) {
            vkDestroySampler(m_device->handle(), m_sampler, nullptr);
        }
    }

    VkSampler handle() const { return m_sampler; }

private:
    VkSampler m_sampler = VK_NULL_HANDLE;
    Device* m_device = nullptr;
};

class BindGroup {
public:
    BindGroup(const BindGroup&) = delete;
    BindGroup& operator=(const BindGroup&) = delete;

    BindGroup(Device* device, const BindGroupCreateInfo& createInfo)
        : m_device(device)
    {
        // Count actual descriptors needed by type
        std::unordered_map<VkDescriptorType, uint32_t> descriptorCounts;

        for (const auto& entry : createInfo.entries) {
            ++descriptorCounts[entry.descriptorType];
        }

        // Create descriptor pool with exact sizes
        std::vector<VkDescriptorPoolSize> poolSizes;
        for (const auto& [type, count] : descriptorCounts) {
            VkDescriptorPoolSize poolSize{};
            poolSize.type = type;
            poolSize.descriptorCount = count;
            poolSizes.push_back(poolSize);
        }

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 1; // Each BindGroup only allocates one descriptor set

        VkResult result = vkCreateDescriptorPool(m_device->handle(), &poolInfo, nullptr, &m_pool);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool");
        }

        // Allocate descriptor set
        VkDescriptorSetLayout setLayout = createInfo.layout;

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &setLayout;

        result = vkAllocateDescriptorSets(m_device->handle(), &allocInfo, &m_descriptorSet);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor set");
        }

        // Update descriptor set
        // Build all the descriptor info arrays first
        std::vector<VkDescriptorBufferInfo> bufferInfos;
        std::vector<VkDescriptorImageInfo> imageInfos;
        std::vector<VkWriteDescriptorSet> descriptorWrites;

        // Reserve space to avoid reallocation and pointer invalidation
        bufferInfos.reserve(createInfo.entries.size());
        imageInfos.reserve(createInfo.entries.size());
        descriptorWrites.reserve(createInfo.entries.size());

        // Track indices for buffer and image infos
        size_t bufferInfoIndex = 0;
        size_t imageInfoIndex = 0;

        for (const auto& entry : createInfo.entries) {
            if (entry.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || entry.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = entry.buffer;
                bufferInfo.offset = entry.bufferOffset;
                bufferInfo.range = entry.bufferSize;
                bufferInfos.push_back(bufferInfo);

                VkWriteDescriptorSet descriptorWrite{};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = m_descriptorSet;
                descriptorWrite.dstBinding = entry.binding;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = entry.descriptorType;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pBufferInfo = &bufferInfos[bufferInfoIndex++];

                descriptorWrites.push_back(descriptorWrite);
            } else if (entry.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER) {
                VkDescriptorImageInfo imageInfo{};
                imageInfo.sampler = entry.sampler;
                imageInfo.imageView = VK_NULL_HANDLE;
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageInfos.push_back(imageInfo);

                VkWriteDescriptorSet descriptorWrite{};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = m_descriptorSet;
                descriptorWrite.dstBinding = entry.binding;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pImageInfo = &imageInfos[imageInfoIndex++];

                descriptorWrites.push_back(descriptorWrite);
            } else if (entry.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || entry.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
                VkDescriptorImageInfo imageInfo{};
                imageInfo.sampler = VK_NULL_HANDLE;
                imageInfo.imageView = entry.imageView;
                imageInfo.imageLayout = entry.imageLayout;
                imageInfos.push_back(imageInfo);

                VkWriteDescriptorSet descriptorWrite{};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = m_descriptorSet;
                descriptorWrite.dstBinding = entry.binding;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = entry.descriptorType;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pImageInfo = &imageInfos[imageInfoIndex++];

                descriptorWrites.push_back(descriptorWrite);
            }
        }

        if (!descriptorWrites.empty()) {
            vkUpdateDescriptorSets(m_device->handle(), static_cast<uint32_t>(descriptorWrites.size()),
                descriptorWrites.data(), 0, nullptr);
        }
    }

    ~BindGroup()
    {
        if (m_pool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_device->handle(), m_pool, nullptr);
        }
    }

    VkDescriptorSet handle() const { return m_descriptorSet; }

private:
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
    Device* m_device = nullptr;
    VkDescriptorPool m_pool = VK_NULL_HANDLE;
};

class RenderPipeline {
public:
    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    RenderPipeline(Device* device, const RenderPipelineCreateInfo& createInfo)
        : m_device(device)
    {
        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(createInfo.bindGroupLayouts.size());
        pipelineLayoutInfo.pSetLayouts = createInfo.bindGroupLayouts.data();

        VkResult result = vkCreatePipelineLayout(m_device->handle(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout");
        }

        // Shader stages
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = createInfo.vertex.module;
        vertShaderStageInfo.pName = createInfo.vertex.entryPoint;

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        uint32_t stageCount = 1;
        if (createInfo.fragment.module != VK_NULL_HANDLE) {
            fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            fragShaderStageInfo.module = createInfo.fragment.module;
            fragShaderStageInfo.pName = createInfo.fragment.entryPoint;
            stageCount = 2;
        }

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        // Process vertex input
        std::vector<VkVertexInputBindingDescription> bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;

        for (size_t i = 0; i < createInfo.vertex.buffers.size(); ++i) {
            const auto& bufferLayout = createInfo.vertex.buffers[i];

            VkVertexInputBindingDescription binding{};
            binding.binding = static_cast<uint32_t>(i);
            binding.stride = static_cast<uint32_t>(bufferLayout.arrayStride);
            binding.inputRate = bufferLayout.stepModeInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
            bindings.push_back(binding);

            attributes.insert(attributes.end(), bufferLayout.attributes.begin(), bufferLayout.attributes.end());
        }

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
        vertexInputInfo.pVertexBindingDescriptions = bindings.data();
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = createInfo.primitive.topology;

        // Viewport
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = 800.0f; // Placeholder, dynamic state will be used
        viewport.height = 600.0f; // Placeholder, dynamic state will be used
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissorRect{};
        scissorRect.offset = { 0, 0 };
        scissorRect.extent = { 800, 600 }; // Placeholder, dynamic state will be used

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.pViewports = &viewport;
        viewportState.pScissors = &scissorRect;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = createInfo.primitive.polygonMode;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = createInfo.primitive.cullMode;
        rasterizer.frontFace = createInfo.primitive.frontFace;

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = createInfo.sampleCount;

        // Color blending
        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
        if (!createInfo.fragment.targets.empty()) {
            for (const auto& target : createInfo.fragment.targets) {
                colorBlendAttachments.push_back(target.blendState);
            }
        } else {
            VkPipelineColorBlendAttachmentState blendAttachment{};
            blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            blendAttachment.blendEnable = VK_FALSE;
            colorBlendAttachments.push_back(blendAttachment);
        }

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
        colorBlending.pAttachments = colorBlendAttachments.data();

        // Dynamic state
        VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        // Create depth stencil state if provided
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        if (createInfo.depthStencil.has_value()) {
            depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = createInfo.depthStencil->depthWriteEnabled ? VK_TRUE : VK_FALSE;
            depthStencil.depthCompareOp = createInfo.depthStencil->depthCompareOp;
            depthStencil.depthBoundsTestEnable = VK_FALSE;
            depthStencil.stencilTestEnable = VK_FALSE;
        }

        // Create graphics pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = stageCount;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        if (createInfo.depthStencil.has_value()) {
            pipelineInfo.pDepthStencilState = &depthStencil;
        }
        pipelineInfo.layout = m_pipelineLayout;
        pipelineInfo.renderPass = createInfo.renderPass;
        pipelineInfo.subpass = 0;

        result = vkCreateGraphicsPipelines(m_device->handle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);

        if (result != VK_SUCCESS) {
            vkDestroyPipelineLayout(m_device->handle(), m_pipelineLayout, nullptr);
            throw std::runtime_error("Failed to create graphics pipeline");
        }
    }

    ~RenderPipeline()
    {
        if (m_pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(m_device->handle(), m_pipeline, nullptr);
        }
        if (m_pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_device->handle(), m_pipelineLayout, nullptr);
        }
    }

    VkPipeline handle() const { return m_pipeline; }
    VkPipelineLayout layout() const { return m_pipelineLayout; }

private:
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    Device* m_device = nullptr;
};

class ComputePipeline {
public:
    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;

    ComputePipeline(Device* device, const ComputePipelineCreateInfo& createInfo)
        : m_device(device)
    {
        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(createInfo.bindGroupLayouts.size());
        pipelineLayoutInfo.pSetLayouts = createInfo.bindGroupLayouts.data();

        VkResult result = vkCreatePipelineLayout(m_device->handle(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create compute pipeline layout");
        }

        // Shader stage
        VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
        computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        computeShaderStageInfo.module = createInfo.module;
        computeShaderStageInfo.pName = createInfo.entryPoint;

        // Create compute pipeline
        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage = computeShaderStageInfo;
        pipelineInfo.layout = m_pipelineLayout;

        result = vkCreateComputePipelines(m_device->handle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
        if (result != VK_SUCCESS) {
            vkDestroyPipelineLayout(m_device->handle(), m_pipelineLayout, nullptr);
            throw std::runtime_error("Failed to create compute pipeline");
        }
    }

    ~ComputePipeline()
    {
        if (m_pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(m_device->handle(), m_pipeline, nullptr);
        }
        if (m_pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_device->handle(), m_pipelineLayout, nullptr);
        }
    }

    VkPipeline handle() const { return m_pipeline; }
    VkPipelineLayout layout() const { return m_pipelineLayout; }

private:
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    Device* m_device = nullptr;
};

class RenderPass {
public:
    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;

    RenderPass(Device* device, const RenderPassCreateInfo& createInfo);
    ~RenderPass();

    VkRenderPass handle() const { return m_renderPass; }
    uint32_t colorAttachmentCount() const { return m_colorAttachmentCount; }
    bool hasDepthStencil() const { return m_hasDepthStencil; }
    const std::vector<bool>& colorHasResolve() const { return m_colorHasResolve; }

private:
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    Device* m_device = nullptr;
    uint32_t m_colorAttachmentCount = 0;
    bool m_hasDepthStencil = false;
    std::vector<bool> m_colorHasResolve; // Track which color attachments have resolve targets
};

class Framebuffer {
public:
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    Framebuffer(Device* device, const FramebufferCreateInfo& createInfo)
        : m_device(device)
        , m_width(createInfo.width)
        , m_height(createInfo.height)
    {
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = createInfo.renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(createInfo.attachments.size());
        framebufferInfo.pAttachments = createInfo.attachments.data();
        framebufferInfo.width = createInfo.width;
        framebufferInfo.height = createInfo.height;
        framebufferInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(m_device->handle(), &framebufferInfo, nullptr, &m_framebuffer);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer");
        }
    }

    ~Framebuffer()
    {
        if (m_framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(m_device->handle(), m_framebuffer, nullptr);
        }
    }

    VkFramebuffer handle() const { return m_framebuffer; }
    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }

private:
    VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
    Device* m_device = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

class Fence {
public:
    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;

    Fence(Device* device, const FenceCreateInfo& createInfo)
        : m_device(device)
    {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        // If signaled = true, create fence in signaled state
        if (createInfo.signaled) {
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        }

        VkResult result = vkCreateFence(m_device->handle(), &fenceInfo, nullptr, &m_fence);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create fence");
        }
    }

    ~Fence()
    {
        if (m_fence != VK_NULL_HANDLE) {
            vkDestroyFence(m_device->handle(), m_fence, nullptr);
        }
    }

    VkFence handle() const { return m_fence; }

    VkResult getStatus(bool* isSignaled) const
    {
        if (!isSignaled) {
            return VK_ERROR_UNKNOWN;
        }

        VkResult result = vkGetFenceStatus(m_device->handle(), m_fence);
        if (result == VK_SUCCESS) {
            *isSignaled = true;
            return VK_SUCCESS;
        } else if (result == VK_NOT_READY) {
            *isSignaled = false;
            return VK_SUCCESS;
        } else {
            return result;
        }
    }

    VkResult wait(uint64_t timeoutNs)
    {
        return vkWaitForFences(m_device->handle(), 1, &m_fence, VK_TRUE, timeoutNs);
    }

    void reset()
    {
        vkResetFences(m_device->handle(), 1, &m_fence);
    }

private:
    VkFence m_fence = VK_NULL_HANDLE;
    Device* m_device = nullptr;
};

class Semaphore {
public:
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

    Semaphore(Device* device, const SemaphoreCreateInfo& createInfo)
        : m_device(device)
        , m_type(createInfo.type)
    {

        if (m_type == SemaphoreType::Timeline) {
            // Timeline semaphore
            VkSemaphoreTypeCreateInfo timelineInfo{};
            timelineInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            timelineInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            timelineInfo.initialValue = createInfo.initialValue;

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            semaphoreInfo.pNext = &timelineInfo;

            VkResult result = vkCreateSemaphore(m_device->handle(), &semaphoreInfo, nullptr, &m_semaphore);
            if (result != VK_SUCCESS) {
                throw std::runtime_error("Failed to create timeline semaphore");
            }
        } else {
            // Binary semaphore
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkResult result = vkCreateSemaphore(m_device->handle(), &semaphoreInfo, nullptr, &m_semaphore);
            if (result != VK_SUCCESS) {
                throw std::runtime_error("Failed to create binary semaphore");
            }
        }
    }

    ~Semaphore()
    {
        if (m_semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device->handle(), m_semaphore, nullptr);
        }
    }

    VkSemaphore handle() const { return m_semaphore; }
    SemaphoreType getType() const { return m_type; }

    VkResult signal(uint64_t value)
    {
        if (m_type != SemaphoreType::Timeline) {
            return VK_ERROR_VALIDATION_FAILED_EXT; // Binary semaphores can't be manually signaled
        }

        VkSemaphoreSignalInfo signalInfo{};
        signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
        signalInfo.semaphore = m_semaphore;
        signalInfo.value = value;

        return vkSignalSemaphore(m_device->handle(), &signalInfo);
    }

    VkResult wait(uint64_t value, uint64_t timeoutNs)
    {
        if (m_type != SemaphoreType::Timeline) {
            return VK_ERROR_VALIDATION_FAILED_EXT; // Binary semaphores can't be manually waited
        }

        VkSemaphoreWaitInfo waitInfo{};
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &m_semaphore;
        waitInfo.pValues = &value;

        return vkWaitSemaphores(m_device->handle(), &waitInfo, timeoutNs);
    }

    uint64_t getValue() const
    {
        if (m_type != SemaphoreType::Timeline) {
            return 0; // Binary semaphores don't have values
        }

        uint64_t value = 0;
        vkGetSemaphoreCounterValue(m_device->handle(), m_semaphore, &value);
        return value;
    }

private:
    VkSemaphore m_semaphore = VK_NULL_HANDLE;
    Device* m_device = nullptr;
    SemaphoreType m_type = SemaphoreType::Binary;
};

class CommandEncoder {
public:
    CommandEncoder(const CommandEncoder&) = delete;
    CommandEncoder& operator=(const CommandEncoder&) = delete;

    CommandEncoder(Device* device)
        : m_device(device)
    {
        // Create command pool
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = device->getQueue()->family();

        VkResult result = vkCreateCommandPool(m_device->handle(), &poolInfo, nullptr, &m_commandPool);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool");
        }

        // Allocate command buffer
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        result = vkAllocateCommandBuffers(m_device->handle(), &allocInfo, &m_commandBuffer);
        if (result != VK_SUCCESS) {
            vkDestroyCommandPool(m_device->handle(), m_commandPool, nullptr);
            throw std::runtime_error("Failed to allocate command buffer");
        }

        // Begin recording
        begin();
    }

    ~CommandEncoder()
    {
        if (m_commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(m_device->handle(), m_commandPool, nullptr);
        }
    }

    VkCommandBuffer handle() const { return m_commandBuffer; }
    VkDevice device() const { return m_device->handle(); }
    Device* getDevice() const { return m_device; }
    VkPipelineLayout currentPipelineLayout() const { return m_currentPipelineLayout; }

    void setCurrentPipelineLayout(VkPipelineLayout layout)
    {
        m_currentPipelineLayout = layout;
    }

    void begin()
    {
        if (!m_isRecording) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
            m_isRecording = true;
        }
    }

    void end()
    {
        if (m_isRecording) {
            vkEndCommandBuffer(m_commandBuffer);
            m_isRecording = false;
        }
    }

    void reset()
    {
        m_currentPipelineLayout = VK_NULL_HANDLE;

        // Reset the command pool (this implicitly resets all command buffers)
        vkResetCommandPool(m_device->handle(), m_commandPool, 0);

        // Mark as not recording since the command buffer was reset
        m_isRecording = false;

        // Begin recording again
        begin();
    }

    void pipelineBarrier(
        const MemoryBarrier* memoryBarriers, uint32_t memoryBarrierCount,
        const BufferBarrier* bufferBarriers, uint32_t bufferBarrierCount,
        const TextureBarrier* textureBarriers, uint32_t textureBarrierCount)
    {
        std::vector<VkMemoryBarrier> memBarriers;
        memBarriers.reserve(memoryBarrierCount);

        std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers;
        bufferMemoryBarriers.reserve(bufferBarrierCount);

        std::vector<VkImageMemoryBarrier> imageBarriers;
        imageBarriers.reserve(textureBarrierCount);

        // Combine pipeline stages from all barriers
        VkPipelineStageFlags srcStage = 0;
        VkPipelineStageFlags dstStage = 0;

        // Process memory barriers
        for (uint32_t i = 0; i < memoryBarrierCount; ++i) {
            const auto& barrier = memoryBarriers[i];

            VkMemoryBarrier vkBarrier{};
            vkBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            vkBarrier.srcAccessMask = barrier.srcAccessMask;
            vkBarrier.dstAccessMask = barrier.dstAccessMask;

            memBarriers.push_back(vkBarrier);

            srcStage |= barrier.srcStageMask;
            dstStage |= barrier.dstStageMask;
        }

        // Process buffer barriers
        for (uint32_t i = 0; i < bufferBarrierCount; ++i) {
            const auto& barrier = bufferBarriers[i];

            VkBufferMemoryBarrier vkBarrier{};
            vkBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            vkBarrier.buffer = barrier.buffer->handle();
            vkBarrier.offset = barrier.offset;
            vkBarrier.size = barrier.size == 0 ? VK_WHOLE_SIZE : barrier.size;
            vkBarrier.srcAccessMask = barrier.srcAccessMask;
            vkBarrier.dstAccessMask = barrier.dstAccessMask;
            vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            bufferMemoryBarriers.push_back(vkBarrier);

            srcStage |= barrier.srcStageMask;
            dstStage |= barrier.dstStageMask;
        }

        // Process texture barriers
        for (uint32_t i = 0; i < textureBarrierCount; ++i) {
            const auto& barrier = textureBarriers[i];

            VkImageMemoryBarrier vkBarrier{};
            vkBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            vkBarrier.image = barrier.texture->handle();
            vkBarrier.subresourceRange.aspectMask = converter::getImageAspectMask(barrier.texture->getFormat());
            vkBarrier.subresourceRange.baseMipLevel = barrier.baseMipLevel;
            vkBarrier.subresourceRange.levelCount = barrier.mipLevelCount;
            vkBarrier.subresourceRange.baseArrayLayer = barrier.baseArrayLayer;
            vkBarrier.subresourceRange.layerCount = barrier.arrayLayerCount;

            vkBarrier.oldLayout = barrier.oldLayout;
            vkBarrier.newLayout = barrier.newLayout;
            vkBarrier.srcAccessMask = barrier.srcAccessMask;
            vkBarrier.dstAccessMask = barrier.dstAccessMask;

            vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            imageBarriers.push_back(vkBarrier);

            srcStage |= barrier.srcStageMask;
            dstStage |= barrier.dstStageMask;

            // Update tracked layout
            barrier.texture->setLayout(barrier.newLayout);
        }

        vkCmdPipelineBarrier(
            m_commandBuffer,
            srcStage,
            dstStage,
            0,
            static_cast<uint32_t>(memBarriers.size()),
            memBarriers.empty() ? nullptr : memBarriers.data(),
            static_cast<uint32_t>(bufferMemoryBarriers.size()),
            bufferMemoryBarriers.empty() ? nullptr : bufferMemoryBarriers.data(),
            static_cast<uint32_t>(imageBarriers.size()),
            imageBarriers.empty() ? nullptr : imageBarriers.data());
    }

    void copyBufferToBuffer(Buffer* source, uint64_t sourceOffset,
        Buffer* destination, uint64_t destinationOffset,
        uint64_t size)
    {
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = sourceOffset;
        copyRegion.dstOffset = destinationOffset;
        copyRegion.size = size;

        vkCmdCopyBuffer(m_commandBuffer, source->handle(), destination->handle(), 1, &copyRegion);
    }

    void copyBufferToTexture(Buffer* source, uint64_t sourceOffset,
        Texture* destination, VkOffset3D origin,
        VkExtent3D extent, uint32_t mipLevel, VkImageLayout finalLayout)
    {
        // Transition image layout to transfer dst optimal
        destination->transitionLayout(this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevel, 1, 0, 1);

        // Copy buffer to image
        VkBufferImageCopy region{};
        region.bufferOffset = sourceOffset;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = converter::getImageAspectMask(destination->getFormat());
        region.imageSubresource.mipLevel = mipLevel;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = origin;
        region.imageExtent = extent;

        vkCmdCopyBufferToImage(m_commandBuffer, source->handle(), destination->handle(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        // Transition image layout to final layout
        destination->transitionLayout(this, finalLayout, mipLevel, 1, 0, 1);
    }

    void copyTextureToBuffer(Texture* source, VkOffset3D origin, uint32_t mipLevel,
        Buffer* destination, uint64_t destinationOffset,
        VkExtent3D extent, VkImageLayout finalLayout)
    {
        // Transition image layout to transfer src optimal
        source->transitionLayout(this, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mipLevel, 1, 0, 1);

        // Copy image to buffer
        VkBufferImageCopy region{};
        region.bufferOffset = destinationOffset;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = converter::getImageAspectMask(source->getFormat());
        region.imageSubresource.mipLevel = mipLevel;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = origin;
        region.imageExtent = extent;

        vkCmdCopyImageToBuffer(m_commandBuffer, source->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            destination->handle(), 1, &region);

        // Transition image layout to final layout
        source->transitionLayout(this, finalLayout, mipLevel, 1, 0, 1);
    }

    void copyTextureToTexture(Texture* source, VkOffset3D sourceOrigin, uint32_t sourceMipLevel,
        Texture* destination, VkOffset3D destinationOrigin, uint32_t destinationMipLevel,
        VkExtent3D extent, VkImageLayout srcFinalLayout, VkImageLayout dstFinalLayout)
    {
        // For 2D textures and arrays, extent.depth represents layer count
        // For 3D textures, it represents actual depth
        uint32_t layerCount = extent.depth;
        uint32_t copyDepth = extent.depth;

        VkExtent3D srcSize = source->getSize();
        bool is3DTexture = (srcSize.depth > 1);

        if (!is3DTexture) {
            copyDepth = 1;
        } else {
            layerCount = 1;
        }

        // Transition images to transfer layouts
        source->transitionLayout(this, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, sourceMipLevel, 1, sourceOrigin.z, layerCount);
        destination->transitionLayout(this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, destinationMipLevel, 1, destinationOrigin.z, layerCount);

        // Copy image to image
        VkImageCopy region{};
        region.srcSubresource.aspectMask = converter::getImageAspectMask(source->getFormat());
        region.srcSubresource.mipLevel = sourceMipLevel;
        region.srcSubresource.baseArrayLayer = is3DTexture ? 0 : sourceOrigin.z;
        region.srcSubresource.layerCount = layerCount;
        region.srcOffset = { sourceOrigin.x, sourceOrigin.y, is3DTexture ? sourceOrigin.z : 0 };
        region.dstSubresource.aspectMask = converter::getImageAspectMask(destination->getFormat());
        region.dstSubresource.mipLevel = destinationMipLevel;
        region.dstSubresource.baseArrayLayer = is3DTexture ? 0 : destinationOrigin.z;
        region.dstSubresource.layerCount = layerCount;
        region.dstOffset = { destinationOrigin.x, destinationOrigin.y, is3DTexture ? destinationOrigin.z : 0 };
        region.extent = { extent.width, extent.height, copyDepth };

        vkCmdCopyImage(m_commandBuffer, source->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            destination->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        // Transition images to final layouts
        source->transitionLayout(this, srcFinalLayout, sourceMipLevel, 1, sourceOrigin.z, layerCount);
        destination->transitionLayout(this, dstFinalLayout, destinationMipLevel, 1, destinationOrigin.z, layerCount);
    }

    void blitTextureToTexture(Texture* source, VkOffset3D sourceOrigin, VkExtent3D sourceExtent, uint32_t sourceMipLevel,
        Texture* destination, VkOffset3D destinationOrigin, VkExtent3D destinationExtent, uint32_t destinationMipLevel,
        VkFilter filter, VkImageLayout srcFinalLayout, VkImageLayout dstFinalLayout)
    {
        // For 2D textures and arrays, extent.depth represents layer count
        // For 3D textures, it represents actual depth
        uint32_t layerCount = sourceExtent.depth;
        uint32_t srcDepth = sourceExtent.depth;
        uint32_t dstDepth = destinationExtent.depth;

        VkExtent3D srcSize = source->getSize();
        bool is3DTexture = (srcSize.depth > 1);

        if (!is3DTexture) {
            srcDepth = 1;
            dstDepth = 1;
        } else {
            layerCount = 1;
        }

        // Transition images to transfer layouts
        source->transitionLayout(this, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, sourceMipLevel, 1, sourceOrigin.z, layerCount);
        destination->transitionLayout(this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, destinationMipLevel, 1, destinationOrigin.z, layerCount);

        // Blit image to image with scaling and filtering
        VkImageBlit region{};
        region.srcSubresource.aspectMask = converter::getImageAspectMask(source->getFormat());
        region.srcSubresource.mipLevel = sourceMipLevel;
        region.srcSubresource.baseArrayLayer = is3DTexture ? 0 : sourceOrigin.z;
        region.srcSubresource.layerCount = layerCount;
        region.srcOffsets[0] = { sourceOrigin.x, sourceOrigin.y, is3DTexture ? sourceOrigin.z : 0 };
        region.srcOffsets[1] = { static_cast<int32_t>(sourceOrigin.x + sourceExtent.width),
            static_cast<int32_t>(sourceOrigin.y + sourceExtent.height),
            is3DTexture ? static_cast<int32_t>(sourceOrigin.z + srcDepth) : 1 };
        region.dstSubresource.aspectMask = converter::getImageAspectMask(destination->getFormat());
        region.dstSubresource.mipLevel = destinationMipLevel;
        region.dstSubresource.baseArrayLayer = is3DTexture ? 0 : destinationOrigin.z;
        region.dstSubresource.layerCount = layerCount;
        region.dstOffsets[0] = { destinationOrigin.x, destinationOrigin.y, is3DTexture ? destinationOrigin.z : 0 };
        region.dstOffsets[1] = { static_cast<int32_t>(destinationOrigin.x + destinationExtent.width),
            static_cast<int32_t>(destinationOrigin.y + destinationExtent.height),
            is3DTexture ? static_cast<int32_t>(destinationOrigin.z + dstDepth) : 1 };

        vkCmdBlitImage(m_commandBuffer, source->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            destination->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, filter);

        // Transition images to final layouts
        source->transitionLayout(this, srcFinalLayout, sourceMipLevel, 1, sourceOrigin.z, layerCount);
        destination->transitionLayout(this, dstFinalLayout, destinationMipLevel, 1, destinationOrigin.z, layerCount);
    }

private:
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    Device* m_device = nullptr;
    bool m_isRecording = false;
    VkPipelineLayout m_currentPipelineLayout = VK_NULL_HANDLE;
};

class RenderPassEncoder {
public:
    RenderPassEncoder(const RenderPassEncoder&) = delete;
    RenderPassEncoder& operator=(const RenderPassEncoder&) = delete;

    RenderPassEncoder(CommandEncoder* commandEncoder, RenderPass* renderPass, Framebuffer* framebuffer, const RenderPassEncoderBeginInfo& beginInfo);

    ~RenderPassEncoder()
    {
        vkCmdEndRenderPass(m_commandBuffer);
    }

    VkCommandBuffer handle() const { return m_commandBuffer; }
    Device* device() const { return m_device; }
    CommandEncoder* commandEncoder() const { return m_commandEncoder; }

    void setPipeline(RenderPipeline* pipeline)
    {
        vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->handle());
        m_commandEncoder->setCurrentPipelineLayout(pipeline->layout());
    }

    void setBindGroup(uint32_t index, BindGroup* bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount)
    {
        VkPipelineLayout layout = m_commandEncoder->currentPipelineLayout();
        if (layout != VK_NULL_HANDLE) {
            VkDescriptorSet set = bindGroup->handle();
            vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, index, 1, &set, dynamicOffsetCount, dynamicOffsets);
        }
    }

    void setVertexBuffer(uint32_t slot, Buffer* buffer, uint64_t offset)
    {
        VkBuffer vkBuf = buffer->handle();
        VkDeviceSize offsets[] = { offset };
        vkCmdBindVertexBuffers(m_commandBuffer, slot, 1, &vkBuf, offsets);
    }

    void setIndexBuffer(Buffer* buffer, VkIndexType indexType, uint64_t offset)
    {
        vkCmdBindIndexBuffer(m_commandBuffer, buffer->handle(), offset, indexType);
    }

    void setViewport(const Viewport& viewport)
    {
        VkViewport vkViewport{};
        vkViewport.x = viewport.x;
        vkViewport.y = viewport.y;
        vkViewport.width = viewport.width;
        vkViewport.height = viewport.height;
        vkViewport.minDepth = viewport.minDepth;
        vkViewport.maxDepth = viewport.maxDepth;
        vkCmdSetViewport(m_commandBuffer, 0, 1, &vkViewport);
    }

    void setScissorRect(const ScissorRect& scissor)
    {
        VkRect2D vkScissor{};
        vkScissor.offset = { scissor.x, scissor.y };
        vkScissor.extent = { scissor.width, scissor.height };
        vkCmdSetScissor(m_commandBuffer, 0, 1, &vkScissor);
    }

    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        vkCmdDraw(m_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance)
    {
        vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
    }

private:
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    Device* m_device = nullptr;
    CommandEncoder* m_commandEncoder = nullptr;
};

class ComputePassEncoder {
public:
    ComputePassEncoder(const ComputePassEncoder&) = delete;
    ComputePassEncoder& operator=(const ComputePassEncoder&) = delete;

    ComputePassEncoder(CommandEncoder* commandEncoder, const ComputePassEncoderCreateInfo& createInfo);

    ~ComputePassEncoder()
    {
        // Compute pass has no special cleanup
    }

    VkCommandBuffer handle() const { return m_commandBuffer; }
    Device* device() const { return m_device; }
    CommandEncoder* commandEncoder() const { return m_commandEncoder; }

    void setPipeline(ComputePipeline* pipeline)
    {
        vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->handle());
        m_commandEncoder->setCurrentPipelineLayout(pipeline->layout());
    }

    void setBindGroup(uint32_t index, BindGroup* bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount)
    {
        VkPipelineLayout layout = m_commandEncoder->currentPipelineLayout();
        if (layout != VK_NULL_HANDLE) {
            VkDescriptorSet set = bindGroup->handle();
            vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, index, 1, &set, dynamicOffsetCount, dynamicOffsets);
        }
    }

    void dispatchWorkgroups(uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ)
    {
        vkCmdDispatch(m_commandBuffer, workgroupCountX, workgroupCountY, workgroupCountZ);
    }

private:
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    Device* m_device = nullptr;
    CommandEncoder* m_commandEncoder = nullptr;
};

} // namespace gfx::backend::vulkan

#endif // GFX_SRC_BACKEND_VULKAN_CORE_ENTITIES_H