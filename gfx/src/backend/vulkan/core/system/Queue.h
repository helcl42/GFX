#ifndef GFX_VULKAN_QUEUE_H
#define GFX_VULKAN_QUEUE_H

#include "../CoreTypes.h"

namespace gfx::backend::vulkan::core {

class Device;
class Buffer;
class Texture;

class Queue {
public:
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    Queue(Device* device, uint32_t queueFamily);
    ~Queue() = default;

    VkQueue handle() const;
    VkDevice device() const;
    VkPhysicalDevice physicalDevice() const;
    uint32_t family() const;

    VkResult submit(const SubmitInfo& submitInfo);
    void waitIdle();

    // Write data directly to a buffer by mapping it
    void writeBuffer(Buffer* buffer, uint64_t offset, const void* data, uint64_t size);

    // Write data directly to a texture using staging buffer
    void writeTexture(Texture* texture, const VkOffset3D& origin, uint32_t mipLevel, const void* data, uint64_t dataSize, const VkExtent3D& extent, VkImageLayout finalLayout);

private:
    VkQueue m_queue = VK_NULL_HANDLE;
    Device* m_device = nullptr;
    uint32_t m_queueFamily = 0;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_QUEUE_H