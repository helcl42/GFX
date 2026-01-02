#pragma once

#include <webgpu/webgpu.h>
#include <gfx/gfx.h>

// Forward declare internal WebGPU types
namespace gfx::webgpu {
    struct InstanceCreateInfo;
    struct PlatformWindowHandle;
    enum class SemaphoreType;
    // BufferUsage is an alias to WGPUBufferUsage, so we use WGPUBufferUsage directly
}

// ============================================================================
// WebGPU Conversion Functions
// ============================================================================

namespace gfx::convertor {

// ============================================================================
// Internal Type Conversions
// ============================================================================

gfx::webgpu::InstanceCreateInfo gfxDescriptorToWebGPUInstanceCreateInfo(const GfxInstanceDescriptor* descriptor);
gfx::webgpu::PlatformWindowHandle gfxWindowHandleToWebGPUPlatformWindowHandle(const GfxPlatformWindowHandle& gfxHandle);
gfx::webgpu::SemaphoreType gfxSemaphoreTypeToWebGPUSemaphoreType(GfxSemaphoreType gfxType);

// Reverse conversions - internal to Gfx API types
GfxBufferUsage webgpuBufferUsageToGfxBufferUsage(WGPUBufferUsage usage);
GfxSemaphoreType webgpuSemaphoreTypeToGfxSemaphoreType(gfx::webgpu::SemaphoreType type);

// ============================================================================
// String utilities
// ============================================================================

inline WGPUStringView gfxStringView(const char* str)
{
    if (!str) {
        return WGPUStringView{ nullptr, WGPU_STRLEN };
    }
    return WGPUStringView{ str, WGPU_STRLEN };
}

// ============================================================================
// Texture format conversions
// ============================================================================

inline WGPUTextureFormat gfxFormatToWGPUFormat(GfxTextureFormat format)
{
    switch (format) {
    case GFX_TEXTURE_FORMAT_R8_UNORM:
        return WGPUTextureFormat_R8Unorm;
    case GFX_TEXTURE_FORMAT_R8G8_UNORM:
        return WGPUTextureFormat_RG8Unorm;
    case GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM:
        return WGPUTextureFormat_RGBA8Unorm;
    case GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM_SRGB:
        return WGPUTextureFormat_RGBA8UnormSrgb;
    case GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM:
        return WGPUTextureFormat_BGRA8Unorm;
    case GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB:
        return WGPUTextureFormat_BGRA8UnormSrgb;
    case GFX_TEXTURE_FORMAT_R16_FLOAT:
        return WGPUTextureFormat_R16Float;
    case GFX_TEXTURE_FORMAT_R16G16_FLOAT:
        return WGPUTextureFormat_RG16Float;
    case GFX_TEXTURE_FORMAT_R16G16B16A16_FLOAT:
        return WGPUTextureFormat_RGBA16Float;
    case GFX_TEXTURE_FORMAT_R32_FLOAT:
        return WGPUTextureFormat_R32Float;
    case GFX_TEXTURE_FORMAT_R32G32_FLOAT:
        return WGPUTextureFormat_RG32Float;
    case GFX_TEXTURE_FORMAT_R32G32B32A32_FLOAT:
        return WGPUTextureFormat_RGBA32Float;
    case GFX_TEXTURE_FORMAT_DEPTH16_UNORM:
        return WGPUTextureFormat_Depth16Unorm;
    case GFX_TEXTURE_FORMAT_DEPTH24_PLUS:
        return WGPUTextureFormat_Depth24Plus;
    case GFX_TEXTURE_FORMAT_DEPTH32_FLOAT:
        return WGPUTextureFormat_Depth32Float;
    case GFX_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8:
        return WGPUTextureFormat_Depth24PlusStencil8;
    case GFX_TEXTURE_FORMAT_DEPTH32_FLOAT_STENCIL8:
        return WGPUTextureFormat_Depth32FloatStencil8;
    default:
        return WGPUTextureFormat_Undefined;
    }
}

inline GfxTextureFormat wgpuFormatToGfxFormat(WGPUTextureFormat format)
{
    switch (format) {
    case WGPUTextureFormat_R8Unorm:
        return GFX_TEXTURE_FORMAT_R8_UNORM;
    case WGPUTextureFormat_RG8Unorm:
        return GFX_TEXTURE_FORMAT_R8G8_UNORM;
    case WGPUTextureFormat_RGBA8Unorm:
        return GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    case WGPUTextureFormat_RGBA8UnormSrgb:
        return GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM_SRGB;
    case WGPUTextureFormat_BGRA8Unorm:
        return GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM;
    case WGPUTextureFormat_BGRA8UnormSrgb:
        return GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB;
    case WGPUTextureFormat_R16Float:
        return GFX_TEXTURE_FORMAT_R16_FLOAT;
    case WGPUTextureFormat_RG16Float:
        return GFX_TEXTURE_FORMAT_R16G16_FLOAT;
    case WGPUTextureFormat_RGBA16Float:
        return GFX_TEXTURE_FORMAT_R16G16B16A16_FLOAT;
    case WGPUTextureFormat_R32Float:
        return GFX_TEXTURE_FORMAT_R32_FLOAT;
    case WGPUTextureFormat_RG32Float:
        return GFX_TEXTURE_FORMAT_R32G32_FLOAT;
    case WGPUTextureFormat_RGBA32Float:
        return GFX_TEXTURE_FORMAT_R32G32B32A32_FLOAT;
    case WGPUTextureFormat_Depth16Unorm:
        return GFX_TEXTURE_FORMAT_DEPTH16_UNORM;
    case WGPUTextureFormat_Depth24Plus:
        return GFX_TEXTURE_FORMAT_DEPTH24_PLUS;
    case WGPUTextureFormat_Depth32Float:
        return GFX_TEXTURE_FORMAT_DEPTH32_FLOAT;
    case WGPUTextureFormat_Depth24PlusStencil8:
        return GFX_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8;
    case WGPUTextureFormat_Depth32FloatStencil8:
        return GFX_TEXTURE_FORMAT_DEPTH32_FLOAT_STENCIL8;
    default:
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
}

// Present mode conversions
inline GfxPresentMode wgpuPresentModeToGfxPresentMode(WGPUPresentMode mode)
{
    switch (mode) {
    case WGPUPresentMode_Immediate:
        return GFX_PRESENT_MODE_IMMEDIATE;
    case WGPUPresentMode_Mailbox:
        return GFX_PRESENT_MODE_MAILBOX;
    case WGPUPresentMode_Fifo:
        return GFX_PRESENT_MODE_FIFO;
    case WGPUPresentMode_FifoRelaxed:
        return GFX_PRESENT_MODE_FIFO_RELAXED;
    default:
        return GFX_PRESENT_MODE_FIFO;
    }
}

inline WGPUPresentMode gfxPresentModeToWGPU(GfxPresentMode mode)
{
    switch (mode) {
    case GFX_PRESENT_MODE_IMMEDIATE:
        return WGPUPresentMode_Immediate;
    case GFX_PRESENT_MODE_FIFO:
        return WGPUPresentMode_Fifo;
    case GFX_PRESENT_MODE_FIFO_RELAXED:
        return WGPUPresentMode_FifoRelaxed;
    case GFX_PRESENT_MODE_MAILBOX:
        return WGPUPresentMode_Mailbox;
    default:
        return WGPUPresentMode_Fifo;
    }
}

// Utility functions
inline bool formatHasStencil(GfxTextureFormat format)
{
    switch (format) {
    case GFX_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8:
    case GFX_TEXTURE_FORMAT_DEPTH32_FLOAT_STENCIL8:
        return true;
    default:
        return false;
    }
}

// Load/Store operations
inline WGPULoadOp gfxLoadOpToWGPULoadOp(GfxLoadOp loadOp)
{
    switch (loadOp) {
    case GFX_LOAD_OP_LOAD:
        return WGPULoadOp_Load;
    case GFX_LOAD_OP_CLEAR:
        return WGPULoadOp_Clear;
    case GFX_LOAD_OP_DONT_CARE:
        return WGPULoadOp_Undefined;
    default:
        return WGPULoadOp_Undefined;
    }
}

inline WGPUStoreOp gfxStoreOpToWGPUStoreOp(GfxStoreOp storeOp)
{
    switch (storeOp) {
    case GFX_STORE_OP_STORE:
        return WGPUStoreOp_Store;
    case GFX_STORE_OP_DONT_CARE:
        return WGPUStoreOp_Discard;
    default:
        return WGPUStoreOp_Discard;
    }
}

// Buffer usage conversions
inline WGPUBufferUsage gfxBufferUsageToWGPU(GfxBufferUsage usage)
{
    WGPUBufferUsage wgpu_usage = WGPUBufferUsage_None;
    if (usage & GFX_BUFFER_USAGE_MAP_READ) {
        wgpu_usage |= WGPUBufferUsage_MapRead;
    }
    if (usage & GFX_BUFFER_USAGE_MAP_WRITE) {
        wgpu_usage |= WGPUBufferUsage_MapWrite;
    }
    if (usage & GFX_BUFFER_USAGE_COPY_SRC) {
        wgpu_usage |= WGPUBufferUsage_CopySrc;
    }
    if (usage & GFX_BUFFER_USAGE_COPY_DST) {
        wgpu_usage |= WGPUBufferUsage_CopyDst;
    }
    if (usage & GFX_BUFFER_USAGE_INDEX) {
        wgpu_usage |= WGPUBufferUsage_Index;
    }
    if (usage & GFX_BUFFER_USAGE_VERTEX) {
        wgpu_usage |= WGPUBufferUsage_Vertex;
    }
    if (usage & GFX_BUFFER_USAGE_UNIFORM) {
        wgpu_usage |= WGPUBufferUsage_Uniform;
    }
    if (usage & GFX_BUFFER_USAGE_STORAGE) {
        wgpu_usage |= WGPUBufferUsage_Storage;
    }
    if (usage & GFX_BUFFER_USAGE_INDIRECT) {
        wgpu_usage |= WGPUBufferUsage_Indirect;
    }
    return wgpu_usage;
}

// Texture usage conversions
inline WGPUTextureUsage gfxTextureUsageToWGPU(GfxTextureUsage usage)
{
    WGPUTextureUsage wgpu_usage = WGPUTextureUsage_None;
    if (usage & GFX_TEXTURE_USAGE_COPY_SRC) {
        wgpu_usage |= WGPUTextureUsage_CopySrc;
    }
    if (usage & GFX_TEXTURE_USAGE_COPY_DST) {
        wgpu_usage |= WGPUTextureUsage_CopyDst;
    }
    if (usage & GFX_TEXTURE_USAGE_TEXTURE_BINDING) {
        wgpu_usage |= WGPUTextureUsage_TextureBinding;
    }
    if (usage & GFX_TEXTURE_USAGE_STORAGE_BINDING) {
        wgpu_usage |= WGPUTextureUsage_StorageBinding;
    }
    if (usage & GFX_TEXTURE_USAGE_RENDER_ATTACHMENT) {
        wgpu_usage |= WGPUTextureUsage_RenderAttachment;
    }
    return wgpu_usage;
}

// Sampler conversions
inline WGPUAddressMode gfxAddressModeToWGPU(GfxAddressMode mode)
{
    switch (mode) {
    case GFX_ADDRESS_MODE_REPEAT:
        return WGPUAddressMode_Repeat;
    case GFX_ADDRESS_MODE_MIRROR_REPEAT:
        return WGPUAddressMode_MirrorRepeat;
    case GFX_ADDRESS_MODE_CLAMP_TO_EDGE:
        return WGPUAddressMode_ClampToEdge;
    default:
        return WGPUAddressMode_ClampToEdge;
    }
}

inline WGPUFilterMode gfxFilterModeToWGPU(GfxFilterMode mode)
{
    return (mode == GFX_FILTER_MODE_LINEAR) ? WGPUFilterMode_Linear : WGPUFilterMode_Nearest;
}

inline WGPUMipmapFilterMode gfxMipmapFilterModeToWGPU(GfxFilterMode mode)
{
    return (mode == GFX_FILTER_MODE_LINEAR) ? WGPUMipmapFilterMode_Linear : WGPUMipmapFilterMode_Nearest;
}

// Pipeline state conversions
inline WGPUPrimitiveTopology gfxPrimitiveTopologyToWGPU(GfxPrimitiveTopology topology)
{
    switch (topology) {
    case GFX_PRIMITIVE_TOPOLOGY_POINT_LIST:
        return WGPUPrimitiveTopology_PointList;
    case GFX_PRIMITIVE_TOPOLOGY_LINE_LIST:
        return WGPUPrimitiveTopology_LineList;
    case GFX_PRIMITIVE_TOPOLOGY_LINE_STRIP:
        return WGPUPrimitiveTopology_LineStrip;
    case GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
        return WGPUPrimitiveTopology_TriangleList;
    case GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
        return WGPUPrimitiveTopology_TriangleStrip;
    default:
        return WGPUPrimitiveTopology_TriangleList;
    }
}

inline WGPUFrontFace gfxFrontFaceToWGPU(GfxFrontFace frontFace)
{
    return (frontFace == GFX_FRONT_FACE_COUNTER_CLOCKWISE) ? WGPUFrontFace_CCW : WGPUFrontFace_CW;
}

inline WGPUCullMode gfxCullModeToWGPU(GfxCullMode cullMode)
{
    switch (cullMode) {
    case GFX_CULL_MODE_NONE:
        return WGPUCullMode_None;
    case GFX_CULL_MODE_FRONT:
        return WGPUCullMode_Front;
    case GFX_CULL_MODE_BACK:
        return WGPUCullMode_Back;
    default:
        return WGPUCullMode_None;
    }
}

inline WGPUIndexFormat gfxIndexFormatToWGPU(GfxIndexFormat format)
{
    switch (format) {
    case GFX_INDEX_FORMAT_UINT16:
        return WGPUIndexFormat_Uint16;
    case GFX_INDEX_FORMAT_UINT32:
        return WGPUIndexFormat_Uint32;
    default:
        return WGPUIndexFormat_Undefined;
    }
}

// Blend state conversions
inline WGPUBlendOperation gfxBlendOperationToWGPU(GfxBlendOperation operation)
{
    switch (operation) {
    case GFX_BLEND_OPERATION_ADD:
        return WGPUBlendOperation_Add;
    case GFX_BLEND_OPERATION_SUBTRACT:
        return WGPUBlendOperation_Subtract;
    case GFX_BLEND_OPERATION_REVERSE_SUBTRACT:
        return WGPUBlendOperation_ReverseSubtract;
    case GFX_BLEND_OPERATION_MIN:
        return WGPUBlendOperation_Min;
    case GFX_BLEND_OPERATION_MAX:
        return WGPUBlendOperation_Max;
    default:
        return WGPUBlendOperation_Add;
    }
}

inline WGPUBlendFactor gfxBlendFactorToWGPU(GfxBlendFactor factor)
{
    switch (factor) {
    case GFX_BLEND_FACTOR_ZERO:
        return WGPUBlendFactor_Zero;
    case GFX_BLEND_FACTOR_ONE:
        return WGPUBlendFactor_One;
    case GFX_BLEND_FACTOR_SRC:
        return WGPUBlendFactor_Src;
    case GFX_BLEND_FACTOR_ONE_MINUS_SRC:
        return WGPUBlendFactor_OneMinusSrc;
    case GFX_BLEND_FACTOR_SRC_ALPHA:
        return WGPUBlendFactor_SrcAlpha;
    case GFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
        return WGPUBlendFactor_OneMinusSrcAlpha;
    case GFX_BLEND_FACTOR_DST:
        return WGPUBlendFactor_Dst;
    case GFX_BLEND_FACTOR_ONE_MINUS_DST:
        return WGPUBlendFactor_OneMinusDst;
    case GFX_BLEND_FACTOR_DST_ALPHA:
        return WGPUBlendFactor_DstAlpha;
    case GFX_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
        return WGPUBlendFactor_OneMinusDstAlpha;
    case GFX_BLEND_FACTOR_SRC_ALPHA_SATURATED:
        return WGPUBlendFactor_SrcAlphaSaturated;
    case GFX_BLEND_FACTOR_CONSTANT:
        return WGPUBlendFactor_Constant;
    case GFX_BLEND_FACTOR_ONE_MINUS_CONSTANT:
        return WGPUBlendFactor_OneMinusConstant;
    default:
        return WGPUBlendFactor_Zero;
    }
}

// Depth/Stencil conversions
inline WGPUCompareFunction gfxCompareFunctionToWGPU(GfxCompareFunction func)
{
    switch (func) {
    case GFX_COMPARE_FUNCTION_NEVER:
        return WGPUCompareFunction_Never;
    case GFX_COMPARE_FUNCTION_LESS:
        return WGPUCompareFunction_Less;
    case GFX_COMPARE_FUNCTION_EQUAL:
        return WGPUCompareFunction_Equal;
    case GFX_COMPARE_FUNCTION_LESS_EQUAL:
        return WGPUCompareFunction_LessEqual;
    case GFX_COMPARE_FUNCTION_GREATER:
        return WGPUCompareFunction_Greater;
    case GFX_COMPARE_FUNCTION_NOT_EQUAL:
        return WGPUCompareFunction_NotEqual;
    case GFX_COMPARE_FUNCTION_GREATER_EQUAL:
        return WGPUCompareFunction_GreaterEqual;
    case GFX_COMPARE_FUNCTION_ALWAYS:
        return WGPUCompareFunction_Always;
    default:
        return WGPUCompareFunction_Always;
    }
}

inline WGPUStencilOperation gfxStencilOperationToWGPU(GfxStencilOperation op)
{
    switch (op) {
    case GFX_STENCIL_OPERATION_KEEP:
        return WGPUStencilOperation_Keep;
    case GFX_STENCIL_OPERATION_ZERO:
        return WGPUStencilOperation_Zero;
    case GFX_STENCIL_OPERATION_REPLACE:
        return WGPUStencilOperation_Replace;
    case GFX_STENCIL_OPERATION_INVERT:
        return WGPUStencilOperation_Invert;
    case GFX_STENCIL_OPERATION_INCREMENT_CLAMP:
        return WGPUStencilOperation_IncrementClamp;
    case GFX_STENCIL_OPERATION_DECREMENT_CLAMP:
        return WGPUStencilOperation_DecrementClamp;
    case GFX_STENCIL_OPERATION_INCREMENT_WRAP:
        return WGPUStencilOperation_IncrementWrap;
    case GFX_STENCIL_OPERATION_DECREMENT_WRAP:
        return WGPUStencilOperation_DecrementWrap;
    default:
        return WGPUStencilOperation_Keep;
    }
}

// Texture binding conversions
inline WGPUTextureSampleType gfxTextureSampleTypeToWGPU(GfxTextureSampleType sampleType)
{
    switch (sampleType) {
    case GFX_TEXTURE_SAMPLE_TYPE_FLOAT:
        return WGPUTextureSampleType_Float;
    case GFX_TEXTURE_SAMPLE_TYPE_UNFILTERABLE_FLOAT:
        return WGPUTextureSampleType_UnfilterableFloat;
    case GFX_TEXTURE_SAMPLE_TYPE_DEPTH:
        return WGPUTextureSampleType_Depth;
    case GFX_TEXTURE_SAMPLE_TYPE_SINT:
        return WGPUTextureSampleType_Sint;
    case GFX_TEXTURE_SAMPLE_TYPE_UINT:
        return WGPUTextureSampleType_Uint;
    default:
        return WGPUTextureSampleType_Float;
    }
}

// Vertex format conversions
inline WGPUVertexFormat gfxFormatToWGPUVertexFormat(GfxTextureFormat format)
{
    switch (format) {
    case GFX_TEXTURE_FORMAT_R32_FLOAT:
        return WGPUVertexFormat_Float32;
    case GFX_TEXTURE_FORMAT_R32G32_FLOAT:
        return WGPUVertexFormat_Float32x2;
    case GFX_TEXTURE_FORMAT_R32G32B32_FLOAT:
        return WGPUVertexFormat_Float32x3;
    case GFX_TEXTURE_FORMAT_R32G32B32A32_FLOAT:
        return WGPUVertexFormat_Float32x4;
    case GFX_TEXTURE_FORMAT_R16G16_FLOAT:
        return WGPUVertexFormat_Float16x2;
    case GFX_TEXTURE_FORMAT_R16G16B16A16_FLOAT:
        return WGPUVertexFormat_Float16x4;
    case GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM:
        return WGPUVertexFormat_Unorm8x4;
    case GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM_SRGB:
        return WGPUVertexFormat_Unorm8x4;
    default:
        return static_cast<WGPUVertexFormat>(0);
    }
}

// Texture dimension conversions
inline WGPUTextureDimension gfxTextureTypeToWGPU(GfxTextureType type)
{
    switch (type) {
    case GFX_TEXTURE_TYPE_1D:
        return WGPUTextureDimension_1D;
    case GFX_TEXTURE_TYPE_2D:
        return WGPUTextureDimension_2D;
    case GFX_TEXTURE_TYPE_CUBE:
        return WGPUTextureDimension_2D; // Cube maps are 2D arrays in WebGPU
    case GFX_TEXTURE_TYPE_3D:
        return WGPUTextureDimension_3D;
    default:
        return WGPUTextureDimension_2D;
    }
}

inline WGPUTextureViewDimension gfxTextureViewTypeToWGPU(GfxTextureViewType type)
{
    switch (type) {
    case GFX_TEXTURE_VIEW_TYPE_1D:
        return WGPUTextureViewDimension_1D;
    case GFX_TEXTURE_VIEW_TYPE_2D:
        return WGPUTextureViewDimension_2D;
    case GFX_TEXTURE_VIEW_TYPE_3D:
        return WGPUTextureViewDimension_3D;
    case GFX_TEXTURE_VIEW_TYPE_CUBE:
        return WGPUTextureViewDimension_Cube;
    case GFX_TEXTURE_VIEW_TYPE_1D_ARRAY:
        return WGPUTextureViewDimension_1D; // WebGPU doesn't have 1D arrays
    case GFX_TEXTURE_VIEW_TYPE_2D_ARRAY:
        return WGPUTextureViewDimension_2DArray;
    case GFX_TEXTURE_VIEW_TYPE_CUBE_ARRAY:
        return WGPUTextureViewDimension_CubeArray;
    default:
        return WGPUTextureViewDimension_2D;
    }
}

} // namespace gfx::convertor
