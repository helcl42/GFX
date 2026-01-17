#ifndef GFX_VULKAN_CORE_TYPES_H
#define GFX_VULKAN_CORE_TYPES_H

#include "../common/Common.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace gfx::backend::vulkan::core {

// Forward declarations for SubmitInfo
class CommandEncoder;
class Fence;
class Semaphore;
struct RenderPassEncoderCreateInfo;

// Forward declarations for barriers
class Buffer;
class Texture;

// ============================================================================
// Internal Type Definitions
// ============================================================================

// Internal debug message types - pure Vulkan internal enums
enum class DebugMessageSeverity {
    Verbose = 0,
    Info = 1,
    Warning = 2,
    Error = 3
};

enum class DebugMessageType {
    General = 0,
    Validation = 1,
    Performance = 2
};

enum class SemaphoreType {
    Binary,
    Timeline
};

enum class DeviceTypePreference {
    HighPerformance, // Prefer discrete GPU
    LowPower, // Prefer integrated GPU
    SoftwareRenderer // Force CPU-based software renderer
};

enum class InstanceFeatureType {
    Invalid = 0,
    Surface = 1
};

enum class DeviceFeatureType {
    Invalid = 0,
    Swapchain = 1
};

// ============================================================================
// Internal CreateInfo structs - pure Vulkan types, no GFX dependencies
// ============================================================================
struct BufferCreateInfo {
    size_t size;
    VkBufferUsageFlags usage;
    bool mapped;
};

struct BufferImportInfo {
    size_t size;
    VkBufferUsageFlags usage;
    bool mapped;
};
struct BufferInfo {
    uint64_t size;
    VkBufferUsageFlags usage;
    bool mapped;
};
struct TextureCreateInfo {
    VkFormat format;
    VkExtent3D size;
    VkImageUsageFlags usage;
    VkSampleCountFlagBits sampleCount;
    uint32_t mipLevelCount;
    VkImageType imageType;
    uint32_t arrayLayers;
    VkImageCreateFlags flags; // For cube maps, etc.
};

struct TextureImportInfo {
    VkFormat format;
    VkExtent3D size;
    VkImageUsageFlags usage;
    VkSampleCountFlagBits sampleCount;
    uint32_t mipLevelCount;
    VkImageType imageType;
    uint32_t arrayLayers;
    VkImageCreateFlags flags; // For cube maps, etc.
};

struct TextureInfo {
    VkImageType imageType;
    VkExtent3D size;
    uint32_t arrayLayers;
    VkFormat format;
    uint32_t mipLevelCount;
    VkSampleCountFlagBits sampleCount;
    VkImageUsageFlags usage;
};

struct SwapchainInfo {
    uint32_t width;
    uint32_t height;
    VkFormat format;
    uint32_t imageCount;
    VkPresentModeKHR presentMode;
};

struct TextureViewCreateInfo {
    VkImageViewType viewType;
    VkFormat format; // VK_FORMAT_UNDEFINED means use texture's format
    uint32_t baseMipLevel;
    uint32_t mipLevelCount;
    uint32_t baseArrayLayer;
    uint32_t arrayLayerCount;
};

struct ShaderCreateInfo {
    const void* code;
    size_t codeSize;
    const char* entryPoint; // nullptr means "main"
};

struct SemaphoreCreateInfo {
    SemaphoreType type;
    uint64_t initialValue;
};

struct FenceCreateInfo {
    bool signaled; // true = create in signaled state
};

struct MemoryBarrier {
    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
};

struct BufferBarrier {
    class Buffer* buffer;
    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
    uint64_t offset;
    uint64_t size; // 0 means whole buffer
};

struct TextureBarrier {
    class Texture* texture;
    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
    VkImageLayout oldLayout;
    VkImageLayout newLayout;
    uint32_t baseMipLevel;
    uint32_t mipLevelCount;
    uint32_t baseArrayLayer;
    uint32_t arrayLayerCount;
};

struct Viewport {
    float x;
    float y;
    float width;
    float height;
    float minDepth;
    float maxDepth;
};

struct ScissorRect {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
};

struct SamplerCreateInfo {
    VkSamplerAddressMode addressModeU;
    VkSamplerAddressMode addressModeV;
    VkSamplerAddressMode addressModeW;
    VkFilter magFilter;
    VkFilter minFilter;
    VkSamplerMipmapMode mipmapMode;
    float lodMinClamp;
    float lodMaxClamp;
    uint32_t maxAnisotropy;
    VkCompareOp compareOp; // VK_COMPARE_OP_MAX_ENUM means no compare
};

// Forward declaration for complex types
struct BindGroupLayoutEntry {
    uint32_t binding;
    VkDescriptorType descriptorType;
    VkShaderStageFlags stageFlags;
};

struct BindGroupLayoutCreateInfo {
    std::vector<BindGroupLayoutEntry> entries;
};

struct BindGroupEntry {
    uint32_t binding;
    VkDescriptorType descriptorType;
    // Union-like storage for different resource types
    VkBuffer buffer;
    VkDeviceSize bufferOffset;
    VkDeviceSize bufferSize;
    VkSampler sampler;
    VkImageView imageView;
    VkImageLayout imageLayout;
};

struct BindGroupCreateInfo {
    VkDescriptorSetLayout layout; // From BindGroupLayout
    std::vector<BindGroupEntry> entries;
};

struct InstanceCreateInfo {
    bool enableValidation;
    const char* applicationName = "GfxWrapper Application";
    uint32_t applicationVersion = 1;
    std::vector<InstanceFeatureType> enabledFeatures;
};

