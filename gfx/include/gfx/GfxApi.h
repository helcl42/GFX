#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ============================================================================
// DLL Export/Import Macros for Windows
// ============================================================================

#if defined(_WIN32) || defined(__CYGWIN__)
#ifdef GFX_BUILDING_DLL
// Building the DLL - export symbols
#ifdef __GNUC__
#define GFX_API __attribute__((dllexport))
#else
#define GFX_API __declspec(dllexport)
#endif
#else
// Using the DLL - import symbols
#ifdef __GNUC__
#define GFX_API __attribute__((dllimport))
#else
#define GFX_API __declspec(dllimport)
#endif
#endif
#else
// Non-Windows platforms - use visibility attributes for shared libraries
#if defined(__GNUC__) && __GNUC__ >= 4
#define GFX_API __attribute__((visibility("default")))
#else
#define GFX_API
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Core Enumerations
// ============================================================================

typedef enum {
    GFX_BACKEND_VULKAN,
    GFX_BACKEND_WEBGPU,
    GFX_BACKEND_AUTO
} GfxBackend;

typedef enum {
    GFX_POWER_PREFERENCE_UNDEFINED,
    GFX_POWER_PREFERENCE_LOW_POWER,
    GFX_POWER_PREFERENCE_HIGH_PERFORMANCE
} GfxPowerPreference;

typedef enum {
    GFX_PRESENT_MODE_IMMEDIATE,
    GFX_PRESENT_MODE_FIFO,
    GFX_PRESENT_MODE_FIFO_RELAXED,
    GFX_PRESENT_MODE_MAILBOX
} GfxPresentMode;

typedef enum {
    GFX_PRIMITIVE_TOPOLOGY_POINT_LIST,
    GFX_PRIMITIVE_TOPOLOGY_LINE_LIST,
    GFX_PRIMITIVE_TOPOLOGY_LINE_STRIP,
    GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
} GfxPrimitiveTopology;

typedef enum {
    GFX_FRONT_FACE_COUNTER_CLOCKWISE,
    GFX_FRONT_FACE_CLOCKWISE
} GfxFrontFace;

typedef enum {
    GFX_CULL_MODE_NONE,
    GFX_CULL_MODE_FRONT,
    GFX_CULL_MODE_BACK,
    GFX_CULL_MODE_FRONT_AND_BACK
} GfxCullMode;

typedef enum {
    GFX_POLYGON_MODE_FILL,
    GFX_POLYGON_MODE_LINE,
    GFX_POLYGON_MODE_POINT
} GfxPolygonMode;

typedef enum {
    GFX_INDEX_FORMAT_UINT16,
    GFX_INDEX_FORMAT_UINT32
} GfxIndexFormat;

typedef enum {
    GFX_TEXTURE_FORMAT_UNDEFINED,
    GFX_TEXTURE_FORMAT_R8_UNORM,
    GFX_TEXTURE_FORMAT_R8G8_UNORM,
    GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM,
    GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM_SRGB,
    GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM,
    GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB,
    GFX_TEXTURE_FORMAT_R16_FLOAT,
    GFX_TEXTURE_FORMAT_R16G16_FLOAT,
    GFX_TEXTURE_FORMAT_R16G16B16A16_FLOAT,
    GFX_TEXTURE_FORMAT_R32_FLOAT,
    GFX_TEXTURE_FORMAT_R32G32_FLOAT,
    GFX_TEXTURE_FORMAT_R32G32B32_FLOAT,
    GFX_TEXTURE_FORMAT_R32G32B32A32_FLOAT,
    GFX_TEXTURE_FORMAT_DEPTH16_UNORM,
    GFX_TEXTURE_FORMAT_DEPTH24_PLUS,
    GFX_TEXTURE_FORMAT_DEPTH32_FLOAT,
    GFX_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8,
    GFX_TEXTURE_FORMAT_DEPTH32_FLOAT_STENCIL8
} GfxTextureFormat;

typedef enum {
    GFX_TEXTURE_TYPE_1D,
    GFX_TEXTURE_TYPE_2D,
    GFX_TEXTURE_TYPE_3D,
    GFX_TEXTURE_TYPE_CUBE
} GfxTextureType;

typedef enum {
    GFX_TEXTURE_VIEW_TYPE_1D,
    GFX_TEXTURE_VIEW_TYPE_2D,
    GFX_TEXTURE_VIEW_TYPE_3D,
    GFX_TEXTURE_VIEW_TYPE_CUBE,
    GFX_TEXTURE_VIEW_TYPE_1D_ARRAY,
    GFX_TEXTURE_VIEW_TYPE_2D_ARRAY,
    GFX_TEXTURE_VIEW_TYPE_CUBE_ARRAY
} GfxTextureViewType;

typedef enum {
    GFX_TEXTURE_SAMPLE_TYPE_FLOAT,
    GFX_TEXTURE_SAMPLE_TYPE_UNFILTERABLE_FLOAT,
    GFX_TEXTURE_SAMPLE_TYPE_DEPTH,
    GFX_TEXTURE_SAMPLE_TYPE_SINT,
    GFX_TEXTURE_SAMPLE_TYPE_UINT
} GfxTextureSampleType;

