#ifndef GFX_VULKAN_SEMAPHORE_H
#define GFX_VULKAN_SEMAPHORE_H

#include "../CoreTypes.h"

namespace gfx::backend::vulkan::core {

class Device;

class Semaphore {
public:
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

    Semaphore(Device* device, const SemaphoreCreateInfo& createInfo);
    ~Semaphore();

    VkSemaphore handle() const;
    SemaphoreType getType() const;

    VkResult signal(uint64_t value);
    VkResult wait(uint64_t value, uint64_t timeoutNs);
    uint64_t getValue() const;

private:
    VkSemaphore m_semaphore = VK_NULL_HANDLE;
    Device* m_device = nullptr;
    SemaphoreType m_type = SemaphoreType::Binary;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_SEMAPHORE_H