struct AdapterCreateInfo {
    uint32_t adapterIndex = UINT32_MAX; // Adapter index (UINT32_MAX = use preference)
    DeviceTypePreference devicePreference = DeviceTypePreference::HighPerformance; // Only used when adapterIndex is UINT32_MAX
};

struct DeviceCreateInfo {
    float queuePriority = 1.0f;
    std::vector<DeviceFeatureType> enabledFeatures;
};

struct PlatformWindowHandle {
    // Platform-specific window handles (Vulkan native)
    enum class Platform {
        Unknown,
        Xlib,
        Xcb,
        Wayland,
        Win32,
        Metal,
        Android,
        Emscripten
    } platform;

    union {
        struct {
            void* display; // Display*
            unsigned long window; // Window
        } xlib;
        struct {
            void* connection; // xcb_connection_t*
            uint32_t window; // xcb_window_t
        } xcb;
        struct {
            void* display; // wl_display*
            void* surface; // wl_surface*
        } wayland;
        struct {
            void* hinstance; // HINSTANCE
            void* hwnd; // HWND
        } win32;
        struct {
            void* layer; // CAMetalLayer*
        } metal;
        struct {
            void* window; // ANativeWindow*
        } android;
        struct {
            const char* canvasSelector; // CSS selector for canvas element (e.g., "#canvas")
        } emscripten;
    } handle;
};

struct SurfaceCreateInfo {
    PlatformWindowHandle windowHandle;
};

struct SwapchainCreateInfo {
    uint32_t width;
    uint32_t height;
    VkFormat format;
    VkColorSpaceKHR colorSpace;
    VkPresentModeKHR presentMode;
    uint32_t imageCount;
};

// Pipeline CreateInfo structs - these are complex
struct VertexBufferLayout {
    uint64_t arrayStride;
    bool stepModeInstance;
    std::vector<VkVertexInputAttributeDescription> attributes;
};

struct VertexState {
    VkShaderModule module;
    const char* entryPoint;
    std::vector<VertexBufferLayout> buffers;
};

struct ColorTargetState {
    VkFormat format;
    VkColorComponentFlags writeMask;
    VkPipelineColorBlendAttachmentState blendState;
};

struct FragmentState {
    VkShaderModule module;
    const char* entryPoint;
    std::vector<ColorTargetState> targets;
};

struct PrimitiveState {
    VkPrimitiveTopology topology;
    VkPolygonMode polygonMode;
    VkCullModeFlags cullMode;
    VkFrontFace frontFace;
};

struct DepthStencilState {
    VkFormat format;
    bool depthWriteEnabled;
    VkCompareOp depthCompareOp;
};

struct RenderPipelineCreateInfo {
    VkRenderPass renderPass = VK_NULL_HANDLE; // Render pass this pipeline will be used with
    std::vector<VkDescriptorSetLayout> bindGroupLayouts;
    VertexState vertex;
    FragmentState fragment;
    PrimitiveState primitive;
    std::optional<DepthStencilState> depthStencil;
    VkSampleCountFlagBits sampleCount;
};

struct ComputePipelineCreateInfo {
    std::vector<VkDescriptorSetLayout> bindGroupLayouts;
    VkShaderModule module;
    const char* entryPoint;
};

// Color attachment target for render pass (main or resolve)
struct RenderPassColorAttachmentTarget {
    VkFormat format;
    VkSampleCountFlagBits sampleCount;
    VkAttachmentLoadOp loadOp;
    VkAttachmentStoreOp storeOp;
    VkImageLayout finalLayout;
};

// Color attachment with optional resolve target
struct RenderPassColorAttachment {
    RenderPassColorAttachmentTarget target;
    std::optional<RenderPassColorAttachmentTarget> resolveTarget;
};

// Depth/stencil attachment target for render pass (main or resolve)
struct RenderPassDepthStencilAttachmentTarget {
    VkFormat format;
    VkSampleCountFlagBits sampleCount;
    VkAttachmentLoadOp depthLoadOp;
    VkAttachmentStoreOp depthStoreOp;
    VkAttachmentLoadOp stencilLoadOp;
    VkAttachmentStoreOp stencilStoreOp;
    VkImageLayout finalLayout;
};

// Depth/stencil attachment with optional resolve target
struct RenderPassDepthStencilAttachment {
    RenderPassDepthStencilAttachmentTarget target;
    std::optional<RenderPassDepthStencilAttachmentTarget> resolveTarget;
};

struct RenderPassCreateInfo {
    std::vector<RenderPassColorAttachment> colorAttachments;
    std::optional<RenderPassDepthStencilAttachment> depthStencilAttachment;
};

struct FramebufferCreateInfo {
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkImageView> attachments; // Interleaved: [color0, resolve0, color1, resolve1, ..., depth, depthResolve]
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t colorAttachmentCount = 0; // Number of color attachments (not including resolves)
    bool hasDepthResolve = false;
};

struct RenderPassEncoderBeginInfo {
    std::vector<VkClearColorValue> colorClearValues;
    float depthClearValue;
    uint32_t stencilClearValue;
};

struct ComputePassEncoderCreateInfo {
    const char* label;
};

struct SubmitInfo {
    CommandEncoder** commandEncoders;
    uint32_t commandEncoderCount;
    Fence* signalFence;
    Semaphore** waitSemaphores;
    uint64_t* waitValues;
    uint32_t waitSemaphoreCount;
    Semaphore** signalSemaphores;
    uint64_t* signalValues;
    uint32_t signalSemaphoreCount;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_CREATEINFO_H