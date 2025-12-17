#include "GfxApi.h"
#include "GfxBackend.h"

// Platform-specific Vulkan extensions
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#define VK_USE_PLATFORM_XCB_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#elif defined(__APPLE__)
#define VK_USE_PLATFORM_MACOS_MVK
#endif

#include <vulkan/vulkan.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#include <X11/Xlib-xcb.h>
#include <X11/Xlib.h>
#include <xcb/xcb.h>
#endif

// ============================================================================
// Internal Structures
// ============================================================================

// Deferred deletion queue for resources that need to be cleaned up after GPU finishes
#define MAX_FRAMES_IN_FLIGHT 3

typedef struct {
    VkRenderPass renderPass;
    VkFramebuffer framebuffer;
    uint64_t frameIndex;
} DeferredResourceCleanup;

typedef struct {
    DeferredResourceCleanup* items;
    uint32_t count;
    uint32_t capacity;
} DeferredDeletionQueue;

struct GfxInstance_T {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    bool validationEnabled;
};

struct GfxAdapter_T {
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    uint32_t graphicsQueueFamily;
    uint32_t presentQueueFamily;
    GfxInstance instance;
};

struct GfxDevice_T {
    VkDevice device;
    GfxAdapter adapter;
    GfxQueue queue;
    DeferredDeletionQueue deletionQueue;
    uint64_t currentFrameIndex;
};

struct GfxQueue_T {
    VkQueue queue;
    uint32_t queueFamily;
    GfxDevice device;
};

struct GfxSurface_T {
    VkSurfaceKHR surface;
    GfxPlatformWindowHandle windowHandle;
    uint32_t width;
    uint32_t height;
    GfxInstance instance;
};

struct GfxSwapchain_T {
    VkSwapchainKHR swapchain;
    VkFormat format;
    VkExtent2D extent;
    VkImage* images;
    VkImageView* imageViews;
    GfxTextureView* textureViews;
    uint32_t imageCount;
    uint32_t currentImageIndex;
    bool needsRecreation;
    GfxDevice device;
    GfxSurface surface;
    VkFence acquireFence; // Fence for vkAcquireNextImageKHR synchronization
};

struct GfxBuffer_T {
    VkBuffer buffer;
    VkDeviceMemory memory;
    uint64_t size;
    GfxBufferUsage usage;
    void* mappedData;
    GfxDevice device;
};

struct GfxTexture_T {
    VkImage image;
    VkDeviceMemory memory;
    VkFormat format;
    GfxExtent3D extent;
    uint32_t mipLevels;
    uint32_t arrayLayers;
    VkSampleCountFlagBits samples;
    GfxTextureUsage usage;
    GfxDevice device;
};

struct GfxTextureView_T {
    VkImageView imageView;
    GfxTexture texture;
    GfxTextureFormat format;
    GfxDevice device;
    uint32_t width; // Store dimensions for swapchain texture views
    uint32_t height;
};

struct GfxSampler_T {
    VkSampler sampler;
    GfxDevice device;
};

struct GfxShader_T {
    VkShaderModule shaderModule;
    char* entryPoint;
    GfxDevice device;
};

struct GfxRenderPipeline_T {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    GfxDevice device;
};

struct GfxComputePipeline_T {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    GfxDevice device;
};

struct GfxCommandEncoder_T {
    VkCommandBuffer commandBuffer;
    VkCommandPool commandPool;
    bool isRecording;
    GfxDevice device;
    // Track resources created during render passes for proper cleanup
    // Use fixed-size arrays to avoid realloc heap corruption issues
    VkRenderPass renderPasses[32];
    VkFramebuffer framebuffers[32];
    uint32_t resourceCount;
};

struct GfxRenderPassEncoder_T {
    VkCommandBuffer commandBuffer;
    VkRenderPass renderPass;
    VkFramebuffer framebuffer;
    bool isRecording;
    GfxCommandEncoder encoder;
    GfxRenderPipeline currentPipeline; // Track current pipeline for descriptor set binding
    uint32_t viewportWidth;
    uint32_t viewportHeight;
    bool renderPassStarted;
    // Store attachment info for deferred render pass begin
    VkImageView* attachmentViews;
    uint32_t attachmentCount;
    VkClearValue* clearValues;
    uint32_t clearValueCount;
};

struct GfxComputePassEncoder_T {
    VkCommandBuffer commandBuffer;
    bool isRecording;
    GfxCommandEncoder encoder;
};

struct GfxBindGroupLayout_T {
    VkDescriptorSetLayout descriptorSetLayout;
    GfxDevice device;
};

struct GfxBindGroup_T {
    VkDescriptorSet descriptorSet;
    VkDescriptorPool descriptorPool; // Store pool for proper cleanup
    GfxBindGroupLayout layout;
    GfxDevice device;
};

struct GfxFence_T {
    VkFence fence;
    GfxDevice device;
};

struct GfxSemaphore_T {
    VkSemaphore semaphore;
    GfxSemaphoreType type;
    uint64_t value; // For timeline semaphores
    GfxDevice device;
};

// ============================================================================
// Utility Functions
// ============================================================================

static VkFormat gfxTextureFormatToVkFormat(GfxTextureFormat format)
{
    switch (format) {
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

static GfxTextureFormat vkFormatToGfxTextureFormat(VkFormat format)
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
    default:
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
}

static VkBufferUsageFlags gfxBufferUsageToVkBufferUsage(GfxBufferUsage usage)
{
    VkBufferUsageFlags vkUsage = 0;
    if (usage & GFX_BUFFER_USAGE_COPY_SRC)
        vkUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (usage & GFX_BUFFER_USAGE_COPY_DST)
        vkUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (usage & GFX_BUFFER_USAGE_INDEX)
        vkUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (usage & GFX_BUFFER_USAGE_VERTEX)
        vkUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (usage & GFX_BUFFER_USAGE_UNIFORM)
        vkUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (usage & GFX_BUFFER_USAGE_STORAGE)
        vkUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (usage & GFX_BUFFER_USAGE_INDIRECT)
        vkUsage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    return vkUsage;
}

static bool isDepthFormat(VkFormat format)
{
    return format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT;
}

static VkImageUsageFlags gfxTextureUsageToVkImageUsage(GfxTextureUsage usage, VkFormat format)
{
    VkImageUsageFlags vkUsage = 0;
    if (usage & GFX_TEXTURE_USAGE_COPY_SRC)
        vkUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (usage & GFX_TEXTURE_USAGE_COPY_DST)
        vkUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (usage & GFX_TEXTURE_USAGE_TEXTURE_BINDING)
        vkUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (usage & GFX_TEXTURE_USAGE_STORAGE_BINDING)
        vkUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
    if (usage & GFX_TEXTURE_USAGE_RENDER_ATTACHMENT) {
        // Use depth/stencil attachment for depth formats, color attachment for others
        if (isDepthFormat(format)) {
            vkUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        } else {
            vkUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
    }
    return vkUsage;
}

static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return UINT32_MAX;
}

static VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    (void)pUserData;

    // Determine severity prefix
    const char* severityStr = "UNKNOWN";
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        severityStr = "ERROR";
    } else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        severityStr = "WARNING";
    } else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        severityStr = "INFO";
    } else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        severityStr = "VERBOSE";
    }

    // Determine message type
    const char* typeStr = "";
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
        typeStr = "[GENERAL] ";
    } else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
        typeStr = "[VALIDATION] ";
    } else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
        typeStr = "[PERFORMANCE] ";
    }

    fprintf(stderr, "[Vulkan %s] %s", severityStr, typeStr);

    // Include message ID if available
    if (pCallbackData->pMessageIdName) {
        fprintf(stderr, "%s: ", pCallbackData->pMessageIdName);
    }

    fprintf(stderr, "%s\n", pCallbackData->pMessage);

    // Print object information if available
    if (pCallbackData->objectCount > 0) {
        fprintf(stderr, "  Objects involved:\n");
        for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
            fprintf(stderr, "    [%u] Type: %d, Handle: 0x%llx",
                i,
                pCallbackData->pObjects[i].objectType,
                (unsigned long long)pCallbackData->pObjects[i].objectHandle);
            if (pCallbackData->pObjects[i].pObjectName) {
                fprintf(stderr, ", Name: %s", pCallbackData->pObjects[i].pObjectName);
            }
            fprintf(stderr, "\n");
        }
    }

    fflush(stderr);

    return VK_FALSE;
}

// ============================================================================
// Additional utility functions
// ============================================================================

static VkPresentModeKHR gfxPresentModeToVkPresentMode(GfxPresentMode mode)
{
    switch (mode) {
    case GFX_PRESENT_MODE_IMMEDIATE:
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    case GFX_PRESENT_MODE_FIFO:
        return VK_PRESENT_MODE_FIFO_KHR;
    case GFX_PRESENT_MODE_FIFO_RELAXED:
        return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    case GFX_PRESENT_MODE_MAILBOX:
        return VK_PRESENT_MODE_MAILBOX_KHR;
    default:
        return VK_PRESENT_MODE_FIFO_KHR;
    }
}

static VkPrimitiveTopology gfxPrimitiveTopologyToVkPrimitiveTopology(GfxPrimitiveTopology topology)
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

static VkIndexType gfxIndexFormatToVkIndexType(GfxIndexFormat format)
{
    switch (format) {
    case GFX_INDEX_FORMAT_UINT16:
        return VK_INDEX_TYPE_UINT16;
    case GFX_INDEX_FORMAT_UINT32:
        return VK_INDEX_TYPE_UINT32;
    default:
        return VK_INDEX_TYPE_UINT16;
    }
}

static VkFilter gfxFilterModeToVkFilter(GfxFilterMode mode)
{
    switch (mode) {
    case GFX_FILTER_MODE_NEAREST:
        return VK_FILTER_NEAREST;
    case GFX_FILTER_MODE_LINEAR:
        return VK_FILTER_LINEAR;
    default:
        return VK_FILTER_NEAREST;
    }
}

static VkSamplerMipmapMode gfxFilterModeToVkSamplerMipmapMode(GfxFilterMode mode)
{
    switch (mode) {
    case GFX_FILTER_MODE_NEAREST:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    case GFX_FILTER_MODE_LINEAR:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    default:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    }
}

static VkSamplerAddressMode gfxAddressModeToVkSamplerAddressMode(GfxAddressMode mode)
{
    switch (mode) {
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

static VkCompareOp gfxCompareFunctionToVkCompareOp(GfxCompareFunction func)
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
        return VK_COMPARE_OP_NEVER;
    }
}

static VkStencilOp gfxStencilOperationToVkStencilOp(GfxStencilOperation op)
{
    switch (op) {
    case GFX_STENCIL_OPERATION_KEEP:
        return VK_STENCIL_OP_KEEP;
    case GFX_STENCIL_OPERATION_ZERO:
        return VK_STENCIL_OP_ZERO;
    case GFX_STENCIL_OPERATION_REPLACE:
        return VK_STENCIL_OP_REPLACE;
    case GFX_STENCIL_OPERATION_INCREMENT_CLAMP:
        return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
    case GFX_STENCIL_OPERATION_DECREMENT_CLAMP:
        return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
    case GFX_STENCIL_OPERATION_INVERT:
        return VK_STENCIL_OP_INVERT;
    case GFX_STENCIL_OPERATION_INCREMENT_WRAP:
        return VK_STENCIL_OP_INCREMENT_AND_WRAP;
    case GFX_STENCIL_OPERATION_DECREMENT_WRAP:
        return VK_STENCIL_OP_DECREMENT_AND_WRAP;
    default:
        return VK_STENCIL_OP_KEEP;
    }
}

static uint32_t getFormatBitsPerPixel(VkFormat format)
{
    switch (format) {
    case VK_FORMAT_R8_UNORM:
        return 8;
    case VK_FORMAT_R8G8_UNORM:
        return 16;
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SRGB:
        return 32;
    case VK_FORMAT_R16_SFLOAT:
        return 16;
    case VK_FORMAT_R16G16_SFLOAT:
        return 32;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return 64;
    case VK_FORMAT_R32_SFLOAT:
        return 32;
    case VK_FORMAT_R32G32_SFLOAT:
        return 64;
    case VK_FORMAT_R32G32B32_SFLOAT:
        return 96;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return 128;
    case VK_FORMAT_D16_UNORM:
        return 16;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return 32;
    case VK_FORMAT_D32_SFLOAT:
        return 32;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return 40;
    default:
        return 32; // Default to 32 bits
    }
}

// Simple WGSL to SPIR-V compiler stub - in a real implementation, you'd use
// a proper WGSL compiler like Tint or naga
static uint32_t* compileWGSLToSPIRV(const char* wgslCode, const char* entryPoint, size_t* spirvSize)
{
    // For now, we'll create a simple passthrough shader that just outputs the vertex position
    // This is a minimal SPIR-V shader that can be used for testing
    // In a real implementation, you would compile WGSL to SPIR-V using a proper compiler

    // For now, just log a warning and return a basic shader
    fprintf(stderr, "[WARN] WGSL compilation not implemented - shader creation will fail\n");
    fprintf(stderr, "[WARN] To use Vulkan shaders, provide pre-compiled SPIR-V binary code\n");

    (void)wgslCode;
    (void)entryPoint;
    *spirvSize = 0;
    return NULL;
}

// ============================================================================
// Missing Device Functions
// ============================================================================