typedef enum {
    GFX_TEXTURE_USAGE_NONE = 0,
    GFX_TEXTURE_USAGE_COPY_SRC = 1 << 0,
    GFX_TEXTURE_USAGE_COPY_DST = 1 << 1,
    GFX_TEXTURE_USAGE_TEXTURE_BINDING = 1 << 2,
    GFX_TEXTURE_USAGE_STORAGE_BINDING = 1 << 3,
    GFX_TEXTURE_USAGE_RENDER_ATTACHMENT = 1 << 4
} GfxTextureUsage;

typedef enum {
    GFX_TEXTURE_LAYOUT_UNDEFINED, // Initial layout, contents undefined
    GFX_TEXTURE_LAYOUT_GENERAL, // General purpose, can be used for anything but may be slow
    GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT, // Optimal for color render target
    GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT, // Optimal for depth/stencil render target
    GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY, // Optimal for reading depth/stencil
    GFX_TEXTURE_LAYOUT_SHADER_READ_ONLY, // Optimal for sampling in shaders
    GFX_TEXTURE_LAYOUT_TRANSFER_SRC, // Optimal for copy source
    GFX_TEXTURE_LAYOUT_TRANSFER_DST, // Optimal for copy destination
    GFX_TEXTURE_LAYOUT_PRESENT_SRC // Optimal for presentation
} GfxTextureLayout;

typedef enum {
    GFX_PIPELINE_STAGE_NONE = 0,
    GFX_PIPELINE_STAGE_TOP_OF_PIPE = 0x00000001,
    GFX_PIPELINE_STAGE_DRAW_INDIRECT = 0x00000002,
    GFX_PIPELINE_STAGE_VERTEX_INPUT = 0x00000004,
    GFX_PIPELINE_STAGE_VERTEX_SHADER = 0x00000008,
    GFX_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER = 0x00000010,
    GFX_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER = 0x00000020,
    GFX_PIPELINE_STAGE_GEOMETRY_SHADER = 0x00000040,
    GFX_PIPELINE_STAGE_FRAGMENT_SHADER = 0x00000080,
    GFX_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS = 0x00000100,
    GFX_PIPELINE_STAGE_LATE_FRAGMENT_TESTS = 0x00000200,
    GFX_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT = 0x00000400,
    GFX_PIPELINE_STAGE_COMPUTE_SHADER = 0x00000800,
    GFX_PIPELINE_STAGE_TRANSFER = 0x00001000,
    GFX_PIPELINE_STAGE_BOTTOM_OF_PIPE = 0x00002000,
    GFX_PIPELINE_STAGE_ALL_GRAPHICS = 0x0000FFFF,
    GFX_PIPELINE_STAGE_ALL_COMMANDS = 0x00010000
} GfxPipelineStage;

typedef enum {
    GFX_ACCESS_NONE = 0,
    GFX_ACCESS_INDIRECT_COMMAND_READ = 1 << 0, // 0x01
    GFX_ACCESS_INDEX_READ = 1 << 1, // 0x02
    GFX_ACCESS_VERTEX_ATTRIBUTE_READ = 1 << 2, // 0x04
    GFX_ACCESS_UNIFORM_READ = 1 << 3, // 0x08
    GFX_ACCESS_INPUT_ATTACHMENT_READ = 1 << 4, // 0x10
    GFX_ACCESS_SHADER_READ = 1 << 5, // 0x20
    GFX_ACCESS_SHADER_WRITE = 1 << 6, // 0x40
    GFX_ACCESS_COLOR_ATTACHMENT_READ = 1 << 7, // 0x80
    GFX_ACCESS_COLOR_ATTACHMENT_WRITE = 1 << 8, // 0x100
    GFX_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ = 1 << 9, // 0x200
    GFX_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE = 1 << 10, // 0x400
    GFX_ACCESS_TRANSFER_READ = 1 << 11, // 0x800
    GFX_ACCESS_TRANSFER_WRITE = 1 << 12, // 0x1000
    GFX_ACCESS_MEMORY_READ = 1 << 14, // 0x4000
    GFX_ACCESS_MEMORY_WRITE = 1 << 15 // 0x8000
} GfxAccessFlags;

typedef enum {
    GFX_BUFFER_USAGE_NONE = 0,
    GFX_BUFFER_USAGE_MAP_READ = 1 << 0,
    GFX_BUFFER_USAGE_MAP_WRITE = 1 << 1,
    GFX_BUFFER_USAGE_COPY_SRC = 1 << 2,
    GFX_BUFFER_USAGE_COPY_DST = 1 << 3,
    GFX_BUFFER_USAGE_INDEX = 1 << 4,
    GFX_BUFFER_USAGE_VERTEX = 1 << 5,
    GFX_BUFFER_USAGE_UNIFORM = 1 << 6,
    GFX_BUFFER_USAGE_STORAGE = 1 << 7,
    GFX_BUFFER_USAGE_INDIRECT = 1 << 8
} GfxBufferUsage;

typedef enum {
    GFX_SHADER_STAGE_NONE = 0,
    GFX_SHADER_STAGE_VERTEX = 1 << 0,
    GFX_SHADER_STAGE_FRAGMENT = 1 << 1,
    GFX_SHADER_STAGE_COMPUTE = 1 << 2
} GfxShaderStage;

