#include "CommandExecutor.h"

#include "../system/Device.h"
#include "../system/Queue.h"

#include <stdexcept>

namespace gfx::backend::vulkan::core {

CommandExecutor::CommandExecutor(Queue* queue)
    : m_queue(queue)
    , m_device(VK_NULL_HANDLE)
    , m_commandPool(VK_NULL_HANDLE)
{
    if (!m_queue) {
        throw std::runtime_error("Queue is null");
    }

    m_device = m_queue->device();

    // Create transient command pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = m_queue->family();

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create transient command pool");
    }
}

CommandExecutor::~CommandExecutor()
{
    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    }
}

void CommandExecutor::execute(const std::function<void(VkCommandBuffer)>& recordFunc)
{
    // Allocate command buffer
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    // Begin recording
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // Record user commands
    recordFunc(commandBuffer);

    // End recording
    vkEndCommandBuffer(commandBuffer);

    // Create fence for synchronization
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence;
    vkCreateFence(m_device, &fenceInfo, nullptr, &fence);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_queue->handle(), 1, &submitInfo, fence);
    vkWaitForFences(m_device, 1, &fence, VK_TRUE, UINT64_MAX);

    // Cleanup
    vkDestroyFence(m_device, fence, nullptr);
    // Command buffer is automatically freed when pool is reset or destroyed
}

} // namespace gfx::backend::vulkan::core
