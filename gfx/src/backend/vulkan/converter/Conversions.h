#ifndef GFX_VULKAN_CONVERSIONS_H
#define GFX_VULKAN_CONVERSIONS_H

#include "../common/Common.h"

#include <gfx/gfx.h>

// Forward declare CreateInfo types and internal types
namespace gfx::backend::vulkan {
class Buffer;
class Texture;
class Swapchain;

struct BufferInfo;
struct TextureInfo;
struct SwapchainInfo;

struct BufferCreateInfo;
struct BufferImportInfo;
struct ShaderCreateInfo;
struct SemaphoreCreateInfo;
struct FenceCreateInfo;
struct TextureCreateInfo;
struct TextureImportInfo;
struct TextureViewCreateInfo;
struct SamplerCreateInfo;
struct InstanceCreateInfo;
struct AdapterCreateInfo;
struct DeviceCreateInfo;
struct SurfaceCreateInfo;
struct SwapchainCreateInfo;
struct BindGroupLayoutCreateInfo;
struct BindGroupCreateInfo;
struct RenderPipelineCreateInfo;
struct ComputePipelineCreateInfo;
struct RenderPassCreateInfo;
struct FramebufferCreateInfo;
struct RenderPassEncoderBeginInfo;
struct RenderPassEncoderCreateInfo;
struct ComputePassEncoderCreateInfo;
struct SubmitInfo;
struct MemoryBarrier;
struct BufferBarrier;
struct TextureBarrier;
struct Viewport;
struct ScissorRect;

enum class DebugMessageSeverity;
enum class DebugMessageType;
enum class SemaphoreType;
enum class InstanceFeatureType;
enum class DeviceFeatureType;
} // namespace gfx::backend::vulkan

