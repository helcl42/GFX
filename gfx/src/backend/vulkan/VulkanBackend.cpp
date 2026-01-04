
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
        auto createInfo = converter::gfxDescriptorToInstanceCreateInfo(descriptor);
        auto* instance = new gfx::vulkan::Instance(createInfo);
        *outInstance = converter::toGfx<GfxInstance>(instance);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create instance: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void VulkanBackend::instanceDestroy(GfxInstance instance) const
{
    delete converter::toNative<Instance>(instance);
}

void VulkanBackend::instanceSetDebugCallback(GfxInstance instance, GfxDebugCallback callback, void* userData) const
{
    if (!instance) {
        return;
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
}

GfxResult VulkanBackend::instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter) const
{
    if (!instance || !outAdapter) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
}

uint32_t VulkanBackend::instanceEnumerateAdapters(GfxInstance instance, GfxAdapter* adapters, uint32_t maxAdapters) const
{
    if (!instance || !adapters || maxAdapters == 0) {
        return 0;
    }

    auto* inst = converter::toNative<Instance>(instance);
    return gfx::vulkan::Adapter::enumerate(inst, reinterpret_cast<gfx::vulkan::Adapter**>(adapters), maxAdapters);
}

// Adapter functions
void VulkanBackend::adapterDestroy(GfxAdapter adapter) const
{
    delete converter::toNative<Adapter>(adapter);
}

GfxResult VulkanBackend::adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice) const
{
    if (!adapter || !outDevice) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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

const char* VulkanBackend::adapterGetName(GfxAdapter adapter) const
{
    if (!adapter) {
        return nullptr;
    }
    auto* adap = converter::toNative<Adapter>(adapter);
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
    auto* adap = converter::toNative<Adapter>(adapter);
    *outLimits = converter::vkPropertiesToGfxDeviceLimits(adap->getLimits());
}

// Device functions
void VulkanBackend::deviceDestroy(GfxDevice device) const
{
    delete converter::toNative<Device>(device);
}

GfxQueue VulkanBackend::deviceGetQueue(GfxDevice device) const
{
    if (!device) {
        return nullptr;
    }
    auto* dev = converter::toNative<Device>(device);
    return converter::toGfx<GfxQueue>(dev->getQueue());
}

GfxResult VulkanBackend::deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface) const
{
    if (!device || !descriptor || !outSurface) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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

GfxResult VulkanBackend::deviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture) const
{
    if (!device || !descriptor || !outTexture) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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

GfxResult VulkanBackend::deviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler) const
{
    if (!device || !descriptor || !outSampler) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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

GfxResult VulkanBackend::deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence) const
{
    if (!device || !outFence) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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

void VulkanBackend::deviceWaitIdle(GfxDevice device) const
{
    if (!device) {
        return;
    }
    auto* dev = converter::toNative<Device>(device);
    dev->waitIdle();
}

void VulkanBackend::deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits) const
{
    if (!device || !outLimits) {
        return;
    }
    auto* dev = converter::toNative<Device>(device);
    *outLimits = converter::vkPropertiesToGfxDeviceLimits(dev->getLimits());
}

// Surface functions
void VulkanBackend::surfaceDestroy(GfxSurface surface) const
{
    delete converter::toNative<Surface>(surface);
}

uint32_t VulkanBackend::surfaceGetSupportedFormats(GfxSurface surface, GfxTextureFormat* formats, uint32_t maxFormats) const
{
    if (!surface) {
        return 0;
    }

    auto* surf = converter::toNative<Surface>(surface);
    auto surfaceFormats = surf->getSupportedFormats();

    // Convert to GfxTextureFormat
    if (formats && maxFormats > 0) {
        uint32_t copyCount = std::min(static_cast<uint32_t>(surfaceFormats.size()), maxFormats);
        for (uint32_t i = 0; i < copyCount; ++i) {
            formats[i] = converter::vkFormatToGfxFormat(surfaceFormats[i].format);
        }
    }

    return static_cast<uint32_t>(surfaceFormats.size());
}

