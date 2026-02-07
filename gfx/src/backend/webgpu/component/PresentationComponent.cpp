#include "PresentationComponent.h"

#include "common/Logger.h"

#include "../common/Common.h"
#include "../converter/Conversions.h"
#include "../validator/Validations.h"

#include "../core/presentation/Surface.h"
#include "../core/presentation/Swapchain.h"
#include "../core/resource/TextureView.h"
#include "../core/sync/Fence.h"
#include "../core/sync/Semaphore.h"
#include "../core/system/Adapter.h"
#include "../core/system/Device.h"
#include "../core/system/Instance.h"

#include <stdexcept>

namespace gfx::backend::webgpu::component {

// Surface functions
GfxResult PresentationComponent::deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface) const
{
#ifdef GFX_HEADLESS_BUILD
    (void)device;
    (void)descriptor;
    (void)outSurface;
    gfx::common::Logger::instance().logError("Surface creation is not available in headless builds");
    return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
#else
    GfxResult validationResult = validator::validateDeviceCreateSurface(device, descriptor, outSurface);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUSurfaceCreateInfo(descriptor);
        auto* surface = new core::Surface(devicePtr->getAdapter()->getInstance()->handle(), devicePtr->getAdapter()->handle(), createInfo);
        *outSurface = converter::toGfx<GfxSurface>(surface);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create surface: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
#endif
}

GfxResult PresentationComponent::surfaceDestroy(GfxSurface surface) const
{
    GfxResult validationResult = validator::validateSurfaceDestroy(surface);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Surface>(surface);
    return GFX_RESULT_SUCCESS;
}

GfxResult PresentationComponent::surfaceEnumerateSupportedFormats(GfxSurface surface, uint32_t* formatCount, GfxTextureFormat* formats) const
{
    GfxResult validationResult = validator::validateSurfaceEnumerateSupportedFormats(surface, formatCount);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* surf = converter::toNative<core::Surface>(surface);

    // Query surface capabilities
    WGPUSurfaceCapabilities capabilities = surf->getCapabilities();

    uint32_t count = static_cast<uint32_t>(capabilities.formatCount);

    if (count == 0) {
        wgpuSurfaceCapabilitiesFreeMembers(capabilities);
        *formatCount = 0;
        return GFX_RESULT_SUCCESS;
    }

    // Convert to GfxTextureFormat
    if (formats) {
        uint32_t copyCount = std::min(count, *formatCount);
        for (uint32_t i = 0; i < copyCount; ++i) {
            formats[i] = converter::wgpuFormatToGfxFormat(capabilities.formats[i]);
        }
    }

    wgpuSurfaceCapabilitiesFreeMembers(capabilities);
    *formatCount = count;
    return GFX_RESULT_SUCCESS;
}

GfxResult PresentationComponent::surfaceEnumerateSupportedPresentModes(GfxSurface surface, uint32_t* presentModeCount, GfxPresentMode* presentModes) const
{
    GfxResult validationResult = validator::validateSurfaceEnumerateSupportedPresentModes(surface, presentModeCount);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* surf = converter::toNative<core::Surface>(surface);

    // Query surface capabilities
    WGPUSurfaceCapabilities capabilities = surf->getCapabilities();

    uint32_t count = static_cast<uint32_t>(capabilities.presentModeCount);

    if (count == 0) {
        wgpuSurfaceCapabilitiesFreeMembers(capabilities);
        *presentModeCount = 0;
        return GFX_RESULT_SUCCESS;
    }

    // Convert to GfxPresentMode
    if (presentModes) {
        uint32_t copyCount = std::min(count, *presentModeCount);
        for (uint32_t i = 0; i < copyCount; ++i) {
            presentModes[i] = converter::wgpuPresentModeToGfxPresentMode(capabilities.presentModes[i]);
        }
    }

    wgpuSurfaceCapabilitiesFreeMembers(capabilities);
    *presentModeCount = count;
    return GFX_RESULT_SUCCESS;
}

