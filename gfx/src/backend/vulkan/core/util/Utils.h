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

// ============================================================================
// Vulkan Error Handling
// ============================================================================

// Convert VkResult to human-readable string
const char* vkResultToString(VkResult result);

// ============================================================================
// Debug Message Utilities
// ============================================================================

enum class DebugMessageSeverity;
enum class DebugMessageType;

// Convert Vulkan debug severity to internal enum
DebugMessageSeverity convertVkDebugSeverity(VkDebugUtilsMessageSeverityFlagBitsEXT vkSeverity);

// Convert Vulkan debug message type to internal enum
DebugMessageType convertVkDebugType(VkDebugUtilsMessageTypeFlagsEXT vkType);

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_CORE_UTILS_H
