#ifndef GFX_VULKAN_TEXTURE_H
#define GFX_VULKAN_TEXTURE_H

#include "../CoreTypes.h"

namespace gfx::backend::vulkan::core {

class CommandEncoder;
class Device;

class Texture {
public:
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Device* device, const TextureCreateInfo& createInfo);
    Texture(Device* device, VkImage image, const TextureCreateInfo& createInfo);
    Texture(Device* device, VkImage image, const TextureImportInfo& importInfo);
    ~Texture();

    VkImage handle() const;
    VkDevice device() const;
    VkImageType getImageType() const;
    VkExtent3D getSize() const;
    uint32_t getArrayLayers() const;
    VkFormat getFormat() const;
    uint32_t getMipLevelCount() const;
    VkSampleCountFlagBits getSampleCount() const;
    VkImageUsageFlags getUsage() const;
    const TextureInfo& getInfo() const;

    VkImageLayout getLayout() const;
    void setLayout(VkImageLayout layout);

    void transitionLayout(CommandEncoder* encoder, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount);
    void transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount);
    void generateMipmaps(CommandEncoder* encoder);
    void generateMipmapsRange(CommandEncoder* encoder, uint32_t baseMipLevel, uint32_t levelCount);

private:
    // Internal layout transition with explicit old layout (for mipmap generation)
    void transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount);
    
    static TextureInfo createTextureInfo(const TextureCreateInfo& info);
    static TextureInfo createTextureInfo(const TextureImportInfo& info);

    Device* m_device = nullptr;
    bool m_ownsResources = true;
    TextureInfo m_info{};
    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkImageLayout m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_TEXTURE_H