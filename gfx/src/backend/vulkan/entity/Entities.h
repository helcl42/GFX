#pragma once

#include "../common/VulkanCommon.h"
#include "../converter/GfxVulkanConverter.h"
#include "CreateInfo.h" // Internal CreateInfo structs

#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace gfx::vulkan {

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
class Fence;
class Semaphore;

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

    Instance(const InstanceCreateInfo& createInfo)
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "GfxWrapper Application";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "GfxWrapper";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_1;

        VkInstanceCreateInfo vkCreateInfo{};
        vkCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        vkCreateInfo.pApplicationInfo = &appInfo;

        // Extensions
        std::vector<const char*> extensions = {};
        if (!createInfo.enableHeadless) {
            extensions.push_back("VK_KHR_surface");
#ifdef _WIN32
            extensions.push_back("VK_KHR_win32_surface");
#elif defined(__linux__)
            extensions.push_back("VK_KHR_xlib_surface");
            extensions.push_back("VK_KHR_xcb_surface");
            extensions.push_back("VK_KHR_wayland_surface");
#elif defined(__APPLE__)
            extensions.push_back("VK_MVK_macos_surface");
#endif
        }

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
            throw std::runtime_error("Failed to create Vulkan instance: " + std::string(gfx::convertor::vkResultToString(result)));
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
            DebugMessageSeverity severity = gfx::convertor::convertVkDebugSeverity(messageSeverity);
            DebugMessageType type = gfx::convertor::convertVkDebugType(messageType);

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
    const char* getName() const { return m_properties.deviceName; }
    uint32_t getGraphicsQueueFamily() const { return m_graphicsQueueFamily; }
    Instance* getInstance() const { return m_instance; }

private:
    void initializeAdapterInfo()
    {
        vkGetPhysicalDeviceProperties(m_physicalDevice, &m_properties);

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
    uint32_t m_graphicsQueueFamily = UINT32_MAX;
};

class Queue {
public:
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    Queue(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t queueFamily)
        : m_physicalDevice(physicalDevice)
        , m_queueFamily(queueFamily)
    {
        vkGetDeviceQueue(device, queueFamily, 0, &m_queue);
    }

    VkQueue handle() const { return m_queue; }
    VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
    uint32_t family() const { return m_queueFamily; }

private:
    VkQueue m_queue = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    uint32_t m_queueFamily = 0;
};

class Device {
public:
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    Device(Adapter* adapter, const DeviceCreateInfo& createInfo)
        : m_adapter(adapter)
    {
        (void)createInfo; // Currently unused

        // Queue create info
        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = m_adapter->getGraphicsQueueFamily();
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        // Device features
        VkPhysicalDeviceFeatures deviceFeatures{};

        // Device extensions
        std::vector<const char*> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

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

        m_queue = std::make_unique<Queue>(m_device, m_adapter->handle(), m_adapter->getGraphicsQueueFamily());
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

private:
    VkDevice m_device = VK_NULL_HANDLE;
    Adapter* m_adapter = nullptr; // Non-owning pointer
    std::unique_ptr<Queue> m_queue;
};

class Shader {
public:
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(VkDevice device, const ShaderCreateInfo& createInfo)
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

        VkResult result = vkCreateShaderModule(m_device, &vkCreateInfo, nullptr, &m_shaderModule);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module");
        }
    }

    ~Shader()
    {
        if (m_shaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(m_device, m_shaderModule, nullptr);
        }
    }

    VkShaderModule handle() const { return m_shaderModule; }
    const char* entryPoint() const { return m_entryPoint.c_str(); }

private:
    VkShaderModule m_shaderModule = VK_NULL_HANDLE;
    std::string m_entryPoint;
    VkDevice m_device = VK_NULL_HANDLE;
};

class BindGroupLayout {
public:
    BindGroupLayout(const BindGroupLayout&) = delete;
    BindGroupLayout& operator=(const BindGroupLayout&) = delete;

    BindGroupLayout(VkDevice device, const BindGroupLayoutCreateInfo& createInfo)
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

        VkResult result = vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_layout);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout");
        }
    }

    ~BindGroupLayout()
    {
        if (m_layout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_device, m_layout, nullptr);
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
    VkDevice m_device = VK_NULL_HANDLE;
    std::unordered_map<uint32_t, VkDescriptorType> m_bindingTypes;
};

