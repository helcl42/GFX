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

} // namespace gfx::vulkan
