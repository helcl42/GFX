#include "Entities.h"

namespace gfx::vulkan {

// ============================================================================
// Queue Implementation
// ============================================================================

Queue::Queue(Device* device, uint32_t queueFamily)
    : m_device(device)
    , m_queueFamily(queueFamily)
{
    vkGetDeviceQueue(device->handle(), queueFamily, 0, &m_queue);
}

VkDevice Queue::device() const
{
    return m_device->handle();
}

VkPhysicalDevice Queue::physicalDevice() const
{
    return m_device->getAdapter()->handle();
}

VkResult Queue::submit(const SubmitInfo& submitInfo)
{
    // Convert command encoders to command buffers
    std::vector<VkCommandBuffer> commandBuffers;
    commandBuffers.reserve(submitInfo.commandEncoderCount);
    for (uint32_t i = 0; i < submitInfo.commandEncoderCount; ++i) {
        commandBuffers.push_back(submitInfo.commandEncoders[i]->handle());
    }

    // Convert wait semaphores
    std::vector<VkSemaphore> waitSemaphores;
    std::vector<uint64_t> waitValues;
    std::vector<VkPipelineStageFlags> waitStages;
    waitSemaphores.reserve(submitInfo.waitSemaphoreCount);
    waitStages.reserve(submitInfo.waitSemaphoreCount);

    bool hasTimelineWait = false;
    for (uint32_t i = 0; i < submitInfo.waitSemaphoreCount; ++i) {
        waitSemaphores.push_back(submitInfo.waitSemaphores[i]->handle());
        waitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        if (submitInfo.waitSemaphores[i]->getType() == SemaphoreType::Timeline) {
            hasTimelineWait = true;
            uint64_t value = submitInfo.waitValues ? submitInfo.waitValues[i] : 0;
            waitValues.push_back(value);
        } else {
            waitValues.push_back(0);
        }
    }

    // Convert signal semaphores
    std::vector<VkSemaphore> signalSemaphores;
    std::vector<uint64_t> signalValues;
    signalSemaphores.reserve(submitInfo.signalSemaphoreCount);

    bool hasTimelineSignal = false;
    for (uint32_t i = 0; i < submitInfo.signalSemaphoreCount; ++i) {
        signalSemaphores.push_back(submitInfo.signalSemaphores[i]->handle());

        if (submitInfo.signalSemaphores[i]->getType() == SemaphoreType::Timeline) {
            hasTimelineSignal = true;
            uint64_t value = submitInfo.signalValues ? submitInfo.signalValues[i] : 0;
            signalValues.push_back(value);
        } else {
            signalValues.push_back(0);
        }
    }

    // Timeline semaphore info (if needed)
    VkTimelineSemaphoreSubmitInfo timelineInfo{};
    if (hasTimelineWait || hasTimelineSignal) {
        timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        timelineInfo.waitSemaphoreValueCount = static_cast<uint32_t>(waitValues.size());
        timelineInfo.pWaitSemaphoreValues = waitValues.empty() ? nullptr : waitValues.data();
        timelineInfo.signalSemaphoreValueCount = static_cast<uint32_t>(signalValues.size());
        timelineInfo.pSignalSemaphoreValues = signalValues.empty() ? nullptr : signalValues.data();
    }

    // Build submit info
    VkSubmitInfo vkSubmitInfo{};
    vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    if (hasTimelineWait || hasTimelineSignal) {
        vkSubmitInfo.pNext = &timelineInfo;
    }
    vkSubmitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
    vkSubmitInfo.pCommandBuffers = commandBuffers.empty() ? nullptr : commandBuffers.data();
    vkSubmitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    vkSubmitInfo.pWaitSemaphores = waitSemaphores.empty() ? nullptr : waitSemaphores.data();
    vkSubmitInfo.pWaitDstStageMask = waitStages.empty() ? nullptr : waitStages.data();
    vkSubmitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
    vkSubmitInfo.pSignalSemaphores = signalSemaphores.empty() ? nullptr : signalSemaphores.data();

    // Get fence if provided
    VkFence fence = VK_NULL_HANDLE;
    if (submitInfo.signalFence) {
        fence = submitInfo.signalFence->handle();
    }

    return vkQueueSubmit(m_queue, 1, &vkSubmitInfo, fence);
}

void Queue::writeBuffer(Buffer* buffer, uint64_t offset, const void* data, uint64_t size)
{
    void* mapped = buffer->map();
    if (mapped) {
        // Buffer is host-visible, can map directly
        memcpy(static_cast<char*>(mapped) + offset, data, size);
        buffer->unmap();
    } else {
        // Buffer is not host-visible (device-local), use staging buffer
        VkDevice vkDevice = device();

        // Create staging buffer
        VkBufferCreateInfo stagingInfo{};
        stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingInfo.size = size;
        stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkBuffer stagingBuffer;
        if (vkCreateBuffer(vkDevice, &stagingInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create staging buffer for writeBuffer");
        }

        // Allocate staging buffer memory (host-visible)
        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(vkDevice, stagingBuffer, &memReq);

        const VkPhysicalDeviceMemoryProperties& memProps = m_device->getAdapter()->getMemoryProperties();

        uint32_t memTypeIndex = UINT32_MAX;
        VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
            if ((memReq.memoryTypeBits & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & flags) == flags) {
                memTypeIndex = i;
                break;
            }
        }

        if (memTypeIndex == UINT32_MAX) {
            vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
            throw std::runtime_error("Failed to find suitable memory type for staging buffer");
        }

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = memTypeIndex;

        VkDeviceMemory stagingMemory;
        if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
            vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
            throw std::runtime_error("Failed to allocate staging buffer memory");
        }

        vkBindBufferMemory(vkDevice, stagingBuffer, stagingMemory, 0);

        // Map and copy data to staging buffer
        void* stagingMapped;
        vkMapMemory(vkDevice, stagingMemory, 0, size, 0, &stagingMapped);
        memcpy(stagingMapped, data, size);
        vkUnmapMemory(vkDevice, stagingMemory);

        // Create and submit copy command
        VkCommandBufferAllocateInfo cmdAllocInfo{};
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandBufferCount = 1;

        VkCommandPool cmdPool;
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = family();
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        if (vkCreateCommandPool(vkDevice, &poolInfo, nullptr, &cmdPool) != VK_SUCCESS) {
            vkFreeMemory(vkDevice, stagingMemory, nullptr);
            vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
            throw std::runtime_error("Failed to create command pool for buffer write");
        }

        cmdAllocInfo.commandPool = cmdPool;

        VkCommandBuffer cmdBuffer;
        vkAllocateCommandBuffers(vkDevice, &cmdAllocInfo, &cmdBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(cmdBuffer, &beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = offset;
        copyRegion.size = size;
        vkCmdCopyBuffer(cmdBuffer, stagingBuffer, buffer->handle(), 1, &copyRegion);

        vkEndCommandBuffer(cmdBuffer);

        // Create fence for synchronization
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VkFence fence = VK_NULL_HANDLE;
        vkCreateFence(vkDevice, &fenceInfo, nullptr, &fence);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffer;

        vkQueueSubmit(m_queue, 1, &submitInfo, fence);
        vkWaitForFences(vkDevice, 1, &fence, VK_TRUE, UINT64_MAX);
        vkDestroyFence(vkDevice, fence, nullptr);

        // Cleanup
        vkDestroyCommandPool(vkDevice, cmdPool, nullptr);
        vkFreeMemory(vkDevice, stagingMemory, nullptr);
        vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
    }
}

void Queue::writeTexture(Texture* texture, const VkOffset3D& origin, uint32_t mipLevel,
    const void* data, uint64_t dataSize,
    const VkExtent3D& extent, VkImageLayout finalLayout)
{
    VkDevice device = texture->device();

    // Create staging buffer
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = dataSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create staging buffer for texture upload\n");
        return;
    }

    // Get memory requirements and allocate
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);