typedef enum {
    GFX_FILTER_MODE_NEAREST,
    GFX_FILTER_MODE_LINEAR
} GfxFilterMode;

typedef enum {
    GFX_ADDRESS_MODE_REPEAT,
    GFX_ADDRESS_MODE_MIRROR_REPEAT,
    GFX_ADDRESS_MODE_CLAMP_TO_EDGE
} GfxAddressMode;

typedef enum {
    GFX_COMPARE_FUNCTION_NEVER,
    GFX_COMPARE_FUNCTION_LESS,
    GFX_COMPARE_FUNCTION_EQUAL,
    GFX_COMPARE_FUNCTION_LESS_EQUAL,
    GFX_COMPARE_FUNCTION_GREATER,
    GFX_COMPARE_FUNCTION_NOT_EQUAL,
    GFX_COMPARE_FUNCTION_GREATER_EQUAL,
    GFX_COMPARE_FUNCTION_ALWAYS
} GfxCompareFunction;

typedef enum {
    GFX_BLEND_OPERATION_ADD,
    GFX_BLEND_OPERATION_SUBTRACT,
    GFX_BLEND_OPERATION_REVERSE_SUBTRACT,
    GFX_BLEND_OPERATION_MIN,
    GFX_BLEND_OPERATION_MAX
} GfxBlendOperation;

typedef enum {
    GFX_BLEND_FACTOR_ZERO,
    GFX_BLEND_FACTOR_ONE,
    GFX_BLEND_FACTOR_SRC,
    GFX_BLEND_FACTOR_ONE_MINUS_SRC,
    GFX_BLEND_FACTOR_SRC_ALPHA,
    GFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    GFX_BLEND_FACTOR_DST,
    GFX_BLEND_FACTOR_ONE_MINUS_DST,
    GFX_BLEND_FACTOR_DST_ALPHA,
    GFX_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
    GFX_BLEND_FACTOR_SRC_ALPHA_SATURATED,
    GFX_BLEND_FACTOR_CONSTANT,
    GFX_BLEND_FACTOR_ONE_MINUS_CONSTANT
} GfxBlendFactor;

typedef enum {
    GFX_STENCIL_OPERATION_KEEP,
    GFX_STENCIL_OPERATION_ZERO,
    GFX_STENCIL_OPERATION_REPLACE,
    GFX_STENCIL_OPERATION_INCREMENT_CLAMP,
    GFX_STENCIL_OPERATION_DECREMENT_CLAMP,
    GFX_STENCIL_OPERATION_INVERT,
    GFX_STENCIL_OPERATION_INCREMENT_WRAP,
    GFX_STENCIL_OPERATION_DECREMENT_WRAP
} GfxStencilOperation;

typedef enum {
    GFX_SAMPLE_COUNT_1 = 1,
    GFX_SAMPLE_COUNT_2 = 2,
    GFX_SAMPLE_COUNT_4 = 4,
    GFX_SAMPLE_COUNT_8 = 8,
    GFX_SAMPLE_COUNT_16 = 16,
    GFX_SAMPLE_COUNT_32 = 32,
    GFX_SAMPLE_COUNT_64 = 64
} GfxSampleCount;

// Result codes
typedef enum GfxResult {
    GFX_RESULT_SUCCESS = 0,

    // General errors
    GFX_RESULT_ERROR_INVALID_PARAMETER,
    GFX_RESULT_ERROR_OUT_OF_MEMORY,
    GFX_RESULT_ERROR_DEVICE_LOST,
    GFX_RESULT_ERROR_SURFACE_LOST,
    GFX_RESULT_ERROR_OUT_OF_DATE,

    // Operation-specific errors
    GFX_RESULT_TIMEOUT,
    GFX_RESULT_NOT_READY,

    // Backend errors
    GFX_RESULT_ERROR_BACKEND_NOT_LOADED,
    GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED,

    // Unknown/generic
    GFX_RESULT_ERROR_UNKNOWN
} GfxResult;

// ============================================================================
// Forward Declarations (Opaque Handles)
// ============================================================================

typedef struct GfxInstance_T* GfxInstance;
typedef struct GfxAdapter_T* GfxAdapter;
typedef struct GfxDevice_T* GfxDevice;
typedef struct GfxQueue_T* GfxQueue;
typedef struct GfxBuffer_T* GfxBuffer;
typedef struct GfxTexture_T* GfxTexture;
typedef struct GfxTextureView_T* GfxTextureView;
typedef struct GfxSampler_T* GfxSampler;
typedef struct GfxShader_T* GfxShader;
typedef struct GfxRenderPipeline_T* GfxRenderPipeline;
typedef struct GfxComputePipeline_T* GfxComputePipeline;
typedef struct GfxCommandEncoder_T* GfxCommandEncoder;
typedef struct GfxRenderPassEncoder_T* GfxRenderPassEncoder;
typedef struct GfxComputePassEncoder_T* GfxComputePassEncoder;
typedef struct GfxBindGroup_T* GfxBindGroup;
typedef struct GfxBindGroupLayout_T* GfxBindGroupLayout;
typedef struct GfxSurface_T* GfxSurface;
typedef struct GfxSwapchain_T* GfxSwapchain;
typedef struct GfxFence_T* GfxFence;
typedef struct GfxSemaphore_T* GfxSemaphore;