GfxResult vulkan_deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence)
{
    if (!device || !descriptor || !outFence)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *outFence = NULL;

    GfxFence fence = malloc(sizeof(struct GfxFence_T));
    if (!fence)
        return GFX_RESULT_ERROR_OUT_OF_MEMORY;

    memset(fence, 0, sizeof(struct GfxFence_T));
    fence->device = device;

    VkFenceCreateInfo createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.flags = descriptor->signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    VkResult result = vkCreateFence(device->device, &createInfo, NULL, &fence->fence);
    if (result != VK_SUCCESS) {
        free(fence);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    *outFence = fence;
    return GFX_RESULT_SUCCESS;
}

GfxResult vulkan_deviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore)
{
    if (!device || !descriptor || !outSemaphore)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *outSemaphore = NULL;

    GfxSemaphore semaphore = malloc(sizeof(struct GfxSemaphore_T));
    if (!semaphore)
        return GFX_RESULT_ERROR_OUT_OF_MEMORY;

    memset(semaphore, 0, sizeof(struct GfxSemaphore_T));
    semaphore->device = device;
    semaphore->type = descriptor->type;
    semaphore->value = descriptor->initialValue;

    VkSemaphoreCreateInfo createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // For timeline semaphores, we'd need VK_KHR_timeline_semaphore extension
    // This is a simplified implementation that only supports binary semaphores
    if (descriptor->type == GFX_SEMAPHORE_TYPE_TIMELINE) {
        // Timeline semaphores would require additional setup
        // For now, fall back to binary semaphore
        semaphore->type = GFX_SEMAPHORE_TYPE_BINARY;
    }

    VkResult result = vkCreateSemaphore(device->device, &createInfo, NULL, &semaphore->semaphore);
    if (result != VK_SUCCESS) {
        free(semaphore);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    *outSemaphore = semaphore;
    return GFX_RESULT_SUCCESS;
}

void vulkan_deviceWaitIdle(GfxDevice device)
{
    if (!device)
        return;
    vkDeviceWaitIdle(device->device);
}

// ============================================================================
// Enhanced Queue Operations
// ============================================================================

GfxResult vulkan_queueSubmitWithSync(GfxQueue queue, const GfxSubmitInfo* submitInfo)
{
    if (!queue || !submitInfo)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    // Convert command encoders to command buffers
    VkCommandBuffer* commandBuffers = NULL;
    if (submitInfo->commandEncoderCount > 0) {
        commandBuffers = malloc(submitInfo->commandEncoderCount * sizeof(VkCommandBuffer));
        for (uint32_t i = 0; i < submitInfo->commandEncoderCount; i++) {
            commandBuffers[i] = submitInfo->commandEncoders[i]->commandBuffer;
        }
    }

    // Convert wait semaphores
    VkSemaphore* waitSemaphores = NULL;
    VkPipelineStageFlags* waitStages = NULL;
    if (submitInfo->waitSemaphoreCount > 0) {
        waitSemaphores = malloc(submitInfo->waitSemaphoreCount * sizeof(VkSemaphore));
        waitStages = malloc(submitInfo->waitSemaphoreCount * sizeof(VkPipelineStageFlags));
        for (uint32_t i = 0; i < submitInfo->waitSemaphoreCount; i++) {
            waitSemaphores[i] = submitInfo->waitSemaphores[i]->semaphore;
            waitStages[i] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Default stage
        }
    }

    // Convert signal semaphores
    VkSemaphore* signalSemaphores = NULL;
    if (submitInfo->signalSemaphoreCount > 0) {
        signalSemaphores = malloc(submitInfo->signalSemaphoreCount * sizeof(VkSemaphore));
        for (uint32_t i = 0; i < submitInfo->signalSemaphoreCount; i++) {
            signalSemaphores[i] = submitInfo->signalSemaphores[i]->semaphore;
        }
    }

    VkSubmitInfo vkSubmitInfo = { 0 };
    vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    vkSubmitInfo.commandBufferCount = submitInfo->commandEncoderCount;
    vkSubmitInfo.pCommandBuffers = commandBuffers;
    vkSubmitInfo.waitSemaphoreCount = submitInfo->waitSemaphoreCount;
    vkSubmitInfo.pWaitSemaphores = waitSemaphores;
    vkSubmitInfo.pWaitDstStageMask = waitStages;
    vkSubmitInfo.signalSemaphoreCount = submitInfo->signalSemaphoreCount;
    vkSubmitInfo.pSignalSemaphores = signalSemaphores;

    VkFence fence = submitInfo->signalFence ? submitInfo->signalFence->fence : VK_NULL_HANDLE;

    vkQueueSubmit(queue->queue, 1, &vkSubmitInfo, fence);

    // Cleanup
    if (commandBuffers)
        free(commandBuffers);
    if (waitSemaphores)
        free(waitSemaphores);
    if (waitStages)
        free(waitStages);
    if (signalSemaphores)
        free(signalSemaphores);

    return GFX_RESULT_SUCCESS;
}

GfxResult vulkan_queueWaitIdle(GfxQueue queue)
{
    if (!queue)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    vkQueueWaitIdle(queue->queue);
    return GFX_RESULT_SUCCESS;
}

// ============================================================================
// Fence Implementation
// ============================================================================

void vulkan_fenceDestroy(GfxFence fence)
{
    if (!fence)
        return;

    vkDestroyFence(fence->device->device, fence->fence, NULL);
    free(fence);
}

GfxResult vulkan_fenceGetStatus(GfxFence fence, bool* isSignaled)
{
    if (!fence || !isSignaled)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    VkResult result = vkGetFenceStatus(fence->device->device, fence->fence);
    switch (result) {
    case VK_SUCCESS:
        *isSignaled = true;
        return GFX_RESULT_SUCCESS;
    case VK_NOT_READY:
        *isSignaled = false;
        return GFX_RESULT_SUCCESS;
    default:
        *isSignaled = false;
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult vulkan_fenceWait(GfxFence fence, uint64_t timeoutNs)
{
    if (!fence)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    VkResult result = vkWaitForFences(fence->device->device, 1, &fence->fence, VK_TRUE, timeoutNs);
    switch (result) {
    case VK_SUCCESS:
        return GFX_RESULT_SUCCESS;
    case VK_TIMEOUT:
        return GFX_RESULT_TIMEOUT;
    default:
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void vulkan_fenceReset(GfxFence fence)
{
    if (!fence)
        return;
    vkResetFences(fence->device->device, 1, &fence->fence);
}

// ============================================================================
// Semaphore Implementation
// ============================================================================

void vulkan_semaphoreDestroy(GfxSemaphore semaphore)
{
    if (!semaphore)
        return;

    vkDestroySemaphore(semaphore->device->device, semaphore->semaphore, NULL);
    free(semaphore);
}

GfxSemaphoreType vulkan_semaphoreGetType(GfxSemaphore semaphore)
{
    return semaphore ? semaphore->type : GFX_SEMAPHORE_TYPE_BINARY;
}

GfxResult vulkan_semaphoreSignal(GfxSemaphore semaphore, uint64_t value)
{
    if (!semaphore)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    // For timeline semaphores, we'd need to use vkSignalSemaphore
    // This is a simplified implementation that only supports binary semaphores
    if (semaphore->type == GFX_SEMAPHORE_TYPE_TIMELINE) {
        // Timeline semaphore signaling would require VK_KHR_timeline_semaphore
        semaphore->value = value;
        return GFX_RESULT_SUCCESS; // Stub implementation
    }

    // Binary semaphores are signaled through queue operations, not directly
    return GFX_RESULT_SUCCESS;
}

GfxResult vulkan_semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs)
{
    (void)value;
    (void)timeoutNs;
    if (!semaphore)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    // For timeline semaphores, we'd need to use vkWaitSemaphores
    // This is a simplified implementation
    if (semaphore->type == GFX_SEMAPHORE_TYPE_TIMELINE) {
        // Timeline semaphore waiting would require VK_KHR_timeline_semaphore
        return GFX_RESULT_SUCCESS; // Stub implementation
    }

    // Binary semaphores are waited on through queue operations, not directly
    return GFX_RESULT_SUCCESS;
}

uint64_t vulkan_semaphoreGetValue(GfxSemaphore semaphore)
{
    if (!semaphore)
        return 0;

    if (semaphore->type == GFX_SEMAPHORE_TYPE_TIMELINE) {
        // For timeline semaphores, we'd need to use vkGetSemaphoreCounterValue
        return semaphore->value; // Simplified implementation
    }

    // Binary semaphores don't have values
    return 0;
}

// ============================================================================
// Bind Group Layout Implementation
// ============================================================================

GfxResult vulkan_deviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout)
{
    if (!device || !descriptor || !outLayout)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *outLayout = NULL;

    GfxBindGroupLayout layout = malloc(sizeof(struct GfxBindGroupLayout_T));
    if (!layout)
        return GFX_RESULT_ERROR_OUT_OF_MEMORY;

    memset(layout, 0, sizeof(struct GfxBindGroupLayout_T));
    layout->device = device;

    // Convert GfxBindGroupLayoutEntry to VkDescriptorSetLayoutBinding
    VkDescriptorSetLayoutBinding* bindings = malloc(descriptor->entryCount * sizeof(VkDescriptorSetLayoutBinding));
    if (!bindings) {
        free(layout);
        return GFX_RESULT_ERROR_OUT_OF_MEMORY;
    }

    for (uint32_t i = 0; i < descriptor->entryCount; i++) {
        const GfxBindGroupLayoutEntry* entry = &descriptor->entries[i];
        VkDescriptorSetLayoutBinding* binding = &bindings[i];

        binding->binding = entry->binding;
        binding->descriptorCount = 1;
        binding->stageFlags = 0;

        // Convert shader stages
        if (entry->visibility & GFX_SHADER_STAGE_VERTEX) {
            binding->stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
        }
        if (entry->visibility & GFX_SHADER_STAGE_FRAGMENT) {
            binding->stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        if (entry->visibility & GFX_SHADER_STAGE_COMPUTE) {
            binding->stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
        }

        // Determine descriptor type based on explicit type field
        switch (entry->type) {
        case GFX_BINDING_TYPE_BUFFER:
            binding->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            break;
        case GFX_BINDING_TYPE_SAMPLER:
            binding->descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            break;
        case GFX_BINDING_TYPE_TEXTURE:
            binding->descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            break;
        case GFX_BINDING_TYPE_STORAGE_TEXTURE:
            binding->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            break;
        default:
            binding->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            break;
        }

        binding->pImmutableSamplers = NULL;

        printf("[DEBUG] Bind group layout entry %u: binding=%u, type=%d, stages=0x%x\n",
            i, binding->binding, binding->descriptorType, binding->stageFlags);
    }

    VkDescriptorSetLayoutCreateInfo createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = descriptor->entryCount;
    createInfo.pBindings = bindings;

    printf("[DEBUG] Creating descriptor set layout with %u bindings...\n", descriptor->entryCount);
    VkResult result = vkCreateDescriptorSetLayout(device->device, &createInfo, NULL, &layout->descriptorSetLayout);
    printf("[DEBUG] vkCreateDescriptorSetLayout returned: %d, handle: %p\n", result, (void*)(uintptr_t)layout->descriptorSetLayout);
    printf("[DEBUG] Layout struct address: %p\n", (void*)layout);
    printf("[DEBUG] descriptorSetLayout field address: %p\n", (void*)&layout->descriptorSetLayout);
    printf("[DEBUG] device field address: %p\n", (void*)&layout->device);
    printf("[DEBUG] Sizeof VkDescriptorSetLayout: %zu\n", sizeof(VkDescriptorSetLayout));
    printf("[DEBUG] Sizeof GfxDevice: %zu\n", sizeof(GfxDevice));
    free(bindings);

    if (result != VK_SUCCESS) {
        printf("[ERROR] Failed to create descriptor set layout! VkResult = %d\n", result);
        free(layout);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    // Validate the handle is not NULL
    if (layout->descriptorSetLayout == VK_NULL_HANDLE) {
        printf("[ERROR] Descriptor set layout handle is VK_NULL_HANDLE!\n");
        free(layout);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    printf("[DEBUG] Successfully created bind group layout with valid handle: %p\n",
        (void*)(uintptr_t)layout->descriptorSetLayout);
    printf("[DEBUG] About to return layout pointer: %p\n", (void*)layout);
    printf("[DEBUG] Verifying layout->descriptorSetLayout one more time: %p\n",
        (void*)(uintptr_t)layout->descriptorSetLayout);

    *outLayout = layout;
    return GFX_RESULT_SUCCESS;
}

void vulkan_bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout)
{
    if (!bindGroupLayout)
        return;

    vkDestroyDescriptorSetLayout(bindGroupLayout->device->device, bindGroupLayout->descriptorSetLayout, NULL);
    free(bindGroupLayout);
}

// ============================================================================
// Bind Group Implementation
// ============================================================================

GfxResult vulkan_deviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup)
{
    if (!device || !descriptor || !descriptor->layout || !outBindGroup)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *outBindGroup = NULL;

    GfxBindGroup bindGroup = malloc(sizeof(struct GfxBindGroup_T));
    if (!bindGroup)
        return GFX_RESULT_ERROR_OUT_OF_MEMORY;

    memset(bindGroup, 0, sizeof(struct GfxBindGroup_T));
    bindGroup->device = device;
    bindGroup->layout = descriptor->layout;

    // Count descriptor types needed
    uint32_t uniformBufferCount = 0;
    uint32_t combinedImageSamplerCount = 0;
    uint32_t samplerCount = 0;
    uint32_t sampledImageCount = 0;
    uint32_t storageBufferCount = 0;

    for (uint32_t i = 0; i < descriptor->entryCount; i++) {
        switch (descriptor->entries[i].type) {
        case GFX_BIND_GROUP_ENTRY_TYPE_BUFFER:
            uniformBufferCount++;
            break;
        case GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER:
            samplerCount++;
            break;
        case GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW:
            sampledImageCount++;
            break;
        default:
            break;
        }
    }

    // Build pool sizes array with only non-zero counts
    VkDescriptorPoolSize poolSizes[5];
    uint32_t poolSizeCount = 0;

    if (uniformBufferCount > 0) {
        poolSizes[poolSizeCount].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[poolSizeCount].descriptorCount = uniformBufferCount;
        poolSizeCount++;
    }
    if (combinedImageSamplerCount > 0) {
        poolSizes[poolSizeCount].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[poolSizeCount].descriptorCount = combinedImageSamplerCount;
        poolSizeCount++;
    }
    if (samplerCount > 0) {
        poolSizes[poolSizeCount].type = VK_DESCRIPTOR_TYPE_SAMPLER;
        poolSizes[poolSizeCount].descriptorCount = samplerCount;
        poolSizeCount++;
    }
    if (sampledImageCount > 0) {
        poolSizes[poolSizeCount].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        poolSizes[poolSizeCount].descriptorCount = sampledImageCount;
        poolSizeCount++;
    }
    if (storageBufferCount > 0) {
        poolSizes[poolSizeCount].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[poolSizeCount].descriptorCount = storageBufferCount;
        poolSizeCount++;
    }

    // Ensure we have at least one pool size
    if (poolSizeCount == 0) {
        fprintf(stderr, "Error: No descriptor types specified in bind group\n");
        free(bindGroup);
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    VkDescriptorPoolCreateInfo poolInfo = { 0 };
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = poolSizeCount;
    poolInfo.pPoolSizes = poolSizes;

    VkResult result = vkCreateDescriptorPool(device->device, &poolInfo, NULL, &bindGroup->descriptorPool);
    if (result != VK_SUCCESS) {
        free(bindGroup);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo allocInfo = { 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = bindGroup->descriptorPool;
    allocInfo.descriptorSetCount = 1;

    printf("[DEBUG] About to allocate descriptor set\n");
    printf("[DEBUG] Descriptor set layout pointer: %p\n", (void*)descriptor->layout);
    printf("[DEBUG] Descriptor set layout handle: %p\n", (void*)(uintptr_t)descriptor->layout->descriptorSetLayout);

    allocInfo.pSetLayouts = &descriptor->layout->descriptorSetLayout;

    printf("[DEBUG] Calling vkAllocateDescriptorSets...\n");
    result = vkAllocateDescriptorSets(device->device, &allocInfo, &bindGroup->descriptorSet);
    printf("[DEBUG] vkAllocateDescriptorSets returned: %d\n", result);

    if (result != VK_SUCCESS) {
        vkDestroyDescriptorPool(device->device, bindGroup->descriptorPool, NULL);
        free(bindGroup);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    // Update descriptor set with resources
    VkWriteDescriptorSet* writes = malloc(descriptor->entryCount * sizeof(VkWriteDescriptorSet));
    VkDescriptorBufferInfo* bufferInfos = malloc(descriptor->entryCount * sizeof(VkDescriptorBufferInfo));
    VkDescriptorImageInfo* imageInfos = malloc(descriptor->entryCount * sizeof(VkDescriptorImageInfo));

    for (uint32_t i = 0; i < descriptor->entryCount; i++) {
        const GfxBindGroupEntry* entry = &descriptor->entries[i];
        VkWriteDescriptorSet* write = &writes[i];

        write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write->pNext = NULL;
        write->dstSet = bindGroup->descriptorSet;
        write->dstBinding = entry->binding;
        write->dstArrayElement = 0;
        write->descriptorCount = 1;

        switch (entry->type) {
        case GFX_BIND_GROUP_ENTRY_TYPE_BUFFER:
            write->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            bufferInfos[i].buffer = entry->resource.buffer.buffer->buffer;
            bufferInfos[i].offset = entry->resource.buffer.offset;
            bufferInfos[i].range = entry->resource.buffer.size == 0 ? VK_WHOLE_SIZE : entry->resource.buffer.size;
            write->pBufferInfo = &bufferInfos[i];
            write->pImageInfo = NULL;
            write->pTexelBufferView = NULL;
            break;

        case GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER:
            write->descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            imageInfos[i].sampler = entry->resource.sampler->sampler;
            imageInfos[i].imageView = VK_NULL_HANDLE;
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            write->pBufferInfo = NULL;
            write->pImageInfo = &imageInfos[i];
            write->pTexelBufferView = NULL;
            break;

        case GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW:
            write->descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            imageInfos[i].sampler = VK_NULL_HANDLE;
            imageInfos[i].imageView = entry->resource.textureView->imageView;
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            write->pBufferInfo = NULL;
            write->pImageInfo = &imageInfos[i];
            write->pTexelBufferView = NULL;
            break;

        default:
            // Invalid entry type
            continue;
        }
    }

    vkUpdateDescriptorSets(device->device, descriptor->entryCount, writes, 0, NULL);

    free(writes);
    free(bufferInfos);
    free(imageInfos);

    *outBindGroup = bindGroup;
    return GFX_RESULT_SUCCESS;
}

void vulkan_bindGroupDestroy(GfxBindGroup bindGroup)
{
    if (!bindGroup)
        return;

    if (bindGroup->descriptorPool) {
        vkDestroyDescriptorPool(bindGroup->device->device, bindGroup->descriptorPool, NULL);
    }
    free(bindGroup);
}

// ============================================================================
// Instance Implementation
// ============================================================================

GfxResult vulkan_createInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance)
{
    if (!descriptor || !outInstance)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *outInstance = NULL;

    GfxInstance instance = malloc(sizeof(struct GfxInstance_T));
    if (!instance)
        return GFX_RESULT_ERROR_OUT_OF_MEMORY;

    memset(instance, 0, sizeof(struct GfxInstance_T));
    instance->validationEnabled = descriptor->enableValidation;

    // Application info
    VkApplicationInfo appInfo = { 0 };
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = descriptor->applicationName ? descriptor->applicationName : "GfxWrapper Application";
    appInfo.applicationVersion = descriptor->applicationVersion;
    appInfo.pEngineName = "GfxWrapper";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Instance create info
    VkInstanceCreateInfo createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Extensions - build a list that includes required extensions plus any additional ones we need
    uint32_t baseExtensionCount = descriptor->requiredExtensionCount;
    uint32_t additionalExtensions = 0;

// On Linux, always add VK_KHR_xlib_surface in addition to what GLFW provides
#ifdef __linux__
    additionalExtensions++; // For VK_KHR_xlib_surface
#endif

    // Add debug extension if validation is enabled
    if (descriptor->enableValidation) {
        additionalExtensions++;
    }

    uint32_t totalExtensionCount = baseExtensionCount + additionalExtensions;
    const char** allExtensions = malloc(totalExtensionCount * sizeof(const char*));

    // Copy base extensions
    if (descriptor->requiredExtensions) {
        memcpy(allExtensions, descriptor->requiredExtensions, baseExtensionCount * sizeof(const char*));
    }

    uint32_t currentIndex = baseExtensionCount;

#ifdef __linux__
    // Add Xlib surface extension on Linux
    allExtensions[currentIndex++] = "VK_KHR_xlib_surface";
#endif

    if (descriptor->enableValidation) {
        allExtensions[currentIndex++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }

    printf("[DEBUG] Creating Vulkan instance with %u extensions:\n", totalExtensionCount);
    for (uint32_t i = 0; i < totalExtensionCount; i++) {
        printf("[DEBUG]   - %s\n", allExtensions[i]);
    }
    fflush(stdout);

    createInfo.enabledExtensionCount = totalExtensionCount;
    createInfo.ppEnabledExtensionNames = allExtensions;

    // Validation layers
    const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };
    if (descriptor->enableValidation) {
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = validationLayers;
    }

    printf("[DEBUG] Calling vkCreateInstance...\n");
    fflush(stdout);
    VkResult result = vkCreateInstance(&createInfo, NULL, &instance->instance);
    printf("[DEBUG] vkCreateInstance returned: %d\n", result);
    fflush(stdout);

    free(allExtensions);

    if (result != VK_SUCCESS) {
        free(instance);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    // Setup debug messenger if validation is enabled
    if (descriptor->enableValidation) {
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = { 0 };
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;

        PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(instance->instance, "vkCreateDebugUtilsMessengerEXT");
        if (func) {
            func(instance->instance, &debugCreateInfo, NULL, &instance->debugMessenger);
        }
    }

    *outInstance = instance;
    return GFX_RESULT_SUCCESS;
}

void vulkan_instanceDestroy(GfxInstance instance)
{
    if (!instance)
        return;

    if (instance->validationEnabled && instance->debugMessenger) {
        PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(instance->instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func) {
            func(instance->instance, instance->debugMessenger, NULL);
        }
    }

    vkDestroyInstance(instance->instance, NULL);
    free(instance);
}

GfxResult vulkan_instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter)
{
    if (!instance || !outAdapter)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *outAdapter = NULL;

    printf("[DEBUG] instanceRequestAdapter: Starting adapter enumeration...\n");
    fflush(stdout);

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance->instance, &deviceCount, NULL);
    printf("[DEBUG] instanceRequestAdapter: Found %u physical devices\n", deviceCount);
    fflush(stdout);

    if (deviceCount == 0)
        return GFX_RESULT_ERROR_UNKNOWN;

    VkPhysicalDevice* devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance->instance, &deviceCount, devices);
    printf("[DEBUG] instanceRequestAdapter: Enumerated devices successfully\n");
    fflush(stdout);

    // Select the best device based on preferences
    VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties selectedProps;

    for (uint32_t i = 0; i < deviceCount; i++) {
        VkPhysicalDeviceProperties props;
        printf("[DEBUG] instanceRequestAdapter: Getting properties for device %u...\n", i);
        fflush(stdout);
        vkGetPhysicalDeviceProperties(devices[i], &props);
        printf("[DEBUG] instanceRequestAdapter: Device %u: %s\n", i, props.deviceName);
        fflush(stdout);

        // Prefer discrete GPU if high performance is requested
        if (descriptor && descriptor->powerPreference == GFX_POWER_PREFERENCE_HIGH_PERFORMANCE) {
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                selectedDevice = devices[i];
                selectedProps = props;
                break;
            }
        }

        // Otherwise, take the first suitable device
        if (selectedDevice == VK_NULL_HANDLE) {
            selectedDevice = devices[i];
            selectedProps = props;
        }
    }

    free(devices);
    if (selectedDevice == VK_NULL_HANDLE)
        return GFX_RESULT_ERROR_UNKNOWN;

    printf("[DEBUG] instanceRequestAdapter: Selected device, finding queue families...\n");
    fflush(stdout);

    // Find queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(selectedDevice, &queueFamilyCount, NULL);

    VkQueueFamilyProperties* queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(selectedDevice, &queueFamilyCount, queueFamilies);

    uint32_t graphicsFamily = UINT32_MAX;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsFamily = i;
            break;
        }
    }

    free(queueFamilies);
    if (graphicsFamily == UINT32_MAX)
        return GFX_RESULT_ERROR_UNKNOWN;

    printf("[DEBUG] instanceRequestAdapter: Creating adapter structure...\n");
    fflush(stdout);

    // Create adapter
    GfxAdapter adapter = malloc(sizeof(struct GfxAdapter_T));
    if (!adapter)
        return GFX_RESULT_ERROR_OUT_OF_MEMORY;

    adapter->physicalDevice = selectedDevice;
    adapter->properties = selectedProps;
    adapter->graphicsQueueFamily = graphicsFamily;
    adapter->presentQueueFamily = graphicsFamily; // Assume same for now
    adapter->instance = instance;

    vkGetPhysicalDeviceFeatures(selectedDevice, &adapter->features);

    printf("[DEBUG] instanceRequestAdapter: Adapter created successfully\n");
    fflush(stdout);

    *outAdapter = adapter;
    return GFX_RESULT_SUCCESS;
}

uint32_t vulkan_instanceEnumerateAdapters(GfxInstance instance, GfxAdapter* adapters, uint32_t maxAdapters)
{
    if (!instance)
        return 0;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance->instance, &deviceCount, NULL);
    if (deviceCount == 0)
        return 0;

    VkPhysicalDevice* devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance->instance, &deviceCount, devices);

    uint32_t adapterCount = deviceCount < maxAdapters ? deviceCount : maxAdapters;

    if (adapters) {
        for (uint32_t i = 0; i < adapterCount; i++) {
            adapters[i] = malloc(sizeof(struct GfxAdapter_T));
            if (adapters[i]) {
                adapters[i]->physicalDevice = devices[i];
                vkGetPhysicalDeviceProperties(devices[i], &adapters[i]->properties);
                vkGetPhysicalDeviceFeatures(devices[i], &adapters[i]->features);
                adapters[i]->instance = instance;

                // Find graphics queue family
                uint32_t queueFamilyCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, NULL);

                VkQueueFamilyProperties* queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
                vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queueFamilyCount, queueFamilies);

                adapters[i]->graphicsQueueFamily = UINT32_MAX;
                for (uint32_t j = 0; j < queueFamilyCount; j++) {
                    if (queueFamilies[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                        adapters[i]->graphicsQueueFamily = j;
                        break;
                    }
                }
                adapters[i]->presentQueueFamily = adapters[i]->graphicsQueueFamily;

                free(queueFamilies);
            }
        }
    }

    free(devices);
    return adapterCount;
}

