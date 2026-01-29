#ifndef GFX_VULKAN_CORE_UTILS_H
#define GFX_VULKAN_CORE_UTILS_H

#include "../../common/Common.h"

namespace gfx::backend::vulkan::core {

// ============================================================================
// Vulkan Format and Image Utilities
// ============================================================================

// Get the appropriate image aspect mask for a given format
VkImageAspectFlags getImageAspectMask(VkFormat format);

// Get the appropriate access flags for a given image layout
VkAccessFlags getVkAccessFlagsForLayout(VkImageLayout layout);

// Check if format has depth component
bool isDepthFormat(VkFormat format);

// Check if format has stencil component
bool hasStencilComponent(VkFormat format);

// Find suitable memory type index for given requirements and properties
// Returns UINT32_MAX if no suitable memory type is found
uint32_t findMemoryType(const VkPhysicalDeviceMemoryProperties& memProperties, uint32_t memoryTypeBits, VkMemoryPropertyFlags requiredProperties);

// ============================================================================
// Vulkan Error Handling
// ============================================================================

// Convert VkResult to human-readable string
const char* vkResultToString(VkResult result);

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_CORE_UTILS_H
