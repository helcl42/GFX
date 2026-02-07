#include "DeviceComponent.h"

#include "backend/vulkan/common/Common.h"
#include "common/Logger.h"
#include "backend/vulkan/converter/Conversions.h"
#include "backend/vulkan/validator/Validations.h"

#include "backend/vulkan/core/presentation/Surface.h"
#include "backend/vulkan/core/presentation/Swapchain.h"
#include "backend/vulkan/core/system/Device.h"
#include "backend/vulkan/core/system/Queue.h"
#include "backend/vulkan/core/sync/Fence.h"
#include "backend/vulkan/core/sync/Semaphore.h"
#include "backend/vulkan/core/resource/TextureView.h"

#include <stdexcept>
#include <vector>

namespace gfx::backend::vulkan::component {

// Device functions
GfxResult DeviceComponent::deviceDestroy(GfxDevice device) const
{
    GfxResult validationResult = validator::validateDeviceDestroy(device);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Device>(device);
    return GFX_RESULT_SUCCESS;
}

GfxResult DeviceComponent::deviceGetQueue(GfxDevice device, GfxQueue* outQueue) const
{
    GfxResult validationResult = validator::validateDeviceGetQueue(device, outQueue);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* dev = converter::toNative<core::Device>(device);
    *outQueue = converter::toGfx<GfxQueue>(dev->getQueue());
    return GFX_RESULT_SUCCESS;
}

GfxResult DeviceComponent::deviceGetQueueByIndex(GfxDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, GfxQueue* outQueue) const
{
    GfxResult validationResult = validator::validateDeviceGetQueueByIndex(device, outQueue);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* dev = converter::toNative<core::Device>(device);
    auto* queue = dev->getQueueByIndex(queueFamilyIndex, queueIndex);

    if (!queue) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }

    *outQueue = converter::toGfx<GfxQueue>(queue);
    return GFX_RESULT_SUCCESS;
}

