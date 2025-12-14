#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
    GFX_TEXTURE_USAGE_NONE = 0,
    GFX_TEXTURE_USAGE_COPY_SRC = 1 << 0,
    GFX_TEXTURE_USAGE_COPY_DST = 1 << 1,
    GFX_TEXTURE_USAGE_TEXTURE_BINDING = 1 << 2,
    GFX_TEXTURE_USAGE_STORAGE_BINDING = 1 << 3,
    GFX_TEXTURE_USAGE_RENDER_ATTACHMENT = 1 << 4
} GfxTextureUsage;

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

// Result codes
typedef enum GfxResult {
    GFX_RESULT_SUCCESS = 0,
    GFX_RESULT_ERROR_INVALID_PARAMETER,
    GFX_RESULT_TIMEOUT,
    GFX_RESULT_ERROR_UNKNOWN
} GfxResult;

// ============================================================================
// Utility Structures
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
    uint32_t x;
    uint32_t y;
    uint32_t z;
} GfxOrigin3D;

// ============================================================================
// Platform Abstraction
// ============================================================================

#ifdef _WIN32
typedef struct {
    void* hwnd; // HWND - Window handle
    void* hinstance; // HINSTANCE - Application instance
} GfxPlatformWindowHandle;

#elif defined(__linux__)
typedef struct {
    void* window; // Window (X11) or wl_surface* (Wayland)
    void* display; // Display* (X11) or wl_display* (Wayland)
    bool isWayland; // true for Wayland, false for X11
    // XCB support (alternative to Xlib)
    void* xcb_connection; // xcb_connection_t* (XCB)
    uint32_t xcb_window; // xcb_window_t (XCB)
} GfxPlatformWindowHandle;

#elif defined(__APPLE__)
typedef struct {
    void* nsWindow; // NSWindow*
    void* metalLayer; // CAMetalLayer* (optional)
} GfxPlatformWindowHandle;

#else
typedef struct {
    void* handle;
    void* display;
    void* extra;
} GfxPlatformWindowHandle;
#endif

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
    const char* applicationName;
    uint32_t applicationVersion;
    const char** requiredExtensions;
    uint32_t requiredExtensionCount;
} GfxInstanceDescriptor;

typedef struct {
    GfxPowerPreference powerPreference;
    bool forceFallbackAdapter;
} GfxAdapterDescriptor;

typedef struct {
    const char* label;
    const char** requiredFeatures;
    uint32_t requiredFeatureCount;
} GfxDeviceDescriptor;

typedef struct {
    const char* label;
    GfxPlatformWindowHandle windowHandle;
    uint32_t width;
    uint32_t height;
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
    GfxExtent3D size;
    uint32_t mipLevelCount;
    uint32_t sampleCount;
    GfxTextureFormat format;
    GfxTextureUsage usage;
} GfxTextureDescriptor;

typedef struct {
    const char* label;
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
    bool frontFaceCounterClockwise;
    bool cullBackFace;
    bool unclippedDepth;
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
    uint32_t sampleCount;
    // Bind group layouts for the pipeline
    GfxBindGroupLayout* bindGroupLayouts;
    uint32_t bindGroupLayoutCount;
} GfxRenderPipelineDescriptor;

typedef struct {
    const char* label;
    GfxShader compute;
    const char* entryPoint;
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
        bool multisampled;
    } texture;

    struct {
        GfxTextureFormat format;
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

// ============================================================================
// API Functions
// ============================================================================

// Backend loading/unloading functions
// These should be called before creating any instances
// Call gfxLoadBackend or gfxLoadAllBackends at application startup
// Call gfxUnloadBackend or gfxUnloadAllBackends at application shutdown
bool gfxLoadBackend(GfxBackend backend);
void gfxUnloadBackend(GfxBackend backend);
bool gfxLoadAllBackends(void);
void gfxUnloadAllBackends(void);

// Instance functions
GfxInstance gfxCreateInstance(const GfxInstanceDescriptor* descriptor);
void gfxInstanceDestroy(GfxInstance instance);
GfxAdapter gfxInstanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor);
uint32_t gfxInstanceEnumerateAdapters(GfxInstance instance, GfxAdapter* adapters, uint32_t maxAdapters);

