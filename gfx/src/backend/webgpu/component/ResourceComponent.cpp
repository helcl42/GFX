#include "ResourceComponent.h"

#include "common/Logger.h"

#include "../common/Common.h"
#include "../converter/Conversions.h"
#include "../validator/Validations.h"

#include "../core/resource/BindGroup.h"
#include "../core/resource/BindGroupLayout.h"
#include "../core/resource/Buffer.h"
#include "../core/resource/Sampler.h"
#include "../core/resource/Shader.h"
#include "../core/resource/Texture.h"
#include "../core/resource/TextureView.h"
#include "../core/system/Device.h"

#include <stdexcept>

namespace gfx::backend::webgpu::component {

// Buffer functions
GfxResult ResourceComponent::deviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer) const
{
    GfxResult validationResult = validator::validateDeviceCreateBuffer(device, descriptor, outBuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUBufferCreateInfo(descriptor);
        auto* buffer = new core::Buffer(devicePtr, createInfo);
        *outBuffer = converter::toGfx<GfxBuffer>(buffer);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create buffer: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult ResourceComponent::deviceImportBuffer(GfxDevice device, const GfxBufferImportDescriptor* descriptor, GfxBuffer* outBuffer) const
{
    GfxResult validationResult = validator::validateDeviceImportBuffer(device, descriptor, outBuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto wgpuBuffer = reinterpret_cast<WGPUBuffer>(descriptor->nativeHandle);
        auto importInfo = converter::gfxExternalDescriptorToWebGPUBufferImportInfo(descriptor);
        auto* buffer = new core::Buffer(devicePtr, wgpuBuffer, importInfo);
        *outBuffer = converter::toGfx<GfxBuffer>(buffer);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to import buffer: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult ResourceComponent::bufferDestroy(GfxBuffer buffer) const
{
    GfxResult validationResult = validator::validateBufferDestroy(buffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Buffer>(buffer);
    return GFX_RESULT_SUCCESS;
}

GfxResult ResourceComponent::bufferGetInfo(GfxBuffer buffer, GfxBufferInfo* outInfo) const
{
    GfxResult validationResult = validator::validateBufferGetInfo(buffer, outInfo);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* buf = converter::toNative<core::Buffer>(buffer);
    *outInfo = converter::wgpuBufferToGfxBufferInfo(buf->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult ResourceComponent::bufferGetNativeHandle(GfxBuffer buffer, void** outHandle) const
{
    GfxResult validationResult = validator::validateBufferGetNativeHandle(buffer, outHandle);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* buf = converter::toNative<core::Buffer>(buffer);
    *outHandle = reinterpret_cast<void*>(buf->handle());
    return GFX_RESULT_SUCCESS;
}

GfxResult ResourceComponent::bufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer) const
{
    GfxResult validationResult = validator::validateBufferMap(buffer, outMappedPointer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* bufferPtr = converter::toNative<core::Buffer>(buffer);
    void* mappedData = bufferPtr->map(offset, size);

    if (!mappedData) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    *outMappedPointer = mappedData;
    return GFX_RESULT_SUCCESS;
}

GfxResult ResourceComponent::bufferUnmap(GfxBuffer buffer) const
{
    GfxResult validationResult = validator::validateBufferUnmap(buffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* bufferPtr = converter::toNative<core::Buffer>(buffer);
    bufferPtr->unmap();
    return GFX_RESULT_SUCCESS;
}

GfxResult ResourceComponent::bufferFlushMappedRange(GfxBuffer buffer, uint64_t offset, uint64_t size) const
{
    GfxResult validationResult = validator::validateBufferFlushMappedRange(buffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* buf = converter::toNative<core::Buffer>(buffer);
    buf->flushMappedRange(offset, size);
    return GFX_RESULT_SUCCESS;
}

GfxResult ResourceComponent::bufferInvalidateMappedRange(GfxBuffer buffer, uint64_t offset, uint64_t size) const
{
    GfxResult validationResult = validator::validateBufferInvalidateMappedRange(buffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* buf = converter::toNative<core::Buffer>(buffer);
    buf->invalidateMappedRange(offset, size);
    return GFX_RESULT_SUCCESS;
}

// Texture functions
GfxResult ResourceComponent::deviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture) const
{
    GfxResult validationResult = validator::validateDeviceCreateTexture(device, descriptor, outTexture);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUTextureCreateInfo(descriptor);
        auto* texture = new core::Texture(devicePtr, createInfo);
        *outTexture = converter::toGfx<GfxTexture>(texture);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create texture: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult ResourceComponent::deviceImportTexture(GfxDevice device, const GfxTextureImportDescriptor* descriptor, GfxTexture* outTexture) const
{
    GfxResult validationResult = validator::validateDeviceImportTexture(device, descriptor, outTexture);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto wgpuTexture = reinterpret_cast<WGPUTexture>(descriptor->nativeHandle);
        auto importInfo = converter::gfxExternalDescriptorToWebGPUTextureImportInfo(descriptor);
        auto* texture = new core::Texture(devicePtr, wgpuTexture, importInfo);
        *outTexture = converter::toGfx<GfxTexture>(texture);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to import texture: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult ResourceComponent::textureDestroy(GfxTexture texture) const
{
    GfxResult validationResult = validator::validateTextureDestroy(texture);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Texture>(texture);
    return GFX_RESULT_SUCCESS;
}

GfxResult ResourceComponent::textureGetInfo(GfxTexture texture, GfxTextureInfo* outInfo) const
{
    GfxResult validationResult = validator::validateTextureGetInfo(texture, outInfo);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* texturePtr = converter::toNative<core::Texture>(texture);
    *outInfo = converter::wgpuTextureInfoToGfxTextureInfo(texturePtr->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult ResourceComponent::textureGetNativeHandle(GfxTexture texture, void** outHandle) const
{
    GfxResult validationResult = validator::validateTextureGetNativeHandle(texture, outHandle);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* texturePtr = converter::toNative<core::Texture>(texture);
    *outHandle = reinterpret_cast<void*>(texturePtr->handle());
    return GFX_RESULT_SUCCESS;
}

GfxResult ResourceComponent::textureGetLayout(GfxTexture texture, GfxTextureLayout* outLayout) const
{
    // WebGPU doesn't have explicit layouts, return GENERAL as a reasonable default
    GfxResult validationResult = validator::validateTextureGetLayout(texture, outLayout);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    *outLayout = GFX_TEXTURE_LAYOUT_GENERAL;
    return GFX_RESULT_SUCCESS;
}

GfxResult ResourceComponent::textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView) const
{
    GfxResult validationResult = validator::validateTextureCreateView(texture, descriptor, outView);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* texturePtr = converter::toNative<core::Texture>(texture);
        auto createInfo = converter::gfxDescriptorToWebGPUTextureViewCreateInfo(descriptor);
        auto* view = new core::TextureView(texturePtr, createInfo);
        *outView = converter::toGfx<GfxTextureView>(view);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create texture view: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

// TextureView functions
GfxResult ResourceComponent::textureViewDestroy(GfxTextureView textureView) const
{
    GfxResult validationResult = validator::validateTextureViewDestroy(textureView);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::TextureView>(textureView);
    return GFX_RESULT_SUCCESS;
}

// Sampler functions
GfxResult ResourceComponent::deviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler) const
{
    GfxResult validationResult = validator::validateDeviceCreateSampler(device, descriptor, outSampler);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUSamplerCreateInfo(descriptor);
        auto* sampler = new core::Sampler(devicePtr, createInfo);
        *outSampler = converter::toGfx<GfxSampler>(sampler);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create sampler: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult ResourceComponent::samplerDestroy(GfxSampler sampler) const
{
    GfxResult validationResult = validator::validateSamplerDestroy(sampler);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Sampler>(sampler);
    return GFX_RESULT_SUCCESS;
}

// Shader functions
GfxResult ResourceComponent::deviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader) const
{
    GfxResult validationResult = validator::validateDeviceCreateShader(device, descriptor, outShader);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUShaderCreateInfo(descriptor);
        auto* shader = new core::Shader(devicePtr, createInfo);
        *outShader = converter::toGfx<GfxShader>(shader);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create shader: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult ResourceComponent::shaderDestroy(GfxShader shader) const
{
    GfxResult validationResult = validator::validateShaderDestroy(shader);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Shader>(shader);
    return GFX_RESULT_SUCCESS;
}

// BindGroupLayout functions
GfxResult ResourceComponent::deviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout) const
{
    GfxResult validationResult = validator::validateDeviceCreateBindGroupLayout(device, descriptor, outLayout);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUBindGroupLayoutCreateInfo(descriptor);
        auto* layout = new core::BindGroupLayout(devicePtr, createInfo);
        *outLayout = converter::toGfx<GfxBindGroupLayout>(layout);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create bind group layout: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult ResourceComponent::bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout) const
{
    GfxResult validationResult = validator::validateBindGroupLayoutDestroy(bindGroupLayout);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::BindGroupLayout>(bindGroupLayout);
    return GFX_RESULT_SUCCESS;
}

// BindGroup functions
GfxResult ResourceComponent::deviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup) const
{
    GfxResult validationResult = validator::validateDeviceCreateBindGroup(device, descriptor, outBindGroup);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto* layoutPtr = converter::toNative<core::BindGroupLayout>(descriptor->layout);
        auto createInfo = converter::gfxDescriptorToWebGPUBindGroupCreateInfo(descriptor, layoutPtr->handle());
        auto* bindGroup = new core::BindGroup(devicePtr, createInfo);
        *outBindGroup = converter::toGfx<GfxBindGroup>(bindGroup);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create bind group: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult ResourceComponent::bindGroupDestroy(GfxBindGroup bindGroup) const
{
    GfxResult validationResult = validator::validateBindGroupDestroy(bindGroup);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::BindGroup>(bindGroup);
    return GFX_RESULT_SUCCESS;
}

} // namespace gfx::backend::webgpu::component
