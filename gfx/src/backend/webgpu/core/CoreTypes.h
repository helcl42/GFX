#ifndef GFX_WEBGPU_CORE_TYPES_H
#define GFX_WEBGPU_CORE_TYPES_H

#include "../common/Common.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace gfx::backend::webgpu::core {

// Forward declarations for SubmitInfo
class CommandEncoder;
class Fence;
class Semaphore;
class TextureView;

// ============================================================================
// Internal Extension Names
// ============================================================================

namespace extensions {
    constexpr const char* SURFACE = "gfx_surface";
    constexpr const char* DEBUG = "gfx_debug";
    constexpr const char* SWAPCHAIN = "gfx_swapchain";
    constexpr const char* TIMELINE_SEMAPHORE = "gfx_timeline_semaphore";
    constexpr const char* ANISOTROPIC_FILTERING = "gfx_anisotropic_filtering";
} // namespace extensions

// ============================================================================
// Internal Type Definitions
// ============================================================================

// Internal semaphore type
enum class SemaphoreType {
    Binary,
    Timeline
};

// Internal shader source type
enum class ShaderSourceType {
    WGSL = 0,
    SPIRV = 1
};

// Queue family properties (WebGPU has single unified queue)
struct QueueFamilyProperties {
    uint32_t queueCount; // Always 1 for WebGPU
    bool supportsGraphics; // Always true
    bool supportsCompute; // Always true
    bool supportsTransfer; // Always true
};

// ============================================================================
// Internal CreateInfo structs - pure WebGPU types, no GFX dependencies
// ============================================================================

struct AdapterCreateInfo {
    uint32_t adapterIndex = UINT32_MAX; // Adapter index (UINT32_MAX = use preference)
    WGPUPowerPreference powerPreference;
    bool forceFallbackAdapter;
};

struct AdapterInfo {
    std::string name; // Device name (e.g., "NVIDIA GeForce RTX 4090")
    std::string driverDescription; // Driver description (may be NULL for WebGPU)
    uint32_t vendorID; // PCI vendor ID (0x1002=AMD, 0x10DE=NVIDIA, 0x8086=Intel, 0=Unknown)
    uint32_t deviceID; // PCI device ID (0=Unknown)
    WGPUAdapterType adapterType; // Discrete, Integrated, CPU, or Unknown
};

struct BufferCreateInfo {
    size_t size;
    WGPUBufferUsage usage;
    uint32_t memoryProperties; // Stored for API consistency (WebGPU doesn't use memory properties)
};

struct BufferImportInfo {
    size_t size;
    WGPUBufferUsage usage;
    uint32_t memoryProperties; // Stored for API consistency (WebGPU doesn't use memory properties)
};

struct BufferInfo {
    uint64_t size;
    WGPUBufferUsage usage;
    uint32_t memoryProperties; // Stored for API consistency (WebGPU doesn't use memory properties)
};

struct TextureInfo {
    WGPUTextureDimension dimension;
    WGPUExtent3D size;
    uint32_t arrayLayers;
    WGPUTextureFormat format;
    uint32_t mipLevels;
    uint32_t sampleCount;
    WGPUTextureUsage usage;
};

struct SurfaceInfo {
    uint32_t minImageCount;
    uint32_t maxImageCount;
    uint32_t minWidth;
    uint32_t minHeight;
    uint32_t maxWidth;
    uint32_t maxHeight;
};

struct SwapchainInfo {
    uint32_t width;
    uint32_t height;
    WGPUTextureFormat format;
    uint32_t imageCount;
    WGPUPresentMode presentMode;
};

struct TextureCreateInfo {
    WGPUTextureFormat format;
    WGPUExtent3D size;
    WGPUTextureUsage usage;
    uint32_t sampleCount;
    uint32_t mipLevelCount;
    WGPUTextureDimension dimension;
    uint32_t arrayLayers;
};