namespace gfx::backend::vulkan::converter {

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
// Debug Message Conversion Functions
// ============================================================================

DebugMessageSeverity convertVkDebugSeverity(VkDebugUtilsMessageSeverityFlagBitsEXT vkSeverity);
DebugMessageType convertVkDebugType(VkDebugUtilsMessageTypeFlagsEXT vkType);

// ============================================================================
// Type Conversion Functions
// ============================================================================

SemaphoreType gfxSemaphoreTypeToVulkanSemaphoreType(GfxSemaphoreType type);

// ============================================================================
// Format Conversion Functions
// ============================================================================

VkFormat gfxFormatToVkFormat(GfxTextureFormat format);
GfxTextureFormat vkFormatToGfxFormat(VkFormat format);
VkBufferUsageFlags gfxBufferUsageToVkBufferUsage(GfxBufferUsage gfxUsage);
GfxBufferUsage vkBufferUsageToGfxBufferUsage(VkBufferUsageFlags vkUsage);
bool gfxBufferUsageToMappedFlag(GfxBufferUsage gfxUsage);
GfxBufferUsage mappedFlagToVkBufferUsage(bool mapped);
VkImageUsageFlags gfxTextureUsageToVkImageUsage(GfxTextureUsage gfxUsage, VkFormat format);
GfxTextureUsage vkImageUsageToGfxTextureUsage(VkImageUsageFlags vkUsage);
VkPipelineStageFlags gfxPipelineStageFlagsToVkPipelineStageFlags(GfxPipelineStage gfxStage);
VkAccessFlags gfxAccessFlagsToVkAccessFlags(GfxAccessFlags gfxAccessFlags);
VkIndexType gfxIndexFormatToVkIndexType(GfxIndexFormat format);
Viewport gfxViewportToViewport(const GfxViewport* viewport);
ScissorRect gfxScissorRectToScissorRect(const GfxScissorRect* scissor);

// ============================================================================
// Barrier Conversion
// ============================================================================

MemoryBarrier gfxMemoryBarrierToMemoryBarrier(const GfxMemoryBarrier& barrier);
BufferBarrier gfxBufferBarrierToBufferBarrier(const GfxBufferBarrier& barrier);
TextureBarrier gfxTextureBarrierToTextureBarrier(const GfxTextureBarrier& barrier);

// ============================================================================
// Device Limits Conversion
// ============================================================================

GfxDeviceLimits vkPropertiesToGfxDeviceLimits(const VkPhysicalDeviceProperties& properties);

// ============================================================================
// Adapter Type Conversion
// ============================================================================

GfxAdapterType vkDeviceTypeToGfxAdapterType(VkPhysicalDeviceType deviceType);

// ============================================================================
// Adapter Info Conversion
// ============================================================================

GfxAdapterInfo vkPropertiesToGfxAdapterInfo(const VkPhysicalDeviceProperties& properties);

// ============================================================================
// Other Format Functions
// ============================================================================

bool isDepthFormat(VkFormat format);
VkAttachmentLoadOp gfxLoadOpToVkLoadOp(GfxLoadOp loadOp);
VkAttachmentStoreOp gfxStoreOpToVkStoreOp(GfxStoreOp storeOp);
GfxPresentMode vkPresentModeToGfxPresentMode(VkPresentModeKHR mode);
VkPresentModeKHR gfxPresentModeToVkPresentMode(GfxPresentMode mode);
bool hasStencilComponent(VkFormat format);
VkImageAspectFlags getImageAspectMask(VkFormat format);
VkImageLayout gfxLayoutToVkImageLayout(GfxTextureLayout layout);
GfxTextureLayout vkImageLayoutToGfxLayout(VkImageLayout layout);
VkImageType gfxTextureTypeToVkImageType(GfxTextureType type);
GfxTextureType vkImageTypeToGfxTextureType(VkImageType type);
VkImageViewType gfxTextureViewTypeToVkImageViewType(GfxTextureViewType type);
VkSampleCountFlagBits sampleCountToVkSampleCount(GfxSampleCount sampleCount);
GfxSampleCount vkSampleCountToGfxSampleCount(VkSampleCountFlagBits vkSampleCount);
GfxTextureInfo vkTextureInfoToGfxTextureInfo(const TextureInfo& info);
GfxSwapchainInfo vkSwapchainInfoToGfxSwapchainInfo(const SwapchainInfo& info);
GfxBufferInfo vkBufferToGfxBufferInfo(const BufferInfo& info);
GfxExtent3D vkExtent3DToGfxExtent3D(const VkExtent3D& vkExtent);
VkExtent3D gfxExtent3DToVkExtent3D(const GfxExtent3D* gfxExtent);
VkOffset3D gfxOrigin3DToVkOffset3D(const GfxOrigin3D* gfxOrigin);
GfxAccessFlags vkAccessFlagsToGfxAccessFlags(VkAccessFlags vkAccessFlags);
VkCullModeFlags gfxCullModeToVkCullMode(GfxCullMode cullMode);
VkFrontFace gfxFrontFaceToVkFrontFace(GfxFrontFace frontFace);
VkPolygonMode gfxPolygonModeToVkPolygonMode(GfxPolygonMode polygonMode);
VkPrimitiveTopology gfxPrimitiveTopologyToVkPrimitiveTopology(GfxPrimitiveTopology topology);
VkSamplerAddressMode gfxAddressModeToVkAddressMode(GfxAddressMode addressMode);
VkFilter gfxFilterToVkFilter(GfxFilterMode filter);
VkSamplerMipmapMode gfxFilterModeToVkMipMapFilterMode(GfxFilterMode filter);
VkBlendFactor gfxBlendFactorToVkBlendFactor(GfxBlendFactor factor);
VkBlendOp gfxBlendOpToVkBlendOp(GfxBlendOperation op);
VkCompareOp gfxCompareOpToVkCompareOp(GfxCompareFunction func);
const char* vkResultToString(VkResult result);

// ============================================================================
// CreateInfo Conversion Functions - GfxDescriptor to Internal CreateInfo
// ============================================================================

BufferCreateInfo gfxDescriptorToBufferCreateInfo(const GfxBufferDescriptor* descriptor);
BufferImportInfo gfxExternalDescriptorToBufferImportInfo(const GfxExternalBufferDescriptor* descriptor);
ShaderCreateInfo gfxDescriptorToShaderCreateInfo(const GfxShaderDescriptor* descriptor);
SemaphoreCreateInfo gfxDescriptorToSemaphoreCreateInfo(const GfxSemaphoreDescriptor* descriptor);
FenceCreateInfo gfxDescriptorToFenceCreateInfo(const GfxFenceDescriptor* descriptor);
TextureCreateInfo gfxDescriptorToTextureCreateInfo(const GfxTextureDescriptor* descriptor);
TextureImportInfo gfxExternalDescriptorToTextureImportInfo(const GfxExternalTextureDescriptor* descriptor);
TextureViewCreateInfo gfxDescriptorToTextureViewCreateInfo(const GfxTextureViewDescriptor* descriptor);
SamplerCreateInfo gfxDescriptorToSamplerCreateInfo(const GfxSamplerDescriptor* descriptor);
InstanceCreateInfo gfxDescriptorToInstanceCreateInfo(const GfxInstanceDescriptor* descriptor);
AdapterCreateInfo gfxDescriptorToAdapterCreateInfo(const GfxAdapterDescriptor* descriptor);
DeviceCreateInfo gfxDescriptorToDeviceCreateInfo(const GfxDeviceDescriptor* descriptor);
SurfaceCreateInfo gfxDescriptorToSurfaceCreateInfo(const GfxSurfaceDescriptor* descriptor);
SwapchainCreateInfo gfxDescriptorToSwapchainCreateInfo(const GfxSwapchainDescriptor* descriptor);
BindGroupLayoutCreateInfo gfxDescriptorToBindGroupLayoutCreateInfo(const GfxBindGroupLayoutDescriptor* descriptor);

// ============================================================================
// Entity-dependent CreateInfo Conversion Functions
// These require full entity definitions (Shader, BindGroupLayout, etc.)
// ============================================================================

BindGroupCreateInfo gfxDescriptorToBindGroupCreateInfo(const GfxBindGroupDescriptor* descriptor);
RenderPipelineCreateInfo gfxDescriptorToRenderPipelineCreateInfo(const GfxRenderPipelineDescriptor* descriptor);
ComputePipelineCreateInfo gfxDescriptorToComputePipelineCreateInfo(const GfxComputePipelineDescriptor* descriptor);
RenderPassCreateInfo gfxRenderPassDescriptorToRenderPassCreateInfo(const GfxRenderPassDescriptor* descriptor);
FramebufferCreateInfo gfxFramebufferDescriptorToFramebufferCreateInfo(const GfxFramebufferDescriptor* descriptor);
RenderPassEncoderCreateInfo gfxRenderPassDescriptorToCreateInfo(const GfxRenderPassDescriptor* descriptor);
RenderPassEncoderBeginInfo gfxRenderPassBeginDescriptorToBeginInfo(const GfxRenderPassBeginDescriptor* descriptor);
ComputePassEncoderCreateInfo gfxComputePassBeginDescriptorToCreateInfo(const GfxComputePassBeginDescriptor* descriptor);
SubmitInfo gfxDescriptorToSubmitInfo(const GfxSubmitInfo* descriptor);

} // namespace gfx::backend::vulkan::converter

#endif // GFX_VULKAN_CONVERTER_H