// ============================================================================
// Debug Message Types
// ============================================================================

typedef enum {
    GFX_DEBUG_MESSAGE_SEVERITY_VERBOSE = 0,
    GFX_DEBUG_MESSAGE_SEVERITY_INFO = 1,
    GFX_DEBUG_MESSAGE_SEVERITY_WARNING = 2,
    GFX_DEBUG_MESSAGE_SEVERITY_ERROR = 3
} GfxDebugMessageSeverity;

typedef enum {
    GFX_DEBUG_MESSAGE_TYPE_GENERAL = 0,
    GFX_DEBUG_MESSAGE_TYPE_VALIDATION = 1,
    GFX_DEBUG_MESSAGE_TYPE_PERFORMANCE = 2
} GfxDebugMessageType;

// ============================================================================
// Debug Callback
// ============================================================================

typedef void (*GfxDebugCallback)(
    GfxDebugMessageSeverity severity,
    GfxDebugMessageType type,
    const char* message,
    void* userData);

// ============================================================================
// Core Structures
// ============================================================================

typedef struct {
    float r;
    float g;
    float b;
    float a;
} GfxColor;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
} GfxExtent3D;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t z;
} GfxOrigin3D;

typedef struct {
    float x;
    float y;
    float width;
    float height;
    float minDepth;
    float maxDepth;
} GfxViewport;

typedef struct {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
} GfxScissorRect;

typedef struct {
    GfxTexture texture;
    GfxTextureLayout oldLayout;
    GfxTextureLayout newLayout;
    GfxPipelineStage srcStageMask;
    GfxPipelineStage dstStageMask;
    GfxAccessFlags srcAccessMask;
    GfxAccessFlags dstAccessMask;
    uint32_t baseMipLevel;
    uint32_t mipLevelCount;
    uint32_t baseArrayLayer;
    uint32_t arrayLayerCount;
} GfxTextureBarrier;

// ============================================================================
// Platform Abstraction
// ============================================================================

// Common windowing system enum for all platforms
typedef enum {
    GFX_WINDOWING_SYSTEM_WIN32,
    GFX_WINDOWING_SYSTEM_X11,
    GFX_WINDOWING_SYSTEM_WAYLAND,
    GFX_WINDOWING_SYSTEM_XCB,
    GFX_WINDOWING_SYSTEM_COCOA
} GfxWindowingSystem;

// Common platform window handle struct with union for all windowing systems
typedef struct {
    GfxWindowingSystem windowingSystem;
    union {
        struct {
            void* hwnd; // HWND - Window handle
            void* hinstance; // HINSTANCE - Application instance
        } win32;
        struct {
            void* window; // Window
            void* display; // Display*
        } x11;
        struct {
            void* surface; // wl_surface*
            void* display; // wl_display*
        } wayland;
        struct {
            void* connection; // xcb_connection_t*
            uint32_t window; // xcb_window_t
        } xcb;
        struct {
            void* nsWindow; // NSWindow*
            void* metalLayer; // CAMetalLayer* (optional)
        } cocoa;
    };
} GfxPlatformWindowHandle;

// ============================================================================
// Synchronization Enumerations
// ============================================================================

typedef enum {
    GFX_FENCE_STATUS_UNSIGNALED,
    GFX_FENCE_STATUS_SIGNALED,
    GFX_FENCE_STATUS_ERROR
} GfxFenceStatus;

typedef enum {
    GFX_SEMAPHORE_TYPE_BINARY,
    GFX_SEMAPHORE_TYPE_TIMELINE
} GfxSemaphoreType;

// ============================================================================
// Descriptor Structures
// ============================================================================

typedef struct {
    GfxBackend backend;
    bool enableValidation;
    bool enabledHeadless;
    const char* applicationName;
    uint32_t applicationVersion;
    const char** requiredExtensions;
    uint32_t requiredExtensionCount;
} GfxInstanceDescriptor;

// - list required features Graphics/Compute/Present
// - pass device override index in ??
typedef struct {
    GfxPowerPreference powerPreference;
    bool forceFallbackAdapter;
} GfxAdapterDescriptor;

// Device limits
typedef struct {
    uint32_t minUniformBufferOffsetAlignment;
    uint32_t minStorageBufferOffsetAlignment;
    uint32_t maxUniformBufferBindingSize;
    uint32_t maxStorageBufferBindingSize;
    uint64_t maxBufferSize;
    uint32_t maxTextureDimension1D;
    uint32_t maxTextureDimension2D;
    uint32_t maxTextureDimension3D;
    uint32_t maxTextureArrayLayers;
} GfxDeviceLimits;

// - query presentable device when not headless
// - pass device override index in
// - handle heaadless
// - additional extensions
typedef struct {
    const char* label;
    const char** requiredFeatures;
    uint32_t requiredFeatureCount;
} GfxDeviceDescriptor;

typedef struct {
    const char* label;
    GfxPlatformWindowHandle windowHandle;
} GfxSurfaceDescriptor;

