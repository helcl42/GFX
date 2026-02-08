#include "Swapchain.h"

#include "Surface.h"

#include "../resource/TextureView.h"
#include "../system/Device.h"

#include "common/Logger.h"

#include <stdexcept>

namespace gfx::backend::webgpu::core {

Swapchain::Swapchain(Device* device, Surface* surface, const SwapchainCreateInfo& createInfo)
    : m_device(device)
    , m_surface(surface->handle())
    , m_info(createSwapchainInfo(createInfo))
{
    // Get surface capabilities
    const WGPUSurfaceCapabilities& capabilities = surface->getCapabilities();

    // Choose format
    WGPUTextureFormat selectedFormat = WGPUTextureFormat_Undefined;
    for (size_t i = 0; i < capabilities.formatCount; ++i) {
        if (capabilities.formats[i] == m_info.format) {
            selectedFormat = capabilities.formats[i];
            break;
        }
    }

    // If requested format not found, fall back to first available format
    if (selectedFormat == WGPUTextureFormat_Undefined && capabilities.formatCount > 0) {
        selectedFormat = capabilities.formats[0];
        gfx::common::Logger::instance().logWarning("[WebGPU Swapchain] Requested format {} not supported, using format {}", static_cast<int>(m_info.format), static_cast<int>(selectedFormat));
    }

    if (selectedFormat == WGPUTextureFormat_Undefined) {
        throw std::runtime_error("No supported surface formats available for swapchain");
    }

    m_info.format = selectedFormat;

    // Choose present mode
    WGPUPresentMode selectedPresentMode = WGPUPresentMode_Undefined;
    for (size_t i = 0; i < capabilities.presentModeCount; ++i) {
        if (capabilities.presentModes[i] == m_info.presentMode) {
            selectedPresentMode = capabilities.presentModes[i];
            break;
        }
    }

    // If requested present mode not found, fall back to first available mode
    if (selectedPresentMode == WGPUPresentMode_Undefined && capabilities.presentModeCount > 0) {
        selectedPresentMode = capabilities.presentModes[0];
        gfx::common::Logger::instance().logWarning("[WebGPU Swapchain] Requested present mode {} not supported, using mode {}", static_cast<int>(m_info.presentMode), static_cast<int>(selectedPresentMode));
    }

    if (selectedPresentMode == WGPUPresentMode_Undefined) {
        throw std::runtime_error("No supported present modes available for swapchain");
    }

    m_info.presentMode = selectedPresentMode;

    // Create stable TextureView wrapper ONCE (never deleted until swapchain destroyed)
    // This allows multiple framebuffers to reference the same TextureView*
    m_currentView = std::make_unique<TextureView>(this);

    // Configure surface for direct rendering
    WGPUSurfaceConfiguration config = WGPU_SURFACE_CONFIGURATION_INIT;
    config.device = m_device->handle();
    config.format = m_info.format;
    config.usage = createInfo.usage;
    config.width = m_info.width;
    config.height = m_info.height;
    config.presentMode = m_info.presentMode;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(m_surface, &config);
}

Swapchain::~Swapchain()
{
    // m_currentView automatically cleaned up by unique_ptr
    if (m_currentRawView) {
        wgpuTextureViewRelease(m_currentRawView);
    }
    if (m_currentTexture) {
        wgpuTextureRelease(m_currentTexture);
    }
}

// Accessors
WGPUSurface Swapchain::handle() const
{
    return m_surface;
}

WGPUDevice Swapchain::device() const
{
    return m_device->handle();
}

uint32_t Swapchain::getWidth() const
{
    return m_info.width;
}

uint32_t Swapchain::getHeight() const
{
    return m_info.height;
}

WGPUTextureFormat Swapchain::getFormat() const
{
    return m_info.format;
}

WGPUPresentMode Swapchain::getPresentMode() const
{
    return m_info.presentMode;
}

uint32_t Swapchain::getImageCount() const
{
    return m_info.imageCount;
}

const SwapchainInfo& Swapchain::getInfo() const
{
    return m_info;
}

// Swapchain operations (call in order: acquireNextImage -> getCurrentTextureView -> present)
WGPUSurfaceGetCurrentTextureStatus Swapchain::acquireNextImage()
{
    // Clean up previous frame's raw view handle only
    // Keep m_currentView wrapper stable for framebuffer references
    if (m_currentRawView) {
        wgpuTextureViewRelease(m_currentRawView);
        m_currentRawView = nullptr;
    }

    WGPUSurfaceTexture surfaceTexture = WGPU_SURFACE_TEXTURE_INIT;
    wgpuSurfaceGetCurrentTexture(m_surface, &surfaceTexture);

    if (surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal || surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
        if (m_currentTexture) {
            wgpuTextureRelease(m_currentTexture);
        }
        m_currentTexture = surfaceTexture.texture;

        // Create the view from the new texture
        m_currentRawView = wgpuTextureCreateView(m_currentTexture, nullptr);
        if (!m_currentRawView) {
            gfx::common::Logger::instance().logError("[WebGPU] Failed to create texture view");
        }
    } else if (surfaceTexture.texture) {
        wgpuTextureRelease(surfaceTexture.texture);
    }

    return surfaceTexture.status;
}

TextureView* Swapchain::getCurrentTextureView()
{
    // Just return the stable wrapper (created in constructor)
    // It will dynamically resolve to the current texture when handle() is called
    // No need to check m_currentTexture here - that's checked in handle()
    return m_currentView.get();
}

WGPUTextureView Swapchain::getCurrentNativeTextureView() const
{
    return m_currentRawView;
}

void Swapchain::present()
{
#ifndef GFX_HAS_EMSCRIPTEN
    wgpuSurfacePresent(m_surface);
#endif
    if (m_currentTexture) {
        wgpuTextureRelease(m_currentTexture);
        m_currentTexture = nullptr;
    }
}

SwapchainInfo Swapchain::createSwapchainInfo(const SwapchainCreateInfo& createInfo)
{
    SwapchainInfo info{};
    info.width = createInfo.width;
    info.height = createInfo.height;
    info.format = createInfo.format;
    info.imageCount = createInfo.imageCount;
    info.presentMode = createInfo.presentMode;
    return info;
}

} // namespace gfx::backend::webgpu::core