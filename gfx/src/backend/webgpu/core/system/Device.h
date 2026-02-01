#ifndef GFX_WEGPU_DEVICE_H
#define GFX_WEGPU_DEVICE_H

#include "../CoreTypes.h"

#include <memory>

namespace gfx::backend::webgpu::core {

class Adapter;
class Queue;
class Blit;

class Device {
public:
    // Prevent copying
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    // Constructor 1: Request device from adapter with createInfo
    Device(Adapter* adapter, const DeviceCreateInfo& createInfo);
    ~Device();

    WGPUDevice handle() const;
    Queue* getQueue();
    Adapter* getAdapter();

    WGPULimits getLimits() const;

    void waitIdle() const;

    bool supportsShaderFormat(ShaderSourceType format) const;

    Blit* getBlit();

private:
    WGPUDevice m_device = nullptr;
    Adapter* m_adapter = nullptr; // Non-owning pointer
    std::unique_ptr<Queue> m_queue;
    std::unique_ptr<Blit> m_blit;
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEGPU_DEVICE_H