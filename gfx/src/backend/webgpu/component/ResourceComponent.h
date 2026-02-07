#ifndef GFX_BACKEND_WEBGPU_RESOURCE_COMPONENT_H
#define GFX_BACKEND_WEBGPU_RESOURCE_COMPONENT_H

#include <gfx/gfx.h>

namespace gfx::backend::webgpu::component {

class ResourceComponent {
public:
    ResourceComponent() = default;
    ~ResourceComponent() = default;

    // Prevent copying
    ResourceComponent(const ResourceComponent&) = delete;
    ResourceComponent& operator=(const ResourceComponent&) = delete;

    // Buffer functions
    GfxResult deviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer) const;
    GfxResult deviceImportBuffer(GfxDevice device, const GfxBufferImportDescriptor* descriptor, GfxBuffer* outBuffer) const;
    GfxResult bufferDestroy(GfxBuffer buffer) const;
    GfxResult bufferGetInfo(GfxBuffer buffer, GfxBufferInfo* outInfo) const;
    GfxResult bufferGetNativeHandle(GfxBuffer buffer, void** outHandle) const;
    GfxResult bufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer) const;
    GfxResult bufferUnmap(GfxBuffer buffer) const;
    GfxResult bufferFlushMappedRange(GfxBuffer buffer, uint64_t offset, uint64_t size) const;
    GfxResult bufferInvalidateMappedRange(GfxBuffer buffer, uint64_t offset, uint64_t size) const;

    // Texture functions
    GfxResult deviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture) const;
    GfxResult deviceImportTexture(GfxDevice device, const GfxTextureImportDescriptor* descriptor, GfxTexture* outTexture) const;
    GfxResult textureDestroy(GfxTexture texture) const;
    GfxResult textureGetInfo(GfxTexture texture, GfxTextureInfo* outInfo) const;
    GfxResult textureGetNativeHandle(GfxTexture texture, void** outHandle) const;
    GfxResult textureGetLayout(GfxTexture texture, GfxTextureLayout* outLayout) const;
    GfxResult textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView) const;

    // TextureView functions
    GfxResult textureViewDestroy(GfxTextureView textureView) const;

    // Sampler functions
    GfxResult deviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler) const;
    GfxResult samplerDestroy(GfxSampler sampler) const;

    // Shader functions
    GfxResult deviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader) const;
    GfxResult shaderDestroy(GfxShader shader) const;

    // BindGroupLayout functions
    GfxResult deviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout) const;
    GfxResult bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout) const;

    // BindGroup functions
    GfxResult deviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup) const;
    GfxResult bindGroupDestroy(GfxBindGroup bindGroup) const;
};

} // namespace gfx::backend::webgpu::component

#endif // GFX_BACKEND_WEBGPU_RESOURCE_COMPONENT_H
