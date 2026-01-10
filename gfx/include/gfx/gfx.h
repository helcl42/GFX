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
    GFX_BACKEND_VULKAN = 0,
    GFX_BACKEND_WEBGPU = 1,
    GFX_BACKEND_AUTO = 2,
    GFX_BACKEND_MAX_ENUM = 0x7FFFFFFF
} GfxBackend;

typedef enum {
    GFX_ADAPTER_TYPE_DISCRETE_GPU = 0,
    GFX_ADAPTER_TYPE_INTEGRATED_GPU = 1,
    GFX_ADAPTER_TYPE_CPU = 2,
    GFX_ADAPTER_TYPE_UNKNOWN = 3,
    GFX_ADAPTER_TYPE_MAX_ENUM = 0x7FFFFFFF
} GfxAdapterType;

typedef enum {
    GFX_ADAPTER_PREFERENCE_UNDEFINED = 0,
    GFX_ADAPTER_PREFERENCE_LOW_POWER = 1,
    GFX_ADAPTER_PREFERENCE_HIGH_PERFORMANCE = 2,
    GFX_ADAPTER_PREFERENCE_SOFTWARE = 3,
    GFX_ADAPTER_PREFERENCE_MAX_ENUM = 0x7FFFFFFF
} GfxAdapterPreference;

typedef enum {
    GFX_PRESENT_MODE_IMMEDIATE = 0,
    GFX_PRESENT_MODE_FIFO = 1,
    GFX_PRESENT_MODE_FIFO_RELAXED = 2,
    GFX_PRESENT_MODE_MAILBOX = 3,
    GFX_PRESENT_MODE_MAX_ENUM = 0x7FFFFFFF
} GfxPresentMode;

typedef enum {
    GFX_PRIMITIVE_TOPOLOGY_POINT_LIST = 0,
    GFX_PRIMITIVE_TOPOLOGY_LINE_LIST = 1,
    GFX_PRIMITIVE_TOPOLOGY_LINE_STRIP = 2,
    GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3,
    GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 4,
    GFX_PRIMITIVE_TOPOLOGY_MAX_ENUM = 0x7FFFFFFF
} GfxPrimitiveTopology;

typedef enum {
    GFX_FRONT_FACE_COUNTER_CLOCKWISE = 0,
    GFX_FRONT_FACE_CLOCKWISE = 1,
    GFX_FRONT_FACE_MAX_ENUM = 0x7FFFFFFF
} GfxFrontFace;

typedef enum {
    GFX_CULL_MODE_NONE = 0,
    GFX_CULL_MODE_FRONT = 1,
    GFX_CULL_MODE_BACK = 2,
    GFX_CULL_MODE_FRONT_AND_BACK = 3,
    GFX_CULL_MODE_MAX_ENUM = 0x7FFFFFFF
} GfxCullMode;

typedef enum {
    GFX_POLYGON_MODE_FILL = 0,
    GFX_POLYGON_MODE_LINE = 1,
    GFX_POLYGON_MODE_POINT = 2,
    GFX_POLYGON_MODE_MAX_ENUM = 0x7FFFFFFF
} GfxPolygonMode;

typedef enum {
    GFX_INDEX_FORMAT_UINT16 = 0,
    GFX_INDEX_FORMAT_UINT32 = 1,
    GFX_INDEX_FORMAT_MAX_ENUM = 0x7FFFFFFF
} GfxIndexFormat;

typedef enum {
    GFX_TEXTURE_FORMAT_UNDEFINED = 0,
    GFX_TEXTURE_FORMAT_R8_UNORM = 1,
    GFX_TEXTURE_FORMAT_R8G8_UNORM = 2,
    GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM = 3,
    GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM_SRGB = 4,
    GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM = 5,
    GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB = 6,
    GFX_TEXTURE_FORMAT_R16_FLOAT = 7,
    GFX_TEXTURE_FORMAT_R16G16_FLOAT = 8,
    GFX_TEXTURE_FORMAT_R16G16B16A16_FLOAT = 9,
    GFX_TEXTURE_FORMAT_R32_FLOAT = 10,
    GFX_TEXTURE_FORMAT_R32G32_FLOAT = 11,
    GFX_TEXTURE_FORMAT_R32G32B32_FLOAT = 12,
    GFX_TEXTURE_FORMAT_R32G32B32A32_FLOAT = 13,
    GFX_TEXTURE_FORMAT_DEPTH16_UNORM = 14,
    GFX_TEXTURE_FORMAT_DEPTH24_PLUS = 15,
    GFX_TEXTURE_FORMAT_DEPTH32_FLOAT = 16,
    GFX_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8 = 17,
    GFX_TEXTURE_FORMAT_DEPTH32_FLOAT_STENCIL8 = 18,
    GFX_TEXTURE_FORMAT_MAX_ENUM = 0x7FFFFFFF
} GfxTextureFormat;