class Surface {
public:
    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;

    Surface(VkInstance instance, VkPhysicalDevice physicalDevice, const SurfaceCreateInfo& createInfo)
        : m_instance(instance)
        , m_physicalDevice(physicalDevice)
    {
        switch (createInfo.platform) {
        case SurfaceCreateInfo::Platform::Xlib:
            if (createInfo.handle.xlib.display) {
                VkXlibSurfaceCreateInfoKHR vkCreateInfo{};
                vkCreateInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
                vkCreateInfo.dpy = static_cast<Display*>(createInfo.handle.xlib.display);
                vkCreateInfo.window = static_cast<Window>(createInfo.handle.xlib.window);

                VkResult result = vkCreateXlibSurfaceKHR(m_instance, &vkCreateInfo, nullptr, &m_surface);
                if (result != VK_SUCCESS) {
                    throw std::runtime_error("Failed to create Xlib surface");
                }
            }
            break;
        // Other platforms can be added here
        default:
            throw std::runtime_error("Unsupported windowing platform");
        }
    }

    ~Surface()
    {
        if (m_surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        }
    }

    VkInstance instance() const { return m_instance; }
    VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
    VkSurfaceKHR handle() const { return m_surface; }

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
};

class Swapchain {
public:
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    Swapchain(VkDevice device, VkPhysicalDevice physicalDevice, const SwapchainCreateInfo& createInfo)
        : m_device(device)
        , m_physicalDevice(physicalDevice)
        , m_surface(createInfo.surface)
        , m_width(createInfo.width)
        , m_height(createInfo.height)
        , m_format(createInfo.format)
        , m_presentMode(createInfo.presentMode)
    {
        // Check if queue family supports presentation
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, createInfo.queueFamily, createInfo.surface, &presentSupport);
        if (presentSupport != VK_TRUE) {
            throw std::runtime_error("Selected queue family does not support presentation");
        }

