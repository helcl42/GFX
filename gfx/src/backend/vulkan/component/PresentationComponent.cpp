#include "PresentationComponent.h"

#include "common/Logger.h"

#include "backend/vulkan/common/Common.h"
#include "backend/vulkan/converter/Conversions.h"
#include "backend/vulkan/validator/Validations.h"

#include "backend/vulkan/core/presentation/Surface.h"
#include "backend/vulkan/core/presentation/Swapchain.h"
#include "backend/vulkan/core/sync/Fence.h"
#include "backend/vulkan/core/sync/Semaphore.h"
#include "backend/vulkan/core/system/Device.h"

#include <algorithm>
#include <vector>

namespace gfx::backend::vulkan::component {

// Surface functions
GfxResult PresentationComponent::deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface) const
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

GfxResult PresentationComponent::surfaceDestroy(GfxSurface surface) const
{
    GfxResult validationResult = validator::validateSurfaceDestroy(surface);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Surface>(surface);
    return GFX_RESULT_SUCCESS;
}

GfxResult PresentationComponent::surfaceGetInfo(GfxSurface surface, GfxSurfaceInfo* outInfo) const
{
    GfxResult validationResult = validator::validateSurfaceGetInfo(surface, outInfo);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* surf = converter::toNative<core::Surface>(surface);
    *outInfo = converter::vkSurfaceCapabilitiesToGfxSurfaceInfo(surf->getCapabilities());
    return GFX_RESULT_SUCCESS;
}

GfxResult PresentationComponent::surfaceEnumerateSupportedFormats(GfxSurface surface, uint32_t* formatCount, GfxTextureFormat* formats) const
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

GfxResult PresentationComponent::surfaceEnumerateSupportedPresentModes(GfxSurface surface, uint32_t* presentModeCount, GfxPresentMode* presentModes) const
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
GfxResult PresentationComponent::deviceCreateSwapchain(GfxDevice device, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain) const
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

    auto* sc = converter::toNative<core::Swapchain>(swapchain);
    *outInfo = converter::vkSwapchainInfoToGfxSwapchainInfo(sc->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult PresentationComponent::swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const
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

GfxResult PresentationComponent::swapchainGetTextureView(GfxSwapchain swapchain, uint32_t imageIndex, GfxTextureView* outView) const
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

GfxResult PresentationComponent::swapchainGetCurrentTextureView(GfxSwapchain swapchain, GfxTextureView* outView) const
{
    GfxResult validationResult = validator::validateSwapchainGetCurrentTextureView(swapchain, outView);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* sc = converter::toNative<core::Swapchain>(swapchain);
    *outView = converter::toGfx<GfxTextureView>(sc->getCurrentTextureView());
    return GFX_RESULT_SUCCESS;
}

GfxResult PresentationComponent::swapchainPresent(GfxSwapchain swapchain, const GfxPresentDescriptor* presentDescriptor) const
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

} // namespace gfx::backend::vulkan::component