uint32_t VulkanBackend::surfaceGetSupportedPresentModes(GfxSurface surface, GfxPresentMode* presentModes, uint32_t maxModes) const
{
    if (!surface) {
        return 0;
    }

    auto* surf = converter::toNative<Surface>(surface);
    auto vkPresentModes = surf->getSupportedPresentModes();

    // Convert to GfxPresentMode
    if (presentModes && maxModes > 0) {
        uint32_t copyCount = std::min(static_cast<uint32_t>(vkPresentModes.size()), maxModes);
        for (uint32_t i = 0; i < copyCount; ++i) {
            presentModes[i] = converter::vkPresentModeToGfxPresentMode(vkPresentModes[i]);
        }
    }

    return static_cast<uint32_t>(vkPresentModes.size());
}

// Swapchain functions
void VulkanBackend::swapchainDestroy(GfxSwapchain swapchain) const
{
    delete converter::toNative<Swapchain>(swapchain);
}

uint32_t VulkanBackend::swapchainGetWidth(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return 0;
    }
    auto* sc = converter::toNative<Swapchain>(swapchain);
    return sc->getWidth();
}

uint32_t VulkanBackend::swapchainGetHeight(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return 0;
    }
    auto* sc = converter::toNative<Swapchain>(swapchain);
    return sc->getHeight();
}

GfxTextureFormat VulkanBackend::swapchainGetFormat(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
    auto* sc = converter::toNative<Swapchain>(swapchain);
    return converter::vkFormatToGfxFormat(sc->getFormat());
}

uint32_t VulkanBackend::swapchainGetBufferCount(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return 0;
    }
    auto* sc = converter::toNative<Swapchain>(swapchain);
    return sc->getImageCount();
}

GfxResult VulkanBackend::swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const
{
    if (!swapchain || !outImageIndex) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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

GfxTextureView VulkanBackend::swapchainGetImageView(GfxSwapchain swapchain, uint32_t imageIndex) const
{
    if (!swapchain) {
        return nullptr;
    }

    auto* sc = converter::toNative<Swapchain>(swapchain);
    if (imageIndex >= sc->getImageCount()) {
        return nullptr;
    }

    return converter::toGfx<GfxTextureView>(sc->getTextureView(imageIndex));
}

GfxTextureView VulkanBackend::swapchainGetCurrentTextureView(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return nullptr;
    }

    auto* sc = converter::toNative<Swapchain>(swapchain);
    return converter::toGfx<GfxTextureView>(sc->getCurrentTextureView());
}

GfxResult VulkanBackend::swapchainPresent(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo) const
{
    if (!swapchain) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
void VulkanBackend::bufferDestroy(GfxBuffer buffer) const
{
    delete converter::toNative<Buffer>(buffer);
}

uint64_t VulkanBackend::bufferGetSize(GfxBuffer buffer) const
{
    if (!buffer) {
        return 0;
    }
    auto* buf = converter::toNative<Buffer>(buffer);
    return buf->size();
}

GfxBufferUsage VulkanBackend::bufferGetUsage(GfxBuffer buffer) const
{
    if (!buffer) {
        return static_cast<GfxBufferUsage>(0);
    }
    auto* buf = converter::toNative<Buffer>(buffer);
    return converter::vkBufferUsageToGfxBufferUsage(buf->getUsage());
}

GfxResult VulkanBackend::bufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer) const
{
    if (!buffer || !outMappedPointer) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    (void)offset;
    (void)size;

    auto* buf = converter::toNative<Buffer>(buffer);
    *outMappedPointer = buf->map();
    return GFX_RESULT_SUCCESS;
}

void VulkanBackend::bufferUnmap(GfxBuffer buffer) const
{
    if (!buffer) {
        return;
    }
    auto* buf = converter::toNative<Buffer>(buffer);
    buf->unmap();
}

// Texture functions
void VulkanBackend::textureDestroy(GfxTexture texture) const
{
    delete converter::toNative<Texture>(texture);
}

GfxExtent3D VulkanBackend::textureGetSize(GfxTexture texture) const
{
    if (!texture) {
        GfxExtent3D empty = { 0, 0, 0 };
        return empty;
    }
    auto* tex = converter::toNative<Texture>(texture);
    return converter::vkExtent3DToGfxExtent3D(tex->getSize());
}