    const VkPhysicalDeviceMemoryProperties& memProperties = m_device->getAdapter()->getMemoryProperties();

    uint32_t memoryTypeIndex = UINT32_MAX;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            memoryTypeIndex = i;
            break;
        }
    }

    if (memoryTypeIndex == UINT32_MAX) {
        fprintf(stderr, "Failed to find suitable memory type for staging buffer\n");
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        return;
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    result = vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate staging buffer memory\n");
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        return;
    }

    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

    // Copy data to staging buffer
    void* mappedData = nullptr;
    vkMapMemory(device, stagingMemory, 0, dataSize, 0, &mappedData);
    memcpy(mappedData, data, dataSize);
    vkUnmapMemory(device, stagingMemory);

    // Create temporary command buffer
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = m_queueFamily;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);

    VkCommandBufferAllocateInfo allocCmdInfo{};
    allocCmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocCmdInfo.commandPool = commandPool;
    allocCmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocCmdInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device, &allocCmdInfo, &commandBuffer);

    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // Transition image to transfer dst optimal
    texture->transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevel, 1, 0, 1);

    // Copy buffer to image
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0; // Tightly packed
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = converter::getImageAspectMask(texture->getFormat());
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = origin;
    region.imageExtent = extent;

    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, texture->handle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition image to final layout
    texture->transitionLayout(commandBuffer, finalLayout, mipLevel, 1, 0, 1);

    // End and submit
    vkEndCommandBuffer(commandBuffer);

    // Create fence for synchronization
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence = VK_NULL_HANDLE;
    vkCreateFence(device, &fenceInfo, nullptr, &fence);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_queue, 1, &submitInfo, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(device, fence, nullptr);

    // Cleanup
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
}