// Adapter functions
void gfxAdapterDestroy(GfxAdapter adapter);
GfxDevice gfxAdapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor);
const char* gfxAdapterGetName(GfxAdapter adapter);
GfxBackend gfxAdapterGetBackend(GfxAdapter adapter);

// Device functions
void gfxDeviceDestroy(GfxDevice device);
GfxQueue gfxDeviceGetQueue(GfxDevice device);
GfxSurface gfxDeviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor);
GfxSwapchain gfxDeviceCreateSwapchain(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor);
GfxBuffer gfxDeviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor);
GfxTexture gfxDeviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor);
GfxSampler gfxDeviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor);
GfxShader gfxDeviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor);
GfxBindGroupLayout gfxDeviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor);
GfxBindGroup gfxDeviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor);
GfxRenderPipeline gfxDeviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor);
GfxComputePipeline gfxDeviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor);
GfxCommandEncoder gfxDeviceCreateCommandEncoder(GfxDevice device, const char* label);
GfxFence gfxDeviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor);
GfxSemaphore gfxDeviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor);
void gfxDeviceWaitIdle(GfxDevice device);

// Surface functions
void gfxSurfaceDestroy(GfxSurface surface);
uint32_t gfxSurfaceGetWidth(GfxSurface surface);
uint32_t gfxSurfaceGetHeight(GfxSurface surface);
void gfxSurfaceResize(GfxSurface surface, uint32_t width, uint32_t height);
uint32_t gfxSurfaceGetSupportedFormats(GfxSurface surface, GfxTextureFormat* formats, uint32_t maxFormats);
uint32_t gfxSurfaceGetSupportedPresentModes(GfxSurface surface, GfxPresentMode* presentModes, uint32_t maxModes);
GfxPlatformWindowHandle gfxSurfaceGetPlatformHandle(GfxSurface surface);

// Swapchain functions
void gfxSwapchainDestroy(GfxSwapchain swapchain);
uint32_t gfxSwapchainGetWidth(GfxSwapchain swapchain);
uint32_t gfxSwapchainGetHeight(GfxSwapchain swapchain);
GfxTextureFormat gfxSwapchainGetFormat(GfxSwapchain swapchain);
uint32_t gfxSwapchainGetBufferCount(GfxSwapchain swapchain);
GfxTextureView gfxSwapchainGetCurrentTextureView(GfxSwapchain swapchain);
void gfxSwapchainPresent(GfxSwapchain swapchain);
void gfxSwapchainResize(GfxSwapchain swapchain, uint32_t width, uint32_t height);
bool gfxSwapchainNeedsRecreation(GfxSwapchain swapchain);

// Buffer functions
void gfxBufferDestroy(GfxBuffer buffer);
uint64_t gfxBufferGetSize(GfxBuffer buffer);
GfxBufferUsage gfxBufferGetUsage(GfxBuffer buffer);
void* gfxBufferMapAsync(GfxBuffer buffer, uint64_t offset, uint64_t size);
void gfxBufferUnmap(GfxBuffer buffer);

// Texture functions
void gfxTextureDestroy(GfxTexture texture);
GfxExtent3D gfxTextureGetSize(GfxTexture texture);
GfxTextureFormat gfxTextureGetFormat(GfxTexture texture);
uint32_t gfxTextureGetMipLevelCount(GfxTexture texture);
uint32_t gfxTextureGetSampleCount(GfxTexture texture);
GfxTextureUsage gfxTextureGetUsage(GfxTexture texture);
GfxTextureView gfxTextureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor);

// TextureView functions
void gfxTextureViewDestroy(GfxTextureView textureView);
GfxTexture gfxTextureViewGetTexture(GfxTextureView textureView);

// Sampler functions
void gfxSamplerDestroy(GfxSampler sampler);

// Shader functions
void gfxShaderDestroy(GfxShader shader);

// BindGroupLayout functions
void gfxBindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout);

// BindGroup functions
void gfxBindGroupDestroy(GfxBindGroup);

// RenderPipeline functions
void gfxRenderPipelineDestroy(GfxRenderPipeline renderPipeline);

