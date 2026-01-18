#ifndef GFX_WEBGPU_QUEUE_H
#define GFX_WEBGPU_QUEUE_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class Device;
class Buffer;
class Texture;

class Queue {
public:
    // Prevent copying
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    Queue(WGPUQueue queue, Device* device);
    ~Queue();

    WGPUQueue handle() const;
    Device* getDevice() const;

    // Submit command encoders with optional fence signaling
    bool submit(const SubmitInfo& submitInfo);

    // Write data directly to a buffer
    void writeBuffer(Buffer* buffer, uint64_t offset, const void* data, uint64_t size);
    // Write data directly to a texture
    void writeTexture(Texture* texture, uint32_t mipLevel, const WGPUOrigin3D& origin, const void* data, uint64_t dataSize, uint32_t bytesPerRow, const WGPUExtent3D& extent);

    // Wait for all submitted work to complete
    bool waitIdle();

private:
    WGPUQueue m_queue = nullptr;
    Device* m_device = nullptr; // Non-owning pointer to parent device
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_QUEUE_H