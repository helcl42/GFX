#pragma once

#include "../common/VulkanCommon.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace gfx::vulkan {

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

// ============================================================================
// Internal CreateInfo structs - pure Vulkan types, no GFX dependencies
// ============================================================================
struct BufferCreateInfo {
    size_t size;
    VkBufferUsageFlags usage;
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
    bool enableHeadless;
};

struct AdapterCreateInfo {
    DeviceTypePreference devicePreference = DeviceTypePreference::HighPerformance;
};

struct DeviceCreateInfo {
    // Currently Device doesn't use descriptor parameters
    // Placeholder for future extensibility
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
    VkSurfaceKHR surface;
    uint32_t queueFamily;
    uint32_t width;
    uint32_t height;
    VkFormat format;
    VkColorSpaceKHR colorSpace;
    VkPresentModeKHR presentMode;
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

} // namespace gfx::vulkan