typedef struct {
    const char* label;
    uint32_t width;
    uint32_t height;
    GfxTextureFormat format;
    GfxTextureUsage usage;
    GfxPresentMode presentMode;
    uint32_t bufferCount;
} GfxSwapchainDescriptor;

typedef struct {
    const char* label;
    uint64_t size;
    GfxBufferUsage usage;
    bool mappedAtCreation;
} GfxBufferDescriptor;

typedef struct {
    const char* label;
    GfxTextureType type;
    GfxExtent3D size;
    uint32_t arrayLayerCount;
    uint32_t mipLevelCount;
    GfxSampleCount sampleCount;
    GfxTextureFormat format;
    GfxTextureUsage usage;
} GfxTextureDescriptor;

typedef struct {
    const char* label;
    GfxTextureViewType viewType;
    GfxTextureFormat format;
    uint32_t baseMipLevel;
    uint32_t mipLevelCount;
    uint32_t baseArrayLayer;
    uint32_t arrayLayerCount;
} GfxTextureViewDescriptor;

typedef struct {
    const char* label;
    GfxAddressMode addressModeU;
    GfxAddressMode addressModeV;
    GfxAddressMode addressModeW;
    GfxFilterMode magFilter;
    GfxFilterMode minFilter;
    GfxFilterMode mipmapFilter;
    float lodMinClamp;
    float lodMaxClamp;
    GfxCompareFunction* compare; // NULL if not used
    uint16_t maxAnisotropy;
} GfxSamplerDescriptor;

typedef struct {
    const char* label;
    const void* code; // Shader code - can be WGSL source (const char*) or SPIR-V binary (uint32_t*)
    size_t codeSize; // Size in bytes - for SPIR-V binary, or 0 for null-terminated WGSL string
    const char* entryPoint;
} GfxShaderDescriptor;

typedef struct {
    GfxBlendOperation operation;
    GfxBlendFactor srcFactor;
    GfxBlendFactor dstFactor;
} GfxBlendComponent;

typedef struct {
    GfxBlendComponent color;
    GfxBlendComponent alpha;
} GfxBlendState;

typedef struct {
    GfxTextureFormat format;
    GfxBlendState* blend; // NULL if not used
    uint32_t writeMask;
} GfxColorTargetState;

typedef struct {
    GfxTextureFormat format;
    uint64_t offset;
    uint32_t shaderLocation;
} GfxVertexAttribute;

typedef struct {
    uint64_t arrayStride;
    const GfxVertexAttribute* attributes;
    uint32_t attributeCount;
    bool stepModeInstance;
} GfxVertexBufferLayout;

typedef struct {
    GfxShader module;
    const char* entryPoint;
    const GfxVertexBufferLayout* buffers;
    uint32_t bufferCount;
} GfxVertexState;

typedef struct {
    GfxShader module;
    const char* entryPoint;
    const GfxColorTargetState* targets;
    uint32_t targetCount;
} GfxFragmentState;

typedef struct {
    GfxPrimitiveTopology topology;
    GfxIndexFormat* stripIndexFormat; // NULL if not used
    GfxFrontFace frontFace;
    GfxCullMode cullMode;
    GfxPolygonMode polygonMode;
} GfxPrimitiveState;

typedef struct {
    GfxCompareFunction compare;
    GfxStencilOperation failOp;
    GfxStencilOperation depthFailOp;
    GfxStencilOperation passOp;
} GfxStencilFaceState;

typedef struct {
    GfxTextureFormat format;
    bool depthWriteEnabled;
    GfxCompareFunction depthCompare;
    GfxStencilFaceState stencilFront;
    GfxStencilFaceState stencilBack;
    uint32_t stencilReadMask;
    uint32_t stencilWriteMask;
    int32_t depthBias;
    float depthBiasSlopeScale;
    float depthBiasClamp;
} GfxDepthStencilState;

typedef struct {
    const char* label;
    GfxVertexState* vertex;
    GfxFragmentState* fragment; // NULL if not used
    GfxPrimitiveState* primitive;
    GfxDepthStencilState* depthStencil; // NULL if not used
    GfxSampleCount sampleCount;
    // Bind group layouts for the pipeline
    GfxBindGroupLayout* bindGroupLayouts;
    uint32_t bindGroupLayoutCount;
} GfxRenderPipelineDescriptor;

typedef struct {
    const char* label;
    GfxShader compute;
    const char* entryPoint;
    // Bind group layouts for the pipeline
    GfxBindGroupLayout* bindGroupLayouts;
    uint32_t bindGroupLayoutCount;
} GfxComputePipelineDescriptor;

typedef enum {
    GFX_BIND_GROUP_ENTRY_TYPE_BUFFER,
    GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER,
    GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW
} GfxBindGroupEntryType;

typedef enum {
    GFX_BINDING_TYPE_BUFFER,
    GFX_BINDING_TYPE_SAMPLER,
    GFX_BINDING_TYPE_TEXTURE,
    GFX_BINDING_TYPE_STORAGE_TEXTURE
} GfxBindingType;

