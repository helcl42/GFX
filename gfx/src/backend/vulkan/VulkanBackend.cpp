
#include "common/VulkanCommon.h"

#include "VulkanBackend.h"
#include "converter/GfxVulkanConverter.h"
#include "entity/Entities.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace gfx::vulkan {

// Instance functions
GfxResult VulkanBackend::createInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance) const
{
    if (!descriptor || !outInstance) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto createInfo = gfx::convertor::gfxDescriptorToInstanceCreateInfo(descriptor);
        auto* instance = new gfx::vulkan::Instance(createInfo);
        *outInstance = reinterpret_cast<GfxInstance>(instance);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create instance: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void VulkanBackend::instanceDestroy(GfxInstance instance) const
{
    delete reinterpret_cast<gfx::vulkan::Instance*>(instance);
}

void VulkanBackend::instanceSetDebugCallback(GfxInstance instance, GfxDebugCallback callback, void* userData) const
{
    if (!instance) {
        return;
    }
    auto* inst = reinterpret_cast<gfx::vulkan::Instance*>(instance);

    // Create adapter callback that converts internal enums to Gfx API enums
    if (callback) {
        // Create a struct to hold both the callback and userData
        struct CallbackData {
            GfxDebugCallback callback;
            void* userData;
        };

        auto* callbackData = new CallbackData{ callback, userData };

        // Create a static callback that uses the adapter
        auto staticCallback = +[](gfx::vulkan::DebugMessageSeverity severity, gfx::vulkan::DebugMessageType type, const char* message, void* dataPtr) {
            auto* data = static_cast<CallbackData*>(dataPtr);

            // Convert internal enum to GfxDebugMessageSeverity
            GfxDebugMessageSeverity gfxSeverity;
            switch (severity) {
            case gfx::vulkan::DebugMessageSeverity::Verbose:
                gfxSeverity = GFX_DEBUG_MESSAGE_SEVERITY_VERBOSE;
                break;
            case gfx::vulkan::DebugMessageSeverity::Info:
                gfxSeverity = GFX_DEBUG_MESSAGE_SEVERITY_INFO;
                break;
            case gfx::vulkan::DebugMessageSeverity::Warning:
                gfxSeverity = GFX_DEBUG_MESSAGE_SEVERITY_WARNING;
                break;
            case gfx::vulkan::DebugMessageSeverity::Error:
                gfxSeverity = GFX_DEBUG_MESSAGE_SEVERITY_ERROR;
                break;
            }

            // Convert internal enum to GfxDebugMessageType
            GfxDebugMessageType gfxType;
            switch (type) {
            case gfx::vulkan::DebugMessageType::General:
                gfxType = GFX_DEBUG_MESSAGE_TYPE_GENERAL;
                break;
            case gfx::vulkan::DebugMessageType::Validation:
                gfxType = GFX_DEBUG_MESSAGE_TYPE_VALIDATION;
                break;
            case gfx::vulkan::DebugMessageType::Performance:
                gfxType = GFX_DEBUG_MESSAGE_TYPE_PERFORMANCE;
                break;
            }

            data->callback(gfxSeverity, gfxType, message, data->userData);
        };

        inst->setDebugCallback(staticCallback, callbackData);
    } else {
        inst->setDebugCallback(nullptr, nullptr);
    }
}

GfxResult VulkanBackend::instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter) const
{
    if (!instance || !outAdapter) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* inst = reinterpret_cast<gfx::vulkan::Instance*>(instance);

    try {
        auto createInfo = gfx::convertor::gfxDescriptorToAdapterCreateInfo(descriptor);
        auto* adapter = new gfx::vulkan::Adapter(inst, createInfo);
        *outAdapter = reinterpret_cast<GfxAdapter>(adapter);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to request adapter: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

uint32_t VulkanBackend::instanceEnumerateAdapters(GfxInstance instance, GfxAdapter* adapters, uint32_t maxAdapters) const
{
    if (!instance || !adapters || maxAdapters == 0) {
        return 0;
    }

    auto* inst = reinterpret_cast<gfx::vulkan::Instance*>(instance);

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(inst->handle(), &deviceCount, nullptr);

    if (deviceCount == 0) {
        return 0;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(inst->handle(), &deviceCount, devices.data());

    // Create an adapter for each physical device
    uint32_t count = std::min(deviceCount, maxAdapters);
    for (uint32_t i = 0; i < count; ++i) {
        adapters[i] = reinterpret_cast<GfxAdapter>(new gfx::vulkan::Adapter(inst, devices[i]));
    }

    return count;
}

// Adapter functions
void VulkanBackend::adapterDestroy(GfxAdapter adapter) const
{
    delete reinterpret_cast<gfx::vulkan::Adapter*>(adapter);
}

GfxResult VulkanBackend::adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice) const
{
    if (!adapter || !outDevice) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        // Device doesn't own the adapter - just keeps a pointer
        auto* adapterPtr = reinterpret_cast<gfx::vulkan::Adapter*>(adapter);
        auto createInfo = gfx::convertor::gfxDescriptorToDeviceCreateInfo(descriptor);
        auto* device = new gfx::vulkan::Device(adapterPtr, createInfo);
        *outDevice = reinterpret_cast<GfxDevice>(device);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create device: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

const char* VulkanBackend::adapterGetName(GfxAdapter adapter) const
{
    if (!adapter) {
        return nullptr;
    }
    auto* adap = reinterpret_cast<gfx::vulkan::Adapter*>(adapter);
    return adap->getName();
}

GfxBackend VulkanBackend::adapterGetBackend(GfxAdapter adapter) const
{
    (void)adapter;
    return GFX_BACKEND_VULKAN;
}

void VulkanBackend::adapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits) const
{
    if (!adapter || !outLimits) {
        return;
    }
    auto* adap = reinterpret_cast<gfx::vulkan::Adapter*>(adapter);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(adap->handle(), &properties);

    outLimits->minUniformBufferOffsetAlignment = static_cast<uint32_t>(properties.limits.minUniformBufferOffsetAlignment);
    outLimits->minStorageBufferOffsetAlignment = static_cast<uint32_t>(properties.limits.minStorageBufferOffsetAlignment);
    outLimits->maxUniformBufferBindingSize = properties.limits.maxUniformBufferRange;
    outLimits->maxStorageBufferBindingSize = properties.limits.maxStorageBufferRange;
    outLimits->maxBufferSize = UINT64_MAX; // Vulkan doesn't have a single max, use practical limit
    outLimits->maxTextureDimension1D = properties.limits.maxImageDimension1D;
    outLimits->maxTextureDimension2D = properties.limits.maxImageDimension2D;
    outLimits->maxTextureDimension3D = properties.limits.maxImageDimension3D;
    outLimits->maxTextureArrayLayers = properties.limits.maxImageArrayLayers;
}

// Device functions
void VulkanBackend::deviceDestroy(GfxDevice device) const
{
    delete reinterpret_cast<gfx::vulkan::Device*>(device);
}

GfxQueue VulkanBackend::deviceGetQueue(GfxDevice device) const
{
    if (!device) {
        return nullptr;
    }
    auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
    return reinterpret_cast<GfxQueue>(dev->getQueue());
}

GfxResult VulkanBackend::deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface) const
{
    if (!device || !descriptor || !outSurface) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto* inst = dev->getAdapter()->getInstance();
        auto createInfo = gfx::convertor::gfxDescriptorToSurfaceCreateInfo(descriptor);
        auto* surface = new gfx::vulkan::Surface(inst->handle(), dev->getAdapter()->handle(), createInfo);
        *outSurface = reinterpret_cast<GfxSurface>(surface);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create surface: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateSwapchain(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain) const
{
    if (!device || !surface || !descriptor || !outSwapchain) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto* surf = reinterpret_cast<gfx::vulkan::Surface*>(surface);
        auto createInfo = gfx::convertor::gfxDescriptorToSwapchainCreateInfo(
            surf->handle(),
            dev->getAdapter()->handle(),
            dev->getAdapter()->getGraphicsQueueFamily(),
            descriptor);
        auto* swapchain = new gfx::vulkan::Swapchain(dev->handle(), dev->getAdapter()->handle(), createInfo);
        *outSwapchain = reinterpret_cast<GfxSwapchain>(swapchain);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create swapchain: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer) const
{
    if (!device || !descriptor || !outBuffer) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto createInfo = gfx::convertor::gfxDescriptorToBufferCreateInfo(descriptor);
        auto* buffer = new gfx::vulkan::Buffer(
            dev->handle(),
            dev->getAdapter()->handle(),
            createInfo);
        *outBuffer = reinterpret_cast<GfxBuffer>(buffer);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create buffer: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture) const
{
    if (!device || !descriptor || !outTexture) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto createInfo = gfx::convertor::gfxDescriptorToTextureCreateInfo(descriptor);
        auto* texture = new gfx::vulkan::Texture(
            dev->handle(),
            dev->getAdapter()->handle(),
            createInfo);
        *outTexture = reinterpret_cast<GfxTexture>(texture);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create texture: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler) const
{
    if (!device || !descriptor || !outSampler) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto createInfo = gfx::convertor::gfxDescriptorToSamplerCreateInfo(descriptor);
        auto* sampler = new gfx::vulkan::Sampler(dev->handle(), createInfo);
        *outSampler = reinterpret_cast<GfxSampler>(sampler);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create sampler: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader) const
{
    if (!device || !descriptor || !outShader) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto createInfo = gfx::convertor::gfxDescriptorToShaderCreateInfo(descriptor);
        auto* shader = new gfx::vulkan::Shader(dev->handle(), createInfo);
        *outShader = reinterpret_cast<GfxShader>(shader);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create shader: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout) const
{
    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto createInfo = gfx::convertor::gfxDescriptorToBindGroupLayoutCreateInfo(descriptor);
        auto* layout = new gfx::vulkan::BindGroupLayout(dev->handle(), createInfo);
        *outLayout = reinterpret_cast<GfxBindGroupLayout>(layout);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup) const
{
    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto createInfo = gfx::convertor::gfxDescriptorToBindGroupCreateInfo(descriptor);
        auto* bindGroup = new gfx::vulkan::BindGroup(dev->handle(), createInfo);
        *outBindGroup = reinterpret_cast<GfxBindGroup>(bindGroup);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline) const
{
    if (!device || !descriptor || !outPipeline) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto createInfo = gfx::convertor::gfxDescriptorToRenderPipelineCreateInfo(descriptor);
        auto* pipeline = new gfx::vulkan::RenderPipeline(dev->handle(), createInfo);
        *outPipeline = reinterpret_cast<GfxRenderPipeline>(pipeline);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create render pipeline: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline) const
{
    if (!device || !descriptor || !outPipeline) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto createInfo = gfx::convertor::gfxDescriptorToComputePipelineCreateInfo(descriptor);
        auto* pipeline = new gfx::vulkan::ComputePipeline(dev->handle(), createInfo);
        *outPipeline = reinterpret_cast<GfxComputePipeline>(pipeline);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create compute pipeline: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateCommandEncoder(GfxDevice device, const GfxCommandEncoderDescriptor* descriptor, GfxCommandEncoder* outEncoder) const
{
    if (!device || !descriptor || !outEncoder) {
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

    (void)descriptor->label; // Unused for now
}

GfxResult VulkanBackend::deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence) const
{
    if (!device || !outFence) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto createInfo = gfx::convertor::gfxDescriptorToFenceCreateInfo(descriptor);
        auto* fence = new gfx::vulkan::Fence(dev->handle(), createInfo);
        *outFence = reinterpret_cast<GfxFence>(fence);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create fence: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore) const
{
    if (!device || !outSemaphore) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
        auto createInfo = gfx::convertor::gfxDescriptorToSemaphoreCreateInfo(descriptor);
        auto* semaphore = new gfx::vulkan::Semaphore(dev->handle(), createInfo);
        *outSemaphore = reinterpret_cast<GfxSemaphore>(semaphore);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create semaphore: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void VulkanBackend::deviceWaitIdle(GfxDevice device) const
{
    if (!device) {
        return;
    }
    auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
    vkDeviceWaitIdle(dev->handle());
}

void VulkanBackend::devicePoll(GfxDevice device) const
{
    // Vulkan doesn't need polling - commands execute immediately
    (void)device;
}

void VulkanBackend::deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits) const
{
    if (!device || !outLimits) {
        return;
    }
    auto* dev = reinterpret_cast<gfx::vulkan::Device*>(device);
    auto* adapter = dev->getAdapter();

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(adapter->handle(), &properties);

    outLimits->minUniformBufferOffsetAlignment = properties.limits.minUniformBufferOffsetAlignment;
    outLimits->minStorageBufferOffsetAlignment = properties.limits.minStorageBufferOffsetAlignment;
    outLimits->maxUniformBufferBindingSize = properties.limits.maxUniformBufferRange;
    outLimits->maxStorageBufferBindingSize = properties.limits.maxStorageBufferRange;
    outLimits->maxBufferSize = UINT64_MAX; // Vulkan doesn't expose a direct limit
    outLimits->maxTextureDimension1D = properties.limits.maxImageDimension1D;
    outLimits->maxTextureDimension2D = properties.limits.maxImageDimension2D;
    outLimits->maxTextureDimension3D = properties.limits.maxImageDimension3D;
    outLimits->maxTextureArrayLayers = properties.limits.maxImageArrayLayers;
}

// Surface functions
void VulkanBackend::surfaceDestroy(GfxSurface surface) const
{
    delete reinterpret_cast<gfx::vulkan::Surface*>(surface);
}

uint32_t VulkanBackend::surfaceGetSupportedFormats(GfxSurface surface, GfxTextureFormat* formats, uint32_t maxFormats) const
{
    if (!surface) {
        return 0;
    }

    auto* surf = reinterpret_cast<gfx::vulkan::Surface*>(surface);

    // Query supported surface formats from Vulkan
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(surf->physicalDevice(), surf->handle(), &formatCount, nullptr);

    if (formatCount == 0) {
        return 0;
    }

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(surf->physicalDevice(), surf->handle(), &formatCount, surfaceFormats.data());

    // Convert to GfxTextureFormat
    if (formats && maxFormats > 0) {
        uint32_t copyCount = std::min(formatCount, maxFormats);
        for (uint32_t i = 0; i < copyCount; ++i) {
            formats[i] = gfx::convertor::vkFormatToGfxFormat(surfaceFormats[i].format);
        }
    }

    return formatCount;
}

uint32_t VulkanBackend::surfaceGetSupportedPresentModes(GfxSurface surface, GfxPresentMode* presentModes, uint32_t maxModes) const
{
    if (!surface) {
        return 0;
    }

    auto* surf = reinterpret_cast<gfx::vulkan::Surface*>(surface);

    // Query supported present modes from Vulkan
    uint32_t modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(surf->physicalDevice(), surf->handle(), &modeCount, nullptr);

    if (modeCount == 0) {
        return 0;
    }

    std::vector<VkPresentModeKHR> vkPresentModes(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(surf->physicalDevice(), surf->handle(), &modeCount, vkPresentModes.data());

    // Convert to GfxPresentMode
    if (presentModes && maxModes > 0) {
        uint32_t copyCount = std::min(modeCount, maxModes);
        for (uint32_t i = 0; i < copyCount; ++i) {
            presentModes[i] = gfx::convertor::vkPresentModeToGfxPresentMode(vkPresentModes[i]);
        }
    }

    return modeCount;
}

// Swapchain functions
void VulkanBackend::swapchainDestroy(GfxSwapchain swapchain) const
{
    delete reinterpret_cast<gfx::vulkan::Swapchain*>(swapchain);
}

uint32_t VulkanBackend::swapchainGetWidth(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return 0;
    }
    auto* sc = reinterpret_cast<gfx::vulkan::Swapchain*>(swapchain);
    return sc->getWidth();
}

uint32_t VulkanBackend::swapchainGetHeight(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return 0;
    }
    auto* sc = reinterpret_cast<gfx::vulkan::Swapchain*>(swapchain);
    return sc->getHeight();
}

GfxTextureFormat VulkanBackend::swapchainGetFormat(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
    auto* sc = reinterpret_cast<gfx::vulkan::Swapchain*>(swapchain);
    return gfx::convertor::vkFormatToGfxFormat(sc->getFormat());
}

uint32_t VulkanBackend::swapchainGetBufferCount(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return 0;
    }
    auto* sc = reinterpret_cast<gfx::vulkan::Swapchain*>(swapchain);
    return sc->getImageCount();
}

GfxResult VulkanBackend::swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const
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

GfxTextureView VulkanBackend::swapchainGetImageView(GfxSwapchain swapchain, uint32_t imageIndex) const
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

GfxTextureView VulkanBackend::swapchainGetCurrentTextureView(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return nullptr;
    }

    auto* sc = reinterpret_cast<gfx::vulkan::Swapchain*>(swapchain);
    return reinterpret_cast<GfxTextureView>(sc->getCurrentTextureView());
}