// ============================================================================
// Adapter Implementation
// ============================================================================

void vulkan_adapterDestroy(GfxAdapter adapter)
{
    if (adapter) {
        free(adapter);
    }
}

GfxResult vulkan_adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice)
{
    (void)descriptor;
    if (!adapter || !outDevice)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *outDevice = NULL;

    GfxDevice device = malloc(sizeof(struct GfxDevice_T));
    if (!device)
        return GFX_RESULT_ERROR_OUT_OF_MEMORY;

    memset(device, 0, sizeof(struct GfxDevice_T));
    device->adapter = adapter;

    // ...existing code for queue creation...

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = { 0 };
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = adapter->graphicsQueueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures = { 0 };

    // Check for swapchain extension support
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(adapter->physicalDevice, NULL, &extensionCount, NULL);
    VkExtensionProperties* availableExtensions = malloc(extensionCount * sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(adapter->physicalDevice, NULL, &extensionCount, availableExtensions);

    bool swapchainSupported = false;
    for (uint32_t i = 0; i < extensionCount; i++) {
        if (strcmp(availableExtensions[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            swapchainSupported = true;
            break;
        }
    }
    free(availableExtensions);

    const char* deviceExtensions[16];
    uint32_t enabledExtensionCount = 0;

    if (swapchainSupported) {
        deviceExtensions[enabledExtensionCount++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    }

    VkDeviceCreateInfo createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = enabledExtensionCount;
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    VkResult result = vkCreateDevice(adapter->physicalDevice, &createInfo, NULL, &device->device);
    if (result != VK_SUCCESS) {
        free(device);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    // Create queue wrapper
    device->queue = malloc(sizeof(struct GfxQueue_T));
    if (device->queue) {
        vkGetDeviceQueue(device->device, adapter->graphicsQueueFamily, 0, &device->queue->queue);
        device->queue->queueFamily = adapter->graphicsQueueFamily;
        device->queue->device = device;
    }

    *outDevice = device;
    return GFX_RESULT_SUCCESS;
}

const char* vulkan_adapterGetName(GfxAdapter adapter)
{
    return adapter ? adapter->properties.deviceName : "Unknown";
}

GfxBackend vulkan_adapterGetBackend(GfxAdapter adapter)
{
    (void)adapter;
    return GFX_BACKEND_VULKAN;
}

// ============================================================================
// Device Implementation
// ============================================================================

void vulkan_deviceDestroy(GfxDevice device)
{
    if (!device)
        return;

    if (device->queue) {
        free(device->queue);
    }

    vkDestroyDevice(device->device, NULL);
    free(device);
}

GfxQueue vulkan_deviceGetQueue(GfxDevice device)
{
    return device ? device->queue : NULL;
}

GfxResult vulkan_deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface)
{
    if (!device || !descriptor || !outSurface)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *outSurface = NULL;

    GfxSurface surface = malloc(sizeof(struct GfxSurface_T));
    if (!surface)
        return GFX_RESULT_ERROR_OUT_OF_MEMORY;

    memset(surface, 0, sizeof(struct GfxSurface_T));
    surface->windowHandle = descriptor->windowHandle;
    surface->width = descriptor->width;
    surface->height = descriptor->height;
    surface->instance = device->adapter->instance;

    // Create platform-specific surface
    VkResult result = VK_ERROR_INITIALIZATION_FAILED;

#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = (HWND)descriptor->windowHandle.hwnd;
    createInfo.hinstance = (HINSTANCE)descriptor->windowHandle.hinstance;

    PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(surface->instance->instance, "vkCreateWin32SurfaceKHR");
    if (vkCreateWin32SurfaceKHR) {
        result = vkCreateWin32SurfaceKHR(surface->instance->instance, &createInfo, NULL, &surface->surface);
    }

#elif defined(__linux__)
    if (!descriptor->windowHandle.isWayland) {
        // Try XCB first if xcb_connection is provided
        if (descriptor->windowHandle.xcb_connection && descriptor->windowHandle.xcb_window) {
            printf("[DEBUG] Creating XCB surface...\n");
            printf("[DEBUG] XCB Connection: %p\n", descriptor->windowHandle.xcb_connection);
            printf("[DEBUG] XCB Window: %u\n", descriptor->windowHandle.xcb_window);
            fflush(stdout);

            // Check if XCB connection has an error
            xcb_connection_t* conn = (xcb_connection_t*)descriptor->windowHandle.xcb_connection;
            int conn_error = xcb_connection_has_error(conn);
            printf("[DEBUG] XCB connection error status: %d\n", conn_error);
            fflush(stdout);

            if (conn_error) {
                printf("[DEBUG] ERROR: XCB connection has error! Falling back to Xlib.\n");
                fflush(stdout);
                result = VK_ERROR_INITIALIZATION_FAILED;
            } else {
                VkXcbSurfaceCreateInfoKHR createInfo = { 0 };
                createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
                createInfo.connection = conn;
                createInfo.window = (xcb_window_t)descriptor->windowHandle.xcb_window;

                printf("[DEBUG] Getting vkCreateXcbSurfaceKHR function pointer...\n");
                fflush(stdout);
                PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR)vkGetInstanceProcAddr(surface->instance->instance, "vkCreateXcbSurfaceKHR");

                if (vkCreateXcbSurfaceKHR) {
                    printf("[DEBUG] Calling vkCreateXcbSurfaceKHR...\n");
                    fflush(stdout);
                    result = vkCreateXcbSurfaceKHR(surface->instance->instance, &createInfo, NULL, &surface->surface);
                    printf("[DEBUG] vkCreateXcbSurfaceKHR returned: %d\n", result);
                    fflush(stdout);
                } else {
                    printf("[DEBUG] ERROR: vkCreateXcbSurfaceKHR function pointer is NULL!\n");
                    fflush(stdout);
                    result = VK_ERROR_INITIALIZATION_FAILED;
                }
            }
        }

        // Fall back to Xlib if XCB failed or wasn't available
        if (result != VK_SUCCESS && descriptor->windowHandle.display && descriptor->windowHandle.window) {
            printf("[DEBUG] Falling back to Xlib surface creation...\n");
            printf("[DEBUG] Display: %p\n", descriptor->windowHandle.display);
            printf("[DEBUG] Window: %p\n", descriptor->windowHandle.window);

            VkXlibSurfaceCreateInfoKHR createInfo = { 0 };
            createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
            createInfo.dpy = (Display*)descriptor->windowHandle.display;
            createInfo.window = (Window)(uintptr_t)descriptor->windowHandle.window;

            printf("[DEBUG] Getting vkCreateXlibSurfaceKHR function pointer...\n");
            PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR)vkGetInstanceProcAddr(surface->instance->instance, "vkCreateXlibSurfaceKHR");

            if (vkCreateXlibSurfaceKHR) {
                printf("[DEBUG] Calling vkCreateXlibSurfaceKHR...\n");
                result = vkCreateXlibSurfaceKHR(surface->instance->instance, &createInfo, NULL, &surface->surface);
                printf("[DEBUG] vkCreateXlibSurfaceKHR returned: %d\n", result);
            } else {
                printf("[DEBUG] ERROR: vkCreateXlibSurfaceKHR function pointer is NULL!\n");
                result = VK_ERROR_INITIALIZATION_FAILED;
            }
        }

        if (result != VK_SUCCESS) {
            printf("[DEBUG] ERROR: No valid X11 or XCB window handle provided!\n");
        }
    } else {
        // Wayland - simplified implementation without proper Wayland headers
        result = VK_ERROR_INITIALIZATION_FAILED; // Placeholder - would need proper Wayland implementation
    }

#elif defined(__APPLE__)
    // macOS - simplified implementation without proper macOS headers
    result = VK_ERROR_INITIALIZATION_FAILED; // Placeholder - would need proper macOS implementation
#endif

    if (result != VK_SUCCESS) {
        free(surface);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    *outSurface = surface;
    return GFX_RESULT_SUCCESS;
}

GfxResult vulkan_deviceCreateSwapchain(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain)
{
    if (!device || !surface || !descriptor || !outSwapchain)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *outSwapchain = NULL;

    GfxSwapchain swapchain = malloc(sizeof(struct GfxSwapchain_T));
    if (!swapchain)
        return GFX_RESULT_ERROR_OUT_OF_MEMORY;

    memset(swapchain, 0, sizeof(struct GfxSwapchain_T));
    swapchain->device = device;
    swapchain->surface = surface;

    // Query surface capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->adapter->physicalDevice, surface->surface, &capabilities);

    // Choose extent
    VkExtent2D extent;
    if (capabilities.currentExtent.width != UINT32_MAX) {
        extent = capabilities.currentExtent;
    } else {
        extent.width = descriptor->width;
        extent.height = descriptor->height;
        extent.width = extent.width < capabilities.minImageExtent.width ? capabilities.minImageExtent.width : extent.width;
        extent.width = extent.width > capabilities.maxImageExtent.width ? capabilities.maxImageExtent.width : extent.width;
        extent.height = extent.height < capabilities.minImageExtent.height ? capabilities.minImageExtent.height : extent.height;
        extent.height = extent.height > capabilities.maxImageExtent.height ? capabilities.maxImageExtent.height : extent.height;
    }

    // Choose image count
    uint32_t imageCount = descriptor->bufferCount;
    if (imageCount < capabilities.minImageCount) {
        imageCount = capabilities.minImageCount;
    }
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    // Query surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device->adapter->physicalDevice, surface->surface, &formatCount, NULL);
    VkSurfaceFormatKHR* formats = malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(device->adapter->physicalDevice, surface->surface, &formatCount, formats);

    // Choose format
    VkFormat format = gfxTextureFormatToVkFormat(descriptor->format);
    VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    bool formatFound = false;
    for (uint32_t i = 0; i < formatCount; i++) {
        if (formats[i].format == format) {
            colorSpace = formats[i].colorSpace;
            formatFound = true;
            break;
        }
    }

    if (!formatFound && formatCount > 0) {
        format = formats[0].format;
        colorSpace = formats[0].colorSpace;
    }

    free(formats);

    // Query present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device->adapter->physicalDevice, surface->surface, &presentModeCount, NULL);
    VkPresentModeKHR* presentModes = malloc(presentModeCount * sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(device->adapter->physicalDevice, surface->surface, &presentModeCount, presentModes);

    // Choose present mode
    VkPresentModeKHR presentMode = gfxPresentModeToVkPresentMode(descriptor->presentMode);
    bool presentModeFound = false;
    for (uint32_t i = 0; i < presentModeCount; i++) {
        if (presentModes[i] == presentMode) {
            presentModeFound = true;
            break;
        }
    }

    if (!presentModeFound) {
        presentMode = VK_PRESENT_MODE_FIFO_KHR; // Always available
    }

    free(presentModes);

    // Create swapchain
    VkSwapchainCreateInfoKHR createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface->surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = format;
    createInfo.imageColorSpace = colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(device->device, &createInfo, NULL, &swapchain->swapchain);
    if (result != VK_SUCCESS) {
        free(swapchain);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    // Store swapchain properties
    swapchain->format = format;
    swapchain->extent = extent;
    swapchain->imageCount = imageCount;

    // Create fence for image acquisition synchronization
    VkFenceCreateInfo fenceInfo = { 0 };
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled so first acquire works
    vkCreateFence(device->device, &fenceInfo, NULL, &swapchain->acquireFence);

    // Get swapchain images
    vkGetSwapchainImagesKHR(device->device, swapchain->swapchain, &swapchain->imageCount, NULL);
    swapchain->images = malloc(swapchain->imageCount * sizeof(VkImage));
    vkGetSwapchainImagesKHR(device->device, swapchain->swapchain, &swapchain->imageCount, swapchain->images);

    // Create image views
    swapchain->imageViews = malloc(swapchain->imageCount * sizeof(VkImageView));
    swapchain->textureViews = malloc(swapchain->imageCount * sizeof(GfxTextureView));

    for (uint32_t i = 0; i < swapchain->imageCount; i++) {
        VkImageViewCreateInfo viewInfo = { 0 };
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapchain->images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkResult viewResult = vkCreateImageView(device->device, &viewInfo, NULL, &swapchain->imageViews[i]);
        printf("[SWAPCHAIN DEBUG] Created image view %u: result=%d, handle=%p\n",
            i, viewResult, (void*)(uintptr_t)swapchain->imageViews[i]);

        // Create GfxTextureView wrapper
        swapchain->textureViews[i] = malloc(sizeof(struct GfxTextureView_T));
        memset(swapchain->textureViews[i], 0, sizeof(struct GfxTextureView_T));
        swapchain->textureViews[i]->imageView = swapchain->imageViews[i];
        swapchain->textureViews[i]->texture = NULL; // Swapchain images don't have GfxTexture wrappers
        swapchain->textureViews[i]->format = vkFormatToGfxTextureFormat(format);
        swapchain->textureViews[i]->device = device;
        swapchain->textureViews[i]->width = extent.width;
        swapchain->textureViews[i]->height = extent.height;

        printf("[SWAPCHAIN DEBUG] Texture view %u: textureView=%p, imageView=%p, width=%u, height=%u\n",
            i, (void*)swapchain->textureViews[i],
            (void*)(uintptr_t)swapchain->textureViews[i]->imageView,
            swapchain->textureViews[i]->width, swapchain->textureViews[i]->height);
    }

    *outSwapchain = swapchain;
    return GFX_RESULT_SUCCESS;
}

GfxResult vulkan_deviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer)
{
    if (!device || !descriptor || !outBuffer)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *outBuffer = NULL;

    GfxBuffer buffer = malloc(sizeof(struct GfxBuffer_T));
    if (!buffer)
        return GFX_RESULT_ERROR_OUT_OF_MEMORY;

    memset(buffer, 0, sizeof(struct GfxBuffer_T));
    buffer->size = descriptor->size;
    buffer->usage = descriptor->usage;
    buffer->device = device;

    // Create buffer
    VkBufferCreateInfo bufferInfo = { 0 };
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = descriptor->size;
    bufferInfo.usage = gfxBufferUsageToVkBufferUsage(descriptor->usage);
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device->device, &bufferInfo, NULL, &buffer->buffer);
    if (result != VK_SUCCESS) {
        free(buffer);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    // Allocate memory
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device->device, buffer->buffer, &memRequirements);

    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    uint32_t memoryType = findMemoryType(device->adapter->physicalDevice, memRequirements.memoryTypeBits, properties);

    if (memoryType == UINT32_MAX) {
        vkDestroyBuffer(device->device, buffer->buffer, NULL);
        free(buffer);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    VkMemoryAllocateInfo allocInfo = { 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryType;

    result = vkAllocateMemory(device->device, &allocInfo, NULL, &buffer->memory);
    if (result != VK_SUCCESS) {
        vkDestroyBuffer(device->device, buffer->buffer, NULL);
        free(buffer);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    vkBindBufferMemory(device->device, buffer->buffer, buffer->memory, 0);

    // Map memory if requested
    if (descriptor->mappedAtCreation) {
        vkMapMemory(device->device, buffer->memory, 0, descriptor->size, 0, &buffer->mappedData);
    }

    *outBuffer = buffer;
    return GFX_RESULT_SUCCESS;
}

GfxResult vulkan_deviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture)
{
    if (!device || !descriptor || !outTexture)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *outTexture = NULL;

    GfxTexture texture = malloc(sizeof(struct GfxTexture_T));
    if (!texture)
        return GFX_RESULT_ERROR_OUT_OF_MEMORY;

    memset(texture, 0, sizeof(struct GfxTexture_T));
    texture->device = device;
    texture->format = gfxTextureFormatToVkFormat(descriptor->format);
    texture->extent = descriptor->size;
    texture->mipLevels = descriptor->mipLevelCount;
    texture->arrayLayers = descriptor->size.depth;
    texture->samples = VK_SAMPLE_COUNT_1_BIT; // Simple default
    texture->usage = descriptor->usage;

    // Create image
    VkImageCreateInfo imageInfo = (VkImageCreateInfo){ 0 };
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = descriptor->size.width;
    imageInfo.extent.height = descriptor->size.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = descriptor->mipLevelCount;
    imageInfo.arrayLayers = descriptor->size.depth;
    imageInfo.format = texture->format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = gfxTextureUsageToVkImageUsage(descriptor->usage, texture->format);
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateImage(device->device, &imageInfo, NULL, &texture->image);
    if (result != VK_SUCCESS) {
        free(texture);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    // Allocate memory
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device->device, texture->image, &memRequirements);

    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    uint32_t memoryType = findMemoryType(device->adapter->physicalDevice, memRequirements.memoryTypeBits, properties);

    if (memoryType == UINT32_MAX) {
        vkDestroyImage(device->device, texture->image, NULL);
        free(texture);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    VkMemoryAllocateInfo allocInfo = (VkMemoryAllocateInfo){ 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryType;

    result = vkAllocateMemory(device->device, &allocInfo, NULL, &texture->memory);
    if (result != VK_SUCCESS) {
        vkDestroyImage(device->device, texture->image, NULL);
        free(texture);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    vkBindImageMemory(device->device, texture->image, texture->memory, 0);

    *outTexture = texture;
    return GFX_RESULT_SUCCESS;
}

// ============================================================================
// Buffer Implementation
// ============================================================================

void vulkan_bufferDestroy(GfxBuffer buffer)
{
    if (!buffer)
        return;

    if (buffer->mappedData) {
        vkUnmapMemory(buffer->device->device, buffer->memory);
    }

    vkDestroyBuffer(buffer->device->device, buffer->buffer, NULL);
    vkFreeMemory(buffer->device->device, buffer->memory, NULL);
    free(buffer);
}

uint64_t vulkan_bufferGetSize(GfxBuffer buffer)
{
    return buffer ? buffer->size : 0;
}

GfxBufferUsage vulkan_bufferGetUsage(GfxBuffer buffer)
{
    return buffer ? buffer->usage : GFX_BUFFER_USAGE_NONE;
}

GfxResult vulkan_bufferMapAsync(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer)
{
    if (!buffer || !outMappedPointer)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *outMappedPointer = NULL;

    if (!buffer->mappedData) {
        VkResult result = vkMapMemory(buffer->device->device, buffer->memory, offset,
            size == 0 ? VK_WHOLE_SIZE : size, 0, &buffer->mappedData);
        if (result != VK_SUCCESS)
            return GFX_RESULT_ERROR_UNKNOWN;
    }

    *outMappedPointer = buffer->mappedData;
    return GFX_RESULT_SUCCESS;
}

void vulkan_bufferUnmap(GfxBuffer buffer)
{
    if (!buffer || !buffer->mappedData)
        return;

    vkUnmapMemory(buffer->device->device, buffer->memory);
    buffer->mappedData = NULL;
}

// ============================================================================
// Queue Implementation
// ============================================================================

GfxResult vulkan_queueSubmit(GfxQueue queue, GfxCommandEncoder commandEncoder)
{
    if (!queue || !commandEncoder)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    VkSubmitInfo submitInfo = { 0 };
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandEncoder->commandBuffer;

    vkQueueSubmit(queue->queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue->queue); // Simple synchronization for now

    return GFX_RESULT_SUCCESS;
}

void vulkan_queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size)
{
    if (!queue || !buffer || !data)
        return;

    void* mappedData = NULL;
    GfxResult result = vulkan_bufferMapAsync(buffer, offset, size, &mappedData);
    if (result == GFX_RESULT_SUCCESS && mappedData) {
        memcpy(mappedData, data, size);
        vulkan_bufferUnmap(buffer);
    }
}

void vulkan_queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
    const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent)
{
    if (!queue || !texture || !data || !extent || dataSize == 0)
        return;

    GfxDevice device = queue->device;

    // Create staging buffer
    VkBufferCreateInfo bufferInfo = { 0 };
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = dataSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer stagingBuffer;
    VkResult result = vkCreateBuffer(device->device, &bufferInfo, NULL, &stagingBuffer);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "[ERROR] Failed to create staging buffer for texture upload\n");
        return;
    }

    // Allocate staging buffer memory
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device->device, stagingBuffer, &memRequirements);

    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    uint32_t memoryType = findMemoryType(device->adapter->physicalDevice, memRequirements.memoryTypeBits, properties);

    if (memoryType == UINT32_MAX) {
        fprintf(stderr, "[ERROR] Failed to find suitable memory type for staging buffer\n");
        vkDestroyBuffer(device->device, stagingBuffer, NULL);
        return;
    }

    VkMemoryAllocateInfo allocInfo = { 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryType;

    VkDeviceMemory stagingMemory;
    result = vkAllocateMemory(device->device, &allocInfo, NULL, &stagingMemory);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "[ERROR] Failed to allocate staging buffer memory\n");
        vkDestroyBuffer(device->device, stagingBuffer, NULL);
        return;
    }

    vkBindBufferMemory(device->device, stagingBuffer, stagingMemory, 0);

    // Copy data to staging buffer
    void* mappedData;
    vkMapMemory(device->device, stagingMemory, 0, dataSize, 0, &mappedData);
    memcpy(mappedData, data, dataSize);
    vkUnmapMemory(device->device, stagingMemory);

    // Create command buffer for copy operation
    VkCommandPoolCreateInfo poolInfo = { 0 };
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = queue->queueFamily;

    VkCommandPool commandPool;
    vkCreateCommandPool(device->device, &poolInfo, NULL, &commandPool);

    VkCommandBufferAllocateInfo allocCmdInfo = { 0 };
    allocCmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocCmdInfo.commandPool = commandPool;
    allocCmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocCmdInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device->device, &allocCmdInfo, &commandBuffer);

    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo = { 0 };
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // Transition image layout to transfer dst optimal
    VkImageMemoryBarrier barrier = { 0 };
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = texture->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = mipLevel;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    // Copy buffer to image
    VkBufferImageCopy region = { 0 };
    region.bufferOffset = 0;
    region.bufferRowLength = bytesPerRow > 0 ? (bytesPerRow * 8) / getFormatBitsPerPixel(texture->format) : 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = (VkOffset3D){ origin ? origin->x : 0, origin ? origin->y : 0, origin ? origin->z : 0 };
    region.imageExtent = (VkExtent3D){ extent->width, extent->height, extent->depth };

    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, texture->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition image layout to shader read optimal
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    // End command buffer
    vkEndCommandBuffer(commandBuffer);

    // Submit command buffer
    VkSubmitInfo submitInfo = { 0 };
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(queue->queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue->queue);

    // Cleanup
    vkDestroyCommandPool(device->device, commandPool, NULL);
    vkDestroyBuffer(device->device, stagingBuffer, NULL);
    vkFreeMemory(device->device, stagingMemory, NULL);
}

// ============================================================================
// Swapchain Implementation
// ============================================================================

void vulkan_swapchainDestroy(GfxSwapchain swapchain)
{
    if (!swapchain)
        return;

    // Destroy fence
    if (swapchain->acquireFence) {
        vkDestroyFence(swapchain->device->device, swapchain->acquireFence, NULL);
    }

    // Destroy texture views
    if (swapchain->textureViews) {
        for (uint32_t i = 0; i < swapchain->imageCount; i++) {
            if (swapchain->textureViews[i]) {
                free(swapchain->textureViews[i]);
            }
        }
        free(swapchain->textureViews);
    }

    // Destroy image views
    if (swapchain->imageViews) {
        for (uint32_t i = 0; i < swapchain->imageCount; i++) {
            vkDestroyImageView(swapchain->device->device, swapchain->imageViews[i], NULL);
        }
        free(swapchain->imageViews);
    }

    if (swapchain->images) {
        free(swapchain->images);
    }

    vkDestroySwapchainKHR(swapchain->device->device, swapchain->swapchain, NULL);
    free(swapchain);
}

uint32_t vulkan_swapchainGetWidth(GfxSwapchain swapchain)
{
    return swapchain ? swapchain->extent.width : 0;
}

uint32_t vulkan_swapchainGetHeight(GfxSwapchain swapchain)
{
    return swapchain ? swapchain->extent.height : 0;
}

GfxTextureFormat vulkan_swapchainGetFormat(GfxSwapchain swapchain)
{
    return swapchain ? vkFormatToGfxTextureFormat(swapchain->format) : GFX_TEXTURE_FORMAT_UNDEFINED;
}

uint32_t vulkan_swapchainGetBufferCount(GfxSwapchain swapchain)
{
    return swapchain ? swapchain->imageCount : 0;
}

GfxTextureView vulkan_swapchainGetCurrentTextureView(GfxSwapchain swapchain)
{
    if (!swapchain)
        return NULL;

    // Wait for previous acquire to complete
    vkWaitForFences(swapchain->device->device, 1, &swapchain->acquireFence, VK_TRUE, UINT64_MAX);
    vkResetFences(swapchain->device->device, 1, &swapchain->acquireFence);

    // Acquire next image with fence synchronization
    VkResult result = vkAcquireNextImageKHR(swapchain->device->device, swapchain->swapchain,
        UINT64_MAX, VK_NULL_HANDLE, swapchain->acquireFence,
        &swapchain->currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        swapchain->needsRecreation = true;
        return NULL;
    }

    if (result != VK_SUCCESS) {
        return NULL;
    }

    GfxTextureView view = swapchain->textureViews[swapchain->currentImageIndex];

    // DEFENSIVE FIX: Re-sync the imageView from the swapchain's authoritative array
    // to avoid using potentially corrupted cached values
    view->imageView = swapchain->imageViews[swapchain->currentImageIndex];

    return view;
}

GfxResult vulkan_swapchainPresent(GfxSwapchain swapchain)
{
    if (!swapchain)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    // Wait for the acquire fence before presenting
    vkWaitForFences(swapchain->device->device, 1, &swapchain->acquireFence, VK_TRUE, UINT64_MAX);

    VkPresentInfoKHR presentInfo = { 0 };
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 0;
    presentInfo.pWaitSemaphores = NULL;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain->swapchain;
    presentInfo.pImageIndices = &swapchain->currentImageIndex;

    VkResult result = vkQueuePresentKHR(swapchain->device->queue->queue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        swapchain->needsRecreation = true;
    }

    return GFX_RESULT_SUCCESS;
}

void vulkan_swapchainResize(GfxSwapchain swapchain, uint32_t width, uint32_t height)
{
    if (!swapchain)
        return;

    // Store new dimensions
    swapchain->surface->width = width;
    swapchain->surface->height = height;
    swapchain->needsRecreation = true;
}

bool vulkan_swapchainNeedsRecreation(GfxSwapchain swapchain)
{
    return swapchain ? swapchain->needsRecreation : false;
}

// ============================================================================
// Surface Implementation
// ============================================================================

void vulkan_surfaceDestroy(GfxSurface surface)
{
    if (!surface)
        return;

    vkDestroySurfaceKHR(surface->instance->instance, surface->surface, NULL);
    free(surface);
}

uint32_t vulkan_surfaceGetWidth(GfxSurface surface)
{
    return surface ? surface->width : 0;
}

uint32_t vulkan_surfaceGetHeight(GfxSurface surface)
{
    return surface ? surface->height : 0;
}

void vulkan_surfaceResize(GfxSurface surface, uint32_t width, uint32_t height)
{
    if (!surface)
        return;
    surface->width = width;
    surface->height = height;
}

uint32_t vulkan_surfaceGetSupportedFormats(GfxSurface surface, GfxTextureFormat* formats, uint32_t maxFormats)
{
    if (!surface)
        return 0;

    // We need to get the physical device from somewhere - let's add it to surface creation
    // For now, we'll return a stub implementation
    if (formats && maxFormats > 0) {
        // Return some common formats as a fallback
        GfxTextureFormat commonFormats[] = {
            GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM,
            GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM,
            GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB,
            GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM_SRGB
        };

        uint32_t count = (sizeof(commonFormats) / sizeof(commonFormats[0])) < maxFormats ? (sizeof(commonFormats) / sizeof(commonFormats[0])) : maxFormats;

        for (uint32_t i = 0; i < count; i++) {
            formats[i] = commonFormats[i];
        }

        return count;
    }

    return 4; // Number of common formats
}

uint32_t vulkan_surfaceGetSupportedPresentModes(GfxSurface surface, GfxPresentMode* presentModes, uint32_t maxModes)
{
    if (!surface)
        return 0;

    if (presentModes && maxModes > 0) {
        // Return some common present modes as a fallback
        GfxPresentMode commonModes[] = {
            GFX_PRESENT_MODE_FIFO,
            GFX_PRESENT_MODE_IMMEDIATE,
            GFX_PRESENT_MODE_MAILBOX,
            GFX_PRESENT_MODE_FIFO_RELAXED
        };

        uint32_t count = (sizeof(commonModes) / sizeof(commonModes[0])) < maxModes ? (sizeof(commonModes) / sizeof(commonModes[0])) : maxModes;

        for (uint32_t i = 0; i < count; i++) {
            presentModes[i] = commonModes[i];
        }

        return count;
    }

    return 4; // Number of common present modes
}

GfxPlatformWindowHandle vulkan_surfaceGetPlatformHandle(GfxSurface surface)
{
    return surface ? surface->windowHandle : (GfxPlatformWindowHandle){ 0 };
}

// ============================================================================
// Texture Implementation
// ============================================================================

void vulkan_textureDestroy(GfxTexture texture)
{
    if (!texture)
        return;

    vkDestroyImage(texture->device->device, texture->image, NULL);
    vkFreeMemory(texture->device->device, texture->memory, NULL);
    free(texture);
}

GfxExtent3D vulkan_textureGetSize(GfxTexture texture)
{
    return texture ? texture->extent : (GfxExtent3D){ 0, 0, 0 };
}

GfxTextureFormat vulkan_textureGetFormat(GfxTexture texture)
{
    return texture ? vkFormatToGfxTextureFormat(texture->format) : GFX_TEXTURE_FORMAT_UNDEFINED;
}

uint32_t vulkan_textureGetMipLevelCount(GfxTexture texture)
{
    return texture ? texture->mipLevels : 0;
}

uint32_t vulkan_textureGetSampleCount(GfxTexture texture)
{
    return texture ? texture->samples : 0;
}

GfxTextureUsage vulkan_textureGetUsage(GfxTexture texture)
{
    return texture ? texture->usage : GFX_TEXTURE_USAGE_NONE;
}

GfxResult vulkan_textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView)
{
    if (!texture || !outView)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *outView = NULL;

    GfxTextureView textureView = malloc(sizeof(struct GfxTextureView_T));
    if (!textureView)
        return GFX_RESULT_ERROR_OUT_OF_MEMORY;

    memset(textureView, 0, sizeof(struct GfxTextureView_T));
    textureView->texture = texture;
    textureView->device = texture->device;
    textureView->width = texture->extent.width;
    textureView->height = texture->extent.height;

    VkFormat format = descriptor ? gfxTextureFormatToVkFormat(descriptor->format) : texture->format;
    textureView->format = descriptor ? descriptor->format : vkFormatToGfxTextureFormat(texture->format);

    VkImageViewCreateInfo viewInfo = { 0 };
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = texture->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = isDepthFormat(format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = descriptor ? descriptor->baseMipLevel : 0;
    viewInfo.subresourceRange.levelCount = descriptor ? descriptor->mipLevelCount : texture->mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = descriptor ? descriptor->baseArrayLayer : 0;
    viewInfo.subresourceRange.layerCount = descriptor ? descriptor->arrayLayerCount : texture->arrayLayers;

    VkResult result = vkCreateImageView(texture->device->device, &viewInfo, NULL, &textureView->imageView);
    if (result != VK_SUCCESS) {
        free(textureView);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    *outView = textureView;
    return GFX_RESULT_SUCCESS;
}

void vulkan_textureViewDestroy(GfxTextureView textureView)
{
    if (!textureView)
        return;

    vkDestroyImageView(textureView->device->device, textureView->imageView, NULL);
    free(textureView);
}

GfxTexture vulkan_textureViewGetTexture(GfxTextureView textureView)
{
    return textureView ? textureView->texture : NULL;
}

// ============================================================================
// Sampler Implementation
// ============================================================================

GfxResult vulkan_deviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler)
{
    if (!device || !descriptor || !outSampler)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *outSampler = NULL;

    GfxSampler sampler = malloc(sizeof(struct GfxSampler_T));
    if (!sampler)
        return GFX_RESULT_ERROR_OUT_OF_MEMORY;

    memset(sampler, 0, sizeof(struct GfxSampler_T));
    sampler->device = device;

    VkSamplerCreateInfo samplerInfo = { 0 };
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = gfxFilterModeToVkFilter(descriptor->magFilter);
    samplerInfo.minFilter = gfxFilterModeToVkFilter(descriptor->minFilter);
    samplerInfo.addressModeU = gfxAddressModeToVkSamplerAddressMode(descriptor->addressModeU);
    samplerInfo.addressModeV = gfxAddressModeToVkSamplerAddressMode(descriptor->addressModeV);
    samplerInfo.addressModeW = gfxAddressModeToVkSamplerAddressMode(descriptor->addressModeW);
    samplerInfo.anisotropyEnable = descriptor->maxAnisotropy > 1 ? VK_TRUE : VK_FALSE;
    samplerInfo.maxAnisotropy = (float)descriptor->maxAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = descriptor->compare ? VK_TRUE : VK_FALSE;
    samplerInfo.compareOp = descriptor->compare ? gfxCompareFunctionToVkCompareOp(*descriptor->compare) : VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = gfxFilterModeToVkSamplerMipmapMode(descriptor->mipmapFilter);
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = descriptor->lodMinClamp;
    samplerInfo.maxLod = descriptor->lodMaxClamp;

    VkResult result = vkCreateSampler(device->device, &samplerInfo, NULL, &sampler->sampler);
    if (result != VK_SUCCESS) {
        free(sampler);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    *outSampler = sampler;
    return GFX_RESULT_SUCCESS;
}

void vulkan_samplerDestroy(GfxSampler sampler)
{
    if (!sampler)
        return;

    vkDestroySampler(sampler->device->device, sampler->sampler, NULL);
    free(sampler);
}

// ============================================================================
// Shader Implementation
// ============================================================================

GfxResult vulkan_deviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader)
{
    if (!device || !descriptor || !outShader)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *outShader = NULL;

    GfxShader shader = malloc(sizeof(struct GfxShader_T));
    if (!shader)
        return GFX_RESULT_ERROR_OUT_OF_MEMORY;

    memset(shader, 0, sizeof(struct GfxShader_T));
    shader->device = device;

    // Store entry point
    if (descriptor->entryPoint) {
        size_t len = strlen(descriptor->entryPoint) + 1;
        shader->entryPoint = malloc(len);
        strcpy(shader->entryPoint, descriptor->entryPoint);
    }

    // Check if this is pre-compiled SPIR-V binary or WGSL source
    const uint32_t* spirvCode = NULL;
    size_t spirvSize = 0;
    bool needsFreeing = false;

    if (descriptor->codeSize > 0) {
        // Binary SPIR-V code provided
        spirvCode = (const uint32_t*)descriptor->code;
        spirvSize = descriptor->codeSize;
        printf("[DEBUG] Using pre-compiled SPIR-V shader (%zu bytes)\n", spirvSize);
    } else {
        // WGSL source code - attempt to compile
        spirvCode = compileWGSLToSPIRV((const char*)descriptor->code, descriptor->entryPoint, &spirvSize);
        needsFreeing = true;

        if (!spirvCode) {
            // Compilation failed
            if (shader->entryPoint)
                free(shader->entryPoint);
            free(shader);
            return GFX_RESULT_ERROR_UNKNOWN;
        }
    }

    VkShaderModuleCreateInfo createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirvSize;
    createInfo.pCode = spirvCode;

    VkResult result = vkCreateShaderModule(device->device, &createInfo, NULL, &shader->shaderModule);

    if (needsFreeing && spirvCode) {
        free((void*)spirvCode);
    }

    if (result != VK_SUCCESS) {
        if (shader->entryPoint)
            free(shader->entryPoint);
        free(shader);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    *outShader = shader;
    return GFX_RESULT_SUCCESS;
}

void vulkan_shaderDestroy(GfxShader shader)
{
    if (!shader)
        return;

    vkDestroyShaderModule(shader->device->device, shader->shaderModule, NULL);
    if (shader->entryPoint)
        free(shader->entryPoint);
    free(shader);
}

// ============================================================================
// Command Encoder Implementation
// ============================================================================

GfxResult vulkan_deviceCreateCommandEncoder(GfxDevice device, const char* label, GfxCommandEncoder* outEncoder)
{
    (void)label;
    if (!device || !outEncoder)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *outEncoder = NULL;

    GfxCommandEncoder encoder = malloc(sizeof(struct GfxCommandEncoder_T));
    if (!encoder)
        return GFX_RESULT_ERROR_OUT_OF_MEMORY;

    memset(encoder, 0, sizeof(struct GfxCommandEncoder_T));
    encoder->device = device;

    // Create command pool
    VkCommandPoolCreateInfo poolInfo = { 0 };
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = device->adapter->graphicsQueueFamily;

    VkResult result = vkCreateCommandPool(device->device, &poolInfo, NULL, &encoder->commandPool);
    if (result != VK_SUCCESS) {
        free(encoder);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    // Allocate command buffer
    VkCommandBufferAllocateInfo allocInfo = { 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = encoder->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(device->device, &allocInfo, &encoder->commandBuffer);
    if (result != VK_SUCCESS) {
        vkDestroyCommandPool(device->device, encoder->commandPool, NULL);
        free(encoder);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo = { 0 };
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(encoder->commandBuffer, &beginInfo);
    encoder->isRecording = true;

    *outEncoder = encoder;
    return GFX_RESULT_SUCCESS;
}

void vulkan_commandEncoderDestroy(GfxCommandEncoder commandEncoder)
{
    if (!commandEncoder)
        return;

    // Clean up tracked render passes and framebuffers
    for (uint32_t i = 0; i < commandEncoder->resourceCount; i++) {
        if (commandEncoder->renderPasses[i] != VK_NULL_HANDLE) {
            vkDestroyRenderPass(commandEncoder->device->device, commandEncoder->renderPasses[i], NULL);
        }
        if (commandEncoder->framebuffers[i] != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(commandEncoder->device->device, commandEncoder->framebuffers[i], NULL);
        }
    }

    vkDestroyCommandPool(commandEncoder->device->device, commandEncoder->commandPool, NULL);
    free(commandEncoder);
}

void vulkan_commandEncoderFinish(GfxCommandEncoder commandEncoder)
{
    if (!commandEncoder || !commandEncoder->isRecording)
        return;

    vkEndCommandBuffer(commandEncoder->commandBuffer);
    commandEncoder->isRecording = false;
}

// ============================================================================
// Render Pass Encoder Implementation
// ============================================================================

GfxResult vulkan_commandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder,
    const GfxTextureView* colorAttachments, uint32_t colorAttachmentCount,
    const GfxColor* clearColors,
    GfxTextureView depthStencilAttachment,
    float depthClearValue, uint32_t stencilClearValue,
    GfxRenderPassEncoder* outRenderPass)
{
    if (!commandEncoder || !colorAttachments || colorAttachmentCount == 0 || !outRenderPass)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *outRenderPass = NULL;

    GfxRenderPassEncoder encoder = malloc(sizeof(struct GfxRenderPassEncoder_T));
    if (!encoder)
        return GFX_RESULT_ERROR_OUT_OF_MEMORY;

    memset(encoder, 0, sizeof(struct GfxRenderPassEncoder_T));
    encoder->commandBuffer = commandEncoder->commandBuffer;
    encoder->encoder = commandEncoder;
    encoder->isRecording = true;

    // Get dimensions from first color attachment
    uint32_t width, height;

    // Try to get dimensions from texture if available
    if (colorAttachments[0]->texture) {
        width = colorAttachments[0]->texture->extent.width;
        height = colorAttachments[0]->texture->extent.height;
    } else {
        // This is a swapchain image - use the stored dimensions in the texture view
        width = colorAttachments[0]->width;
        height = colorAttachments[0]->height;
    }

    encoder->viewportWidth = width;
    encoder->viewportHeight = height;

    // Create a compatible render pass that matches the pipeline's render pass structure
    VkAttachmentDescription* attachments = malloc((colorAttachmentCount + (depthStencilAttachment ? 1 : 0)) * sizeof(VkAttachmentDescription));
    VkAttachmentReference* colorRefs = malloc(colorAttachmentCount * sizeof(VkAttachmentReference));

    for (uint32_t i = 0; i < colorAttachmentCount; i++) {
        attachments[i].flags = 0;
        attachments[i].format = gfxTextureFormatToVkFormat(colorAttachments[i]->format);
        attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[i].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        colorRefs[i].attachment = i;
        colorRefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    VkAttachmentReference depthRef = { 0 };
    if (depthStencilAttachment) {
        uint32_t depthIndex = colorAttachmentCount;
        attachments[depthIndex].flags = 0;
        attachments[depthIndex].format = gfxTextureFormatToVkFormat(depthStencilAttachment->format);
        attachments[depthIndex].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[depthIndex].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[depthIndex].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[depthIndex].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[depthIndex].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[depthIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[depthIndex].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        depthRef.attachment = depthIndex;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpass = { 0 };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = colorAttachmentCount;
    subpass.pColorAttachments = colorRefs;
    subpass.pDepthStencilAttachment = depthStencilAttachment ? &depthRef : NULL;

    // Add subpass dependency to match pipeline expectations
    VkSubpassDependency dependency = { 0 };
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    if (depthStencilAttachment) {
        dependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    VkRenderPassCreateInfo renderPassInfo = { 0 };
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = colorAttachmentCount + (depthStencilAttachment ? 1 : 0);
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(commandEncoder->device->device, &renderPassInfo, NULL, &encoder->renderPass);

    free(attachments);
    free(colorRefs);

    if (result != VK_SUCCESS) {
        free(encoder);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    // Create framebuffer
    VkImageView* fbAttachments = malloc((colorAttachmentCount + (depthStencilAttachment ? 1 : 0)) * sizeof(VkImageView));
    for (uint32_t i = 0; i < colorAttachmentCount; i++) {
        fbAttachments[i] = colorAttachments[i]->imageView;
    }
    if (depthStencilAttachment) {
        fbAttachments[colorAttachmentCount] = depthStencilAttachment->imageView;
    }

    VkFramebufferCreateInfo framebufferInfo = { 0 };
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = encoder->renderPass;
    framebufferInfo.attachmentCount = colorAttachmentCount + (depthStencilAttachment ? 1 : 0);
    framebufferInfo.pAttachments = fbAttachments;
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = 1;

    result = vkCreateFramebuffer(commandEncoder->device->device, &framebufferInfo, NULL, &encoder->framebuffer);
    free(fbAttachments);

    if (result != VK_SUCCESS) {
        vkDestroyRenderPass(commandEncoder->device->device, encoder->renderPass, NULL);
        free(encoder);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    // Track these resources in the command encoder for cleanup after submission
    if (commandEncoder->resourceCount >= 32) {
        fprintf(stderr, "[ERROR] Too many render passes created in a single command encoder (max 32)\n");
        vkDestroyFramebuffer(commandEncoder->device->device, encoder->framebuffer, NULL);
        vkDestroyRenderPass(commandEncoder->device->device, encoder->renderPass, NULL);
        free(encoder);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    commandEncoder->renderPasses[commandEncoder->resourceCount] = encoder->renderPass;
    commandEncoder->framebuffers[commandEncoder->resourceCount] = encoder->framebuffer;
    commandEncoder->resourceCount++;

    // Begin render pass
    VkClearValue* clearValues = malloc((colorAttachmentCount + (depthStencilAttachment ? 1 : 0)) * sizeof(VkClearValue));
    for (uint32_t i = 0; i < colorAttachmentCount; i++) {
        if (clearColors) {
            clearValues[i].color.float32[0] = clearColors[i].r;
            clearValues[i].color.float32[1] = clearColors[i].g;
            clearValues[i].color.float32[2] = clearColors[i].b;
            clearValues[i].color.float32[3] = clearColors[i].a;
        } else {
            clearValues[i].color.float32[0] = 0.0f;
            clearValues[i].color.float32[1] = 0.0f;
            clearValues[i].color.float32[2] = 0.0f;
            clearValues[i].color.float32[3] = 1.0f;
        }
    }
    if (depthStencilAttachment) {
        clearValues[colorAttachmentCount].depthStencil.depth = depthClearValue;
        clearValues[colorAttachmentCount].depthStencil.stencil = stencilClearValue;
    }

    VkRenderPassBeginInfo renderPassBegin = { 0 };
    renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBegin.renderPass = encoder->renderPass;
    renderPassBegin.framebuffer = encoder->framebuffer;
    renderPassBegin.renderArea.offset = (VkOffset2D){ 0, 0 };
    renderPassBegin.renderArea.extent = (VkExtent2D){ width, height };
    renderPassBegin.clearValueCount = colorAttachmentCount + (depthStencilAttachment ? 1 : 0);
    renderPassBegin.pClearValues = clearValues;

    vkCmdBeginRenderPass(encoder->commandBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
    free(clearValues);

    *outRenderPass = encoder;
    return GFX_RESULT_SUCCESS;
}

void vulkan_renderPassEncoderDestroy(GfxRenderPassEncoder renderPassEncoder)
{
    if (!renderPassEncoder)
        return;

    // DO NOT destroy render pass and framebuffer here - they are still in use by the command buffer!
    // They will be cleaned up when the command encoder (and its command pool) is destroyed.
    // The command pool destruction will implicitly free all command buffers and their referenced resources.

    free(renderPassEncoder);
}

void vulkan_renderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder)
{
    if (!renderPassEncoder || !renderPassEncoder->isRecording)
        return;

    vkCmdEndRenderPass(renderPassEncoder->commandBuffer);
    renderPassEncoder->isRecording = false;
}

void vulkan_renderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline)
{
    if (!renderPassEncoder || !pipeline)
        return;

    vkCmdBindPipeline(renderPassEncoder->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    renderPassEncoder->currentPipeline = pipeline; // Track current pipeline

    // Set dynamic viewport and scissor state
    VkViewport viewport = { 0 };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)renderPassEncoder->viewportWidth;
    viewport.height = (float)renderPassEncoder->viewportHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = { 0 };
    scissor.offset = (VkOffset2D){ 0, 0 };
    scissor.extent = (VkExtent2D){ renderPassEncoder->viewportWidth, renderPassEncoder->viewportHeight };

    vkCmdSetViewport(renderPassEncoder->commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(renderPassEncoder->commandBuffer, 0, 1, &scissor);
}

void vulkan_renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size)
{
    (void)size;
    if (!renderPassEncoder || !buffer)
        return;

    VkBuffer vertexBuffers[] = { buffer->buffer };
    VkDeviceSize offsets[] = { offset };
    vkCmdBindVertexBuffers(renderPassEncoder->commandBuffer, slot, 1, vertexBuffers, offsets);
}

void vulkan_renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size)
{
    (void)size;
    if (!renderPassEncoder || !buffer)
        return;

    vkCmdBindIndexBuffer(renderPassEncoder->commandBuffer, buffer->buffer, offset, gfxIndexFormatToVkIndexType(format));
}

void vulkan_renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    if (!renderPassEncoder)
        return;

    vkCmdDraw(renderPassEncoder->commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void vulkan_renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance)
{
    if (!renderPassEncoder)
        return;

    vkCmdDrawIndexed(renderPassEncoder->commandBuffer, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}

void vulkan_renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup)
{
    if (!renderPassEncoder || !bindGroup)
        return;

    // Need to have a pipeline set before binding descriptor sets
    if (!renderPassEncoder->currentPipeline) {
        fprintf(stderr, "[ERROR] Cannot bind descriptor set without a pipeline set first!\n");
        return;
    }

    // Bind descriptor sets using the current pipeline layout
    vkCmdBindDescriptorSets(renderPassEncoder->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        renderPassEncoder->currentPipeline->pipelineLayout, index, 1, &bindGroup->descriptorSet, 0, NULL);
}

// ============================================================================
// Render Pipeline Implementation
// ============================================================================

static VkBlendFactor gfxBlendFactorToVkBlendFactor(GfxBlendFactor factor)
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
    case GFX_BLEND_FACTOR_SRC_ALPHA:
        return VK_BLEND_FACTOR_SRC_ALPHA;
    case GFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case GFX_BLEND_FACTOR_DST:
        return VK_BLEND_FACTOR_DST_COLOR;
    case GFX_BLEND_FACTOR_ONE_MINUS_DST:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    case GFX_BLEND_FACTOR_DST_ALPHA:
        return VK_BLEND_FACTOR_DST_ALPHA;
    case GFX_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    case GFX_BLEND_FACTOR_SRC_ALPHA_SATURATED:
        return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
    case GFX_BLEND_FACTOR_CONSTANT:
        return VK_BLEND_FACTOR_CONSTANT_COLOR;
    case GFX_BLEND_FACTOR_ONE_MINUS_CONSTANT:
        return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
    default:
        return VK_BLEND_FACTOR_ZERO;
    }
}

static VkBlendOp gfxBlendOperationToVkBlendOp(GfxBlendOperation op)
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

GfxResult vulkan_deviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline)
{
    if (!device || !descriptor || !outPipeline)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *outPipeline = NULL;

    GfxRenderPipeline pipeline = malloc(sizeof(struct GfxRenderPipeline_T));
    if (!pipeline)
        return GFX_RESULT_ERROR_OUT_OF_MEMORY;

    memset(pipeline, 0, sizeof(struct GfxRenderPipeline_T));
    pipeline->device = device;

    // Create pipeline layout with descriptor set layouts
    VkDescriptorSetLayout* setLayouts = NULL;
    if (descriptor->bindGroupLayoutCount > 0 && descriptor->bindGroupLayouts) {
        printf("[PIPELINE DEBUG] About to extract %u bind group layouts\n", descriptor->bindGroupLayoutCount);
        printf("[PIPELINE DEBUG] bindGroupLayouts pointer: %p\n", (void*)descriptor->bindGroupLayouts);

        setLayouts = malloc(descriptor->bindGroupLayoutCount * sizeof(VkDescriptorSetLayout));
        for (uint32_t i = 0; i < descriptor->bindGroupLayoutCount; i++) {
            printf("[PIPELINE DEBUG] Accessing bindGroupLayouts[%u]...\n", i);
            printf("[PIPELINE DEBUG]   Address of bindGroupLayouts[%u]: %p\n", i, (void*)&descriptor->bindGroupLayouts[i]);
            printf("[PIPELINE DEBUG]   Value of bindGroupLayouts[%u]: %p\n", i, (void*)descriptor->bindGroupLayouts[i]);

            GfxBindGroupLayout layout = descriptor->bindGroupLayouts[i];
            printf("[PIPELINE DEBUG]   layout pointer: %p\n", (void*)layout);
            printf("[PIPELINE DEBUG]   layout->descriptorSetLayout address: %p\n", (void*)&layout->descriptorSetLayout);
            printf("[PIPELINE DEBUG]   layout->descriptorSetLayout value: %p\n", (void*)(uintptr_t)layout->descriptorSetLayout);

            setLayouts[i] = layout->descriptorSetLayout;
        }
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = { 0 };
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = descriptor->bindGroupLayoutCount;
    pipelineLayoutInfo.pSetLayouts = setLayouts;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = NULL;

    VkResult result = vkCreatePipelineLayout(device->device, &pipelineLayoutInfo, NULL, &pipeline->pipelineLayout);

    if (setLayouts) {
        free(setLayouts);
    }

    if (result != VK_SUCCESS) {
        free(pipeline);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    // Create a compatible render pass for the pipeline
    // Build attachment descriptions from pipeline descriptor
    uint32_t attachmentCount = 0;
    uint32_t colorAttachmentCount = 0;
    bool hasDepthAttachment = false;

    if (descriptor->fragment && descriptor->fragment->targets) {
        colorAttachmentCount = descriptor->fragment->targetCount;
        attachmentCount = colorAttachmentCount;
    } else {
        // Default to one color attachment
        colorAttachmentCount = 1;
        attachmentCount = 1;
    }

    if (descriptor->depthStencil) {
        hasDepthAttachment = true;
        attachmentCount++;
    }

    VkAttachmentDescription* attachments = malloc(attachmentCount * sizeof(VkAttachmentDescription));
    VkAttachmentReference* colorRefs = malloc(colorAttachmentCount * sizeof(VkAttachmentReference));

    // Color attachments
    for (uint32_t i = 0; i < colorAttachmentCount; i++) {
        VkFormat format = VK_FORMAT_B8G8R8A8_UNORM; // Default
        if (descriptor->fragment && descriptor->fragment->targets && i < descriptor->fragment->targetCount) {
            format = gfxTextureFormatToVkFormat(descriptor->fragment->targets[i].format);
        }

        attachments[i].flags = 0;
        attachments[i].format = format;
        attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[i].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        colorRefs[i].attachment = i;
        colorRefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    // Depth attachment
    VkAttachmentReference depthRef = { 0 };
    if (hasDepthAttachment) {
        uint32_t depthIndex = colorAttachmentCount;
        VkFormat depthFormat = gfxTextureFormatToVkFormat(descriptor->depthStencil->format);

        attachments[depthIndex].flags = 0;
        attachments[depthIndex].format = depthFormat;
        attachments[depthIndex].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[depthIndex].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[depthIndex].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[depthIndex].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[depthIndex].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[depthIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[depthIndex].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        depthRef.attachment = depthIndex;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpass = { 0 };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = colorAttachmentCount;
    subpass.pColorAttachments = colorRefs;
    subpass.pDepthStencilAttachment = hasDepthAttachment ? &depthRef : NULL;

    VkSubpassDependency dependency = { 0 };
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    if (hasDepthAttachment) {
        dependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    VkRenderPassCreateInfo renderPassInfo = { 0 };
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachmentCount;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    result = vkCreateRenderPass(device->device, &renderPassInfo, NULL, &pipeline->renderPass);

    free(attachments);
    free(colorRefs);

    if (result != VK_SUCCESS) {
        vkDestroyPipelineLayout(device->device, pipeline->pipelineLayout, NULL);
        free(pipeline);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    // Shader stages
    VkPipelineShaderStageCreateInfo shaderStages[2];
    memset(shaderStages, 0, sizeof(shaderStages)); // Initialize all fields to zero
    uint32_t stageCount = 0;

    // Vertex shader
    if (descriptor->vertex->module) {
        shaderStages[stageCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[stageCount].pNext = NULL;
        shaderStages[stageCount].flags = 0;
        shaderStages[stageCount].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[stageCount].module = descriptor->vertex->module->shaderModule;
        shaderStages[stageCount].pName = descriptor->vertex->entryPoint ? descriptor->vertex->entryPoint : "main";
        shaderStages[stageCount].pSpecializationInfo = NULL;
        stageCount++;
    }

    // Fragment shader
    if (descriptor->fragment && descriptor->fragment->module) {
        shaderStages[stageCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[stageCount].pNext = NULL;
        shaderStages[stageCount].flags = 0;
        shaderStages[stageCount].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[stageCount].module = descriptor->fragment->module->shaderModule;
        shaderStages[stageCount].pName = descriptor->fragment->entryPoint ? descriptor->fragment->entryPoint : "main";
        shaderStages[stageCount].pSpecializationInfo = NULL;
        stageCount++;
    }

    // Vertex input state
    VkVertexInputBindingDescription* bindingDescriptions = NULL;
    VkVertexInputAttributeDescription* attributeDescriptions = NULL;
    uint32_t totalAttributes = 0;

    if (descriptor->vertex->buffers && descriptor->vertex->bufferCount > 0) {
        bindingDescriptions = malloc(descriptor->vertex->bufferCount * sizeof(VkVertexInputBindingDescription));

        // Count total attributes
        for (uint32_t i = 0; i < descriptor->vertex->bufferCount; i++) {
            totalAttributes += descriptor->vertex->buffers[i].attributeCount;
        }

        attributeDescriptions = malloc(totalAttributes * sizeof(VkVertexInputAttributeDescription));

        uint32_t attrIndex = 0;
        for (uint32_t i = 0; i < descriptor->vertex->bufferCount; i++) {
            const GfxVertexBufferLayout* buffer = &descriptor->vertex->buffers[i];

            bindingDescriptions[i].binding = i;
            bindingDescriptions[i].stride = (uint32_t)buffer->arrayStride;
            bindingDescriptions[i].inputRate = buffer->stepModeInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;

            for (uint32_t j = 0; j < buffer->attributeCount; j++) {
                const GfxVertexAttribute* attr = &buffer->attributes[j];

                attributeDescriptions[attrIndex].binding = i;
                attributeDescriptions[attrIndex].location = attr->shaderLocation;
                attributeDescriptions[attrIndex].format = gfxTextureFormatToVkFormat(attr->format);
                attributeDescriptions[attrIndex].offset = (uint32_t)attr->offset;
                attrIndex++;
            }
        }
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = { 0 };
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = descriptor->vertex->bufferCount;
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions;
    vertexInputInfo.vertexAttributeDescriptionCount = totalAttributes;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    // Input assembly state
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = { 0 };
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = gfxPrimitiveTopologyToVkPrimitiveTopology(descriptor->primitive->topology);
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    const uint32_t viewportWidth = 800;
    const uint32_t viewportHeight = 600;

    VkViewport viewport = { 0 };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)viewportWidth;
    viewport.height = (float)viewportHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = { 0 };
    scissor.offset = (VkOffset2D){ 0, 0 };
    scissor.extent = (VkExtent2D){ viewportWidth, viewportHeight }; // Default size

    // Viewport state (dynamic)
    // Even with dynamic state, we need to provide dummy viewport/scissor or set pViewports/pScissors to NULL
    VkPipelineViewportStateCreateInfo viewportState = { 0 };
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport; // NULL since we use dynamic state
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor; // NULL since we use dynamic state

    // Rasterization state
    VkPipelineRasterizationStateCreateInfo rasterizer = { 0 };
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = descriptor->primitive->unclippedDepth ? VK_TRUE : VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = descriptor->primitive->cullBackFace ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
    rasterizer.frontFace = descriptor->primitive->frontFaceCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling state
    VkPipelineMultisampleStateCreateInfo multisampling = { 0 };
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Color blending state
    VkPipelineColorBlendAttachmentState* colorBlendAttachments = NULL;
    // colorAttachmentCount was already declared earlier, reuse it

    if (descriptor->fragment && descriptor->fragment->targets) {
        colorBlendAttachments = malloc(colorAttachmentCount * sizeof(VkPipelineColorBlendAttachmentState));

        for (uint32_t i = 0; i < colorAttachmentCount; i++) {
            const GfxColorTargetState* target = &descriptor->fragment->targets[i];
            VkPipelineColorBlendAttachmentState* attachment = &colorBlendAttachments[i];

            attachment->colorWriteMask = target->writeMask;
            attachment->blendEnable = target->blend ? VK_TRUE : VK_FALSE;

            if (target->blend) {
                attachment->srcColorBlendFactor = gfxBlendFactorToVkBlendFactor(target->blend->color.srcFactor);
                attachment->dstColorBlendFactor = gfxBlendFactorToVkBlendFactor(target->blend->color.dstFactor);
                attachment->colorBlendOp = gfxBlendOperationToVkBlendOp(target->blend->color.operation);
                attachment->srcAlphaBlendFactor = gfxBlendFactorToVkBlendFactor(target->blend->alpha.srcFactor);
                attachment->dstAlphaBlendFactor = gfxBlendFactorToVkBlendFactor(target->blend->alpha.dstFactor);
                attachment->alphaBlendOp = gfxBlendOperationToVkBlendOp(target->blend->alpha.operation);
            } else {
                attachment->srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                attachment->dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                attachment->colorBlendOp = VK_BLEND_OP_ADD;
                attachment->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                attachment->dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                attachment->alphaBlendOp = VK_BLEND_OP_ADD;
            }
        }
    } else {
        // Default single color attachment
        colorBlendAttachments = malloc(sizeof(VkPipelineColorBlendAttachmentState));
        colorBlendAttachments[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachments[0].blendEnable = VK_FALSE;
        colorBlendAttachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachments[0].colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachments[0].alphaBlendOp = VK_BLEND_OP_ADD;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending = { 0 };
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = colorAttachmentCount;
    colorBlending.pAttachments = colorBlendAttachments;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // Dynamic state
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = { 0 };
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // Depth/stencil state
    VkPipelineDepthStencilStateCreateInfo depthStencil = { 0 };
    VkPipelineDepthStencilStateCreateInfo* pDepthStencil = NULL;

    if (descriptor->depthStencil) {
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = descriptor->depthStencil->depthWriteEnabled ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp = gfxCompareFunctionToVkCompareOp(descriptor->depthStencil->depthCompare);
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;

        // Stencil state
        depthStencil.stencilTestEnable = VK_TRUE;
        depthStencil.front.failOp = gfxStencilOperationToVkStencilOp(descriptor->depthStencil->stencilFront.failOp);
        depthStencil.front.passOp = gfxStencilOperationToVkStencilOp(descriptor->depthStencil->stencilFront.passOp);
        depthStencil.front.depthFailOp = gfxStencilOperationToVkStencilOp(descriptor->depthStencil->stencilFront.depthFailOp);
        depthStencil.front.compareOp = gfxCompareFunctionToVkCompareOp(descriptor->depthStencil->stencilFront.compare);
        depthStencil.front.compareMask = descriptor->depthStencil->stencilReadMask;
        depthStencil.front.writeMask = descriptor->depthStencil->stencilWriteMask;
        depthStencil.front.reference = 0;

        depthStencil.back.failOp = gfxStencilOperationToVkStencilOp(descriptor->depthStencil->stencilBack.failOp);
        depthStencil.back.passOp = gfxStencilOperationToVkStencilOp(descriptor->depthStencil->stencilBack.passOp);
        depthStencil.back.depthFailOp = gfxStencilOperationToVkStencilOp(descriptor->depthStencil->stencilBack.depthFailOp);
        depthStencil.back.compareOp = gfxCompareFunctionToVkCompareOp(descriptor->depthStencil->stencilBack.compare);
        depthStencil.back.compareMask = descriptor->depthStencil->stencilReadMask;
        depthStencil.back.writeMask = descriptor->depthStencil->stencilWriteMask;
        depthStencil.back.reference = 0;

        pDepthStencil = &depthStencil;
    }

    // Create graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo;
    memset(&pipelineInfo, 0, sizeof(pipelineInfo)); // Initialize everything to zero first
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = NULL;
    pipelineInfo.flags = 0;
    pipelineInfo.stageCount = stageCount;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pTessellationState = NULL;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = pDepthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipeline->pipelineLayout;
    pipelineInfo.renderPass = pipeline->renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipeline->pipeline);

    // Cleanup temporary allocations
    if (bindingDescriptions)
        free(bindingDescriptions);
    if (attributeDescriptions)
        free(attributeDescriptions);
    if (colorBlendAttachments)
        free(colorBlendAttachments);

    if (result != VK_SUCCESS) {
        vkDestroyRenderPass(device->device, pipeline->renderPass, NULL);
        vkDestroyPipelineLayout(device->device, pipeline->pipelineLayout, NULL);
        free(pipeline);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    *outPipeline = pipeline;
    return GFX_RESULT_SUCCESS;
}

void vulkan_renderPipelineDestroy(GfxRenderPipeline renderPipeline)
{
    if (!renderPipeline)
        return;

    vkDestroyPipeline(renderPipeline->device->device, renderPipeline->pipeline, NULL);
    vkDestroyRenderPass(renderPipeline->device->device, renderPipeline->renderPass, NULL);
    vkDestroyPipelineLayout(renderPipeline->device->device, renderPipeline->pipelineLayout, NULL);
    free(renderPipeline);
}

GfxResult vulkan_deviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline)
{
    if (!device || !descriptor || !descriptor->compute || !outPipeline)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *outPipeline = NULL;

    GfxComputePipeline pipeline = malloc(sizeof(struct GfxComputePipeline_T));
    if (!pipeline)
        return GFX_RESULT_ERROR_OUT_OF_MEMORY;

    memset(pipeline, 0, sizeof(struct GfxComputePipeline_T));
    pipeline->device = device;

    // Create pipeline layout (simplified)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = { 0 };
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = NULL;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = NULL;

    VkResult result = vkCreatePipelineLayout(device->device, &pipelineLayoutInfo, NULL, &pipeline->pipelineLayout);
    if (result != VK_SUCCESS) {
        free(pipeline);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    // Create compute pipeline
    VkComputePipelineCreateInfo pipelineInfo = { 0 };
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = descriptor->compute->shaderModule;
    pipelineInfo.stage.pName = descriptor->entryPoint ? descriptor->entryPoint : "main";
    pipelineInfo.layout = pipeline->pipelineLayout;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    result = vkCreateComputePipelines(device->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipeline->pipeline);
    if (result != VK_SUCCESS) {
        vkDestroyPipelineLayout(device->device, pipeline->pipelineLayout, NULL);
        free(pipeline);
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    *outPipeline = pipeline;
    return GFX_RESULT_SUCCESS;
}

// ============================================================================
// Compute Pipeline Implementation
// ============================================================================

void vulkan_computePipelineDestroy(GfxComputePipeline computePipeline)
{
    if (!computePipeline)
        return;

    vkDestroyPipeline(computePipeline->device->device, computePipeline->pipeline, NULL);
    vkDestroyPipelineLayout(computePipeline->device->device, computePipeline->pipelineLayout, NULL);
    free(computePipeline);
}

// ============================================================================
// Compute Pass Encoder Implementation
// ============================================================================

GfxResult vulkan_commandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const char* label, GfxComputePassEncoder* outComputePass)
{
    (void)label;
    if (!commandEncoder || !outComputePass)
        return GFX_RESULT_ERROR_INVALID_PARAMETER;

    *outComputePass = NULL;

    GfxComputePassEncoder encoder = malloc(sizeof(struct GfxComputePassEncoder_T));
    if (!encoder)
        return GFX_RESULT_ERROR_OUT_OF_MEMORY;

    memset(encoder, 0, sizeof(struct GfxComputePassEncoder_T));
    encoder->commandBuffer = commandEncoder->commandBuffer;
    encoder->encoder = commandEncoder;
    encoder->isRecording = true;

    // No special setup needed for compute pass in Vulkan - just return the encoder
    *outComputePass = encoder;
    return GFX_RESULT_SUCCESS;
}

void vulkan_computePassEncoderDestroy(GfxComputePassEncoder computePassEncoder)
{
    if (!computePassEncoder)
        return;
    free(computePassEncoder);
}

void vulkan_computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline)
{
    if (!computePassEncoder || !pipeline)
        return;

    vkCmdBindPipeline(computePassEncoder->commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipeline);
}

void vulkan_computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup)
{
    if (!computePassEncoder || !bindGroup)
        return;

    // Bind descriptor sets - in a full implementation you'd need the pipeline layout
    // This is a simplified version
    vkCmdBindDescriptorSets(computePassEncoder->commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
        VK_NULL_HANDLE, index, 1, &bindGroup->descriptorSet, 0, NULL);
}

void vulkan_computePassEncoderDispatchWorkgroups(GfxComputePassEncoder computePassEncoder,
    uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ)
{
    if (!computePassEncoder)
        return;

    vkCmdDispatch(computePassEncoder->commandBuffer, workgroupCountX, workgroupCountY, workgroupCountZ);
}

void vulkan_computePassEncoderEnd(GfxComputePassEncoder computePassEncoder)
{
    if (!computePassEncoder || !computePassEncoder->isRecording)
        return;

    // No special cleanup needed for compute pass in Vulkan
    computePassEncoder->isRecording = false;
}

// ============================================================================
// Enhanced Copy Operations Implementation
// ============================================================================

void vulkan_commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset, uint32_t bytesPerRow,
    GfxTexture destination, const GfxOrigin3D* origin,
    const GfxExtent3D* extent, uint32_t mipLevel)
{
    (void)bytesPerRow;
    if (!commandEncoder || !source || !destination || !origin || !extent)
        return;

    // Transition image layout to transfer dst optimal
    VkImageMemoryBarrier barrier = { 0 };
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = destination->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = mipLevel;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(commandEncoder->commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    // Copy buffer to image
    VkBufferImageCopy region = { 0 };
    region.bufferOffset = sourceOffset;
    region.bufferRowLength = 0; // Tightly packed
    region.bufferImageHeight = 0; // Tightly packed
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = (VkOffset3D){ origin->x, origin->y, origin->z };
    region.imageExtent = (VkExtent3D){ extent->width, extent->height, extent->depth };

    vkCmdCopyBufferToImage(commandEncoder->commandBuffer, source->buffer, destination->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition image layout to shader read optimal
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandEncoder->commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
}

void vulkan_commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* origin, uint32_t mipLevel,
    GfxBuffer destination, uint64_t destinationOffset, uint32_t bytesPerRow,
    const GfxExtent3D* extent)
{
    (void)bytesPerRow;
    if (!commandEncoder || !source || !destination || !origin || !extent)
        return;

    // Transition image layout to transfer src optimal
    VkImageMemoryBarrier barrier = { 0 };
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = source->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = mipLevel;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(commandEncoder->commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    // Copy image to buffer
    VkBufferImageCopy region = { 0 };
    region.bufferOffset = destinationOffset;
    region.bufferRowLength = 0; // Tightly packed
    region.bufferImageHeight = 0; // Tightly packed
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = (VkOffset3D){ origin->x, origin->y, origin->z };
    region.imageExtent = (VkExtent3D){ extent->width, extent->height, extent->depth };

    vkCmdCopyImageToBuffer(commandEncoder->commandBuffer, source->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination->buffer, 1, &region);

    // Transition image layout back to shader read optimal
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandEncoder->commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
}

// ============================================================================
// Stub implementations for remaining functions
// ============================================================================

void vulkan_commandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset,
    GfxBuffer destination, uint64_t destinationOffset,
    uint64_t size)
{
    if (!commandEncoder || !source || !destination)
        return;

    VkBufferCopy copyRegion = { 0 };
    copyRegion.srcOffset = sourceOffset;
    copyRegion.dstOffset = destinationOffset;
    copyRegion.size = size;

    vkCmdCopyBuffer(commandEncoder->commandBuffer, source->buffer, destination->buffer, 1, &copyRegion);
}

// ============================================================================
// Backend Function Table Export
// ============================================================================

static const GfxBackendAPI vulkan_api = {
    .createInstance = vulkan_createInstance,
    .instanceDestroy = vulkan_instanceDestroy,
    .instanceRequestAdapter = vulkan_instanceRequestAdapter,
    .instanceEnumerateAdapters = vulkan_instanceEnumerateAdapters,
    .adapterDestroy = vulkan_adapterDestroy,
    .adapterCreateDevice = vulkan_adapterCreateDevice,
    .adapterGetName = vulkan_adapterGetName,
    .adapterGetBackend = vulkan_adapterGetBackend,
    .deviceDestroy = vulkan_deviceDestroy,
    .deviceGetQueue = vulkan_deviceGetQueue,
    .deviceCreateSurface = vulkan_deviceCreateSurface,
    .deviceCreateSwapchain = vulkan_deviceCreateSwapchain,
    .deviceCreateBuffer = vulkan_deviceCreateBuffer,
    .deviceCreateTexture = vulkan_deviceCreateTexture,
    .deviceCreateSampler = vulkan_deviceCreateSampler,
    .deviceCreateShader = vulkan_deviceCreateShader,
    .deviceCreateBindGroupLayout = vulkan_deviceCreateBindGroupLayout,
    .deviceCreateBindGroup = vulkan_deviceCreateBindGroup,
    .deviceCreateRenderPipeline = vulkan_deviceCreateRenderPipeline,
    .deviceCreateComputePipeline = vulkan_deviceCreateComputePipeline,
    .deviceCreateCommandEncoder = vulkan_deviceCreateCommandEncoder,
    .deviceCreateFence = vulkan_deviceCreateFence,
    .deviceCreateSemaphore = vulkan_deviceCreateSemaphore,
    .deviceWaitIdle = vulkan_deviceWaitIdle,
    .surfaceDestroy = vulkan_surfaceDestroy,
    .surfaceGetWidth = vulkan_surfaceGetWidth,
    .surfaceGetHeight = vulkan_surfaceGetHeight,
    .surfaceResize = vulkan_surfaceResize,
    .surfaceGetSupportedFormats = vulkan_surfaceGetSupportedFormats,
    .surfaceGetSupportedPresentModes = vulkan_surfaceGetSupportedPresentModes,
    .surfaceGetPlatformHandle = vulkan_surfaceGetPlatformHandle,
    .swapchainDestroy = vulkan_swapchainDestroy,
    .swapchainGetWidth = vulkan_swapchainGetWidth,
    .swapchainGetHeight = vulkan_swapchainGetHeight,
    .swapchainGetFormat = vulkan_swapchainGetFormat,
    .swapchainGetBufferCount = vulkan_swapchainGetBufferCount,
    .swapchainGetCurrentTextureView = vulkan_swapchainGetCurrentTextureView,
    .swapchainPresent = vulkan_swapchainPresent,
    .swapchainResize = vulkan_swapchainResize,
    .swapchainNeedsRecreation = vulkan_swapchainNeedsRecreation,
    .bufferDestroy = vulkan_bufferDestroy,
    .bufferGetSize = vulkan_bufferGetSize,
    .bufferGetUsage = vulkan_bufferGetUsage,
    .bufferMapAsync = vulkan_bufferMapAsync,
    .bufferUnmap = vulkan_bufferUnmap,
    .textureDestroy = vulkan_textureDestroy,
    .textureGetSize = vulkan_textureGetSize,
    .textureGetFormat = vulkan_textureGetFormat,
    .textureGetMipLevelCount = vulkan_textureGetMipLevelCount,
    .textureGetSampleCount = vulkan_textureGetSampleCount,
    .textureGetUsage = vulkan_textureGetUsage,
    .textureCreateView = vulkan_textureCreateView,
    .textureViewDestroy = vulkan_textureViewDestroy,
    .textureViewGetTexture = vulkan_textureViewGetTexture,
    .samplerDestroy = vulkan_samplerDestroy,
    .shaderDestroy = vulkan_shaderDestroy,
    .bindGroupLayoutDestroy = vulkan_bindGroupLayoutDestroy,
    .bindGroupDestroy = vulkan_bindGroupDestroy,
    .renderPipelineDestroy = vulkan_renderPipelineDestroy,
    .computePipelineDestroy = vulkan_computePipelineDestroy,
    .queueSubmit = vulkan_queueSubmit,
    .queueSubmitWithSync = vulkan_queueSubmitWithSync,
    .queueWriteBuffer = vulkan_queueWriteBuffer,
    .queueWriteTexture = vulkan_queueWriteTexture,
    .queueWaitIdle = vulkan_queueWaitIdle,
    .commandEncoderDestroy = vulkan_commandEncoderDestroy,
    .commandEncoderBeginRenderPass = vulkan_commandEncoderBeginRenderPass,
    .commandEncoderBeginComputePass = vulkan_commandEncoderBeginComputePass,
    .commandEncoderCopyBufferToBuffer = vulkan_commandEncoderCopyBufferToBuffer,
    .commandEncoderCopyBufferToTexture = vulkan_commandEncoderCopyBufferToTexture,
    .commandEncoderCopyTextureToBuffer = vulkan_commandEncoderCopyTextureToBuffer,
    .commandEncoderFinish = vulkan_commandEncoderFinish,
    .renderPassEncoderDestroy = vulkan_renderPassEncoderDestroy,
    .renderPassEncoderSetPipeline = vulkan_renderPassEncoderSetPipeline,
    .renderPassEncoderSetBindGroup = vulkan_renderPassEncoderSetBindGroup,
    .renderPassEncoderSetVertexBuffer = vulkan_renderPassEncoderSetVertexBuffer,
    .renderPassEncoderSetIndexBuffer = vulkan_renderPassEncoderSetIndexBuffer,
    .renderPassEncoderDraw = vulkan_renderPassEncoderDraw,
    .renderPassEncoderDrawIndexed = vulkan_renderPassEncoderDrawIndexed,
    .renderPassEncoderEnd = vulkan_renderPassEncoderEnd,
    .computePassEncoderDestroy = vulkan_computePassEncoderDestroy,
    .computePassEncoderSetPipeline = vulkan_computePassEncoderSetPipeline,
    .computePassEncoderSetBindGroup = vulkan_computePassEncoderSetBindGroup,
    .computePassEncoderDispatchWorkgroups = vulkan_computePassEncoderDispatchWorkgroups,
    .computePassEncoderEnd = vulkan_computePassEncoderEnd,
    .fenceDestroy = vulkan_fenceDestroy,
    .fenceGetStatus = vulkan_fenceGetStatus,
    .fenceWait = vulkan_fenceWait,
    .fenceReset = vulkan_fenceReset,
    .semaphoreDestroy = vulkan_semaphoreDestroy,
    .semaphoreGetType = vulkan_semaphoreGetType,
    .semaphoreSignal = vulkan_semaphoreSignal,
    .semaphoreWait = vulkan_semaphoreWait,
    .semaphoreGetValue = vulkan_semaphoreGetValue,
};

const GfxBackendAPI* gfxGetVulkanBackend(void)
{
    return &vulkan_api;
}