struct TextureImportInfo {
    WGPUTextureFormat format;
    WGPUExtent3D size;
    WGPUTextureUsage usage;
    uint32_t sampleCount;
    uint32_t mipLevelCount;
    WGPUTextureDimension dimension;
    uint32_t arrayLayers;
};

struct TextureViewCreateInfo {
    WGPUTextureViewDimension viewDimension;
    WGPUTextureFormat format; // WGPUTextureFormat_Undefined means use texture's format
    uint32_t baseMipLevel;
    uint32_t mipLevelCount;
    uint32_t baseArrayLayer;
    uint32_t arrayLayerCount;
};

struct ShaderCreateInfo {
    ShaderSourceType sourceType;
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

struct QuerySetCreateInfo {
    const char* label = nullptr;
    WGPUQueryType type = WGPUQueryType_Occlusion;
    uint32_t count = 0;
};

struct CommandEncoderCreateInfo {
    const char* label; // nullptr means no label
};

struct SubmitInfo {
    CommandEncoder** commandEncoders;
    uint32_t commandEncoderCount;
    Fence* signalFence;

    // Semaphores (stored but not used by WebGPU backend)
    Semaphore** waitSemaphores;
    uint64_t* waitValues;
    uint32_t waitSemaphoreCount;
    Semaphore** signalSemaphores;
    uint64_t* signalValues;
    uint32_t signalSemaphoreCount;
};

struct SamplerCreateInfo {
    WGPUAddressMode addressModeU;
    WGPUAddressMode addressModeV;
    WGPUAddressMode addressModeW;
    WGPUFilterMode magFilter;
    WGPUFilterMode minFilter;
    WGPUMipmapFilterMode mipmapFilter;
    float lodMinClamp;
    float lodMaxClamp;
    uint32_t maxAnisotropy;
    WGPUCompareFunction compareFunction;
};

struct InstanceCreateInfo {
    const char* applicationName = "Gfx Application";
    uint32_t applicationVersion = 1;
    std::vector<std::string> enabledExtensions;
};

struct DeviceCreateInfo {
    std::vector<std::string> enabledExtensions;
};

// Platform-specific window handles (WebGPU native)
struct PlatformWindowHandle {
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
            void* connection;
            uint32_t window;
        } xcb;
        struct {
            void* display;
            void* surface;
        } wayland;
        struct {
            void* hinstance;
            void* hwnd;
        } win32;
        struct {
            void* layer;
        } metal;
        struct {
            void* window;
        } android;
        struct {
            const char* canvasSelector;
        } emscripten;
    } handle;
};

struct SurfaceCreateInfo {
    PlatformWindowHandle windowHandle;
};

struct SwapchainCreateInfo {
    WGPUSurface surface;
    uint32_t width;
    uint32_t height;
    WGPUTextureFormat format;
    WGPUTextureUsage usage;
    WGPUPresentMode presentMode;
    uint32_t imageCount;
};

// Pipeline CreateInfo structs
struct BindGroupLayoutEntry {
    uint32_t binding;
    WGPUShaderStage visibility;

    // Buffer binding
    WGPUBufferBindingType bufferType;
    WGPUBool bufferHasDynamicOffset;
    uint64_t bufferMinBindingSize;

    // Sampler binding
    WGPUSamplerBindingType samplerType;

    // Texture binding
    WGPUTextureSampleType textureSampleType;
    WGPUTextureViewDimension textureViewDimension;
    WGPUBool textureMultisampled;

    // Storage texture binding
    WGPUStorageTextureAccess storageTextureAccess;
    WGPUTextureFormat storageTextureFormat;
    WGPUTextureViewDimension storageTextureViewDimension;
};

struct BindGroupLayoutCreateInfo {
    std::vector<BindGroupLayoutEntry> entries;
};

