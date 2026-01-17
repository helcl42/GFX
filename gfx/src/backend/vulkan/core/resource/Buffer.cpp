#include "Buffer.h"

#include "../system/Adapter.h"
#include "../system/Device.h"

#include <stdexcept>

namespace gfx::backend::vulkan::core {

// Owning constructor - creates and manages VkBuffer and memory
Buffer::Buffer(Device* device, const BufferCreateInfo& createInfo)
    : m_device(device)
    , m_ownsResources(true)
    , m_memory(VK_NULL_HANDLE)
    , m_info(createBufferInfo(createInfo))
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = m_info.size;
    bufferInfo.usage = m_info.usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(m_device->handle(), &bufferInfo, nullptr, &m_buffer);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device->handle(), m_buffer, &memRequirements);

    // Find memory type
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_device->getAdapter()->handle(), &memProperties);

    uint32_t memoryTypeIndex = UINT32_MAX;
    VkMemoryPropertyFlags properties = createInfo.mapped
        ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            memoryTypeIndex = i;
            break;
        }
    }

    if (memoryTypeIndex == UINT32_MAX) {
        vkDestroyBuffer(m_device->handle(), m_buffer, nullptr);
        throw std::runtime_error("Failed to find suitable memory type");
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    result = vkAllocateMemory(m_device->handle(), &allocInfo, nullptr, &m_memory);
    if (result != VK_SUCCESS) {
        vkDestroyBuffer(m_device->handle(), m_buffer, nullptr);
        throw std::runtime_error("Failed to allocate buffer memory");
    }

    vkBindBufferMemory(m_device->handle(), m_buffer, m_memory, 0);
}

// Non-owning constructor - wraps an existing VkBuffer
Buffer::Buffer(Device* device, VkBuffer buffer, const BufferImportInfo& importInfo)
    : m_device(device)
    , m_ownsResources(false)
    , m_buffer(buffer)
    , m_memory(VK_NULL_HANDLE)
    , m_info(createBufferInfo(importInfo))
{
}

Buffer::~Buffer()
{
    if (m_ownsResources) {
        if (m_memory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device->handle(), m_memory, nullptr);
        }
        if (m_buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device->handle(), m_buffer, nullptr);
        }
    }
}

void* Buffer::map()
{
    if (!m_info.mapped) {
        return nullptr;
    }

    void* data;
    vkMapMemory(m_device->handle(), m_memory, 0, m_info.size, 0, &data);
    return data;
}

void Buffer::unmap()
{
    if (!m_info.mapped) {
        return;
    }

    vkUnmapMemory(m_device->handle(), m_memory);
}

VkBuffer Buffer::handle() const
{
    return m_buffer;
}

size_t Buffer::size() const
{
    return m_info.size;
}

VkBufferUsageFlags Buffer::getUsage() const
{
    return m_info.usage;
}

const BufferInfo& Buffer::getInfo() const
{
    return m_info;
}

BufferInfo Buffer::createBufferInfo(const BufferCreateInfo& createInfo)
{
    BufferInfo info{};
    info.size = createInfo.size;
    info.usage = createInfo.usage;
    info.mapped = createInfo.mapped;
    return info;
}

BufferInfo Buffer::createBufferInfo(const BufferImportInfo& importInfo)
{
    BufferInfo info{};
    info.size = importInfo.size;
    info.usage = importInfo.usage;
    info.mapped = importInfo.mapped;
    return info;
}

} // namespace gfx::backend::vulkan::core