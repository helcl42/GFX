#include "GfxApi.h"
#include "GfxBackend.h"

// Platform-specific Vulkan extensions
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#define VK_USE_PLATFORM_XCB_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#elif defined(__APPLE__)
#define VK_USE_PLATFORM_MACOS_MVK
#endif

#include <vulkan/vulkan.h>

#ifdef __linux__
#include <X11/Xlib-xcb.h>
#include <X11/Xlib.h>
#include <xcb/xcb.h>
#endif

#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// ============================================================================
// Format Conversion Helpers
// ============================================================================

namespace {
VkFormat gfxFormatToVkFormat(GfxTextureFormat format)
{
    switch (format) {
    case GFX_TEXTURE_FORMAT_UNDEFINED:
        return VK_FORMAT_UNDEFINED;
    case GFX_TEXTURE_FORMAT_R8_UNORM:
        return VK_FORMAT_R8_UNORM;
    case GFX_TEXTURE_FORMAT_R8G8_UNORM:
        return VK_FORMAT_R8G8_UNORM;
    case GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM_SRGB:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM:
        return VK_FORMAT_B8G8R8A8_UNORM;
    case GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB:
        return VK_FORMAT_B8G8R8A8_SRGB;
    case GFX_TEXTURE_FORMAT_R16_FLOAT:
        return VK_FORMAT_R16_SFLOAT;
    case GFX_TEXTURE_FORMAT_R16G16_FLOAT:
        return VK_FORMAT_R16G16_SFLOAT;
    case GFX_TEXTURE_FORMAT_R16G16B16A16_FLOAT:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case GFX_TEXTURE_FORMAT_R32_FLOAT:
        return VK_FORMAT_R32_SFLOAT;
    case GFX_TEXTURE_FORMAT_R32G32_FLOAT:
        return VK_FORMAT_R32G32_SFLOAT;
    case GFX_TEXTURE_FORMAT_R32G32B32_FLOAT:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case GFX_TEXTURE_FORMAT_R32G32B32A32_FLOAT:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case GFX_TEXTURE_FORMAT_DEPTH16_UNORM:
        return VK_FORMAT_D16_UNORM;
    case GFX_TEXTURE_FORMAT_DEPTH24_PLUS:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case GFX_TEXTURE_FORMAT_DEPTH32_FLOAT:
        return VK_FORMAT_D32_SFLOAT;
    case GFX_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case GFX_TEXTURE_FORMAT_DEPTH32_FLOAT_STENCIL8:
        return VK_FORMAT_D32_SFLOAT_S8_UINT;
    default:
        return VK_FORMAT_UNDEFINED;
    }
}

bool isDepthFormat(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D16_UNORM;
}

bool hasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}

VkImageAspectFlags getImageAspectMask(VkFormat format)
{
    if (isDepthFormat(format)) {
        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (hasStencilComponent(format)) {
            aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        return aspectMask;
    }
    return VK_IMAGE_ASPECT_COLOR_BIT;
}

VkImageLayout gfxLayoutToVkImageLayout(GfxTextureLayout layout)
{
    switch (layout) {
    case GFX_TEXTURE_LAYOUT_UNDEFINED:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    case GFX_TEXTURE_LAYOUT_GENERAL:
        return VK_IMAGE_LAYOUT_GENERAL;
    case GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    case GFX_TEXTURE_LAYOUT_SHADER_READ_ONLY:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case GFX_TEXTURE_LAYOUT_TRANSFER_SRC:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case GFX_TEXTURE_LAYOUT_TRANSFER_DST:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case GFX_TEXTURE_LAYOUT_PRESENT_SRC:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    default:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

} // namespace

// ============================================================================
// Forward Declarations
// ============================================================================

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
} // namespace gfx::vulkan

// ============================================================================
// Internal C++ Classes with RAII
// ============================================================================

namespace gfx::vulkan {

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
        appInfo.apiVersion = VK_API_VERSION_1_3;

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
            throw std::runtime_error("Failed to create Vulkan instance");
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

    void setupDebugMessenger()
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;

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
        (void)messageType;
        (void)pUserData;

        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            fprintf(stderr, "[Vulkan] %s\n", pCallbackData->pMessage);
        }
        return VK_FALSE;
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

        for (uint32_t i = 0; i < queueFamilyCount; i++) {
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
        // TODO in descriptor
        // - query presentable device when not headless
        // - pass device override index in
        // - handle heaadless
        // - additional extensions
        // - additional features
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

        for (uint32_t i = 0; i < descriptor->entryCount; i++) {
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

private:
    VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
};

class Surface {
public:
    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;

    Surface(VkInstance instance, const GfxSurfaceDescriptor* descriptor)
        : m_instance(instance)
        , m_width(descriptor ? descriptor->width : 0)
        , m_height(descriptor ? descriptor->height : 0)
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
    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }
    GfxPlatformWindowHandle getPlatformHandle() const { return m_windowHandle; }

    void resize(uint32_t width, uint32_t height)
    {
        m_width = width;
        m_height = height;
    }

private:
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkInstance m_instance = VK_NULL_HANDLE;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    GfxPlatformWindowHandle m_windowHandle;
};

class Swapchain {
public:
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    Swapchain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
        const GfxSwapchainDescriptor* descriptor)
        : m_device(device)
        , m_physicalDevice(physicalDevice)
        , m_surface(surface)
        , m_width(descriptor->width)
        , m_height(descriptor->height)
    {
        // Query surface capabilities
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

        // Choose format
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

        VkSurfaceFormatKHR surfaceFormat = formats[0];
        m_format = surfaceFormat.format;

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
        createInfo.imageExtent = { m_width, m_height };
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
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

        // Create texture view objects (not just VkImageViews)
        m_textureViews.reserve(imageCount);
        for (size_t i = 0; i < imageCount; i++) {
            m_textureViews.push_back(std::make_unique<TextureView>(m_device, m_images[i], m_format));
        }

        // Get present queue (assume queue family 0)
        vkGetDeviceQueue(m_device, 0, 0, &m_presentQueue);

        // Create fence for acquire (used internally by legacy present function)
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        vkCreateFence(m_device, &fenceInfo, nullptr, &m_acquireFence);

        // Don't pre-acquire an image - let explicit acquire handle it
        m_currentImageIndex = 0;
    }

    ~Swapchain()
    {
        // TextureView objects will be automatically destroyed by unique_ptr
        if (m_acquireFence != VK_NULL_HANDLE) {
            vkDestroyFence(m_device, m_acquireFence, nullptr);
        }
        if (m_swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        }
    }

    VkSwapchainKHR handle() const { return m_swapchain; }
    uint32_t getImageCount() const { return static_cast<uint32_t>(m_images.size()); }
    TextureView* getTextureView(uint32_t index) const { return m_textureViews[index].get(); }
    TextureView* getCurrentTextureView() const { return m_textureViews[m_currentImageIndex].get(); }
    VkFormat getFormat() const { return m_format; }
    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }
    uint32_t getCurrentImageIndex() const { return m_currentImageIndex; }

    VkResult acquireNextImage(uint64_t timeoutNs, VkSemaphore semaphore, VkFence fence, uint32_t* outImageIndex)
    {
        VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, timeoutNs, semaphore, fence, outImageIndex);
        if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR) {
            m_currentImageIndex = *outImageIndex;
        }
        return result;
    }

    void present()
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 0;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_swapchain;
        presentInfo.pImageIndices = &m_currentImageIndex;

        vkQueuePresentKHR(m_presentQueue, &presentInfo);

        // Acquire next image for next frame
        vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, VK_NULL_HANDLE, m_acquireFence, &m_currentImageIndex);
        vkWaitForFences(m_device, 1, &m_acquireFence, VK_TRUE, UINT64_MAX);
        vkResetFences(m_device, 1, &m_acquireFence);
    }

    VkResult presentWithSync(const std::vector<VkSemaphore>& waitSemaphores)
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

    void resize(uint32_t newWidth, uint32_t newHeight)
    {
        if (newWidth == 0 || newHeight == 0) {
            return;
        }

        // Wait for device to be idle
        vkDeviceWaitIdle(m_device);

        m_width = newWidth;
        m_height = newHeight;

        // Clean up old texture views
        m_textureViews.clear();

        // Store old swapchain for recreation
        VkSwapchainKHR oldSwapchain = m_swapchain;

        // Query surface capabilities
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &capabilities);

        // Choose format
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());

        VkSurfaceFormatKHR surfaceFormat = formats[0];
        m_format = surfaceFormat.format;

        // Create new swapchain
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_surface;
        createInfo.minImageCount = std::min(3u, capabilities.minImageCount + 1);
        if (capabilities.maxImageCount > 0) {
            createInfo.minImageCount = std::min(createInfo.minImageCount, capabilities.maxImageCount);
        }
        createInfo.imageFormat = m_format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = { m_width, m_height };
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = oldSwapchain;

        VkResult result = vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to recreate swapchain");
        }

        // Destroy old swapchain
        if (oldSwapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_device, oldSwapchain, nullptr);
        }

        // Get new swapchain images
        uint32_t imageCount;
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
        m_images.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_images.data());

        // Create new texture views
        m_textureViews.reserve(imageCount);
        for (size_t i = 0; i < imageCount; i++) {
            m_textureViews.push_back(std::make_unique<TextureView>(m_device, m_images[i], m_format));
        }

        // Acquire first image
        vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, VK_NULL_HANDLE, m_acquireFence, &m_currentImageIndex);
        vkWaitForFences(m_device, 1, &m_acquireFence, VK_TRUE, UINT64_MAX);
        vkResetFences(m_device, 1, &m_acquireFence);
    }

