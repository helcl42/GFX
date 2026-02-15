#include "Conversions.h"

#include "../common/Common.h"
#include "../core/query/QuerySet.h"
#include "../core/render/RenderPass.h"
#include "../core/resource/BindGroup.h"
#include "../core/resource/BindGroupLayout.h"
#include "../core/resource/Buffer.h"
#include "../core/resource/Sampler.h"
#include "../core/resource/Shader.h"
#include "../core/resource/Texture.h"
#include "../core/resource/TextureView.h"

#include <cstring>
#include <vector>

namespace gfx::backend::vulkan::converter {

// ============================================================================
// Extension Name Mapping
// ============================================================================

const char* instanceExtensionNameToGfx(const char* internalName)
{
    if (std::strcmp(internalName, core::extensions::SURFACE) == 0) {
        return GFX_INSTANCE_EXTENSION_SURFACE;
    }
    if (std::strcmp(internalName, core::extensions::DEBUG) == 0) {
        return GFX_INSTANCE_EXTENSION_DEBUG;
    }
    // Unknown extension - return as-is
    return internalName;
}

const char* deviceExtensionNameToGfx(const char* internalName)
{
    if (std::strcmp(internalName, core::extensions::SWAPCHAIN) == 0) {
        return GFX_DEVICE_EXTENSION_SWAPCHAIN;
    }
    if (std::strcmp(internalName, core::extensions::TIMELINE_SEMAPHORE) == 0) {
        return GFX_DEVICE_EXTENSION_TIMELINE_SEMAPHORE;
    }
    if (std::strcmp(internalName, core::extensions::MULTIVIEW) == 0) {
        return GFX_DEVICE_EXTENSION_MULTIVIEW;
    }
    if (std::strcmp(internalName, core::extensions::ANISOTROPIC_FILTERING) == 0) {
        return GFX_DEVICE_EXTENSION_ANISOTROPIC_FILTERING;
    }
    // Unknown extension - return as-is
    return internalName;
}

// ============================================================================
// Device Limits Conversion
// ============================================================================

GfxDeviceLimits vkPropertiesToGfxDeviceLimits(const VkPhysicalDeviceProperties& properties)
{
    GfxDeviceLimits limits{};
    limits.minUniformBufferOffsetAlignment = static_cast<uint32_t>(properties.limits.minUniformBufferOffsetAlignment);
    limits.minStorageBufferOffsetAlignment = static_cast<uint32_t>(properties.limits.minStorageBufferOffsetAlignment);
    limits.maxUniformBufferBindingSize = properties.limits.maxUniformBufferRange;
    limits.maxStorageBufferBindingSize = properties.limits.maxStorageBufferRange;
    limits.maxBufferSize = UINT64_MAX; // Vulkan doesn't have a single max, use practical limit
    limits.maxTextureDimension1D = properties.limits.maxImageDimension1D;
    limits.maxTextureDimension2D = properties.limits.maxImageDimension2D;
    limits.maxTextureDimension3D = properties.limits.maxImageDimension3D;
    limits.maxTextureArrayLayers = properties.limits.maxImageArrayLayers;
    return limits;
}

// ============================================================================
// Queue Family Conversion
// ============================================================================

GfxQueueFlags vkQueueFlagsToGfx(VkQueueFlags vkFlags)
{
    GfxQueueFlags flags = GFX_QUEUE_FLAG_NONE;

    if (vkFlags & VK_QUEUE_GRAPHICS_BIT) {
        flags = static_cast<GfxQueueFlags>(flags | GFX_QUEUE_FLAG_GRAPHICS);
    }
    if (vkFlags & VK_QUEUE_COMPUTE_BIT) {
        flags = static_cast<GfxQueueFlags>(flags | GFX_QUEUE_FLAG_COMPUTE);
    }
    if (vkFlags & VK_QUEUE_TRANSFER_BIT) {
        flags = static_cast<GfxQueueFlags>(flags | GFX_QUEUE_FLAG_TRANSFER);
    }
    if (vkFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
        flags = static_cast<GfxQueueFlags>(flags | GFX_QUEUE_FLAG_SPARSE_BINDING);
    }

    return flags;
}

GfxQueueFamilyProperties vkQueueFamilyPropertiesToGfx(const VkQueueFamilyProperties& vkProps)
{
    GfxQueueFamilyProperties gfxProps = {};
    gfxProps.flags = vkQueueFlagsToGfx(vkProps.queueFlags);
    gfxProps.queueCount = vkProps.queueCount;
    return gfxProps;
}

// ============================================================================
// Adapter Type Conversion
// ============================================================================

GfxAdapterType vkDeviceTypeToGfxAdapterType(VkPhysicalDeviceType deviceType)
{
    switch (deviceType) {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        return GFX_ADAPTER_TYPE_DISCRETE_GPU;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return GFX_ADAPTER_TYPE_INTEGRATED_GPU;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        return GFX_ADAPTER_TYPE_CPU;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
    default:
        return GFX_ADAPTER_TYPE_UNKNOWN;
    }
}

// ============================================================================
// Adapter Info Conversion
// ============================================================================

GfxAdapterInfo vkPropertiesToGfxAdapterInfo(const VkPhysicalDeviceProperties& properties)
{
    GfxAdapterInfo info{};
    info.name = properties.deviceName;
    info.driverDescription = nullptr; // Vulkan doesn't provide driver description in properties
    info.vendorID = properties.vendorID;
    info.deviceID = properties.deviceID;
    info.backend = GFX_BACKEND_VULKAN;
    info.adapterType = vkDeviceTypeToGfxAdapterType(properties.deviceType);
    return info;
}

// ============================================================================
// Type Conversion Functions
// ============================================================================

gfx::backend::vulkan::core::SemaphoreType gfxSemaphoreTypeToVulkanSemaphoreType(GfxSemaphoreType type)
{
    switch (type) {
    case GFX_SEMAPHORE_TYPE_BINARY:
        return gfx::backend::vulkan::core::SemaphoreType::Binary;
    case GFX_SEMAPHORE_TYPE_TIMELINE:
        return gfx::backend::vulkan::core::SemaphoreType::Timeline;
    default:
        return gfx::backend::vulkan::core::SemaphoreType::Binary;
    }
}

GfxSemaphoreType vulkanSemaphoreTypeToGfxSemaphoreType(gfx::backend::vulkan::core::SemaphoreType type)
{
    switch (type) {
    case gfx::backend::vulkan::core::SemaphoreType::Binary:
        return GFX_SEMAPHORE_TYPE_BINARY;
    case gfx::backend::vulkan::core::SemaphoreType::Timeline:
        return GFX_SEMAPHORE_TYPE_TIMELINE;
    default:
        return GFX_SEMAPHORE_TYPE_BINARY;
    }
}

gfx::backend::vulkan::core::ShaderSourceType gfxShaderSourceTypeToVulkanShaderSourceType(GfxShaderSourceType type)
{
    switch (type) {
    case GFX_SHADER_SOURCE_WGSL:
        return gfx::backend::vulkan::core::ShaderSourceType::WGSL;
    case GFX_SHADER_SOURCE_SPIRV:
        return gfx::backend::vulkan::core::ShaderSourceType::SPIRV;
    default:
        return gfx::backend::vulkan::core::ShaderSourceType::SPIRV;
    }
}

// ============================================================================
// Format Conversion Functions
// ============================================================================

VkFormat gfxFormatToVkFormat(GfxTextureFormat format)
{
    switch (format) {
    case GFX_TEXTURE_FORMAT_UNDEFINED:
        return VK_FORMAT_UNDEFINED;
    case GFX_TEXTURE_FORMAT_R8_UNORM:
        return VK_FORMAT_R8_UNORM;
    case GFX_TEXTURE_FORMAT_R8G8_UNORM:
        return VK_FORMAT_R8G8_UNORM;
    case GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM_SRGB:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM:
        return VK_FORMAT_B8G8R8A8_UNORM;
    case GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB:
        return VK_FORMAT_B8G8R8A8_SRGB;
    case GFX_TEXTURE_FORMAT_R16_FLOAT:
        return VK_FORMAT_R16_SFLOAT;
    case GFX_TEXTURE_FORMAT_R16G16_FLOAT:
        return VK_FORMAT_R16G16_SFLOAT;
    case GFX_TEXTURE_FORMAT_R16G16B16A16_FLOAT:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case GFX_TEXTURE_FORMAT_R32_FLOAT:
        return VK_FORMAT_R32_SFLOAT;
    case GFX_TEXTURE_FORMAT_R32G32_FLOAT:
        return VK_FORMAT_R32G32_SFLOAT;
    case GFX_TEXTURE_FORMAT_R32G32B32_FLOAT:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case GFX_TEXTURE_FORMAT_R32G32B32A32_FLOAT:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case GFX_TEXTURE_FORMAT_DEPTH16_UNORM:
        return VK_FORMAT_D16_UNORM;
    case GFX_TEXTURE_FORMAT_DEPTH24_PLUS:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case GFX_TEXTURE_FORMAT_DEPTH32_FLOAT:
        return VK_FORMAT_D32_SFLOAT;
    case GFX_TEXTURE_FORMAT_STENCIL8:
        return VK_FORMAT_S8_UINT;
    case GFX_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case GFX_TEXTURE_FORMAT_DEPTH32_FLOAT_STENCIL8:
        return VK_FORMAT_D32_SFLOAT_S8_UINT;
    default:
        return VK_FORMAT_UNDEFINED;
    }
}

bool isDepthFormat(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D16_UNORM;
}

VkAttachmentLoadOp gfxLoadOpToVkLoadOp(GfxLoadOp loadOp)
{
    switch (loadOp) {
    case GFX_LOAD_OP_LOAD:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    case GFX_LOAD_OP_CLEAR:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case GFX_LOAD_OP_DONT_CARE:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    default:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
}

VkAttachmentStoreOp gfxStoreOpToVkStoreOp(GfxStoreOp storeOp)
{
    switch (storeOp) {
    case GFX_STORE_OP_STORE:
        return VK_ATTACHMENT_STORE_OP_STORE;
    case GFX_STORE_OP_DONT_CARE:
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    default:
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
}

GfxTextureFormat vkFormatToGfxFormat(VkFormat format)
{
    switch (format) {
    case VK_FORMAT_R8_UNORM:
        return GFX_TEXTURE_FORMAT_R8_UNORM;
    case VK_FORMAT_R8G8_UNORM:
        return GFX_TEXTURE_FORMAT_R8G8_UNORM;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_R8G8B8A8_SRGB:
        return GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM_SRGB;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM;
    case VK_FORMAT_B8G8R8A8_SRGB:
        return GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB;
    case VK_FORMAT_R16_SFLOAT:
        return GFX_TEXTURE_FORMAT_R16_FLOAT;
    case VK_FORMAT_R16G16_SFLOAT:
        return GFX_TEXTURE_FORMAT_R16G16_FLOAT;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return GFX_TEXTURE_FORMAT_R16G16B16A16_FLOAT;
    case VK_FORMAT_R32_SFLOAT:
        return GFX_TEXTURE_FORMAT_R32_FLOAT;
    case VK_FORMAT_R32G32_SFLOAT:
        return GFX_TEXTURE_FORMAT_R32G32_FLOAT;
    case VK_FORMAT_R32G32B32_SFLOAT:
        return GFX_TEXTURE_FORMAT_R32G32B32_FLOAT;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return GFX_TEXTURE_FORMAT_R32G32B32A32_FLOAT;
    case VK_FORMAT_D16_UNORM:
        return GFX_TEXTURE_FORMAT_DEPTH16_UNORM;
    case VK_FORMAT_S8_UINT:
        return GFX_TEXTURE_FORMAT_STENCIL8;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return GFX_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8;
    case VK_FORMAT_D32_SFLOAT:
        return GFX_TEXTURE_FORMAT_DEPTH32_FLOAT;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return GFX_TEXTURE_FORMAT_DEPTH32_FLOAT_STENCIL8;
    default:
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
}

GfxBufferUsageFlags vkBufferUsageToGfxBufferUsage(VkBufferUsageFlags vkUsage)
{
    GfxBufferUsageFlags usage = GFX_BUFFER_USAGE_NONE;
    if (vkUsage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) {
        usage |= GFX_BUFFER_USAGE_COPY_SRC;
    }
    if (vkUsage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) {
        usage |= GFX_BUFFER_USAGE_COPY_DST;
    }
    if (vkUsage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) {
        usage |= GFX_BUFFER_USAGE_INDEX;
    }
    if (vkUsage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) {
        usage |= GFX_BUFFER_USAGE_VERTEX;
    }
    if (vkUsage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
        usage |= GFX_BUFFER_USAGE_UNIFORM;
    }
    if (vkUsage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
        usage |= GFX_BUFFER_USAGE_STORAGE;
    }
    if (vkUsage & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT) {
        usage |= GFX_BUFFER_USAGE_INDIRECT;
    }
    return usage;
}

VkMemoryPropertyFlags gfxMemoryPropertyToVkMemoryProperty(GfxMemoryPropertyFlags gfxMemoryProperty)
{
    VkMemoryPropertyFlags flags = 0;
    if (gfxMemoryProperty & GFX_MEMORY_PROPERTY_DEVICE_LOCAL) {
        flags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }
    if (gfxMemoryProperty & GFX_MEMORY_PROPERTY_HOST_VISIBLE) {
        flags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }
    if (gfxMemoryProperty & GFX_MEMORY_PROPERTY_HOST_COHERENT) {
        flags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    if (gfxMemoryProperty & GFX_MEMORY_PROPERTY_HOST_CACHED) {
        flags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    }
    return flags;
}

GfxMemoryPropertyFlags vkMemoryPropertyToGfxMemoryProperty(VkMemoryPropertyFlags vkMemoryProperty)
{
    GfxMemoryPropertyFlags flags = 0;
    if (vkMemoryProperty & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
        flags |= GFX_MEMORY_PROPERTY_DEVICE_LOCAL;
    }
    if (vkMemoryProperty & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        flags |= GFX_MEMORY_PROPERTY_HOST_VISIBLE;
    }
    if (vkMemoryProperty & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
        flags |= GFX_MEMORY_PROPERTY_HOST_COHERENT;
    }
    if (vkMemoryProperty & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
        flags |= GFX_MEMORY_PROPERTY_HOST_CACHED;
    }
    return flags;
}

GfxTextureUsageFlags vkImageUsageToGfxTextureUsage(VkImageUsageFlags vkUsage)
{
    GfxTextureUsageFlags usage = GFX_TEXTURE_USAGE_NONE;
    if (vkUsage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
        usage |= GFX_TEXTURE_USAGE_COPY_SRC;
    }
    if (vkUsage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        usage |= GFX_TEXTURE_USAGE_COPY_DST;
    }
    if (vkUsage & VK_IMAGE_USAGE_SAMPLED_BIT) {
        usage |= GFX_TEXTURE_USAGE_TEXTURE_BINDING;
    }
    if (vkUsage & VK_IMAGE_USAGE_STORAGE_BIT) {
        usage |= GFX_TEXTURE_USAGE_STORAGE_BINDING;
    }
    if (vkUsage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
        usage |= GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;
    }
    return usage;
}

GfxPresentMode vkPresentModeToGfxPresentMode(VkPresentModeKHR mode)
{
    switch (mode) {
    case VK_PRESENT_MODE_IMMEDIATE_KHR:
        return GFX_PRESENT_MODE_IMMEDIATE;
    case VK_PRESENT_MODE_MAILBOX_KHR:
        return GFX_PRESENT_MODE_MAILBOX;
    case VK_PRESENT_MODE_FIFO_KHR:
        return GFX_PRESENT_MODE_FIFO;
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
        return GFX_PRESENT_MODE_FIFO_RELAXED;
    default:
        return GFX_PRESENT_MODE_FIFO;
    }
}

VkPresentModeKHR gfxPresentModeToVkPresentMode(GfxPresentMode mode)
{
    switch (mode) {
    case GFX_PRESENT_MODE_IMMEDIATE:
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    case GFX_PRESENT_MODE_MAILBOX:
        return VK_PRESENT_MODE_MAILBOX_KHR;
    case GFX_PRESENT_MODE_FIFO:
        return VK_PRESENT_MODE_FIFO_KHR;
    case GFX_PRESENT_MODE_FIFO_RELAXED:
        return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    default:
        return VK_PRESENT_MODE_FIFO_KHR;
    }
}

bool hasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_S8_UINT;
}

VkImageAspectFlags getImageAspectMask(VkFormat format)
{
    if (isDepthFormat(format)) {
        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (hasStencilComponent(format)) {
            aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        return aspectMask;
    }
    return VK_IMAGE_ASPECT_COLOR_BIT;
}

VkImageLayout gfxLayoutToVkImageLayout(GfxTextureLayout layout)
{
    switch (layout) {
    case GFX_TEXTURE_LAYOUT_UNDEFINED:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    case GFX_TEXTURE_LAYOUT_GENERAL:
        return VK_IMAGE_LAYOUT_GENERAL;
    case GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    case GFX_TEXTURE_LAYOUT_SHADER_READ_ONLY:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case GFX_TEXTURE_LAYOUT_TRANSFER_SRC:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case GFX_TEXTURE_LAYOUT_TRANSFER_DST:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case GFX_TEXTURE_LAYOUT_PRESENT_SRC:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    default:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

GfxTextureLayout vkImageLayoutToGfxLayout(VkImageLayout layout)
{
    switch (layout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        return GFX_TEXTURE_LAYOUT_UNDEFINED;
    case VK_IMAGE_LAYOUT_GENERAL:
        return GFX_TEXTURE_LAYOUT_GENERAL;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        return GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return GFX_TEXTURE_LAYOUT_SHADER_READ_ONLY;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return GFX_TEXTURE_LAYOUT_TRANSFER_SRC;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return GFX_TEXTURE_LAYOUT_TRANSFER_DST;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return GFX_TEXTURE_LAYOUT_PRESENT_SRC;
    default:
        return GFX_TEXTURE_LAYOUT_UNDEFINED;
    }
}

VkAccessFlags getVkAccessFlagsForLayout(VkImageLayout layout)
{
    switch (layout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        return 0;
    case VK_IMAGE_LAYOUT_GENERAL:
        return VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_ACCESS_SHADER_READ_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return VK_ACCESS_TRANSFER_READ_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return VK_ACCESS_TRANSFER_WRITE_BIT;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return VK_ACCESS_MEMORY_READ_BIT;
    default:
        return 0;
    }
}

VkImageType gfxTextureTypeToVkImageType(GfxTextureType type)
{
    switch (type) {
    case GFX_TEXTURE_TYPE_1D:
        return VK_IMAGE_TYPE_1D;
    case GFX_TEXTURE_TYPE_2D:
    case GFX_TEXTURE_TYPE_CUBE:
        return VK_IMAGE_TYPE_2D;
    case GFX_TEXTURE_TYPE_3D:
        return VK_IMAGE_TYPE_3D;
    default:
        return VK_IMAGE_TYPE_2D;
    }
}

GfxTextureType vkImageTypeToGfxTextureType(VkImageType type)
{
    switch (type) {
    case VK_IMAGE_TYPE_1D:
        return GFX_TEXTURE_TYPE_1D;
    case VK_IMAGE_TYPE_2D:
        return GFX_TEXTURE_TYPE_2D; // Note: Can't distinguish CUBE from this alone
    case VK_IMAGE_TYPE_3D:
        return GFX_TEXTURE_TYPE_3D;
    default:
        return GFX_TEXTURE_TYPE_2D;
    }
}

VkImageViewType gfxTextureViewTypeToVkImageViewType(GfxTextureViewType type)
{
    switch (type) {
    case GFX_TEXTURE_VIEW_TYPE_1D:
        return VK_IMAGE_VIEW_TYPE_1D;
    case GFX_TEXTURE_VIEW_TYPE_2D:
        return VK_IMAGE_VIEW_TYPE_2D;
    case GFX_TEXTURE_VIEW_TYPE_3D:
        return VK_IMAGE_VIEW_TYPE_3D;
    case GFX_TEXTURE_VIEW_TYPE_CUBE:
        return VK_IMAGE_VIEW_TYPE_CUBE;
    case GFX_TEXTURE_VIEW_TYPE_1D_ARRAY:
        return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
    case GFX_TEXTURE_VIEW_TYPE_2D_ARRAY:
        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    case GFX_TEXTURE_VIEW_TYPE_CUBE_ARRAY:
        return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    default:
        return VK_IMAGE_VIEW_TYPE_2D;
    }
}

VkSampleCountFlagBits sampleCountToVkSampleCount(GfxSampleCount sampleCount)
{
    switch (sampleCount) {
    case GFX_SAMPLE_COUNT_1:
        return VK_SAMPLE_COUNT_1_BIT;
    case GFX_SAMPLE_COUNT_2:
        return VK_SAMPLE_COUNT_2_BIT;
    case GFX_SAMPLE_COUNT_4:
        return VK_SAMPLE_COUNT_4_BIT;
    case GFX_SAMPLE_COUNT_8:
        return VK_SAMPLE_COUNT_8_BIT;
    case GFX_SAMPLE_COUNT_16:
        return VK_SAMPLE_COUNT_16_BIT;
    case GFX_SAMPLE_COUNT_32:
        return VK_SAMPLE_COUNT_32_BIT;
    case GFX_SAMPLE_COUNT_64:
        return VK_SAMPLE_COUNT_64_BIT;
    default:
        return VK_SAMPLE_COUNT_1_BIT;
    }
}

GfxSampleCount vkSampleCountToGfxSampleCount(VkSampleCountFlagBits vkSampleCount)
{
    switch (vkSampleCount) {
    case VK_SAMPLE_COUNT_1_BIT:
        return GFX_SAMPLE_COUNT_1;
    case VK_SAMPLE_COUNT_2_BIT:
        return GFX_SAMPLE_COUNT_2;
    case VK_SAMPLE_COUNT_4_BIT:
        return GFX_SAMPLE_COUNT_4;
    case VK_SAMPLE_COUNT_8_BIT:
        return GFX_SAMPLE_COUNT_8;
    case VK_SAMPLE_COUNT_16_BIT:
        return GFX_SAMPLE_COUNT_16;
    case VK_SAMPLE_COUNT_32_BIT:
        return GFX_SAMPLE_COUNT_32;
    case VK_SAMPLE_COUNT_64_BIT:
        return GFX_SAMPLE_COUNT_64;
    default:
        return GFX_SAMPLE_COUNT_1;
    }
}

GfxTextureInfo vkTextureInfoToGfxTextureInfo(const core::TextureInfo& info)
{
    GfxTextureInfo gfxInfo{};
    gfxInfo.type = vkImageTypeToGfxTextureType(info.imageType);
    gfxInfo.size = vkExtent3DToGfxExtent3D(info.size);
    gfxInfo.arrayLayerCount = info.arrayLayers;
    gfxInfo.mipLevelCount = info.mipLevelCount;
    gfxInfo.sampleCount = vkSampleCountToGfxSampleCount(info.sampleCount);
    gfxInfo.format = vkFormatToGfxFormat(info.format);
    gfxInfo.usage = vkImageUsageToGfxTextureUsage(info.usage);
    return gfxInfo;
}

GfxSurfaceInfo vkSurfaceCapabilitiesToGfxSurfaceInfo(const VkSurfaceCapabilitiesKHR& caps)
{
    GfxSurfaceInfo info{};
    info.minImageCount = caps.minImageCount;
    info.maxImageCount = caps.maxImageCount;
    info.minExtent = vkExtent2DToGfxExtent2D(caps.minImageExtent);
    info.maxExtent = vkExtent2DToGfxExtent2D(caps.maxImageExtent);
    return info;
}

GfxSwapchainInfo vkSwapchainInfoToGfxSwapchainInfo(const core::SwapchainInfo& info)
{
    GfxSwapchainInfo gfxInfo{};
    gfxInfo.extent = { info.width, info.height };
    gfxInfo.format = vkFormatToGfxFormat(info.format);
    gfxInfo.imageCount = info.imageCount;
    gfxInfo.presentMode = vkPresentModeToGfxPresentMode(info.presentMode);
    return gfxInfo;
}

GfxBufferInfo vkBufferToGfxBufferInfo(const core::BufferInfo& info)
{
    GfxBufferInfo gfxInfo{};
    gfxInfo.size = info.size;
    gfxInfo.usage = info.originalUsage; // Use preserved original GFX usage flags
    gfxInfo.memoryProperties = vkMemoryPropertyToGfxMemoryProperty(info.memoryProperties);
    return gfxInfo;
}

GfxExtent3D vkExtent3DToGfxExtent3D(const VkExtent3D& vkExtent)
{
    return { vkExtent.width, vkExtent.height, vkExtent.depth };
}

VkExtent3D gfxExtent3DToVkExtent3D(const GfxExtent3D* gfxExtent)
{
    return { gfxExtent->width, gfxExtent->height, gfxExtent->depth };
}

VkOffset3D gfxOrigin3DToVkOffset3D(const GfxOrigin3D* gfxOrigin)
{
    return { gfxOrigin->x, gfxOrigin->y, gfxOrigin->z };
}

GfxExtent2D vkExtent2DToGfxExtent2D(const VkExtent2D& vkExtent)
{
    return { vkExtent.width, vkExtent.height };
}

VkExtent2D gfxExtent2DToVkExtent2D(const GfxExtent2D& gfxExtent)
{
    return { gfxExtent.width, gfxExtent.height };
}

VkOffset2D gfxOrigin2DToVkOffset2D(const GfxOrigin2D& gfxOrigin)
{
    return { gfxOrigin.x, gfxOrigin.y };
}

GfxAccessFlags vkAccessFlagsToGfxAccessFlags(VkAccessFlags vkAccessFlags)
{
    GfxAccessFlags flags = GFX_ACCESS_NONE;

    if (vkAccessFlags & VK_ACCESS_INDIRECT_COMMAND_READ_BIT) {
        flags = static_cast<GfxAccessFlags>(flags | GFX_ACCESS_INDIRECT_COMMAND_READ);
    }
    if (vkAccessFlags & VK_ACCESS_INDEX_READ_BIT) {
        flags = static_cast<GfxAccessFlags>(flags | GFX_ACCESS_INDEX_READ);
    }
    if (vkAccessFlags & VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT) {
        flags = static_cast<GfxAccessFlags>(flags | GFX_ACCESS_VERTEX_ATTRIBUTE_READ);
    }
    if (vkAccessFlags & VK_ACCESS_UNIFORM_READ_BIT) {
        flags = static_cast<GfxAccessFlags>(flags | GFX_ACCESS_UNIFORM_READ);
    }
    if (vkAccessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT) {
        flags = static_cast<GfxAccessFlags>(flags | GFX_ACCESS_INPUT_ATTACHMENT_READ);
    }
    if (vkAccessFlags & VK_ACCESS_SHADER_READ_BIT) {
        flags = static_cast<GfxAccessFlags>(flags | GFX_ACCESS_SHADER_READ);
    }
    if (vkAccessFlags & VK_ACCESS_SHADER_WRITE_BIT) {
        flags = static_cast<GfxAccessFlags>(flags | GFX_ACCESS_SHADER_WRITE);
    }
    if (vkAccessFlags & VK_ACCESS_COLOR_ATTACHMENT_READ_BIT) {
        flags = static_cast<GfxAccessFlags>(flags | GFX_ACCESS_COLOR_ATTACHMENT_READ);
    }
    if (vkAccessFlags & VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT) {
        flags = static_cast<GfxAccessFlags>(flags | GFX_ACCESS_COLOR_ATTACHMENT_WRITE);
    }
    if (vkAccessFlags & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT) {
        flags = static_cast<GfxAccessFlags>(flags | GFX_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ);
    }
    if (vkAccessFlags & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT) {
        flags = static_cast<GfxAccessFlags>(flags | GFX_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE);
    }
    if (vkAccessFlags & VK_ACCESS_TRANSFER_READ_BIT) {
        flags = static_cast<GfxAccessFlags>(flags | GFX_ACCESS_TRANSFER_READ);
    }
    if (vkAccessFlags & VK_ACCESS_TRANSFER_WRITE_BIT) {
        flags = static_cast<GfxAccessFlags>(flags | GFX_ACCESS_TRANSFER_WRITE);
    }
    if (vkAccessFlags & VK_ACCESS_HOST_READ_BIT) {
        flags = static_cast<GfxAccessFlags>(flags | GFX_ACCESS_MEMORY_READ);
    }
    if (vkAccessFlags & VK_ACCESS_HOST_WRITE_BIT) {
        flags = static_cast<GfxAccessFlags>(flags | GFX_ACCESS_MEMORY_WRITE);
    }
    if (vkAccessFlags & VK_ACCESS_MEMORY_READ_BIT) {
        flags = static_cast<GfxAccessFlags>(flags | GFX_ACCESS_MEMORY_READ);
    }
    if (vkAccessFlags & VK_ACCESS_MEMORY_WRITE_BIT) {
        flags = static_cast<GfxAccessFlags>(flags | GFX_ACCESS_MEMORY_WRITE);
    }

    return flags;
}

VkCullModeFlags gfxCullModeToVkCullMode(GfxCullMode cullMode)
{
    switch (cullMode) {
    case GFX_CULL_MODE_NONE:
        return VK_CULL_MODE_NONE;
    case GFX_CULL_MODE_FRONT:
        return VK_CULL_MODE_FRONT_BIT;
    case GFX_CULL_MODE_BACK:
        return VK_CULL_MODE_BACK_BIT;
    case GFX_CULL_MODE_FRONT_AND_BACK:
        return VK_CULL_MODE_FRONT_BIT | VK_CULL_MODE_BACK_BIT;
    default:
        return VK_CULL_MODE_NONE;
    }
}

VkFrontFace gfxFrontFaceToVkFrontFace(GfxFrontFace frontFace)
{
    switch (frontFace) {
    case GFX_FRONT_FACE_COUNTER_CLOCKWISE:
        return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    case GFX_FRONT_FACE_CLOCKWISE:
        return VK_FRONT_FACE_CLOCKWISE;
    default:
        return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }
}

VkPolygonMode gfxPolygonModeToVkPolygonMode(GfxPolygonMode polygonMode)
{
    switch (polygonMode) {
    case GFX_POLYGON_MODE_FILL:
        return VK_POLYGON_MODE_FILL;
    case GFX_POLYGON_MODE_LINE:
        return VK_POLYGON_MODE_LINE;
    case GFX_POLYGON_MODE_POINT:
        return VK_POLYGON_MODE_POINT;
    default:
        return VK_POLYGON_MODE_FILL;
    }
}

VkPrimitiveTopology gfxPrimitiveTopologyToVkPrimitiveTopology(GfxPrimitiveTopology topology)
{
    switch (topology) {
    case GFX_PRIMITIVE_TOPOLOGY_POINT_LIST:
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case GFX_PRIMITIVE_TOPOLOGY_LINE_LIST:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case GFX_PRIMITIVE_TOPOLOGY_LINE_STRIP:
        return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    default:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

VkVertexInputRate gfxVertexStepModeToVkVertexInputRate(GfxVertexStepMode mode)
{
    switch (mode) {
    case GFX_VERTEX_STEP_MODE_VERTEX:
        return VK_VERTEX_INPUT_RATE_VERTEX;
    case GFX_VERTEX_STEP_MODE_INSTANCE:
        return VK_VERTEX_INPUT_RATE_INSTANCE;
    default:
        return VK_VERTEX_INPUT_RATE_VERTEX;
    }
}

VkSamplerAddressMode gfxAddressModeToVkAddressMode(GfxAddressMode addressMode)
{
    switch (addressMode) {
    case GFX_ADDRESS_MODE_REPEAT:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case GFX_ADDRESS_MODE_MIRROR_REPEAT:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case GFX_ADDRESS_MODE_CLAMP_TO_EDGE:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    default:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

VkFilter gfxFilterToVkFilter(GfxFilterMode filter)
{
    switch (filter) {
    case GFX_FILTER_MODE_NEAREST:
        return VK_FILTER_NEAREST;
    case GFX_FILTER_MODE_LINEAR:
        return VK_FILTER_LINEAR;
    default:
        return VK_FILTER_NEAREST;
    }
}

VkSamplerMipmapMode gfxFilterModeToVkMipMapFilterMode(GfxFilterMode filter)
{
    switch (filter) {
    case GFX_FILTER_MODE_NEAREST:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    case GFX_FILTER_MODE_LINEAR:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    default:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    }
}

VkBlendFactor gfxBlendFactorToVkBlendFactor(GfxBlendFactor factor)
{
    switch (factor) {
    case GFX_BLEND_FACTOR_ZERO:
        return VK_BLEND_FACTOR_ZERO;
    case GFX_BLEND_FACTOR_ONE:
        return VK_BLEND_FACTOR_ONE;
    case GFX_BLEND_FACTOR_SRC:
        return VK_BLEND_FACTOR_SRC_COLOR;
    case GFX_BLEND_FACTOR_ONE_MINUS_SRC:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case GFX_BLEND_FACTOR_DST:
        return VK_BLEND_FACTOR_DST_COLOR;
    case GFX_BLEND_FACTOR_ONE_MINUS_DST:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    case GFX_BLEND_FACTOR_SRC_ALPHA:
        return VK_BLEND_FACTOR_SRC_ALPHA;
    case GFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case GFX_BLEND_FACTOR_DST_ALPHA:
        return VK_BLEND_FACTOR_DST_ALPHA;
    case GFX_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    case GFX_BLEND_FACTOR_CONSTANT:
        return VK_BLEND_FACTOR_CONSTANT_COLOR;
    case GFX_BLEND_FACTOR_ONE_MINUS_CONSTANT:
        return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
    case GFX_BLEND_FACTOR_SRC_ALPHA_SATURATED:
        return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
    default:
        return VK_BLEND_FACTOR_ZERO;
    }
}

VkBlendOp gfxBlendOpToVkBlendOp(GfxBlendOperation op)
{
    switch (op) {
    case GFX_BLEND_OPERATION_ADD:
        return VK_BLEND_OP_ADD;
    case GFX_BLEND_OPERATION_SUBTRACT:
        return VK_BLEND_OP_SUBTRACT;
    case GFX_BLEND_OPERATION_REVERSE_SUBTRACT:
        return VK_BLEND_OP_REVERSE_SUBTRACT;
    case GFX_BLEND_OPERATION_MIN:
        return VK_BLEND_OP_MIN;
    case GFX_BLEND_OPERATION_MAX:
        return VK_BLEND_OP_MAX;
    default:
        return VK_BLEND_OP_ADD;
    }
}

VkCompareOp gfxCompareOpToVkCompareOp(GfxCompareFunction func)
{
    switch (func) {
    case GFX_COMPARE_FUNCTION_NEVER:
        return VK_COMPARE_OP_NEVER;
    case GFX_COMPARE_FUNCTION_LESS:
        return VK_COMPARE_OP_LESS;
    case GFX_COMPARE_FUNCTION_EQUAL:
        return VK_COMPARE_OP_EQUAL;
    case GFX_COMPARE_FUNCTION_LESS_EQUAL:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case GFX_COMPARE_FUNCTION_GREATER:
        return VK_COMPARE_OP_GREATER;
    case GFX_COMPARE_FUNCTION_NOT_EQUAL:
        return VK_COMPARE_OP_NOT_EQUAL;
    case GFX_COMPARE_FUNCTION_GREATER_EQUAL:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case GFX_COMPARE_FUNCTION_ALWAYS:
        return VK_COMPARE_OP_ALWAYS;
    default:
        return VK_COMPARE_OP_MAX_ENUM;
    }
}

VkQueryType gfxQueryTypeToVkQueryType(GfxQueryType type)
{
    switch (type) {
    case GFX_QUERY_TYPE_OCCLUSION:
        return VK_QUERY_TYPE_OCCLUSION;
    case GFX_QUERY_TYPE_TIMESTAMP:
        return VK_QUERY_TYPE_TIMESTAMP;
    default:
        return VK_QUERY_TYPE_MAX_ENUM;
    }
}

// ============================================================================
// CreateInfo Conversion Functions - GfxDescriptor to Internal CreateInfo
// ============================================================================

VkBufferUsageFlags gfxBufferUsageToVkBufferUsage(GfxBufferUsageFlags gfxUsage)
{
    VkBufferUsageFlags usage = 0;
    if (gfxUsage & GFX_BUFFER_USAGE_COPY_SRC) {
        usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    if (gfxUsage & GFX_BUFFER_USAGE_COPY_DST) {
        usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    if (gfxUsage & GFX_BUFFER_USAGE_UNIFORM) {
        usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    if (gfxUsage & GFX_BUFFER_USAGE_STORAGE) {
        usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if (gfxUsage & GFX_BUFFER_USAGE_INDEX) {
        usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    if (gfxUsage & GFX_BUFFER_USAGE_VERTEX) {
        usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (gfxUsage & GFX_BUFFER_USAGE_INDIRECT) {
        usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    }
    return usage;
}

VkImageUsageFlags gfxTextureUsageToVkImageUsage(GfxTextureUsageFlags gfxUsage, VkFormat format)
{
    VkImageUsageFlags usage = 0;
    if (gfxUsage & GFX_TEXTURE_USAGE_COPY_SRC) {
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (gfxUsage & GFX_TEXTURE_USAGE_COPY_DST) {
        usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if (gfxUsage & GFX_TEXTURE_USAGE_TEXTURE_BINDING) {
        usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (gfxUsage & GFX_TEXTURE_USAGE_STORAGE_BINDING) {
        usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (gfxUsage & GFX_TEXTURE_USAGE_RENDER_ATTACHMENT) {
        // Check if this is a depth/stencil format
        if (isDepthFormat(format)) {
            usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        } else {
            usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
    }
    return usage;
}

VkPipelineStageFlags gfxPipelineStageFlagsToVkPipelineStageFlags(GfxPipelineStageFlags gfxStage)
{
    VkPipelineStageFlags vkStage = 0;
    if (gfxStage & GFX_PIPELINE_STAGE_TOP_OF_PIPE) {
        vkStage |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }
    if (gfxStage & GFX_PIPELINE_STAGE_DRAW_INDIRECT) {
        vkStage |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
    }
    if (gfxStage & GFX_PIPELINE_STAGE_VERTEX_INPUT) {
        vkStage |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
    }
    if (gfxStage & GFX_PIPELINE_STAGE_VERTEX_SHADER) {
        vkStage |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    }
    if (gfxStage & GFX_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER) {
        vkStage |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
    }
    if (gfxStage & GFX_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER) {
        vkStage |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
    }
    if (gfxStage & GFX_PIPELINE_STAGE_GEOMETRY_SHADER) {
        vkStage |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
    }
    if (gfxStage & GFX_PIPELINE_STAGE_FRAGMENT_SHADER) {
        vkStage |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    if (gfxStage & GFX_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS) {
        vkStage |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    if (gfxStage & GFX_PIPELINE_STAGE_LATE_FRAGMENT_TESTS) {
        vkStage |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    }
    if (gfxStage & GFX_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT) {
        vkStage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    if (gfxStage & GFX_PIPELINE_STAGE_COMPUTE_SHADER) {
        vkStage |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    if (gfxStage & GFX_PIPELINE_STAGE_TRANSFER) {
        vkStage |= VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    if (gfxStage & GFX_PIPELINE_STAGE_BOTTOM_OF_PIPE) {
        vkStage |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }
    if (gfxStage & GFX_PIPELINE_STAGE_ALL_GRAPHICS) {
        vkStage |= VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    }
    if (gfxStage & GFX_PIPELINE_STAGE_ALL_COMMANDS) {
        vkStage |= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }
    return vkStage;
}

VkAccessFlags gfxAccessFlagsToVkAccessFlags(GfxAccessFlags gfxAccessFlags)
{
    VkAccessFlags vkAccessFlags = 0;
    if (gfxAccessFlags & GFX_ACCESS_INDIRECT_COMMAND_READ) {
        vkAccessFlags |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    }
    if (gfxAccessFlags & GFX_ACCESS_INDEX_READ) {
        vkAccessFlags |= VK_ACCESS_INDEX_READ_BIT;
    }
    if (gfxAccessFlags & GFX_ACCESS_VERTEX_ATTRIBUTE_READ) {
        vkAccessFlags |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    }
    if (gfxAccessFlags & GFX_ACCESS_UNIFORM_READ) {
        vkAccessFlags |= VK_ACCESS_UNIFORM_READ_BIT;
    }
    if (gfxAccessFlags & GFX_ACCESS_INPUT_ATTACHMENT_READ) {
        vkAccessFlags |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    }
    if (gfxAccessFlags & GFX_ACCESS_SHADER_READ) {
        vkAccessFlags |= VK_ACCESS_SHADER_READ_BIT;
    }
    if (gfxAccessFlags & GFX_ACCESS_SHADER_WRITE) {
        vkAccessFlags |= VK_ACCESS_SHADER_WRITE_BIT;
    }
    if (gfxAccessFlags & GFX_ACCESS_COLOR_ATTACHMENT_READ) {
        vkAccessFlags |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    }
    if (gfxAccessFlags & GFX_ACCESS_COLOR_ATTACHMENT_WRITE) {
        vkAccessFlags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    if (gfxAccessFlags & GFX_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ) {
        vkAccessFlags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    }
    if (gfxAccessFlags & GFX_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE) {
        vkAccessFlags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    if (gfxAccessFlags & GFX_ACCESS_TRANSFER_READ) {
        vkAccessFlags |= VK_ACCESS_TRANSFER_READ_BIT;
    }
    if (gfxAccessFlags & GFX_ACCESS_TRANSFER_WRITE) {
        vkAccessFlags |= VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    if (gfxAccessFlags & GFX_ACCESS_MEMORY_READ) {
        vkAccessFlags |= VK_ACCESS_MEMORY_READ_BIT;
    }
    if (gfxAccessFlags & GFX_ACCESS_MEMORY_WRITE) {
        vkAccessFlags |= VK_ACCESS_MEMORY_WRITE_BIT;
    }
    return vkAccessFlags;
}

VkIndexType gfxIndexFormatToVkIndexType(GfxIndexFormat format)
{
    switch (format) {
    case GFX_INDEX_FORMAT_UINT16:
        return VK_INDEX_TYPE_UINT16;
    case GFX_INDEX_FORMAT_UINT32:
        return VK_INDEX_TYPE_UINT32;
    case GFX_INDEX_FORMAT_UNDEFINED:
    default:
        return VK_INDEX_TYPE_UINT32;
    }
}

core::Viewport gfxViewportToViewport(const GfxViewport* viewport)
{
    return { viewport->x, viewport->y, viewport->width, viewport->height, viewport->minDepth, viewport->maxDepth };
}

core::ScissorRect gfxScissorRectToScissorRect(const GfxScissorRect* scissor)
{
    auto origin = gfxOrigin2DToVkOffset2D(scissor->origin);
    auto extent = gfxExtent2DToVkExtent2D(scissor->extent);
    return { origin.x, origin.y, extent.width, extent.height };
}

core::MemoryBarrier gfxMemoryBarrierToMemoryBarrier(const GfxMemoryBarrier& barrier)
{
    return {
        gfxPipelineStageFlagsToVkPipelineStageFlags(barrier.srcStageMask),
        gfxPipelineStageFlagsToVkPipelineStageFlags(barrier.dstStageMask),
        gfxAccessFlagsToVkAccessFlags(barrier.srcAccessMask),
        gfxAccessFlagsToVkAccessFlags(barrier.dstAccessMask)
    };
}

core::BufferBarrier gfxBufferBarrierToBufferBarrier(const GfxBufferBarrier& barrier)
{
    return {
        toNative<core::Buffer>(barrier.buffer),
        gfxPipelineStageFlagsToVkPipelineStageFlags(barrier.srcStageMask),
        gfxPipelineStageFlagsToVkPipelineStageFlags(barrier.dstStageMask),
        gfxAccessFlagsToVkAccessFlags(barrier.srcAccessMask),
        gfxAccessFlagsToVkAccessFlags(barrier.dstAccessMask),
        barrier.offset,
        barrier.size
    };
}

core::TextureBarrier gfxTextureBarrierToTextureBarrier(const GfxTextureBarrier& barrier)
{
    return {
        toNative<core::Texture>(barrier.texture),
        gfxPipelineStageFlagsToVkPipelineStageFlags(barrier.srcStageMask),
        gfxPipelineStageFlagsToVkPipelineStageFlags(barrier.dstStageMask),
        gfxAccessFlagsToVkAccessFlags(barrier.srcAccessMask),
        gfxAccessFlagsToVkAccessFlags(barrier.dstAccessMask),
        gfxLayoutToVkImageLayout(barrier.oldLayout),
        gfxLayoutToVkImageLayout(barrier.newLayout),
        barrier.baseMipLevel,
        barrier.mipLevelCount,
        barrier.baseArrayLayer,
        barrier.arrayLayerCount
    };
}

core::BufferCreateInfo gfxDescriptorToBufferCreateInfo(const GfxBufferDescriptor* descriptor)
{
    core::BufferCreateInfo createInfo{};
    createInfo.size = descriptor->size;
    createInfo.usage = gfxBufferUsageToVkBufferUsage(descriptor->usage);
    createInfo.originalUsage = descriptor->usage; // Preserve original GFX usage
    createInfo.memoryProperties = gfxMemoryPropertyToVkMemoryProperty(descriptor->memoryProperties);
    return createInfo;
}

core::BufferImportInfo gfxExternalDescriptorToBufferImportInfo(const GfxBufferImportDescriptor* descriptor)
{
    core::BufferImportInfo createInfo{};
    createInfo.size = descriptor->size;
    createInfo.usage = gfxBufferUsageToVkBufferUsage(descriptor->usage);
    createInfo.originalUsage = descriptor->usage; // Preserve original GFX usage
    createInfo.memoryProperties = 0; // Imported buffers don't specify memory properties
    return createInfo;
}

core::ShaderCreateInfo gfxDescriptorToShaderCreateInfo(const GfxShaderDescriptor* descriptor)
{
    core::ShaderCreateInfo createInfo{};
    createInfo.code = descriptor->code;
    createInfo.codeSize = descriptor->codeSize;
    createInfo.entryPoint = descriptor->entryPoint;
    return createInfo;
}

core::SemaphoreCreateInfo gfxDescriptorToSemaphoreCreateInfo(const GfxSemaphoreDescriptor* descriptor)
{
    core::SemaphoreCreateInfo createInfo{};

    if (descriptor) {
        createInfo.type = gfxSemaphoreTypeToVulkanSemaphoreType(descriptor->type);
        createInfo.initialValue = descriptor->initialValue;
    } else {
        createInfo.type = core::SemaphoreType::Binary;
        createInfo.initialValue = 0;
    }

    return createInfo;
}

core::FenceCreateInfo gfxDescriptorToFenceCreateInfo(const GfxFenceDescriptor* descriptor)
{
    core::FenceCreateInfo createInfo{};

    if (descriptor) {
        createInfo.signaled = descriptor->signaled;
    } else {
        createInfo.signaled = false;
    }

    return createInfo;
}

core::TextureCreateInfo gfxDescriptorToTextureCreateInfo(const GfxTextureDescriptor* descriptor)
{
    core::TextureCreateInfo createInfo{};
    createInfo.format = gfxFormatToVkFormat(descriptor->format);
    createInfo.size.width = descriptor->size.width;
    createInfo.size.height = descriptor->size.height;
    createInfo.size.depth = descriptor->size.depth;
    createInfo.sampleCount = sampleCountToVkSampleCount(descriptor->sampleCount);
    createInfo.mipLevelCount = descriptor->mipLevelCount;
    createInfo.imageType = gfxTextureTypeToVkImageType(descriptor->type);
    createInfo.arrayLayers = descriptor->arrayLayerCount > 0 ? descriptor->arrayLayerCount : 1;
    createInfo.flags = 0;

    // Set cube map flag if needed
    if (descriptor->type == GFX_TEXTURE_TYPE_CUBE) {
        createInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    createInfo.usage = gfxTextureUsageToVkImageUsage(descriptor->usage, createInfo.format);

    return createInfo;
}

core::TextureImportInfo gfxExternalDescriptorToTextureImportInfo(const GfxTextureImportDescriptor* descriptor)
{
    core::TextureImportInfo importInfo{};
    importInfo.format = gfxFormatToVkFormat(descriptor->format);
    importInfo.size.width = descriptor->size.width;
    importInfo.size.height = descriptor->size.height;
    importInfo.size.depth = descriptor->size.depth;
    importInfo.sampleCount = sampleCountToVkSampleCount(descriptor->sampleCount);
    importInfo.mipLevelCount = descriptor->mipLevelCount;
    importInfo.imageType = gfxTextureTypeToVkImageType(descriptor->type);
    importInfo.arrayLayers = descriptor->arrayLayerCount > 0 ? descriptor->arrayLayerCount : 1;
    importInfo.flags = 0;

    // Set cube map flag if specified
    if (descriptor->type == GFX_TEXTURE_TYPE_CUBE) {
        importInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    importInfo.usage = gfxTextureUsageToVkImageUsage(descriptor->usage, importInfo.format);

    return importInfo;
}

core::TextureViewCreateInfo gfxDescriptorToTextureViewCreateInfo(const GfxTextureViewDescriptor* descriptor)
{
    core::TextureViewCreateInfo createInfo{};
    createInfo.viewType = gfxTextureViewTypeToVkImageViewType(descriptor->viewType);
    createInfo.format = gfxFormatToVkFormat(descriptor->format);
    createInfo.baseMipLevel = descriptor->baseMipLevel;
    createInfo.mipLevelCount = descriptor->mipLevelCount;
    createInfo.baseArrayLayer = descriptor->baseArrayLayer;
    createInfo.arrayLayerCount = descriptor->arrayLayerCount;
    return createInfo;
}

core::SamplerCreateInfo gfxDescriptorToSamplerCreateInfo(const GfxSamplerDescriptor* descriptor)
{
    core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = gfxAddressModeToVkAddressMode(descriptor->addressModeU);
    createInfo.addressModeV = gfxAddressModeToVkAddressMode(descriptor->addressModeV);
    createInfo.addressModeW = gfxAddressModeToVkAddressMode(descriptor->addressModeW);
    createInfo.magFilter = gfxFilterToVkFilter(descriptor->magFilter);
    createInfo.minFilter = gfxFilterToVkFilter(descriptor->minFilter);
    createInfo.mipmapMode = gfxFilterModeToVkMipMapFilterMode(descriptor->mipmapFilter);
    createInfo.lodMinClamp = descriptor->lodMinClamp;
    createInfo.lodMaxClamp = descriptor->lodMaxClamp;
    createInfo.maxAnisotropy = descriptor->maxAnisotropy;
    createInfo.compareOp = gfxCompareOpToVkCompareOp(descriptor->compare);
    return createInfo;
}

core::InstanceCreateInfo gfxDescriptorToInstanceCreateInfo(const GfxInstanceDescriptor* descriptor)
{
    core::InstanceCreateInfo createInfo{};

    if (descriptor) {
        createInfo.applicationName = descriptor->applicationName ? descriptor->applicationName : "Gfx Application";
        createInfo.applicationVersion = descriptor->applicationVersion;

        // Convert enabled extensions from const char** to std::vector<std::string>
        if (descriptor->enabledExtensions && descriptor->enabledExtensionCount > 0) {
            createInfo.enabledExtensions.reserve(descriptor->enabledExtensionCount);
            for (uint32_t i = 0; i < descriptor->enabledExtensionCount; ++i) {
                createInfo.enabledExtensions.push_back(descriptor->enabledExtensions[i]);
            }
        }
    } else {
        createInfo.applicationName = "Gfx Application";
        createInfo.applicationVersion = 1;
    }

    return createInfo;
}
core::AdapterCreateInfo gfxDescriptorToAdapterCreateInfo(const GfxAdapterDescriptor* descriptor)
{
    core::AdapterCreateInfo createInfo{};

    if (descriptor) {
        // Handle adapter index if specified
        if (descriptor->adapterIndex != UINT32_MAX) {
            createInfo.adapterIndex = descriptor->adapterIndex;
        } else {
            createInfo.adapterIndex = UINT32_MAX;
        }

        // Handle preference-based selection
        if (descriptor->preference == GFX_ADAPTER_PREFERENCE_SOFTWARE) {
            createInfo.devicePreference = core::DeviceTypePreference::SoftwareRenderer;
        } else if (descriptor->preference == GFX_ADAPTER_PREFERENCE_LOW_POWER) {
            createInfo.devicePreference = core::DeviceTypePreference::LowPower;
        } else {
            createInfo.devicePreference = core::DeviceTypePreference::HighPerformance;
        }
    } else {
        createInfo.adapterIndex = UINT32_MAX;
        createInfo.devicePreference = core::DeviceTypePreference::HighPerformance;
    }

    return createInfo;
}
core::DeviceCreateInfo gfxDescriptorToDeviceCreateInfo(const GfxDeviceDescriptor* descriptor)
{
    core::DeviceCreateInfo createInfo{};

    if (descriptor) {
        // Convert enabled extensions from const char** to std::vector<std::string>
        if (descriptor->enabledExtensions && descriptor->enabledExtensionCount > 0) {
            createInfo.enabledExtensions.reserve(descriptor->enabledExtensionCount);
            for (uint32_t i = 0; i < descriptor->enabledExtensionCount; ++i) {
                createInfo.enabledExtensions.push_back(descriptor->enabledExtensions[i]);
            }
        }
    }

    return createInfo;
}

core::PlatformWindowHandle gfxWindowHandleToPlatformWindowHandle(const GfxPlatformWindowHandle& gfxHandle)
{
    core::PlatformWindowHandle handle{};

    // Convert GfxPlatformWindowHandle to Vulkan-native platform handles
    switch (gfxHandle.windowingSystem) {
    case GFX_WINDOWING_SYSTEM_XCB:
        handle.platform = core::PlatformWindowHandle::Platform::Xcb;
        handle.handle.xcb.connection = gfxHandle.xcb.connection;
        handle.handle.xcb.window = gfxHandle.xcb.window;
        break;
    case GFX_WINDOWING_SYSTEM_XLIB:
        handle.platform = core::PlatformWindowHandle::Platform::Xlib;
        handle.handle.xlib.display = gfxHandle.xlib.display;
        handle.handle.xlib.window = gfxHandle.xlib.window;
        break;
    case GFX_WINDOWING_SYSTEM_WAYLAND:
        handle.platform = core::PlatformWindowHandle::Platform::Wayland;
        handle.handle.wayland.display = gfxHandle.wayland.display;
        handle.handle.wayland.surface = gfxHandle.wayland.surface;
        break;
    case GFX_WINDOWING_SYSTEM_WIN32:
        handle.platform = core::PlatformWindowHandle::Platform::Win32;
        handle.handle.win32.hinstance = gfxHandle.win32.hinstance;
        handle.handle.win32.hwnd = gfxHandle.win32.hwnd;
        break;
    case GFX_WINDOWING_SYSTEM_METAL:
        handle.platform = core::PlatformWindowHandle::Platform::Metal;
        handle.handle.metal.layer = gfxHandle.metal.layer;
        break;
    case GFX_WINDOWING_SYSTEM_EMSCRIPTEN:
        handle.platform = core::PlatformWindowHandle::Platform::Emscripten;
        handle.handle.emscripten.canvasSelector = gfxHandle.emscripten.canvasSelector;
        break;
    case GFX_WINDOWING_SYSTEM_ANDROID:
        handle.platform = core::PlatformWindowHandle::Platform::Android;
        handle.handle.android.window = gfxHandle.android.window;
        break;
    default:
        handle.platform = core::PlatformWindowHandle::Platform::Unknown;
        break;
    }

    return handle;
}

core::SurfaceCreateInfo gfxDescriptorToSurfaceCreateInfo(const GfxSurfaceDescriptor* descriptor)
{
    core::SurfaceCreateInfo createInfo{};
    if (descriptor) {
        createInfo.windowHandle = gfxWindowHandleToPlatformWindowHandle(descriptor->windowHandle);
    }
    return createInfo;
}

core::SwapchainCreateInfo gfxDescriptorToSwapchainCreateInfo(const GfxSwapchainDescriptor* descriptor)
{
    core::SwapchainCreateInfo createInfo{};
    createInfo.width = descriptor->extent.width;
    createInfo.height = descriptor->extent.height;
    createInfo.format = gfxFormatToVkFormat(descriptor->format);
    createInfo.presentMode = gfxPresentModeToVkPresentMode(descriptor->presentMode);
    createInfo.imageCount = descriptor->imageCount;
    return createInfo;
}

core::BindGroupLayoutCreateInfo gfxDescriptorToBindGroupLayoutCreateInfo(const GfxBindGroupLayoutDescriptor* descriptor)
{
    core::BindGroupLayoutCreateInfo createInfo{};

    for (uint32_t i = 0; i < descriptor->entryCount; ++i) {
        const auto& entry = descriptor->entries[i];

        core::BindGroupLayoutEntry layoutEntry{};
        layoutEntry.binding = entry.binding;

        // Convert GfxBindingType to VkDescriptorType
        switch (entry.type) {
        case GFX_BINDING_TYPE_BUFFER:
            layoutEntry.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            break;
        case GFX_BINDING_TYPE_SAMPLER:
            layoutEntry.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            break;
        case GFX_BINDING_TYPE_TEXTURE:
            layoutEntry.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            break;
        case GFX_BINDING_TYPE_STORAGE_TEXTURE:
            layoutEntry.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            break;
        default:
            // Unknown type - leave as Undefined
            break;
        }

        // Convert GfxShaderStage to VkShaderStageFlags
        layoutEntry.stageFlags = 0;
        if (entry.visibility & GFX_SHADER_STAGE_VERTEX) {
            layoutEntry.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
        }
        if (entry.visibility & GFX_SHADER_STAGE_FRAGMENT) {
            layoutEntry.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        if (entry.visibility & GFX_SHADER_STAGE_COMPUTE) {
            layoutEntry.stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
        }

        createInfo.entries.push_back(layoutEntry);
    }

    return createInfo;
}

// ============================================================================
// Entity-dependent CreateInfo Conversion Functions
// ============================================================================

core::BindGroupCreateInfo gfxDescriptorToBindGroupCreateInfo(const GfxBindGroupDescriptor* descriptor)
{
    core::BindGroupCreateInfo createInfo{};
    auto* layout = converter::toNative<core::BindGroupLayout>(descriptor->layout);
    createInfo.layout = layout->handle();

    for (uint32_t i = 0; i < descriptor->entryCount; ++i) {
        const auto& entry = descriptor->entries[i];

        core::BindGroupEntry bindEntry{};
        bindEntry.binding = entry.binding;

        if (entry.type == GFX_BIND_GROUP_ENTRY_TYPE_BUFFER) {
            auto* buffer = converter::toNative<core::Buffer>(entry.resource.buffer.buffer);
            bindEntry.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            bindEntry.buffer = buffer->handle();
            bindEntry.bufferOffset = entry.resource.buffer.offset;
            bindEntry.bufferSize = entry.resource.buffer.size;
        } else if (entry.type == GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER) {
            auto* sampler = converter::toNative<core::Sampler>(entry.resource.sampler);
            bindEntry.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            bindEntry.sampler = sampler->handle();
        } else if (entry.type == GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW) {
            auto* textureView = converter::toNative<core::TextureView>(entry.resource.textureView);
            bindEntry.descriptorType = layout->getBindingType(entry.binding);
            bindEntry.imageView = textureView->handle();

            // Set image layout based on descriptor type
            if (bindEntry.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
                bindEntry.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            } else {
                bindEntry.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
        }

        createInfo.entries.push_back(bindEntry);
    }

    return createInfo;
}

core::RenderPipelineCreateInfo gfxDescriptorToRenderPipelineCreateInfo(const GfxRenderPipelineDescriptor* descriptor)
{
    core::RenderPipelineCreateInfo createInfo{};

    // Render pass (if provided)
    if (descriptor->renderPass) {
        auto* renderPass = converter::toNative<core::RenderPass>(descriptor->renderPass);
        createInfo.renderPass = renderPass->handle();
    }

    // Bind group layouts
    for (uint32_t i = 0; i < descriptor->bindGroupLayoutCount; ++i) {
        auto* layout = converter::toNative<core::BindGroupLayout>(descriptor->bindGroupLayouts[i]);
        createInfo.bindGroupLayouts.push_back(layout->handle());
    }

    // Vertex state
    auto* vertShader = converter::toNative<core::Shader>(descriptor->vertex->module);
    createInfo.vertex.module = vertShader->handle();
    createInfo.vertex.entryPoint = vertShader->entryPoint();

    for (uint32_t i = 0; i < descriptor->vertex->bufferCount; ++i) {
        const auto& bufferLayout = descriptor->vertex->buffers[i];

        core::VertexBufferLayout vkBufferLayout{};
        vkBufferLayout.arrayStride = bufferLayout.arrayStride;
        vkBufferLayout.inputRate = gfxVertexStepModeToVkVertexInputRate(bufferLayout.stepMode);

        for (uint32_t j = 0; j < bufferLayout.attributeCount; ++j) {
            const auto& attr = bufferLayout.attributes[j];

            VkVertexInputAttributeDescription attribute{};
            attribute.binding = i;
            attribute.location = attr.shaderLocation;
            attribute.offset = static_cast<uint32_t>(attr.offset);
            attribute.format = gfxFormatToVkFormat(attr.format);

            vkBufferLayout.attributes.push_back(attribute);
        }

        createInfo.vertex.buffers.push_back(vkBufferLayout);
    }

    // Fragment state
    if (descriptor->fragment) {
        auto* fragShader = converter::toNative<core::Shader>(descriptor->fragment->module);
        createInfo.fragment.module = fragShader->handle();
        createInfo.fragment.entryPoint = fragShader->entryPoint();

        for (uint32_t i = 0; i < descriptor->fragment->targetCount; ++i) {
            const auto& target = descriptor->fragment->targets[i];

            core::ColorTargetState vkTarget{};
            vkTarget.format = gfxFormatToVkFormat(target.format);

            // Convert GfxColorWriteMask to VkColorComponentFlags
            vkTarget.writeMask = 0;
            if (target.writeMask & 0x1) {
                vkTarget.writeMask |= VK_COLOR_COMPONENT_R_BIT;
            }
            if (target.writeMask & 0x2) {
                vkTarget.writeMask |= VK_COLOR_COMPONENT_G_BIT;
            }
            if (target.writeMask & 0x4) {
                vkTarget.writeMask |= VK_COLOR_COMPONENT_B_BIT;
            }
            if (target.writeMask & 0x8) {
                vkTarget.writeMask |= VK_COLOR_COMPONENT_A_BIT;
            }

            VkPipelineColorBlendAttachmentState blendState{};
            blendState.colorWriteMask = vkTarget.writeMask;

            if (target.blend) {
                blendState.blendEnable = VK_TRUE;
                blendState.srcColorBlendFactor = gfxBlendFactorToVkBlendFactor(target.blend->color.srcFactor);
                blendState.dstColorBlendFactor = gfxBlendFactorToVkBlendFactor(target.blend->color.dstFactor);
                blendState.colorBlendOp = gfxBlendOpToVkBlendOp(target.blend->color.operation);
                blendState.srcAlphaBlendFactor = gfxBlendFactorToVkBlendFactor(target.blend->alpha.srcFactor);
                blendState.dstAlphaBlendFactor = gfxBlendFactorToVkBlendFactor(target.blend->alpha.dstFactor);
                blendState.alphaBlendOp = gfxBlendOpToVkBlendOp(target.blend->alpha.operation);
            } else {
                blendState.blendEnable = VK_FALSE;
            }

            vkTarget.blendState = blendState;
            createInfo.fragment.targets.push_back(vkTarget);
        }
    }

    // Primitive state
    createInfo.primitive.topology = gfxPrimitiveTopologyToVkPrimitiveTopology(descriptor->primitive->topology);
    createInfo.primitive.polygonMode = gfxPolygonModeToVkPolygonMode(descriptor->primitive->polygonMode);
    createInfo.primitive.cullMode = gfxCullModeToVkCullMode(descriptor->primitive->cullMode);
    createInfo.primitive.frontFace = gfxFrontFaceToVkFrontFace(descriptor->primitive->frontFace);

    // Depth stencil state
    if (descriptor->depthStencil) {
        core::DepthStencilState depthStencil{};
        depthStencil.format = gfxFormatToVkFormat(descriptor->depthStencil->format);
        depthStencil.depthWriteEnabled = descriptor->depthStencil->depthWriteEnabled;
        depthStencil.depthCompareOp = gfxCompareOpToVkCompareOp(descriptor->depthStencil->depthCompare);
        createInfo.depthStencil = depthStencil;
    }

    // Sample count
    createInfo.sampleCount = sampleCountToVkSampleCount(descriptor->sampleCount);

    return createInfo;
}

core::ComputePipelineCreateInfo gfxDescriptorToComputePipelineCreateInfo(const GfxComputePipelineDescriptor* descriptor)
{
    core::ComputePipelineCreateInfo createInfo{};

    // Bind group layouts
    for (uint32_t i = 0; i < descriptor->bindGroupLayoutCount; ++i) {
        auto* layout = converter::toNative<core::BindGroupLayout>(descriptor->bindGroupLayouts[i]);
        createInfo.bindGroupLayouts.push_back(layout->handle());
    }

    // Compute shader
    auto* computeShader = converter::toNative<core::Shader>(descriptor->compute);
    createInfo.module = computeShader->handle();
    createInfo.entryPoint = computeShader->entryPoint();

    return createInfo;
}

core::SubmitInfo gfxDescriptorToSubmitInfo(const GfxSubmitDescriptor* descriptor)
{
    core::SubmitInfo submitInfo{};
    // Note: Array pointer conversions use reinterpret_cast as toNative<> is for individual objects
    submitInfo.commandEncoders = reinterpret_cast<core::CommandEncoder**>(descriptor->commandEncoders);
    submitInfo.commandEncoderCount = descriptor->commandEncoderCount;
    submitInfo.signalFence = converter::toNative<core::Fence>(descriptor->signalFence);
    submitInfo.waitSemaphores = reinterpret_cast<core::Semaphore**>(descriptor->waitSemaphores);
    submitInfo.waitValues = descriptor->waitValues;
    submitInfo.waitSemaphoreCount = descriptor->waitSemaphoreCount;
    submitInfo.signalSemaphores = reinterpret_cast<core::Semaphore**>(descriptor->signalSemaphores);
    submitInfo.signalValues = descriptor->signalValues;
    submitInfo.signalSemaphoreCount = descriptor->signalSemaphoreCount;
    return submitInfo;
}

core::RenderPassCreateInfo gfxRenderPassDescriptorToRenderPassCreateInfo(const GfxRenderPassDescriptor* descriptor)
{
    core::RenderPassCreateInfo createInfo{};

    // Convert color attachments
    for (uint32_t i = 0; i < descriptor->colorAttachmentCount; ++i) {
        const GfxRenderPassColorAttachment& colorAtt = descriptor->colorAttachments[i];
        const GfxRenderPassColorAttachmentTarget& target = colorAtt.target;

        core::RenderPassColorAttachment attachment{};
        attachment.target.format = gfxFormatToVkFormat(target.format);
        attachment.target.sampleCount = sampleCountToVkSampleCount(target.sampleCount);
        attachment.target.loadOp = gfxLoadOpToVkLoadOp(target.ops.loadOp);
        attachment.target.storeOp = gfxStoreOpToVkStoreOp(target.ops.storeOp);
        attachment.target.finalLayout = gfxLayoutToVkImageLayout(target.finalLayout);

        // Convert resolve target if present
        if (colorAtt.resolveTarget) {
            const GfxRenderPassColorAttachmentTarget& resolveTarget = *colorAtt.resolveTarget;

            core::RenderPassColorAttachmentTarget resolveTargetInfo{};
            resolveTargetInfo.format = gfxFormatToVkFormat(resolveTarget.format);
            resolveTargetInfo.sampleCount = sampleCountToVkSampleCount(resolveTarget.sampleCount);
            resolveTargetInfo.loadOp = gfxLoadOpToVkLoadOp(resolveTarget.ops.loadOp);
            resolveTargetInfo.storeOp = gfxStoreOpToVkStoreOp(resolveTarget.ops.storeOp);
            resolveTargetInfo.finalLayout = gfxLayoutToVkImageLayout(resolveTarget.finalLayout);

            attachment.resolveTarget = resolveTargetInfo;
        }

        createInfo.colorAttachments.push_back(attachment);
    }

    // Convert depth/stencil attachment
    if (descriptor->depthStencilAttachment) {
        const GfxRenderPassDepthStencilAttachment& depthAtt = *descriptor->depthStencilAttachment;
        const GfxRenderPassDepthStencilAttachmentTarget& target = depthAtt.target;

        core::RenderPassDepthStencilAttachment depthStencilAttachment{};
        depthStencilAttachment.target.format = gfxFormatToVkFormat(target.format);
        depthStencilAttachment.target.sampleCount = sampleCountToVkSampleCount(target.sampleCount);
        depthStencilAttachment.target.depthLoadOp = gfxLoadOpToVkLoadOp(target.depthOps.loadOp);
        depthStencilAttachment.target.depthStoreOp = gfxStoreOpToVkStoreOp(target.depthOps.storeOp);
        depthStencilAttachment.target.stencilLoadOp = gfxLoadOpToVkLoadOp(target.stencilOps.loadOp);
        depthStencilAttachment.target.stencilStoreOp = gfxStoreOpToVkStoreOp(target.stencilOps.storeOp);
        depthStencilAttachment.target.finalLayout = gfxLayoutToVkImageLayout(target.finalLayout);

        // Convert resolve target if present
        if (depthAtt.resolveTarget) {
            const GfxRenderPassDepthStencilAttachmentTarget& resolveTarget = *depthAtt.resolveTarget;

            core::RenderPassDepthStencilAttachmentTarget resolveTargetInfo{};
            resolveTargetInfo.format = gfxFormatToVkFormat(resolveTarget.format);
            resolveTargetInfo.sampleCount = sampleCountToVkSampleCount(resolveTarget.sampleCount);
            resolveTargetInfo.depthLoadOp = gfxLoadOpToVkLoadOp(resolveTarget.depthOps.loadOp);
            resolveTargetInfo.depthStoreOp = gfxStoreOpToVkStoreOp(resolveTarget.depthOps.storeOp);
            resolveTargetInfo.stencilLoadOp = gfxLoadOpToVkLoadOp(resolveTarget.stencilOps.loadOp);
            resolveTargetInfo.stencilStoreOp = gfxStoreOpToVkStoreOp(resolveTarget.stencilOps.storeOp);
            resolveTargetInfo.finalLayout = gfxLayoutToVkImageLayout(resolveTarget.finalLayout);

            depthStencilAttachment.resolveTarget = resolveTargetInfo;
        }

        createInfo.depthStencilAttachment = depthStencilAttachment;
    }

    // Handle multiview extension in pNext chain
    const GfxChainHeader* chainNode = static_cast<const GfxChainHeader*>(descriptor->pNext);
    while (chainNode) {
        if (chainNode->sType == GFX_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_DESCRIPTOR) {
            const GfxRenderPassMultiviewDescriptor* multiview = static_cast<const GfxRenderPassMultiviewDescriptor*>(static_cast<const void*>(chainNode));
            createInfo.viewMask = multiview->viewMask;
            for (uint32_t i = 0; i < multiview->correlationMaskCount; ++i) {
                createInfo.correlationMasks.push_back(multiview->correlationMasks[i]);
            }
        }
        // Move to next extension in chain
        chainNode = static_cast<const GfxChainHeader*>(chainNode->pNext);
    }

    return createInfo;
}

core::FramebufferCreateInfo gfxFramebufferDescriptorToFramebufferCreateInfo(const GfxFramebufferDescriptor* descriptor)
{
    core::FramebufferCreateInfo createInfo{};

    // Extract render pass handle
    if (descriptor->renderPass) {
        auto* renderPass = toNative<core::RenderPass>(descriptor->renderPass);
        createInfo.renderPass = renderPass->handle();
    }

    createInfo.colorAttachmentCount = descriptor->colorAttachmentCount;

    // In Vulkan, attachments are ordered: [color0, resolve0, color1, resolve1, ..., depth, depthResolve]
    // But only if resolve targets exist
    for (uint32_t i = 0; i < descriptor->colorAttachmentCount; ++i) {
        const GfxFramebufferAttachment& colorAtt = descriptor->colorAttachments[i];

        auto* view = toNative<core::TextureView>(colorAtt.view);
        createInfo.attachments.push_back(view->handle());

        // Add resolve target if provided
        if (colorAtt.resolveTarget) {
            auto* resolveView = toNative<core::TextureView>(colorAtt.resolveTarget);
            createInfo.attachments.push_back(resolveView->handle());
        }
    }

    // Convert depth/stencil attachment view
    if (descriptor->depthStencilAttachment.view) {
        auto* view = toNative<core::TextureView>(descriptor->depthStencilAttachment.view);
        createInfo.attachments.push_back(view->handle());

        // Add depth resolve target if provided
        if (descriptor->depthStencilAttachment.resolveTarget) {
            auto* resolveView = toNative<core::TextureView>(descriptor->depthStencilAttachment.resolveTarget);
            createInfo.attachments.push_back(resolveView->handle());
            createInfo.hasDepthResolve = true;
        }
    }

    createInfo.width = descriptor->extent.width;
    createInfo.height = descriptor->extent.height;

    return createInfo;
}

core::RenderPassEncoderBeginInfo gfxRenderPassBeginDescriptorToBeginInfo(const GfxRenderPassBeginDescriptor* descriptor)
{
    core::RenderPassEncoderBeginInfo beginInfo{};

    // Convert color clear values
    for (uint32_t i = 0; i < descriptor->colorClearValueCount; ++i) {
        const GfxColor& color = descriptor->colorClearValues[i];
        VkClearColorValue clearValue{};
        clearValue.float32[0] = color.r;
        clearValue.float32[1] = color.g;
        clearValue.float32[2] = color.b;
        clearValue.float32[3] = color.a;
        beginInfo.colorClearValues.push_back(clearValue);
    }

    beginInfo.depthClearValue = descriptor->depthClearValue;
    beginInfo.stencilClearValue = descriptor->stencilClearValue;

    return beginInfo;
}

core::ComputePassEncoderCreateInfo gfxComputePassBeginDescriptorToCreateInfo(const GfxComputePassBeginDescriptor* descriptor)
{
    core::ComputePassEncoderCreateInfo createInfo{};
    createInfo.label = descriptor->label;
    return createInfo;
}

core::QuerySetCreateInfo gfxDescriptorToQuerySetCreateInfo(const GfxQuerySetDescriptor* descriptor)
{
    core::QuerySetCreateInfo createInfo{};

    if (descriptor) {
        createInfo.label = descriptor->label;
        createInfo.type = gfxQueryTypeToVkQueryType(descriptor->type);
        createInfo.count = descriptor->count;
    } else {
        createInfo.type = VK_QUERY_TYPE_OCCLUSION;
        createInfo.count = 1;
    }

    return createInfo;
}

} // namespace gfx::backend::vulkan::converter
