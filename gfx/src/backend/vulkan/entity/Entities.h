#pragma once

#include "../common/VulkanCommon.h"

#include <gfx/gfx.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace gfx::vulkan {

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

class Instance {
public:
    // Prevent copying
    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    Instance(const GfxInstanceDescriptor* descriptor)
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "GfxWrapper Application";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "GfxWrapper";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_1;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Extensions
        std::vector<const char*> extensions = {};
        if (!descriptor->enabledHeadless) {
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

        m_validationEnabled = descriptor && descriptor->enableValidation;
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

        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        // Validation layers
        std::vector<const char*> layers;
        if (m_validationEnabled) {
            layers.push_back("VK_LAYER_KHRONOS_validation");
        }
        createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
        createInfo.ppEnabledLayerNames = layers.data();

        VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
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

    VkInstance handle() const { return m_instance; }

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    bool m_validationEnabled = false;
    GfxDebugCallback m_userCallback = nullptr;
    void* m_userCallbackData = nullptr;

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
            // Map Vulkan severity to Gfx severity
            GfxDebugMessageSeverity severity = GFX_DEBUG_MESSAGE_SEVERITY_INFO;
            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
                severity = GFX_DEBUG_MESSAGE_SEVERITY_VERBOSE;
            } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
                severity = GFX_DEBUG_MESSAGE_SEVERITY_INFO;
            } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
                severity = GFX_DEBUG_MESSAGE_SEVERITY_WARNING;
            } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
                severity = GFX_DEBUG_MESSAGE_SEVERITY_ERROR;
            }

            // Map Vulkan type to Gfx type
            GfxDebugMessageType type = GFX_DEBUG_MESSAGE_TYPE_GENERAL;
            if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
                type = GFX_DEBUG_MESSAGE_TYPE_VALIDATION;
            } else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
                type = GFX_DEBUG_MESSAGE_TYPE_PERFORMANCE;
            }

            instance->m_userCallback(severity, type, pCallbackData->pMessage, instance->m_userCallbackData);
        }
        return VK_FALSE;
    }

public:
    void setDebugCallback(GfxDebugCallback callback, void* userData)
    {
        m_userCallback = callback;
        m_userCallbackData = userData;
    }
};

class Adapter {
public:
    Adapter(const Adapter&) = delete;
    Adapter& operator=(const Adapter&) = delete;

    Adapter(Instance* instance, VkPhysicalDevice pd)
        : m_physicalDevice(pd)
        , m_instance(instance)
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
    }

    VkPhysicalDevice handle() const { return m_physicalDevice; }
    const char* getName() const { return m_properties.deviceName; }
    uint32_t getGraphicsQueueFamily() const { return m_graphicsQueueFamily; }
    Instance* getInstance() const { return m_instance; }

