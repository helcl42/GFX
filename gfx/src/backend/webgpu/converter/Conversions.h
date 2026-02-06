#ifndef GFX_WEBGPU_CONVERSIONS_H
#define GFX_WEBGPU_CONVERSIONS_H

#include "../common/Common.h"

#include <gfx/gfx.h>

// Forward declare internal WebGPU types
namespace gfx::backend::webgpu {
namespace core {
    struct AdapterInfo;
    struct BufferInfo;
    struct TextureInfo;
    struct SwapchainInfo;
    struct QueueFamilyProperties;

    struct AdapterCreateInfo;
    struct InstanceCreateInfo;
    struct DeviceCreateInfo;
    struct BufferCreateInfo;
    struct BufferImportInfo;
    struct TextureCreateInfo;
    struct TextureImportInfo;
    struct TextureViewCreateInfo;
    struct ShaderCreateInfo;
    struct SamplerCreateInfo;
    struct SemaphoreCreateInfo;
    struct FenceCreateInfo;
    struct SurfaceCreateInfo;
    struct SwapchainCreateInfo;
    struct BindGroupLayoutCreateInfo;
    struct BindGroupCreateInfo;
    struct RenderPipelineCreateInfo;
    struct ComputePipelineCreateInfo;
    struct CommandEncoderCreateInfo;
    struct RenderPassCreateInfo;
    struct FramebufferCreateInfo;
    struct RenderPassEncoderBeginInfo;
    struct ComputePassEncoderCreateInfo;

    struct SubmitInfo;
    struct PlatformWindowHandle;
    struct QuerySetCreateInfo;

    enum class SemaphoreType;
    enum class InstanceFeatureType;
    enum class DeviceFeatureType;
    enum class ShaderSourceType;

    class Buffer;
    class Texture;
    class Swapchain;
} // namespace core
} // namespace gfx::backend::webgpu

// ============================================================================
// WebGPU Conversion Functions
// ============================================================================

