#pragma once

#include "../common/WebGPUCommon.h"

#include <gfx/gfx.h>

// Forward declare internal WebGPU types
namespace gfx::webgpu {
struct AdapterInfo;
struct TextureInfo;

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

enum class SemaphoreType;
} // namespace gfx::webgpu

// ============================================================================
// WebGPU Conversion Functions
// ============================================================================

namespace gfx::webgpu::converter {

// Template function to convert internal C++ pointer to opaque C handle
// Usage: toGfx<GfxDevice>(devicePtr)
template <typename GfxHandle, typename InternalType>
inline GfxHandle toGfx(InternalType* ptr)
{
    return reinterpret_cast<GfxHandle>(ptr);
}

// Template function to convert opaque C handle to internal C++ pointer
// Usage: toNative<Device>(device)
template <typename InternalType, typename GfxHandle>
inline InternalType* toNative(GfxHandle handle)
{
    return reinterpret_cast<InternalType*>(handle);
}

// ============================================================================
// Type Conversion Functions
// ============================================================================

gfx::webgpu::SemaphoreType gfxSemaphoreTypeToWebGPUSemaphoreType(GfxSemaphoreType gfxType);

// ============================================================================
// Adapter Type Conversion
// ============================================================================

GfxAdapterType wgpuAdapterTypeToGfxAdapterType(WGPUAdapterType adapterType);

// ============================================================================
// Adapter Info Conversion
// ============================================================================

GfxAdapterInfo wgpuAdapterToGfxAdapterInfo(const gfx::webgpu::AdapterInfo& info);

// ============================================================================
// CreateInfo Conversion Functions - GfxDescriptor to Internal CreateInfo
// ============================================================================

gfx::webgpu::AdapterCreateInfo gfxDescriptorToWebGPUAdapterCreateInfo(const GfxAdapterDescriptor* descriptor);
gfx::webgpu::InstanceCreateInfo gfxDescriptorToWebGPUInstanceCreateInfo(const GfxInstanceDescriptor* descriptor);
gfx::webgpu::DeviceCreateInfo gfxDescriptorToWebGPUDeviceCreateInfo(const GfxDeviceDescriptor* descriptor);
gfx::webgpu::BufferCreateInfo gfxDescriptorToWebGPUBufferCreateInfo(const GfxBufferDescriptor* descriptor);
gfx::webgpu::BufferImportInfo gfxExternalDescriptorToWebGPUBufferImportInfo(const GfxExternalBufferDescriptor* descriptor);
gfx::webgpu::TextureCreateInfo gfxDescriptorToWebGPUTextureCreateInfo(const GfxTextureDescriptor* descriptor);
gfx::webgpu::TextureImportInfo gfxExternalDescriptorToWebGPUTextureImportInfo(const GfxExternalTextureDescriptor* descriptor);
gfx::webgpu::TextureViewCreateInfo gfxDescriptorToWebGPUTextureViewCreateInfo(const GfxTextureViewDescriptor* descriptor);
gfx::webgpu::ShaderCreateInfo gfxDescriptorToWebGPUShaderCreateInfo(const GfxShaderDescriptor* descriptor);
gfx::webgpu::SamplerCreateInfo gfxDescriptorToWebGPUSamplerCreateInfo(const GfxSamplerDescriptor* descriptor);
gfx::webgpu::SemaphoreCreateInfo gfxDescriptorToWebGPUSemaphoreCreateInfo(const GfxSemaphoreDescriptor* descriptor);
gfx::webgpu::FenceCreateInfo gfxDescriptorToWebGPUFenceCreateInfo(const GfxFenceDescriptor* descriptor);
gfx::webgpu::SurfaceCreateInfo gfxDescriptorToWebGPUSurfaceCreateInfo(const GfxSurfaceDescriptor* descriptor);
gfx::webgpu::SwapchainCreateInfo gfxDescriptorToWebGPUSwapchainCreateInfo(const GfxSwapchainDescriptor* descriptor);
gfx::webgpu::BindGroupLayoutCreateInfo gfxDescriptorToWebGPUBindGroupLayoutCreateInfo(const GfxBindGroupLayoutDescriptor* descriptor);

// Entity-dependent CreateInfo Conversion Functions
gfx::webgpu::BindGroupCreateInfo gfxDescriptorToWebGPUBindGroupCreateInfo(const GfxBindGroupDescriptor* descriptor, WGPUBindGroupLayout layout);
gfx::webgpu::RenderPipelineCreateInfo gfxDescriptorToWebGPURenderPipelineCreateInfo(const GfxRenderPipelineDescriptor* descriptor);
gfx::webgpu::ComputePipelineCreateInfo gfxDescriptorToWebGPUComputePipelineCreateInfo(const GfxComputePipelineDescriptor* descriptor);
gfx::webgpu::CommandEncoderCreateInfo gfxDescriptorToWebGPUCommandEncoderCreateInfo(const GfxCommandEncoderDescriptor* descriptor);
gfx::webgpu::SubmitInfo gfxDescriptorToWebGPUSubmitInfo(const GfxSubmitInfo* descriptor);

// ============================================================================
// Reverse conversions - internal to Gfx API types
// ============================================================================

GfxBufferUsage webgpuBufferUsageToGfxBufferUsage(WGPUBufferUsage usage);
GfxSemaphoreType webgpuSemaphoreTypeToGfxSemaphoreType(gfx::webgpu::SemaphoreType type);
GfxTextureInfo wgpuTextureInfoToGfxTextureInfo(const gfx::webgpu::TextureInfo& info);

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
WGPUBufferUsage gfxBufferUsageToWGPU(GfxBufferUsage usage);

// Texture usage conversions
WGPUTextureUsage gfxTextureUsageToWGPU(GfxTextureUsage usage);
GfxTextureUsage wgpuTextureUsageToGfxTextureUsage(WGPUTextureUsage usage);

// Sampler conversions
WGPUAddressMode gfxAddressModeToWGPU(GfxAddressMode mode);
WGPUFilterMode gfxFilterModeToWGPU(GfxFilterMode mode);
WGPUMipmapFilterMode gfxMipmapFilterModeToWGPU(GfxFilterMode mode);

// Pipeline state conversions
WGPUPrimitiveTopology gfxPrimitiveTopologyToWGPU(GfxPrimitiveTopology topology);
WGPUFrontFace gfxFrontFaceToWGPU(GfxFrontFace frontFace);
WGPUCullMode gfxCullModeToWGPU(GfxCullMode cullMode);
WGPUIndexFormat gfxIndexFormatToWGPU(GfxIndexFormat format);

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
RenderPassCreateInfo gfxRenderPassDescriptorToRenderPassCreateInfo(
    const GfxRenderPassDescriptor* descriptor);

FramebufferCreateInfo gfxFramebufferDescriptorToFramebufferCreateInfo(
    const GfxFramebufferDescriptor* descriptor);

RenderPassEncoderBeginInfo gfxRenderPassBeginDescriptorToBeginInfo(
    const GfxRenderPassBeginDescriptor* descriptor);

ComputePassEncoderCreateInfo gfxComputePassBeginDescriptorToCreateInfo(
    const GfxComputePassBeginDescriptor* descriptor);

} // namespace gfx::webgpu::converter