typedef enum {
    GFX_TEXTURE_TYPE_1D = 0,
    GFX_TEXTURE_TYPE_2D = 1,
    GFX_TEXTURE_TYPE_3D = 2,
    GFX_TEXTURE_TYPE_CUBE = 3,
    GFX_TEXTURE_TYPE_MAX_ENUM = 0x7FFFFFFF
} GfxTextureType;

typedef enum {
    GFX_TEXTURE_VIEW_TYPE_1D = 0,
    GFX_TEXTURE_VIEW_TYPE_2D = 1,
    GFX_TEXTURE_VIEW_TYPE_3D = 2,
    GFX_TEXTURE_VIEW_TYPE_CUBE = 3,
    GFX_TEXTURE_VIEW_TYPE_1D_ARRAY = 4,
    GFX_TEXTURE_VIEW_TYPE_2D_ARRAY = 5,
    GFX_TEXTURE_VIEW_TYPE_CUBE_ARRAY = 6,
    GFX_TEXTURE_VIEW_TYPE_MAX_ENUM = 0x7FFFFFFF
} GfxTextureViewType;

typedef enum {
    GFX_TEXTURE_SAMPLE_TYPE_FLOAT = 0,
    GFX_TEXTURE_SAMPLE_TYPE_UNFILTERABLE_FLOAT = 1,
    GFX_TEXTURE_SAMPLE_TYPE_DEPTH = 2,
    GFX_TEXTURE_SAMPLE_TYPE_SINT = 3,
    GFX_TEXTURE_SAMPLE_TYPE_UINT = 4,
    GFX_TEXTURE_SAMPLE_TYPE_MAX_ENUM = 0x7FFFFFFF
} GfxTextureSampleType;

typedef enum {
    GFX_TEXTURE_USAGE_NONE = 0,
    GFX_TEXTURE_USAGE_COPY_SRC = 1 << 0,
    GFX_TEXTURE_USAGE_COPY_DST = 1 << 1,
    GFX_TEXTURE_USAGE_TEXTURE_BINDING = 1 << 2,
    GFX_TEXTURE_USAGE_STORAGE_BINDING = 1 << 3,
    GFX_TEXTURE_USAGE_RENDER_ATTACHMENT = 1 << 4,
    GFX_TEXTURE_USAGE_MAX_ENUM = 0x7FFFFFFF
} GfxTextureUsage;

typedef enum {
    GFX_TEXTURE_LAYOUT_UNDEFINED = 0, // Initial layout, contents undefined
    GFX_TEXTURE_LAYOUT_GENERAL = 1, // General purpose, can be used for anything but may be slow
    GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT = 2, // Optimal for color render target
    GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT = 3, // Optimal for depth/stencil render target
    GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY = 4, // Optimal for reading depth/stencil
    GFX_TEXTURE_LAYOUT_SHADER_READ_ONLY = 5, // Optimal for sampling in shaders
    GFX_TEXTURE_LAYOUT_TRANSFER_SRC = 6, // Optimal for copy source
    GFX_TEXTURE_LAYOUT_TRANSFER_DST = 7, // Optimal for copy destination
    GFX_TEXTURE_LAYOUT_PRESENT_SRC = 8, // Optimal for presentation
    GFX_TEXTURE_LAYOUT_MAX_ENUM = 0x7FFFFFFF
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
    GFX_PIPELINE_STAGE_ALL_COMMANDS = 0x00010000,
    GFX_PIPELINE_STAGE_MAX_ENUM = 0x7FFFFFFF
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
    GFX_ACCESS_MEMORY_WRITE = 1 << 15, // 0x8000
    GFX_ACCESS_MAX_ENUM = 0x7FFFFFFF
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
    GFX_BUFFER_USAGE_INDIRECT = 1 << 8,
    GFX_BUFFER_USAGE_MAX_ENUM = 0x7FFFFFFF
} GfxBufferUsage;

