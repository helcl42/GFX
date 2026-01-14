#ifndef GFX_WEBGPU_CONVERSIONS_H
#define GFX_WEBGPU_CONVERSIONS_H

#include "../common/Common.h"

#include <gfx/gfx.h>

// Forward declare internal WebGPU types
namespace gfx::backend::webgpu {
class Buffer;
class Texture;
class Swapchain;

struct AdapterInfo;
struct BufferInfo;
struct TextureInfo;
struct SwapchainInfo;

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
} // namespace gfx::backend::webgpu

// ============================================================================
// WebGPU Conversion Functions
// ============================================================================

namespace gfx::backend::webgpu::converter {

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

SemaphoreType gfxSemaphoreTypeToWebGPUSemaphoreType(GfxSemaphoreType gfxType);

// ============================================================================
// Adapter Type Conversion
// ============================================================================

GfxAdapterType wgpuAdapterTypeToGfxAdapterType(WGPUAdapterType adapterType);

// ============================================================================
// Adapter Info Conversion
// ============================================================================

GfxAdapterInfo wgpuAdapterToGfxAdapterInfo(const AdapterInfo& info);

// ============================================================================
// CreateInfo Conversion Functions - GfxDescriptor to Internal CreateInfo
// ============================================================================

AdapterCreateInfo gfxDescriptorToWebGPUAdapterCreateInfo(const GfxAdapterDescriptor* descriptor);
InstanceCreateInfo gfxDescriptorToWebGPUInstanceCreateInfo(const GfxInstanceDescriptor* descriptor);
DeviceCreateInfo gfxDescriptorToWebGPUDeviceCreateInfo(const GfxDeviceDescriptor* descriptor);
BufferCreateInfo gfxDescriptorToWebGPUBufferCreateInfo(const GfxBufferDescriptor* descriptor);
BufferImportInfo gfxExternalDescriptorToWebGPUBufferImportInfo(const GfxExternalBufferDescriptor* descriptor);
TextureCreateInfo gfxDescriptorToWebGPUTextureCreateInfo(const GfxTextureDescriptor* descriptor);
TextureImportInfo gfxExternalDescriptorToWebGPUTextureImportInfo(const GfxExternalTextureDescriptor* descriptor);
TextureViewCreateInfo gfxDescriptorToWebGPUTextureViewCreateInfo(const GfxTextureViewDescriptor* descriptor);
ShaderCreateInfo gfxDescriptorToWebGPUShaderCreateInfo(const GfxShaderDescriptor* descriptor);
SamplerCreateInfo gfxDescriptorToWebGPUSamplerCreateInfo(const GfxSamplerDescriptor* descriptor);
SemaphoreCreateInfo gfxDescriptorToWebGPUSemaphoreCreateInfo(const GfxSemaphoreDescriptor* descriptor);
FenceCreateInfo gfxDescriptorToWebGPUFenceCreateInfo(const GfxFenceDescriptor* descriptor);
SurfaceCreateInfo gfxDescriptorToWebGPUSurfaceCreateInfo(const GfxSurfaceDescriptor* descriptor);
SwapchainCreateInfo gfxDescriptorToWebGPUSwapchainCreateInfo(const GfxSwapchainDescriptor* descriptor);
BindGroupLayoutCreateInfo gfxDescriptorToWebGPUBindGroupLayoutCreateInfo(const GfxBindGroupLayoutDescriptor* descriptor);

// Entity-dependent CreateInfo Conversion Functions
BindGroupCreateInfo gfxDescriptorToWebGPUBindGroupCreateInfo(const GfxBindGroupDescriptor* descriptor, WGPUBindGroupLayout layout);
RenderPipelineCreateInfo gfxDescriptorToWebGPURenderPipelineCreateInfo(const GfxRenderPipelineDescriptor* descriptor);
ComputePipelineCreateInfo gfxDescriptorToWebGPUComputePipelineCreateInfo(const GfxComputePipelineDescriptor* descriptor);
CommandEncoderCreateInfo gfxDescriptorToWebGPUCommandEncoderCreateInfo(const GfxCommandEncoderDescriptor* descriptor);
SubmitInfo gfxDescriptorToWebGPUSubmitInfo(const GfxSubmitInfo* descriptor);

// ============================================================================
// Reverse conversions - internal to Gfx API types
// ============================================================================

GfxBufferUsage webgpuBufferUsageToGfxBufferUsage(WGPUBufferUsage usage);
GfxSemaphoreType webgpuSemaphoreTypeToGfxSemaphoreType(SemaphoreType type);
GfxTextureInfo wgpuTextureInfoToGfxTextureInfo(const TextureInfo& info);
GfxSwapchainInfo wgpuSwapchainInfoToGfxSwapchainInfo(const SwapchainInfo& info);
GfxBufferInfo wgpuBufferToGfxBufferInfo(const BufferInfo& info);

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

} // namespace gfx::backend::webgpu::converter

#endif // GFX_WEBGPU_CONVERTER_H