private:
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties m_properties{};
    uint32_t m_graphicsQueueFamily = UINT32_MAX;
    Instance* m_instance = nullptr;
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

    Device(Adapter* adapter, const GfxDeviceDescriptor* descriptor)
        : m_adapter(adapter)
    {
        (void)descriptor;

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

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkResult result = vkCreateDevice(m_adapter->handle(), &createInfo, nullptr, &m_device);
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

    Shader(VkDevice device, const GfxShaderDescriptor* descriptor)
        : m_device(device)
    {
        if (descriptor->entryPoint) {
            m_entryPoint = descriptor->entryPoint;
        } else {
            m_entryPoint = "main";
        }

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = descriptor->codeSize;
        createInfo.pCode = reinterpret_cast<const uint32_t*>(descriptor->code);

        VkResult result = vkCreateShaderModule(m_device, &createInfo, nullptr, &m_shaderModule);
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

    BindGroupLayout(VkDevice device, const GfxBindGroupLayoutDescriptor* descriptor)
        : m_device(device)
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        for (uint32_t i = 0; i < descriptor->entryCount; ++i) {
            const auto& entry = descriptor->entries[i];

            VkDescriptorSetLayoutBinding binding{};
            binding.binding = entry.binding;
            binding.descriptorCount = 1;

            // Convert GfxBindingType to VkDescriptorType
            switch (entry.type) {
            case GFX_BINDING_TYPE_BUFFER:
                binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                break;
            case GFX_BINDING_TYPE_SAMPLER:
                binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                break;
            case GFX_BINDING_TYPE_TEXTURE:
                binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                break;
            case GFX_BINDING_TYPE_STORAGE_TEXTURE:
                binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                break;
            }

            // Convert GfxShaderStage to VkShaderStageFlags
            binding.stageFlags = 0;
            if (entry.visibility & GFX_SHADER_STAGE_VERTEX) {
                binding.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
            }
            if (entry.visibility & GFX_SHADER_STAGE_FRAGMENT) {
                binding.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
            }
            if (entry.visibility & GFX_SHADER_STAGE_COMPUTE) {
                binding.stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
            }

            bindings.push_back(binding);

            // Store binding info for later queries
            m_bindingTypes[entry.binding] = binding.descriptorType;
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

    Surface(VkInstance instance, VkPhysicalDevice physicalDevice, const GfxSurfaceDescriptor* descriptor)
        : m_instance(instance)
        , m_physicalDevice(physicalDevice)
        , m_windowHandle(descriptor ? descriptor->windowHandle : GfxPlatformWindowHandle{})
    {
        if (descriptor && descriptor->windowHandle.windowingSystem == GFX_WINDOWING_SYSTEM_X11 && descriptor->windowHandle.x11.display) {
            VkXlibSurfaceCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
            createInfo.dpy = static_cast<Display*>(descriptor->windowHandle.x11.display);
            createInfo.window = reinterpret_cast<Window>(descriptor->windowHandle.x11.window);

            VkResult result = vkCreateXlibSurfaceKHR(m_instance, &createInfo, nullptr, &m_surface);
            if (result != VK_SUCCESS) {
                throw std::runtime_error("Failed to create Xlib surface");
            }
        }
    }

    ~Surface()
    {
        if (m_surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        }
    }

    VkSurfaceKHR handle() const { return m_surface; }
    VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
    GfxPlatformWindowHandle getPlatformHandle() const { return m_windowHandle; }

private:
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    GfxPlatformWindowHandle m_windowHandle;
};

class Swapchain {
public:
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    Swapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
        uint32_t queueFamily, const GfxSwapchainDescriptor* descriptor)
        : m_device(device)
        , m_physicalDevice(physicalDevice)
        , m_surface(surface)
        , m_width(descriptor->width)
        , m_height(descriptor->height)
    {
        // Check if queue family supports presentation
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamily, surface, &presentSupport);
        if (presentSupport != VK_TRUE) {
            throw std::runtime_error("Selected queue family does not support presentation");
        }

        // Query surface capabilities
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

        // Choose format
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        if (formatCount == 0) {
            throw std::runtime_error("No surface formats available");
        }
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

        // Try to find the requested format
        VkFormat requestedFormat = gfx::convertor::gfxFormatToVkFormat(descriptor->format);
        VkSurfaceFormatKHR surfaceFormat = formats[0]; // Default to first available

        for (const auto& availableFormat : formats) {
            if (availableFormat.format == requestedFormat) {
                surfaceFormat = availableFormat;
                break;
            }
        }

        m_format = surfaceFormat.format;

        // Choose present mode
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

        // Try to find the requested present mode
        VkPresentModeKHR requestedPresentMode = gfx::convertor::gfxPresentModeToVkPresentMode(descriptor->presentMode);
        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // Default to FIFO (always supported)

        for (const auto& availableMode : presentModes) {
            if (availableMode == requestedPresentMode) {
                presentMode = requestedPresentMode;
                break;
            }
        }

        m_presentMode = presentMode;

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
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = std::min(3u, capabilities.minImageCount + 1);
        if (capabilities.maxImageCount > 0) {
            createInfo.minImageCount = std::min(createInfo.minImageCount, capabilities.maxImageCount);
        }
        createInfo.imageFormat = m_format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = actualExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        VkResult result = vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain);
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
            GfxTextureDescriptor textureDesc{};
            textureDesc.type = GFX_TEXTURE_TYPE_2D;
            textureDesc.size = { m_width, m_height, 1 };
            textureDesc.format = gfx::convertor::vkFormatToGfxFormat(m_format);
            textureDesc.mipLevelCount = 1;
            textureDesc.arrayLayerCount = 1;
            textureDesc.sampleCount = GFX_SAMPLE_COUNT_1;
            textureDesc.usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;
            m_textures.push_back(std::make_unique<Texture>(m_device, m_images[i], &textureDesc));

            // Create TextureView for the texture
            GfxTextureViewDescriptor viewDesc{};
            viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
            viewDesc.format = GFX_TEXTURE_FORMAT_UNDEFINED; // Use texture's format
            viewDesc.mipLevelCount = 1;
            viewDesc.baseArrayLayer = 0;
            viewDesc.arrayLayerCount = 1;
            m_textureViews.push_back(std::make_unique<TextureView>(m_textures[i].get(), &viewDesc));
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
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_currentImageIndex = 0;
    VkPresentModeKHR m_presentMode = VK_PRESENT_MODE_FIFO_KHR;
};