GfxResult VulkanBackend::swapchainPresent(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo) const
{
    if (!swapchain) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* sc = reinterpret_cast<gfx::vulkan::Swapchain*>(swapchain);

    std::vector<VkSemaphore> waitSemaphores;
    if (presentInfo && presentInfo->waitSemaphoreCount > 0) {
        waitSemaphores.reserve(presentInfo->waitSemaphoreCount);
        for (uint32_t i = 0; i < presentInfo->waitSemaphoreCount; ++i) {
            auto* sem = reinterpret_cast<gfx::vulkan::Semaphore*>(presentInfo->waitSemaphores[i]);
            if (sem) {
                waitSemaphores.push_back(sem->handle());
            }
        }
    }

    VkResult result = sc->present(waitSemaphores);

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

// Buffer functions
void VulkanBackend::bufferDestroy(GfxBuffer buffer) const
{
    delete reinterpret_cast<gfx::vulkan::Buffer*>(buffer);
}

uint64_t VulkanBackend::bufferGetSize(GfxBuffer buffer) const
{
    if (!buffer) {
        return 0;
    }
    auto* buf = reinterpret_cast<gfx::vulkan::Buffer*>(buffer);
    return buf->size();
}

GfxBufferUsage VulkanBackend::bufferGetUsage(GfxBuffer buffer) const
{
    if (!buffer) {
        return static_cast<GfxBufferUsage>(0);
    }
    auto* buf = reinterpret_cast<gfx::vulkan::Buffer*>(buffer);
    VkBufferUsageFlags vkUsage = buf->getUsage();

    // Convert VkBufferUsageFlags to GfxBufferUsage
    GfxBufferUsage usage = GFX_BUFFER_USAGE_NONE;
    if (vkUsage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
        usage = static_cast<GfxBufferUsage>(usage | GFX_BUFFER_USAGE_COPY_SRC);
    if (vkUsage & VK_BUFFER_USAGE_TRANSFER_DST_BIT)
        usage = static_cast<GfxBufferUsage>(usage | GFX_BUFFER_USAGE_COPY_DST);
    if (vkUsage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
        usage = static_cast<GfxBufferUsage>(usage | GFX_BUFFER_USAGE_INDEX);
    if (vkUsage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
        usage = static_cast<GfxBufferUsage>(usage | GFX_BUFFER_USAGE_VERTEX);
    if (vkUsage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        usage = static_cast<GfxBufferUsage>(usage | GFX_BUFFER_USAGE_UNIFORM);
    if (vkUsage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        usage = static_cast<GfxBufferUsage>(usage | GFX_BUFFER_USAGE_STORAGE);
    if (vkUsage & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
        usage = static_cast<GfxBufferUsage>(usage | GFX_BUFFER_USAGE_INDIRECT);
    return usage;
}

GfxResult VulkanBackend::bufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer) const
{
    if (!buffer || !outMappedPointer) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    (void)offset;
    (void)size;

    auto* buf = reinterpret_cast<gfx::vulkan::Buffer*>(buffer);
    *outMappedPointer = buf->map();
    return GFX_RESULT_SUCCESS;
}

void VulkanBackend::bufferUnmap(GfxBuffer buffer) const
{
    if (!buffer) {
        return;
    }
    auto* buf = reinterpret_cast<gfx::vulkan::Buffer*>(buffer);
    buf->unmap();
}

// Texture functions
void VulkanBackend::textureDestroy(GfxTexture texture) const
{
    delete reinterpret_cast<gfx::vulkan::Texture*>(texture);
}

GfxExtent3D VulkanBackend::textureGetSize(GfxTexture texture) const
{
    if (!texture) {
        GfxExtent3D empty = { 0, 0, 0 };
        return empty;
    }
    auto* tex = reinterpret_cast<gfx::vulkan::Texture*>(texture);
    VkExtent3D vkSize = tex->getSize();
    GfxExtent3D size = { vkSize.width, vkSize.height, vkSize.depth };
    return size;
}

GfxTextureFormat VulkanBackend::textureGetFormat(GfxTexture texture) const
{
    if (!texture) {
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
    auto* tex = reinterpret_cast<gfx::vulkan::Texture*>(texture);
    return gfx::convertor::vkFormatToGfxFormat(tex->getFormat());
}

uint32_t VulkanBackend::textureGetMipLevelCount(GfxTexture texture) const
{
    if (!texture) {
        return 0;
    }
    auto* tex = reinterpret_cast<gfx::vulkan::Texture*>(texture);
    return tex->getMipLevelCount();
}

GfxSampleCount VulkanBackend::textureGetSampleCount(GfxTexture texture) const
{
    if (!texture) {
        return GFX_SAMPLE_COUNT_1;
    }
    auto* tex = reinterpret_cast<gfx::vulkan::Texture*>(texture);
    return gfx::convertor::vkSampleCountToGfxSampleCount(tex->getSampleCount());
}

GfxTextureUsage VulkanBackend::textureGetUsage(GfxTexture texture) const
{
    if (!texture) {
        return static_cast<GfxTextureUsage>(0);
    }
    auto* tex = reinterpret_cast<gfx::vulkan::Texture*>(texture);
    VkImageUsageFlags vkUsage = tex->getUsage();

    // Convert VkImageUsageFlags to GfxTextureUsage
    GfxTextureUsage usage = GFX_TEXTURE_USAGE_NONE;
    if (vkUsage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
        usage = static_cast<GfxTextureUsage>(usage | GFX_TEXTURE_USAGE_COPY_SRC);
    if (vkUsage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        usage = static_cast<GfxTextureUsage>(usage | GFX_TEXTURE_USAGE_COPY_DST);
    if (vkUsage & VK_IMAGE_USAGE_SAMPLED_BIT)
        usage = static_cast<GfxTextureUsage>(usage | GFX_TEXTURE_USAGE_TEXTURE_BINDING);
    if (vkUsage & VK_IMAGE_USAGE_STORAGE_BIT)
        usage = static_cast<GfxTextureUsage>(usage | GFX_TEXTURE_USAGE_STORAGE_BINDING);
    if (vkUsage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
        usage = static_cast<GfxTextureUsage>(usage | GFX_TEXTURE_USAGE_RENDER_ATTACHMENT);
    }
    return usage;
}

GfxTextureLayout VulkanBackend::textureGetLayout(GfxTexture texture) const
{
    if (!texture) {
        return GFX_TEXTURE_LAYOUT_UNDEFINED;
    }
    auto* tex = reinterpret_cast<gfx::vulkan::Texture*>(texture);
    return gfx::convertor::vkImageLayoutToGfxLayout(tex->getLayout());
}

GfxResult VulkanBackend::textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView) const
{
    if (!texture || !outView) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* tex = reinterpret_cast<gfx::vulkan::Texture*>(texture);
        auto createInfo = gfx::convertor::gfxDescriptorToTextureViewCreateInfo(descriptor);
        auto* view = new gfx::vulkan::TextureView(tex, createInfo);
        *outView = reinterpret_cast<GfxTextureView>(view);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create texture view: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

// TextureView functions
void VulkanBackend::textureViewDestroy(GfxTextureView textureView) const
{
    delete reinterpret_cast<gfx::vulkan::TextureView*>(textureView);
}

// Sampler functions
void VulkanBackend::samplerDestroy(GfxSampler sampler) const
{
    delete reinterpret_cast<gfx::vulkan::Sampler*>(sampler);
}

// Shader functions
void VulkanBackend::shaderDestroy(GfxShader shader) const
{
    delete reinterpret_cast<gfx::vulkan::Shader*>(shader);
}

// BindGroupLayout functions
void VulkanBackend::bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout) const
{
    delete reinterpret_cast<gfx::vulkan::BindGroupLayout*>(bindGroupLayout);
}

// BindGroup functions
void VulkanBackend::bindGroupDestroy(GfxBindGroup bindGroup) const
{
    delete reinterpret_cast<gfx::vulkan::BindGroup*>(bindGroup);
}

// RenderPipeline functions
void VulkanBackend::renderPipelineDestroy(GfxRenderPipeline renderPipeline) const
{
    delete reinterpret_cast<gfx::vulkan::RenderPipeline*>(renderPipeline);
}

// ComputePipeline functions
void VulkanBackend::computePipelineDestroy(GfxComputePipeline computePipeline) const
{
    delete reinterpret_cast<gfx::vulkan::ComputePipeline*>(computePipeline);
}

// Queue functions
GfxResult VulkanBackend::queueSubmit(GfxQueue queue, const GfxSubmitInfo* submitInfo) const
{
    if (!queue || !submitInfo) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* q = reinterpret_cast<gfx::vulkan::Queue*>(queue);

    // Convert command encoders to command buffers
    std::vector<VkCommandBuffer> commandBuffers;
    commandBuffers.reserve(submitInfo->commandEncoderCount);
    for (uint32_t i = 0; i < submitInfo->commandEncoderCount; ++i) {
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
    for (uint32_t i = 0; i < submitInfo->waitSemaphoreCount; ++i) {
        auto* sem = reinterpret_cast<gfx::vulkan::Semaphore*>(submitInfo->waitSemaphores[i]);
        waitSemaphores.push_back(sem->handle());
        waitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        if (sem->getType() == gfx::vulkan::SemaphoreType::Timeline) {
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
    for (uint32_t i = 0; i < submitInfo->signalSemaphoreCount; ++i) {
        auto* sem = reinterpret_cast<gfx::vulkan::Semaphore*>(submitInfo->signalSemaphores[i]);
        signalSemaphores.push_back(sem->handle());

        if (sem->getType() == gfx::vulkan::SemaphoreType::Timeline) {
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

void VulkanBackend::queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size) const
{
    if (!queue || !buffer || !data) {
        return;
    }

    // Map, write, unmap
    auto* buf = reinterpret_cast<gfx::vulkan::Buffer*>(buffer);
    void* mapped = buf->map();
    if (mapped) {
        memcpy(static_cast<char*>(mapped) + offset, data, size);
        buf->unmap();
    }
}

void VulkanBackend::queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
    const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
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

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
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
    barrier.oldLayout = tex->getLayout();
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = tex->handle();
    barrier.subresourceRange.aspectMask = gfx::convertor::getImageAspectMask(tex->getFormat());
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
    region.imageSubresource.aspectMask = gfx::convertor::getImageAspectMask(tex->getFormat());
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
    barrier.newLayout = gfx::convertor::gfxLayoutToVkImageLayout(finalLayout);
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = static_cast<VkAccessFlags>(gfxGetAccessFlagsForLayout(finalLayout));

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Update tracked layout
    tex->setLayout(gfx::convertor::gfxLayoutToVkImageLayout(finalLayout));

    // End and submit
    vkEndCommandBuffer(commandBuffer);

    // Create fence for synchronization
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence = VK_NULL_HANDLE;
    vkCreateFence(device, &fenceInfo, nullptr, &fence);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(q->handle(), 1, &submitInfo, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(device, fence, nullptr);

    // Cleanup
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    (void)bytesPerRow; // Unused - assuming tightly packed data
}

GfxResult VulkanBackend::queueWaitIdle(GfxQueue queue) const
{
    if (!queue) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* q = reinterpret_cast<gfx::vulkan::Queue*>(queue);
    vkQueueWaitIdle(q->handle());
    return GFX_RESULT_SUCCESS;
}

// CommandEncoder functions
void VulkanBackend::commandEncoderDestroy(GfxCommandEncoder commandEncoder) const
{
    delete reinterpret_cast<gfx::vulkan::CommandEncoder*>(commandEncoder);
}

GfxResult VulkanBackend::commandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder,
    const GfxRenderPassDescriptor* descriptor,
    GfxRenderPassEncoder* outRenderPass) const
{
    if (!commandEncoder || !outRenderPass || !descriptor) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    // Extract parameters from descriptor
    const GfxColorAttachment* colorAttachments = descriptor->colorAttachments;
    uint32_t colorAttachmentCount = descriptor->colorAttachmentCount;
    const GfxDepthStencilAttachment* depthStencilAttachment = descriptor->depthStencilAttachment;

    // Must have at least one attachment (color or depth)
    if (colorAttachmentCount == 0 && !depthStencilAttachment) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    // Validate all color attachments have valid targets
    for (uint32_t i = 0; i < colorAttachmentCount; ++i) {
        if (!colorAttachments[i].target.view) {
            return GFX_RESULT_ERROR_INVALID_PARAMETER;
        }
    }

    for (uint32_t i = 0; i < colorAttachmentCount; ++i) {
        if (colorAttachments[i].target.finalLayout == GFX_TEXTURE_LAYOUT_UNDEFINED) {
            return GFX_RESULT_ERROR_INVALID_PARAMETER;
        }
    }

    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(commandEncoder);
    VkCommandBuffer cmdBuf = enc->handle();

    // Determine framebuffer dimensions from first available attachment with valid texture
    uint32_t width = 0;
    uint32_t height = 0;

    // Try to get dimensions from color attachments
    if (colorAttachmentCount > 0 && colorAttachments) {
        for (uint32_t i = 0; i < colorAttachmentCount && (width == 0 || height == 0); ++i) {
            auto* view = reinterpret_cast<gfx::vulkan::TextureView*>(colorAttachments[i].target.view);
            auto size = view->getTexture()->getSize();
            width = size.width;
            height = size.height;
        }
    }

    // Fall back to depth attachment if no color attachment had valid dimensions
    if ((width == 0 || height == 0) && depthStencilAttachment) {
        auto* depthView = reinterpret_cast<gfx::vulkan::TextureView*>(depthStencilAttachment->target.view);
        auto size = depthView->getTexture()->getSize();
        width = size.width;
        height = size.height;
    }

    // Build Vulkan attachments and references
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorRefs;
    std::vector<VkAttachmentReference> resolveRefs;

    uint32_t attachmentIndex = 0;
    uint32_t numColorRefs = 0; // Track actual color attachments (not resolve targets)

    // Process color attachments
    for (uint32_t i = 0; i < colorAttachmentCount; ++i) {
        const GfxColorAttachmentTarget* target = &colorAttachments[i].target;
        auto* colorView = reinterpret_cast<gfx::vulkan::TextureView*>(target->view);
        VkSampleCountFlagBits samples = colorView->getTexture()->getSampleCount();

        bool isMSAA = (samples > VK_SAMPLE_COUNT_1_BIT);

        // Add color attachment
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = colorView->getFormat();
        colorAttachment.samples = samples;
        colorAttachment.loadOp = gfx::convertor::gfxLoadOpToVkLoadOp(target->ops.loadOp);
        colorAttachment.storeOp = gfx::convertor::gfxStoreOpToVkStoreOp(target->ops.storeOp);
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        // Use UNDEFINED if we're clearing or don't care, otherwise preserve contents with COLOR_ATTACHMENT_OPTIMAL
        colorAttachment.initialLayout = (target->ops.loadOp == GFX_LOAD_OP_LOAD)
            ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            : VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = gfx::convertor::gfxLayoutToVkImageLayout(target->finalLayout);
        attachments.push_back(colorAttachment);

        VkAttachmentReference colorRef{};
        colorRef.attachment = attachmentIndex++;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorRefs.push_back(colorRef);
        ++numColorRefs;

        // Check if this attachment has a resolve target
        bool hasResolve = false;
        if (colorAttachments[i].resolveTarget) {
            const GfxColorAttachmentTarget* resolveTarget = colorAttachments[i].resolveTarget;
            auto* resolveView = reinterpret_cast<gfx::vulkan::TextureView*>(resolveTarget->view);

            VkAttachmentDescription resolveAttachment{};
            resolveAttachment.format = resolveView->getFormat();
            resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            resolveAttachment.loadOp = gfx::convertor::gfxLoadOpToVkLoadOp(resolveTarget->ops.loadOp);
            resolveAttachment.storeOp = gfx::convertor::gfxStoreOpToVkStoreOp(resolveTarget->ops.storeOp);
            resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            // Use UNDEFINED if we're clearing or don't care, otherwise preserve contents with COLOR_ATTACHMENT_OPTIMAL
            resolveAttachment.initialLayout = (resolveTarget->ops.loadOp == GFX_LOAD_OP_LOAD)
                ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                : VK_IMAGE_LAYOUT_UNDEFINED;
            resolveAttachment.finalLayout = gfx::convertor::gfxLayoutToVkImageLayout(resolveTarget->finalLayout);
            attachments.push_back(resolveAttachment);

            VkAttachmentReference resolveRef{};
            resolveRef.attachment = attachmentIndex++;
            resolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            resolveRefs.push_back(resolveRef);

            hasResolve = true;
        }

        // MSAA without resolve needs unused reference
        if (isMSAA && !hasResolve) {
            VkAttachmentReference unusedRef{};
            unusedRef.attachment = VK_ATTACHMENT_UNUSED;
            unusedRef.layout = VK_IMAGE_LAYOUT_UNDEFINED;
            resolveRefs.push_back(unusedRef);
        }
    }

    // Add depth/stencil attachment if provided
    VkAttachmentReference depthRef{};
    bool hasDepth = false;

    if (depthStencilAttachment) {
        const GfxDepthStencilAttachmentTarget* target = &depthStencilAttachment->target;
        auto* depthView = reinterpret_cast<gfx::vulkan::TextureView*>(target->view);

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depthView->getFormat();
        depthAttachment.samples = depthView->getTexture()->getSampleCount();

        // Handle depth operations (required if depth pointer is set)
        if (target->depthOps) {
            depthAttachment.loadOp = gfx::convertor::gfxLoadOpToVkLoadOp(target->depthOps->loadOp);
            depthAttachment.storeOp = gfx::convertor::gfxStoreOpToVkStoreOp(target->depthOps->storeOp);
        } else {
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }

        // Handle stencil operations (required if stencil pointer is set)
        if (target->stencilOps) {
            depthAttachment.stencilLoadOp = gfx::convertor::gfxLoadOpToVkLoadOp(target->stencilOps->loadOp);
            depthAttachment.stencilStoreOp = gfx::convertor::gfxStoreOpToVkStoreOp(target->stencilOps->storeOp);
        } else {
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }

        // Use UNDEFINED if we're clearing or don't care about both depth AND stencil, otherwise preserve contents
        bool loadDepth = (target->depthOps && target->depthOps->loadOp == GFX_LOAD_OP_LOAD);
        bool loadStencil = (target->stencilOps && target->stencilOps->loadOp == GFX_LOAD_OP_LOAD);
        depthAttachment.initialLayout = (loadDepth || loadStencil)
            ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            : VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = gfx::convertor::gfxLayoutToVkImageLayout(target->finalLayout);
        attachments.push_back(depthAttachment);

        depthRef.attachment = attachmentIndex++;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        hasDepth = true;
    }

    // Create subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
    subpass.pColorAttachments = colorRefs.empty() ? nullptr : colorRefs.data();
    subpass.pResolveAttachments = resolveRefs.empty() ? nullptr : resolveRefs.data();
    subpass.pDepthStencilAttachment = hasDepth ? &depthRef : nullptr;

    // Create render pass
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

    // Create framebuffer with all views (color + resolve + depth) in same order as attachments
    std::vector<VkImageView> fbAttachments;
    fbAttachments.reserve(attachments.size());

    for (uint32_t i = 0; i < colorAttachmentCount; ++i) {
        // Add color attachment
        auto* colorView = reinterpret_cast<gfx::vulkan::TextureView*>(colorAttachments[i].target.view);
        fbAttachments.push_back(colorView->handle());

        // Add resolve target if present
        if (colorAttachments[i].resolveTarget) {
            auto* resolveView = reinterpret_cast<gfx::vulkan::TextureView*>(colorAttachments[i].resolveTarget->view);
            fbAttachments.push_back(resolveView->handle());
        }
    }

    if (depthStencilAttachment) {
        auto* depthView = reinterpret_cast<gfx::vulkan::TextureView*>(depthStencilAttachment->target.view);
        fbAttachments.push_back(depthView->handle());
    }

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(fbAttachments.size());
    framebufferInfo.pAttachments = fbAttachments.data();
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = 1;

    VkFramebuffer framebuffer;
    result = vkCreateFramebuffer(enc->device(), &framebufferInfo, nullptr, &framebuffer);
    if (result != VK_SUCCESS) {
        vkDestroyRenderPass(enc->device(), renderPass, nullptr);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    // Track for cleanup
    enc->trackRenderPass(renderPass, framebuffer);

    // Build clear values matching attachment descriptions order
    std::vector<VkClearValue> clearValues;
    clearValues.reserve(attachments.size());

    uint32_t clearColorIdx = 0;
    for (size_t i = 0; i < attachments.size(); ++i) {
        VkClearValue clearValue{};

        if (gfx::convertor::isDepthFormat(attachments[i].format)) {
            // Depth/stencil attachment
            float depthClear = (depthStencilAttachment->target.depthOps) ? depthStencilAttachment->target.depthOps->clearValue : 1.0f;
            uint32_t stencilClear = (depthStencilAttachment->target.stencilOps) ? depthStencilAttachment->target.stencilOps->clearValue : 0;
            clearValue.depthStencil = { depthClear, stencilClear };
        } else {
            // Color attachment - check if it's a resolve target
            bool isPrevMSAA = (i > 0 && attachments[i - 1].samples > VK_SAMPLE_COUNT_1_BIT);
            bool isResolve = (attachments[i].samples == VK_SAMPLE_COUNT_1_BIT && isPrevMSAA);

            if (isResolve) {
                // Resolve target doesn't need clear value (loadOp is DONT_CARE)
                clearValue.color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
            } else {
                // MSAA or regular color attachment - use provided clear color
                if (clearColorIdx < numColorRefs) {
                    const GfxColor& color = colorAttachments[clearColorIdx].target.ops.clearColor;
                    clearValue.color = { { color.r, color.g, color.b, color.a } };
                    clearColorIdx++;
                } else {
                    clearValue.color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
                }
            }
        }

        clearValues.push_back(clearValue);
    }

    // Begin render pass
    VkRenderPassBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = renderPass;
    beginInfo.framebuffer = framebuffer;
    beginInfo.renderArea.offset = { 0, 0 };
    beginInfo.renderArea.extent = { width, height };
    beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    beginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmdBuf, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

    *outRenderPass = reinterpret_cast<GfxRenderPassEncoder>(commandEncoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::commandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const GfxComputePassDescriptor* descriptor, GfxComputePassEncoder* outComputePass) const
{
    if (!commandEncoder || !descriptor || !outComputePass) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    // In Vulkan, compute passes don't require special setup like render passes
    // We just return the command encoder cast to a compute pass encoder
    *outComputePass = reinterpret_cast<GfxComputePassEncoder>(commandEncoder);

    (void)descriptor->label; // Unused for now
    return GFX_RESULT_SUCCESS;
}

void VulkanBackend::commandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset,
    GfxBuffer destination, uint64_t destinationOffset,
    uint64_t size) const
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

void VulkanBackend::commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset, uint32_t bytesPerRow,
    GfxTexture destination, const GfxOrigin3D* origin,
    const GfxExtent3D* extent, uint32_t mipLevel, GfxTextureLayout finalLayout) const
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
    barrier.oldLayout = dstTex->getLayout();
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = dstTex->handle();
    barrier.subresourceRange.aspectMask = gfx::convertor::getImageAspectMask(dstTex->getFormat());
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
    region.imageSubresource.aspectMask = gfx::convertor::getImageAspectMask(dstTex->getFormat());
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { origin->x, origin->y, origin->z };
    region.imageExtent = { extent->width, extent->height, extent->depth };

    vkCmdCopyBufferToImage(cmdBuf, srcBuf->handle(), dstTex->handle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition image layout to final layout
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = gfx::convertor::gfxLayoutToVkImageLayout(finalLayout);
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = static_cast<VkAccessFlags>(gfxGetAccessFlagsForLayout(finalLayout));

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Update tracked layout
    dstTex->setLayout(gfx::convertor::gfxLayoutToVkImageLayout(finalLayout));

    (void)bytesPerRow; // Unused - assuming tightly packed data
}

void VulkanBackend::commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* origin, uint32_t mipLevel,
    GfxBuffer destination, uint64_t destinationOffset, uint32_t bytesPerRow,
    const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
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
    barrier.oldLayout = srcTex->getLayout();
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = srcTex->handle();
    barrier.subresourceRange.aspectMask = gfx::convertor::getImageAspectMask(srcTex->getFormat());
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
    region.imageSubresource.aspectMask = gfx::convertor::getImageAspectMask(srcTex->getFormat());
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { origin->x, origin->y, origin->z };
    region.imageExtent = { extent->width, extent->height, extent->depth };

    vkCmdCopyImageToBuffer(cmdBuf, srcTex->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstBuf->handle(), 1, &region);

    // Transition image layout to final layout
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = gfx::convertor::gfxLayoutToVkImageLayout(finalLayout);
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = static_cast<VkAccessFlags>(gfxGetAccessFlagsForLayout(finalLayout));

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Update tracked layout
    srcTex->setLayout(gfx::convertor::gfxLayoutToVkImageLayout(finalLayout));

    (void)bytesPerRow; // Unused - assuming tightly packed data
}

void VulkanBackend::commandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* sourceOrigin, uint32_t sourceMipLevel,
    GfxTexture destination, const GfxOrigin3D* destinationOrigin, uint32_t destinationMipLevel,
    const GfxExtent3D* extent, GfxTextureLayout srcFinalLayout, GfxTextureLayout dstFinalLayout) const
{
    if (!commandEncoder || !source || !destination || !sourceOrigin || !destinationOrigin || !extent) {
        return;
    }

    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(commandEncoder);
    auto* srcTex = reinterpret_cast<gfx::vulkan::Texture*>(source);
    auto* dstTex = reinterpret_cast<gfx::vulkan::Texture*>(destination);

    VkCommandBuffer cmdBuf = enc->handle();

    // For 2D textures and arrays, extent->depth represents layer count
    // For 3D textures, it represents actual depth
    uint32_t layerCount = extent->depth;
    uint32_t copyDepth = extent->depth;

    // Check if source is a 3D texture (depth > 1 and not an array)
    VkExtent3D srcSize = srcTex->getSize();
    bool is3DTexture = (srcSize.depth > 1);

    if (!is3DTexture) {
        // For 2D/array textures, extent->depth is layer count, actual depth is 1
        copyDepth = 1;
    } else {
        // For 3D textures, there are no array layers
        layerCount = 1;
    }

    // Transition source image to transfer src optimal
    VkImageMemoryBarrier barriers[2] = {};
    barriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barriers[0].oldLayout = srcTex->getLayout();
    barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].image = srcTex->handle();
    barriers[0].subresourceRange.aspectMask = gfx::convertor::getImageAspectMask(srcTex->getFormat());
    barriers[0].subresourceRange.baseMipLevel = sourceMipLevel;
    barriers[0].subresourceRange.levelCount = 1;
    barriers[0].subresourceRange.baseArrayLayer = sourceOrigin->z;
    barriers[0].subresourceRange.layerCount = layerCount;
    barriers[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    // Transition destination image to transfer dst optimal
    barriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barriers[1].oldLayout = dstTex->getLayout();
    barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[1].image = dstTex->handle();
    barriers[1].subresourceRange.aspectMask = gfx::convertor::getImageAspectMask(dstTex->getFormat());
    barriers[1].subresourceRange.baseMipLevel = destinationMipLevel;
    barriers[1].subresourceRange.levelCount = 1;
    barriers[1].subresourceRange.baseArrayLayer = destinationOrigin->z;
    barriers[1].subresourceRange.layerCount = layerCount;
    barriers[1].srcAccessMask = 0;
    barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmdBuf,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 2, barriers);

    // Copy image to image
    VkImageCopy region{};
    region.srcSubresource.aspectMask = gfx::convertor::getImageAspectMask(srcTex->getFormat());
    region.srcSubresource.mipLevel = sourceMipLevel;
    region.srcSubresource.baseArrayLayer = is3DTexture ? 0 : sourceOrigin->z;
    region.srcSubresource.layerCount = layerCount;
    region.srcOffset = { sourceOrigin->x, sourceOrigin->y, is3DTexture ? sourceOrigin->z : 0 };
    region.dstSubresource.aspectMask = gfx::convertor::getImageAspectMask(dstTex->getFormat());
    region.dstSubresource.mipLevel = destinationMipLevel;
    region.dstSubresource.baseArrayLayer = is3DTexture ? 0 : destinationOrigin->z;
    region.dstSubresource.layerCount = layerCount;
    region.dstOffset = { destinationOrigin->x, destinationOrigin->y, is3DTexture ? destinationOrigin->z : 0 };
    region.extent = { extent->width, extent->height, copyDepth };

    vkCmdCopyImage(cmdBuf, srcTex->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstTex->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition images to final layouts
    barriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[0].newLayout = gfx::convertor::gfxLayoutToVkImageLayout(srcFinalLayout);
    barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barriers[0].dstAccessMask = static_cast<VkAccessFlags>(gfxGetAccessFlagsForLayout(srcFinalLayout));

    barriers[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[1].newLayout = gfx::convertor::gfxLayoutToVkImageLayout(dstFinalLayout);
    barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[1].dstAccessMask = static_cast<VkAccessFlags>(gfxGetAccessFlagsForLayout(dstFinalLayout));

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 2, barriers);

    // Update tracked layouts
    srcTex->setLayout(gfx::convertor::gfxLayoutToVkImageLayout(srcFinalLayout));
    dstTex->setLayout(gfx::convertor::gfxLayoutToVkImageLayout(dstFinalLayout));
}

void VulkanBackend::commandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder,
    const GfxMemoryBarrier* memoryBarriers, uint32_t memoryBarrierCount,
    const GfxBufferBarrier* bufferBarriers, uint32_t bufferBarrierCount,
    const GfxTextureBarrier* textureBarriers, uint32_t textureBarrierCount) const
{
    if (!commandEncoder) {
        return;
    }

    if (memoryBarrierCount == 0 && bufferBarrierCount == 0 && textureBarrierCount == 0) {
        return;
    }

    auto* encoder = reinterpret_cast<gfx::vulkan::CommandEncoder*>(commandEncoder);
    VkCommandBuffer cmdBuffer = encoder->handle();

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
        vkBarrier.srcAccessMask = static_cast<VkAccessFlags>(barrier.srcAccessMask);
        vkBarrier.dstAccessMask = static_cast<VkAccessFlags>(barrier.dstAccessMask);

        memBarriers.push_back(vkBarrier);

        srcStage |= static_cast<VkPipelineStageFlags>(barrier.srcStageMask);
        dstStage |= static_cast<VkPipelineStageFlags>(barrier.dstStageMask);
    }

    // Process buffer barriers
    for (uint32_t i = 0; i < bufferBarrierCount; ++i) {
        const auto& barrier = bufferBarriers[i];
        auto* buffer = reinterpret_cast<gfx::vulkan::Buffer*>(barrier.buffer);

        VkBufferMemoryBarrier vkBarrier{};
        vkBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        vkBarrier.buffer = buffer->handle();
        vkBarrier.offset = barrier.offset;
        vkBarrier.size = barrier.size == 0 ? VK_WHOLE_SIZE : barrier.size;
        vkBarrier.srcAccessMask = static_cast<VkAccessFlags>(barrier.srcAccessMask);
        vkBarrier.dstAccessMask = static_cast<VkAccessFlags>(barrier.dstAccessMask);
        vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        bufferMemoryBarriers.push_back(vkBarrier);

        srcStage |= static_cast<VkPipelineStageFlags>(barrier.srcStageMask);
        dstStage |= static_cast<VkPipelineStageFlags>(barrier.dstStageMask);
    }

    for (uint32_t i = 0; i < textureBarrierCount; ++i) {
        const auto& barrier = textureBarriers[i];
        auto* texture = reinterpret_cast<gfx::vulkan::Texture*>(barrier.texture);

        VkImageMemoryBarrier vkBarrier{};
        vkBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        vkBarrier.image = texture->handle();

        // Determine aspect mask based on texture format
        VkFormat format = texture->getFormat();
        if (gfx::convertor::isDepthFormat(format)) {
            vkBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (gfx::convertor::hasStencilComponent(format)) {
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
        vkBarrier.oldLayout = gfx::convertor::gfxLayoutToVkImageLayout(barrier.oldLayout);
        vkBarrier.newLayout = gfx::convertor::gfxLayoutToVkImageLayout(barrier.newLayout);

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
        texture->setLayout(gfx::convertor::gfxLayoutToVkImageLayout(barrier.newLayout));
    }

    vkCmdPipelineBarrier(
        cmdBuffer,
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

void VulkanBackend::commandEncoderEnd(GfxCommandEncoder commandEncoder) const
{
    if (!commandEncoder) {
        return;
    }
    auto* encoder = reinterpret_cast<gfx::vulkan::CommandEncoder*>(commandEncoder);
    encoder->finish();
}

void VulkanBackend::commandEncoderBegin(GfxCommandEncoder commandEncoder) const
{
    if (!commandEncoder) {
        return;
    }
    auto* encoder = reinterpret_cast<gfx::vulkan::CommandEncoder*>(commandEncoder);
    encoder->reset();
}

// RenderPassEncoder functions
void VulkanBackend::renderPassEncoderDestroy(GfxRenderPassEncoder renderPassEncoder) const
{
    (void)renderPassEncoder;
    // Render pass encoder is just a view of command encoder, no separate cleanup
}

void VulkanBackend::renderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline) const
{
    if (!renderPassEncoder || !pipeline) {
        return;
    }
    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(renderPassEncoder);
    auto* pipe = reinterpret_cast<gfx::vulkan::RenderPipeline*>(pipeline);

    VkCommandBuffer cmdBuf = enc->handle();
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->handle());
    enc->setCurrentPipelineLayout(pipe->layout());
}

void VulkanBackend::renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    if (!renderPassEncoder || !bindGroup) {
        return;
    }
    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(renderPassEncoder);
    auto* bg = reinterpret_cast<gfx::vulkan::BindGroup*>(bindGroup);

    VkCommandBuffer cmdBuf = enc->handle();
    VkPipelineLayout layout = enc->currentPipelineLayout();

    if (layout != VK_NULL_HANDLE) {
        VkDescriptorSet set = bg->handle();
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, index, 1, &set, dynamicOffsetCount, dynamicOffsets);
    }
}

void VulkanBackend::renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size) const
{
    if (!renderPassEncoder || !buffer) {
        return;
    }
    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(renderPassEncoder);
    auto* buf = reinterpret_cast<gfx::vulkan::Buffer*>(buffer);

    VkCommandBuffer cmdBuf = enc->handle();
    VkBuffer vkBuf = buf->handle();
    VkDeviceSize offsets[] = { offset };
    vkCmdBindVertexBuffers(cmdBuf, slot, 1, &vkBuf, offsets);

    (void)size;
}

void VulkanBackend::renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size) const
{
    if (!renderPassEncoder || !buffer) {
        return;
    }
    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(renderPassEncoder);
    auto* buf = reinterpret_cast<gfx::vulkan::Buffer*>(buffer);

    VkCommandBuffer cmdBuf = enc->handle();
    VkIndexType indexType = (format == GFX_INDEX_FORMAT_UINT16) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
    vkCmdBindIndexBuffer(cmdBuf, buf->handle(), offset, indexType);

    (void)size;
}

void VulkanBackend::renderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport) const
{
    if (!renderPassEncoder || !viewport) {
        return;
    }
    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(renderPassEncoder);

    VkViewport vkViewport{};
    vkViewport.x = viewport->x;
    vkViewport.y = viewport->y;
    vkViewport.width = viewport->width;
    vkViewport.height = viewport->height;
    vkViewport.minDepth = viewport->minDepth;
    vkViewport.maxDepth = viewport->maxDepth;
    vkCmdSetViewport(enc->handle(), 0, 1, &vkViewport);
}

void VulkanBackend::renderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor) const
{
    if (!renderPassEncoder || !scissor) {
        return;
    }
    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(renderPassEncoder);

    VkRect2D vkScissor{};
    vkScissor.offset = { scissor->x, scissor->y };
    vkScissor.extent = { scissor->width, scissor->height };
    vkCmdSetScissor(enc->handle(), 0, 1, &vkScissor);
}

void VulkanBackend::renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
{
    if (!renderPassEncoder) {
        return;
    }
    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(renderPassEncoder);

    VkCommandBuffer cmdBuf = enc->handle();
    vkCmdDraw(cmdBuf, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanBackend::renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) const
{
    if (!renderPassEncoder) {
        return;
    }
    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(renderPassEncoder);

    VkCommandBuffer cmdBuf = enc->handle();
    vkCmdDrawIndexed(cmdBuf, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}

void VulkanBackend::renderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder) const
{
    if (!renderPassEncoder) {
        return;
    }
    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(renderPassEncoder);
    vkCmdEndRenderPass(enc->handle());
}

// ComputePassEncoder functions
void VulkanBackend::computePassEncoderDestroy(GfxComputePassEncoder computePassEncoder) const
{
    // Compute pass encoder is just a view of command encoder, no separate cleanup
    (void)computePassEncoder;
}

void VulkanBackend::computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline) const
{
    if (!computePassEncoder || !pipeline) {
        return;
    }

    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(computePassEncoder);
    auto* pipe = reinterpret_cast<gfx::vulkan::ComputePipeline*>(pipeline);

    VkCommandBuffer cmdBuf = enc->handle();
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipe->handle());
    enc->setCurrentPipelineLayout(pipe->layout());
}

void VulkanBackend::computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    if (!computePassEncoder || !bindGroup) {
        return;
    }

    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(computePassEncoder);
    auto* bg = reinterpret_cast<gfx::vulkan::BindGroup*>(bindGroup);

    VkCommandBuffer cmdBuf = enc->handle();
    VkPipelineLayout layout = enc->currentPipelineLayout();

    if (layout != VK_NULL_HANDLE) {
        VkDescriptorSet set = bg->handle();
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, layout, index, 1, &set, dynamicOffsetCount, dynamicOffsets);
    }
}

void VulkanBackend::computePassEncoderDispatchWorkgroups(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) const
{
    if (!computePassEncoder) {
        return;
    }

    auto* enc = reinterpret_cast<gfx::vulkan::CommandEncoder*>(computePassEncoder);
    VkCommandBuffer cmdBuf = enc->handle();
    vkCmdDispatch(cmdBuf, workgroupCountX, workgroupCountY, workgroupCountZ);
}

void VulkanBackend::computePassEncoderEnd(GfxComputePassEncoder computePassEncoder) const
{
    // No special cleanup needed for compute pass in Vulkan
    // The command encoder handles all cleanup
    (void)computePassEncoder;
}

// Fence functions
void VulkanBackend::fenceDestroy(GfxFence fence) const
{
    delete reinterpret_cast<gfx::vulkan::Fence*>(fence);
}

GfxResult VulkanBackend::fenceGetStatus(GfxFence fence, bool* isSignaled) const
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto* f = reinterpret_cast<gfx::vulkan::Fence*>(fence);
    VkResult result = f->getStatus(isSignaled);

    // Convert VkResult to GfxResult
    if (result == VK_SUCCESS) {
        return GFX_RESULT_SUCCESS;
    } else if (result == VK_ERROR_DEVICE_LOST) {
        return GFX_RESULT_ERROR_DEVICE_LOST;
    } else {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::fenceWait(GfxFence fence, uint64_t timeoutNs) const
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto* f = reinterpret_cast<gfx::vulkan::Fence*>(fence);
    VkResult result = f->wait(timeoutNs);

    // Convert VkResult to GfxResult
    if (result == VK_SUCCESS) {
        return GFX_RESULT_SUCCESS;
    } else if (result == VK_TIMEOUT) {
        return GFX_RESULT_TIMEOUT;
    } else if (result == VK_ERROR_DEVICE_LOST) {
        return GFX_RESULT_ERROR_DEVICE_LOST;
    } else {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void VulkanBackend::fenceReset(GfxFence fence) const
{
    if (!fence) {
        return;
    }
    auto* f = reinterpret_cast<gfx::vulkan::Fence*>(fence);
    f->reset();
}

// Semaphore functions
void VulkanBackend::semaphoreDestroy(GfxSemaphore semaphore) const
{
    delete reinterpret_cast<gfx::vulkan::Semaphore*>(semaphore);
}

GfxSemaphoreType VulkanBackend::semaphoreGetType(GfxSemaphore semaphore) const
{
    if (!semaphore) {
        return GFX_SEMAPHORE_TYPE_BINARY;
    }
    auto* s = reinterpret_cast<gfx::vulkan::Semaphore*>(semaphore);
    return s->getType() == gfx::vulkan::SemaphoreType::Timeline ? GFX_SEMAPHORE_TYPE_TIMELINE : GFX_SEMAPHORE_TYPE_BINARY;
}

GfxResult VulkanBackend::semaphoreSignal(GfxSemaphore semaphore, uint64_t value) const
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto* s = reinterpret_cast<gfx::vulkan::Semaphore*>(semaphore);
    VkResult result = s->signal(value);
    return (result == VK_SUCCESS) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}
GfxResult VulkanBackend::semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs) const
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto* s = reinterpret_cast<gfx::vulkan::Semaphore*>(semaphore);
    VkResult result = s->wait(value, timeoutNs);
    return (result == VK_SUCCESS) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

uint64_t VulkanBackend::semaphoreGetValue(GfxSemaphore semaphore) const
{
    if (!semaphore) {
        return 0;
    }
    auto* s = reinterpret_cast<gfx::vulkan::Semaphore*>(semaphore);
    return s->getValue();
}

const IBackend* VulkanBackend::create()
{
    static VulkanBackend vulkanBackend;
    return &vulkanBackend;
}

} // namespace gfx::vulkan