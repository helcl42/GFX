#ifndef GFX_VULKAN_DEVICE_H
#define GFX_VULKAN_DEVICE_H

#include "../CoreTypes.h"

#include <memory>

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
    Adapter* getAdapter();
    const VkPhysicalDeviceProperties& getProperties() const;

private:
    VkDevice m_device = VK_NULL_HANDLE;
    Adapter* m_adapter = nullptr; // Non-owning pointer
    std::unique_ptr<Queue> m_queue;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_DEVICE_H