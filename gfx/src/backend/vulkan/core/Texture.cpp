#include "Texture.h"

#include "Adapter.h"
#include "CommandEncoder.h"
#include "Device.h"

#include "../converter/Conversions.h"

#include <stdexcept>

namespace gfx::backend::vulkan::core {

// Owning constructor - creates and manages VkImage and memory
Texture::Texture(Device* device, const TextureCreateInfo& createInfo)
    : m_device(device)
    , m_ownsResources(true)
    , m_info(createTextureInfo(createInfo))
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = m_info.imageType;
    imageInfo.extent = m_info.size;
    imageInfo.mipLevels = m_info.mipLevelCount;
    imageInfo.arrayLayers = m_info.arrayLayers;
    imageInfo.flags = createInfo.flags;
    imageInfo.format = m_info.format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Always create in UNDEFINED, transition explicitly
    imageInfo.usage = m_info.usage;

    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = m_info.sampleCount;

    VkResult result = vkCreateImage(m_device->handle(), &imageInfo, nullptr, &m_image);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device->handle(), m_image, &memRequirements);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_device->getAdapter()->handle(), &memProperties);

    uint32_t memoryTypeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            memoryTypeIndex = i;
            break;
        }
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    result = vkAllocateMemory(m_device->handle(), &allocInfo, nullptr, &m_memory);
    if (result != VK_SUCCESS) {
        vkDestroyImage(m_device->handle(), m_image, nullptr);
        throw std::runtime_error("Failed to allocate image memory");
    }

    vkBindImageMemory(m_device->handle(), m_image, m_memory, 0);
}

// Non-owning constructor - wraps an existing VkImage (e.g., from swapchain)
Texture::Texture(Device* device, VkImage image, const TextureCreateInfo& createInfo)
    : m_device(device)
    , m_ownsResources(false)
    , m_info(createTextureInfo(createInfo))
    , m_image(image)
{
}

// Non-owning constructor for imported textures
Texture::Texture(Device* device, VkImage image, const TextureImportInfo& importInfo)
    : m_device(device)
    , m_ownsResources(false)
    , m_info(createTextureInfo(importInfo))
    , m_image(image)
{
}

Texture::~Texture()
{
    if (m_ownsResources) {
        if (m_memory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device->handle(), m_memory, nullptr);
        }
        if (m_image != VK_NULL_HANDLE) {
            vkDestroyImage(m_device->handle(), m_image, nullptr);
        }
    }
}

VkImage Texture::handle() const
{
    return m_image;
}

VkDevice Texture::device() const
{
    return m_device->handle();
}

VkImageType Texture::getImageType() const
{
    return m_info.imageType;
}

VkExtent3D Texture::getSize() const
{
    return m_info.size;
}

uint32_t Texture::getArrayLayers() const
{
    return m_info.arrayLayers;
}

VkFormat Texture::getFormat() const
{
    return m_info.format;
}

uint32_t Texture::getMipLevelCount() const
{
    return m_info.mipLevelCount;
}

VkSampleCountFlagBits Texture::getSampleCount() const
{
    return m_info.sampleCount;
}

VkImageUsageFlags Texture::getUsage() const
{
    return m_info.usage;
}

const TextureInfo& Texture::getInfo() const
{
    return m_info;
}

VkImageLayout Texture::getLayout() const
{
    return m_currentLayout;
}

void Texture::setLayout(VkImageLayout layout)
{
    m_currentLayout = layout;
}

void Texture::transitionLayout(CommandEncoder* encoder, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount)
{
    transitionLayout(encoder->handle(), newLayout, baseMipLevel, levelCount, baseArrayLayer, layerCount);
}

void Texture::transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = m_currentLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;

    // Determine aspect mask based on format
    barrier.subresourceRange.aspectMask = converter::getImageAspectMask(m_info.format);

    barrier.subresourceRange.baseMipLevel = baseMipLevel;
    barrier.subresourceRange.levelCount = levelCount;
    barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
    barrier.subresourceRange.layerCount = layerCount;

    // Determine pipeline stages and access masks based on layouts
    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;

    barrier.srcAccessMask = converter::getVkAccessFlagsForLayout(m_currentLayout);
    barrier.dstAccessMask = converter::getVkAccessFlagsForLayout(newLayout);

    // Determine source stage
    switch (m_currentLayout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        srcStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        break;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        srcStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        break;
    default:
        srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        break;
    }

    // Determine destination stage
    switch (newLayout) {
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        break;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        break;
    default:
        dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        break;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        srcStage, dstStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    // Update the current layout
    m_currentLayout = newLayout;
}

