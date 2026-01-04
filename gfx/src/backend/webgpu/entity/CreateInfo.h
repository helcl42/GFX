#pragma once

#include "../common/WebGPUCommon.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>
#include <webgpu/webgpu.h>

namespace gfx::webgpu {

// Forward declarations for SubmitInfo
class CommandEncoder;
class Fence;
class Semaphore;

// ============================================================================
// Internal Type Definitions
// ============================================================================

// Internal buffer usage flags (WebGPU native)
using BufferUsage = WGPUBufferUsage;

// Internal semaphore type
enum class SemaphoreType {
    Binary,
    Timeline
};

// ============================================================================
// Internal CreateInfo structs - pure WebGPU types, no GFX dependencies
// ============================================================================

struct AdapterCreateInfo {
    WGPUPowerPreference powerPreference;
    bool forceFallbackAdapter;
};

struct BufferCreateInfo {
    size_t size;
    WGPUBufferUsage usage;
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

struct TextureViewCreateInfo {
    WGPUTextureViewDimension viewDimension;
    WGPUTextureFormat format; // WGPUTextureFormat_Undefined means use texture's format
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
    bool enableValidation;
};

struct DeviceCreateInfo {
    // Currently Device doesn't use descriptor parameters
    // Placeholder for future extensibility
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
    uint32_t bufferCount;
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

} // namespace gfx::webgpu
