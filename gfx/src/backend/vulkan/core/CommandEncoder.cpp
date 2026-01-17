#include "CommandEncoder.h"

#include "Buffer.h"
#include "Device.h"
#include "Queue.h"
#include "Texture.h"

#include "../converter/Conversions.h"

#include <stdexcept>

namespace gfx::backend::vulkan::core {

CommandEncoder::CommandEncoder(Device* device)
    : m_device(device)
{
    // Create command pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = device->getQueue()->family();

    VkResult result = vkCreateCommandPool(m_device->handle(), &poolInfo, nullptr, &m_commandPool);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }

    // Allocate command buffer
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(m_device->handle(), &allocInfo, &m_commandBuffer);
    if (result != VK_SUCCESS) {
        vkDestroyCommandPool(m_device->handle(), m_commandPool, nullptr);
        throw std::runtime_error("Failed to allocate command buffer");
    }

    // Begin recording
    begin();
}

CommandEncoder::~CommandEncoder()
{
    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_device->handle(), m_commandPool, nullptr);
    }
}

VkCommandBuffer CommandEncoder::handle() const
{
    return m_commandBuffer;
}

VkDevice CommandEncoder::device() const
{
    return m_device->handle();
}

Device* CommandEncoder::getDevice() const
{
    return m_device;
}

VkPipelineLayout CommandEncoder::currentPipelineLayout() const
{
    return m_currentPipelineLayout;
}

void CommandEncoder::setCurrentPipelineLayout(VkPipelineLayout layout)
{
    m_currentPipelineLayout = layout;
}

void CommandEncoder::begin()
{
    if (!m_isRecording) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
        m_isRecording = true;
    }
}

void CommandEncoder::end()
{
    if (m_isRecording) {
        vkEndCommandBuffer(m_commandBuffer);
        m_isRecording = false;
    }
}

void CommandEncoder::reset()
{
    m_currentPipelineLayout = VK_NULL_HANDLE;

    // Reset the command pool (this implicitly resets all command buffers)
    vkResetCommandPool(m_device->handle(), m_commandPool, 0);

    // Mark as not recording since the command buffer was reset
    m_isRecording = false;

    // Begin recording again
    begin();
}

void CommandEncoder::pipelineBarrier(const MemoryBarrier* memoryBarriers, uint32_t memoryBarrierCount, const BufferBarrier* bufferBarriers, uint32_t bufferBarrierCount, const TextureBarrier* textureBarriers, uint32_t textureBarrierCount)
{
    std::vector<VkMemoryBarrier> memBarriers;
    memBarriers.reserve(memoryBarrierCount);

    std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers;
    bufferMemoryBarriers.reserve(bufferBarrierCount);

    std::vector<VkImageMemoryBarrier> imageBarriers;
    imageBarriers.reserve(textureBarrierCount);

    // Combine pipeline stages from all barriers
    VkPipelineStageFlags srcStage = 0;
    VkPipelineStageFlags dstStage = 0;

    // Process memory barriers
    for (uint32_t i = 0; i < memoryBarrierCount; ++i) {
        const auto& barrier = memoryBarriers[i];

        VkMemoryBarrier vkBarrier{};
        vkBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        vkBarrier.srcAccessMask = barrier.srcAccessMask;
        vkBarrier.dstAccessMask = barrier.dstAccessMask;

        memBarriers.push_back(vkBarrier);

        srcStage |= barrier.srcStageMask;
        dstStage |= barrier.dstStageMask;
    }

    // Process buffer barriers
    for (uint32_t i = 0; i < bufferBarrierCount; ++i) {
        const auto& barrier = bufferBarriers[i];

        VkBufferMemoryBarrier vkBarrier{};
        vkBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        vkBarrier.buffer = barrier.buffer->handle();
        vkBarrier.offset = barrier.offset;
        vkBarrier.size = barrier.size == 0 ? VK_WHOLE_SIZE : barrier.size;
        vkBarrier.srcAccessMask = barrier.srcAccessMask;
        vkBarrier.dstAccessMask = barrier.dstAccessMask;
        vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        bufferMemoryBarriers.push_back(vkBarrier);

        srcStage |= barrier.srcStageMask;
        dstStage |= barrier.dstStageMask;
    }

    // Process texture barriers
    for (uint32_t i = 0; i < textureBarrierCount; ++i) {
        const auto& barrier = textureBarriers[i];

        VkImageMemoryBarrier vkBarrier{};
        vkBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        vkBarrier.image = barrier.texture->handle();
        vkBarrier.subresourceRange.aspectMask = converter::getImageAspectMask(barrier.texture->getFormat());
        vkBarrier.subresourceRange.baseMipLevel = barrier.baseMipLevel;
        vkBarrier.subresourceRange.levelCount = barrier.mipLevelCount;
        vkBarrier.subresourceRange.baseArrayLayer = barrier.baseArrayLayer;
        vkBarrier.subresourceRange.layerCount = barrier.arrayLayerCount;

        vkBarrier.oldLayout = barrier.oldLayout;
        vkBarrier.newLayout = barrier.newLayout;
        vkBarrier.srcAccessMask = barrier.srcAccessMask;
        vkBarrier.dstAccessMask = barrier.dstAccessMask;

        vkBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        imageBarriers.push_back(vkBarrier);

        srcStage |= barrier.srcStageMask;
        dstStage |= barrier.dstStageMask;

        // Update tracked layout
        barrier.texture->setLayout(barrier.newLayout);
    }

    vkCmdPipelineBarrier(m_commandBuffer, srcStage, dstStage, 0, static_cast<uint32_t>(memBarriers.size()), memBarriers.empty() ? nullptr : memBarriers.data(), static_cast<uint32_t>(bufferMemoryBarriers.size()), bufferMemoryBarriers.empty() ? nullptr : bufferMemoryBarriers.data(), static_cast<uint32_t>(imageBarriers.size()), imageBarriers.empty() ? nullptr : imageBarriers.data());
}

