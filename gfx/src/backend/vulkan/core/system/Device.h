#ifndef GFX_VULKAN_DEVICE_H
#define GFX_VULKAN_DEVICE_H

#include "../CoreTypes.h"

#include <memory>
#include <unordered_map>

namespace gfx::backend::vulkan::core {

class Adapter;
class Queue;

class Device {
public:
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    Device(Adapter* adapter, const DeviceCreateInfo& createInfo);
    ~Device();

    void waitIdle();

    VkDevice handle() const;
    Queue* getQueue();
    Queue* getQueueByIndex(uint32_t queueFamilyIndex, uint32_t queueIndex);
    Adapter* getAdapter();
    const VkPhysicalDeviceProperties& getProperties() const;

private:
    VkDevice m_device = VK_NULL_HANDLE;
    Adapter* m_adapter = nullptr; // Non-owning pointer
    
    // Map of (queueFamilyIndex << 16 | queueIndex) -> Queue
    std::unordered_map<uint64_t, std::unique_ptr<Queue>> m_queues;
    Queue* m_defaultQueue = nullptr; // Non-owning pointer to default queue
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_DEVICE_H