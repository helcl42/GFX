#ifndef GFX_VULKAN_TEXTUREVIEW_H
#define GFX_VULKAN_TEXTUREVIEW_H

#include "../CoreTypes.h"

namespace gfx::backend::vulkan::core {

class Texture;

class TextureView {
public:
    TextureView(const TextureView&) = delete;
    TextureView& operator=(const TextureView&) = delete;

    TextureView(Texture* texture, const TextureViewCreateInfo& createInfo);
    ~TextureView();

    VkImageView handle() const;
    Texture* getTexture() const;
    VkFormat getFormat() const;

private:
    VkDevice m_device = VK_NULL_HANDLE;
    Texture* m_texture = nullptr;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkFormat m_format; // View format (may differ from texture format)
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_TEXTUREVIEW_H