class Buffer {
public:
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(VkDevice device, VkPhysicalDevice physicalDevice, const GfxBufferDescriptor* descriptor)
        : m_device(device)
        , m_size(descriptor->size)
        , m_usage(descriptor->usage)
    {
        // Convert GfxBufferUsage to VkBufferUsageFlags
        VkBufferUsageFlags usage = 0;
        if (descriptor->usage & GFX_BUFFER_USAGE_COPY_SRC) {
            usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }
        if (descriptor->usage & GFX_BUFFER_USAGE_COPY_DST) {
            usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }
        if (descriptor->usage & GFX_BUFFER_USAGE_INDEX) {
            usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        }
        if (descriptor->usage & GFX_BUFFER_USAGE_VERTEX) {
            usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }
        if (descriptor->usage & GFX_BUFFER_USAGE_UNIFORM) {
            usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }
        if (descriptor->usage & GFX_BUFFER_USAGE_STORAGE) {
            usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
        if (descriptor->usage & GFX_BUFFER_USAGE_INDIRECT) {
            usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        }

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = m_size;
        bufferInfo.usage = usage;
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
    GfxBufferUsage getUsage() const { return m_usage; }

private:
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    size_t m_size = 0;
    GfxBufferUsage m_usage = static_cast<GfxBufferUsage>(0);
};

class Texture {
public:
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Owning constructor - creates and manages VkImage and memory
    Texture(VkDevice device, VkPhysicalDevice physicalDevice, const GfxTextureDescriptor* descriptor)
        : m_device(device)
        , m_size(descriptor->size)
        , m_format(descriptor->format)
        , m_mipLevelCount(descriptor->mipLevelCount)
        , m_sampleCount(descriptor->sampleCount)
        , m_usage(descriptor->usage)
        , m_ownsResources(true)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = gfx::convertor::gfxTextureTypeToVkImageType(descriptor->type);
        imageInfo.extent.width = descriptor->size.width;
        imageInfo.extent.height = descriptor->size.height;
        imageInfo.extent.depth = descriptor->size.depth;
        imageInfo.mipLevels = descriptor->mipLevelCount;
        imageInfo.arrayLayers = descriptor->arrayLayerCount > 0 ? descriptor->arrayLayerCount : 1;

        // Set cube map flag if needed
        if (descriptor->type == GFX_TEXTURE_TYPE_CUBE) {
            imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            // Cube maps must have 6 or 6*N array layers
            if (imageInfo.arrayLayers < 6) {
                imageInfo.arrayLayers = 6;
            }
        }
        imageInfo.format = gfx::convertor::gfxFormatToVkFormat(descriptor->format);
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Always create in UNDEFINED, transition explicitly

        // Convert GfxTextureUsage to VkImageUsageFlags
        VkImageUsageFlags usage = 0;
        if (descriptor->usage & GFX_TEXTURE_USAGE_COPY_SRC) {
            usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        if (descriptor->usage & GFX_TEXTURE_USAGE_COPY_DST) {
            usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if (descriptor->usage & GFX_TEXTURE_USAGE_TEXTURE_BINDING) {
            usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if (descriptor->usage & GFX_TEXTURE_USAGE_STORAGE_BINDING) {
            usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        }
        if (descriptor->usage & GFX_TEXTURE_USAGE_RENDER_ATTACHMENT) {
            // Check if this is a depth/stencil format
            VkFormat format = imageInfo.format;
            if (gfx::convertor::isDepthFormat(format)) {
                usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            } else {
                usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            }
        }
        imageInfo.usage = usage;

        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = gfx::convertor::sampleCountToVkSampleCount(descriptor->sampleCount);

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
    Texture(VkDevice device, VkImage image, const GfxTextureDescriptor* descriptor)
        : m_image(image)
        , m_device(device)
        , m_size(descriptor->size)
        , m_format(descriptor->format)
        , m_mipLevelCount(descriptor->mipLevelCount)
        , m_sampleCount(descriptor->sampleCount)
        , m_usage(descriptor->usage)
        , m_ownsResources(false)
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
    GfxExtent3D getSize() const { return m_size; }
    GfxTextureFormat getFormat() const { return m_format; }
    uint32_t getMipLevelCount() const { return m_mipLevelCount; }
    GfxSampleCount getSampleCount() const { return m_sampleCount; }
    GfxTextureUsage getUsage() const { return m_usage; }
    GfxTextureLayout getLayout() const { return m_currentLayout; }
    void setLayout(GfxTextureLayout layout) { m_currentLayout = layout; }

private:
    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    GfxExtent3D m_size;
    GfxTextureFormat m_format;
    uint32_t m_mipLevelCount;
    GfxSampleCount m_sampleCount;
    GfxTextureUsage m_usage;
    GfxTextureLayout m_currentLayout = GFX_TEXTURE_LAYOUT_UNDEFINED;
    bool m_ownsResources = true;
};

class TextureView {
public:
    TextureView(const TextureView&) = delete;
    TextureView& operator=(const TextureView&) = delete;

    TextureView(Texture* texture, const GfxTextureViewDescriptor* descriptor)
        : m_device(texture->device())
        , m_texture(texture)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = texture->handle();
        viewInfo.viewType = gfx::convertor::gfxTextureViewTypeToVkImageViewType(descriptor->viewType);
        // Use texture's format if not explicitly specified in descriptor
        m_format = (descriptor->format == GFX_TEXTURE_FORMAT_UNDEFINED)
            ? texture->getFormat()
            : descriptor->format;
        viewInfo.format = gfx::convertor::gfxFormatToVkFormat(m_format);
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

        viewInfo.subresourceRange.baseMipLevel = descriptor ? descriptor->baseMipLevel : 0;
        viewInfo.subresourceRange.levelCount = descriptor ? descriptor->mipLevelCount : 1;
        viewInfo.subresourceRange.baseArrayLayer = descriptor ? descriptor->baseArrayLayer : 0;
        viewInfo.subresourceRange.layerCount = descriptor ? descriptor->arrayLayerCount : 1;

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
    VkFormat getFormat() const { return gfx::convertor::gfxFormatToVkFormat(m_format); }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    Texture* m_texture = nullptr;
    VkImageView m_imageView = VK_NULL_HANDLE;
    GfxTextureFormat m_format; // View format (may differ from texture format)
};

class Sampler {
public:
    Sampler(const Sampler&) = delete;
    Sampler& operator=(const Sampler&) = delete;

    Sampler(VkDevice device, const GfxSamplerDescriptor* descriptor)
        : m_device(device)
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        // Address modes
        samplerInfo.addressModeU = static_cast<VkSamplerAddressMode>(descriptor->addressModeU);
        samplerInfo.addressModeV = static_cast<VkSamplerAddressMode>(descriptor->addressModeV);
        samplerInfo.addressModeW = static_cast<VkSamplerAddressMode>(descriptor->addressModeW);

        // Filter modes
        samplerInfo.magFilter = (descriptor->magFilter == GFX_FILTER_MODE_LINEAR) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        samplerInfo.minFilter = (descriptor->minFilter == GFX_FILTER_MODE_LINEAR) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        samplerInfo.mipmapMode = (descriptor->mipmapFilter == GFX_FILTER_MODE_LINEAR) ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;

        // LOD
        samplerInfo.minLod = descriptor->lodMinClamp;
        samplerInfo.maxLod = descriptor->lodMaxClamp;

        // Anisotropy
        if (descriptor->maxAnisotropy > 1) {
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = static_cast<float>(descriptor->maxAnisotropy);
        } else {
            samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.maxAnisotropy = 1.0f;
        }

        // Compare operation for depth textures
        if (descriptor->compare) {
            samplerInfo.compareEnable = VK_TRUE;
            samplerInfo.compareOp = static_cast<VkCompareOp>(*descriptor->compare);
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

    BindGroup(VkDevice device, const GfxBindGroupDescriptor* descriptor)
        : m_device(device)
    {
        // Count actual descriptors needed by type
        auto* layout = reinterpret_cast<BindGroupLayout*>(descriptor->layout);
        std::unordered_map<VkDescriptorType, uint32_t> descriptorCounts;

        for (uint32_t i = 0; i < descriptor->entryCount; ++i) {
            const auto& entry = descriptor->entries[i];
            VkDescriptorType type;

            if (entry.type == GFX_BIND_GROUP_ENTRY_TYPE_BUFFER) {
                type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            } else if (entry.type == GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER) {
                type = VK_DESCRIPTOR_TYPE_SAMPLER;
            } else if (entry.type == GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW) {
                type = layout->getBindingType(entry.binding);
            } else {
                continue;
            }

            ++descriptorCounts[type];
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
        VkDescriptorSetLayout setLayout = layout->handle();

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
        bufferInfos.reserve(descriptor->entryCount);
        imageInfos.reserve(descriptor->entryCount);
        descriptorWrites.reserve(descriptor->entryCount);

        // Track indices for buffer and image infos
        size_t bufferInfoIndex = 0;
        size_t imageInfoIndex = 0;

        for (uint32_t i = 0; i < descriptor->entryCount; ++i) {
            const auto& entry = descriptor->entries[i];

            if (entry.type == GFX_BIND_GROUP_ENTRY_TYPE_BUFFER) {
                auto* buffer = reinterpret_cast<Buffer*>(entry.resource.buffer.buffer);

                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = buffer->handle();
                bufferInfo.offset = entry.resource.buffer.offset;
                bufferInfo.range = entry.resource.buffer.size;
                bufferInfos.push_back(bufferInfo);

                VkWriteDescriptorSet descriptorWrite{};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = m_descriptorSet;
                descriptorWrite.dstBinding = entry.binding;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pBufferInfo = &bufferInfos[bufferInfoIndex++];

                descriptorWrites.push_back(descriptorWrite);
            } else if (entry.type == GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER) {
                auto* sampler = reinterpret_cast<Sampler*>(entry.resource.sampler);

                VkDescriptorImageInfo imageInfo{};
                imageInfo.sampler = sampler->handle();
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
            } else if (entry.type == GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW) {
                auto* textureView = reinterpret_cast<TextureView*>(entry.resource.textureView);

                // Query the bind group layout for the correct descriptor type
                VkDescriptorType descriptorType = layout->getBindingType(entry.binding);

                VkDescriptorImageInfo imageInfo{};
                imageInfo.sampler = VK_NULL_HANDLE;
                imageInfo.imageView = textureView->handle();

                // Set image layout based on descriptor type from the layout
                // Storage images use GENERAL layout
                // Sampled images use SHADER_READ_ONLY_OPTIMAL layout
                if (descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
                    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                } else {
                    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }

                imageInfos.push_back(imageInfo);

                VkWriteDescriptorSet descriptorWrite{};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = m_descriptorSet;
                descriptorWrite.dstBinding = entry.binding;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = descriptorType;
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

    RenderPipeline(VkDevice device, const GfxRenderPipelineDescriptor* descriptor)
        : m_device(device)
    {
        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        // Process bind group layouts
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        for (uint32_t i = 0; i < descriptor->bindGroupLayoutCount; ++i) {
            auto* layout = reinterpret_cast<BindGroupLayout*>(descriptor->bindGroupLayouts[i]);
            descriptorSetLayouts.push_back(layout->handle());
        }

        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

        VkResult result = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout");
        }

        // Shader stages
        auto* vertShader = reinterpret_cast<Shader*>(descriptor->vertex->module);
        auto* fragShader = descriptor->fragment ? reinterpret_cast<Shader*>(descriptor->fragment->module) : nullptr;

        if (!vertShader || vertShader->handle() == VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
            throw std::runtime_error("Invalid vertex shader");
        }

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShader->handle();
        vertShaderStageInfo.pName = vertShader->entryPoint();

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        uint32_t stageCount = 1;
        if (fragShader) {
            if (fragShader->handle() == VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
                throw std::runtime_error("Invalid fragment shader");
            }
            fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            fragShaderStageInfo.module = fragShader->handle();
            fragShaderStageInfo.pName = fragShader->entryPoint();
            stageCount = 2;
        }

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        // Process vertex input
        std::vector<VkVertexInputBindingDescription> bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;

        for (uint32_t i = 0; i < descriptor->vertex->bufferCount; ++i) {
            const auto& bufferLayout = descriptor->vertex->buffers[i];

            VkVertexInputBindingDescription binding{};
            binding.binding = i;
            binding.stride = static_cast<uint32_t>(bufferLayout.arrayStride);
            binding.inputRate = bufferLayout.stepModeInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
            bindings.push_back(binding);

            for (uint32_t j = 0; j < bufferLayout.attributeCount; ++j) {
                const auto& attr = bufferLayout.attributes[j];

                VkVertexInputAttributeDescription attribute{};
                attribute.binding = i;
                attribute.location = attr.shaderLocation;
                attribute.offset = static_cast<uint32_t>(attr.offset);
                attribute.format = gfx::convertor::gfxFormatToVkFormat(attr.format);

                attributes.push_back(attribute);
            }
        }

        VkSampleCountFlagBits vkSampleCount = gfx::convertor::sampleCountToVkSampleCount(descriptor->sampleCount);

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
        vertexInputInfo.pVertexBindingDescriptions = bindings.data();
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = static_cast<VkPrimitiveTopology>(descriptor->primitive->topology);

        // Viewport
        VkViewport viewport{};
        viewport.x = 0.0f;
        inputAssembly.topology = static_cast<VkPrimitiveTopology>(descriptor->primitive->topology);
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
        rasterizer.polygonMode = gfx::convertor::gfxPolygonModeToVkPolygonMode(descriptor->primitive->polygonMode);
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = gfx::convertor::gfxCullModeToVkCullMode(descriptor->primitive->cullMode);
        rasterizer.frontFace = gfx::convertor::gfxFrontFaceToVkFrontFace(descriptor->primitive->frontFace);

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = vkSampleCount;

        // Color blending - process all color targets
        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
        if (descriptor->fragment && descriptor->fragment->targetCount > 0) {
            colorBlendAttachments.reserve(descriptor->fragment->targetCount);

            for (uint32_t i = 0; i < descriptor->fragment->targetCount; ++i) {
                const auto& target = descriptor->fragment->targets[i];
                VkPipelineColorBlendAttachmentState blendAttachment{};

                // Convert GfxColorWriteMask to VkColorComponentFlags
                blendAttachment.colorWriteMask = 0;
                if (target.writeMask & 0x1)
                    blendAttachment.colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
                if (target.writeMask & 0x2)
                    blendAttachment.colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
                if (target.writeMask & 0x4)
                    blendAttachment.colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
                if (target.writeMask & 0x8)
                    blendAttachment.colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;

                // Configure blend state if provided
                if (target.blend) {
                    blendAttachment.blendEnable = VK_TRUE;

                    // Color blend
                    blendAttachment.srcColorBlendFactor = static_cast<VkBlendFactor>(target.blend->color.srcFactor);
                    blendAttachment.dstColorBlendFactor = static_cast<VkBlendFactor>(target.blend->color.dstFactor);
                    blendAttachment.colorBlendOp = static_cast<VkBlendOp>(target.blend->color.operation);

                    // Alpha blend
                    blendAttachment.srcAlphaBlendFactor = static_cast<VkBlendFactor>(target.blend->alpha.srcFactor);
                    blendAttachment.dstAlphaBlendFactor = static_cast<VkBlendFactor>(target.blend->alpha.dstFactor);
                    blendAttachment.alphaBlendOp = static_cast<VkBlendOp>(target.blend->alpha.operation);
                } else {
                    blendAttachment.blendEnable = VK_FALSE;
                }

                colorBlendAttachments.push_back(blendAttachment);
            }
        } else {
            // Fallback: single target with all channels enabled
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
        if (descriptor->depthStencil) {
            depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = descriptor->depthStencil->depthWriteEnabled ? VK_TRUE : VK_FALSE;
            depthStencil.depthCompareOp = static_cast<VkCompareOp>(descriptor->depthStencil->depthCompare);
            depthStencil.depthBoundsTestEnable = VK_FALSE;
            depthStencil.stencilTestEnable = VK_FALSE;
        }

        // Create a temporary render pass for pipeline creation using actual formats from descriptor
        // This render pass must be compatible with the one used during actual rendering
        // Supports Multiple Render Targets (MRT) with optional MSAA resolve per target
        std::vector<VkAttachmentDescription> attachments;
        std::vector<VkAttachmentReference> colorAttachmentRefs;
        std::vector<VkAttachmentReference> resolveAttachmentRefs;
        bool needsResolve = vkSampleCount > VK_SAMPLE_COUNT_1_BIT;

        // Process all fragment shader color targets (MRT support)
        if (!descriptor->fragment || descriptor->fragment->targetCount == 0) {
            vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
            throw std::runtime_error("Fragment shader must define at least one color target");
        }

        uint32_t targetCount = descriptor->fragment->targetCount;

        for (uint32_t i = 0; i < targetCount; ++i) {
            VkFormat targetFormat = gfx::convertor::gfxFormatToVkFormat(descriptor->fragment->targets[i].format);

            // Create color attachment at pipeline sample count
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = targetFormat;
            colorAttachment.samples = vkSampleCount;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = needsResolve ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachments.push_back(colorAttachment);

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = static_cast<uint32_t>(attachments.size() - 1);
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachmentRefs.push_back(colorAttachmentRef);

            // Add resolve attachment if MSAA is enabled
            // Note: This assumes all color targets get resolved - adjust per-target if needed
            if (needsResolve) {
                VkAttachmentDescription resolveAttachment{};
                resolveAttachment.format = targetFormat;
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

        // Add depth attachment if present (always after all color attachments)
        VkAttachmentReference depthAttachmentRef{};
        if (descriptor->depthStencil) {
            VkAttachmentDescription depthAttachment{};
            depthAttachment.format = gfx::convertor::gfxFormatToVkFormat(descriptor->depthStencil->format);
            depthAttachment.samples = vkSampleCount;
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
        if (descriptor->depthStencil) {
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
        if (descriptor->depthStencil) {
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

    ComputePipeline(VkDevice device, const GfxComputePipelineDescriptor* descriptor)
        : m_device(device)
    {
        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        // Process bind group layouts
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        for (uint32_t i = 0; i < descriptor->bindGroupLayoutCount; ++i) {
            auto* layout = reinterpret_cast<BindGroupLayout*>(descriptor->bindGroupLayouts[i]);
            descriptorSetLayouts.push_back(layout->handle());
        }

        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

        VkResult result = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create compute pipeline layout");
        }

        // Shader stage
        auto* computeShader = reinterpret_cast<Shader*>(descriptor->compute);

        VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
        computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        computeShaderStageInfo.module = computeShader->handle();
        computeShaderStageInfo.pName = descriptor->entryPoint ? descriptor->entryPoint : "main";

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

    Fence(VkDevice device, const GfxFenceDescriptor* descriptor)
        : m_device(device)
    {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        // If signaled = true, create fence in signaled state
        if (descriptor && descriptor->signaled) {
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

    GfxResult getStatus(bool* isSignaled) const
    {
        if (!isSignaled) {
            return GFX_RESULT_ERROR_INVALID_PARAMETER;
        }

        VkResult result = vkGetFenceStatus(m_device, m_fence);
        if (result == VK_SUCCESS) {
            *isSignaled = true;
            return GFX_RESULT_SUCCESS;
        } else if (result == VK_NOT_READY) {
            *isSignaled = false;
            return GFX_RESULT_SUCCESS;
        } else {
            return GFX_RESULT_ERROR_UNKNOWN;
        }
    }

    GfxResult wait(uint64_t timeoutNs)
    {
        VkResult result = vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, timeoutNs);
        if (result == VK_SUCCESS) {
            return GFX_RESULT_SUCCESS;
        } else if (result == VK_TIMEOUT) {
            return GFX_RESULT_TIMEOUT;
        } else {
            return GFX_RESULT_ERROR_UNKNOWN;
        }
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

    Semaphore(VkDevice device, const GfxSemaphoreDescriptor* descriptor)
        : m_device(device)
        , m_type(descriptor ? descriptor->type : GFX_SEMAPHORE_TYPE_BINARY)
    {

        if (m_type == GFX_SEMAPHORE_TYPE_TIMELINE) {
            // Timeline semaphore
            VkSemaphoreTypeCreateInfo timelineInfo{};
            timelineInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            timelineInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            timelineInfo.initialValue = descriptor ? descriptor->initialValue : 0;

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
    GfxSemaphoreType type() const { return m_type; }

    GfxResult signal(uint64_t value)
    {
        if (m_type != GFX_SEMAPHORE_TYPE_TIMELINE) {
            return GFX_RESULT_ERROR_INVALID_PARAMETER; // Binary semaphores can't be manually signaled
        }

        VkSemaphoreSignalInfo signalInfo{};
        signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
        signalInfo.semaphore = m_semaphore;
        signalInfo.value = value;

        VkResult result = vkSignalSemaphore(m_device, &signalInfo);
        return (result == VK_SUCCESS) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
    }

    GfxResult wait(uint64_t value, uint64_t timeoutNs)
    {
        if (m_type != GFX_SEMAPHORE_TYPE_TIMELINE) {
            return GFX_RESULT_ERROR_INVALID_PARAMETER; // Binary semaphores can't be manually waited
        }

        VkSemaphoreWaitInfo waitInfo{};
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &m_semaphore;
        waitInfo.pValues = &value;

        VkResult result = vkWaitSemaphores(m_device, &waitInfo, timeoutNs);
        if (result == VK_SUCCESS) {
            return GFX_RESULT_SUCCESS;
        } else if (result == VK_TIMEOUT) {
            return GFX_RESULT_TIMEOUT;
        } else {
            return GFX_RESULT_ERROR_UNKNOWN;
        }
    }

    uint64_t getValue() const
    {
        if (m_type != GFX_SEMAPHORE_TYPE_TIMELINE) {
            return 0; // Binary semaphores don't have values
        }

        uint64_t value = 0;
        vkGetSemaphoreCounterValue(m_device, m_semaphore, &value);
        return value;
    }

private:
    VkSemaphore m_semaphore = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    GfxSemaphoreType m_type;
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