typedef enum {
    GFX_SHADER_STAGE_NONE = 0,
    GFX_SHADER_STAGE_VERTEX = 1 << 0,
    GFX_SHADER_STAGE_FRAGMENT = 1 << 1,
    GFX_SHADER_STAGE_COMPUTE = 1 << 2,
    GFX_SHADER_STAGE_MAX_ENUM = 0x7FFFFFFF
} GfxShaderStage;

typedef enum {
    GFX_FILTER_MODE_NEAREST = 0,
    GFX_FILTER_MODE_LINEAR = 1,
    GFX_FILTER_MODE_MAX_ENUM = 0x7FFFFFFF
} GfxFilterMode;

typedef enum {
    GFX_ADDRESS_MODE_REPEAT = 0,
    GFX_ADDRESS_MODE_MIRROR_REPEAT = 1,
    GFX_ADDRESS_MODE_CLAMP_TO_EDGE = 2,
    GFX_ADDRESS_MODE_MAX_ENUM = 0x7FFFFFFF
} GfxAddressMode;

typedef enum {
    GFX_COMPARE_FUNCTION_UNDEFINED = 0,
    GFX_COMPARE_FUNCTION_NEVER = 1,
    GFX_COMPARE_FUNCTION_LESS = 2,
    GFX_COMPARE_FUNCTION_EQUAL = 3,
    GFX_COMPARE_FUNCTION_LESS_EQUAL = 4,
    GFX_COMPARE_FUNCTION_GREATER = 5,
    GFX_COMPARE_FUNCTION_NOT_EQUAL = 6,
    GFX_COMPARE_FUNCTION_GREATER_EQUAL = 7,
    GFX_COMPARE_FUNCTION_ALWAYS = 8,
    GFX_COMPARE_FUNCTION_MAX_ENUM = 0x7FFFFFFF
} GfxCompareFunction;

typedef enum {
    GFX_BLEND_OPERATION_ADD = 0,
    GFX_BLEND_OPERATION_SUBTRACT = 1,
    GFX_BLEND_OPERATION_REVERSE_SUBTRACT = 2,
    GFX_BLEND_OPERATION_MIN = 3,
    GFX_BLEND_OPERATION_MAX = 4,
    GFX_BLEND_OPERATION_MAX_ENUM = 0x7FFFFFFF
} GfxBlendOperation;

typedef enum {
    GFX_BLEND_FACTOR_ZERO = 0,
    GFX_BLEND_FACTOR_ONE = 1,
    GFX_BLEND_FACTOR_SRC = 2,
    GFX_BLEND_FACTOR_ONE_MINUS_SRC = 3,
    GFX_BLEND_FACTOR_SRC_ALPHA = 4,
    GFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA = 5,
    GFX_BLEND_FACTOR_DST = 6,
    GFX_BLEND_FACTOR_ONE_MINUS_DST = 7,
    GFX_BLEND_FACTOR_DST_ALPHA = 8,
    GFX_BLEND_FACTOR_ONE_MINUS_DST_ALPHA = 9,
    GFX_BLEND_FACTOR_SRC_ALPHA_SATURATED = 10,
    GFX_BLEND_FACTOR_CONSTANT = 11,
    GFX_BLEND_FACTOR_ONE_MINUS_CONSTANT = 12,
    GFX_BLEND_FACTOR_MAX_ENUM = 0x7FFFFFFF
} GfxBlendFactor;

typedef enum {
    GFX_STENCIL_OPERATION_KEEP = 0,
    GFX_STENCIL_OPERATION_ZERO = 1,
    GFX_STENCIL_OPERATION_REPLACE = 2,
    GFX_STENCIL_OPERATION_INCREMENT_CLAMP = 3,
    GFX_STENCIL_OPERATION_DECREMENT_CLAMP = 4,
    GFX_STENCIL_OPERATION_INVERT = 5,
    GFX_STENCIL_OPERATION_INCREMENT_WRAP = 6,
    GFX_STENCIL_OPERATION_DECREMENT_WRAP = 7,
    GFX_STENCIL_OPERATION_MAX_ENUM = 0x7FFFFFFF
} GfxStencilOperation;

typedef enum {
    GFX_SAMPLE_COUNT_1 = 1,
    GFX_SAMPLE_COUNT_2 = 2,
    GFX_SAMPLE_COUNT_4 = 4,
    GFX_SAMPLE_COUNT_8 = 8,
    GFX_SAMPLE_COUNT_16 = 16,
    GFX_SAMPLE_COUNT_32 = 32,
    GFX_SAMPLE_COUNT_64 = 64,
    GFX_SAMPLE_COUNT_MAX_ENUM = 0x7FFFFFFF
} GfxSampleCount;