GfxTextureFormat VulkanBackend::textureGetFormat(GfxTexture texture) const
{
    if (!texture) {
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
    auto* tex = converter::toNative<Texture>(texture);
    return converter::vkFormatToGfxFormat(tex->getFormat());
}

uint32_t VulkanBackend::textureGetMipLevelCount(GfxTexture texture) const
{
    if (!texture) {
        return 0;
    }
    auto* tex = converter::toNative<Texture>(texture);
    return tex->getMipLevelCount();
}

GfxSampleCount VulkanBackend::textureGetSampleCount(GfxTexture texture) const
{
    if (!texture) {
        return GFX_SAMPLE_COUNT_1;
    }
    auto* tex = converter::toNative<Texture>(texture);
    return converter::vkSampleCountToGfxSampleCount(tex->getSampleCount());
}

GfxTextureUsage VulkanBackend::textureGetUsage(GfxTexture texture) const
{
    if (!texture) {
        return static_cast<GfxTextureUsage>(0);
    }
    auto* tex = converter::toNative<Texture>(texture);
    return converter::vkImageUsageToGfxTextureUsage(tex->getUsage());
}

GfxTextureLayout VulkanBackend::textureGetLayout(GfxTexture texture) const
{
    if (!texture) {
        return GFX_TEXTURE_LAYOUT_UNDEFINED;
    }
    auto* tex = converter::toNative<Texture>(texture);
    return converter::vkImageLayoutToGfxLayout(tex->getLayout());
}

GfxResult VulkanBackend::textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView) const
{
    if (!texture || !outView) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
void VulkanBackend::textureViewDestroy(GfxTextureView textureView) const
{
    delete converter::toNative<TextureView>(textureView);
}

// Sampler functions
void VulkanBackend::samplerDestroy(GfxSampler sampler) const
{
    delete converter::toNative<Sampler>(sampler);
}

// Shader functions
void VulkanBackend::shaderDestroy(GfxShader shader) const
{
    delete converter::toNative<Shader>(shader);
}

// BindGroupLayout functions
void VulkanBackend::bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout) const
{
    delete converter::toNative<BindGroupLayout>(bindGroupLayout);
}

// BindGroup functions
void VulkanBackend::bindGroupDestroy(GfxBindGroup bindGroup) const
{
    delete converter::toNative<BindGroup>(bindGroup);
}

// RenderPipeline functions
void VulkanBackend::renderPipelineDestroy(GfxRenderPipeline renderPipeline) const
{
    delete converter::toNative<RenderPipeline>(renderPipeline);
}

// ComputePipeline functions
void VulkanBackend::computePipelineDestroy(GfxComputePipeline computePipeline) const
{
    delete converter::toNative<ComputePipeline>(computePipeline);
}

// Queue functions
GfxResult VulkanBackend::queueSubmit(GfxQueue queue, const GfxSubmitInfo* submitInfo) const
{
    if (!queue || !submitInfo) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* q = converter::toNative<Queue>(queue);
    auto internalSubmitInfo = converter::gfxDescriptorToSubmitInfo(submitInfo);
    VkResult result = q->submit(internalSubmitInfo);
    return (result == VK_SUCCESS) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

void VulkanBackend::queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size) const
{
    if (!queue || !buffer || !data) {
        return;
    }

    // Map, write, unmap
    auto* buf = converter::toNative<Buffer>(buffer);
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

    auto* q = converter::toNative<Queue>(queue);
    auto* tex = converter::toNative<Texture>(texture);

    q->writeTexture(tex, origin, mipLevel, data, dataSize, extent, finalLayout);

    (void)bytesPerRow; // Unused - assuming tightly packed data
}

GfxResult VulkanBackend::queueWaitIdle(GfxQueue queue) const
{
    if (!queue) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* q = converter::toNative<Queue>(queue);
    q->waitIdle();
    return GFX_RESULT_SUCCESS;
}

// CommandEncoder functions
void VulkanBackend::commandEncoderDestroy(GfxCommandEncoder commandEncoder) const
{
    delete converter::toNative<CommandEncoder>(commandEncoder);
}