typedef struct {
    uint32_t binding;
    GfxShaderStage visibility;
    GfxBindingType type; // Explicitly specify the binding type

    // Resource type - use type field to determine which is valid
    struct {
        bool hasDynamicOffset;
        uint64_t minBindingSize;
    } buffer;

    struct {
        bool comparison;
    } sampler;

    struct {
        GfxTextureSampleType sampleType;
        GfxTextureViewType viewDimension;
        bool multisampled;
    } texture;

    struct {
        GfxTextureFormat format;
        GfxTextureViewType viewDimension;
        bool writeOnly;
    } storageTexture;
} GfxBindGroupLayoutEntry;

typedef struct {
    const char* label;
    const GfxBindGroupLayoutEntry* entries;
    uint32_t entryCount;
} GfxBindGroupLayoutDescriptor;

typedef struct {
    uint32_t binding;
    GfxBindGroupEntryType type;
    union {
        struct {
            GfxBuffer buffer;
            uint64_t offset;
            uint64_t size;
        } buffer;
        GfxSampler sampler;
        GfxTextureView textureView;
    } resource;
} GfxBindGroupEntry;

typedef struct {
    const char* label;
    GfxBindGroupLayout layout;
    const GfxBindGroupEntry* entries;
    uint32_t entryCount;
} GfxBindGroupDescriptor;

typedef struct {
    const char* label;
    bool signaled; // Initial state - true for signaled, false for unsignaled
} GfxFenceDescriptor;

typedef struct {
    const char* label;
    GfxSemaphoreType type;
    uint64_t initialValue; // For timeline semaphores, ignored for binary
} GfxSemaphoreDescriptor;

typedef struct {
    GfxCommandEncoder* commandEncoders;
    uint32_t commandEncoderCount;

    // Wait semaphores (must be signaled before execution)
    GfxSemaphore* waitSemaphores;
    uint64_t* waitValues; // For timeline semaphores, NULL for binary
    uint32_t waitSemaphoreCount;

    // Signal semaphores (will be signaled after execution)
    GfxSemaphore* signalSemaphores;
    uint64_t* signalValues; // For timeline semaphores, NULL for binary
    uint32_t signalSemaphoreCount;

    // Optional fence to signal when all commands complete
    GfxFence signalFence;
} GfxSubmitInfo;

typedef struct {
    // Wait semaphores (rendering must complete before present)
    GfxSemaphore* waitSemaphores;
    uint32_t waitSemaphoreCount;
} GfxPresentInfo;

// ============================================================================
// API Functions
// ============================================================================

// Backend loading/unloading functions
// These should be called before creating any instances
// Call gfxLoadBackend or gfxLoadAllBackends at application startup
// Call gfxUnloadBackend or gfxUnloadAllBackends at application shutdown
GFX_API bool gfxLoadBackend(GfxBackend backend);
GFX_API void gfxUnloadBackend(GfxBackend backend);
GFX_API bool gfxLoadAllBackends(void);
GFX_API void gfxUnloadAllBackends(void);

// Instance functions
GFX_API GfxResult gfxCreateInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance);
GFX_API void gfxInstanceDestroy(GfxInstance instance);
GFX_API void gfxInstanceSetDebugCallback(GfxInstance instance, GfxDebugCallback callback, void* userData);
GFX_API GfxResult gfxInstanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter);
GFX_API uint32_t gfxInstanceEnumerateAdapters(GfxInstance instance, GfxAdapter* adapters, uint32_t maxAdapters);

// Adapter functions
GFX_API void gfxAdapterDestroy(GfxAdapter adapter);
GFX_API GfxResult gfxAdapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice);
GFX_API const char* gfxAdapterGetName(GfxAdapter adapter);
GFX_API GfxBackend gfxAdapterGetBackend(GfxAdapter adapter);

// Device functions
GFX_API void gfxDeviceDestroy(GfxDevice device);
GFX_API GfxQueue gfxDeviceGetQueue(GfxDevice device);
GFX_API GfxResult gfxDeviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface);
GFX_API GfxResult gfxDeviceCreateSwapchain(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain);
GFX_API GfxResult gfxDeviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer);
GFX_API GfxResult gfxDeviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture);
GFX_API GfxResult gfxDeviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler);
GFX_API GfxResult gfxDeviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader);
GFX_API GfxResult gfxDeviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout);
GFX_API GfxResult gfxDeviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup);
GFX_API GfxResult gfxDeviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline);
GFX_API GfxResult gfxDeviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline);
GFX_API GfxResult gfxDeviceCreateCommandEncoder(GfxDevice device, const char* label, GfxCommandEncoder* outEncoder);
GFX_API GfxResult gfxDeviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence);
GFX_API GfxResult gfxDeviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore);
GFX_API void gfxDeviceWaitIdle(GfxDevice device);
GFX_API void gfxDeviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits);

// Alignment helper functions
// Use these to align buffer offsets/sizes to device requirements:
// Example: uint64_t alignedOffset = gfxAlignUp(offset, limits.minUniformBufferOffsetAlignment);
GFX_API uint64_t gfxAlignUp(uint64_t value, uint64_t alignment);
GFX_API uint64_t gfxAlignDown(uint64_t value, uint64_t alignment);

