#include "../command/CommandEncoder.h"

#include "../resource/Buffer.h"
#include "../resource/Texture.h"
#include "../system/Device.h"
#include "../util/Blit.h"
#include "../util/Utils.h"

namespace gfx::backend::webgpu::core {

CommandEncoder::CommandEncoder(Device* device, const CommandEncoderCreateInfo& createInfo)
    : m_device(device)
    , m_finished(false)
{
    WGPUCommandEncoderDescriptor desc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
    if (createInfo.label) {
        desc.label = toStringView(createInfo.label);
    }

    m_encoder = wgpuDeviceCreateCommandEncoder(m_device->handle(), &desc);
    if (!m_encoder) {
        throw std::runtime_error("Failed to create WebGPU CommandEncoder");
    }
}

CommandEncoder::~CommandEncoder()
{
    if (m_encoder) {
        wgpuCommandEncoderRelease(m_encoder);
    }
}

WGPUCommandEncoder CommandEncoder::handle() const
{
    return m_encoder;
}

Device* CommandEncoder::getDevice() const
{
    return m_device;
}

void CommandEncoder::markFinished()
{
    m_finished = true;
}

bool CommandEncoder::isFinished() const
{
    return m_finished;
}

// Recreate the encoder if it has been finished
bool CommandEncoder::recreateIfNeeded()
{
    if (!m_finished) {
        return true; // Already valid
    }

    // Release old encoder
    if (m_encoder) {
        wgpuCommandEncoderRelease(m_encoder);
        m_encoder = nullptr;
    }

    // Create new encoder
    WGPUCommandEncoderDescriptor desc = WGPU_COMMAND_ENCODER_DESCRIPTOR_INIT;
    m_encoder = wgpuDeviceCreateCommandEncoder(m_device->handle(), &desc);
    if (!m_encoder) {
        return false;
    }

    m_finished = false;
    return true;
}

// Copy operations
void CommandEncoder::copyBufferToBuffer(Buffer* source, uint64_t sourceOffset, Buffer* destination, uint64_t destinationOffset, uint64_t size)
{
    wgpuCommandEncoderCopyBufferToBuffer(m_encoder,
        source->handle(), sourceOffset,
        destination->handle(), destinationOffset,
        size);
}

void CommandEncoder::copyBufferToTexture(Buffer* source, uint64_t sourceOffset, uint32_t bytesPerRow, Texture* destination, const WGPUOrigin3D& origin, const WGPUExtent3D& extent, uint32_t mipLevel)
{
    WGPUTexelCopyBufferInfo sourceInfo = WGPU_TEXEL_COPY_BUFFER_INFO_INIT;
    sourceInfo.buffer = source->handle();
    sourceInfo.layout.offset = sourceOffset;
    sourceInfo.layout.bytesPerRow = bytesPerRow;

    WGPUTexelCopyTextureInfo destInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    destInfo.texture = destination->handle();
    destInfo.mipLevel = mipLevel;
    destInfo.origin = origin;

    wgpuCommandEncoderCopyBufferToTexture(m_encoder, &sourceInfo, &destInfo, &extent);
}

void CommandEncoder::copyTextureToBuffer(Texture* source, const WGPUOrigin3D& origin, uint32_t mipLevel, Buffer* destination, uint64_t destinationOffset, uint32_t bytesPerRow, const WGPUExtent3D& extent)
{
    WGPUTexelCopyTextureInfo sourceInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    sourceInfo.texture = source->handle();
    sourceInfo.mipLevel = mipLevel;
    sourceInfo.origin = origin;

    WGPUTexelCopyBufferInfo destInfo = WGPU_TEXEL_COPY_BUFFER_INFO_INIT;
    destInfo.buffer = destination->handle();
    destInfo.layout.offset = destinationOffset;
    destInfo.layout.bytesPerRow = bytesPerRow;

    wgpuCommandEncoderCopyTextureToBuffer(m_encoder, &sourceInfo, &destInfo, &extent);
}

void CommandEncoder::copyTextureToTexture(Texture* source, const WGPUOrigin3D& sourceOrigin, uint32_t sourceMipLevel, Texture* destination, const WGPUOrigin3D& destinationOrigin, uint32_t destinationMipLevel, const WGPUExtent3D& extent)
{
    // For 2D textures and arrays, depth represents layer count
    // For 3D textures, it represents actual depth
    WGPUOrigin3D srcOrigin = sourceOrigin;
    WGPUOrigin3D dstOrigin = destinationOrigin;
    if (source->getDimension() != WGPUTextureDimension_3D) {
        srcOrigin.z = 0;
        dstOrigin.z = 0;
    }

    WGPUTexelCopyTextureInfo sourceInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    sourceInfo.texture = source->handle();
    sourceInfo.mipLevel = sourceMipLevel;
    sourceInfo.origin = srcOrigin;

    WGPUTexelCopyTextureInfo destInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    destInfo.texture = destination->handle();
    destInfo.mipLevel = destinationMipLevel;
    destInfo.origin = dstOrigin;

    wgpuCommandEncoderCopyTextureToTexture(m_encoder, &sourceInfo, &destInfo, &extent);
}

void CommandEncoder::blitTextureToTexture(Texture* source, const WGPUOrigin3D& sourceOrigin, const WGPUExtent3D& sourceExtent, uint32_t sourceMipLevel, Texture* destination, const WGPUOrigin3D& destinationOrigin, const WGPUExtent3D& destinationExtent, uint32_t destinationMipLevel, WGPUFilterMode filter)
{
    // Get the Blit helper from the device
    Blit* blit = m_device->getBlit();
    blit->execute(m_encoder,
        source->handle(), sourceOrigin, sourceExtent, sourceMipLevel,
        destination->handle(), destinationOrigin, destinationExtent, destinationMipLevel,
        filter);
}

} // namespace gfx::backend::webgpu::core