GfxResult VulkanBackend::commandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder,
    const GfxRenderPassDescriptor* descriptor,
    GfxRenderPassEncoder* outRenderPass) const
{
    if (!commandEncoder || !outRenderPass || !descriptor) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    // Must have at least one attachment (color or depth)
    if (descriptor->colorAttachmentCount == 0 && !descriptor->depthStencilAttachment) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    // Validate all color attachments have valid targets
    for (uint32_t i = 0; i < descriptor->colorAttachmentCount; ++i) {
        if (!descriptor->colorAttachments[i].target.view) {
            return GFX_RESULT_ERROR_INVALID_PARAMETER;
        }
    }

    for (uint32_t i = 0; i < descriptor->colorAttachmentCount; ++i) {
        if (descriptor->colorAttachments[i].target.finalLayout == GFX_TEXTURE_LAYOUT_UNDEFINED) {
            return GFX_RESULT_ERROR_INVALID_PARAMETER;
        }
    }

    try {
        auto* encoderPtr = converter::toNative<CommandEncoder>(commandEncoder);
        auto createInfo = converter::gfxRenderPassDescriptorToCreateInfo(descriptor);
        RenderPassEncoder* renderPassEncoder = new RenderPassEncoder(encoderPtr, createInfo);
        *outRenderPass = converter::toGfx<GfxRenderPassEncoder>(renderPassEncoder);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception&) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult VulkanBackend::commandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const GfxComputePassDescriptor* descriptor, GfxComputePassEncoder* outComputePass) const
{
    if (!commandEncoder || !descriptor || !outComputePass) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* encoderPtr = converter::toNative<CommandEncoder>(commandEncoder);
        auto createInfo = converter::gfxComputePassDescriptorToCreateInfo(descriptor);
        ComputePassEncoder* computePassEncoder = new ComputePassEncoder(encoderPtr, createInfo);
        *outComputePass = converter::toGfx<GfxComputePassEncoder>(computePassEncoder);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception&) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void VulkanBackend::commandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset,
    GfxBuffer destination, uint64_t destinationOffset,
    uint64_t size) const
{
    if (!commandEncoder || !source || !destination) {
        return;
    }

    auto* enc = converter::toNative<CommandEncoder>(commandEncoder);
    auto* srcBuf = converter::toNative<Buffer>(source);
    auto* dstBuf = converter::toNative<Buffer>(destination);

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

    auto* enc = converter::toNative<CommandEncoder>(commandEncoder);
    auto* srcBuf = converter::toNative<Buffer>(source);
    auto* dstTex = converter::toNative<Texture>(destination);

    VkCommandBuffer cmdBuf = enc->handle();

    // Transition image layout to transfer dst optimal
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = dstTex->getLayout();
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = dstTex->handle();
    barrier.subresourceRange.aspectMask = converter::getImageAspectMask(dstTex->getFormat());
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
    region.imageSubresource.aspectMask = converter::getImageAspectMask(dstTex->getFormat());
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { origin->x, origin->y, origin->z };
    region.imageExtent = { extent->width, extent->height, extent->depth };

    vkCmdCopyBufferToImage(cmdBuf, srcBuf->handle(), dstTex->handle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition image layout to final layout
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = converter::gfxLayoutToVkImageLayout(finalLayout);
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = static_cast<VkAccessFlags>(gfxGetAccessFlagsForLayout(finalLayout));

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Update tracked layout
    dstTex->setLayout(converter::gfxLayoutToVkImageLayout(finalLayout));

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

    auto* enc = converter::toNative<CommandEncoder>(commandEncoder);
    auto* srcTex = converter::toNative<Texture>(source);
    auto* dstBuf = converter::toNative<Buffer>(destination);

    VkCommandBuffer cmdBuf = enc->handle();

    // Transition image layout to transfer src optimal
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = srcTex->getLayout();
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = srcTex->handle();
    barrier.subresourceRange.aspectMask = converter::getImageAspectMask(srcTex->getFormat());
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
    region.imageSubresource.aspectMask = converter::getImageAspectMask(srcTex->getFormat());
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { origin->x, origin->y, origin->z };
    region.imageExtent = { extent->width, extent->height, extent->depth };

    vkCmdCopyImageToBuffer(cmdBuf, srcTex->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstBuf->handle(), 1, &region);

    // Transition image layout to final layout
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = converter::gfxLayoutToVkImageLayout(finalLayout);
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = static_cast<VkAccessFlags>(gfxGetAccessFlagsForLayout(finalLayout));

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Update tracked layout
    srcTex->setLayout(converter::gfxLayoutToVkImageLayout(finalLayout));

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

    auto* enc = converter::toNative<CommandEncoder>(commandEncoder);
    auto* srcTex = converter::toNative<Texture>(source);
    auto* dstTex = converter::toNative<Texture>(destination);

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
    barriers[0].subresourceRange.aspectMask = converter::getImageAspectMask(srcTex->getFormat());
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
    barriers[1].subresourceRange.aspectMask = converter::getImageAspectMask(dstTex->getFormat());
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
    region.srcSubresource.aspectMask = converter::getImageAspectMask(srcTex->getFormat());
    region.srcSubresource.mipLevel = sourceMipLevel;
    region.srcSubresource.baseArrayLayer = is3DTexture ? 0 : sourceOrigin->z;
    region.srcSubresource.layerCount = layerCount;
    region.srcOffset = { sourceOrigin->x, sourceOrigin->y, is3DTexture ? sourceOrigin->z : 0 };
    region.dstSubresource.aspectMask = converter::getImageAspectMask(dstTex->getFormat());
    region.dstSubresource.mipLevel = destinationMipLevel;
    region.dstSubresource.baseArrayLayer = is3DTexture ? 0 : destinationOrigin->z;
    region.dstSubresource.layerCount = layerCount;
    region.dstOffset = { destinationOrigin->x, destinationOrigin->y, is3DTexture ? destinationOrigin->z : 0 };
    region.extent = { extent->width, extent->height, copyDepth };

    vkCmdCopyImage(cmdBuf, srcTex->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dstTex->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition images to final layouts
    barriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[0].newLayout = converter::gfxLayoutToVkImageLayout(srcFinalLayout);
    barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barriers[0].dstAccessMask = static_cast<VkAccessFlags>(gfxGetAccessFlagsForLayout(srcFinalLayout));

    barriers[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[1].newLayout = converter::gfxLayoutToVkImageLayout(dstFinalLayout);
    barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[1].dstAccessMask = static_cast<VkAccessFlags>(gfxGetAccessFlagsForLayout(dstFinalLayout));

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 2, barriers);

    // Update tracked layouts
    srcTex->setLayout(converter::gfxLayoutToVkImageLayout(srcFinalLayout));
    dstTex->setLayout(converter::gfxLayoutToVkImageLayout(dstFinalLayout));
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

    auto* encoder = converter::toNative<CommandEncoder>(commandEncoder);
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
        auto* buffer = converter::toNative<Buffer>(barrier.buffer);

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
        auto* texture = converter::toNative<Texture>(barrier.texture);

        VkImageMemoryBarrier vkBarrier{};
        vkBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        vkBarrier.image = texture->handle();

        // Determine aspect mask based on texture format
        VkFormat format = texture->getFormat();
        if (converter::isDepthFormat(format)) {
            vkBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (converter::hasStencilComponent(format)) {
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
        vkBarrier.oldLayout = converter::gfxLayoutToVkImageLayout(barrier.oldLayout);
        vkBarrier.newLayout = converter::gfxLayoutToVkImageLayout(barrier.newLayout);

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
        texture->setLayout(converter::gfxLayoutToVkImageLayout(barrier.newLayout));
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
    auto* encoder = converter::toNative<CommandEncoder>(commandEncoder);
    encoder->finish();
}

void VulkanBackend::commandEncoderBegin(GfxCommandEncoder commandEncoder) const
{
    if (!commandEncoder) {
        return;
    }
    auto* encoder = converter::toNative<CommandEncoder>(commandEncoder);
    encoder->reset();
}

// RenderPassEncoder functions
void VulkanBackend::renderPassEncoderDestroy(GfxRenderPassEncoder renderPassEncoder) const
{
    // Encoder is already deleted in renderPassEncoderEnd()
    // This is a no-op for safety if user calls destroy after end
    (void)renderPassEncoder;
}

void VulkanBackend::renderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline) const
{
    if (!renderPassEncoder || !pipeline) {
        return;
    }
    auto* rpe = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    auto* pipe = converter::toNative<RenderPipeline>(pipeline);

    VkCommandBuffer cmdBuf = rpe->handle();
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->handle());
    rpe->commandEncoder()->setCurrentPipelineLayout(pipe->layout());
}

void VulkanBackend::renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    if (!renderPassEncoder || !bindGroup) {
        return;
    }
    auto* rpe = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    auto* bg = converter::toNative<BindGroup>(bindGroup);

    VkCommandBuffer cmdBuf = rpe->handle();
    VkPipelineLayout layout = rpe->commandEncoder()->currentPipelineLayout();

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
    auto* rpe = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    auto* buf = converter::toNative<Buffer>(buffer);

    VkCommandBuffer cmdBuf = rpe->handle();
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
    auto* rpe = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    auto* buf = converter::toNative<Buffer>(buffer);

    VkCommandBuffer cmdBuf = rpe->handle();
    VkIndexType indexType = (format == GFX_INDEX_FORMAT_UINT16) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
    vkCmdBindIndexBuffer(cmdBuf, buf->handle(), offset, indexType);

    (void)size;
}

void VulkanBackend::renderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport) const
{
    if (!renderPassEncoder || !viewport) {
        return;
    }
    auto* rpe = converter::toNative<RenderPassEncoder>(renderPassEncoder);

    VkViewport vkViewport{};
    vkViewport.x = viewport->x;
    vkViewport.y = viewport->y;
    vkViewport.width = viewport->width;
    vkViewport.height = viewport->height;
    vkViewport.minDepth = viewport->minDepth;
    vkViewport.maxDepth = viewport->maxDepth;
    vkCmdSetViewport(rpe->handle(), 0, 1, &vkViewport);
}

