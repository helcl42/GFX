#include "TextureView.h"

#include "Texture.h"

#include "../util/Utils.h"

#include <stdexcept>

namespace gfx::backend::vulkan::core {

TextureView::TextureView(Texture* texture, const TextureViewCreateInfo& createInfo)
    : m_device(texture->device())
    , m_texture(texture)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = texture->handle();
    viewInfo.viewType = createInfo.viewType;
    // Use texture's format if VK_FORMAT_UNDEFINED
    m_format = (createInfo.format == VK_FORMAT_UNDEFINED)
        ? texture->getFormat()
        : createInfo.format;
    viewInfo.format = m_format;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = getImageAspectMask(viewInfo.format);
    viewInfo.subresourceRange.baseMipLevel = createInfo.baseMipLevel;
    viewInfo.subresourceRange.levelCount = createInfo.mipLevelCount;
    viewInfo.subresourceRange.baseArrayLayer = createInfo.baseArrayLayer;
    viewInfo.subresourceRange.layerCount = createInfo.arrayLayerCount;

    VkResult result = vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageView);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image view");
    }
}

TextureView::~TextureView()
{
    if (m_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_imageView, nullptr);
    }
}

VkImageView TextureView::handle() const
{
    return m_imageView;
}

Texture* TextureView::getTexture() const
{
    return m_texture;
}

VkFormat TextureView::getFormat() const
{
    return m_format;
}

} // namespace gfx::backend::vulkan::core