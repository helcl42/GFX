#pragma once

#include "../common/VulkanCommon.h"

#include <gfx/gfx.h>

// Forward declare CreateInfo types and internal types
namespace gfx::vulkan {
struct BufferCreateInfo;
struct ShaderCreateInfo;
struct SemaphoreCreateInfo;
struct FenceCreateInfo;
struct TextureCreateInfo;
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
struct RenderPassEncoderCreateInfo;
struct ComputePassEncoderCreateInfo;
struct SubmitInfo;

enum class DebugMessageSeverity;
enum class DebugMessageType;
enum class SemaphoreType;
} // namespace gfx::vulkan

namespace gfx::vulkan::converter {

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

gfx::vulkan::DebugMessageSeverity convertVkDebugSeverity(VkDebugUtilsMessageSeverityFlagBitsEXT vkSeverity);
gfx::vulkan::DebugMessageType convertVkDebugType(VkDebugUtilsMessageTypeFlagsEXT vkType);

// ============================================================================
// Type Conversion Functions
// ============================================================================

gfx::vulkan::SemaphoreType gfxSemaphoreTypeToVulkanSemaphoreType(GfxSemaphoreType type);

// ============================================================================
// Format Conversion Functions
// ============================================================================

VkFormat gfxFormatToVkFormat(GfxTextureFormat format);
GfxTextureFormat vkFormatToGfxFormat(VkFormat format);
GfxBufferUsage vkBufferUsageToGfxBufferUsage(VkBufferUsageFlags vkUsage);
GfxTextureUsage vkImageUsageToGfxTextureUsage(VkImageUsageFlags vkUsage);

// ============================================================================
// Device Limits Conversion
// ============================================================================

GfxDeviceLimits vkPropertiesToGfxDeviceLimits(const VkPhysicalDeviceProperties& properties);

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
VkImageViewType gfxTextureViewTypeToVkImageViewType(GfxTextureViewType type);
VkSampleCountFlagBits sampleCountToVkSampleCount(GfxSampleCount sampleCount);
GfxSampleCount vkSampleCountToGfxSampleCount(VkSampleCountFlagBits vkSampleCount);
GfxExtent3D vkExtent3DToGfxExtent3D(const VkExtent3D& vkExtent);
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

gfx::vulkan::BufferCreateInfo gfxDescriptorToBufferCreateInfo(const GfxBufferDescriptor* descriptor);
gfx::vulkan::ShaderCreateInfo gfxDescriptorToShaderCreateInfo(const GfxShaderDescriptor* descriptor);
gfx::vulkan::SemaphoreCreateInfo gfxDescriptorToSemaphoreCreateInfo(const GfxSemaphoreDescriptor* descriptor);
gfx::vulkan::FenceCreateInfo gfxDescriptorToFenceCreateInfo(const GfxFenceDescriptor* descriptor);
gfx::vulkan::TextureCreateInfo gfxDescriptorToTextureCreateInfo(const GfxTextureDescriptor* descriptor);
gfx::vulkan::TextureViewCreateInfo gfxDescriptorToTextureViewCreateInfo(const GfxTextureViewDescriptor* descriptor);
gfx::vulkan::SamplerCreateInfo gfxDescriptorToSamplerCreateInfo(const GfxSamplerDescriptor* descriptor);
gfx::vulkan::InstanceCreateInfo gfxDescriptorToInstanceCreateInfo(const GfxInstanceDescriptor* descriptor);
gfx::vulkan::AdapterCreateInfo gfxDescriptorToAdapterCreateInfo(const GfxAdapterDescriptor* descriptor);
gfx::vulkan::DeviceCreateInfo gfxDescriptorToDeviceCreateInfo(const GfxDeviceDescriptor* descriptor);
gfx::vulkan::SurfaceCreateInfo gfxDescriptorToSurfaceCreateInfo(const GfxSurfaceDescriptor* descriptor);
gfx::vulkan::SwapchainCreateInfo gfxDescriptorToSwapchainCreateInfo(const GfxSwapchainDescriptor* descriptor);
gfx::vulkan::BindGroupLayoutCreateInfo gfxDescriptorToBindGroupLayoutCreateInfo(const GfxBindGroupLayoutDescriptor* descriptor);

// ============================================================================
// Entity-dependent CreateInfo Conversion Functions
// These require full entity definitions (Shader, BindGroupLayout, etc.)
// ============================================================================

gfx::vulkan::BindGroupCreateInfo gfxDescriptorToBindGroupCreateInfo(const GfxBindGroupDescriptor* descriptor);
gfx::vulkan::RenderPipelineCreateInfo gfxDescriptorToRenderPipelineCreateInfo(const GfxRenderPipelineDescriptor* descriptor);
gfx::vulkan::ComputePipelineCreateInfo gfxDescriptorToComputePipelineCreateInfo(const GfxComputePipelineDescriptor* descriptor);
gfx::vulkan::RenderPassEncoderCreateInfo gfxRenderPassDescriptorToCreateInfo(const GfxRenderPassDescriptor* descriptor);
gfx::vulkan::ComputePassEncoderCreateInfo gfxComputePassDescriptorToCreateInfo(const GfxComputePassDescriptor* descriptor);
gfx::vulkan::SubmitInfo gfxDescriptorToSubmitInfo(const GfxSubmitInfo* descriptor);

} // namespace gfx::vulkan::converter
