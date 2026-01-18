#include "../resource/Texture.h"

#include "../command/CommandEncoder.h"
#include "../system/Adapter.h"
#include "../system/Device.h"
#include "../system/Instance.h"
#include "../util/Blit.h"

#include <stdexcept>

namespace gfx::backend::webgpu::core {

Texture::Texture(Device* device, const TextureCreateInfo& createInfo)
    : m_device(device)
    , m_ownsResources(true)
    , m_info(createTextureInfo(createInfo))
{
    WGPUTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_INIT;
    desc.dimension = createInfo.dimension;
    desc.size = createInfo.size; // Already has correct depthOrArrayLayers
    desc.format = createInfo.format;
    desc.mipLevelCount = createInfo.mipLevelCount;
    desc.sampleCount = createInfo.sampleCount;
    desc.usage = createInfo.usage;
    desc.viewFormatCount = 0;
    desc.viewFormats = nullptr;

    m_texture = wgpuDeviceCreateTexture(m_device->handle(), &desc);
    if (!m_texture) {
        throw std::runtime_error("Failed to create WebGPU texture");
    }
}

// Non-owning constructor - wraps an existing WGPUTexture
Texture::Texture(Device* device, WGPUTexture texture, const TextureCreateInfo& createInfo)
    : m_device(device)
    , m_ownsResources(false)
    , m_texture(texture)
    , m_info(createTextureInfo(createInfo))
{
}

// Non-owning constructor for imported textures
Texture::Texture(Device* device, WGPUTexture texture, const TextureImportInfo& importInfo)
    : m_device(device)
    , m_ownsResources(false)
    , m_texture(texture)
    , m_info(createTextureInfo(importInfo))
{
}

Texture::~Texture()
{
    if (m_ownsResources && m_texture) {
        wgpuTextureRelease(m_texture);
    }
}

WGPUTexture Texture::handle() const
{
    return m_texture;
}

WGPUTextureDimension Texture::getDimension() const
{
    return m_info.dimension;
}

WGPUExtent3D Texture::getSize() const
{
    return m_info.size;
}

uint32_t Texture::getArrayLayers() const
{
    return m_info.arrayLayers;
}

WGPUTextureFormat Texture::getFormat() const
{
    return m_info.format;
}

uint32_t Texture::getMipLevels() const
{
    return m_info.mipLevels;
}

uint32_t Texture::getSampleCount() const
{
    return m_info.sampleCount;
}

WGPUTextureUsage Texture::getUsage() const
{
    return m_info.usage;
}

const TextureInfo& Texture::getInfo() const
{
    return m_info;
}

void Texture::generateMipmaps(CommandEncoder* encoder)
{
    if (m_info.mipLevels <= 1) {
        return; // No mipmaps to generate
    }

    generateMipmapsRange(encoder, 0, m_info.mipLevels);
}

void Texture::generateMipmapsRange(CommandEncoder* encoder, uint32_t baseMipLevel, uint32_t levelCount)
{
    if (levelCount <= 1) {
        return; // Nothing to generate
    }

    // Get the Blit helper from the device
    Blit* blit = encoder->getDevice()->getBlit();

    // Generate each mip level by blitting from the previous level
    for (uint32_t i = 0; i < levelCount - 1; ++i) {
        uint32_t srcMip = baseMipLevel + i;
        uint32_t dstMip = srcMip + 1;

        // Calculate extents for each mip level
        WGPUExtent3D srcSize = m_info.size;
        uint32_t srcWidth = std::max(1u, srcSize.width >> srcMip);
        uint32_t srcHeight = std::max(1u, srcSize.height >> srcMip);
        uint32_t dstWidth = std::max(1u, srcSize.width >> dstMip);
        uint32_t dstHeight = std::max(1u, srcSize.height >> dstMip);

        WGPUOrigin3D origin = { 0, 0, 0 };
        WGPUExtent3D srcExtent = { srcWidth, srcHeight, 1 };
        WGPUExtent3D dstExtent = { dstWidth, dstHeight, 1 };

        // Use linear filtering for mipmap generation
        blit->execute(encoder->handle(),
            m_texture, origin, srcExtent, srcMip,
            m_texture, origin, dstExtent, dstMip,
            WGPUFilterMode_Linear);
    }
}

TextureInfo Texture::createTextureInfo(const TextureCreateInfo& createInfo)
{
    TextureInfo info{};
    info.dimension = createInfo.dimension;
    info.size = createInfo.size;
    info.arrayLayers = createInfo.size.depthOrArrayLayers;
    info.format = createInfo.format;
    info.mipLevels = createInfo.mipLevelCount;
    info.sampleCount = createInfo.sampleCount;
    info.usage = createInfo.usage;
    return info;
}

TextureInfo Texture::createTextureInfo(const TextureImportInfo& importInfo)
{
    TextureInfo info{};
    info.dimension = importInfo.dimension;
    info.size = importInfo.size;
    info.arrayLayers = importInfo.size.depthOrArrayLayers;
    info.format = importInfo.format;
    info.mipLevels = importInfo.mipLevelCount;
    info.sampleCount = importInfo.sampleCount;
    info.usage = importInfo.usage;
    return info;
}

} // namespace gfx::backend::webgpu::core