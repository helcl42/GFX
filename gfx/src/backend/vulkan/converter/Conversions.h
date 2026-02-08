#ifndef GFX_VULKAN_CONVERSIONS_H
#define GFX_VULKAN_CONVERSIONS_H

#include "../common/Common.h"

#include "../core/CoreTypes.h"

#include <gfx/gfx.h>
namespace gfx::backend::vulkan::converter {

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

core::SemaphoreType gfxSemaphoreTypeToVulkanSemaphoreType(GfxSemaphoreType type);
GfxSemaphoreType vulkanSemaphoreTypeToGfxSemaphoreType(core::SemaphoreType type);
core::ShaderSourceType gfxShaderSourceTypeToVulkanShaderSourceType(GfxShaderSourceType type);

// ============================================================================
// Format Conversion Functions
// ============================================================================

VkFormat gfxFormatToVkFormat(GfxTextureFormat format);
GfxTextureFormat vkFormatToGfxFormat(VkFormat format);
VkBufferUsageFlags gfxBufferUsageToVkBufferUsage(GfxBufferUsageFlags gfxUsage);
GfxBufferUsageFlags vkBufferUsageToGfxBufferUsage(VkBufferUsageFlags vkUsage);
VkMemoryPropertyFlags gfxMemoryPropertyToVkMemoryProperty(GfxMemoryPropertyFlags gfxMemoryProperty);
GfxMemoryPropertyFlags vkMemoryPropertyToGfxMemoryProperty(VkMemoryPropertyFlags vkMemoryProperty);
VkImageUsageFlags gfxTextureUsageToVkImageUsage(GfxTextureUsageFlags gfxUsage, VkFormat format);
GfxTextureUsageFlags vkImageUsageToGfxTextureUsage(VkImageUsageFlags vkUsage);
VkPipelineStageFlags gfxPipelineStageFlagsToVkPipelineStageFlags(GfxPipelineStageFlags gfxStage);
VkAccessFlags gfxAccessFlagsToVkAccessFlags(GfxAccessFlags gfxAccessFlags);
VkIndexType gfxIndexFormatToVkIndexType(GfxIndexFormat format);
core::Viewport gfxViewportToViewport(const GfxViewport* viewport);
core::ScissorRect gfxScissorRectToScissorRect(const GfxScissorRect* scissor);

// ============================================================================
// Barrier Conversion
// ============================================================================

core::MemoryBarrier gfxMemoryBarrierToMemoryBarrier(const GfxMemoryBarrier& barrier);
core::BufferBarrier gfxBufferBarrierToBufferBarrier(const GfxBufferBarrier& barrier);
core::TextureBarrier gfxTextureBarrierToTextureBarrier(const GfxTextureBarrier& barrier);

// ============================================================================
// Device Limits Conversion
// ============================================================================

GfxDeviceLimits vkPropertiesToGfxDeviceLimits(const VkPhysicalDeviceProperties& properties);

// ============================================================================
// Queue Family Conversion
// ============================================================================

GfxQueueFlags vkQueueFlagsToGfx(VkQueueFlags vkFlags);
GfxQueueFamilyProperties vkQueueFamilyPropertiesToGfx(const VkQueueFamilyProperties& vkProps);

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
VkAccessFlags getVkAccessFlagsForLayout(VkImageLayout layout);
VkImageType gfxTextureTypeToVkImageType(GfxTextureType type);
GfxTextureType vkImageTypeToGfxTextureType(VkImageType type);
VkImageViewType gfxTextureViewTypeToVkImageViewType(GfxTextureViewType type);
VkSampleCountFlagBits sampleCountToVkSampleCount(GfxSampleCount sampleCount);
GfxSampleCount vkSampleCountToGfxSampleCount(VkSampleCountFlagBits vkSampleCount);
GfxTextureInfo vkTextureInfoToGfxTextureInfo(const core::TextureInfo& info);
GfxSurfaceInfo vkSurfaceCapabilitiesToGfxSurfaceInfo(const VkSurfaceCapabilitiesKHR& caps);
GfxSwapchainInfo vkSwapchainInfoToGfxSwapchainInfo(const core::SwapchainInfo& info);
GfxBufferInfo vkBufferToGfxBufferInfo(const core::BufferInfo& info);
GfxExtent3D vkExtent3DToGfxExtent3D(const VkExtent3D& vkExtent);
VkExtent3D gfxExtent3DToVkExtent3D(const GfxExtent3D* gfxExtent);
VkOffset3D gfxOrigin3DToVkOffset3D(const GfxOrigin3D* gfxOrigin);
GfxExtent2D vkExtent2DToGfxExtent2D(const VkExtent2D& vkExtent);
VkExtent2D gfxExtent2DToVkExtent2D(const GfxExtent2D& gfxExtent);
VkOffset2D gfxOrigin2DToVkOffset2D(const GfxOrigin2D& gfxOrigin);
GfxAccessFlags vkAccessFlagsToGfxAccessFlags(VkAccessFlags vkAccessFlags);
VkCullModeFlags gfxCullModeToVkCullMode(GfxCullMode cullMode);
VkFrontFace gfxFrontFaceToVkFrontFace(GfxFrontFace frontFace);
VkPolygonMode gfxPolygonModeToVkPolygonMode(GfxPolygonMode polygonMode);
VkPrimitiveTopology gfxPrimitiveTopologyToVkPrimitiveTopology(GfxPrimitiveTopology topology);
VkVertexInputRate gfxVertexStepModeToVkVertexInputRate(GfxVertexStepMode mode);
VkSamplerAddressMode gfxAddressModeToVkAddressMode(GfxAddressMode addressMode);
VkFilter gfxFilterToVkFilter(GfxFilterMode filter);
VkSamplerMipmapMode gfxFilterModeToVkMipMapFilterMode(GfxFilterMode filter);
VkBlendFactor gfxBlendFactorToVkBlendFactor(GfxBlendFactor factor);
VkBlendOp gfxBlendOpToVkBlendOp(GfxBlendOperation op);
VkCompareOp gfxCompareOpToVkCompareOp(GfxCompareFunction func);
VkQueryType gfxQueryTypeToVkQueryType(GfxQueryType type);

// ============================================================================
// XInfo Conversion Functions - GfxDescriptor to Internal XInfo
// ============================================================================

core::BufferCreateInfo gfxDescriptorToBufferCreateInfo(const GfxBufferDescriptor* descriptor);
core::BufferImportInfo gfxExternalDescriptorToBufferImportInfo(const GfxBufferImportDescriptor* descriptor);
core::ShaderCreateInfo gfxDescriptorToShaderCreateInfo(const GfxShaderDescriptor* descriptor);
core::SemaphoreCreateInfo gfxDescriptorToSemaphoreCreateInfo(const GfxSemaphoreDescriptor* descriptor);
core::FenceCreateInfo gfxDescriptorToFenceCreateInfo(const GfxFenceDescriptor* descriptor);
core::TextureCreateInfo gfxDescriptorToTextureCreateInfo(const GfxTextureDescriptor* descriptor);
core::TextureImportInfo gfxExternalDescriptorToTextureImportInfo(const GfxTextureImportDescriptor* descriptor);
core::TextureViewCreateInfo gfxDescriptorToTextureViewCreateInfo(const GfxTextureViewDescriptor* descriptor);
core::SamplerCreateInfo gfxDescriptorToSamplerCreateInfo(const GfxSamplerDescriptor* descriptor);
core::InstanceCreateInfo gfxDescriptorToInstanceCreateInfo(const GfxInstanceDescriptor* descriptor);
core::AdapterCreateInfo gfxDescriptorToAdapterCreateInfo(const GfxAdapterDescriptor* descriptor);
core::DeviceCreateInfo gfxDescriptorToDeviceCreateInfo(const GfxDeviceDescriptor* descriptor);
core::SurfaceCreateInfo gfxDescriptorToSurfaceCreateInfo(const GfxSurfaceDescriptor* descriptor);
core::SwapchainCreateInfo gfxDescriptorToSwapchainCreateInfo(const GfxSwapchainDescriptor* descriptor);
core::BindGroupLayoutCreateInfo gfxDescriptorToBindGroupLayoutCreateInfo(const GfxBindGroupLayoutDescriptor* descriptor);
core::BindGroupCreateInfo gfxDescriptorToBindGroupCreateInfo(const GfxBindGroupDescriptor* descriptor);
core::RenderPipelineCreateInfo gfxDescriptorToRenderPipelineCreateInfo(const GfxRenderPipelineDescriptor* descriptor);
core::ComputePipelineCreateInfo gfxDescriptorToComputePipelineCreateInfo(const GfxComputePipelineDescriptor* descriptor);
core::RenderPassCreateInfo gfxRenderPassDescriptorToRenderPassCreateInfo(const GfxRenderPassDescriptor* descriptor);
core::FramebufferCreateInfo gfxFramebufferDescriptorToFramebufferCreateInfo(const GfxFramebufferDescriptor* descriptor);
core::RenderPassEncoderCreateInfo gfxRenderPassDescriptorToCreateInfo(const GfxRenderPassDescriptor* descriptor);
core::RenderPassEncoderBeginInfo gfxRenderPassBeginDescriptorToBeginInfo(const GfxRenderPassBeginDescriptor* descriptor);
core::ComputePassEncoderCreateInfo gfxComputePassBeginDescriptorToCreateInfo(const GfxComputePassBeginDescriptor* descriptor);
core::SubmitInfo gfxDescriptorToSubmitInfo(const GfxSubmitDescriptor* descriptor);
core::QuerySetCreateInfo gfxDescriptorToQuerySetCreateInfo(const GfxQuerySetDescriptor* descriptor);

} // namespace gfx::backend::vulkan::converter

#endif // GFX_VULKAN_CONVERTER_H