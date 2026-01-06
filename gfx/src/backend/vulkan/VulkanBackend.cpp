
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

void VulkanBackend::adapterGetInfo(GfxAdapter adapter, GfxAdapterInfo* outInfo) const
{
    if (!adapter || !outInfo) {
        return;
    }

    auto* adap = converter::toNative<Adapter>(adapter);
    *outInfo = converter::vkPropertiesToGfxAdapterInfo(adap->getProperties());
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

GfxResult VulkanBackend::deviceImportBuffer(GfxDevice device, const GfxExternalBufferDescriptor* descriptor, GfxBuffer* outBuffer) const
{
    if (!device || !descriptor || !outBuffer || !descriptor->nativeHandle) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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

GfxResult VulkanBackend::deviceImportTexture(GfxDevice device, const GfxExternalTextureDescriptor* descriptor, GfxTexture* outTexture) const
{
    if (!device || !descriptor || !outTexture || !descriptor->nativeHandle) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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

uint32_t VulkanBackend::swapchainGetImageCount(GfxSwapchain swapchain) const
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

    auto* q = converter::toNative<Queue>(queue);
    auto* buf = converter::toNative<Buffer>(buffer);
    q->writeBuffer(buf, offset, data, size);
}

void VulkanBackend::queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
    const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
{
    if (!queue || !texture || !data || !extent || dataSize == 0) {
        return;
    }

    auto* q = converter::toNative<Queue>(queue);
    auto* tex = converter::toNative<Texture>(texture);

    VkOffset3D vkOrigin = origin ? converter::gfxOrigin3DToVkOffset3D(origin) : VkOffset3D{ 0, 0, 0 };
    VkExtent3D vkExtent = converter::gfxExtent3DToVkExtent3D(extent);
    VkImageLayout vkLayout = converter::gfxLayoutToVkImageLayout(finalLayout);

    q->writeTexture(tex, vkOrigin, mipLevel, data, dataSize, vkExtent, vkLayout);

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
        auto* renderPassEncoder = new RenderPassEncoder(encoderPtr, createInfo);
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
        auto* computePassEncoder = new ComputePassEncoder(encoderPtr, createInfo);
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

    enc->copyBufferToBuffer(srcBuf, sourceOffset, dstBuf, destinationOffset, size);
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

    VkOffset3D vkOrigin = converter::gfxOrigin3DToVkOffset3D(origin);
    VkExtent3D vkExtent = converter::gfxExtent3DToVkExtent3D(extent);
    VkImageLayout vkLayout = converter::gfxLayoutToVkImageLayout(finalLayout);

    enc->copyBufferToTexture(srcBuf, sourceOffset, dstTex, vkOrigin, vkExtent, mipLevel, vkLayout);

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

    VkOffset3D vkOrigin = converter::gfxOrigin3DToVkOffset3D(origin);
    VkExtent3D vkExtent = converter::gfxExtent3DToVkExtent3D(extent);
    VkImageLayout vkLayout = converter::gfxLayoutToVkImageLayout(finalLayout);

    enc->copyTextureToBuffer(srcTex, vkOrigin, mipLevel, dstBuf, destinationOffset, vkExtent, vkLayout);

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

    VkOffset3D vkSrcOrigin = converter::gfxOrigin3DToVkOffset3D(sourceOrigin);
    VkOffset3D vkDstOrigin = converter::gfxOrigin3DToVkOffset3D(destinationOrigin);
    VkExtent3D vkExtent = converter::gfxExtent3DToVkExtent3D(extent);
    VkImageLayout vkSrcLayout = converter::gfxLayoutToVkImageLayout(srcFinalLayout);
    VkImageLayout vkDstLayout = converter::gfxLayoutToVkImageLayout(dstFinalLayout);

    enc->copyTextureToTexture(srcTex, vkSrcOrigin, sourceMipLevel,
        dstTex, vkDstOrigin, destinationMipLevel,
        vkExtent, vkSrcLayout, vkDstLayout);
}

// TODO - add member function to CommandEncoder for pipeline barrier
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