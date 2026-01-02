#pragma once

#include <gfx/gfx.h>
#include <vulkan/vulkan.h>

namespace gfx::convertor {

// ============================================================================
// Format Conversion Functions
// ============================================================================

inline VkFormat gfxFormatToVkFormat(GfxTextureFormat format)
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
    case GFX_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case GFX_TEXTURE_FORMAT_DEPTH32_FLOAT_STENCIL8:
        return VK_FORMAT_D32_SFLOAT_S8_UINT;
    default:
        return VK_FORMAT_UNDEFINED;
    }
}

inline bool isDepthFormat(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D16_UNORM;
}

inline VkAttachmentLoadOp gfxLoadOpToVkLoadOp(GfxLoadOp loadOp)
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

inline VkAttachmentStoreOp gfxStoreOpToVkStoreOp(GfxStoreOp storeOp)
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

inline GfxTextureFormat vkFormatToGfxFormat(VkFormat format)
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

inline GfxPresentMode vkPresentModeToGfxPresentMode(VkPresentModeKHR mode)
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
        return GFX_PRESENT_MODE_FIFO; // Safe default
    }
}

inline VkPresentModeKHR gfxPresentModeToVkPresentMode(GfxPresentMode mode)
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
        return VK_PRESENT_MODE_FIFO_KHR; // Safe default
    }
}

inline bool hasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}

inline VkImageAspectFlags getImageAspectMask(VkFormat format)
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

inline VkImageLayout gfxLayoutToVkImageLayout(GfxTextureLayout layout)
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

inline VkImageType gfxTextureTypeToVkImageType(GfxTextureType type)
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

inline VkImageViewType gfxTextureViewTypeToVkImageViewType(GfxTextureViewType type)
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

inline VkSampleCountFlagBits sampleCountToVkSampleCount(GfxSampleCount sampleCount)
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

inline VkCullModeFlags gfxCullModeToVkCullMode(GfxCullMode cullMode)
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

inline VkFrontFace gfxFrontFaceToVkFrontFace(GfxFrontFace frontFace)
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

inline VkPolygonMode gfxPolygonModeToVkPolygonMode(GfxPolygonMode polygonMode)
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

inline const char* vkResultToString(VkResult result)
{
    switch (result) {
    case VK_SUCCESS:
        return "VK_SUCCESS";
    case VK_NOT_READY:
        return "VK_NOT_READY";
    case VK_TIMEOUT:
        return "VK_TIMEOUT";
    case VK_EVENT_SET:
        return "VK_EVENT_SET";
    case VK_EVENT_RESET:
        return "VK_EVENT_RESET";
    case VK_INCOMPLETE:
        return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
        return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
        return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:
        return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
        return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
        return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS:
        return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
        return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL:
        return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_SURFACE_LOST_KHR:
        return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR:
        return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR:
        return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT:
        return "VK_ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_INVALID_SHADER_NV:
        return "VK_ERROR_INVALID_SHADER_NV";
    case VK_ERROR_OUT_OF_POOL_MEMORY:
        return "VK_ERROR_OUT_OF_POOL_MEMORY";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
        return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case VK_ERROR_FRAGMENTATION:
        return "VK_ERROR_FRAGMENTATION";
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
        return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
    default:
        return "VK_UNKNOWN_ERROR";
    }
}

} // namespace gfx::convertor