        // Query surface capabilities
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, createInfo.surface, &capabilities);

        // Choose format - already converted in createInfo
        // No format conversion needed

        // Choose present mode - already converted in createInfo
        // No present mode conversion needed

        // Determine actual swapchain extent
        // If currentExtent is defined, we MUST use it. Otherwise, we can choose within min/max bounds.
        VkExtent2D actualExtent;
        if (capabilities.currentExtent.width != UINT32_MAX) {
            // Window manager is telling us the size - we must use it
            actualExtent = capabilities.currentExtent;
            m_width = actualExtent.width;
            m_height = actualExtent.height;
        } else {
            // We can choose the extent within bounds
            actualExtent.width = std::max(capabilities.minImageExtent.width,
                std::min(m_width, capabilities.maxImageExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height,
                std::min(m_height, capabilities.maxImageExtent.height));
            m_width = actualExtent.width;
            m_height = actualExtent.height;
        }

        // Create swapchain
        VkSwapchainCreateInfoKHR vkCreateInfo{};
        vkCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        vkCreateInfo.surface = createInfo.surface;
        vkCreateInfo.minImageCount = std::min(3u, capabilities.minImageCount + 1);
        if (capabilities.maxImageCount > 0) {
            vkCreateInfo.minImageCount = std::min(vkCreateInfo.minImageCount, capabilities.maxImageCount);
        }
        vkCreateInfo.imageFormat = m_format;
        vkCreateInfo.imageColorSpace = createInfo.colorSpace;
        vkCreateInfo.imageExtent = actualExtent;
        vkCreateInfo.imageArrayLayers = 1;
        vkCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        vkCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateInfo.preTransform = capabilities.currentTransform;
        vkCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        vkCreateInfo.presentMode = m_presentMode;
        vkCreateInfo.clipped = VK_TRUE;

        VkResult result = vkCreateSwapchainKHR(m_device, &vkCreateInfo, nullptr, &m_swapchain);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain");
        }

        // Get swapchain images
        uint32_t imageCount;
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
        m_images.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_images.data());

        m_textures.reserve(imageCount);
        m_textureViews.reserve(imageCount);
        for (size_t i = 0; i < imageCount; ++i) {
            // Create non-owning Texture wrapper for swapchain image
            gfx::vulkan::TextureCreateInfo textureCreateInfo{};
            textureCreateInfo.format = m_format;
            textureCreateInfo.size = { m_width, m_height, 1 };
            textureCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            textureCreateInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
            textureCreateInfo.mipLevelCount = 1;
            textureCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            textureCreateInfo.arrayLayers = 1;
            textureCreateInfo.flags = 0;
            m_textures.push_back(std::make_unique<Texture>(m_device, m_images[i], textureCreateInfo));

            // Create TextureView for the texture
            gfx::vulkan::TextureViewCreateInfo viewCreateInfo{};
            viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewCreateInfo.format = VK_FORMAT_UNDEFINED; // Use texture's format
            viewCreateInfo.baseMipLevel = 0;
            viewCreateInfo.mipLevelCount = 1;
            viewCreateInfo.baseArrayLayer = 0;
            viewCreateInfo.arrayLayerCount = 1;
            m_textureViews.push_back(std::make_unique<TextureView>(m_textures[i].get(), viewCreateInfo));
        }

        // Get present queue (assume queue family 0)
        vkGetDeviceQueue(m_device, 0, 0, &m_presentQueue);

        // Don't pre-acquire an image - let explicit acquire handle it
        m_currentImageIndex = 0;
    }

    ~Swapchain()
    {
        // Texture and TextureView objects will be automatically destroyed by unique_ptr
        if (m_swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        }
    }

    VkSwapchainKHR handle() const { return m_swapchain; }
    uint32_t getImageCount() const { return static_cast<uint32_t>(m_images.size()); }
    Texture* getTexture(uint32_t index) const { return m_textures[index].get(); }
    Texture* getCurrentTexture() const { return m_textures[m_currentImageIndex].get(); }
    TextureView* getTextureView(uint32_t index) const { return m_textureViews[index].get(); }
    TextureView* getCurrentTextureView() const { return m_textureViews[m_currentImageIndex].get(); }
    VkFormat getFormat() const { return m_format; }
    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }
    uint32_t getCurrentImageIndex() const { return m_currentImageIndex; }
    VkPresentModeKHR getPresentMode() const { return m_presentMode; }

    VkResult acquireNextImage(uint64_t timeoutNs, VkSemaphore semaphore, VkFence fence, uint32_t* outImageIndex)
    {
        VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, timeoutNs, semaphore, fence, outImageIndex);
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
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    std::vector<VkImage> m_images;
    std::vector<std::unique_ptr<Texture>> m_textures;
    std::vector<std::unique_ptr<TextureView>> m_textureViews;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    uint32_t m_currentImageIndex = 0;
    VkPresentModeKHR m_presentMode = VK_PRESENT_MODE_FIFO_KHR;
};

class Buffer {
public:
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(VkDevice device, VkPhysicalDevice physicalDevice, const BufferCreateInfo& createInfo)
        : m_device(device)
        , m_size(createInfo.size)
        , m_usage(createInfo.usage)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = m_size;
        bufferInfo.usage = m_usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_buffer);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create buffer");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_device, m_buffer, &memRequirements);

        // Find memory type
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        uint32_t memoryTypeIndex = UINT32_MAX;
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
            if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                memoryTypeIndex = i;
                break;
            }
        }

        if (memoryTypeIndex == UINT32_MAX) {
            vkDestroyBuffer(m_device, m_buffer, nullptr);
            throw std::runtime_error("Failed to find suitable memory type");
        }

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = memoryTypeIndex;

        result = vkAllocateMemory(m_device, &allocInfo, nullptr, &m_memory);
        if (result != VK_SUCCESS) {
            vkDestroyBuffer(m_device, m_buffer, nullptr);
            throw std::runtime_error("Failed to allocate buffer memory");
        }

        vkBindBufferMemory(m_device, m_buffer, m_memory, 0);
    }

    ~Buffer()
    {
        if (m_memory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, m_memory, nullptr);
        }
        if (m_buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, m_buffer, nullptr);
        }
    }

    VkBuffer handle() const { return m_buffer; }

    void* map()
    {
        void* data;
        vkMapMemory(m_device, m_memory, 0, m_size, 0, &data);
        return data;
    }

    void unmap()
    {
        vkUnmapMemory(m_device, m_memory);
    }

    size_t size() const { return m_size; }
    VkBufferUsageFlags getUsage() const { return m_usage; }

private:
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    size_t m_size = 0;
    VkBufferUsageFlags m_usage = 0;
};