void VulkanBackend::renderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor) const
{
    if (!renderPassEncoder || !scissor) {
        return;
    }
    auto* rpe = converter::toNative<RenderPassEncoder>(renderPassEncoder);

    VkRect2D vkScissor{};
    vkScissor.offset = { scissor->x, scissor->y };
    vkScissor.extent = { scissor->width, scissor->height };
    vkCmdSetScissor(rpe->handle(), 0, 1, &vkScissor);
}

void VulkanBackend::renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
{
    if (!renderPassEncoder) {
        return;
    }
    auto* rpe = converter::toNative<RenderPassEncoder>(renderPassEncoder);

    VkCommandBuffer cmdBuf = rpe->handle();
    vkCmdDraw(cmdBuf, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanBackend::renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) const
{
    if (!renderPassEncoder) {
        return;
    }
    auto* rpe = converter::toNative<RenderPassEncoder>(renderPassEncoder);

    VkCommandBuffer cmdBuf = rpe->handle();
    vkCmdDrawIndexed(cmdBuf, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}

void VulkanBackend::renderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder) const
{
    if (!renderPassEncoder) {
        return;
    }
    auto* rpe = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    vkCmdEndRenderPass(rpe->handle());
    delete rpe;
}

// ComputePassEncoder functions
void VulkanBackend::computePassEncoderDestroy(GfxComputePassEncoder computePassEncoder) const
{
    // Encoder is already deleted in computePassEncoderEnd()
    // This is a no-op for safety if user calls destroy after end
    (void)computePassEncoder;
}

void VulkanBackend::computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline) const
{
    if (!computePassEncoder || !pipeline) {
        return;
    }

    auto* cpe = converter::toNative<ComputePassEncoder>(computePassEncoder);
    auto* pipe = converter::toNative<ComputePipeline>(pipeline);

    VkCommandBuffer cmdBuf = cpe->handle();
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipe->handle());
    cpe->commandEncoder()->setCurrentPipelineLayout(pipe->layout());
}