private:
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    std::vector<VkImage> m_images;
    std::vector<std::unique_ptr<TextureView>> m_textureViews;
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_currentImageIndex = 0;
    VkFence m_acquireFence = VK_NULL_HANDLE;
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

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
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

class BindGroup {
public:
    BindGroup(const BindGroup&) = delete;
    BindGroup& operator=(const BindGroup&) = delete;

    BindGroup(VkDevice device, const GfxBindGroupDescriptor* descriptor)
        : m_device(device)
    {
        // Create descriptor pool
        std::vector<VkDescriptorPoolSize> poolSizes;
        VkDescriptorPoolSize uniformBufferSize{};
        uniformBufferSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferSize.descriptorCount = 10;
        poolSizes.push_back(uniformBufferSize);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 10;

        VkResult result = vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_pool);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool");
        }

        // Allocate descriptor set
        auto* layout = reinterpret_cast<BindGroupLayout*>(descriptor->layout);
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
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        std::vector<VkDescriptorBufferInfo> bufferInfos;

        for (uint32_t i = 0; i < descriptor->entryCount; i++) {
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
                descriptorWrite.pBufferInfo = &bufferInfos.back();

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

class Texture {
public:
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(VkDevice device, VkPhysicalDevice physicalDevice, const GfxTextureDescriptor* descriptor)
        : m_device(device)
        , m_size(descriptor->size)
        , m_format(descriptor->format)
        , m_mipLevelCount(descriptor->mipLevelCount)
        , m_sampleCount(descriptor->sampleCount)
        , m_usage(descriptor->usage)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = descriptor->size.width;
        imageInfo.extent.height = descriptor->size.height;
        imageInfo.extent.depth = descriptor->size.depth;
        imageInfo.mipLevels = descriptor->mipLevelCount;
        imageInfo.arrayLayers = 1;
        imageInfo.format = gfxFormatToVkFormat(descriptor->format);
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
            if (isDepthFormat(format)) {
                usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            } else {
                usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            }
        }
        imageInfo.usage = usage;

        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        VkResult result = vkCreateImage(m_device, &imageInfo, nullptr, &m_image);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_device, m_image, &memRequirements);

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        uint32_t memoryTypeIndex = UINT32_MAX;
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
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

    ~Texture()
    {
        if (m_memory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, m_memory, nullptr);
        }
        if (m_image != VK_NULL_HANDLE) {
            vkDestroyImage(m_device, m_image, nullptr);
        }
    }

    VkImage handle() const { return m_image; }
    VkDevice device() const { return m_device; }
    GfxExtent3D getSize() const { return m_size; }
    GfxTextureFormat getFormat() const { return m_format; }
    uint32_t getMipLevelCount() const { return m_mipLevelCount; }
    uint32_t getSampleCount() const { return m_sampleCount; }
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
    uint32_t m_sampleCount;
    GfxTextureUsage m_usage;
    GfxTextureLayout m_currentLayout = GFX_TEXTURE_LAYOUT_UNDEFINED;
};

class TextureView {
public:
    TextureView(const TextureView&) = delete;
    TextureView& operator=(const TextureView&) = delete;

