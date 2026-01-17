#ifndef GFX_VULKAN_BUFFER_H
#define GFX_VULKAN_BUFFER_H

#include "../CoreTypes.h"

namespace gfx::backend::vulkan::core {

class Device;

class Buffer {
public:
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Device* device, const BufferCreateInfo& createInfo);
    Buffer(Device* device, VkBuffer buffer, const BufferImportInfo& importInfo);
    ~Buffer();

    void* map();
    void unmap();

    VkBuffer handle() const;
    size_t size() const;
    VkBufferUsageFlags getUsage() const;
    const BufferInfo& getInfo() const;

private:
    static BufferInfo createBufferInfo(const BufferCreateInfo& createInfo);
    static BufferInfo createBufferInfo(const BufferImportInfo& importInfo);

private:
    Device* m_device = nullptr;
    bool m_ownsResources = true;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    BufferInfo m_info{};
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_BUFFER_H