// Surface functions
GFX_API void gfxSurfaceDestroy(GfxSurface surface);
GFX_API uint32_t gfxSurfaceGetSupportedFormats(GfxSurface surface, GfxTextureFormat* formats, uint32_t maxFormats);
GFX_API uint32_t gfxSurfaceGetSupportedPresentModes(GfxSurface surface, GfxPresentMode* presentModes, uint32_t maxModes);
GFX_API GfxPlatformWindowHandle gfxSurfaceGetPlatformHandle(GfxSurface surface);

// Swapchain functions
GFX_API void gfxSwapchainDestroy(GfxSwapchain swapchain);
GFX_API uint32_t gfxSwapchainGetWidth(GfxSwapchain swapchain);
GFX_API uint32_t gfxSwapchainGetHeight(GfxSwapchain swapchain);
GFX_API GfxTextureFormat gfxSwapchainGetFormat(GfxSwapchain swapchain);
GFX_API uint32_t gfxSwapchainGetBufferCount(GfxSwapchain swapchain);
GFX_API GfxResult gfxSwapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs,
    GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex);
GFX_API GfxTextureView gfxSwapchainGetImageView(GfxSwapchain swapchain, uint32_t imageIndex);
GFX_API GfxTextureView gfxSwapchainGetCurrentTextureView(GfxSwapchain swapchain);
GFX_API GfxResult gfxSwapchainPresent(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo);

// Buffer functions
GFX_API void gfxBufferDestroy(GfxBuffer buffer);
GFX_API uint64_t gfxBufferGetSize(GfxBuffer buffer);
GFX_API GfxBufferUsage gfxBufferGetUsage(GfxBuffer buffer);
GFX_API GfxResult gfxBufferMapAsync(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer);
GFX_API void gfxBufferUnmap(GfxBuffer buffer);

// Texture functions
GFX_API void gfxTextureDestroy(GfxTexture texture);
GFX_API GfxExtent3D gfxTextureGetSize(GfxTexture texture);
GFX_API GfxTextureFormat gfxTextureGetFormat(GfxTexture texture);
GFX_API uint32_t gfxTextureGetMipLevelCount(GfxTexture texture);
GFX_API GfxSampleCount gfxTextureGetSampleCount(GfxTexture texture);
GFX_API GfxTextureUsage gfxTextureGetUsage(GfxTexture texture);
GFX_API GfxTextureLayout gfxTextureGetLayout(GfxTexture texture);
GFX_API GfxResult gfxTextureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView);

// TextureView functions
GFX_API void gfxTextureViewDestroy(GfxTextureView textureView);

// Sampler functions
GFX_API void gfxSamplerDestroy(GfxSampler sampler);

// Shader functions
GFX_API void gfxShaderDestroy(GfxShader shader);

// BindGroupLayout functions
GFX_API void gfxBindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout);

// BindGroup functions
GFX_API void gfxBindGroupDestroy(GfxBindGroup);

// RenderPipeline functions
GFX_API void gfxRenderPipelineDestroy(GfxRenderPipeline renderPipeline);

// ComputePipeline functions
GFX_API void gfxComputePipelineDestroy(GfxComputePipeline computePipeline);

// Queue functions
GFX_API GfxResult gfxQueueSubmit(GfxQueue queue, GfxCommandEncoder commandEncoder);
GFX_API GfxResult gfxQueueSubmitWithSync(GfxQueue queue, const GfxSubmitInfo* submitInfo);
GFX_API void gfxQueueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size);
GFX_API void gfxQueueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
    const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout);
GFX_API GfxResult gfxQueueWaitIdle(GfxQueue queue);

// CommandEncoder functions
GFX_API void gfxCommandEncoderDestroy(GfxCommandEncoder commandEncoder);
GFX_API GfxResult gfxCommandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder,
    const GfxTextureView* colorAttachments, uint32_t colorAttachmentCount,
    const GfxColor* clearColors,
    const GfxTextureLayout* colorFinalLayouts,
    GfxTextureView depthStencilAttachment,
    float depthClearValue, uint32_t stencilClearValue,
    GfxTextureLayout depthFinalLayout,
    GfxRenderPassEncoder* outRenderPass);
GFX_API GfxResult gfxCommandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const char* label, GfxComputePassEncoder* outComputePass);
GFX_API void gfxCommandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset,
    GfxBuffer destination, uint64_t destinationOffset,
    uint64_t size);
GFX_API void gfxCommandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset, uint32_t bytesPerRow,
    GfxTexture destination, const GfxOrigin3D* origin,
    const GfxExtent3D* extent, uint32_t mipLevel, GfxTextureLayout finalLayout);
GFX_API void gfxCommandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* origin, uint32_t mipLevel,
    GfxBuffer destination, uint64_t destinationOffset, uint32_t bytesPerRow,
    const GfxExtent3D* extent, GfxTextureLayout finalLayout);
GFX_API void gfxCommandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* sourceOrigin, uint32_t sourceMipLevel,
    GfxTexture destination, const GfxOrigin3D* destinationOrigin, uint32_t destinationMipLevel,
    const GfxExtent3D* extent, GfxTextureLayout sourceFinalLayout, GfxTextureLayout destinationFinalLayout);
GFX_API void gfxCommandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder,
    const GfxTextureBarrier* textureBarriers, uint32_t textureBarrierCount);
