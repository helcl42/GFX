#ifndef GFX_VULKAN_FENCE_H
#define GFX_VULKAN_FENCE_H

#include "../CoreTypes.h"

namespace gfx::backend::vulkan::core {

class Device;

class Fence {
public:
    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;

    Fence(Device* device, const FenceCreateInfo& createInfo);
    ~Fence();

    VkFence handle() const;
    VkResult getStatus(bool* isSignaled) const;
    VkResult wait(uint64_t timeoutNs);
    void reset();

private:
    VkFence m_fence = VK_NULL_HANDLE;
    Device* m_device = nullptr;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_FENCE_H