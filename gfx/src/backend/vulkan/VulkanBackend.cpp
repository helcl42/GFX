
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
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto createInfo = converter::gfxDescriptorToInstanceCreateInfo(descriptor);
        auto* instance = new gfx::vulkan::Instance(createInfo);
        *outInstance = converter::toGfx<GfxInstance>(instance);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create instance: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::instanceDestroy(GfxInstance instance) const
{
    delete converter::toNative<Instance>(instance);
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::instanceSetDebugCallback(GfxInstance instance, GfxDebugCallback callback, void* userData) const
{
    if (!instance) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* inst = converter::toNative<Instance>(instance);

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
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter) const
{
    if (!instance || !outAdapter) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* inst = converter::toNative<Instance>(instance);

    try {
        auto createInfo = converter::gfxDescriptorToAdapterCreateInfo(descriptor);
        auto* adapter = new gfx::vulkan::Adapter(inst, createInfo);
        *outAdapter = converter::toGfx<GfxAdapter>(adapter);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to request adapter: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::instanceEnumerateAdapters(GfxInstance instance, uint32_t* adapterCount, GfxAdapter* adapters) const
{
    if (!instance || !adapterCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* inst = converter::toNative<Instance>(instance);
    uint32_t count = gfx::vulkan::Adapter::enumerate(inst, reinterpret_cast<gfx::vulkan::Adapter**>(adapters), adapters ? *adapterCount : 0);
    *adapterCount = count;
    return GFX_RESULT_SUCCESS;
}

// Adapter functions
GfxResult VulkanBackend::adapterDestroy(GfxAdapter adapter) const
{
    delete converter::toNative<Adapter>(adapter);
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice) const
{
    if (!adapter || !outDevice) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        // Device doesn't own the adapter - just keeps a pointer
        auto* adapterPtr = converter::toNative<Adapter>(adapter);
        auto createInfo = converter::gfxDescriptorToDeviceCreateInfo(descriptor);
        auto* device = new gfx::vulkan::Device(adapterPtr, createInfo);
        *outDevice = converter::toGfx<GfxDevice>(device);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create device: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::adapterGetInfo(GfxAdapter adapter, GfxAdapterInfo* outInfo) const
{
    if (!adapter || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* adap = converter::toNative<Adapter>(adapter);
    *outInfo = converter::vkPropertiesToGfxAdapterInfo(adap->getProperties());
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::adapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits) const
{
    if (!adapter || !outLimits) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* adap = converter::toNative<Adapter>(adapter);
    *outLimits = converter::vkPropertiesToGfxDeviceLimits(adap->getProperties());
    return GFX_RESULT_SUCCESS;
}

// Device functions
GfxResult VulkanBackend::deviceDestroy(GfxDevice device) const
{
    delete converter::toNative<Device>(device);
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::deviceGetQueue(GfxDevice device, GfxQueue* outQueue) const
{
    if (!device || !outQueue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* dev = converter::toNative<Device>(device);
    *outQueue = converter::toGfx<GfxQueue>(dev->getQueue());
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface) const
{
    if (!device || !descriptor || !outSurface) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* dev = converter::toNative<Device>(device);
        auto createInfo = converter::gfxDescriptorToSurfaceCreateInfo(descriptor);
        auto* surface = new gfx::vulkan::Surface(dev->getAdapter(), createInfo);
        *outSurface = converter::toGfx<GfxSurface>(surface);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create surface: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateSwapchain(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain) const
{
    if (!device || !surface || !descriptor || !outSwapchain) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* dev = converter::toNative<Device>(device);
        auto* surf = converter::toNative<Surface>(surface);
        auto createInfo = converter::gfxDescriptorToSwapchainCreateInfo(descriptor);
        auto* swapchain = new gfx::vulkan::Swapchain(dev, surf, createInfo);
        *outSwapchain = converter::toGfx<GfxSwapchain>(swapchain);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create swapchain: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer) const
{
    if (!device || !descriptor || !outBuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* dev = converter::toNative<Device>(device);
        auto createInfo = converter::gfxDescriptorToBufferCreateInfo(descriptor);
        auto* buffer = new gfx::vulkan::Buffer(dev, createInfo);
        *outBuffer = converter::toGfx<GfxBuffer>(buffer);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create buffer: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceImportBuffer(GfxDevice device, const GfxExternalBufferDescriptor* descriptor, GfxBuffer* outBuffer) const
{
    if (!device || !descriptor || !outBuffer || !descriptor->nativeHandle) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* dev = converter::toNative<Device>(device);
        VkBuffer vkBuffer = reinterpret_cast<VkBuffer>(const_cast<void*>(descriptor->nativeHandle));
        auto importInfo = converter::gfxExternalDescriptorToBufferImportInfo(descriptor);
        auto* buffer = new gfx::vulkan::Buffer(dev, vkBuffer, importInfo);
        *outBuffer = converter::toGfx<GfxBuffer>(buffer);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to import buffer: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture) const
{
    if (!device || !descriptor || !outTexture) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* dev = converter::toNative<Device>(device);
        auto createInfo = converter::gfxDescriptorToTextureCreateInfo(descriptor);
        auto* texture = new gfx::vulkan::Texture(dev, createInfo);
        *outTexture = converter::toGfx<GfxTexture>(texture);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create texture: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceImportTexture(GfxDevice device, const GfxExternalTextureDescriptor* descriptor, GfxTexture* outTexture) const
{
    if (!device || !descriptor || !outTexture || !descriptor->nativeHandle) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* dev = converter::toNative<Device>(device);
        VkImage vkImage = reinterpret_cast<VkImage>(const_cast<void*>(descriptor->nativeHandle));
        auto importInfo = converter::gfxExternalDescriptorToTextureImportInfo(descriptor);
        auto* texture = new gfx::vulkan::Texture(dev, vkImage, importInfo);
        texture->setLayout(converter::gfxLayoutToVkImageLayout(descriptor->currentLayout));
        *outTexture = converter::toGfx<GfxTexture>(texture);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to import texture: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler) const
{
    if (!device || !descriptor || !outSampler) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* dev = converter::toNative<Device>(device);
        auto createInfo = converter::gfxDescriptorToSamplerCreateInfo(descriptor);
        auto* sampler = new gfx::vulkan::Sampler(dev, createInfo);
        *outSampler = converter::toGfx<GfxSampler>(sampler);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create sampler: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader) const
{
    if (!device || !descriptor || !outShader) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* dev = converter::toNative<Device>(device);
        auto createInfo = converter::gfxDescriptorToShaderCreateInfo(descriptor);
        auto* shader = new gfx::vulkan::Shader(dev, createInfo);
        *outShader = converter::toGfx<GfxShader>(shader);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create shader: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout) const
{
    try {
        auto* dev = converter::toNative<Device>(device);
        auto createInfo = converter::gfxDescriptorToBindGroupLayoutCreateInfo(descriptor);
        auto* layout = new gfx::vulkan::BindGroupLayout(dev, createInfo);
        *outLayout = converter::toGfx<GfxBindGroupLayout>(layout);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup) const
{
    try {
        auto* dev = converter::toNative<Device>(device);
        auto createInfo = converter::gfxDescriptorToBindGroupCreateInfo(descriptor);
        auto* bindGroup = new gfx::vulkan::BindGroup(dev, createInfo);
        *outBindGroup = converter::toGfx<GfxBindGroup>(bindGroup);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline) const
{
    if (!device || !descriptor || !outPipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* dev = converter::toNative<Device>(device);
        auto createInfo = converter::gfxDescriptorToRenderPipelineCreateInfo(descriptor);
        auto* pipeline = new gfx::vulkan::RenderPipeline(dev, createInfo);
        *outPipeline = converter::toGfx<GfxRenderPipeline>(pipeline);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create render pipeline: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline) const
{
    if (!device || !descriptor || !outPipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* dev = converter::toNative<Device>(device);
        auto createInfo = converter::gfxDescriptorToComputePipelineCreateInfo(descriptor);
        auto* pipeline = new gfx::vulkan::ComputePipeline(dev, createInfo);
        *outPipeline = converter::toGfx<GfxComputePipeline>(pipeline);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create compute pipeline: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateCommandEncoder(GfxDevice device, const GfxCommandEncoderDescriptor* descriptor, GfxCommandEncoder* outEncoder) const
{
    if (!device || !descriptor || !outEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* dev = converter::toNative<Device>(device);
        auto* encoder = new gfx::vulkan::CommandEncoder(dev);
        *outEncoder = converter::toGfx<GfxCommandEncoder>(encoder);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create command encoder: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    (void)descriptor->label; // Unused for now
}

GfxResult VulkanBackend::deviceCreateRenderPass(GfxDevice device, const GfxRenderPassDescriptor* descriptor, GfxRenderPass* outRenderPass) const
{
    if (!device || !descriptor || !outRenderPass) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* dev = converter::toNative<Device>(device);
        auto createInfo = converter::gfxRenderPassDescriptorToRenderPassCreateInfo(descriptor);
        auto* renderPass = new gfx::vulkan::RenderPass(dev, createInfo);
        *outRenderPass = converter::toGfx<GfxRenderPass>(renderPass);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create render pass: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::deviceCreateFramebuffer(GfxDevice device, const GfxFramebufferDescriptor* descriptor, GfxFramebuffer* outFramebuffer) const
{
    if (!device || !descriptor || !outFramebuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* dev = converter::toNative<Device>(device);
        auto createInfo = converter::gfxFramebufferDescriptorToFramebufferCreateInfo(descriptor);
        auto* framebuffer = new gfx::vulkan::Framebuffer(dev, createInfo);
        *outFramebuffer = converter::toGfx<GfxFramebuffer>(framebuffer);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create framebuffer: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence) const
{
    if (!device || !outFence) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* dev = converter::toNative<Device>(device);
        auto createInfo = converter::gfxDescriptorToFenceCreateInfo(descriptor);
        auto* fence = new gfx::vulkan::Fence(dev, createInfo);
        *outFence = converter::toGfx<GfxFence>(fence);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create fence: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore) const
{
    if (!device || !outSemaphore) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* dev = converter::toNative<Device>(device);
        auto createInfo = converter::gfxDescriptorToSemaphoreCreateInfo(descriptor);
        auto* semaphore = new gfx::vulkan::Semaphore(dev, createInfo);
        *outSemaphore = converter::toGfx<GfxSemaphore>(semaphore);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create semaphore: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::deviceWaitIdle(GfxDevice device) const
{
    if (!device) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* dev = converter::toNative<Device>(device);
    dev->waitIdle();
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits) const
{
    if (!device || !outLimits) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* dev = converter::toNative<Device>(device);
    *outLimits = converter::vkPropertiesToGfxDeviceLimits(dev->getProperties());
    return GFX_RESULT_SUCCESS;
}

// Surface functions
GfxResult VulkanBackend::surfaceDestroy(GfxSurface surface) const
{
    delete converter::toNative<Surface>(surface);
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::surfaceEnumerateSupportedFormats(GfxSurface surface, uint32_t* formatCount, GfxTextureFormat* formats) const
{
    if (!surface || !formatCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* surf = converter::toNative<Surface>(surface);
    auto surfaceFormats = surf->getSupportedFormats();
    uint32_t count = static_cast<uint32_t>(surfaceFormats.size());

    // Convert to GfxTextureFormat
    if (formats) {
        uint32_t copyCount = std::min(count, *formatCount);
        for (uint32_t i = 0; i < copyCount; ++i) {
            formats[i] = converter::vkFormatToGfxFormat(surfaceFormats[i].format);
        }
    }

    *formatCount = count;
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::surfaceEnumerateSupportedPresentModes(GfxSurface surface, uint32_t* presentModeCount, GfxPresentMode* presentModes) const
{
    if (!surface || !presentModeCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* surf = converter::toNative<Surface>(surface);
    auto vkPresentModes = surf->getSupportedPresentModes();
    uint32_t count = static_cast<uint32_t>(vkPresentModes.size());

    // Convert to GfxPresentMode
    if (presentModes) {
        uint32_t copyCount = std::min(count, *presentModeCount);
        for (uint32_t i = 0; i < copyCount; ++i) {
            presentModes[i] = converter::vkPresentModeToGfxPresentMode(vkPresentModes[i]);
        }
    }

    *presentModeCount = count;
    return GFX_RESULT_SUCCESS;
}

// Swapchain functions
GfxResult VulkanBackend::swapchainDestroy(GfxSwapchain swapchain) const
{
    delete converter::toNative<Swapchain>(swapchain);
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::swapchainGetInfo(GfxSwapchain swapchain, GfxSwapchainInfo* outInfo) const
{
    if (!swapchain || !outInfo) {
        if (outInfo) {
            outInfo->width = 0;
            outInfo->height = 0;
            outInfo->format = GFX_TEXTURE_FORMAT_UNDEFINED;
            outInfo->imageCount = 0;
        }
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* sc = converter::toNative<Swapchain>(swapchain);
    *outInfo = converter::vkSwapchainInfoToGfxSwapchainInfo(sc->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const
{
    if (!swapchain || !outImageIndex) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* sc = converter::toNative<Swapchain>(swapchain);

    VkSemaphore vkSemaphore = VK_NULL_HANDLE;
    if (imageAvailableSemaphore) {
        auto* sem = converter::toNative<Semaphore>(imageAvailableSemaphore);
        vkSemaphore = sem->handle();
    }

    VkFence vkFence = VK_NULL_HANDLE;
    if (fence) {
        auto* f = converter::toNative<Fence>(fence);
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

GfxResult VulkanBackend::swapchainGetTextureView(GfxSwapchain swapchain, uint32_t imageIndex, GfxTextureView* outView) const
{
    if (!swapchain || !outView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* sc = converter::toNative<Swapchain>(swapchain);
    if (imageIndex >= sc->getImageCount()) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    *outView = converter::toGfx<GfxTextureView>(sc->getTextureView(imageIndex));
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::swapchainGetCurrentTextureView(GfxSwapchain swapchain, GfxTextureView* outView) const
{
    if (!swapchain || !outView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* sc = converter::toNative<Swapchain>(swapchain);
    *outView = converter::toGfx<GfxTextureView>(sc->getCurrentTextureView());
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::swapchainPresent(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo) const
{
    if (!swapchain) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* sc = converter::toNative<Swapchain>(swapchain);

    std::vector<VkSemaphore> waitSemaphores;
    if (presentInfo && presentInfo->waitSemaphoreCount > 0) {
        waitSemaphores.reserve(presentInfo->waitSemaphoreCount);
        for (uint32_t i = 0; i < presentInfo->waitSemaphoreCount; ++i) {
            auto* sem = converter::toNative<Semaphore>(presentInfo->waitSemaphores[i]);
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
GfxResult VulkanBackend::bufferDestroy(GfxBuffer buffer) const
{
    delete converter::toNative<Buffer>(buffer);
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::bufferGetInfo(GfxBuffer buffer, GfxBufferInfo* outInfo) const
{
    if (!buffer || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* buf = converter::toNative<Buffer>(buffer);
    *outInfo = converter::vkBufferToGfxBufferInfo(buf->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::bufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer) const
{
    if (!buffer || !outMappedPointer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    (void)offset;
    (void)size;

    auto* buf = converter::toNative<Buffer>(buffer);
    *outMappedPointer = buf->map();
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::bufferUnmap(GfxBuffer buffer) const
{
    if (!buffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* buf = converter::toNative<Buffer>(buffer);
    buf->unmap();
    return GFX_RESULT_SUCCESS;
}

// Texture functions
GfxResult VulkanBackend::textureDestroy(GfxTexture texture) const
{
    delete converter::toNative<Texture>(texture);
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::textureGetInfo(GfxTexture texture, GfxTextureInfo* outInfo) const
{
    if (!texture || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* tex = converter::toNative<Texture>(texture);
    *outInfo = converter::vkTextureInfoToGfxTextureInfo(tex->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::textureGetLayout(GfxTexture texture, GfxTextureLayout* outLayout) const
{
    if (!texture || !outLayout) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* tex = converter::toNative<Texture>(texture);
    *outLayout = converter::vkImageLayoutToGfxLayout(tex->getLayout());
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView) const
{
    if (!texture || !outView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* tex = converter::toNative<Texture>(texture);
        auto createInfo = converter::gfxDescriptorToTextureViewCreateInfo(descriptor);
        auto* view = new gfx::vulkan::TextureView(tex, createInfo);
        *outView = converter::toGfx<GfxTextureView>(view);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create texture view: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

// TextureView functions
GfxResult VulkanBackend::textureViewDestroy(GfxTextureView textureView) const
{
    delete converter::toNative<TextureView>(textureView);
    return GFX_RESULT_SUCCESS;
}

// Sampler functions
GfxResult VulkanBackend::samplerDestroy(GfxSampler sampler) const
{
    delete converter::toNative<Sampler>(sampler);
    return GFX_RESULT_SUCCESS;
}

// Shader functions
GfxResult VulkanBackend::shaderDestroy(GfxShader shader) const
{
    delete converter::toNative<Shader>(shader);
    return GFX_RESULT_SUCCESS;
}

// BindGroupLayout functions
GfxResult VulkanBackend::bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout) const
{
    delete converter::toNative<BindGroupLayout>(bindGroupLayout);
    return GFX_RESULT_SUCCESS;
}

// BindGroup functions
GfxResult VulkanBackend::bindGroupDestroy(GfxBindGroup bindGroup) const
{
    delete converter::toNative<BindGroup>(bindGroup);
    return GFX_RESULT_SUCCESS;
}

// RenderPipeline functions
GfxResult VulkanBackend::renderPipelineDestroy(GfxRenderPipeline renderPipeline) const
{
    delete converter::toNative<RenderPipeline>(renderPipeline);
    return GFX_RESULT_SUCCESS;
}

// ComputePipeline functions
GfxResult VulkanBackend::computePipelineDestroy(GfxComputePipeline computePipeline) const
{
    delete converter::toNative<ComputePipeline>(computePipeline);
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::renderPassDestroy(GfxRenderPass renderPass) const
{
    delete converter::toNative<RenderPass>(renderPass);
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::framebufferDestroy(GfxFramebuffer framebuffer) const
{
    delete converter::toNative<Framebuffer>(framebuffer);
    return GFX_RESULT_SUCCESS;
}

// Queue functions
GfxResult VulkanBackend::queueSubmit(GfxQueue queue, const GfxSubmitInfo* submitInfo) const
{
    if (!queue || !submitInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* q = converter::toNative<Queue>(queue);
    auto internalSubmitInfo = converter::gfxDescriptorToSubmitInfo(submitInfo);
    VkResult result = q->submit(internalSubmitInfo);
    return (result == VK_SUCCESS) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

GfxResult VulkanBackend::queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size) const
{
    if (!queue || !buffer || !data) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* q = converter::toNative<Queue>(queue);
    auto* buf = converter::toNative<Buffer>(buffer);
    q->writeBuffer(buf, offset, data, size);
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
    const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
{
    if (!queue || !texture || !data || !extent || dataSize == 0) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* q = converter::toNative<Queue>(queue);
    auto* tex = converter::toNative<Texture>(texture);

    VkOffset3D vkOrigin = origin ? converter::gfxOrigin3DToVkOffset3D(origin) : VkOffset3D{ 0, 0, 0 };
    VkExtent3D vkExtent = converter::gfxExtent3DToVkExtent3D(extent);
    VkImageLayout vkLayout = converter::gfxLayoutToVkImageLayout(finalLayout);

    q->writeTexture(tex, vkOrigin, mipLevel, data, dataSize, vkExtent, vkLayout);

    (void)bytesPerRow; // Unused - assuming tightly packed data
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::queueWaitIdle(GfxQueue queue) const
{
    if (!queue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* q = converter::toNative<Queue>(queue);
    q->waitIdle();
    return GFX_RESULT_SUCCESS;
}

// CommandEncoder functions
GfxResult VulkanBackend::commandEncoderDestroy(GfxCommandEncoder commandEncoder) const
{
    delete converter::toNative<CommandEncoder>(commandEncoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::commandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder,
    const GfxRenderPassBeginDescriptor* beginDescriptor,
    GfxRenderPassEncoder* outRenderPass) const
{
    if (!commandEncoder || !outRenderPass || !beginDescriptor || !beginDescriptor->renderPass || !beginDescriptor->framebuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* encoderPtr = converter::toNative<CommandEncoder>(commandEncoder);
        auto* renderPass = converter::toNative<RenderPass>(beginDescriptor->renderPass);
        auto* framebuffer = converter::toNative<Framebuffer>(beginDescriptor->framebuffer);
        auto beginInfo = converter::gfxRenderPassBeginDescriptorToBeginInfo(beginDescriptor);
        auto* renderPassEncoder = new RenderPassEncoder(encoderPtr, renderPass, framebuffer, beginInfo);
        *outRenderPass = converter::toGfx<GfxRenderPassEncoder>(renderPassEncoder);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to begin render pass: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::commandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const GfxComputePassBeginDescriptor* beginDescriptor, GfxComputePassEncoder* outComputePass) const
{
    if (!commandEncoder || !beginDescriptor || !outComputePass) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* encoderPtr = converter::toNative<CommandEncoder>(commandEncoder);
        auto createInfo = converter::gfxComputePassBeginDescriptorToCreateInfo(beginDescriptor);
        auto* computePassEncoder = new ComputePassEncoder(encoderPtr, createInfo);
        *outComputePass = converter::toGfx<GfxComputePassEncoder>(computePassEncoder);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception&) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::commandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset,
    GfxBuffer destination, uint64_t destinationOffset,
    uint64_t size) const
{
    if (!commandEncoder || !source || !destination) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* enc = converter::toNative<CommandEncoder>(commandEncoder);
    auto* srcBuf = converter::toNative<Buffer>(source);
    auto* dstBuf = converter::toNative<Buffer>(destination);

    enc->copyBufferToBuffer(srcBuf, sourceOffset, dstBuf, destinationOffset, size);
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset, uint32_t bytesPerRow,
    GfxTexture destination, const GfxOrigin3D* origin,
    const GfxExtent3D* extent, uint32_t mipLevel, GfxTextureLayout finalLayout) const
{

    if (!commandEncoder || !source || !destination || !origin || !extent) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* enc = converter::toNative<CommandEncoder>(commandEncoder);
    auto* srcBuf = converter::toNative<Buffer>(source);
    auto* dstTex = converter::toNative<Texture>(destination);

    VkOffset3D vkOrigin = converter::gfxOrigin3DToVkOffset3D(origin);
    VkExtent3D vkExtent = converter::gfxExtent3DToVkExtent3D(extent);
    VkImageLayout vkLayout = converter::gfxLayoutToVkImageLayout(finalLayout);

    enc->copyBufferToTexture(srcBuf, sourceOffset, dstTex, vkOrigin, vkExtent, mipLevel, vkLayout);

    (void)bytesPerRow; // Unused - assuming tightly packed data
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* origin, uint32_t mipLevel,
    GfxBuffer destination, uint64_t destinationOffset, uint32_t bytesPerRow,
    const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
{
    if (!commandEncoder || !source || !destination || !origin || !extent) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* enc = converter::toNative<CommandEncoder>(commandEncoder);
    auto* srcTex = converter::toNative<Texture>(source);
    auto* dstBuf = converter::toNative<Buffer>(destination);

    VkOffset3D vkOrigin = converter::gfxOrigin3DToVkOffset3D(origin);
    VkExtent3D vkExtent = converter::gfxExtent3DToVkExtent3D(extent);
    VkImageLayout vkLayout = converter::gfxLayoutToVkImageLayout(finalLayout);

    enc->copyTextureToBuffer(srcTex, vkOrigin, mipLevel, dstBuf, destinationOffset, vkExtent, vkLayout);

    (void)bytesPerRow; // Unused - assuming tightly packed data
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::commandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* sourceOrigin, uint32_t sourceMipLevel,
    GfxTexture destination, const GfxOrigin3D* destinationOrigin, uint32_t destinationMipLevel,
    const GfxExtent3D* extent, GfxTextureLayout srcFinalLayout, GfxTextureLayout dstFinalLayout) const
{
    if (!commandEncoder || !source || !destination || !sourceOrigin || !destinationOrigin || !extent) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* enc = converter::toNative<CommandEncoder>(commandEncoder);
    auto* srcTex = converter::toNative<Texture>(source);
    auto* dstTex = converter::toNative<Texture>(destination);

    VkOffset3D vkSrcOrigin = converter::gfxOrigin3DToVkOffset3D(sourceOrigin);
    VkOffset3D vkDstOrigin = converter::gfxOrigin3DToVkOffset3D(destinationOrigin);
    VkExtent3D vkExtent = converter::gfxExtent3DToVkExtent3D(extent);
    VkImageLayout vkSrcLayout = converter::gfxLayoutToVkImageLayout(srcFinalLayout);
    VkImageLayout vkDstLayout = converter::gfxLayoutToVkImageLayout(dstFinalLayout);

    enc->copyTextureToTexture(srcTex, vkSrcOrigin, sourceMipLevel,
        dstTex, vkDstOrigin, destinationMipLevel,
        vkExtent, vkSrcLayout, vkDstLayout);
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::commandEncoderBlitTextureToTexture(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* sourceOrigin, const GfxExtent3D* sourceExtent, uint32_t sourceMipLevel,
    GfxTexture destination, const GfxOrigin3D* destinationOrigin, const GfxExtent3D* destinationExtent, uint32_t destinationMipLevel,
    GfxFilterMode filter, GfxTextureLayout srcFinalLayout, GfxTextureLayout dstFinalLayout) const
{
    if (!commandEncoder || !source || !destination || !sourceOrigin || !sourceExtent || !destinationOrigin || !destinationExtent) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* enc = converter::toNative<CommandEncoder>(commandEncoder);
    auto* srcTex = converter::toNative<Texture>(source);
    auto* dstTex = converter::toNative<Texture>(destination);

    VkOffset3D vkSrcOrigin = converter::gfxOrigin3DToVkOffset3D(sourceOrigin);
    VkExtent3D vkSrcExtent = converter::gfxExtent3DToVkExtent3D(sourceExtent);
    VkOffset3D vkDstOrigin = converter::gfxOrigin3DToVkOffset3D(destinationOrigin);
    VkExtent3D vkDstExtent = converter::gfxExtent3DToVkExtent3D(destinationExtent);
    VkFilter vkFilter = converter::gfxFilterToVkFilter(filter);
    VkImageLayout vkSrcLayout = converter::gfxLayoutToVkImageLayout(srcFinalLayout);
    VkImageLayout vkDstLayout = converter::gfxLayoutToVkImageLayout(dstFinalLayout);

    enc->blitTextureToTexture(srcTex, vkSrcOrigin, vkSrcExtent, sourceMipLevel,
        dstTex, vkDstOrigin, vkDstExtent, destinationMipLevel,
        vkFilter, vkSrcLayout, vkDstLayout);
    return GFX_RESULT_SUCCESS;
}

// TODO - add member function to CommandEncoder for pipeline barrier
GfxResult VulkanBackend::commandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder,
    const GfxMemoryBarrier* memoryBarriers, uint32_t memoryBarrierCount,
    const GfxBufferBarrier* bufferBarriers, uint32_t bufferBarrierCount,
    const GfxTextureBarrier* textureBarriers, uint32_t textureBarrierCount) const
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (memoryBarrierCount == 0 && bufferBarrierCount == 0 && textureBarrierCount == 0) {
        return GFX_RESULT_SUCCESS;
    }

    auto* encoder = converter::toNative<CommandEncoder>(commandEncoder);

    // Convert GFX barriers to internal Vulkan barriers
    std::vector<MemoryBarrier> internalMemBarriers;
    internalMemBarriers.reserve(memoryBarrierCount);
    for (uint32_t i = 0; i < memoryBarrierCount; ++i) {
        internalMemBarriers.push_back(converter::gfxMemoryBarrierToMemoryBarrier(memoryBarriers[i]));
    }

    std::vector<BufferBarrier> internalBufBarriers;
    internalBufBarriers.reserve(bufferBarrierCount);
    for (uint32_t i = 0; i < bufferBarrierCount; ++i) {
        internalBufBarriers.push_back(converter::gfxBufferBarrierToBufferBarrier(bufferBarriers[i]));
    }

    std::vector<TextureBarrier> internalTexBarriers;
    internalTexBarriers.reserve(textureBarrierCount);
    for (uint32_t i = 0; i < textureBarrierCount; ++i) {
        internalTexBarriers.push_back(converter::gfxTextureBarrierToTextureBarrier(textureBarriers[i]));
    }

    encoder->pipelineBarrier(
        internalMemBarriers.data(), static_cast<uint32_t>(internalMemBarriers.size()),
        internalBufBarriers.data(), static_cast<uint32_t>(internalBufBarriers.size()),
        internalTexBarriers.data(), static_cast<uint32_t>(internalTexBarriers.size()));
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::commandEncoderGenerateMipmaps(GfxCommandEncoder commandEncoder, GfxTexture texture) const
{
    if (!commandEncoder || !texture) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoder = converter::toNative<CommandEncoder>(commandEncoder);
    auto* tex = converter::toNative<Texture>(texture);

    tex->generateMipmaps(encoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::commandEncoderGenerateMipmapsRange(GfxCommandEncoder commandEncoder, GfxTexture texture,
    uint32_t baseMipLevel, uint32_t levelCount) const
{
    if (!commandEncoder || !texture) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoder = converter::toNative<CommandEncoder>(commandEncoder);
    auto* tex = converter::toNative<Texture>(texture);

    tex->generateMipmapsRange(encoder, baseMipLevel, levelCount);
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::commandEncoderEnd(GfxCommandEncoder commandEncoder) const
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* encoder = converter::toNative<CommandEncoder>(commandEncoder);
    encoder->end();
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::commandEncoderBegin(GfxCommandEncoder commandEncoder) const
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* encoder = converter::toNative<CommandEncoder>(commandEncoder);
    encoder->reset();
    return GFX_RESULT_SUCCESS;
}

// RenderPassEncoder functions
void VulkanBackend::renderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline) const
{
    if (!renderPassEncoder || !pipeline) {
        return;
    }
    auto* rpe = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    auto* pipe = converter::toNative<RenderPipeline>(pipeline);
    rpe->setPipeline(pipe);
}

void VulkanBackend::renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    if (!renderPassEncoder || !bindGroup) {
        return;
    }
    auto* rpe = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    auto* bg = converter::toNative<BindGroup>(bindGroup);
    rpe->setBindGroup(index, bg, dynamicOffsets, dynamicOffsetCount);
}

void VulkanBackend::renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size) const
{
    if (!renderPassEncoder || !buffer) {
        return;
    }
    auto* rpe = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    auto* buf = converter::toNative<Buffer>(buffer);
    rpe->setVertexBuffer(slot, buf, offset);

    (void)size;
}

void VulkanBackend::renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size) const
{
    if (!renderPassEncoder || !buffer) {
        return;
    }
    auto* rpe = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    auto* buf = converter::toNative<Buffer>(buffer);
    VkIndexType indexType = converter::gfxIndexFormatToVkIndexType(format);
    rpe->setIndexBuffer(buf, indexType, offset);

    (void)size;
}

void VulkanBackend::renderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport) const
{
    if (!renderPassEncoder || !viewport) {
        return;
    }
    auto* rpe = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    Viewport vkViewport = converter::gfxViewportToViewport(viewport);
    rpe->setViewport(vkViewport);
}

void VulkanBackend::renderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor) const
{
    if (!renderPassEncoder || !scissor) {
        return;
    }
    auto* rpe = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    ScissorRect vkScissor = converter::gfxScissorRectToScissorRect(scissor);
    rpe->setScissorRect(vkScissor);
}

void VulkanBackend::renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
{
    if (!renderPassEncoder) {
        return;
    }
    auto* rpe = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    rpe->draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanBackend::renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) const
{
    if (!renderPassEncoder) {
        return;
    }
    auto* rpe = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    rpe->drawIndexed(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}

void VulkanBackend::renderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder) const
{
    if (!renderPassEncoder) {
        return;
    }
    auto* rpe = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    delete rpe;
}

// ComputePassEncoder functions
void VulkanBackend::computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline) const
{
    if (!computePassEncoder || !pipeline) {
        return;
    }
    auto* cpe = converter::toNative<ComputePassEncoder>(computePassEncoder);
    auto* pipe = converter::toNative<ComputePipeline>(pipeline);
    cpe->setPipeline(pipe);
}

void VulkanBackend::computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    if (!computePassEncoder || !bindGroup) {
        return;
    }
    auto* cpe = converter::toNative<ComputePassEncoder>(computePassEncoder);
    auto* bg = converter::toNative<BindGroup>(bindGroup);
    cpe->setBindGroup(index, bg, dynamicOffsets, dynamicOffsetCount);
}

void VulkanBackend::computePassEncoderDispatchWorkgroups(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) const
{
    if (!computePassEncoder) {
        return;
    }

    auto* cpe = converter::toNative<ComputePassEncoder>(computePassEncoder);
    cpe->dispatchWorkgroups(workgroupCountX, workgroupCountY, workgroupCountZ);
}

void VulkanBackend::computePassEncoderEnd(GfxComputePassEncoder computePassEncoder) const
{
    if (!computePassEncoder) {
        return;
    }
    auto* cpe = converter::toNative<ComputePassEncoder>(computePassEncoder);
    delete cpe;
}

// Fence functions
GfxResult VulkanBackend::fenceDestroy(GfxFence fence) const
{
    delete converter::toNative<Fence>(fence);
    return GFX_RESULT_SUCCESS;
}

GfxResult VulkanBackend::fenceGetStatus(GfxFence fence, bool* isSignaled) const
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* f = converter::toNative<Fence>(fence);
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
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* f = converter::toNative<Fence>(fence);
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
    auto* f = converter::toNative<Fence>(fence);
    f->reset();
}

// Semaphore functions
GfxResult VulkanBackend::semaphoreDestroy(GfxSemaphore semaphore) const
{
    delete converter::toNative<Semaphore>(semaphore);
    return GFX_RESULT_SUCCESS;
}

GfxSemaphoreType VulkanBackend::semaphoreGetType(GfxSemaphore semaphore) const
{
    if (!semaphore) {
        return GFX_SEMAPHORE_TYPE_BINARY;
    }
    auto* s = converter::toNative<Semaphore>(semaphore);
    return s->getType() == gfx::vulkan::SemaphoreType::Timeline ? GFX_SEMAPHORE_TYPE_TIMELINE : GFX_SEMAPHORE_TYPE_BINARY;
}

GfxResult VulkanBackend::semaphoreSignal(GfxSemaphore semaphore, uint64_t value) const
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* s = converter::toNative<Semaphore>(semaphore);
    VkResult result = s->signal(value);
    return (result == VK_SUCCESS) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}
GfxResult VulkanBackend::semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs) const
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* s = converter::toNative<Semaphore>(semaphore);
    VkResult result = s->wait(value, timeoutNs);
    return (result == VK_SUCCESS) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

uint64_t VulkanBackend::semaphoreGetValue(GfxSemaphore semaphore) const
{
    if (!semaphore) {
        return 0;
    }
    auto* s = converter::toNative<Semaphore>(semaphore);
    return s->getValue();
}

GfxAccessFlags VulkanBackend::getAccessFlagsForLayout(GfxTextureLayout layout) const
{
    VkImageLayout vkLayout = converter::gfxLayoutToVkImageLayout(layout);
    VkAccessFlags vkAccessFlags = getVkAccessFlagsForLayout(vkLayout);
    return converter::vkAccessFlagsToGfxAccessFlags(vkAccessFlags);
}

const IBackend* VulkanBackend::create()
{
    static VulkanBackend vulkanBackend;
    return &vulkanBackend;
}

} // namespace gfx::vulkan