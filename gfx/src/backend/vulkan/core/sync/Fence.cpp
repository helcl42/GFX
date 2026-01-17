#include "Fence.h"

#include "../system/Device.h"

#include <stdexcept>

namespace gfx::backend::vulkan::core {

Fence::Fence(Device* device, const FenceCreateInfo& createInfo)
    : m_device(device)
{
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    // If signaled = true, create fence in signaled state
    if (createInfo.signaled) {
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    }

    VkResult result = vkCreateFence(m_device->handle(), &fenceInfo, nullptr, &m_fence);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create fence");
    }
}

Fence::~Fence()
{
    if (m_fence != VK_NULL_HANDLE) {
        vkDestroyFence(m_device->handle(), m_fence, nullptr);
    }
}

VkFence Fence::handle() const { return m_fence; }

VkResult Fence::getStatus(bool* isSignaled) const
{
    if (!isSignaled) {
        return VK_ERROR_UNKNOWN;
    }

    VkResult result = vkGetFenceStatus(m_device->handle(), m_fence);
    if (result == VK_SUCCESS) {
        *isSignaled = true;
        return VK_SUCCESS;
    } else if (result == VK_NOT_READY) {
        *isSignaled = false;
        return VK_SUCCESS;
    } else {
        return result;
    }
}

VkResult Fence::wait(uint64_t timeoutNs)
{
    return vkWaitForFences(m_device->handle(), 1, &m_fence, VK_TRUE, timeoutNs);
}

void Fence::reset()
{
    vkResetFences(m_device->handle(), 1, &m_fence);
}

} // namespace gfx::backend::vulkan::core