class Texture {
public:
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Owning constructor - creates and manages VkImage and memory
    Texture(VkDevice device, VkPhysicalDevice physicalDevice, const TextureCreateInfo& createInfo)
        : m_device(device)
        , m_ownsResources(true)
        , m_size(createInfo.size)
        , m_format(createInfo.format)
        , m_mipLevelCount(createInfo.mipLevelCount)
        , m_sampleCount(createInfo.sampleCount)
        , m_usage(createInfo.usage)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = createInfo.imageType;
        imageInfo.extent = m_size;
        imageInfo.mipLevels = createInfo.mipLevelCount;
        imageInfo.arrayLayers = createInfo.arrayLayers;
        imageInfo.flags = createInfo.flags;
        imageInfo.format = m_format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Always create in UNDEFINED, transition explicitly
        imageInfo.usage = m_usage;

        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = m_sampleCount;

        VkResult result = vkCreateImage(m_device, &imageInfo, nullptr, &m_image);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_device, m_image, &memRequirements);

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

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

        result = vkAllocateMemory(m_device, &allocInfo, nullptr, &m_memory);
        if (result != VK_SUCCESS) {
            vkDestroyImage(m_device, m_image, nullptr);
            throw std::runtime_error("Failed to allocate image memory");
        }

        vkBindImageMemory(m_device, m_image, m_memory, 0);
    }

    // Non-owning constructor - wraps an existing VkImage (e.g., from swapchain)
    Texture(VkDevice device, VkImage image, const TextureCreateInfo& createInfo)
        : m_device(device)
        , m_ownsResources(false)
        , m_size(createInfo.size)
        , m_format(createInfo.format)
        , m_mipLevelCount(createInfo.mipLevelCount)
        , m_sampleCount(createInfo.sampleCount)
        , m_usage(createInfo.usage)
        , m_image(image)
    {
    }

    ~Texture()
    {
        if (m_ownsResources) {
            if (m_memory != VK_NULL_HANDLE) {
                vkFreeMemory(m_device, m_memory, nullptr);
            }
            if (m_image != VK_NULL_HANDLE) {
                vkDestroyImage(m_device, m_image, nullptr);
            }
        }
    }

    VkImage handle() const { return m_image; }
    VkDevice device() const { return m_device; }
    VkExtent3D getSize() const { return m_size; }
    VkFormat getFormat() const { return m_format; }
    uint32_t getMipLevelCount() const { return m_mipLevelCount; }
    VkSampleCountFlagBits getSampleCount() const { return m_sampleCount; }
    VkImageUsageFlags getUsage() const { return m_usage; }
    VkImageLayout getLayout() const { return m_currentLayout; }
    void setLayout(VkImageLayout layout) { m_currentLayout = layout; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    bool m_ownsResources = true;
    VkExtent3D m_size;
    VkFormat m_format;
    uint32_t m_mipLevelCount;
    VkSampleCountFlagBits m_sampleCount;
    VkImageUsageFlags m_usage;
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

        // Set aspect mask based on format
        if (gfx::convertor::isDepthFormat(viewInfo.format)) {
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (gfx::convertor::hasStencilComponent(viewInfo.format)) {
                viewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        } else {
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

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

    Sampler(VkDevice device, const SamplerCreateInfo& createInfo)
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

        VkResult result = vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create sampler");
        }
    }

    ~Sampler()
    {
        if (m_sampler != VK_NULL_HANDLE) {
            vkDestroySampler(m_device, m_sampler, nullptr);
        }
    }

    VkSampler handle() const { return m_sampler; }

private:
    VkSampler m_sampler = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
};

class BindGroup {
public:
    BindGroup(const BindGroup&) = delete;
    BindGroup& operator=(const BindGroup&) = delete;

    BindGroup(VkDevice device, const BindGroupCreateInfo& createInfo)
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

        VkResult result = vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_pool);
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

        result = vkAllocateDescriptorSets(m_device, &allocInfo, &m_descriptorSet);
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
            vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()),
                descriptorWrites.data(), 0, nullptr);
        }
    }

    ~BindGroup()
    {
        if (m_pool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_device, m_pool, nullptr);
        }
    }

    VkDescriptorSet handle() const { return m_descriptorSet; }

private:
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkDescriptorPool m_pool = VK_NULL_HANDLE;
};

