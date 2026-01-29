#include "Queue.h"

#include "Adapter.h"
#include "Device.h"

#include "../command/CommandEncoder.h"
#include "../resource/Buffer.h"
#include "../resource/Texture.h"
#include "../sync/Fence.h"
#include "../sync/Semaphore.h"
#include "../util/Utils.h"

#include "common/Logger.h"

#include <cstring>
#include <stdexcept>

namespace gfx::backend::vulkan::core {

Queue::Queue(Device* device, VkQueue queue, uint32_t queueFamily)
    : m_queue(queue)
    , m_device(device)
    , m_queueFamily(queueFamily)
{
}

VkQueue Queue::handle() const
{
    return m_queue;
}

VkDevice Queue::device() const
{
    return m_device->handle();
}

VkPhysicalDevice Queue::physicalDevice() const
{
    return m_device->getAdapter()->handle();
}

uint32_t Queue::family() const
{
    return m_queueFamily;
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

void Queue::waitIdle()
{
    vkQueueWaitIdle(m_queue);
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
        gfx::common::Logger::instance().logError("Failed to create staging buffer for texture upload");
        return;
    }

    // Get memory requirements and allocate
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);

    const VkPhysicalDeviceMemoryProperties& memProperties = m_device->getAdapter()->getMemoryProperties();

    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    uint32_t memoryTypeIndex = findMemoryType(memProperties, memRequirements.memoryTypeBits, properties);

    if (memoryTypeIndex == UINT32_MAX) {
        gfx::common::Logger::instance().logError("Failed to find suitable memory type for staging buffer");
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
        gfx::common::Logger::instance().logError("Failed to allocate staging buffer memory");
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
    region.imageSubresource.aspectMask = getImageAspectMask(texture->getFormat());
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

} // namespace gfx::backend::vulkan::core