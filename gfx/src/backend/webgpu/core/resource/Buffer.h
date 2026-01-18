#ifndef GFX_WEBGPU_BUFFER_H
#define GFX_WEBGPU_BUFFER_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class Device;

class Buffer {
public:
    // Prevent copying
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    // Owning constructor - creates and manages WGPUBuffer
    Buffer(Device* device, const BufferCreateInfo& createInfo);
    // Non-owning constructor for imported buffers
    Buffer(Device* device, WGPUBuffer buffer, const BufferImportInfo& importInfo);
    ~Buffer();

    WGPUBuffer handle() const;
    uint64_t getSize() const;
    WGPUBufferUsage getUsage() const;
    const BufferInfo& getInfo() const;
    Device* getDevice() const;

    // Map buffer for CPU access
    // Returns mapped pointer on success, nullptr on failure
    void* map(uint64_t offset, uint64_t size);
    void unmap();

private:
    static BufferInfo createBufferInfo(const BufferCreateInfo& createInfo);
    static BufferInfo createBufferInfo(const BufferImportInfo& importInfo);

private:
    Device* m_device = nullptr; // Non-owning pointer for device operations
    bool m_ownsResources = true;
    WGPUBuffer m_buffer = nullptr;
    BufferInfo m_info{};
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_BUFFER_H