class RenderPipeline {
public:
    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    RenderPipeline(VkDevice device, const RenderPipelineCreateInfo& createInfo)
        : m_device(device)
    {
        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(createInfo.bindGroupLayouts.size());
        pipelineLayoutInfo.pSetLayouts = createInfo.bindGroupLayouts.data();

        VkResult result = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
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

        // Create a temporary render pass for pipeline creation
        std::vector<VkAttachmentDescription> attachments;
        std::vector<VkAttachmentReference> colorAttachmentRefs;
        std::vector<VkAttachmentReference> resolveAttachmentRefs;
        bool needsResolve = createInfo.sampleCount > VK_SAMPLE_COUNT_1_BIT;

        if (createInfo.fragment.targets.empty()) {
            vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
            throw std::runtime_error("Fragment shader must define at least one color target");
        }

        for (const auto& target : createInfo.fragment.targets) {
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = target.format;
            colorAttachment.samples = createInfo.sampleCount;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = needsResolve ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachments.push_back(colorAttachment);

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = static_cast<uint32_t>(attachments.size() - 1);
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachmentRefs.push_back(colorAttachmentRef);

            if (needsResolve) {
                VkAttachmentDescription resolveAttachment{};
                resolveAttachment.format = target.format;
                resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                attachments.push_back(resolveAttachment);

                VkAttachmentReference resolveAttachmentRef{};
                resolveAttachmentRef.attachment = static_cast<uint32_t>(attachments.size() - 1);
                resolveAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                resolveAttachmentRefs.push_back(resolveAttachmentRef);
            }
        }

        // Add depth attachment if present
        VkAttachmentReference depthAttachmentRef{};
        if (createInfo.depthStencil.has_value()) {
            VkAttachmentDescription depthAttachment{};
            depthAttachment.format = createInfo.depthStencil->format;
            depthAttachment.samples = createInfo.sampleCount;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachments.push_back(depthAttachment);

            depthAttachmentRef.attachment = static_cast<uint32_t>(attachments.size() - 1);
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
        subpass.pColorAttachments = colorAttachmentRefs.data();
        subpass.pResolveAttachments = needsResolve ? resolveAttachmentRefs.data() : nullptr;
        if (createInfo.depthStencil.has_value()) {
            subpass.pDepthStencilAttachment = &depthAttachmentRef;
        }

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        VkRenderPass renderPass;
        vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &renderPass);

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
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);

        vkDestroyRenderPass(m_device, renderPass, nullptr);

        if (result != VK_SUCCESS) {
            vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
            throw std::runtime_error("Failed to create graphics pipeline");
        }
    }

    ~RenderPipeline()
    {
        if (m_pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(m_device, m_pipeline, nullptr);
        }
        if (m_pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        }
    }

    VkPipeline handle() const { return m_pipeline; }
    VkPipelineLayout layout() const { return m_pipelineLayout; }

private:
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
};

class ComputePipeline {
public:
    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;

    ComputePipeline(VkDevice device, const ComputePipelineCreateInfo& createInfo)
        : m_device(device)
    {
        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(createInfo.bindGroupLayouts.size());
        pipelineLayoutInfo.pSetLayouts = createInfo.bindGroupLayouts.data();

        VkResult result = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
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

        result = vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
        if (result != VK_SUCCESS) {
            vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
            throw std::runtime_error("Failed to create compute pipeline");
        }
    }

    ~ComputePipeline()
    {
        if (m_pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(m_device, m_pipeline, nullptr);
        }
        if (m_pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        }
    }

    VkPipeline handle() const { return m_pipeline; }
    VkPipelineLayout layout() const { return m_pipelineLayout; }

private:
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
};

class Fence {
public:
    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;

    Fence(VkDevice device, const FenceCreateInfo& createInfo)
        : m_device(device)
    {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        // If signaled = true, create fence in signaled state
        if (createInfo.signaled) {
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        }

        VkResult result = vkCreateFence(m_device, &fenceInfo, nullptr, &m_fence);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create fence");
        }
    }

    ~Fence()
    {
        if (m_fence != VK_NULL_HANDLE) {
            vkDestroyFence(m_device, m_fence, nullptr);
        }
    }

    VkFence handle() const { return m_fence; }

    VkResult getStatus(bool* isSignaled) const
    {
        if (!isSignaled) {
            return VK_ERROR_UNKNOWN;
        }

        VkResult result = vkGetFenceStatus(m_device, m_fence);
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
        return vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, timeoutNs);
    }

    void reset()
    {
        vkResetFences(m_device, 1, &m_fence);
    }

