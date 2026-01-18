#include "../resource/TextureView.h"

#include "../presentation/Swapchain.h"
#include "../resource/Texture.h"

#include <stdexcept>

namespace gfx::backend::webgpu::core {

// Constructor 1: Create view from Texture with TextureViewCreateInfo
TextureView::TextureView(Texture* texture, const TextureViewCreateInfo& createInfo)
    : m_texture(texture)
{
    WGPUTextureViewDescriptor desc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
    desc.dimension = createInfo.viewDimension;
    desc.format = createInfo.format;
    desc.baseMipLevel = createInfo.baseMipLevel;
    desc.mipLevelCount = createInfo.mipLevelCount;
    desc.baseArrayLayer = createInfo.baseArrayLayer;
    desc.arrayLayerCount = createInfo.arrayLayerCount;

    m_view = wgpuTextureCreateView(texture->handle(), &desc);
    if (!m_view) {
        throw std::runtime_error("Failed to create WebGPU texture view");
    }
}

// Constructor 2: Swapchain texture view (lazily acquires handle)
TextureView::TextureView(Swapchain* swapchain)
    : m_view(nullptr)
    , m_texture(nullptr)
    , m_swapchain(swapchain)
{
}

TextureView::~TextureView()
{
    if (m_view) {
        wgpuTextureViewRelease(m_view);
    }
}

WGPUTextureView TextureView::handle() const
{
    if (m_swapchain) {
        // Get the raw view handle from swapchain (created on-demand in acquireNextImage)
        return m_swapchain->getCurrentNativeTextureView();
    }
    return m_view;
}

Texture* TextureView::getTexture()
{
    return m_texture;
}

} // namespace gfx::backend::webgpu::core