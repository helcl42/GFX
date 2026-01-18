#ifndef GFX_WEBGPU_TEXTURE_VIEW_H
#define GFX_WEBGPU_TEXTURE_VIEW_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class Swapchain;
class Texture;

class TextureView {
public:
    // Prevent copying
    TextureView(const TextureView&) = delete;
    TextureView& operator=(const TextureView&) = delete;

    // Constructor 2: Create view from Texture with TextureViewCreateInfo
    TextureView(Texture* texture, const TextureViewCreateInfo& createInfo);
    // Constructor 3: Create view for current swapchain image
    TextureView(Swapchain* swapchain);

    ~TextureView();

    WGPUTextureView handle() const;
    Texture* getTexture();

private:
    WGPUTextureView m_view = nullptr;
    Texture* m_texture = nullptr; // Non-owning
    Swapchain* m_swapchain = nullptr; // Non-owning
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_TEXTURE_VIEW_H