void CommandEncoder::copyBufferToBuffer(Buffer* source, uint64_t sourceOffset,
    Buffer* destination, uint64_t destinationOffset,
    uint64_t size)
{
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = sourceOffset;
    copyRegion.dstOffset = destinationOffset;
    copyRegion.size = size;

    vkCmdCopyBuffer(m_commandBuffer, source->handle(), destination->handle(), 1, &copyRegion);
}

void CommandEncoder::copyBufferToTexture(Buffer* source, uint64_t sourceOffset, Texture* destination, VkOffset3D origin, VkExtent3D extent, uint32_t mipLevel, VkImageLayout finalLayout)
{
    // Transition image layout to transfer dst optimal
    destination->transitionLayout(this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevel, 1, 0, 1);

    // Copy buffer to image
    VkBufferImageCopy region{};
    region.bufferOffset = sourceOffset;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = converter::getImageAspectMask(destination->getFormat());
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = origin;
    region.imageExtent = extent;

    vkCmdCopyBufferToImage(m_commandBuffer, source->handle(), destination->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition image layout to final layout
    destination->transitionLayout(this, finalLayout, mipLevel, 1, 0, 1);
}

void CommandEncoder::copyTextureToBuffer(Texture* source, VkOffset3D origin, uint32_t mipLevel, Buffer* destination, uint64_t destinationOffset, VkExtent3D extent, VkImageLayout finalLayout)
{
    // Transition image layout to transfer src optimal
    source->transitionLayout(this, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mipLevel, 1, 0, 1);

    // Copy image to buffer
    VkBufferImageCopy region{};
    region.bufferOffset = destinationOffset;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = converter::getImageAspectMask(source->getFormat());
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = origin;
    region.imageExtent = extent;

    vkCmdCopyImageToBuffer(m_commandBuffer, source->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        destination->handle(), 1, &region);

    // Transition image layout to final layout
    source->transitionLayout(this, finalLayout, mipLevel, 1, 0, 1);
}

// TODO grpup src and dst params
void CommandEncoder::copyTextureToTexture(Texture* source, VkOffset3D sourceOrigin, uint32_t sourceMipLevel, Texture* destination, VkOffset3D destinationOrigin, uint32_t destinationMipLevel, VkExtent3D extent, VkImageLayout srcFinalLayout, VkImageLayout dstFinalLayout)
{
    // For 2D textures and arrays, extent.depth represents layer count
    // For 3D textures, it represents actual depth
    uint32_t layerCount = extent.depth;
    uint32_t copyDepth = extent.depth;

    VkExtent3D srcSize = source->getSize();
    bool is3DTexture = (srcSize.depth > 1);

    if (!is3DTexture) {
        copyDepth = 1;
    } else {
        layerCount = 1;
    }

    // Transition images to transfer layouts
    source->transitionLayout(this, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, sourceMipLevel, 1, sourceOrigin.z, layerCount);
    destination->transitionLayout(this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, destinationMipLevel, 1, destinationOrigin.z, layerCount);

    // Copy image to image
    VkImageCopy region{};
    region.srcSubresource.aspectMask = converter::getImageAspectMask(source->getFormat());
    region.srcSubresource.mipLevel = sourceMipLevel;
    region.srcSubresource.baseArrayLayer = is3DTexture ? 0 : sourceOrigin.z;
    region.srcSubresource.layerCount = layerCount;
    region.srcOffset = { sourceOrigin.x, sourceOrigin.y, is3DTexture ? sourceOrigin.z : 0 };
    region.dstSubresource.aspectMask = converter::getImageAspectMask(destination->getFormat());
    region.dstSubresource.mipLevel = destinationMipLevel;
    region.dstSubresource.baseArrayLayer = is3DTexture ? 0 : destinationOrigin.z;
    region.dstSubresource.layerCount = layerCount;
    region.dstOffset = { destinationOrigin.x, destinationOrigin.y, is3DTexture ? destinationOrigin.z : 0 };
    region.extent = { extent.width, extent.height, copyDepth };

    vkCmdCopyImage(m_commandBuffer, source->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition images to final layouts
    source->transitionLayout(this, srcFinalLayout, sourceMipLevel, 1, sourceOrigin.z, layerCount);
    destination->transitionLayout(this, dstFinalLayout, destinationMipLevel, 1, destinationOrigin.z, layerCount);
}

// TODO grpup src and dst params
void CommandEncoder::blitTextureToTexture(Texture* source, VkOffset3D sourceOrigin, VkExtent3D sourceExtent, uint32_t sourceMipLevel, Texture* destination, VkOffset3D destinationOrigin, VkExtent3D destinationExtent, uint32_t destinationMipLevel, VkFilter filter, VkImageLayout srcFinalLayout, VkImageLayout dstFinalLayout)
{
    // For 2D textures and arrays, extent.depth represents layer count
    // For 3D textures, it represents actual depth
    uint32_t layerCount = sourceExtent.depth;
    uint32_t srcDepth = sourceExtent.depth;
    uint32_t dstDepth = destinationExtent.depth;

    VkExtent3D srcSize = source->getSize();
    bool is3DTexture = (srcSize.depth > 1);

    if (!is3DTexture) {
        srcDepth = 1;
        dstDepth = 1;
    } else {
        layerCount = 1;
    }

    // Transition images to transfer layouts
    source->transitionLayout(this, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, sourceMipLevel, 1, sourceOrigin.z, layerCount);
    destination->transitionLayout(this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, destinationMipLevel, 1, destinationOrigin.z, layerCount);

    // Blit image to image with scaling and filtering
    VkImageBlit region{};
    region.srcSubresource.aspectMask = converter::getImageAspectMask(source->getFormat());
    region.srcSubresource.mipLevel = sourceMipLevel;
    region.srcSubresource.baseArrayLayer = is3DTexture ? 0 : sourceOrigin.z;
    region.srcSubresource.layerCount = layerCount;
    region.srcOffsets[0] = { sourceOrigin.x, sourceOrigin.y, is3DTexture ? sourceOrigin.z : 0 };
    region.srcOffsets[1] = { static_cast<int32_t>(sourceOrigin.x + sourceExtent.width),
        static_cast<int32_t>(sourceOrigin.y + sourceExtent.height),
        is3DTexture ? static_cast<int32_t>(sourceOrigin.z + srcDepth) : 1 };
    region.dstSubresource.aspectMask = converter::getImageAspectMask(destination->getFormat());
    region.dstSubresource.mipLevel = destinationMipLevel;
    region.dstSubresource.baseArrayLayer = is3DTexture ? 0 : destinationOrigin.z;
    region.dstSubresource.layerCount = layerCount;
    region.dstOffsets[0] = { destinationOrigin.x, destinationOrigin.y, is3DTexture ? destinationOrigin.z : 0 };
    region.dstOffsets[1] = { static_cast<int32_t>(destinationOrigin.x + destinationExtent.width),
        static_cast<int32_t>(destinationOrigin.y + destinationExtent.height),
        is3DTexture ? static_cast<int32_t>(destinationOrigin.z + dstDepth) : 1 };

    vkCmdBlitImage(m_commandBuffer, source->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, filter);

    // Transition images to final layouts
    source->transitionLayout(this, srcFinalLayout, sourceMipLevel, 1, sourceOrigin.z, layerCount);
    destination->transitionLayout(this, dstFinalLayout, destinationMipLevel, 1, destinationOrigin.z, layerCount);
}

} // namespace gfx::backend::vulkan::core