void Queue::waitIdle()
{
    vkQueueWaitIdle(m_queue);
}

// ============================================================================
// RenderPassEncoder Implementation
// ============================================================================

// Simpler constructor for use with GfxRenderPass
RenderPassEncoder::RenderPassEncoder(CommandEncoder* commandEncoder, RenderPass* renderPass, Framebuffer* framebuffer, const RenderPassEncoderBeginInfo& beginInfo)
    : m_commandBuffer(commandEncoder->handle())
    , m_device(commandEncoder->getDevice())
    , m_commandEncoder(commandEncoder)
{
    // Build clear values array
    std::vector<VkClearValue> clearValues;

    // Add color clear values with dummy values for resolve attachments
    const auto& colorHasResolve = renderPass->colorHasResolve();
    for (size_t i = 0; i < beginInfo.colorClearValues.size(); ++i) {
        // Add the color attachment clear value
        VkClearValue clearValue{};
        clearValue.color = beginInfo.colorClearValues[i];
        clearValues.push_back(clearValue);
        
        // If this color attachment has a resolve target, add a dummy clear value for it
        // (resolve attachments use LOAD_OP_DONT_CARE so the value doesn't matter)
        if (i < colorHasResolve.size() && colorHasResolve[i]) {
            VkClearValue dummyClear{};
            dummyClear.color = {{0.0f, 0.0f, 0.0f, 0.0f}};
            clearValues.push_back(dummyClear);
        }
    }

    // Add depth/stencil clear value if needed
    if (renderPass->hasDepthStencil()) {
        VkClearValue depthStencilClear{};
        depthStencilClear.depthStencil.depth = beginInfo.depthClearValue;
        depthStencilClear.depthStencil.stencil = beginInfo.stencilClearValue;
        clearValues.push_back(depthStencilClear);
    }

    // Begin render pass
    VkRenderPassBeginInfo vkBeginInfo{};
    vkBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    vkBeginInfo.renderPass = renderPass->handle();
    vkBeginInfo.framebuffer = framebuffer->handle();
    vkBeginInfo.renderArea.offset = { 0, 0 };
    vkBeginInfo.renderArea.extent.width = framebuffer->width();
    vkBeginInfo.renderArea.extent.height = framebuffer->height();
    vkBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    vkBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(m_commandBuffer, &vkBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

// ============================================================================
// ComputePassEncoder Implementation
// ============================================================================

ComputePassEncoder::ComputePassEncoder(CommandEncoder* commandEncoder, const ComputePassEncoderCreateInfo& createInfo)
    : m_commandBuffer(commandEncoder->handle())
    , m_device(commandEncoder->getDevice())
    , m_commandEncoder(commandEncoder)
{
    (void)createInfo; // Label unused for now
}

// ============================================================================
// Texture Implementation
// ============================================================================

void Texture::transitionLayout(CommandEncoder* encoder, VkImageLayout newLayout,
    uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount)
{
    transitionLayout(encoder->handle(), newLayout, baseMipLevel, levelCount, baseArrayLayer, layerCount);
}

void Texture::transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newLayout,
    uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount)
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

    barrier.srcAccessMask = getVkAccessFlagsForLayout(m_currentLayout);
    barrier.dstAccessMask = getVkAccessFlagsForLayout(newLayout);

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

// ============================================================================
// RenderPass Implementation
// ============================================================================

RenderPass::RenderPass(Device* device, const RenderPassCreateInfo& createInfo)
    : m_device(device)
{
    // Store metadata
    m_colorAttachmentCount = static_cast<uint32_t>(createInfo.colorAttachments.size());
    m_hasDepthStencil = createInfo.depthStencilAttachment.has_value();

    // Build attachment descriptions and references
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorRefs;
    std::vector<VkAttachmentReference> resolveRefs;

    uint32_t attachmentIndex = 0;

    // Process color attachments
    for (const auto& colorAttachment : createInfo.colorAttachments) {
        const RenderPassColorAttachmentTarget& target = colorAttachment.target;
        bool isMSAA = (target.sampleCount > VK_SAMPLE_COUNT_1_BIT);

        // Add color attachment
        VkAttachmentDescription colorAttachmentDesc{};
        colorAttachmentDesc.format = target.format;
        colorAttachmentDesc.samples = target.sampleCount;
        colorAttachmentDesc.loadOp = target.loadOp;
        colorAttachmentDesc.storeOp = target.storeOp;
        colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentDesc.initialLayout = (target.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
            ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            : VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentDesc.finalLayout = target.finalLayout;
        attachments.push_back(colorAttachmentDesc);

        VkAttachmentReference colorRef{};
        colorRef.attachment = attachmentIndex++;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorRefs.push_back(colorRef);

        // Check if this attachment has a resolve target
        if (colorAttachment.resolveTarget.has_value()) {
            const RenderPassColorAttachmentTarget& resolveTarget = colorAttachment.resolveTarget.value();

            VkAttachmentDescription resolveAttachmentDesc{};
            resolveAttachmentDesc.format = resolveTarget.format;
            resolveAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
            resolveAttachmentDesc.loadOp = resolveTarget.loadOp;
            resolveAttachmentDesc.storeOp = resolveTarget.storeOp;
            resolveAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            resolveAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            resolveAttachmentDesc.initialLayout = (resolveTarget.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
                ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                : VK_IMAGE_LAYOUT_UNDEFINED;
            resolveAttachmentDesc.finalLayout = resolveTarget.finalLayout;
            attachments.push_back(resolveAttachmentDesc);

            VkAttachmentReference resolveRef{};
            resolveRef.attachment = attachmentIndex++;
            resolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            resolveRefs.push_back(resolveRef);
            
            m_colorHasResolve.push_back(true);
        } else if (isMSAA) {
            // MSAA without resolve needs unused reference
            VkAttachmentReference unusedRef{};
            unusedRef.attachment = VK_ATTACHMENT_UNUSED;
            unusedRef.layout = VK_IMAGE_LAYOUT_UNDEFINED;
            resolveRefs.push_back(unusedRef);
            
            m_colorHasResolve.push_back(false);
        } else {
            m_colorHasResolve.push_back(false);
        }
    }

    // Process depth/stencil attachment
    VkAttachmentReference depthRef{};
    bool hasDepth = false;

    if (createInfo.depthStencilAttachment.has_value()) {
        const RenderPassDepthStencilAttachmentTarget& target = createInfo.depthStencilAttachment->target;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = target.format;
        depthAttachment.samples = target.sampleCount;
        depthAttachment.loadOp = target.depthLoadOp;
        depthAttachment.storeOp = target.depthStoreOp;
        depthAttachment.stencilLoadOp = target.stencilLoadOp;
        depthAttachment.stencilStoreOp = target.stencilStoreOp;

        bool loadDepth = (target.depthLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD);
        bool loadStencil = (target.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD);
        depthAttachment.initialLayout = (loadDepth || loadStencil)
            ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            : VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = target.finalLayout;
        attachments.push_back(depthAttachment);

        depthRef.attachment = attachmentIndex++;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        hasDepth = true;
    }

    // Create subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
    subpass.pColorAttachments = colorRefs.empty() ? nullptr : colorRefs.data();
    subpass.pResolveAttachments = resolveRefs.empty() ? nullptr : resolveRefs.data();
    subpass.pDepthStencilAttachment = hasDepth ? &depthRef : nullptr;

    // Create subpass dependency
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = 0;
    dependency.dstStageMask = 0;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = 0;

    // Add color attachment stages if present
    bool hasColor = !colorRefs.empty();
    if (hasColor) {
        dependency.srcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }

    // Add depth/stencil stages if present
    if (hasDepth) {
        dependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Create render pass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(m_device->handle(), &renderPassInfo, nullptr, &m_renderPass);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass");
    }
}

RenderPass::~RenderPass()
{
    if (m_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_device->handle(), m_renderPass, nullptr);
    }
}

} // namespace gfx::vulkan