GfxResult DeviceComponent::deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface) const
{
    GfxResult validationResult = validator::validateDeviceCreateSurface(device, descriptor, outSurface);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

#ifdef GFX_HEADLESS_BUILD
    (void)device;
    (void)descriptor;
    (void)outSurface;
    gfx::common::Logger::instance().logError("Surface creation is not available in headless builds");
    return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
#else
    try {
        auto* dev = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToSurfaceCreateInfo(descriptor);
        auto* surface = new core::Surface(dev->getAdapter(), createInfo);
        *outSurface = converter::toGfx<GfxSurface>(surface);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create surface: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
#endif
}

GfxResult DeviceComponent::deviceCreateSwapchain(GfxDevice device, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain) const
{
    GfxResult validationResult = validator::validateDeviceCreateSwapchain(device, descriptor, outSwapchain);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* dev = converter::toNative<core::Device>(device);
        auto* surf = converter::toNative<core::Surface>(descriptor->surface);
        auto createInfo = converter::gfxDescriptorToSwapchainCreateInfo(descriptor);
        auto* swapchain = new core::Swapchain(dev, surf, createInfo);
        *outSwapchain = converter::toGfx<GfxSwapchain>(swapchain);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create swapchain: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult DeviceComponent::deviceWaitIdle(GfxDevice device) const
{
    GfxResult validationResult = validator::validateDeviceWaitIdle(device);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* dev = converter::toNative<core::Device>(device);
    dev->waitIdle();
    return GFX_RESULT_SUCCESS;
}

GfxResult DeviceComponent::deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits) const
{
    GfxResult validationResult = validator::validateDeviceGetLimits(device, outLimits);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* dev = converter::toNative<core::Device>(device);
    *outLimits = converter::vkPropertiesToGfxDeviceLimits(dev->getProperties());
    return GFX_RESULT_SUCCESS;
}

GfxResult DeviceComponent::deviceSupportsShaderFormat(GfxDevice device, GfxShaderSourceType format, bool* outSupported) const
{
    if (!device || !outSupported) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* devicePtr = converter::toNative<core::Device>(device);
    auto internalFormat = converter::gfxShaderSourceTypeToVulkanShaderSourceType(format);
    *outSupported = devicePtr->supportsShaderFormat(internalFormat);
    return GFX_RESULT_SUCCESS;
}

// Surface functions
GfxResult DeviceComponent::surfaceDestroy(GfxSurface surface) const
{
    GfxResult validationResult = validator::validateSurfaceDestroy(surface);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Surface>(surface);
    return GFX_RESULT_SUCCESS;
}

GfxResult DeviceComponent::surfaceEnumerateSupportedFormats(GfxSurface surface, uint32_t* formatCount, GfxTextureFormat* formats) const
{
    GfxResult validationResult = validator::validateSurfaceEnumerateSupportedFormats(surface, formatCount);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* surf = converter::toNative<core::Surface>(surface);
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

GfxResult DeviceComponent::surfaceEnumerateSupportedPresentModes(GfxSurface surface, uint32_t* presentModeCount, GfxPresentMode* presentModes) const
{
    GfxResult validationResult = validator::validateSurfaceEnumerateSupportedPresentModes(surface, presentModeCount);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* surf = converter::toNative<core::Surface>(surface);
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
GfxResult DeviceComponent::swapchainDestroy(GfxSwapchain swapchain) const
{
    GfxResult validationResult = validator::validateSwapchainDestroy(swapchain);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Swapchain>(swapchain);
    return GFX_RESULT_SUCCESS;
}

GfxResult DeviceComponent::swapchainGetInfo(GfxSwapchain swapchain, GfxSwapchainInfo* outInfo) const
{
    GfxResult validationResult = validator::validateSwapchainGetInfo(swapchain, outInfo);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* sc = converter::toNative<core::Swapchain>(swapchain);
    *outInfo = converter::vkSwapchainInfoToGfxSwapchainInfo(sc->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult DeviceComponent::swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const
{
    GfxResult validationResult = validator::validateSwapchainAcquireNextImage(swapchain, outImageIndex);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* sc = converter::toNative<core::Swapchain>(swapchain);

    VkSemaphore vkSemaphore = VK_NULL_HANDLE;
    if (imageAvailableSemaphore) {
        auto* sem = converter::toNative<core::Semaphore>(imageAvailableSemaphore);
        vkSemaphore = sem->handle();
    }

    VkFence vkFence = VK_NULL_HANDLE;
    if (fence) {
        auto* f = converter::toNative<core::Fence>(fence);
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

GfxResult DeviceComponent::swapchainGetTextureView(GfxSwapchain swapchain, uint32_t imageIndex, GfxTextureView* outView) const
{
    GfxResult validationResult = validator::validateSwapchainGetTextureView(swapchain, outView);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* sc = converter::toNative<core::Swapchain>(swapchain);
    if (imageIndex >= sc->getImageCount()) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    *outView = converter::toGfx<GfxTextureView>(sc->getTextureView(imageIndex));
    return GFX_RESULT_SUCCESS;
}

GfxResult DeviceComponent::swapchainGetCurrentTextureView(GfxSwapchain swapchain, GfxTextureView* outView) const
{
    GfxResult validationResult = validator::validateSwapchainGetCurrentTextureView(swapchain, outView);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* sc = converter::toNative<core::Swapchain>(swapchain);
    *outView = converter::toGfx<GfxTextureView>(sc->getCurrentTextureView());
    return GFX_RESULT_SUCCESS;
}

GfxResult DeviceComponent::swapchainPresent(GfxSwapchain swapchain, const GfxPresentDescriptor* presentDescriptor) const
{
    GfxResult validationResult = validator::validateSwapchainPresent(swapchain, presentDescriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* sc = converter::toNative<core::Swapchain>(swapchain);

    std::vector<VkSemaphore> waitSemaphores;
    if (presentDescriptor && presentDescriptor->waitSemaphoreCount > 0) {
        waitSemaphores.reserve(presentDescriptor->waitSemaphoreCount);
        for (uint32_t i = 0; i < presentDescriptor->waitSemaphoreCount; ++i) {
            auto* sem = converter::toNative<core::Semaphore>(presentDescriptor->waitSemaphores[i]);
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

// Queue functions
GfxResult DeviceComponent::queueSubmit(GfxQueue queue, const GfxSubmitDescriptor* submitDescriptor) const
{
    GfxResult validationResult = validator::validateQueueSubmit(queue, submitDescriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* q = converter::toNative<core::Queue>(queue);
    auto internalSubmitInfo = converter::gfxDescriptorToSubmitInfo(submitDescriptor);
    VkResult result = q->submit(internalSubmitInfo);
    return (result == VK_SUCCESS) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

GfxResult DeviceComponent::queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size) const
{
    GfxResult validationResult = validator::validateQueueWriteBuffer(queue, buffer, data);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* q = converter::toNative<core::Queue>(queue);
    auto* buf = converter::toNative<core::Buffer>(buffer);
    q->writeBuffer(buf, offset, data, size);
    return GFX_RESULT_SUCCESS;
}

GfxResult DeviceComponent::queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel, const void* data, uint64_t dataSize, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
{
    GfxResult validationResult = validator::validateQueueWriteTexture(queue, texture, origin, extent, data);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* q = converter::toNative<core::Queue>(queue);
    auto* tex = converter::toNative<core::Texture>(texture);

    VkOffset3D vkOrigin = origin ? converter::gfxOrigin3DToVkOffset3D(origin) : VkOffset3D{ 0, 0, 0 };
    VkExtent3D vkExtent = converter::gfxExtent3DToVkExtent3D(extent);
    VkImageLayout vkLayout = converter::gfxLayoutToVkImageLayout(finalLayout);

    q->writeTexture(tex, vkOrigin, mipLevel, data, dataSize, vkExtent, vkLayout);

    return GFX_RESULT_SUCCESS;
}

GfxResult DeviceComponent::queueWaitIdle(GfxQueue queue) const
{
    GfxResult validationResult = validator::validateQueueWaitIdle(queue);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* q = converter::toNative<core::Queue>(queue);
    q->waitIdle();
    return GFX_RESULT_SUCCESS;
}

} // namespace gfx::backend::vulkan::component
