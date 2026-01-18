#ifndef GFX_WEBGPU_TEXTURE_H
#define GFX_WEBGPU_TEXTURE_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class Device;

class Texture {
public:
    // Prevent copying
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Owning constructor - creates and manages WGPUTexture
    Texture(Device* device, const TextureCreateInfo& createInfo);
    // Non-owning constructor - wraps an existing WGPUTexture
    Texture(Device* device, WGPUTexture texture, const TextureCreateInfo& createInfo);
    // Non-owning constructor for imported textures
    Texture(Device* device, WGPUTexture texture, const TextureImportInfo& importInfo);
    ~Texture();

    WGPUTexture handle() const;
    WGPUTextureDimension getDimension() const;
    WGPUExtent3D getSize() const;
    uint32_t getArrayLayers() const;
    WGPUTextureFormat getFormat() const;
    uint32_t getMipLevels() const;
    uint32_t getSampleCount() const;
    WGPUTextureUsage getUsage() const;
    const TextureInfo& getInfo() const;

    void generateMipmaps(CommandEncoder* encoder);
    void generateMipmapsRange(CommandEncoder* encoder, uint32_t baseMipLevel, uint32_t levelCount);

private:
    static TextureInfo createTextureInfo(const TextureCreateInfo& createInfo);
    static TextureInfo createTextureInfo(const TextureImportInfo& importInfo);

private:
    Device* m_device = nullptr; // Non-owning pointer for device operations
    bool m_ownsResources = true;
    WGPUTexture m_texture = nullptr;
    TextureInfo m_info = {};
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_TEXTURE_H