struct BindGroupEntry {
    uint32_t binding;
    WGPUBuffer buffer;
    uint64_t bufferOffset;
    uint64_t bufferSize;
    WGPUSampler sampler;
    WGPUTextureView textureView;
};

struct BindGroupCreateInfo {
    WGPUBindGroupLayout layout;
    std::vector<BindGroupEntry> entries;
};

struct VertexAttribute {
    WGPUVertexFormat format;
    uint64_t offset;
    uint32_t shaderLocation;
};

struct VertexBufferLayout {
    uint64_t arrayStride;
    WGPUVertexStepMode stepMode;
    std::vector<VertexAttribute> attributes;
};

struct VertexState {
    WGPUShaderModule module;
    const char* entryPoint;
    std::vector<VertexBufferLayout> buffers;
};

struct BlendComponent {
    WGPUBlendOperation operation;
    WGPUBlendFactor srcFactor;
    WGPUBlendFactor dstFactor;
};

struct BlendState {
    BlendComponent color;
    BlendComponent alpha;
};

struct ColorTargetState {
    WGPUTextureFormat format;
    WGPUColorWriteMask writeMask;
    std::optional<BlendState> blend;
};

struct FragmentState {
    WGPUShaderModule module;
    const char* entryPoint;
    std::vector<ColorTargetState> targets;
};

struct PrimitiveState {
    WGPUPrimitiveTopology topology;
    WGPUIndexFormat stripIndexFormat;
    WGPUFrontFace frontFace;
    WGPUCullMode cullMode;
};

struct StencilFaceState {
    WGPUCompareFunction compare;
    WGPUStencilOperation failOp;
    WGPUStencilOperation depthFailOp;
    WGPUStencilOperation passOp;
};

struct DepthStencilState {
    WGPUTextureFormat format;
    bool depthWriteEnabled;
    WGPUCompareFunction depthCompare;
    StencilFaceState stencilFront;
    StencilFaceState stencilBack;
    uint32_t stencilReadMask;
    uint32_t stencilWriteMask;
    int32_t depthBias;
    float depthBiasSlopeScale;
    float depthBiasClamp;
};

struct RenderPipelineCreateInfo {
    std::vector<WGPUBindGroupLayout> bindGroupLayouts;
    VertexState vertex;
    std::optional<FragmentState> fragment;
    PrimitiveState primitive;
    std::optional<DepthStencilState> depthStencil;
    uint32_t sampleCount;
};

struct ComputePipelineCreateInfo {
    std::vector<WGPUBindGroupLayout> bindGroupLayouts;
    WGPUShaderModule module;
    const char* entryPoint;
};

// Simplified color attachment info for RenderPass (ops only, no views)
struct RenderPassColorAttachment {
    WGPUTextureFormat format;
    WGPULoadOp loadOp;
    WGPUStoreOp storeOp;
};

// Simplified depth/stencil attachment info for RenderPass (ops only, no views)
struct RenderPassDepthStencilAttachment {
    WGPUTextureFormat format;
    WGPULoadOp depthLoadOp;
    WGPUStoreOp depthStoreOp;
    WGPULoadOp stencilLoadOp;
    WGPUStoreOp stencilStoreOp;
};

struct RenderPassCreateInfo {
    std::vector<RenderPassColorAttachment> colorAttachments;
    std::optional<RenderPassDepthStencilAttachment> depthStencilAttachment;
};

struct FramebufferCreateInfo {
    std::vector<TextureView*> colorAttachmentViews; // Pointers to TextureView objects
    std::vector<TextureView*> colorResolveTargetViews; // Optional resolve targets for MSAA
    TextureView* depthStencilAttachmentView = nullptr;
    TextureView* depthStencilResolveTargetView = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct RenderPassEncoderBeginInfo {
    std::vector<WGPUColor> colorClearValues;
    float depthClearValue;
    uint32_t stencilClearValue;
};

struct ComputePassEncoderCreateInfo {
    const char* label;
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_CREATEINFO_H