private:
    VkFence m_fence = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
};

class Semaphore {
public:
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

    Semaphore(VkDevice device, const SemaphoreCreateInfo& createInfo)
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

            VkResult result = vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_semaphore);
            if (result != VK_SUCCESS) {
                throw std::runtime_error("Failed to create timeline semaphore");
            }
        } else {
            // Binary semaphore
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkResult result = vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_semaphore);
            if (result != VK_SUCCESS) {
                throw std::runtime_error("Failed to create binary semaphore");
            }
        }
    }

    ~Semaphore()
    {
        if (m_semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device, m_semaphore, nullptr);
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

        return vkSignalSemaphore(m_device, &signalInfo);
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

        return vkWaitSemaphores(m_device, &waitInfo, timeoutNs);
    }

    uint64_t getValue() const
    {
        if (m_type != SemaphoreType::Timeline) {
            return 0; // Binary semaphores don't have values
        }

        uint64_t value = 0;
        vkGetSemaphoreCounterValue(m_device, m_semaphore, &value);
        return value;
    }

private:
    VkSemaphore m_semaphore = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    SemaphoreType m_type = SemaphoreType::Binary;
};

class CommandEncoder {
public:
    CommandEncoder(const CommandEncoder&) = delete;
    CommandEncoder& operator=(const CommandEncoder&) = delete;

    CommandEncoder(VkDevice device, uint32_t queueFamily)
        : m_device(device)
    {
        // Create command pool
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamily;

        VkResult result = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool");
        }

        // Allocate command buffer
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        result = vkAllocateCommandBuffers(m_device, &allocInfo, &m_commandBuffer);
        if (result != VK_SUCCESS) {
            vkDestroyCommandPool(m_device, m_commandPool, nullptr);
            throw std::runtime_error("Failed to allocate command buffer");
        }

        // Begin recording
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
        m_isRecording = true;
    }

    ~CommandEncoder()
    {
        // Automatic cleanup in reverse order - C++ RAII magic!
        for (auto fb : m_framebuffers) {
            vkDestroyFramebuffer(m_device, fb, nullptr);
        }
        for (auto rp : m_renderPasses) {
            vkDestroyRenderPass(m_device, rp, nullptr);
        }
        if (m_commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        }
    }

    VkCommandBuffer handle() const { return m_commandBuffer; }
    VkDevice device() const { return m_device; }
    VkPipelineLayout currentPipelineLayout() const { return m_currentPipelineLayout; }

    void setCurrentPipelineLayout(VkPipelineLayout layout)
    {
        m_currentPipelineLayout = layout;
    }

    void trackRenderPass(VkRenderPass rp, VkFramebuffer fb)
    {
        m_renderPasses.push_back(rp);
        m_framebuffers.push_back(fb);
        m_currentRenderPass = rp;
    }

    void reset()
    {
        // Clean up per-frame resources
        for (auto fb : m_framebuffers) {
            vkDestroyFramebuffer(m_device, fb, nullptr);
        }
        for (auto rp : m_renderPasses) {
            vkDestroyRenderPass(m_device, rp, nullptr);
        }
        m_framebuffers.clear();
        m_renderPasses.clear();
        m_currentPipelineLayout = VK_NULL_HANDLE;
        m_currentRenderPass = VK_NULL_HANDLE;

        // Reset the command pool (this implicitly resets all command buffers)
        vkResetCommandPool(m_device, m_commandPool, 0);

        // Begin recording again
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
        m_isRecording = true;
    }

    void finish()
    {
        if (m_isRecording) {
            vkEndCommandBuffer(m_commandBuffer);
            m_isRecording = false;
        }
    }

private:
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    bool m_isRecording = false;
    VkPipelineLayout m_currentPipelineLayout = VK_NULL_HANDLE;
    VkRenderPass m_currentRenderPass = VK_NULL_HANDLE;

    // Track resources for cleanup - RAII handles lifetime automatically!
    std::vector<VkRenderPass> m_renderPasses;
    std::vector<VkFramebuffer> m_framebuffers;
};

} // namespace gfx::vulkan