typedef enum {
    GFX_SHADER_SOURCE_WGSL = 0, // WGSL text source (for WebGPU)
    GFX_SHADER_SOURCE_SPIRV = 1, // SPIR-V binary (for Vulkan)
    GFX_SHADER_SOURCE_MAX_ENUM = 0x7FFFFFFF
} GfxShaderSourceType;

// Result codes
typedef enum GfxResult {
    GFX_RESULT_SUCCESS = 0,

    // Operation-specific errors
    GFX_RESULT_TIMEOUT = 1,
    GFX_RESULT_NOT_READY = 2,

    // General errors
    GFX_RESULT_ERROR_INVALID_PARAMETER = -1,
    GFX_RESULT_ERROR_OUT_OF_MEMORY = -2,
    GFX_RESULT_ERROR_DEVICE_LOST = -3,
    GFX_RESULT_ERROR_SURFACE_LOST = -4,
    GFX_RESULT_ERROR_OUT_OF_DATE = -5,
    // Backend errors
    GFX_RESULT_ERROR_BACKEND_NOT_LOADED = -6,
    GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED = -7,

    // Unknown/generic
    GFX_RESULT_ERROR_UNKNOWN = -8,
    GFX_RESULT_MAX_ENUM = 0x7FFFFFFF
} GfxResult;

typedef enum {
    GFX_DEBUG_MESSAGE_SEVERITY_VERBOSE = 0,
    GFX_DEBUG_MESSAGE_SEVERITY_INFO = 1,
    GFX_DEBUG_MESSAGE_SEVERITY_WARNING = 2,
    GFX_DEBUG_MESSAGE_SEVERITY_ERROR = 3,
    GFX_DEBUG_MESSAGE_SEVERITY_MAX_ENUM = 0x7FFFFFFF
} GfxDebugMessageSeverity;

typedef enum {
    GFX_DEBUG_MESSAGE_TYPE_GENERAL = 0,
    GFX_DEBUG_MESSAGE_TYPE_VALIDATION = 1,
    GFX_DEBUG_MESSAGE_TYPE_PERFORMANCE = 2,
    GFX_DEBUG_MESSAGE_TYPE_MAX_ENUM = 0x7FFFFFFF
} GfxDebugMessageType;

typedef enum {
    GFX_LOAD_OP_LOAD = 0, // Load existing contents
    GFX_LOAD_OP_CLEAR = 1, // Clear to specified clear value
    GFX_LOAD_OP_DONT_CARE = 2, // Don't care about initial contents (better performance on tiled GPUs)
    GFX_LOAD_OP_MAX_ENUM = 0x7FFFFFFF
} GfxLoadOp;

typedef enum {
    GFX_STORE_OP_STORE = 0, // Store contents after render pass
    GFX_STORE_OP_DONT_CARE = 1, // Don't care about contents after render pass (better performance for transient attachments)
    GFX_STORE_OP_MAX_ENUM = 0x7FFFFFFF
} GfxStoreOp;

// Color write mask flags (can be combined with bitwise OR)
typedef enum {
    GFX_COLOR_WRITE_MASK_NONE = 0x0,
    GFX_COLOR_WRITE_MASK_RED = 0x1,
    GFX_COLOR_WRITE_MASK_GREEN = 0x2,
    GFX_COLOR_WRITE_MASK_BLUE = 0x4,
    GFX_COLOR_WRITE_MASK_ALPHA = 0x8,
    GFX_COLOR_WRITE_MASK_ALL = GFX_COLOR_WRITE_MASK_RED | GFX_COLOR_WRITE_MASK_GREEN | GFX_COLOR_WRITE_MASK_BLUE | GFX_COLOR_WRITE_MASK_ALPHA,
    GFX_COLOR_WRITE_MASK_MAX_ENUM = 0x7FFFFFFF
} GfxColorWriteMask;

typedef enum {
    GFX_BIND_GROUP_ENTRY_TYPE_BUFFER = 0,
    GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER = 1,
    GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW = 2,
    GFX_BIND_GROUP_ENTRY_TYPE_MAX_ENUM = 0x7FFFFFFF
} GfxBindGroupEntryType;