// Swapchain functions
GfxResult PresentationComponent::deviceCreateSwapchain(GfxDevice device, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain) const
{
    GfxResult validationResult = validator::validateDeviceCreateSwapchain(device, descriptor, outSwapchain);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto* surfacePtr = converter::toNative<core::Surface>(descriptor->surface);
        auto createInfo = converter::gfxDescriptorToWebGPUSwapchainCreateInfo(descriptor);
        auto* swapchain = new core::Swapchain(devicePtr, surfacePtr, createInfo);
        *outSwapchain = converter::toGfx<GfxSwapchain>(swapchain);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create swapchain: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult PresentationComponent::swapchainDestroy(GfxSwapchain swapchain) const
{
    GfxResult validationResult = validator::validateSwapchainDestroy(swapchain);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Swapchain>(swapchain);
    return GFX_RESULT_SUCCESS;
}

GfxResult PresentationComponent::swapchainGetInfo(GfxSwapchain swapchain, GfxSwapchainInfo* outInfo) const
{
    GfxResult validationResult = validator::validateSwapchainGetInfo(swapchain, outInfo);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* swapchainPtr = converter::toNative<core::Swapchain>(swapchain);
    *outInfo = converter::wgpuSwapchainInfoToGfxSwapchainInfo(swapchainPtr->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult PresentationComponent::swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const
{
    GfxResult validationResult = validator::validateSwapchainAcquireNextImage(swapchain, outImageIndex);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    // WebGPU doesn't have explicit acquire semantics with semaphores
    // The surface texture is acquired implicitly when we call wgpuSurfaceGetCurrentTexture
    // For now, we just return image index 0 (WebGPU doesn't expose multiple image indices)
    // The semaphore and fence are noted but WebGPU doesn't support explicit synchronization primitives

    (void)timeoutNs;
    (void)imageAvailableSemaphore; // WebGPU doesn't support explicit semaphore signaling

    auto* swapchainPtr = converter::toNative<core::Swapchain>(swapchain);

    WGPUSurfaceGetCurrentTextureStatus status = swapchainPtr->acquireNextImage();

    GfxResult result = GFX_RESULT_SUCCESS;
    switch (status) {
    case WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal:
    case WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal:
        *outImageIndex = 0; // WebGPU only exposes current image
        result = GFX_RESULT_SUCCESS;
        break;
    case WGPUSurfaceGetCurrentTextureStatus_Timeout:
        result = GFX_RESULT_TIMEOUT;
        break;
    case WGPUSurfaceGetCurrentTextureStatus_Outdated:
        result = GFX_RESULT_ERROR_OUT_OF_DATE;
        break;
    case WGPUSurfaceGetCurrentTextureStatus_Lost:
        result = GFX_RESULT_ERROR_SURFACE_LOST;
        break;
    default:
        result = GFX_RESULT_ERROR_UNKNOWN;
        break;
    }

    // Signal fence if provided (even though WebGPU doesn't truly have fences)
    if (fence && result == GFX_RESULT_SUCCESS) {
        auto* fencePtr = converter::toNative<core::Fence>(fence);
        fencePtr->signal();
    }

    return result;
}

GfxResult PresentationComponent::swapchainGetTextureView(GfxSwapchain swapchain, uint32_t imageIndex, GfxTextureView* outView) const
{
    GfxResult validationResult = validator::validateSwapchainGetTextureView(swapchain, outView);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    // WebGPU doesn't expose multiple swapchain images by index
    // Always return the current texture view regardless of index
    (void)imageIndex;

    return swapchainGetCurrentTextureView(swapchain, outView);
}

GfxResult PresentationComponent::swapchainGetCurrentTextureView(GfxSwapchain swapchain, GfxTextureView* outView) const
{
    GfxResult validationResult = validator::validateSwapchainGetCurrentTextureView(swapchain, outView);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* swapchainPtr = converter::toNative<core::Swapchain>(swapchain);
    *outView = converter::toGfx<GfxTextureView>(swapchainPtr->getCurrentTextureView());
    return GFX_RESULT_SUCCESS;
}

GfxResult PresentationComponent::swapchainPresent(GfxSwapchain swapchain, const GfxPresentDescriptor* presentDescriptor) const
{
    GfxResult validationResult = validator::validateSwapchainPresent(swapchain, presentDescriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    // WebGPU doesn't support explicit wait semaphores for present
    // The queue submission already ensures ordering, so we just present
    // However, we signal the semaphores for API consistency
    if (presentDescriptor && presentDescriptor->waitSemaphoreCount > 0) {
        for (uint32_t i = 0; i < presentDescriptor->waitSemaphoreCount; ++i) {
            if (presentDescriptor->waitSemaphores[i]) {
                auto* sem = converter::toNative<core::Semaphore>(presentDescriptor->waitSemaphores[i]);
                sem->signal();
            }
        }
    }

    auto* swapchainPtr = converter::toNative<core::Swapchain>(swapchain);
    swapchainPtr->present();
    return GFX_RESULT_SUCCESS;
}

} // namespace gfx::backend::webgpu::component