    TextureView(VkDevice device, VkImage image, VkFormat format, GfxTexture texture = nullptr)
        : m_device(device)
        , m_texture(texture)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // Set aspect mask based on format
        if (isDepthFormat(format)) {
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (hasStencilComponent(format)) {
                viewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        } else {
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

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
    GfxTexture getTexture() const { return m_texture; }

private:
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    GfxTexture m_texture = nullptr;
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
        for (uint32_t i = 0; i < descriptor->bindGroupLayoutCount; i++) {
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

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShader->handle();
        vertShaderStageInfo.pName = vertShader->entryPoint();

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        uint32_t stageCount = 1;
        if (fragShader) {
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

        for (uint32_t i = 0; i < descriptor->vertex->bufferCount; i++) {
            const auto& bufferLayout = descriptor->vertex->buffers[i];

            VkVertexInputBindingDescription binding{};
            binding.binding = i;
            binding.stride = static_cast<uint32_t>(bufferLayout.arrayStride);
            binding.inputRate = bufferLayout.stepModeInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
            bindings.push_back(binding);

            for (uint32_t j = 0; j < bufferLayout.attributeCount; j++) {
                const auto& attr = bufferLayout.attributes[j];

                VkVertexInputAttributeDescription attribute{};
                attribute.binding = i;
                attribute.location = attr.shaderLocation;
                attribute.offset = static_cast<uint32_t>(attr.offset);

                // Convert GfxTextureFormat to VkFormat for vertex attributes
                switch (attr.format) {
                case GFX_TEXTURE_FORMAT_R32_FLOAT:
                    attribute.format = VK_FORMAT_R32_SFLOAT;
                    break;
                case GFX_TEXTURE_FORMAT_R32G32_FLOAT:
                    attribute.format = VK_FORMAT_R32G32_SFLOAT;
                    break;
                case GFX_TEXTURE_FORMAT_R32G32B32_FLOAT:
                    attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
                    break;
                case GFX_TEXTURE_FORMAT_R32G32B32A32_FLOAT:
                    attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
                    break;
                default:
                    attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
                    break;
                }

                attributes.push_back(attribute);
            }
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
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // Viewport
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

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

        // Create a simple render pass for pipeline creation
        std::vector<VkAttachmentDescription> attachments;
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachments.push_back(colorAttachment);

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        if (descriptor->depthStencil) {
            VkAttachmentDescription depthAttachment{};
            depthAttachment.format = gfxFormatToVkFormat(descriptor->depthStencil->format);
            depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachments.push_back(depthAttachment);

            depthAttachmentRef.attachment = 1;
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
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
        // Create pipeline layout (empty for now, bind groups can be added later)
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;

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

// ============================================================================
// C API Implementation - Thin Wrappers
// ============================================================================

extern "C" {

GfxResult vulkan_createInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance)
{
    if (!descriptor || !outInstance) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* instance = new gfx::vulkan::Instance(descriptor);
        *outInstance = reinterpret_cast<GfxInstance>(instance);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create instance: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void vulkan_instanceDestroy(GfxInstance instance)
{
    delete reinterpret_cast<gfx::vulkan::Instance*>(instance);
}

GfxResult vulkan_instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter)
{
    if (!instance || !outAdapter) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* inst = reinterpret_cast<gfx::vulkan::Instance*>(instance);

    try {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(inst->handle(), &deviceCount, nullptr);

        if (deviceCount == 0) {
            return GFX_RESULT_ERROR_UNKNOWN;
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(inst->handle(), &deviceCount, devices.data());

        // For simplicity, pick first discrete GPU or just first device
        VkPhysicalDevice selectedDevice = devices[0];
        for (auto device : devices) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                selectedDevice = device;
                break;
            }
        }

        auto* adapter = new gfx::vulkan::Adapter(inst, selectedDevice);
        *outAdapter = reinterpret_cast<GfxAdapter>(adapter);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to request adapter: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    (void)descriptor; // Unused for now
}

uint32_t vulkan_instanceEnumerateAdapters(GfxInstance instance, GfxAdapter* adapters, uint32_t maxAdapters)
{
    if (!instance) {
        return 0;
    }

    auto* inst = reinterpret_cast<gfx::vulkan::Instance*>(instance);

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(inst->handle(), &deviceCount, nullptr);

    if (deviceCount == 0 || !adapters) {
        return deviceCount;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(inst->handle(), &deviceCount, devices.data());

    uint32_t count = std::min(deviceCount, maxAdapters);
    for (uint32_t i = 0; i < count; i++) {
        adapters[i] = reinterpret_cast<GfxAdapter>(new gfx::vulkan::Adapter(inst, devices[i]));
    }

    return count;
}

void vulkan_adapterDestroy(GfxAdapter adapter)
{
    delete reinterpret_cast<gfx::vulkan::Adapter*>(adapter);
}

GfxResult vulkan_adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice)
{
    if (!adapter || !outDevice) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        // Device doesn't own the adapter - just keeps a pointer
        auto* adapterPtr = reinterpret_cast<gfx::vulkan::Adapter*>(adapter);
        auto* device = new gfx::vulkan::Device(adapterPtr, descriptor);
        *outDevice = reinterpret_cast<GfxDevice>(device);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create device: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

const char* vulkan_adapterGetName(GfxAdapter adapter)
{
    if (!adapter) {
        return nullptr;
    }
    auto* adap = reinterpret_cast<gfx::vulkan::Adapter*>(adapter);
    return adap->getName();
}

GfxBackend vulkan_adapterGetBackend(GfxAdapter adapter)
{
    (void)adapter;
    return GFX_BACKEND_VULKAN;
}

void vulkan_deviceDestroy(GfxDevice device)
{
    delete reinterpret_cast<gfx::vulkan::Device*>(device);
}

GfxQueue vulkan_deviceGetQueue(GfxDevice device)
{
    if (!device) {
        return nullptr;
    }
    auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
    return reinterpret_cast<GfxQueue>(dev->getQueue());
}

GfxResult vulkan_deviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader)
{
    if (!device || !descriptor || !outShader) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto* shader = new gfx::vulkan::Shader(dev->handle(), descriptor);
        *outShader = reinterpret_cast<GfxShader>(shader);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create shader: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void vulkan_shaderDestroy(GfxShader shader)
{
    delete reinterpret_cast<gfx::vulkan::Shader*>(shader);
}

GfxResult vulkan_deviceCreateCommandEncoder(GfxDevice device, const char* label, GfxCommandEncoder* outEncoder)
{
    if (!device || !outEncoder) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto* encoder = new gfx::vulkan::CommandEncoder(
            dev->handle(),
            dev->getQueue()->family());
        *outEncoder = reinterpret_cast<GfxCommandEncoder>(encoder);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create command encoder: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    (void)label; // Unused for now
}

void vulkan_commandEncoderDestroy(GfxCommandEncoder commandEncoder)
{
    delete reinterpret_cast<gfx::vulkan::CommandEncoder*>(commandEncoder);
}

void vulkan_commandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder,
    const GfxTextureBarrier* textureBarriers, uint32_t textureBarrierCount)
{
    if (!commandEncoder || !textureBarriers || textureBarrierCount == 0) {
        return;
    }

    auto* encoder = reinterpret_cast<gfx::vulkan::CommandEncoder*>(commandEncoder);
    VkCommandBuffer cmdBuffer = encoder->handle();

    std::vector<VkImageMemoryBarrier> imageBarriers;
    imageBarriers.reserve(textureBarrierCount);

    // Combine pipeline stages from all barriers
    VkPipelineStageFlags srcStage = 0;
    VkPipelineStageFlags dstStage = 0;

    for (uint32_t i = 0; i < textureBarrierCount; ++i) {
        const auto& barrier = textureBarriers[i];
        auto* texture = reinterpret_cast<gfx::vulkan::Texture*>(barrier.texture);

        VkImageMemoryBarrier vkBarrier{};
        vkBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        vkBarrier.image = texture->handle();

        // Determine aspect mask based on texture format
        VkFormat format = gfxFormatToVkFormat(texture->getFormat());
        if (isDepthFormat(format)) {
            vkBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (hasStencilComponent(format)) {
                vkBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        } else {
            vkBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        vkBarrier.subresourceRange.baseMipLevel = barrier.baseMipLevel;
        vkBarrier.subresourceRange.levelCount = barrier.mipLevelCount;
        vkBarrier.subresourceRange.baseArrayLayer = barrier.baseArrayLayer;
        vkBarrier.subresourceRange.layerCount = barrier.arrayLayerCount;

        // Convert layouts
        vkBarrier.oldLayout = gfxLayoutToVkImageLayout(barrier.oldLayout);
        vkBarrier.newLayout = gfxLayoutToVkImageLayout(barrier.newLayout);

        // Convert access masks
        vkBarrier.srcAccessMask = static_cast<VkAccessFlags>(barrier.srcAccessMask);
        vkBarrier.dstAccessMask = static_cast<VkAccessFlags>(barrier.dstAccessMask);

        vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        imageBarriers.push_back(vkBarrier);

        // Combine pipeline stages from all barriers
        srcStage |= static_cast<VkPipelineStageFlags>(barrier.srcStageMask);
        dstStage |= static_cast<VkPipelineStageFlags>(barrier.dstStageMask);

        // Update tracked layout (simplified - tracks whole texture, not per-subresource)
        texture->setLayout(barrier.newLayout);
    }

    vkCmdPipelineBarrier(
        cmdBuffer,
        srcStage,
        dstStage,
        0,
        0, nullptr, // Memory barriers
        0, nullptr, // Buffer barriers
        static_cast<uint32_t>(imageBarriers.size()), imageBarriers.data());
}

void vulkan_commandEncoderFinish(GfxCommandEncoder commandEncoder)
{
    if (!commandEncoder) {
        return;
    }
    auto* encoder = reinterpret_cast<gfx::vulkan::CommandEncoder*>(commandEncoder);
    encoder->finish();
}

GfxResult vulkan_deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface)
{
    if (!device || !descriptor || !outSurface) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto* inst = dev->getAdapter()->getInstance();
        auto* surface = new gfx::vulkan::Surface(inst->handle(), descriptor);
        *outSurface = reinterpret_cast<GfxSurface>(surface);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create surface: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void vulkan_surfaceDestroy(GfxSurface surface)
{
    delete reinterpret_cast<gfx::vulkan::Surface*>(surface);
}

uint32_t vulkan_surfaceGetWidth(GfxSurface surface)
{
    if (!surface) {
        return 0;
    }
    auto* surf = reinterpret_cast<gfx::vulkan::Surface*>(surface);
    return surf->getWidth();
}

uint32_t vulkan_surfaceGetHeight(GfxSurface surface)
{
    if (!surface) {
        return 0;
    }
    auto* surf = reinterpret_cast<gfx::vulkan::Surface*>(surface);
    return surf->getHeight();
}

void vulkan_surfaceResize(GfxSurface surface, uint32_t width, uint32_t height)
{
    if (!surface) {
        return;
    }
    auto* surf = reinterpret_cast<gfx::vulkan::Surface*>(surface);
    surf->resize(width, height);
}

uint32_t vulkan_surfaceGetSupportedFormats(GfxSurface surface, GfxTextureFormat* formats, uint32_t maxFormats)
{
    if (!surface) {
        return 0;
    }

    // For now, return a simple list of commonly supported formats
    // A proper implementation would query the Vulkan surface capabilities
    const GfxTextureFormat supportedFormats[] = {
        GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB,
        GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM,
        GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM_SRGB,
        GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM
    };

    uint32_t count = sizeof(supportedFormats) / sizeof(supportedFormats[0]);

    if (formats && maxFormats > 0) {
        uint32_t copyCount = count < maxFormats ? count : maxFormats;
        for (uint32_t i = 0; i < copyCount; i++) {
            formats[i] = supportedFormats[i];
        }
    }

    return count;
}

uint32_t vulkan_surfaceGetSupportedPresentModes(GfxSurface surface, GfxPresentMode* presentModes, uint32_t maxModes)
{
    if (!surface) {
        return 0;
    }

    // For now, return commonly supported present modes
    // A proper implementation would query the Vulkan surface capabilities
    const GfxPresentMode supportedModes[] = {
        GFX_PRESENT_MODE_FIFO,
        GFX_PRESENT_MODE_IMMEDIATE,
        GFX_PRESENT_MODE_MAILBOX
    };

    uint32_t count = sizeof(supportedModes) / sizeof(supportedModes[0]);

    if (presentModes && maxModes > 0) {
        uint32_t copyCount = count < maxModes ? count : maxModes;
        for (uint32_t i = 0; i < copyCount; i++) {
            presentModes[i] = supportedModes[i];
        }
    }

    return count;
}

GfxPlatformWindowHandle vulkan_surfaceGetPlatformHandle(GfxSurface surface)
{
    if (!surface) {
        return GfxPlatformWindowHandle{};
    }
    auto* surf = reinterpret_cast<gfx::vulkan::Surface*>(surface);
    return surf->getPlatformHandle();
}

GfxResult vulkan_deviceCreateSwapchain(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain)
{
    if (!device || !surface || !descriptor || !outSwapchain) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto* surf = reinterpret_cast<gfx::vulkan::Surface*>(surface);
        auto* swapchain = new gfx::vulkan::Swapchain(
            dev->handle(),
            dev->getAdapter()->handle(),
            surf->handle(),
            descriptor);
        *outSwapchain = reinterpret_cast<GfxSwapchain>(swapchain);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create swapchain: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void vulkan_swapchainDestroy(GfxSwapchain swapchain)
{
    delete reinterpret_cast<gfx::vulkan::Swapchain*>(swapchain);
}

GfxResult vulkan_swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs,
    GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex)
{
    if (!swapchain || !outImageIndex) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* sc = reinterpret_cast<gfx::vulkan::Swapchain*>(swapchain);

    VkSemaphore vkSemaphore = VK_NULL_HANDLE;
    if (imageAvailableSemaphore) {
        auto* sem = reinterpret_cast<gfx::vulkan::Semaphore*>(imageAvailableSemaphore);
        vkSemaphore = sem->handle();
    }

    VkFence vkFence = VK_NULL_HANDLE;
    if (fence) {
        auto* f = reinterpret_cast<gfx::vulkan::Fence*>(fence);
        vkFence = f->handle();
    }

    VkResult result = sc->acquireNextImage(timeoutNs, vkSemaphore, vkFence, outImageIndex);

    switch (result) {
    case VK_SUCCESS:
    case VK_SUBOPTIMAL_KHR:
        return GFX_RESULT_SUCCESS;
    case VK_TIMEOUT:
        return GFX_RESULT_TIMEOUT;
    case VK_NOT_READY:
        return GFX_RESULT_NOT_READY;
    case VK_ERROR_OUT_OF_DATE_KHR:
        return GFX_RESULT_ERROR_OUT_OF_DATE;
    case VK_ERROR_SURFACE_LOST_KHR:
        return GFX_RESULT_ERROR_SURFACE_LOST;
    case VK_ERROR_DEVICE_LOST:
        return GFX_RESULT_ERROR_DEVICE_LOST;
    default:
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxTextureView vulkan_swapchainGetImageView(GfxSwapchain swapchain, uint32_t imageIndex)
{
    if (!swapchain) {
        return nullptr;
    }

    auto* sc = reinterpret_cast<gfx::vulkan::Swapchain*>(swapchain);
    if (imageIndex >= sc->getImageCount()) {
        return nullptr;
    }

    return reinterpret_cast<GfxTextureView>(sc->getTextureView(imageIndex));
}

GfxTextureView vulkan_swapchainGetCurrentTextureView(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return nullptr;
    }

    auto* sc = reinterpret_cast<gfx::vulkan::Swapchain*>(swapchain);
    return reinterpret_cast<GfxTextureView>(sc->getCurrentTextureView());
}

GfxResult vulkan_swapchainPresentWithSync(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo)
{
    if (!swapchain) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* sc = reinterpret_cast<gfx::vulkan::Swapchain*>(swapchain);

    std::vector<VkSemaphore> waitSemaphores;
    if (presentInfo && presentInfo->waitSemaphoreCount > 0) {
        waitSemaphores.reserve(presentInfo->waitSemaphoreCount);
        for (uint32_t i = 0; i < presentInfo->waitSemaphoreCount; i++) {
            auto* sem = reinterpret_cast<gfx::vulkan::Semaphore*>(presentInfo->waitSemaphores[i]);
            if (sem) {
                waitSemaphores.push_back(sem->handle());
            }
        }
    }

    VkResult result = sc->presentWithSync(waitSemaphores);

    switch (result) {
    case VK_SUCCESS:
        return GFX_RESULT_SUCCESS;
    case VK_SUBOPTIMAL_KHR:
        return GFX_RESULT_SUCCESS; // Still success, just suboptimal
    case VK_ERROR_OUT_OF_DATE_KHR:
        return GFX_RESULT_ERROR_OUT_OF_DATE;
    case VK_ERROR_SURFACE_LOST_KHR:
        return GFX_RESULT_ERROR_SURFACE_LOST;
    case VK_ERROR_DEVICE_LOST:
        return GFX_RESULT_ERROR_DEVICE_LOST;
    default:
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult vulkan_swapchainPresent(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto* sc = reinterpret_cast<gfx::vulkan::Swapchain*>(swapchain);
    sc->present();
    return GFX_RESULT_SUCCESS;
}

bool vulkan_swapchainNeedsRecreation(GfxSwapchain swapchain)
{
    (void)swapchain;
    // For now, always return false. Proper implementation would check for VK_ERROR_OUT_OF_DATE_KHR
    return false;
}

void vulkan_swapchainResize(GfxSwapchain swapchain, uint32_t width, uint32_t height)
{
    if (!swapchain) {
        return;
    }
    auto* sc = reinterpret_cast<gfx::vulkan::Swapchain*>(swapchain);
    try {
        sc->resize(width, height);
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to resize swapchain: %s\n", e.what());
    }
}

GfxResult vulkan_deviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer)
{
    if (!device || !descriptor || !outBuffer) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto* buffer = new gfx::vulkan::Buffer(
            dev->handle(),
            dev->getAdapter()->handle(),
            descriptor);
        *outBuffer = reinterpret_cast<GfxBuffer>(buffer);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create buffer: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void vulkan_bufferDestroy(GfxBuffer buffer)
{
    delete reinterpret_cast<gfx::vulkan::Buffer*>(buffer);
}

void* vulkan_bufferMap(GfxBuffer buffer)
{
    if (!buffer) {
        return nullptr;
    }
    auto* buf = reinterpret_cast<gfx::vulkan::Buffer*>(buffer);
    return buf->map();
}

void vulkan_bufferUnmap(GfxBuffer buffer)
{
    if (!buffer) {
        return;
    }
    auto* buf = reinterpret_cast<gfx::vulkan::Buffer*>(buffer);
    buf->unmap();
}

uint64_t vulkan_bufferGetSize(GfxBuffer buffer)
{
    if (!buffer) {
        return 0;
    }
    auto* buf = reinterpret_cast<gfx::vulkan::Buffer*>(buffer);
    return buf->size();
}

GfxBufferUsage vulkan_bufferGetUsage(GfxBuffer buffer)
{
    if (!buffer) {
        return static_cast<GfxBufferUsage>(0);
    }
    auto* buf = reinterpret_cast<gfx::vulkan::Buffer*>(buffer);
    return buf->getUsage();
}

GfxResult vulkan_deviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture)
{
    if (!device || !descriptor || !outTexture) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto* texture = new gfx::vulkan::Texture(
            dev->handle(),
            dev->getAdapter()->handle(),
            descriptor);
        *outTexture = reinterpret_cast<GfxTexture>(texture);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create texture: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void vulkan_textureDestroy(GfxTexture texture)
{
    delete reinterpret_cast<gfx::vulkan::Texture*>(texture);
}

GfxExtent3D vulkan_textureGetSize(GfxTexture texture)
{
    if (!texture) {
        GfxExtent3D empty = { 0, 0, 0 };
        return empty;
    }
    auto* tex = reinterpret_cast<gfx::vulkan::Texture*>(texture);
    return tex->getSize();
}

GfxTextureFormat vulkan_textureGetFormat(GfxTexture texture)
{
    if (!texture) {
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
    auto* tex = reinterpret_cast<gfx::vulkan::Texture*>(texture);
    return tex->getFormat();
}

uint32_t vulkan_textureGetMipLevelCount(GfxTexture texture)
{
    if (!texture) {
        return 0;
    }
    auto* tex = reinterpret_cast<gfx::vulkan::Texture*>(texture);
    return tex->getMipLevelCount();
}

uint32_t vulkan_textureGetSampleCount(GfxTexture texture)
{
    if (!texture) {
        return 0;
    }
    auto* tex = reinterpret_cast<gfx::vulkan::Texture*>(texture);
    return tex->getSampleCount();
}

GfxTextureUsage vulkan_textureGetUsage(GfxTexture texture)
{
    if (!texture) {
        return static_cast<GfxTextureUsage>(0);
    }
    auto* tex = reinterpret_cast<gfx::vulkan::Texture*>(texture);
    return tex->getUsage();
}

GfxTextureLayout vulkan_textureGetLayout(GfxTexture texture)
{
    if (!texture) {
        return GFX_TEXTURE_LAYOUT_UNDEFINED;
    }
    auto* tex = reinterpret_cast<gfx::vulkan::Texture*>(texture);
    return tex->getLayout();
}

GfxResult vulkan_textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView)
{
    if (!texture || !outView) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* tex = reinterpret_cast<gfx::vulkan::Texture*>(texture);
        VkFormat format = descriptor ? gfxFormatToVkFormat(descriptor->format) : VK_FORMAT_UNDEFINED;
        auto* view = new gfx::vulkan::TextureView(
            tex->device(),
            tex->handle(),
            format,
            texture);
        *outView = reinterpret_cast<GfxTextureView>(view);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create texture view: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void vulkan_textureViewDestroy(GfxTextureView textureView)
{
    delete reinterpret_cast<gfx::vulkan::TextureView*>(textureView);
}

GfxTexture vulkan_textureViewGetTexture(GfxTextureView textureView)
{
    if (!textureView) {
        return nullptr;
    }
    auto* view = reinterpret_cast<gfx::vulkan::TextureView*>(textureView);
    return view->getTexture();
}

GfxResult vulkan_deviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler)
{
    if (!device || !descriptor || !outSampler) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto* sampler = new gfx::vulkan::Sampler(dev->handle(), descriptor);
        *outSampler = reinterpret_cast<GfxSampler>(sampler);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create sampler: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void vulkan_samplerDestroy(GfxSampler sampler)
{
    delete reinterpret_cast<gfx::vulkan::Sampler*>(sampler);
}

GfxResult vulkan_deviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline)
{
    if (!device || !descriptor || !outPipeline) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto* pipeline = new gfx::vulkan::RenderPipeline(dev->handle(), descriptor);
        *outPipeline = reinterpret_cast<GfxRenderPipeline>(pipeline);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create render pipeline: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void vulkan_renderPipelineDestroy(GfxRenderPipeline renderPipeline)
{
    delete reinterpret_cast<gfx::vulkan::RenderPipeline*>(renderPipeline);
}

GfxResult vulkan_deviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline)
{
    if (!device || !descriptor || !outPipeline) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto* pipeline = new gfx::vulkan::ComputePipeline(dev->handle(), descriptor);
        *outPipeline = reinterpret_cast<GfxComputePipeline>(pipeline);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create compute pipeline: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void vulkan_computePipelineDestroy(GfxComputePipeline computePipeline)
{
    delete reinterpret_cast<gfx::vulkan::ComputePipeline*>(computePipeline);
}

GfxResult vulkan_deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence)
{
    if (!device || !outFence) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto* fence = new gfx::vulkan::Fence(dev->handle(), descriptor);
        *outFence = reinterpret_cast<GfxFence>(fence);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create fence: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void vulkan_fenceDestroy(GfxFence fence)
{
    delete reinterpret_cast<gfx::vulkan::Fence*>(fence);
}

GfxResult vulkan_fenceGetStatus(GfxFence fence, bool* isSignaled)
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto* f = reinterpret_cast<gfx::vulkan::Fence*>(fence);
    return f->getStatus(isSignaled);
}

GfxResult vulkan_fenceWait(GfxFence fence, uint64_t timeoutNs)
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto* f = reinterpret_cast<gfx::vulkan::Fence*>(fence);
    return f->wait(timeoutNs);
}

void vulkan_fenceReset(GfxFence fence)
{
    if (!fence) {
        return;
    }
    auto* f = reinterpret_cast<gfx::vulkan::Fence*>(fence);
    f->reset();
}

GfxResult vulkan_deviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore)
{
    if (!device || !outSemaphore) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto* semaphore = new gfx::vulkan::Semaphore(dev->handle(), descriptor);
        *outSemaphore = reinterpret_cast<GfxSemaphore>(semaphore);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create semaphore: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void vulkan_semaphoreDestroy(GfxSemaphore semaphore)
{
    delete reinterpret_cast<gfx::vulkan::Semaphore*>(semaphore);
}

GfxSemaphoreType vulkan_semaphoreGetType(GfxSemaphore semaphore)
{
    if (!semaphore) {
        return GFX_SEMAPHORE_TYPE_BINARY;
    }
    auto* s = reinterpret_cast<gfx::vulkan::Semaphore*>(semaphore);
    return s->type();
}

GfxResult vulkan_semaphoreSignal(GfxSemaphore semaphore, uint64_t value)
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto* s = reinterpret_cast<gfx::vulkan::Semaphore*>(semaphore);
    return s->signal(value);
}

GfxResult vulkan_semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs)
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto* s = reinterpret_cast<gfx::vulkan::Semaphore*>(semaphore);
    return s->wait(value, timeoutNs);
}

uint64_t vulkan_semaphoreGetValue(GfxSemaphore semaphore)
{
    if (!semaphore) {
        return 0;
    }
    auto* s = reinterpret_cast<gfx::vulkan::Semaphore*>(semaphore);
    return s->getValue();
}

GfxResult vulkan_queueSubmit(GfxQueue queue, GfxCommandEncoder commandEncoder)
{
    if (!queue || !commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* q = reinterpret_cast<gfx::vulkan::Queue*>(queue);
    auto* encoder = reinterpret_cast<gfx::vulkan::CommandEncoder*>(commandEncoder);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    VkCommandBuffer cmdBuf = encoder->handle();
    submitInfo.pCommandBuffers = &cmdBuf;

    vkQueueSubmit(q->handle(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(q->handle());
    return GFX_RESULT_SUCCESS;
}

GfxResult vulkan_queueSubmitWithSync(GfxQueue queue, const GfxSubmitInfo* submitInfo)
{
    if (!queue || !submitInfo) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* q = reinterpret_cast<gfx::vulkan::Queue*>(queue);

    // Convert command encoders to command buffers
    std::vector<VkCommandBuffer> commandBuffers;
    commandBuffers.reserve(submitInfo->commandEncoderCount);
    for (uint32_t i = 0; i < submitInfo->commandEncoderCount; i++) {
        auto* encoder = reinterpret_cast<gfx::vulkan::CommandEncoder*>(submitInfo->commandEncoders[i]);
        commandBuffers.push_back(encoder->handle());
    }

    // Convert wait semaphores
    std::vector<VkSemaphore> waitSemaphores;
    std::vector<uint64_t> waitValues;
    std::vector<VkPipelineStageFlags> waitStages;
    waitSemaphores.reserve(submitInfo->waitSemaphoreCount);
    waitStages.reserve(submitInfo->waitSemaphoreCount);

    bool hasTimelineWait = false;
    for (uint32_t i = 0; i < submitInfo->waitSemaphoreCount; i++) {
        auto* sem = reinterpret_cast<gfx::vulkan::Semaphore*>(submitInfo->waitSemaphores[i]);
        waitSemaphores.push_back(sem->handle());
        waitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        if (sem->type() == GFX_SEMAPHORE_TYPE_TIMELINE) {
            hasTimelineWait = true;
            uint64_t value = submitInfo->waitValues ? submitInfo->waitValues[i] : 0;
            waitValues.push_back(value);
        } else {
            waitValues.push_back(0);
        }
    }

    // Convert signal semaphores
    std::vector<VkSemaphore> signalSemaphores;
    std::vector<uint64_t> signalValues;
    signalSemaphores.reserve(submitInfo->signalSemaphoreCount);

    bool hasTimelineSignal = false;
    for (uint32_t i = 0; i < submitInfo->signalSemaphoreCount; i++) {
        auto* sem = reinterpret_cast<gfx::vulkan::Semaphore*>(submitInfo->signalSemaphores[i]);
        signalSemaphores.push_back(sem->handle());

        if (sem->type() == GFX_SEMAPHORE_TYPE_TIMELINE) {
            hasTimelineSignal = true;
            uint64_t value = submitInfo->signalValues ? submitInfo->signalValues[i] : 0;
            signalValues.push_back(value);
        } else {
            signalValues.push_back(0);
        }
    }

    // Timeline semaphore info (if needed)
    VkTimelineSemaphoreSubmitInfo timelineInfo{};
    if (hasTimelineWait || hasTimelineSignal) {
        timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        timelineInfo.waitSemaphoreValueCount = static_cast<uint32_t>(waitValues.size());
        timelineInfo.pWaitSemaphoreValues = waitValues.empty() ? nullptr : waitValues.data();
        timelineInfo.signalSemaphoreValueCount = static_cast<uint32_t>(signalValues.size());
        timelineInfo.pSignalSemaphoreValues = signalValues.empty() ? nullptr : signalValues.data();
    }

    // Build submit info
    VkSubmitInfo vkSubmitInfo{};
    vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    if (hasTimelineWait || hasTimelineSignal) {
        vkSubmitInfo.pNext = &timelineInfo;
    }
    vkSubmitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
    vkSubmitInfo.pCommandBuffers = commandBuffers.empty() ? nullptr : commandBuffers.data();
    vkSubmitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    vkSubmitInfo.pWaitSemaphores = waitSemaphores.empty() ? nullptr : waitSemaphores.data();
    vkSubmitInfo.pWaitDstStageMask = waitStages.empty() ? nullptr : waitStages.data();
    vkSubmitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
    vkSubmitInfo.pSignalSemaphores = signalSemaphores.empty() ? nullptr : signalSemaphores.data();

    // Get fence if provided
    VkFence fence = VK_NULL_HANDLE;
    if (submitInfo->signalFence) {
        auto* f = reinterpret_cast<gfx::vulkan::Fence*>(submitInfo->signalFence);
        fence = f->handle();
    }

    VkResult result = vkQueueSubmit(q->handle(), 1, &vkSubmitInfo, fence);
    return (result == VK_SUCCESS) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

// Stub implementations for missing functions to satisfy GfxBackendAPI
static GfxResult vulkan_bufferMapAsync_stub(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer)
{
    (void)offset;
    (void)size;
    if (buffer && outMappedPointer) {
        *outMappedPointer = vulkan_bufferMap(buffer);
        return GFX_RESULT_SUCCESS;
    }
    return GFX_RESULT_ERROR_INVALID_PARAMETER;
}

void vulkan_queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size)
{
    if (!queue || !buffer || !data) {
        return;
    }

    // Map, write, unmap
    void* mapped = vulkan_bufferMap(buffer);
    if (mapped) {
        memcpy(static_cast<char*>(mapped) + offset, data, size);
        vulkan_bufferUnmap(buffer);
    }
}

void vulkan_queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
    const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout)
{
    if (!queue || !texture || !data || !extent || dataSize == 0) {
        return;
    }

    auto* q = reinterpret_cast<gfx::vulkan::Queue*>(queue);
    auto* tex = reinterpret_cast<gfx::vulkan::Texture*>(texture);
    VkDevice device = tex->device();

    // Create staging buffer
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = dataSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create staging buffer for texture upload\n");
        return;
    }

    // Get memory requirements and allocate
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(q->physicalDevice(), &memProperties);

    uint32_t memoryTypeIndex = UINT32_MAX;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            memoryTypeIndex = i;
            break;
        }
    }

    if (memoryTypeIndex == UINT32_MAX) {
        fprintf(stderr, "Failed to find suitable memory type for staging buffer\n");
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        return;
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    result = vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate staging buffer memory\n");
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        return;
    }

    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

    // Copy data to staging buffer
    void* mappedData = nullptr;
    vkMapMemory(device, stagingMemory, 0, dataSize, 0, &mappedData);
    memcpy(mappedData, data, dataSize);
    vkUnmapMemory(device, stagingMemory);

    // Create temporary command buffer
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = q->family();

    VkCommandPool commandPool = VK_NULL_HANDLE;
    vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);

    VkCommandBufferAllocateInfo allocCmdInfo{};
    allocCmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocCmdInfo.commandPool = commandPool;
    allocCmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocCmdInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device, &allocCmdInfo, &commandBuffer);

    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // Transition image to transfer dst optimal
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = gfxLayoutToVkImageLayout(tex->getLayout());
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = tex->handle();
    barrier.subresourceRange.aspectMask = getImageAspectMask(gfxFormatToVkFormat(tex->getFormat()));
    barrier.subresourceRange.baseMipLevel = mipLevel;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Copy buffer to image
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0; // Tightly packed
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = getImageAspectMask(gfxFormatToVkFormat(tex->getFormat()));
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { origin ? origin->x : 0,
        origin ? origin->y : 0,
        origin ? origin->z : 0 };
    region.imageExtent = { extent->width, extent->height, extent->depth };

    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, tex->handle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition image to final layout
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = gfxLayoutToVkImageLayout(finalLayout);
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = static_cast<VkAccessFlags>(gfxGetAccessFlagsForLayout(finalLayout));

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Update tracked layout
    tex->setLayout(finalLayout);

    // End and submit
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(q->handle(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(q->handle());

    // Cleanup
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    (void)bytesPerRow; // Unused - assuming tightly packed data
}

GfxResult vulkan_queueWaitIdle(GfxQueue queue)
{
    if (!queue) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* q = reinterpret_cast<gfx::vulkan::Queue*>(queue);
    vkQueueWaitIdle(q->handle());
    return GFX_RESULT_SUCCESS;
}

// Stub/simple implementations for remaining functions
GfxResult vulkan_deviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout)
{
    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto* layout = new gfx::vulkan::BindGroupLayout(dev->handle(), descriptor);
        *outLayout = reinterpret_cast<GfxBindGroupLayout>(layout);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult vulkan_deviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup)
{
    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto* bindGroup = new gfx::vulkan::BindGroup(dev->handle(), descriptor);
        *outBindGroup = reinterpret_cast<GfxBindGroup>(bindGroup);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void vulkan_bindGroupLayoutDestroy(GfxBindGroupLayout layout)
{
    delete reinterpret_cast<gfx::vulkan::BindGroupLayout*>(layout);
}

void vulkan_bindGroupDestroy(GfxBindGroup bindGroup)
{
    delete reinterpret_cast<gfx::vulkan::BindGroup*>(bindGroup);
}

void vulkan_deviceWaitIdle(GfxDevice device)
{
    if (!device) {
        return;
    }
    auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
    vkDeviceWaitIdle(dev->handle());
}

uint32_t vulkan_swapchainGetWidth(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return 0;
    }
    auto* sc = reinterpret_cast<gfx::vulkan::Swapchain*>(swapchain);
    return sc->getWidth();
}

uint32_t vulkan_swapchainGetHeight(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return 0;
    }
    auto* sc = reinterpret_cast<gfx::vulkan::Swapchain*>(swapchain);
    return sc->getHeight();
}

GfxTextureFormat vulkan_swapchainGetFormat(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
    auto* sc = reinterpret_cast<gfx::vulkan::Swapchain*>(swapchain);
    return static_cast<GfxTextureFormat>(sc->getFormat());
}

uint32_t vulkan_swapchainGetBufferCount(GfxSwapchain swapchain)
{
    if (!swapchain) {
        return 0;
    }
    auto* sc = reinterpret_cast<gfx::vulkan::Swapchain*>(swapchain);
    return sc->getImageCount();
}

GfxResult vulkan_commandEncoderBeginRenderPass(GfxCommandEncoder encoder,
    const GfxTextureView* colorAttachments, uint32_t colorAttachmentCount,
    const GfxColor* clearColors,
    GfxTextureView depthStencilAttachment,
    float depthClearValue, uint32_t stencilClearValue,
    GfxRenderPassEncoder* outRenderPass)
{

    if (!encoder || !outRenderPass) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    if (colorAttachmentCount == 0 || !colorAttachments) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(encoder);
    VkCommandBuffer cmdBuf = enc->handle();

    // Get the color attachment texture view
    auto* colorView = reinterpret_cast<gfx::vulkan::TextureView*>(colorAttachments[0]);

    // Create render pass
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorRefs;

    // Color attachment
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments.push_back(colorAttachment);

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorRefs.push_back(colorRef);

    // Depth attachment if provided
    VkAttachmentReference depthRef{};
    if (depthStencilAttachment) {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = VK_FORMAT_D32_SFLOAT;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.push_back(depthAttachment);

        depthRef.attachment = 1;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
    subpass.pColorAttachments = colorRefs.data();
    if (depthStencilAttachment) {
        subpass.pDepthStencilAttachment = &depthRef;
    }

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkRenderPass renderPass;
    VkResult result = vkCreateRenderPass(enc->device(), &renderPassInfo, nullptr, &renderPass);
    if (result != VK_SUCCESS) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    // Create framebuffer
    std::vector<VkImageView> fbAttachments;
    fbAttachments.push_back(colorView->handle());
    if (depthStencilAttachment) {
        auto* depthView = reinterpret_cast<gfx::vulkan::TextureView*>(depthStencilAttachment);
        fbAttachments.push_back(depthView->handle());
    }

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(fbAttachments.size());
    framebufferInfo.pAttachments = fbAttachments.data();
    framebufferInfo.width = 800; // TODO: get from swapchain
    framebufferInfo.height = 600;
    framebufferInfo.layers = 1;

    VkFramebuffer framebuffer;
    result = vkCreateFramebuffer(enc->device(), &framebufferInfo, nullptr, &framebuffer);
    if (result != VK_SUCCESS) {
        vkDestroyRenderPass(enc->device(), renderPass, nullptr);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    // Track for cleanup
    enc->trackRenderPass(renderPass, framebuffer);

    // Begin render pass
    VkRenderPassBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = renderPass;
    beginInfo.framebuffer = framebuffer;
    beginInfo.renderArea.offset = { 0, 0 };
    beginInfo.renderArea.extent = { 800, 600 };

    std::vector<VkClearValue> clearValues;
    if (clearColors) {
        VkClearValue colorClear{};
        colorClear.color = { { clearColors[0].r, clearColors[0].g, clearColors[0].b, clearColors[0].a } };
        clearValues.push_back(colorClear);
    }
    if (depthStencilAttachment) {
        VkClearValue depthClear{};
        depthClear.depthStencil = { depthClearValue, stencilClearValue };
        clearValues.push_back(depthClear);
    }

    beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    beginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmdBuf, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

    *outRenderPass = reinterpret_cast<GfxRenderPassEncoder>(encoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult vulkan_commandEncoderBeginComputePass(GfxCommandEncoder encoder, const char* label, GfxComputePassEncoder* outComputePass)
{
    if (!encoder || !outComputePass) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    // In Vulkan, compute passes don't require special setup like render passes
    // We just return the command encoder cast to a compute pass encoder
    *outComputePass = reinterpret_cast<GfxComputePassEncoder>(encoder);

    (void)label; // Unused for now
    return GFX_RESULT_SUCCESS;
}

void vulkan_commandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset,
    GfxBuffer destination, uint64_t destinationOffset,
    uint64_t size)
{
    if (!commandEncoder || !source || !destination) {
        return;
    }

    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(commandEncoder);
    auto* srcBuf = reinterpret_cast<gfx::vulkan::Buffer*>(source);
    auto* dstBuf = reinterpret_cast<gfx::vulkan::Buffer*>(destination);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = sourceOffset;
    copyRegion.dstOffset = destinationOffset;
    copyRegion.size = size;

    vkCmdCopyBuffer(enc->handle(), srcBuf->handle(), dstBuf->handle(), 1, &copyRegion);
}

void vulkan_commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset, uint32_t bytesPerRow,
    GfxTexture destination, const GfxOrigin3D* origin,
    const GfxExtent3D* extent, uint32_t mipLevel, GfxTextureLayout finalLayout)
{
    if (!commandEncoder || !source || !destination || !origin || !extent) {
        return;
    }

    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(commandEncoder);
    auto* srcBuf = reinterpret_cast<gfx::vulkan::Buffer*>(source);
    auto* dstTex = reinterpret_cast<gfx::vulkan::Texture*>(destination);

    VkCommandBuffer cmdBuf = enc->handle();

    // Transition image layout to transfer dst optimal
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = gfxLayoutToVkImageLayout(dstTex->getLayout());
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = dstTex->handle();
    barrier.subresourceRange.aspectMask = getImageAspectMask(gfxFormatToVkFormat(dstTex->getFormat()));
    barrier.subresourceRange.baseMipLevel = mipLevel;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Copy buffer to image
    VkBufferImageCopy region{};
    region.bufferOffset = sourceOffset;
    region.bufferRowLength = 0; // Tightly packed
    region.bufferImageHeight = 0; // Tightly packed
    region.imageSubresource.aspectMask = getImageAspectMask(gfxFormatToVkFormat(dstTex->getFormat()));
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { origin->x, origin->y, origin->z };
    region.imageExtent = { extent->width, extent->height, extent->depth };

    vkCmdCopyBufferToImage(cmdBuf, srcBuf->handle(), dstTex->handle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition image layout to final layout
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = gfxLayoutToVkImageLayout(finalLayout);
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = static_cast<VkAccessFlags>(gfxGetAccessFlagsForLayout(finalLayout));

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Update tracked layout
    dstTex->setLayout(finalLayout);

    (void)bytesPerRow; // Unused - assuming tightly packed data
}

void vulkan_commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* origin, uint32_t mipLevel,
    GfxBuffer destination, uint64_t destinationOffset, uint32_t bytesPerRow,
    const GfxExtent3D* extent, GfxTextureLayout finalLayout)
{
    if (!commandEncoder || !source || !destination || !origin || !extent) {
        return;
    }

    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(commandEncoder);
    auto* srcTex = reinterpret_cast<gfx::vulkan::Texture*>(source);
    auto* dstBuf = reinterpret_cast<gfx::vulkan::Buffer*>(destination);

    VkCommandBuffer cmdBuf = enc->handle();

    // Transition image layout to transfer src optimal
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = gfxLayoutToVkImageLayout(srcTex->getLayout());
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = srcTex->handle();
    barrier.subresourceRange.aspectMask = getImageAspectMask(gfxFormatToVkFormat(srcTex->getFormat()));
    barrier.subresourceRange.baseMipLevel = mipLevel;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Copy image to buffer
    VkBufferImageCopy region{};
    region.bufferOffset = destinationOffset;
    region.bufferRowLength = 0; // Tightly packed
    region.bufferImageHeight = 0; // Tightly packed
    region.imageSubresource.aspectMask = getImageAspectMask(gfxFormatToVkFormat(srcTex->getFormat()));
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { origin->x, origin->y, origin->z };
    region.imageExtent = { extent->width, extent->height, extent->depth };

    vkCmdCopyImageToBuffer(cmdBuf, srcTex->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstBuf->handle(), 1, &region);

    // Transition image layout to final layout
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = gfxLayoutToVkImageLayout(finalLayout);
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = static_cast<VkAccessFlags>(gfxGetAccessFlagsForLayout(finalLayout));

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Update tracked layout
    srcTex->setLayout(finalLayout);

    (void)bytesPerRow; // Unused - assuming tightly packed data
}

void vulkan_commandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* sourceOrigin, uint32_t sourceMipLevel,
    GfxTexture destination, const GfxOrigin3D* destinationOrigin, uint32_t destinationMipLevel,
    const GfxExtent3D* extent, GfxTextureLayout srcFinalLayout, GfxTextureLayout dstFinalLayout)
{
    if (!commandEncoder || !source || !destination || !sourceOrigin || !destinationOrigin || !extent) {
        return;
    }

    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(commandEncoder);
    auto* srcTex = reinterpret_cast<gfx::vulkan::Texture*>(source);
    auto* dstTex = reinterpret_cast<gfx::vulkan::Texture*>(destination);

    VkCommandBuffer cmdBuf = enc->handle();

    // Transition source image to transfer src optimal
    VkImageMemoryBarrier barriers[2] = {};
    barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barriers[0].oldLayout = gfxLayoutToVkImageLayout(srcTex->getLayout());
    barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].image = srcTex->handle();
    barriers[0].subresourceRange.aspectMask = getImageAspectMask(gfxFormatToVkFormat(srcTex->getFormat()));
    barriers[0].subresourceRange.baseMipLevel = sourceMipLevel;
    barriers[0].subresourceRange.levelCount = 1;
    barriers[0].subresourceRange.baseArrayLayer = 0;
    barriers[0].subresourceRange.layerCount = 1;
    barriers[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    // Transition destination image to transfer dst optimal
    barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barriers[1].oldLayout = gfxLayoutToVkImageLayout(dstTex->getLayout());
    barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[1].image = dstTex->handle();
    barriers[1].subresourceRange.aspectMask = getImageAspectMask(gfxFormatToVkFormat(dstTex->getFormat()));
    barriers[1].subresourceRange.baseMipLevel = destinationMipLevel;
    barriers[1].subresourceRange.levelCount = 1;
    barriers[1].subresourceRange.baseArrayLayer = 0;
    barriers[1].subresourceRange.layerCount = 1;
    barriers[1].srcAccessMask = 0;
    barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmdBuf,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 2, barriers);

    // Copy image to image
    VkImageCopy region{};
    region.srcSubresource.aspectMask = getImageAspectMask(gfxFormatToVkFormat(srcTex->getFormat()));
    region.srcSubresource.mipLevel = sourceMipLevel;
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount = 1;
    region.srcOffset = { sourceOrigin->x, sourceOrigin->y, sourceOrigin->z };
    region.dstSubresource.aspectMask = getImageAspectMask(gfxFormatToVkFormat(dstTex->getFormat()));
    region.dstSubresource.mipLevel = destinationMipLevel;
    region.dstSubresource.baseArrayLayer = 0;
    region.dstSubresource.layerCount = 1;
    region.dstOffset = { destinationOrigin->x, destinationOrigin->y, destinationOrigin->z };
    region.extent = { extent->width, extent->height, extent->depth };

    vkCmdCopyImage(cmdBuf, srcTex->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstTex->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition images to final layouts
    barriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[0].newLayout = gfxLayoutToVkImageLayout(srcFinalLayout);
    barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barriers[0].dstAccessMask = static_cast<VkAccessFlags>(gfxGetAccessFlagsForLayout(srcFinalLayout));

    barriers[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[1].newLayout = gfxLayoutToVkImageLayout(dstFinalLayout);
    barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[1].dstAccessMask = static_cast<VkAccessFlags>(gfxGetAccessFlagsForLayout(dstFinalLayout));

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 2, barriers);

    // Update tracked layouts
    srcTex->setLayout(srcFinalLayout);
    dstTex->setLayout(dstFinalLayout);
}

void vulkan_renderPassEncoderSetPipeline(GfxRenderPassEncoder encoder, GfxRenderPipeline pipeline)
{
    if (!encoder || !pipeline) {
        return;
    }
    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(encoder);
    auto* pipe = reinterpret_cast<gfx::vulkan::RenderPipeline*>(pipeline);

    VkCommandBuffer cmdBuf = enc->handle();
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->handle());
    enc->setCurrentPipelineLayout(pipe->layout());
}

void vulkan_renderPassEncoderSetBindGroup(GfxRenderPassEncoder encoder, uint32_t index, GfxBindGroup bindGroup)
{
    if (!encoder || !bindGroup) {
        return;
    }
    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(encoder);
    auto* bg = reinterpret_cast<gfx::vulkan::BindGroup*>(bindGroup);

    VkCommandBuffer cmdBuf = enc->handle();
    VkPipelineLayout layout = enc->currentPipelineLayout();

    if (layout != VK_NULL_HANDLE) {
        VkDescriptorSet set = bg->handle();
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, index, 1, &set, 0, nullptr);
    }
}

void vulkan_renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder encoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size)
{
    if (!encoder || !buffer) {
        return;
    }
    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(encoder);
    auto* buf = reinterpret_cast<gfx::vulkan::Buffer*>(buffer);

    VkCommandBuffer cmdBuf = enc->handle();
    VkBuffer vkBuf = buf->handle();
    VkDeviceSize offsets[] = { offset };
    vkCmdBindVertexBuffers(cmdBuf, slot, 1, &vkBuf, offsets);
    (void)size;
}

void vulkan_renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder encoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size)
{
    if (!encoder || !buffer) {
        return;
    }
    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(encoder);
    auto* buf = reinterpret_cast<gfx::vulkan::Buffer*>(buffer);

    VkCommandBuffer cmdBuf = enc->handle();
    VkIndexType indexType = (format == GFX_INDEX_FORMAT_UINT16) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
    vkCmdBindIndexBuffer(cmdBuf, buf->handle(), offset, indexType);
    (void)size;
}

void vulkan_renderPassEncoderSetViewport(GfxRenderPassEncoder encoder, const GfxViewport* viewport)
{
    if (!encoder || !viewport) {
        return;
    }
    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(encoder);

    VkViewport vkViewport{};
    vkViewport.x = viewport->x;
    vkViewport.y = viewport->y;
    vkViewport.width = viewport->width;
    vkViewport.height = viewport->height;
    vkViewport.minDepth = viewport->minDepth;
    vkViewport.maxDepth = viewport->maxDepth;
    vkCmdSetViewport(enc->handle(), 0, 1, &vkViewport);
}

void vulkan_renderPassEncoderSetScissorRect(GfxRenderPassEncoder encoder, const GfxScissorRect* scissor)
{
    if (!encoder || !scissor) {
        return;
    }
    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(encoder);

    VkRect2D vkScissor{};
    vkScissor.offset = { scissor->x, scissor->y };
    vkScissor.extent = { scissor->width, scissor->height };
    vkCmdSetScissor(enc->handle(), 0, 1, &vkScissor);
}

void vulkan_renderPassEncoderDraw(GfxRenderPassEncoder encoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    if (!encoder) {
        return;
    }
    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(encoder);

    // Set viewport and scissor (required for dynamic state)
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = 800.0f;
    viewport.height = 600.0f;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(enc->handle(), 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { 800, 600 };
    vkCmdSetScissor(enc->handle(), 0, 1, &scissor);

    VkCommandBuffer cmdBuf = enc->handle();
    vkCmdDraw(cmdBuf, vertexCount, instanceCount, firstVertex, firstInstance);
}

void vulkan_renderPassEncoderDrawIndexed(GfxRenderPassEncoder encoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance)
{
    if (!encoder) {
        return;
    }
    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(encoder);

    // Set viewport and scissor (required for dynamic state)
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = 800.0f;
    viewport.height = 600.0f;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(enc->handle(), 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { 800, 600 };
    vkCmdSetScissor(enc->handle(), 0, 1, &scissor);

    VkCommandBuffer cmdBuf = enc->handle();
    vkCmdDrawIndexed(cmdBuf, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}

void vulkan_renderPassEncoderEnd(GfxRenderPassEncoder encoder)
{
    if (!encoder) {
        return;
    }
    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(encoder);
    vkCmdEndRenderPass(enc->handle());
}

void vulkan_renderPassEncoderDestroy(GfxRenderPassEncoder encoder)
{
    (void)encoder;
    // Render pass encoder is just a view of command encoder, no separate cleanup
}

void vulkan_computePassEncoderSetPipeline(GfxComputePassEncoder encoder, GfxComputePipeline pipeline)
{
    if (!encoder || !pipeline) {
        return;
    }

    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(encoder);
    auto* pipe = reinterpret_cast<gfx::vulkan::ComputePipeline*>(pipeline);

    VkCommandBuffer cmdBuf = enc->handle();
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipe->handle());
    enc->setCurrentPipelineLayout(pipe->layout());
}

void vulkan_computePassEncoderSetBindGroup(GfxComputePassEncoder encoder, uint32_t index, GfxBindGroup bindGroup)
{
    if (!encoder || !bindGroup) {
        return;
    }

    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(encoder);
    auto* bg = reinterpret_cast<gfx::vulkan::BindGroup*>(bindGroup);

    VkCommandBuffer cmdBuf = enc->handle();
    VkPipelineLayout layout = enc->currentPipelineLayout();

    if (layout != VK_NULL_HANDLE) {
        VkDescriptorSet set = bg->handle();
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, layout, index, 1, &set, 0, nullptr);
    }
}

void vulkan_computePassEncoderDispatchWorkgroups(GfxComputePassEncoder encoder,
    uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ)
{
    if (!encoder) {
        return;
    }

    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(encoder);
    VkCommandBuffer cmdBuf = enc->handle();
    vkCmdDispatch(cmdBuf, workgroupCountX, workgroupCountY, workgroupCountZ);
}

void vulkan_computePassEncoderEnd(GfxComputePassEncoder encoder)
{
    // No special cleanup needed for compute pass in Vulkan
    // The command encoder handles all cleanup
    (void)encoder;
}

void vulkan_computePassEncoderDestroy(GfxComputePassEncoder encoder)
{
    // Compute pass encoder is just a view of command encoder, no separate cleanup
    (void)encoder;
}

// Complete backend API table - using positional initialization to avoid designated initializer issues
static const GfxBackendAPI vulkanBackendApi = {
    .createInstance = vulkan_createInstance,
    .instanceDestroy = vulkan_instanceDestroy,
    .instanceRequestAdapter = vulkan_instanceRequestAdapter,
    .instanceEnumerateAdapters = vulkan_instanceEnumerateAdapters,
    .adapterDestroy = vulkan_adapterDestroy,
    .adapterCreateDevice = vulkan_adapterCreateDevice,
    .adapterGetName = vulkan_adapterGetName,
    .adapterGetBackend = vulkan_adapterGetBackend,
    .deviceDestroy = vulkan_deviceDestroy,
    .deviceGetQueue = vulkan_deviceGetQueue,
    .deviceCreateSurface = vulkan_deviceCreateSurface,
    .deviceCreateSwapchain = vulkan_deviceCreateSwapchain,
    .deviceCreateBuffer = vulkan_deviceCreateBuffer,
    .deviceCreateTexture = vulkan_deviceCreateTexture,
    .deviceCreateSampler = vulkan_deviceCreateSampler,
    .deviceCreateShader = vulkan_deviceCreateShader,
    .deviceCreateBindGroupLayout = vulkan_deviceCreateBindGroupLayout,
    .deviceCreateBindGroup = vulkan_deviceCreateBindGroup,
    .deviceCreateRenderPipeline = vulkan_deviceCreateRenderPipeline,
    .deviceCreateComputePipeline = vulkan_deviceCreateComputePipeline,
    .deviceCreateCommandEncoder = vulkan_deviceCreateCommandEncoder,
    .deviceCreateFence = vulkan_deviceCreateFence,
    .deviceCreateSemaphore = vulkan_deviceCreateSemaphore,
    .deviceWaitIdle = vulkan_deviceWaitIdle,
    .surfaceDestroy = vulkan_surfaceDestroy,
    .surfaceGetWidth = vulkan_surfaceGetWidth,
    .surfaceGetHeight = vulkan_surfaceGetHeight,
    .surfaceResize = vulkan_surfaceResize,
    .surfaceGetSupportedFormats = vulkan_surfaceGetSupportedFormats,
    .surfaceGetSupportedPresentModes = vulkan_surfaceGetSupportedPresentModes,
    .surfaceGetPlatformHandle = vulkan_surfaceGetPlatformHandle,
    .swapchainDestroy = vulkan_swapchainDestroy,
    .swapchainGetWidth = vulkan_swapchainGetWidth,
    .swapchainGetHeight = vulkan_swapchainGetHeight,
    .swapchainGetFormat = vulkan_swapchainGetFormat,
    .swapchainGetBufferCount = vulkan_swapchainGetBufferCount,
    .swapchainAcquireNextImage = vulkan_swapchainAcquireNextImage,
    .swapchainGetImageView = vulkan_swapchainGetImageView,
    .swapchainGetCurrentTextureView = vulkan_swapchainGetCurrentTextureView,
    .swapchainPresentWithSync = vulkan_swapchainPresentWithSync,
    .swapchainPresent = vulkan_swapchainPresent,
    .swapchainResize = vulkan_swapchainResize,
    .swapchainNeedsRecreation = vulkan_swapchainNeedsRecreation,
    .bufferDestroy = vulkan_bufferDestroy,
    .bufferGetSize = vulkan_bufferGetSize,
    .bufferGetUsage = vulkan_bufferGetUsage,
    .bufferMapAsync = vulkan_bufferMapAsync_stub,
    .bufferUnmap = vulkan_bufferUnmap,
    .textureDestroy = vulkan_textureDestroy,
    .textureGetSize = vulkan_textureGetSize,
    .textureGetFormat = vulkan_textureGetFormat,
    .textureGetMipLevelCount = vulkan_textureGetMipLevelCount,
    .textureGetSampleCount = vulkan_textureGetSampleCount,
    .textureGetUsage = vulkan_textureGetUsage,
    .textureGetLayout = vulkan_textureGetLayout,
    .textureCreateView = vulkan_textureCreateView,
    .textureViewDestroy = vulkan_textureViewDestroy,
    .textureViewGetTexture = vulkan_textureViewGetTexture,
    .samplerDestroy = vulkan_samplerDestroy,
    .shaderDestroy = vulkan_shaderDestroy,
    .bindGroupLayoutDestroy = vulkan_bindGroupLayoutDestroy,
    .bindGroupDestroy = vulkan_bindGroupDestroy,
    .renderPipelineDestroy = vulkan_renderPipelineDestroy,
    .computePipelineDestroy = vulkan_computePipelineDestroy,
    .queueSubmit = vulkan_queueSubmit,
    .queueSubmitWithSync = vulkan_queueSubmitWithSync,
    .queueWriteBuffer = vulkan_queueWriteBuffer,
    .queueWriteTexture = vulkan_queueWriteTexture,
    .queueWaitIdle = vulkan_queueWaitIdle,
    .commandEncoderDestroy = vulkan_commandEncoderDestroy,
    .commandEncoderBeginRenderPass = vulkan_commandEncoderBeginRenderPass,
    .commandEncoderBeginComputePass = vulkan_commandEncoderBeginComputePass,
    .commandEncoderCopyBufferToBuffer = vulkan_commandEncoderCopyBufferToBuffer,
    .commandEncoderCopyBufferToTexture = vulkan_commandEncoderCopyBufferToTexture,
    .commandEncoderCopyTextureToBuffer = vulkan_commandEncoderCopyTextureToBuffer,
    .commandEncoderCopyTextureToTexture = vulkan_commandEncoderCopyTextureToTexture,
    .commandEncoderPipelineBarrier = vulkan_commandEncoderPipelineBarrier,
    .commandEncoderFinish = vulkan_commandEncoderFinish,
    .renderPassEncoderDestroy = vulkan_renderPassEncoderDestroy,
    .renderPassEncoderSetPipeline = vulkan_renderPassEncoderSetPipeline,
    .renderPassEncoderSetBindGroup = vulkan_renderPassEncoderSetBindGroup,
    .renderPassEncoderSetVertexBuffer = vulkan_renderPassEncoderSetVertexBuffer,
    .renderPassEncoderSetIndexBuffer = vulkan_renderPassEncoderSetIndexBuffer,
    .renderPassEncoderSetViewport = vulkan_renderPassEncoderSetViewport,
    .renderPassEncoderSetScissorRect = vulkan_renderPassEncoderSetScissorRect,
    .renderPassEncoderDraw = vulkan_renderPassEncoderDraw,
    .renderPassEncoderDrawIndexed = vulkan_renderPassEncoderDrawIndexed,
    .renderPassEncoderEnd = vulkan_renderPassEncoderEnd,
    .computePassEncoderDestroy = vulkan_computePassEncoderDestroy,
    .computePassEncoderSetPipeline = vulkan_computePassEncoderSetPipeline,
    .computePassEncoderSetBindGroup = vulkan_computePassEncoderSetBindGroup,
    .computePassEncoderDispatchWorkgroups = vulkan_computePassEncoderDispatchWorkgroups,
    .computePassEncoderEnd = vulkan_computePassEncoderEnd,
    .fenceDestroy = vulkan_fenceDestroy,
    .fenceGetStatus = vulkan_fenceGetStatus,
    .fenceWait = vulkan_fenceWait,
    .fenceReset = vulkan_fenceReset,
    .semaphoreDestroy = vulkan_semaphoreDestroy,
    .semaphoreGetType = vulkan_semaphoreGetType,
    .semaphoreSignal = vulkan_semaphoreSignal,
    .semaphoreWait = vulkan_semaphoreWait,
    .semaphoreGetValue = vulkan_semaphoreGetValue,
};

const GfxBackendAPI* gfxGetVulkanBackendNew(void)
{
    return &vulkanBackendApi;
}

} // extern "C"