typedef enum {
    GFX_BINDING_TYPE_BUFFER = 0,
    GFX_BINDING_TYPE_SAMPLER = 1,
    GFX_BINDING_TYPE_TEXTURE = 2,
    GFX_BINDING_TYPE_STORAGE_TEXTURE = 3,
    GFX_BINDING_TYPE_MAX_ENUM = 0x7FFFFFFF
} GfxBindingType;

typedef enum {
    GFX_FENCE_STATUS_UNSIGNALED = 0,
    GFX_FENCE_STATUS_SIGNALED = 1,
    GFX_FENCE_STATUS_ERROR = 2,
    GFX_FENCE_STATUS_MAX_ENUM = 0x7FFFFFFF
} GfxFenceStatus;

typedef enum {
    GFX_SEMAPHORE_TYPE_BINARY = 0,
    GFX_SEMAPHORE_TYPE_TIMELINE = 1,
    GFX_SEMAPHORE_TYPE_MAX_ENUM = 0x7FFFFFFF
} GfxSemaphoreType;

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
typedef struct GfxRenderPass_T* GfxRenderPass;
typedef struct GfxFramebuffer_T* GfxFramebuffer;

// ============================================================================
// Callback Function Types
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
    GfxPipelineStage srcStageMask;
    GfxPipelineStage dstStageMask;
    GfxAccessFlags srcAccessMask;
    GfxAccessFlags dstAccessMask;
} GfxMemoryBarrier;

typedef struct {
    GfxBuffer buffer;
    GfxPipelineStage srcStageMask;
    GfxPipelineStage dstStageMask;
    GfxAccessFlags srcAccessMask;
    GfxAccessFlags dstAccessMask;
    uint64_t offset;
    uint64_t size;
} GfxBufferBarrier;

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

// Simple load/store operations (clear values specified separately)
typedef struct {
    GfxLoadOp loadOp;
    GfxStoreOp storeOp;
} GfxLoadStoreOps;

// Color attachment target for render pass (main or resolve)
typedef struct {
    GfxTextureFormat format;
    GfxSampleCount sampleCount;
    GfxLoadStoreOps ops;
    GfxTextureLayout finalLayout;
} GfxRenderPassColorAttachmentTarget;

// Color attachment description for render pass
typedef struct {
    GfxRenderPassColorAttachmentTarget target;
    const GfxRenderPassColorAttachmentTarget* resolveTarget; // NULL if no resolve needed
} GfxRenderPassColorAttachment;

// Depth/stencil attachment target for render pass (main or resolve)
typedef struct {
    GfxTextureFormat format;
    GfxSampleCount sampleCount;
    GfxLoadStoreOps depthOps;
    GfxLoadStoreOps stencilOps;
    GfxTextureLayout finalLayout;
} GfxRenderPassDepthStencilAttachmentTarget;

// Depth/stencil attachment description for render pass
typedef struct {
    GfxRenderPassDepthStencilAttachmentTarget target;
    const GfxRenderPassDepthStencilAttachmentTarget* resolveTarget; // NULL if no resolve needed
} GfxRenderPassDepthStencilAttachment;

// Render pass descriptor: defines attachment formats and load/store operations (cached, reusable)
typedef struct {
    const char* label;

    // Color attachments
    const GfxRenderPassColorAttachment* colorAttachments;
    uint32_t colorAttachmentCount;

    // Depth/stencil attachment (optional)
    const GfxRenderPassDepthStencilAttachment* depthStencilAttachment; // NULL if not used
} GfxRenderPassDescriptor;

// Framebuffer color attachment with optional resolve target
typedef struct {
    GfxTextureView view;
    GfxTextureView resolveTarget; // NULL if no resolve needed
} GfxFramebufferColorAttachment;

// Framebuffer depth/stencil attachment with optional resolve target
typedef struct {
    GfxTextureView view;
    GfxTextureView resolveTarget; // NULL if no resolve needed
} GfxFramebufferDepthStencilAttachment;

// Framebuffer descriptor: binds actual image views to a render pass
typedef struct {
    const char* label;
    GfxRenderPass renderPass; // The render pass this framebuffer is compatible with

    // Color attachments with optional resolve targets
    const GfxFramebufferColorAttachment* colorAttachments;
    uint32_t colorAttachmentCount;

    // Depth/stencil attachment with optional resolve target (use {NULL, NULL} if not used)
    GfxFramebufferDepthStencilAttachment depthStencilAttachment;

    uint32_t width; // Framebuffer width
    uint32_t height; // Framebuffer height
} GfxFramebufferDescriptor;

