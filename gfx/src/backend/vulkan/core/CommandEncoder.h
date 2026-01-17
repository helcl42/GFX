#ifndef GFX_VULKAN_COMMANDENCODER_H
#define GFX_VULKAN_COMMANDENCODER_H

#include "CoreTypes.h"

namespace gfx::backend::vulkan::core {

class Device;

class CommandEncoder {
public:
    CommandEncoder(const CommandEncoder&) = delete;
    CommandEncoder& operator=(const CommandEncoder&) = delete;

    CommandEncoder(Device* device);
    ~CommandEncoder();

    VkCommandBuffer handle() const;
    VkDevice device() const;
    Device* getDevice() const;
    VkPipelineLayout currentPipelineLayout() const;
    void setCurrentPipelineLayout(VkPipelineLayout layout);

    void begin();
    void end();
    void reset();

    void pipelineBarrier(const MemoryBarrier* memoryBarriers, uint32_t memoryBarrierCount, const BufferBarrier* bufferBarriers, uint32_t bufferBarrierCount, const TextureBarrier* textureBarriers, uint32_t textureBarrierCount);

    void copyBufferToBuffer(Buffer* source, uint64_t sourceOffset, Buffer* destination, uint64_t destinationOffset, uint64_t size);
    void copyBufferToTexture(Buffer* source, uint64_t sourceOffset, Texture* destination, VkOffset3D origin, VkExtent3D extent, uint32_t mipLevel, VkImageLayout finalLayout);
    void copyTextureToBuffer(Texture* source, VkOffset3D origin, uint32_t mipLevel, Buffer* destination, uint64_t destinationOffset, VkExtent3D extent, VkImageLayout finalLayout);
    void copyTextureToTexture(Texture* source, VkOffset3D sourceOrigin, uint32_t sourceMipLevel, VkImageLayout srcFinalLayout, Texture* destination, VkOffset3D destinationOrigin, uint32_t destinationMipLevel, VkImageLayout dstFinalLayout, VkExtent3D extent);
    void blitTextureToTexture(Texture* source, VkOffset3D sourceOrigin, VkExtent3D sourceExtent, uint32_t sourceMipLevel, VkImageLayout srcFinalLayout, Texture* destination, VkOffset3D destinationOrigin, VkExtent3D destinationExtent, uint32_t destinationMipLevel, VkImageLayout dstFinalLayout, VkFilter filter);

private:
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    Device* m_device = nullptr;
    bool m_isRecording = false;
    VkPipelineLayout m_currentPipelineLayout = VK_NULL_HANDLE;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_COMMANDENCODER_H