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

void Queue::writeTexture(Texture* texture, const GfxOrigin3D* origin, uint32_t mipLevel,
    const void* data, uint64_t dataSize,
    const GfxExtent3D* extent, GfxTextureLayout finalLayout)
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

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice(), &memProperties);

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
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = texture->getLayout();
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = texture->handle();
    barrier.subresourceRange.aspectMask = converter::getImageAspectMask(texture->getFormat());
    barrier.subresourceRange.baseMipLevel = mipLevel;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Copy buffer to image
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0; // Tightly packed
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = converter::getImageAspectMask(texture->getFormat());
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { origin ? origin->x : 0,
        origin ? origin->y : 0,
        origin ? origin->z : 0 };
    region.imageExtent = { extent->width, extent->height, extent->depth };

    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, texture->handle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition image to final layout
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = converter::gfxLayoutToVkImageLayout(finalLayout);
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = static_cast<VkAccessFlags>(gfxGetAccessFlagsForLayout(finalLayout));

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Update tracked layout
    texture->setLayout(converter::gfxLayoutToVkImageLayout(finalLayout));

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

} // namespace gfx::vulkan