// Render pass begin descriptor: used to BEGIN a render pass (with clear values)
typedef struct {
    const char* label;
    GfxRenderPass renderPass; // The render pass to begin
    GfxFramebuffer framebuffer; // The framebuffer to render into

    // Clear values (per-frame data)
    const GfxColor* colorClearValues; // Array of clear colors (used when loadOp = CLEAR)
    uint32_t colorClearValueCount; // Number of color clear values (should match render pass color attachment count)
    float depthClearValue; // Used when depth loadOp = CLEAR
    uint32_t stencilClearValue; // Used when stencil loadOp = CLEAR
} GfxRenderPassBeginDescriptor;

typedef struct {
    const char* label;
} GfxComputePassBeginDescriptor;

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
    GfxAdapterPreference preference;
} GfxAdapterDescriptor;

// Adapter information
typedef struct {
    const char* name;
    const char* driverDescription;
    uint32_t vendorID;
    uint32_t deviceID;
    GfxAdapterType adapterType;
    GfxBackend backend;
} GfxAdapterInfo;

// Texture information
typedef struct {
    GfxTextureType type;
    GfxExtent3D size;
    uint32_t arrayLayerCount;
    uint32_t mipLevelCount;
    GfxSampleCount sampleCount;
    GfxTextureFormat format;
    GfxTextureUsage usage;
} GfxTextureInfo;

// Swapchain information
typedef struct {
    uint32_t width;
    uint32_t height;
    GfxTextureFormat format;
    uint32_t imageCount;
    GfxPresentMode presentMode;
} GfxSwapchainInfo;

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
    uint64_t size;
    GfxBufferUsage usage;
} GfxBufferDescriptor;

typedef struct {
    const char* label;
    const void* nativeHandle; // VkBuffer or WGPUBuffer (cast to void*)
    uint64_t size;
    GfxBufferUsage usage;
} GfxExternalBufferDescriptor;

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
    const void* nativeHandle; // VkImage or WGPUTexture (cast to void*)
    GfxTextureType type;
    GfxExtent3D size;
    uint32_t arrayLayerCount;
    uint32_t mipLevelCount;
    GfxSampleCount sampleCount;
    GfxTextureFormat format;
    GfxTextureUsage usage;
    GfxTextureLayout currentLayout; // Current layout of the imported texture
} GfxExternalTextureDescriptor;

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
    GfxCompareFunction compare; // GFX_COMPARE_FUNCTION_UNDEFINED if not used
    uint16_t maxAnisotropy;
} GfxSamplerDescriptor;

typedef struct {
    const char* label;
    GfxShaderSourceType sourceType; // Explicitly specify WGSL or SPIR-V
    const void* code; // Shader code - WGSL source (const char*) or SPIR-V binary (uint32_t*)
    size_t codeSize; // Size in bytes - for SPIR-V: byte count, for WGSL: strlen or 0 for null-terminated
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
    uint32_t writeMask; // Combination of GfxColorWriteMask flags
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
    GfxRenderPass renderPass; // Render pass this pipeline will be used with
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
    const char* label;
} GfxCommandEncoderDescriptor;

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
// Platform Specific
// ============================================================================

// Common windowing system enum for all platforms
typedef enum {
    GFX_WINDOWING_SYSTEM_WIN32 = 0,
    GFX_WINDOWING_SYSTEM_XLIB = 1,
    GFX_WINDOWING_SYSTEM_WAYLAND = 2,
    GFX_WINDOWING_SYSTEM_XCB = 3,
    GFX_WINDOWING_SYSTEM_METAL = 4,
    GFX_WINDOWING_SYSTEM_EMSCRIPTEN = 5,
    GFX_WINDOWING_SYSTEM_ANDROID = 6,
    GFX_WINDOWING_SYSTEM_MAX_ENUM = 0x7FFFFFFF
} GfxWindowingSystem;

// Common platform window handle struct with union for all windowing systems
typedef struct GfxWin32Handle {
    void* hwnd; // HWND - Window handle
    void* hinstance; // HINSTANCE - Application instance
} GfxWin32Handle;

typedef struct GfxXlibHandle {
    unsigned long window; // Window
    void* display; // Display*
} GfxXlibHandle;

typedef struct GfxX11Handle {
    void* connection;
    uint32_t window;
} GfxX11Handle;

typedef struct GfxWaylandHandle {
    void* surface; // wl_surface*
    void* display; // wl_display*
} GfxWaylandHandle;