// ComputePipeline functions
void gfxComputePipelineDestroy(GfxComputePipeline computePipeline);

// Queue functions
void gfxQueueSubmit(GfxQueue queue, GfxCommandEncoder commandEncoder);
void gfxQueueSubmitWithSync(GfxQueue queue, const GfxSubmitInfo* submitInfo);
void gfxQueueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size);
void gfxQueueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
    const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent);
void gfxQueueWaitIdle(GfxQueue queue);

// CommandEncoder functions
void gfxCommandEncoderDestroy(GfxCommandEncoder commandEncoder);
GfxRenderPassEncoder gfxCommandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder,
    const GfxTextureView* colorAttachments, uint32_t colorAttachmentCount,
    const GfxColor* clearColors,
    GfxTextureView depthStencilAttachment,
    float depthClearValue, uint32_t stencilClearValue);
GfxComputePassEncoder gfxCommandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const char* label);
void gfxCommandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset,
    GfxBuffer destination, uint64_t destinationOffset,
    uint64_t size);
void gfxCommandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder,
    GfxBuffer source, uint64_t sourceOffset, uint32_t bytesPerRow,
    GfxTexture destination, const GfxOrigin3D* origin,
    const GfxExtent3D* extent, uint32_t mipLevel);
void gfxCommandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* origin, uint32_t mipLevel,
    GfxBuffer destination, uint64_t destinationOffset, uint32_t bytesPerRow,
    const GfxExtent3D* extent);
void gfxCommandEncoderFinish(GfxCommandEncoder commandEncoder);

// RenderPassEncoder functions
void gfxRenderPassEncoderDestroy(GfxRenderPassEncoder renderPassEncoder);
void gfxRenderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline);
void gfxRenderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup);
void gfxRenderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size);
void gfxRenderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size);
void gfxRenderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
void gfxRenderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance);
void gfxRenderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder);

// ComputePassEncoder functions
void gfxComputePassEncoderDestroy(GfxComputePassEncoder computePassEncoder);
void gfxComputePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline);
void gfxComputePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup);
void gfxComputePassEncoderDispatchWorkgroups(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ);
void gfxComputePassEncoderEnd(GfxComputePassEncoder computePassEncoder);

// Fence functions
void gfxFenceDestroy(GfxFence fence);
GfxResult gfxFenceGetStatus(GfxFence fence, bool* isSignaled);
GfxResult gfxFenceWait(GfxFence fence, uint64_t timeoutNs);
void gfxFenceReset(GfxFence fence);

// Semaphore functions
void gfxSemaphoreDestroy(GfxSemaphore semaphore);
GfxSemaphoreType gfxSemaphoreGetType(GfxSemaphore semaphore);
GfxResult gfxSemaphoreSignal(GfxSemaphore semaphore, uint64_t value);
GfxResult gfxSemaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs);
uint64_t gfxSemaphoreGetValue(GfxSemaphore semaphore);

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

static inline GfxOrigin3D gfxOrigin3DMake(uint32_t x, uint32_t y, uint32_t z)
{
    GfxOrigin3D origin = { x, y, z };
    return origin;
}

#ifdef _WIN32
static inline GfxPlatformWindowHandle gfxPlatformWindowHandleMake(void* hwnd, void* hinstance)
{
    GfxPlatformWindowHandle handle = { hwnd, hinstance };
    return handle;
}
#elif defined(__linux__)
static inline GfxPlatformWindowHandle gfxPlatformWindowHandleMake(void* window, void* display, bool isWayland)
{
    GfxPlatformWindowHandle handle = { window, display, isWayland, NULL, 0 };
    return handle;
}
#elif defined(__APPLE__)
static inline GfxPlatformWindowHandle gfxPlatformWindowHandleMake(void* nsWindow, void* metalLayer)
{
    GfxPlatformWindowHandle handle = { nsWindow, metalLayer };
    return handle;
}
#else
static inline GfxPlatformWindowHandle gfxPlatformWindowHandleMake(void* handle, void* display, void* extra)
{
    GfxPlatformWindowHandle platformHandle = { handle, display, extra };
    return platformHandle;
}
#endif

#ifdef __cplusplus
}
#endif