void VulkanBackend::computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    if (!computePassEncoder || !bindGroup) {
        return;
    }

    auto* cpe = converter::toNative<ComputePassEncoder>(computePassEncoder);
    auto* bg = converter::toNative<BindGroup>(bindGroup);

    VkCommandBuffer cmdBuf = cpe->handle();
    VkPipelineLayout layout = cpe->commandEncoder()->currentPipelineLayout();

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

    auto* cpe = converter::toNative<ComputePassEncoder>(computePassEncoder);
    VkCommandBuffer cmdBuf = cpe->handle();
    vkCmdDispatch(cmdBuf, workgroupCountX, workgroupCountY, workgroupCountZ);
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
void VulkanBackend::fenceDestroy(GfxFence fence) const
{
    delete converter::toNative<Fence>(fence);
}

GfxResult VulkanBackend::fenceGetStatus(GfxFence fence, bool* isSignaled) const
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
void VulkanBackend::semaphoreDestroy(GfxSemaphore semaphore) const
{
    delete converter::toNative<Semaphore>(semaphore);
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }
    auto* s = converter::toNative<Semaphore>(semaphore);
    VkResult result = s->signal(value);
    return (result == VK_SUCCESS) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}
GfxResult VulkanBackend::semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs) const
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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

const IBackend* VulkanBackend::create()
{
    static VulkanBackend vulkanBackend;
    return &vulkanBackend;
}

} // namespace gfx::vulkan