typedef struct GfxXcbHandle {
    void* connection; // xcb_connection_t*
    uint32_t window; // xcb_window_t
} GfxXcbHandle;

typedef struct GfxMetalHandle {
    void* layer; // CAMetalLayer* (optional)
} GfxMetalHandle;

typedef struct GfxEmscriptenHandle {
    const char* canvasSelector; // CSS selector for canvas element (e.g., "#canvas")
} GfxEmscriptenHandle;

typedef struct GfxAndroidHandle {
    void* window;
} GfxAndroidHandle;

typedef struct {
    GfxWindowingSystem windowingSystem;
    union {
        GfxWin32Handle win32;
        GfxXlibHandle xlib;
        GfxXcbHandle xcb;
        GfxWaylandHandle wayland;
        GfxMetalHandle metal;
        GfxEmscriptenHandle emscripten;
        GfxAndroidHandle android;
    };
} GfxPlatformWindowHandle;

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
    uint32_t imageCount;
} GfxSwapchainDescriptor;

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
// TODO should we ruturn GfxResult from every function?
// should we return GfxResult here too and use same logic as Vulkan?
GFX_API uint32_t gfxInstanceEnumerateAdapters(GfxInstance instance, GfxAdapter* adapters, uint32_t maxAdapters);

// Adapter functions
GFX_API void gfxAdapterDestroy(GfxAdapter adapter);
GFX_API GfxResult gfxAdapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice);
GFX_API void gfxAdapterGetInfo(GfxAdapter adapter, GfxAdapterInfo* outInfo);
GFX_API void gfxAdapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits);

// Device functions
GFX_API void gfxDeviceDestroy(GfxDevice device);
GFX_API GfxQueue gfxDeviceGetQueue(GfxDevice device);
GFX_API GfxResult gfxDeviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface);
GFX_API GfxResult gfxDeviceCreateSwapchain(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain);
GFX_API GfxResult gfxDeviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer);
GFX_API GfxResult gfxDeviceImportBuffer(GfxDevice device, const GfxExternalBufferDescriptor* descriptor, GfxBuffer* outBuffer);
GFX_API GfxResult gfxDeviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture);
GFX_API GfxResult gfxDeviceImportTexture(GfxDevice device, const GfxExternalTextureDescriptor* descriptor, GfxTexture* outTexture);
GFX_API GfxResult gfxDeviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler);
GFX_API GfxResult gfxDeviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader);
GFX_API GfxResult gfxDeviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout);
GFX_API GfxResult gfxDeviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup);
GFX_API GfxResult gfxDeviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline);
GFX_API GfxResult gfxDeviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline);
GFX_API GfxResult gfxDeviceCreateCommandEncoder(GfxDevice device, const GfxCommandEncoderDescriptor* descriptor, GfxCommandEncoder* outEncoder);
GFX_API GfxResult gfxDeviceCreateRenderPass(GfxDevice device, const GfxRenderPassDescriptor* descriptor, GfxRenderPass* outRenderPass);
GFX_API GfxResult gfxDeviceCreateFramebuffer(GfxDevice device, const GfxFramebufferDescriptor* descriptor, GfxFramebuffer* outFramebuffer);
GFX_API GfxResult gfxDeviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence);
GFX_API GfxResult gfxDeviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore);
GFX_API void gfxDeviceWaitIdle(GfxDevice device);
GFX_API void gfxDeviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits);

// Surface functions
GFX_API void gfxSurfaceDestroy(GfxSurface surface);
GFX_API uint32_t gfxSurfaceGetSupportedFormats(GfxSurface surface, GfxTextureFormat* formats, uint32_t maxFormats);
GFX_API uint32_t gfxSurfaceGetSupportedPresentModes(GfxSurface surface, GfxPresentMode* presentModes, uint32_t maxModes);

// Swapchain functions
GFX_API void gfxSwapchainDestroy(GfxSwapchain swapchain);
GFX_API void gfxSwapchainGetInfo(GfxSwapchain swapchain, GfxSwapchainInfo* outInfo);
GFX_API GfxResult gfxSwapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex);
GFX_API GfxTextureView gfxSwapchainGetImageView(GfxSwapchain swapchain, uint32_t imageIndex);
GFX_API GfxTextureView gfxSwapchainGetCurrentTextureView(GfxSwapchain swapchain);
GFX_API GfxResult gfxSwapchainPresent(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo);

