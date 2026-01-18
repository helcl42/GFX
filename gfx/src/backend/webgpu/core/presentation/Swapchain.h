#ifndef GFX_WEBGPU_SWAPCHAIN_H
#define GFX_WEBGPU_SWAPCHAIN_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class Device;
class Surface;
class TextureView;

class Swapchain {
public:
    // Prevent copying
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    Swapchain(Device* device, Surface* surface, const SwapchainCreateInfo& createInfo);
    ~Swapchain();

    // Accessors
    WGPUSurface handle() const;
    WGPUDevice device() const;
    uint32_t getWidth() const;
    uint32_t getHeight() const;
    WGPUTextureFormat getFormat() const;
    WGPUPresentMode getPresentMode() const;
    uint32_t getImageCount() const;
    const SwapchainInfo& getInfo() const;

    // Swapchain operations (call in order: acquireNextImage -> getCurrentTextureView -> present)
    WGPUSurfaceGetCurrentTextureStatus acquireNextImage();
    TextureView* getCurrentTextureView();
    WGPUTextureView getCurrentNativeTextureView() const;
    void present();

private:
    static SwapchainInfo createSwapchainInfo(const SwapchainCreateInfo& createInfo);

private:
    Device* m_device = nullptr; // Non-owning
    WGPUSurface m_surface = nullptr; // Non-owning
    SwapchainInfo m_info{};
    WGPUTexture m_currentTexture = nullptr; // Current frame texture from surface
    WGPUTextureView m_currentRawView = nullptr; // Current frame raw view handle
    TextureView* m_currentView = nullptr; // Current frame view wrapper (owned)
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_SWAPCHAIN_H