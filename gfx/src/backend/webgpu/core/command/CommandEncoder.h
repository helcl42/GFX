#ifndef GFX_WEBGPU_COMMAND_ENCODER_H
#define GFX_WEBGPU_COMMAND_ENCODER_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class Device;
class Buffer;
class Texture;

class CommandEncoder {
public:
    // Prevent copying
    CommandEncoder(const CommandEncoder&) = delete;
    CommandEncoder& operator=(const CommandEncoder&) = delete;

    CommandEncoder(Device* device, const CommandEncoderCreateInfo& createInfo);
    ~CommandEncoder();

    WGPUCommandEncoder handle() const;
    Device* getDevice() const;

    void markFinished();
    bool isFinished() const;
    // Recreate the encoder if it has been finished
    bool recreateIfNeeded();

    // Copy operations
    void copyBufferToBuffer(Buffer* source, uint64_t sourceOffset, Buffer* destination, uint64_t destinationOffset, uint64_t size);
    void copyBufferToTexture(Buffer* source, uint64_t sourceOffset, uint32_t bytesPerRow, Texture* destination, const WGPUOrigin3D& origin, const WGPUExtent3D& extent, uint32_t mipLevel);
    void copyTextureToBuffer(Texture* source, const WGPUOrigin3D& origin, uint32_t mipLevel, Buffer* destination, uint64_t destinationOffset, uint32_t bytesPerRow, const WGPUExtent3D& extent);
    void copyTextureToTexture(Texture* source, const WGPUOrigin3D& sourceOrigin, uint32_t sourceMipLevel, Texture* destination, const WGPUOrigin3D& destinationOrigin, uint32_t destinationMipLevel, const WGPUExtent3D& extent);
    void blitTextureToTexture(Texture* source, const WGPUOrigin3D& sourceOrigin, const WGPUExtent3D& sourceExtent, uint32_t sourceMipLevel, Texture* destination, const WGPUOrigin3D& destinationOrigin, const WGPUExtent3D& destinationExtent, uint32_t destinationMipLevel, WGPUFilterMode filter);

private:
    Device* m_device = nullptr; // Non-owning pointer
    WGPUCommandEncoder m_encoder = nullptr;
    bool m_finished = false;
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_COMMAND_ENCODER_H