// Buffer functions
GFX_API void gfxBufferDestroy(GfxBuffer buffer);
GFX_API uint64_t gfxBufferGetSize(GfxBuffer buffer);
GFX_API GfxBufferUsage gfxBufferGetUsage(GfxBuffer buffer);
GFX_API GfxResult gfxBufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer);
GFX_API void gfxBufferUnmap(GfxBuffer buffer);

// Texture functions
GFX_API void gfxTextureDestroy(GfxTexture texture);
GFX_API void gfxTextureGetInfo(GfxTexture texture, GfxTextureInfo* outInfo);
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

// RenderPass functions
GFX_API void gfxRenderPassDestroy(GfxRenderPass renderPass);

// Framebuffer functions
GFX_API void gfxFramebufferDestroy(GfxFramebuffer framebuffer);

// Queue functions
GFX_API GfxResult gfxQueueSubmit(GfxQueue queue, const GfxSubmitInfo* submitInfo);
GFX_API void gfxQueueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size);
GFX_API void gfxQueueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
    const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout);
GFX_API GfxResult gfxQueueWaitIdle(GfxQueue queue);

// CommandEncoder functions
GFX_API void gfxCommandEncoderDestroy(GfxCommandEncoder commandEncoder);
GFX_API GfxResult gfxCommandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder,
    const GfxRenderPassBeginDescriptor* beginDescriptor,
    GfxRenderPassEncoder* outRenderPass);
GFX_API GfxResult gfxCommandEncoderBeginComputePass(GfxCommandEncoder commandEncoder,
    const GfxComputePassBeginDescriptor* beginDescriptor,
    GfxComputePassEncoder* outComputePass);
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
GFX_API void gfxCommandEncoderBlitTextureToTexture(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* sourceOrigin, const GfxExtent3D* sourceExtent, uint32_t sourceMipLevel,
    GfxTexture destination, const GfxOrigin3D* destinationOrigin, const GfxExtent3D* destinationExtent, uint32_t destinationMipLevel,
    GfxFilterMode filter, GfxTextureLayout sourceFinalLayout, GfxTextureLayout destinationFinalLayout);
GFX_API void gfxCommandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder,
    const GfxMemoryBarrier* memoryBarriers, uint32_t memoryBarrierCount,
    const GfxBufferBarrier* bufferBarriers, uint32_t bufferBarrierCount,
    const GfxTextureBarrier* textureBarriers, uint32_t textureBarrierCount);
GFX_API void gfxCommandEncoderGenerateMipmaps(GfxCommandEncoder commandEncoder, GfxTexture texture);
GFX_API void gfxCommandEncoderGenerateMipmapsRange(GfxCommandEncoder commandEncoder, GfxTexture texture,
    uint32_t baseMipLevel, uint32_t levelCount);
GFX_API void gfxCommandEncoderEnd(GfxCommandEncoder commandEncoder);
GFX_API void gfxCommandEncoderBegin(GfxCommandEncoder commandEncoder);

// RenderPassEncoder functions
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

// Helper function to deduce access flags from texture layout
// Returns appropriate access flags for the given layout (deterministic mapping)
// Note: WebGPU backends with implicit synchronization may ignore these flags
GFX_API GfxAccessFlags gfxGetAccessFlagsForLayout(GfxTextureLayout layout);

// Alignment helper functions
// Use these to align buffer offsets/sizes to device requirements:
// Example: uint64_t alignedOffset = gfxAlignUp(offset, limits.minUniformBufferOffsetAlignment);
GFX_API uint64_t gfxAlignUp(uint64_t value, uint64_t alignment);
GFX_API uint64_t gfxAlignDown(uint64_t value, uint64_t alignment);

// Cross-platform helpers available on all platforms
GfxPlatformWindowHandle gfxPlatformWindowHandleMakeXlib(void* display, unsigned long window);
GfxPlatformWindowHandle gfxPlatformWindowHandleMakeWayland(void* surface, void* display);
GfxPlatformWindowHandle gfxPlatformWindowHandleMakeXCB(void* connection, uint32_t window);
GfxPlatformWindowHandle gfxPlatformWindowHandleMakeWin32(void* hwnd, void* hinstance);
GfxPlatformWindowHandle gfxPlatformWindowHandleMakeEmscripten(const char* canvasSelector);
GfxPlatformWindowHandle gfxPlatformWindowHandleMakeAndroid(void* window);
GfxPlatformWindowHandle gfxPlatformWindowHandleMakeMetal(void* layer);

#ifdef __cplusplus
}
#endif