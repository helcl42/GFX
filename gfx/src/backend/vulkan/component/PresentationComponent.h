#ifndef GFX_BACKEND_VULKAN_PRESENTATION_COMPONENT_H
#define GFX_BACKEND_VULKAN_PRESENTATION_COMPONENT_H

#include <gfx/gfx.h>

namespace gfx::backend::vulkan::component {

class PresentationComponent {
public:
    // Surface functions
    GfxResult deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface) const;
    GfxResult surfaceDestroy(GfxSurface surface) const;
    GfxResult surfaceGetInfo(GfxSurface surface, GfxSurfaceInfo* outInfo) const;
    GfxResult surfaceEnumerateSupportedFormats(GfxSurface surface, uint32_t* formatCount, GfxFormat* formats) const;
    GfxResult surfaceEnumerateSupportedPresentModes(GfxSurface surface, uint32_t* presentModeCount, GfxPresentMode* presentModes) const;

    // Swapchain functions
    GfxResult deviceCreateSwapchain(GfxDevice device, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain) const;
    GfxResult swapchainDestroy(GfxSwapchain swapchain) const;
    GfxResult swapchainGetInfo(GfxSwapchain swapchain, GfxSwapchainInfo* outInfo) const;
    GfxResult swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const;
    GfxResult swapchainGetTextureView(GfxSwapchain swapchain, uint32_t imageIndex, GfxTextureView* outView) const;
    GfxResult swapchainGetCurrentTextureView(GfxSwapchain swapchain, GfxTextureView* outView) const;
    GfxResult swapchainPresent(GfxSwapchain swapchain, const GfxPresentDescriptor* presentDescriptor) const;
};

} // namespace gfx::backend::vulkan::component

#endif // GFX_BACKEND_VULKAN_PRESENTATION_COMPONENT_H