void Texture::generateMipmaps(CommandEncoder* encoder)
{
    if (m_info.mipLevelCount <= 1) {
        return; // No mipmaps to generate
    }

    generateMipmapsRange(encoder, 0, m_info.mipLevelCount);
}

void Texture::generateMipmapsRange(CommandEncoder* encoder, uint32_t baseMipLevel, uint32_t levelCount)
{
    // Validate range
    if (baseMipLevel >= m_info.mipLevelCount || levelCount == 0) {
        return;
    }
    if (baseMipLevel + levelCount > m_info.mipLevelCount) {
        levelCount = m_info.mipLevelCount - baseMipLevel;
    }

    VkImageLayout initialLayout = m_currentLayout;

    transitionLayout(encoder, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, baseMipLevel, 1, 0, m_info.arrayLayers);

    VkCommandBuffer cmdBuffer = encoder->handle();

    // Blit each mip level from the previous one
    for (uint32_t i = 0; i < levelCount - 1; ++i) {
        uint32_t srcMip = baseMipLevel + i;
        uint32_t dstMip = srcMip + 1;

        // Calculate dimensions for src and dst
        int32_t srcWidth = static_cast<int32_t>(m_info.size.width >> srcMip);
        int32_t srcHeight = static_cast<int32_t>(m_info.size.height >> srcMip);
        int32_t srcDepth = static_cast<int32_t>(m_info.size.depth >> srcMip);

        int32_t dstWidth = static_cast<int32_t>(m_info.size.width >> dstMip);
        int32_t dstHeight = static_cast<int32_t>(m_info.size.height >> dstMip);
        int32_t dstDepth = static_cast<int32_t>(m_info.size.depth >> dstMip);

        // Ensure minimum dimension of 1
        srcWidth = std::max(1, srcWidth);
        srcHeight = std::max(1, srcHeight);
        srcDepth = std::max(1, srcDepth);
        dstWidth = std::max(1, dstWidth);
        dstHeight = std::max(1, dstHeight);
        dstDepth = std::max(1, dstDepth);

        // Transition src mip to TRANSFER_SRC_OPTIMAL
        transitionLayout(encoder, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcMip, 1, 0, m_info.arrayLayers);

        // Transition dst mip to TRANSFER_DST_OPTIMAL
        transitionLayout(encoder, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstMip, 1, 0, m_info.arrayLayers);

        // Blit from src to dst
        VkImageBlit blit = {};
        blit.srcSubresource.aspectMask = converter::getImageAspectMask(m_info.format);
        blit.srcSubresource.mipLevel = srcMip;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = m_info.arrayLayers;
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { srcWidth, srcHeight, srcDepth };

        blit.dstSubresource.aspectMask = converter::getImageAspectMask(m_info.format);
        blit.dstSubresource.mipLevel = dstMip;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = m_info.arrayLayers;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { dstWidth, dstHeight, dstDepth };

        vkCmdBlitImage(cmdBuffer,
            m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);
    }

    transitionLayout(encoder, initialLayout, baseMipLevel, levelCount, 0, m_info.arrayLayers);
}

// Static helper to create TextureInfo from TextureCreateInfo
TextureInfo Texture::createTextureInfo(const TextureCreateInfo& info)
{
    return TextureInfo{
        info.imageType,
        info.size,
        info.arrayLayers,
        info.format,
        info.mipLevelCount,
        info.sampleCount,
        info.usage
    };
}

// Static helper to create TextureInfo from TextureImportInfo
TextureInfo Texture::createTextureInfo(const TextureImportInfo& info)
{
    return TextureInfo{
        info.imageType,
        info.size,
        info.arrayLayers,
        info.format,
        info.mipLevelCount,
        info.sampleCount,
        info.usage
    };
}

} // namespace gfx::backend::vulkan::core