namespace gfx::backend::webgpu::converter {

// Template function to convert internal C++ pointer to opaque C handle
// Usage: toGfx<GfxDevice>(devicePtr)
template <typename GfxHandle, typename InternalType>
inline GfxHandle toGfx(InternalType* ptr) noexcept
{
    return reinterpret_cast<GfxHandle>(ptr);
}

// Template function to convert opaque C handle to internal C++ pointer
// Usage: toNative<Device>(device)
template <typename InternalType, typename GfxHandle>
inline InternalType* toNative(GfxHandle handle) noexcept
{
    return reinterpret_cast<InternalType*>(handle);
}

// ============================================================================
// Extension Name Mapping
// ============================================================================

// Map internal instance extension name to public API constant
const char* instanceExtensionNameToGfx(const char* internalName);

// Map internal device extension name to public API constant
const char* deviceExtensionNameToGfx(const char* internalName);

// ============================================================================
// Type Conversion Functions
// ============================================================================

core::SemaphoreType gfxSemaphoreTypeToWebGPUSemaphoreType(GfxSemaphoreType gfxType);
core::ShaderSourceType gfxShaderSourceTypeToWebGPUShaderSourceType(GfxShaderSourceType type);

// ============================================================================
// Adapter Type Conversion
// ============================================================================

GfxAdapterType wgpuAdapterTypeToGfxAdapterType(WGPUAdapterType adapterType);

// ============================================================================
// Adapter Info Conversion
// ============================================================================

GfxAdapterInfo wgpuAdapterToGfxAdapterInfo(const core::AdapterInfo& info);

// ============================================================================
// Queue Family Conversion
// ============================================================================

GfxQueueFamilyProperties wgpuQueueFamilyPropertiesToGfx(const core::QueueFamilyProperties& props);

// ============================================================================
// CreateInfo Conversion Functions - GfxDescriptor to Internal CreateInfo
// ============================================================================

core::AdapterCreateInfo gfxDescriptorToWebGPUAdapterCreateInfo(const GfxAdapterDescriptor* descriptor);
core::InstanceCreateInfo gfxDescriptorToWebGPUInstanceCreateInfo(const GfxInstanceDescriptor* descriptor);
core::DeviceCreateInfo gfxDescriptorToWebGPUDeviceCreateInfo(const GfxDeviceDescriptor* descriptor);
core::BufferCreateInfo gfxDescriptorToWebGPUBufferCreateInfo(const GfxBufferDescriptor* descriptor);
core::BufferImportInfo gfxExternalDescriptorToWebGPUBufferImportInfo(const GfxBufferImportDescriptor* descriptor);
core::TextureCreateInfo gfxDescriptorToWebGPUTextureCreateInfo(const GfxTextureDescriptor* descriptor);
core::TextureImportInfo gfxExternalDescriptorToWebGPUTextureImportInfo(const GfxTextureImportDescriptor* descriptor);
core::TextureViewCreateInfo gfxDescriptorToWebGPUTextureViewCreateInfo(const GfxTextureViewDescriptor* descriptor);
core::ShaderSourceType gfxShaderSourceTypeToWebGPU(GfxShaderSourceType sourceType);
core::ShaderCreateInfo gfxDescriptorToWebGPUShaderCreateInfo(const GfxShaderDescriptor* descriptor);
core::SamplerCreateInfo gfxDescriptorToWebGPUSamplerCreateInfo(const GfxSamplerDescriptor* descriptor);
core::SemaphoreCreateInfo gfxDescriptorToWebGPUSemaphoreCreateInfo(const GfxSemaphoreDescriptor* descriptor);
core::FenceCreateInfo gfxDescriptorToWebGPUFenceCreateInfo(const GfxFenceDescriptor* descriptor);
core::SurfaceCreateInfo gfxDescriptorToWebGPUSurfaceCreateInfo(const GfxSurfaceDescriptor* descriptor);
core::SwapchainCreateInfo gfxDescriptorToWebGPUSwapchainCreateInfo(const GfxSwapchainDescriptor* descriptor);
core::BindGroupLayoutCreateInfo gfxDescriptorToWebGPUBindGroupLayoutCreateInfo(const GfxBindGroupLayoutDescriptor* descriptor);
core::QuerySetCreateInfo gfxDescriptorToWebGPUQuerySetCreateInfo(const GfxQuerySetDescriptor* descriptor);

// Entity-dependent CreateInfo Conversion Functions
core::BindGroupCreateInfo gfxDescriptorToWebGPUBindGroupCreateInfo(const GfxBindGroupDescriptor* descriptor, WGPUBindGroupLayout layout);
core::RenderPipelineCreateInfo gfxDescriptorToWebGPURenderPipelineCreateInfo(const GfxRenderPipelineDescriptor* descriptor);
core::ComputePipelineCreateInfo gfxDescriptorToWebGPUComputePipelineCreateInfo(const GfxComputePipelineDescriptor* descriptor);
core::CommandEncoderCreateInfo gfxDescriptorToWebGPUCommandEncoderCreateInfo(const GfxCommandEncoderDescriptor* descriptor);
core::SubmitInfo gfxDescriptorToWebGPUSubmitInfo(const GfxSubmitDescriptor* descriptor);

// ============================================================================
// Reverse conversions - internal to Gfx API types
// ============================================================================

GfxBufferUsageFlags webgpuBufferUsageToGfxBufferUsage(WGPUBufferUsage usage);
GfxSemaphoreType webgpuSemaphoreTypeToGfxSemaphoreType(core::SemaphoreType type);
GfxTextureInfo wgpuTextureInfoToGfxTextureInfo(const core::TextureInfo& info);
GfxSwapchainInfo wgpuSwapchainInfoToGfxSwapchainInfo(const core::SwapchainInfo& info);
GfxBufferInfo wgpuBufferToGfxBufferInfo(const core::BufferInfo& info);

// ============================================================================
// String utilities
// ============================================================================

WGPUStringView gfxStringView(const char* str);

// ============================================================================
// Texture format conversions
// ============================================================================

WGPUTextureFormat gfxFormatToWGPUFormat(GfxTextureFormat format);
GfxTextureFormat wgpuFormatToGfxFormat(WGPUTextureFormat format);

// Present mode conversions
GfxPresentMode wgpuPresentModeToGfxPresentMode(WGPUPresentMode mode);
WGPUPresentMode gfxPresentModeToWGPU(GfxPresentMode mode);

// Sample count conversions
GfxSampleCount wgpuSampleCountToGfxSampleCount(uint32_t sampleCount);

// Utility functions
bool formatHasStencil(GfxTextureFormat format);

// Device limits conversion
GfxDeviceLimits wgpuLimitsToGfxDeviceLimits(const WGPULimits& wgpuLimits);

// Load/Store operations
WGPULoadOp gfxLoadOpToWGPULoadOp(GfxLoadOp loadOp);
WGPUStoreOp gfxStoreOpToWGPUStoreOp(GfxStoreOp storeOp);

// Buffer usage conversions
WGPUBufferUsage gfxBufferUsageToWGPU(GfxBufferUsageFlags usage);

// Texture usage conversions
WGPUTextureUsage gfxTextureUsageToWGPU(GfxTextureUsageFlags usage);
GfxTextureUsageFlags wgpuTextureUsageToGfxTextureUsage(WGPUTextureUsage usage);

// Sampler conversions
WGPUAddressMode gfxAddressModeToWGPU(GfxAddressMode mode);
WGPUFilterMode gfxFilterModeToWGPU(GfxFilterMode mode);
WGPUMipmapFilterMode gfxMipmapFilterModeToWGPU(GfxFilterMode mode);

// Pipeline state conversions
WGPUPrimitiveTopology gfxPrimitiveTopologyToWGPU(GfxPrimitiveTopology topology);
WGPUFrontFace gfxFrontFaceToWGPU(GfxFrontFace frontFace);
WGPUCullMode gfxCullModeToWGPU(GfxCullMode cullMode);
WGPUIndexFormat gfxIndexFormatToWGPU(GfxIndexFormat format);
WGPUVertexStepMode gfxVertexStepModeToWGPU(GfxVertexStepMode mode);

// Blend state conversions
WGPUBlendOperation gfxBlendOperationToWGPU(GfxBlendOperation operation);
WGPUBlendFactor gfxBlendFactorToWGPU(GfxBlendFactor factor);

// Depth/Stencil conversions
WGPUCompareFunction gfxCompareFunctionToWGPU(GfxCompareFunction func);
WGPUStencilOperation gfxStencilOperationToWGPU(GfxStencilOperation op);

// Texture binding conversions
WGPUTextureSampleType gfxTextureSampleTypeToWGPU(GfxTextureSampleType sampleType);

// Vertex format conversions
WGPUVertexFormat gfxFormatToWGPUVertexFormat(GfxTextureFormat format);

// Texture dimension conversions
WGPUTextureDimension gfxTextureTypeToWGPUTextureDimension(GfxTextureType type);
GfxTextureType wgpuTextureDimensionToGfxTextureType(WGPUTextureDimension dimension);
WGPUTextureViewDimension gfxTextureViewTypeToWGPU(GfxTextureViewType type);

// Geometry conversions
WGPUOrigin3D gfxOrigin3DToWGPUOrigin3D(const GfxOrigin3D* origin);
WGPUExtent3D gfxExtent3DToWGPUExtent3D(const GfxExtent3D* extent);
GfxExtent3D wgpuExtent3DToGfxExtent3D(const WGPUExtent3D& extent);

// CreateInfo conversions
core::RenderPassCreateInfo gfxRenderPassDescriptorToRenderPassCreateInfo(
    const GfxRenderPassDescriptor* descriptor);

core::FramebufferCreateInfo gfxFramebufferDescriptorToFramebufferCreateInfo(
    const GfxFramebufferDescriptor* descriptor);

core::RenderPassEncoderBeginInfo gfxRenderPassBeginDescriptorToBeginInfo(
    const GfxRenderPassBeginDescriptor* descriptor);

core::ComputePassEncoderCreateInfo gfxComputePassBeginDescriptorToCreateInfo(
    const GfxComputePassBeginDescriptor* descriptor);

} // namespace gfx::backend::webgpu::converter

#endif // GFX_WEBGPU_CONVERTER_H