GFX_API void gfxCommandEncoderEnd(GfxCommandEncoder commandEncoder);
GFX_API void gfxCommandEncoderBegin(GfxCommandEncoder commandEncoder);

// Helper function to deduce access flags from texture layout
GFX_API GfxAccessFlags gfxGetAccessFlagsForLayout(GfxTextureLayout layout);

// RenderPassEncoder functions
GFX_API void gfxRenderPassEncoderDestroy(GfxRenderPassEncoder renderPassEncoder);
GFX_API void gfxRenderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline);
// Dynamic offsets allow using a single bind group with multiple offsets into uniform/storage buffers
// The offsets MUST be aligned to device limits (use gfxAlignUp with minUniformBufferOffsetAlignment)
// Pass NULL and 0 if not using dynamic offsets
GFX_API void gfxRenderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount);
GFX_API void gfxRenderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size);
GFX_API void gfxRenderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size);
GFX_API void gfxRenderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport);
GFX_API void gfxRenderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor);
GFX_API void gfxRenderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
GFX_API void gfxRenderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance);
GFX_API void gfxRenderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder);

// ComputePassEncoder functions
GFX_API void gfxComputePassEncoderDestroy(GfxComputePassEncoder computePassEncoder);
GFX_API void gfxComputePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline);
// Dynamic offsets allow using a single bind group with multiple offsets into uniform/storage buffers
// The offsets MUST be aligned to device limits (use gfxAlignUp with minUniformBufferOffsetAlignment)
// Pass NULL and 0 if not using dynamic offsets
GFX_API void gfxComputePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount);
GFX_API void gfxComputePassEncoderDispatchWorkgroups(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ);
GFX_API void gfxComputePassEncoderEnd(GfxComputePassEncoder computePassEncoder);

// Fence functions
GFX_API void gfxFenceDestroy(GfxFence fence);
GFX_API GfxResult gfxFenceGetStatus(GfxFence fence, bool* isSignaled);
GFX_API GfxResult gfxFenceWait(GfxFence fence, uint64_t timeoutNs);
GFX_API void gfxFenceReset(GfxFence fence);

// Semaphore functions
GFX_API void gfxSemaphoreDestroy(GfxSemaphore semaphore);
GFX_API GfxSemaphoreType gfxSemaphoreGetType(GfxSemaphore semaphore);
GFX_API GfxResult gfxSemaphoreSignal(GfxSemaphore semaphore, uint64_t value);
GFX_API GfxResult gfxSemaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs);
GFX_API uint64_t gfxSemaphoreGetValue(GfxSemaphore semaphore);

// ============================================================================
// Utility Functions
// ============================================================================

// Helper functions for creating common structures
static inline GfxColor gfxColorMake(float r, float g, float b, float a)
{
    GfxColor color = { r, g, b, a };
    return color;
}

static inline GfxExtent3D gfxExtent3DMake(uint32_t width, uint32_t height, uint32_t depth)
{
    GfxExtent3D extent = { width, height, depth };
    return extent;
}

static inline GfxOrigin3D gfxOrigin3DMake(int32_t x, int32_t y, int32_t z)
{
    GfxOrigin3D origin = { x, y, z };
    return origin;
}

// Platform-specific helper functions (defined based on compile target)
#ifdef _WIN32
static inline GfxPlatformWindowHandle gfxPlatformWindowHandleMake(void* hwnd, void* hinstance)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_WIN32;
    handle.win32.hwnd = hwnd;
    handle.win32.hinstance = hinstance;
    return handle;
}
#elif defined(__APPLE__)
static inline GfxPlatformWindowHandle gfxPlatformWindowHandleMake(void* nsWindow, void* metalLayer)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_COCOA;
    handle.cocoa.nsWindow = nsWindow;
    handle.cocoa.metalLayer = metalLayer;
    return handle;
}
#endif

// Cross-platform helpers available on all platforms
static inline GfxPlatformWindowHandle gfxPlatformWindowHandleMakeX11(void* window, void* display)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_X11;
    handle.x11.window = window;
    handle.x11.display = display;
    return handle;
}

static inline GfxPlatformWindowHandle gfxPlatformWindowHandleMakeWayland(void* surface, void* display)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_WAYLAND;
    handle.wayland.surface = surface;
    handle.wayland.display = display;
    return handle;
}

static inline GfxPlatformWindowHandle gfxPlatformWindowHandleMakeXCB(void* connection, uint32_t window)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_XCB;
    handle.xcb.connection = connection;
    handle.xcb.window = window;
    return handle;
}

static inline GfxPlatformWindowHandle gfxPlatformWindowHandleMakeWin32(void* hwnd, void* hinstance)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_WIN32;
    handle.win32.hwnd = hwnd;
    handle.win32.hinstance = hinstance;
    return handle;
}

static inline GfxPlatformWindowHandle gfxPlatformWindowHandleMakeCocoa(void* nsWindow, void* metalLayer)
{
    GfxPlatformWindowHandle handle = {};
    handle.windowingSystem = GFX_WINDOWING_SYSTEM_COCOA;
    handle.cocoa.nsWindow = nsWindow;
    handle.cocoa.metalLayer = metalLayer;
    return handle;
}

#ifdef __cplusplus
}
#endif