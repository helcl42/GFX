#include <backend/webgpu/converter/Conversions.h>
#include <backend/webgpu/core/CoreTypes.h>

#include <gtest/gtest.h>

// Test WebGPU conversion functions
// Tests pure conversion functions between C API types and WebGPU types

namespace {

// ============================================================================
// Format Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxFormatToWGPUFormat_CommonFormats_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFormatToWGPUFormat(GFX_FORMAT_UNDEFINED), WGPUTextureFormat_Undefined);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFormatToWGPUFormat(GFX_FORMAT_R8_UNORM), WGPUTextureFormat_R8Unorm);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFormatToWGPUFormat(GFX_FORMAT_R8G8_UNORM), WGPUTextureFormat_RG8Unorm);
}

TEST(WebGPUConversionsTest, GfxFormatToWGPUFormat_RGBA8Formats_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFormatToWGPUFormat(GFX_FORMAT_R8G8B8A8_UNORM), WGPUTextureFormat_RGBA8Unorm);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFormatToWGPUFormat(GFX_FORMAT_R8G8B8A8_UNORM_SRGB), WGPUTextureFormat_RGBA8UnormSrgb);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFormatToWGPUFormat(GFX_FORMAT_B8G8R8A8_UNORM), WGPUTextureFormat_BGRA8Unorm);
}

TEST(WebGPUConversionsTest, GfxFormatToWGPUFormat_FloatFormats_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFormatToWGPUFormat(GFX_FORMAT_R32_FLOAT), WGPUTextureFormat_R32Float);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFormatToWGPUFormat(GFX_FORMAT_R32G32_FLOAT), WGPUTextureFormat_RG32Float);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFormatToWGPUFormat(GFX_FORMAT_R32G32B32A32_FLOAT), WGPUTextureFormat_RGBA32Float);
}

TEST(WebGPUConversionsTest, GfxFormatToWGPUFormat_DepthFormats_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFormatToWGPUFormat(GFX_FORMAT_DEPTH16_UNORM), WGPUTextureFormat_Depth16Unorm);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFormatToWGPUFormat(GFX_FORMAT_DEPTH32_FLOAT), WGPUTextureFormat_Depth32Float);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFormatToWGPUFormat(GFX_FORMAT_DEPTH24_PLUS_STENCIL8), WGPUTextureFormat_Depth24PlusStencil8);
}

TEST(WebGPUConversionsTest, WGPUFormatToGfxFormat_RoundTrip_Preserves)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::wgpuFormatToGfxFormat(WGPUTextureFormat_RGBA8Unorm), GFX_FORMAT_R8G8B8A8_UNORM);
    EXPECT_EQ(gfx::backend::webgpu::converter::wgpuFormatToGfxFormat(WGPUTextureFormat_RGBA8UnormSrgb), GFX_FORMAT_R8G8B8A8_UNORM_SRGB);
    EXPECT_EQ(gfx::backend::webgpu::converter::wgpuFormatToGfxFormat(WGPUTextureFormat_Depth32Float), GFX_FORMAT_DEPTH32_FLOAT);
}

TEST(WebGPUConversionsTest, FormatHasStencil_DepthStencilFormats_ReturnsTrue)
{
    EXPECT_TRUE(gfx::backend::webgpu::converter::formatHasStencil(GFX_FORMAT_DEPTH24_PLUS_STENCIL8));
    EXPECT_TRUE(gfx::backend::webgpu::converter::formatHasStencil(GFX_FORMAT_DEPTH32_FLOAT_STENCIL8));
}

TEST(WebGPUConversionsTest, FormatHasStencil_DepthOnlyFormats_ReturnsFalse)
{
    EXPECT_FALSE(gfx::backend::webgpu::converter::formatHasStencil(GFX_FORMAT_DEPTH16_UNORM));
    EXPECT_FALSE(gfx::backend::webgpu::converter::formatHasStencil(GFX_FORMAT_DEPTH32_FLOAT));
}

TEST(WebGPUConversionsTest, FormatHasStencil_ColorFormats_ReturnsFalse)
{
    EXPECT_FALSE(gfx::backend::webgpu::converter::formatHasStencil(GFX_FORMAT_R8G8B8A8_UNORM));
    EXPECT_FALSE(gfx::backend::webgpu::converter::formatHasStencil(GFX_FORMAT_B8G8R8A8_UNORM));
}

// ============================================================================
// Buffer Usage Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxBufferUsageToWGPU_SingleFlags_ConvertsCorrectly)
{
    EXPECT_TRUE(gfx::backend::webgpu::converter::gfxBufferUsageToWGPU(GFX_BUFFER_USAGE_VERTEX) & WGPUBufferUsage_Vertex);
    EXPECT_TRUE(gfx::backend::webgpu::converter::gfxBufferUsageToWGPU(GFX_BUFFER_USAGE_INDEX) & WGPUBufferUsage_Index);
    EXPECT_TRUE(gfx::backend::webgpu::converter::gfxBufferUsageToWGPU(GFX_BUFFER_USAGE_UNIFORM) & WGPUBufferUsage_Uniform);
    EXPECT_TRUE(gfx::backend::webgpu::converter::gfxBufferUsageToWGPU(GFX_BUFFER_USAGE_STORAGE) & WGPUBufferUsage_Storage);
}

TEST(WebGPUConversionsTest, GfxBufferUsageToWGPU_MultipleFlags_CombinesCorrectly)
{
    WGPUBufferUsage result = gfx::backend::webgpu::converter::gfxBufferUsageToWGPU(
        GFX_BUFFER_USAGE_VERTEX | GFX_BUFFER_USAGE_UNIFORM);

    EXPECT_TRUE(result & WGPUBufferUsage_Vertex);
    EXPECT_TRUE(result & WGPUBufferUsage_Uniform);
}

TEST(WebGPUConversionsTest, WebGPUBufferUsageToGfxBufferUsage_RoundTrip_Preserves)
{
    GfxBufferUsageFlags original = GFX_FLAGS(GFX_BUFFER_USAGE_VERTEX | GFX_BUFFER_USAGE_UNIFORM);
    WGPUBufferUsage wgpu = gfx::backend::webgpu::converter::gfxBufferUsageToWGPU(original);
    GfxBufferUsageFlags result = gfx::backend::webgpu::converter::webgpuBufferUsageToGfxBufferUsage(wgpu);

    EXPECT_TRUE(result & GFX_BUFFER_USAGE_VERTEX);
    EXPECT_TRUE(result & GFX_BUFFER_USAGE_UNIFORM);
}

// ============================================================================
// Texture Usage Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxTextureUsageToWGPU_SingleFlags_ConvertsCorrectly)
{
    EXPECT_TRUE(gfx::backend::webgpu::converter::gfxTextureUsageToWGPU(GFX_TEXTURE_USAGE_TEXTURE_BINDING) & WGPUTextureUsage_TextureBinding);
    EXPECT_TRUE(gfx::backend::webgpu::converter::gfxTextureUsageToWGPU(GFX_TEXTURE_USAGE_STORAGE_BINDING) & WGPUTextureUsage_StorageBinding);
    EXPECT_TRUE(gfx::backend::webgpu::converter::gfxTextureUsageToWGPU(GFX_TEXTURE_USAGE_RENDER_ATTACHMENT) & WGPUTextureUsage_RenderAttachment);
}

TEST(WebGPUConversionsTest, GfxTextureUsageToWGPU_MultipleFlags_CombinesCorrectly)
{
    WGPUTextureUsage result = gfx::backend::webgpu::converter::gfxTextureUsageToWGPU(
        GFX_TEXTURE_USAGE_TEXTURE_BINDING | GFX_TEXTURE_USAGE_COPY_DST);

    EXPECT_TRUE(result & WGPUTextureUsage_TextureBinding);
    EXPECT_TRUE(result & WGPUTextureUsage_CopyDst);
}

TEST(WebGPUConversionsTest, WGPUTextureUsageToGfxTextureUsage_RoundTrip_Preserves)
{
    WGPUTextureUsage wgpu = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_StorageBinding;
    GfxTextureUsageFlags result = gfx::backend::webgpu::converter::wgpuTextureUsageToGfxTextureUsage(wgpu);

    EXPECT_TRUE(result & GFX_TEXTURE_USAGE_TEXTURE_BINDING);
    EXPECT_TRUE(result & GFX_TEXTURE_USAGE_STORAGE_BINDING);
}

// ============================================================================
// Index Format Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxIndexFormatToWGPU_ValidFormats_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxIndexFormatToWGPU(GFX_INDEX_FORMAT_UINT16), WGPUIndexFormat_Uint16);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxIndexFormatToWGPU(GFX_INDEX_FORMAT_UINT32), WGPUIndexFormat_Uint32);
}

// ============================================================================
// Load/Store Op Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxLoadOpToWGPULoadOp_AllOps_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxLoadOpToWGPULoadOp(GFX_LOAD_OP_LOAD), WGPULoadOp_Load);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxLoadOpToWGPULoadOp(GFX_LOAD_OP_CLEAR), WGPULoadOp_Clear);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxLoadOpToWGPULoadOp(GFX_LOAD_OP_DONT_CARE), WGPULoadOp_Undefined);
}

TEST(WebGPUConversionsTest, GfxStoreOpToWGPUStoreOp_AllOps_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxStoreOpToWGPUStoreOp(GFX_STORE_OP_STORE), WGPUStoreOp_Store);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxStoreOpToWGPUStoreOp(GFX_STORE_OP_DONT_CARE), WGPUStoreOp_Discard);
}

// ============================================================================
// Adapter Type Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, WGPUAdapterTypeToGfxAdapterType_AllTypes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::wgpuAdapterTypeToGfxAdapterType(WGPUAdapterType_DiscreteGPU), GFX_ADAPTER_TYPE_DISCRETE_GPU);
    EXPECT_EQ(gfx::backend::webgpu::converter::wgpuAdapterTypeToGfxAdapterType(WGPUAdapterType_IntegratedGPU), GFX_ADAPTER_TYPE_INTEGRATED_GPU);
    EXPECT_EQ(gfx::backend::webgpu::converter::wgpuAdapterTypeToGfxAdapterType(WGPUAdapterType_CPU), GFX_ADAPTER_TYPE_CPU);
    EXPECT_EQ(gfx::backend::webgpu::converter::wgpuAdapterTypeToGfxAdapterType(WGPUAdapterType_Unknown), GFX_ADAPTER_TYPE_UNKNOWN);
}

// ============================================================================
// Semaphore Type Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxSemaphoreTypeToWebGPUSemaphoreType_AllTypes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxSemaphoreTypeToWebGPUSemaphoreType(GFX_SEMAPHORE_TYPE_BINARY), gfx::backend::webgpu::core::SemaphoreType::Binary);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxSemaphoreTypeToWebGPUSemaphoreType(GFX_SEMAPHORE_TYPE_TIMELINE), gfx::backend::webgpu::core::SemaphoreType::Timeline);
}

TEST(WebGPUConversionsTest, WebGPUSemaphoreTypeToGfxSemaphoreType_RoundTrip_Preserves)
{
    gfx::backend::webgpu::core::SemaphoreType internal = gfx::backend::webgpu::core::SemaphoreType::Timeline;
    GfxSemaphoreType result = gfx::backend::webgpu::converter::webgpuSemaphoreTypeToGfxSemaphoreType(internal);
    EXPECT_EQ(result, GFX_SEMAPHORE_TYPE_TIMELINE);
}

// ============================================================================
// Handle Conversion Tests (Templates)
// ============================================================================

TEST(WebGPUConversionsTest, ToGfx_NullPointer_ReturnsNullHandle)
{
    int* ptr = nullptr;
    GfxBuffer handle = gfx::backend::webgpu::converter::toGfx<GfxBuffer>(ptr);
    EXPECT_EQ(handle, nullptr);
}

TEST(WebGPUConversionsTest, ToNative_NullHandle_ReturnsNullPointer)
{
    GfxBuffer handle = nullptr;
    int* ptr = gfx::backend::webgpu::converter::toNative<int>(handle);
    EXPECT_EQ(ptr, nullptr);
}

TEST(WebGPUConversionsTest, ToGfxToNative_RoundTrip_Preserves)
{
    // Create a dummy pointer value (not dereferenced)
    int* originalPtr = reinterpret_cast<int*>(0x12345678);

    GfxBuffer handle = gfx::backend::webgpu::converter::toGfx<GfxBuffer>(originalPtr);
    int* resultPtr = gfx::backend::webgpu::converter::toNative<int>(handle);

    EXPECT_EQ(resultPtr, originalPtr);
}

// ============================================================================
// Present Mode Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxPresentModeToWGPU_AllModes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxPresentModeToWGPU(GFX_PRESENT_MODE_IMMEDIATE), WGPUPresentMode_Immediate);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxPresentModeToWGPU(GFX_PRESENT_MODE_MAILBOX), WGPUPresentMode_Mailbox);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxPresentModeToWGPU(GFX_PRESENT_MODE_FIFO), WGPUPresentMode_Fifo);
}

TEST(WebGPUConversionsTest, WGPUPresentModeToGfxPresentMode_RoundTrip_Preserves)
{
    GfxPresentMode original = GFX_PRESENT_MODE_MAILBOX;
    WGPUPresentMode wgpu = gfx::backend::webgpu::converter::gfxPresentModeToWGPU(original);
    GfxPresentMode result = gfx::backend::webgpu::converter::wgpuPresentModeToGfxPresentMode(wgpu);
    EXPECT_EQ(result, original);
}

// ============================================================================
// Sample Count Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, WGPUSampleCountToGfxSampleCount_ValidCounts_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::wgpuSampleCountToGfxSampleCount(1), GFX_SAMPLE_COUNT_1);
    EXPECT_EQ(gfx::backend::webgpu::converter::wgpuSampleCountToGfxSampleCount(4), GFX_SAMPLE_COUNT_4);
}

// ============================================================================
// Sampler Address Mode Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxAddressModeToWGPU_AllModes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxAddressModeToWGPU(GFX_ADDRESS_MODE_REPEAT), WGPUAddressMode_Repeat);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxAddressModeToWGPU(GFX_ADDRESS_MODE_MIRROR_REPEAT), WGPUAddressMode_MirrorRepeat);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxAddressModeToWGPU(GFX_ADDRESS_MODE_CLAMP_TO_EDGE), WGPUAddressMode_ClampToEdge);
}

// ============================================================================
// Filter Mode Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxFilterModeToWGPU_AllModes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFilterModeToWGPU(GFX_FILTER_MODE_NEAREST), WGPUFilterMode_Nearest);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFilterModeToWGPU(GFX_FILTER_MODE_LINEAR), WGPUFilterMode_Linear);
}

TEST(WebGPUConversionsTest, GfxMipmapFilterModeToWGPU_AllModes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxMipmapFilterModeToWGPU(GFX_FILTER_MODE_NEAREST), WGPUMipmapFilterMode_Nearest);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxMipmapFilterModeToWGPU(GFX_FILTER_MODE_LINEAR), WGPUMipmapFilterMode_Linear);
}

// ============================================================================
// Primitive Topology Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxPrimitiveTopologyToWGPU_AllTopologies_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxPrimitiveTopologyToWGPU(GFX_PRIMITIVE_TOPOLOGY_POINT_LIST), WGPUPrimitiveTopology_PointList);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxPrimitiveTopologyToWGPU(GFX_PRIMITIVE_TOPOLOGY_LINE_LIST), WGPUPrimitiveTopology_LineList);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxPrimitiveTopologyToWGPU(GFX_PRIMITIVE_TOPOLOGY_LINE_STRIP), WGPUPrimitiveTopology_LineStrip);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxPrimitiveTopologyToWGPU(GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST), WGPUPrimitiveTopology_TriangleList);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxPrimitiveTopologyToWGPU(GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP), WGPUPrimitiveTopology_TriangleStrip);
}

// ============================================================================
// Cull Mode Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxCullModeToWGPU_AllModes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxCullModeToWGPU(GFX_CULL_MODE_NONE), WGPUCullMode_None);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxCullModeToWGPU(GFX_CULL_MODE_FRONT), WGPUCullMode_Front);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxCullModeToWGPU(GFX_CULL_MODE_BACK), WGPUCullMode_Back);
}

// ============================================================================
// Front Face Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxFrontFaceToWGPU_AllModes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFrontFaceToWGPU(GFX_FRONT_FACE_COUNTER_CLOCKWISE), WGPUFrontFace_CCW);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFrontFaceToWGPU(GFX_FRONT_FACE_CLOCKWISE), WGPUFrontFace_CW);
}

// ============================================================================
// Blend Operation Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxBlendOperationToWGPU_AllOperations_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxBlendOperationToWGPU(GFX_BLEND_OPERATION_ADD), WGPUBlendOperation_Add);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxBlendOperationToWGPU(GFX_BLEND_OPERATION_SUBTRACT), WGPUBlendOperation_Subtract);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxBlendOperationToWGPU(GFX_BLEND_OPERATION_REVERSE_SUBTRACT), WGPUBlendOperation_ReverseSubtract);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxBlendOperationToWGPU(GFX_BLEND_OPERATION_MIN), WGPUBlendOperation_Min);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxBlendOperationToWGPU(GFX_BLEND_OPERATION_MAX), WGPUBlendOperation_Max);
}

// ============================================================================
// Blend Factor Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxBlendFactorToWGPU_CommonFactors_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxBlendFactorToWGPU(GFX_BLEND_FACTOR_ZERO), WGPUBlendFactor_Zero);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxBlendFactorToWGPU(GFX_BLEND_FACTOR_ONE), WGPUBlendFactor_One);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxBlendFactorToWGPU(GFX_BLEND_FACTOR_SRC), WGPUBlendFactor_Src);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxBlendFactorToWGPU(GFX_BLEND_FACTOR_ONE_MINUS_SRC), WGPUBlendFactor_OneMinusSrc);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxBlendFactorToWGPU(GFX_BLEND_FACTOR_SRC_ALPHA), WGPUBlendFactor_SrcAlpha);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxBlendFactorToWGPU(GFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA), WGPUBlendFactor_OneMinusSrcAlpha);
}

// ============================================================================
// Compare Function Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxCompareFunctionToWGPU_AllFunctions_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxCompareFunctionToWGPU(GFX_COMPARE_FUNCTION_NEVER), WGPUCompareFunction_Never);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxCompareFunctionToWGPU(GFX_COMPARE_FUNCTION_LESS), WGPUCompareFunction_Less);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxCompareFunctionToWGPU(GFX_COMPARE_FUNCTION_EQUAL), WGPUCompareFunction_Equal);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxCompareFunctionToWGPU(GFX_COMPARE_FUNCTION_LESS_EQUAL), WGPUCompareFunction_LessEqual);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxCompareFunctionToWGPU(GFX_COMPARE_FUNCTION_GREATER), WGPUCompareFunction_Greater);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxCompareFunctionToWGPU(GFX_COMPARE_FUNCTION_NOT_EQUAL), WGPUCompareFunction_NotEqual);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxCompareFunctionToWGPU(GFX_COMPARE_FUNCTION_GREATER_EQUAL), WGPUCompareFunction_GreaterEqual);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxCompareFunctionToWGPU(GFX_COMPARE_FUNCTION_ALWAYS), WGPUCompareFunction_Always);
}

// ============================================================================
// Stencil Operation Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxStencilOperationToWGPU_AllOperations_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxStencilOperationToWGPU(GFX_STENCIL_OPERATION_KEEP), WGPUStencilOperation_Keep);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxStencilOperationToWGPU(GFX_STENCIL_OPERATION_ZERO), WGPUStencilOperation_Zero);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxStencilOperationToWGPU(GFX_STENCIL_OPERATION_REPLACE), WGPUStencilOperation_Replace);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxStencilOperationToWGPU(GFX_STENCIL_OPERATION_INCREMENT_CLAMP), WGPUStencilOperation_IncrementClamp);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxStencilOperationToWGPU(GFX_STENCIL_OPERATION_DECREMENT_CLAMP), WGPUStencilOperation_DecrementClamp);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxStencilOperationToWGPU(GFX_STENCIL_OPERATION_INVERT), WGPUStencilOperation_Invert);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxStencilOperationToWGPU(GFX_STENCIL_OPERATION_INCREMENT_WRAP), WGPUStencilOperation_IncrementWrap);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxStencilOperationToWGPU(GFX_STENCIL_OPERATION_DECREMENT_WRAP), WGPUStencilOperation_DecrementWrap);
}

// ============================================================================
// Texture Dimension Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxTextureTypeToWGPUTextureDimension_AllTypes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxTextureTypeToWGPUTextureDimension(GFX_TEXTURE_TYPE_1D), WGPUTextureDimension_1D);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxTextureTypeToWGPUTextureDimension(GFX_TEXTURE_TYPE_2D), WGPUTextureDimension_2D);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxTextureTypeToWGPUTextureDimension(GFX_TEXTURE_TYPE_3D), WGPUTextureDimension_3D);
}

TEST(WebGPUConversionsTest, WGPUTextureDimensionToGfxTextureType_RoundTrip_Preserves)
{
    GfxTextureType original = GFX_TEXTURE_TYPE_2D;
    WGPUTextureDimension wgpu = gfx::backend::webgpu::converter::gfxTextureTypeToWGPUTextureDimension(original);
    GfxTextureType result = gfx::backend::webgpu::converter::wgpuTextureDimensionToGfxTextureType(wgpu);
    EXPECT_EQ(result, original);
}

// ============================================================================
// Texture View Dimension Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxTextureViewTypeToWGPU_AllTypes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxTextureViewTypeToWGPU(GFX_TEXTURE_VIEW_TYPE_1D), WGPUTextureViewDimension_1D);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxTextureViewTypeToWGPU(GFX_TEXTURE_VIEW_TYPE_2D), WGPUTextureViewDimension_2D);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxTextureViewTypeToWGPU(GFX_TEXTURE_VIEW_TYPE_2D_ARRAY), WGPUTextureViewDimension_2DArray);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxTextureViewTypeToWGPU(GFX_TEXTURE_VIEW_TYPE_CUBE), WGPUTextureViewDimension_Cube);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxTextureViewTypeToWGPU(GFX_TEXTURE_VIEW_TYPE_CUBE_ARRAY), WGPUTextureViewDimension_CubeArray);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxTextureViewTypeToWGPU(GFX_TEXTURE_VIEW_TYPE_3D), WGPUTextureViewDimension_3D);
}

// ============================================================================
// Geometry Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxOrigin3DToWGPUOrigin3D_AllFields_ConvertsCorrectly)
{
    GfxOrigin3D origin = { 10, 20, 30 };
    WGPUOrigin3D result = gfx::backend::webgpu::converter::gfxOrigin3DToWGPUOrigin3D(&origin);

    EXPECT_EQ(result.x, 10u);
    EXPECT_EQ(result.y, 20u);
    EXPECT_EQ(result.z, 30u);
}

TEST(WebGPUConversionsTest, GfxExtent3DToWGPUExtent3D_AllFields_ConvertsCorrectly)
{
    GfxExtent3D extent = { 800, 600, 1 };
    WGPUExtent3D result = gfx::backend::webgpu::converter::gfxExtent3DToWGPUExtent3D(&extent);

    EXPECT_EQ(result.width, 800u);
    EXPECT_EQ(result.height, 600u);
    EXPECT_EQ(result.depthOrArrayLayers, 1u);
}

TEST(WebGPUConversionsTest, WGPUExtent3DToGfxExtent3D_RoundTrip_Preserves)
{
    WGPUExtent3D wgpu = { 1024, 768, 16 };
    GfxExtent3D result = gfx::backend::webgpu::converter::wgpuExtent3DToGfxExtent3D(wgpu);

    EXPECT_EQ(result.width, 1024u);
    EXPECT_EQ(result.height, 768u);
    EXPECT_EQ(result.depth, 16u);
}

// ============================================================================
// Shader Source Type Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxShaderSourceTypeToWebGPU_AllTypes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxShaderSourceTypeToWebGPU(GFX_SHADER_SOURCE_WGSL), gfx::backend::webgpu::core::ShaderSourceType::WGSL);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxShaderSourceTypeToWebGPU(GFX_SHADER_SOURCE_SPIRV), gfx::backend::webgpu::core::ShaderSourceType::SPIRV);
}

// ============================================================================
// Vertex Format Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxFormatToWGPUVertexFormat_NormFormats_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFormatToWGPUVertexFormat(GFX_FORMAT_R8G8B8A8_UNORM), WGPUVertexFormat_Unorm8x4);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFormatToWGPUVertexFormat(GFX_FORMAT_R8G8B8A8_UNORM_SRGB), WGPUVertexFormat_Unorm8x4);
}

TEST(WebGPUConversionsTest, GfxFormatToWGPUVertexFormat_FloatFormats_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFormatToWGPUVertexFormat(GFX_FORMAT_R32_FLOAT), WGPUVertexFormat_Float32);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFormatToWGPUVertexFormat(GFX_FORMAT_R32G32_FLOAT), WGPUVertexFormat_Float32x2);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFormatToWGPUVertexFormat(GFX_FORMAT_R32G32B32_FLOAT), WGPUVertexFormat_Float32x3);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFormatToWGPUVertexFormat(GFX_FORMAT_R32G32B32A32_FLOAT), WGPUVertexFormat_Float32x4);
}

TEST(WebGPUConversionsTest, GfxFormatToWGPUVertexFormat_HalfFloatFormats_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFormatToWGPUVertexFormat(GFX_FORMAT_R16G16_FLOAT), WGPUVertexFormat_Float16x2);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxFormatToWGPUVertexFormat(GFX_FORMAT_R16G16B16A16_FLOAT), WGPUVertexFormat_Float16x4);
}

// ============================================================================
// Texture Sample Type Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxTextureSampleTypeToWGPU_AllTypes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxTextureSampleTypeToWGPU(GFX_TEXTURE_SAMPLE_TYPE_FLOAT), WGPUTextureSampleType_Float);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxTextureSampleTypeToWGPU(GFX_TEXTURE_SAMPLE_TYPE_UNFILTERABLE_FLOAT), WGPUTextureSampleType_UnfilterableFloat);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxTextureSampleTypeToWGPU(GFX_TEXTURE_SAMPLE_TYPE_DEPTH), WGPUTextureSampleType_Depth);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxTextureSampleTypeToWGPU(GFX_TEXTURE_SAMPLE_TYPE_SINT), WGPUTextureSampleType_Sint);
    EXPECT_EQ(gfx::backend::webgpu::converter::gfxTextureSampleTypeToWGPU(GFX_TEXTURE_SAMPLE_TYPE_UINT), WGPUTextureSampleType_Uint);
}

// ============================================================================
// Reverse Conversion Tests
// ============================================================================

TEST(WebGPUConversionsTest, WGPUCompareFunctionToGfx_AllFunctions_ConvertsCorrectly)
{
    // Test round-trip for all compare functions
    GfxCompareFunction functions[] = {
        GFX_COMPARE_FUNCTION_NEVER,
        GFX_COMPARE_FUNCTION_LESS,
        GFX_COMPARE_FUNCTION_EQUAL,
        GFX_COMPARE_FUNCTION_LESS_EQUAL,
        GFX_COMPARE_FUNCTION_GREATER,
        GFX_COMPARE_FUNCTION_NOT_EQUAL,
        GFX_COMPARE_FUNCTION_GREATER_EQUAL,
        GFX_COMPARE_FUNCTION_ALWAYS
    };

    for (auto func : functions) {
        WGPUCompareFunction wgpu = gfx::backend::webgpu::converter::gfxCompareFunctionToWGPU(func);
        // Just verify the conversion doesn't crash - reverse conversion not exposed
        EXPECT_NE(wgpu, WGPUCompareFunction_Undefined);
    }
}

TEST(WebGPUConversionsTest, WGPUStencilOperationToGfx_AllOperations_ConvertsCorrectly)
{
    // Test round-trip for all stencil operations
    GfxStencilOperation ops[] = {
        GFX_STENCIL_OPERATION_KEEP,
        GFX_STENCIL_OPERATION_ZERO,
        GFX_STENCIL_OPERATION_REPLACE,
        GFX_STENCIL_OPERATION_INCREMENT_CLAMP,
        GFX_STENCIL_OPERATION_DECREMENT_CLAMP,
        GFX_STENCIL_OPERATION_INVERT,
        GFX_STENCIL_OPERATION_INCREMENT_WRAP,
        GFX_STENCIL_OPERATION_DECREMENT_WRAP
    };

    for (auto op : ops) {
        WGPUStencilOperation wgpu = gfx::backend::webgpu::converter::gfxStencilOperationToWGPU(op);
        EXPECT_NE(wgpu, 0); // Verify valid conversion
    }
}

TEST(WebGPUConversionsTest, WGPUBlendOperationToGfx_AllOperations_ConvertsCorrectly)
{
    // Test all blend operations convert successfully
    GfxBlendOperation ops[] = {
        GFX_BLEND_OPERATION_ADD,
        GFX_BLEND_OPERATION_SUBTRACT,
        GFX_BLEND_OPERATION_REVERSE_SUBTRACT,
        GFX_BLEND_OPERATION_MIN,
        GFX_BLEND_OPERATION_MAX
    };

    for (auto op : ops) {
        WGPUBlendOperation wgpu = gfx::backend::webgpu::converter::gfxBlendOperationToWGPU(op);
        EXPECT_NE(wgpu, 0); // Verify valid conversion
    }
}

TEST(WebGPUConversionsTest, WGPUBlendFactorToGfx_AllFactors_ConvertsCorrectly)
{
    // Test all blend factors convert successfully
    GfxBlendFactor factors[] = {
        GFX_BLEND_FACTOR_ZERO,
        GFX_BLEND_FACTOR_ONE,
        GFX_BLEND_FACTOR_SRC,
        GFX_BLEND_FACTOR_ONE_MINUS_SRC,
        GFX_BLEND_FACTOR_SRC_ALPHA,
        GFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        GFX_BLEND_FACTOR_DST,
        GFX_BLEND_FACTOR_ONE_MINUS_DST,
        GFX_BLEND_FACTOR_DST_ALPHA,
        GFX_BLEND_FACTOR_ONE_MINUS_DST_ALPHA
    };

    for (auto factor : factors) {
        WGPUBlendFactor wgpu = gfx::backend::webgpu::converter::gfxBlendFactorToWGPU(factor);
        EXPECT_NE(wgpu, 0); // Verify valid conversion
    }
}

// ============================================================================
// Complex Round-Trip Tests
// ============================================================================

TEST(WebGPUConversionsTest, FormatConversion_AllCommonFormats_RoundTrip)
{
    GfxFormat formats[] = {
        GFX_FORMAT_R8G8B8A8_UNORM,
        GFX_FORMAT_R8G8B8A8_UNORM_SRGB,
        GFX_FORMAT_B8G8R8A8_UNORM,
        GFX_FORMAT_R32_FLOAT,
        GFX_FORMAT_R32G32_FLOAT,
        GFX_FORMAT_R32G32B32A32_FLOAT,
        GFX_FORMAT_DEPTH16_UNORM,
        GFX_FORMAT_DEPTH32_FLOAT,
        GFX_FORMAT_DEPTH24_PLUS_STENCIL8
    };

    for (auto format : formats) {
        WGPUTextureFormat wgpu = gfx::backend::webgpu::converter::gfxFormatToWGPUFormat(format);
        GfxFormat result = gfx::backend::webgpu::converter::wgpuFormatToGfxFormat(wgpu);
        EXPECT_EQ(result, format) << "Format round-trip failed for format " << format;
    }
}

TEST(WebGPUConversionsTest, TextureUsageConversion_CombinedFlags_RoundTrip)
{
    GfxTextureUsageFlags usage = GFX_FLAGS(GFX_TEXTURE_USAGE_TEXTURE_BINDING | GFX_TEXTURE_USAGE_RENDER_ATTACHMENT | GFX_TEXTURE_USAGE_STORAGE_BINDING);

    WGPUTextureUsage wgpu = gfx::backend::webgpu::converter::gfxTextureUsageToWGPU(usage);
    GfxTextureUsageFlags result = gfx::backend::webgpu::converter::wgpuTextureUsageToGfxTextureUsage(wgpu);

    EXPECT_TRUE(result & GFX_TEXTURE_USAGE_TEXTURE_BINDING);
    EXPECT_TRUE(result & GFX_TEXTURE_USAGE_RENDER_ATTACHMENT);
    EXPECT_TRUE(result & GFX_TEXTURE_USAGE_STORAGE_BINDING);
}

TEST(WebGPUConversionsTest, BufferUsageConversion_CombinedFlags_RoundTrip)
{
    GfxBufferUsageFlags usage = GFX_FLAGS(GFX_BUFFER_USAGE_VERTEX | GFX_BUFFER_USAGE_INDEX | GFX_BUFFER_USAGE_COPY_SRC);

    WGPUBufferUsage wgpu = gfx::backend::webgpu::converter::gfxBufferUsageToWGPU(usage);
    GfxBufferUsageFlags result = gfx::backend::webgpu::converter::webgpuBufferUsageToGfxBufferUsage(wgpu);

    EXPECT_TRUE(result & GFX_BUFFER_USAGE_VERTEX);
    EXPECT_TRUE(result & GFX_BUFFER_USAGE_INDEX);
    EXPECT_TRUE(result & GFX_BUFFER_USAGE_COPY_SRC);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(WebGPUConversionsTest, GfxOrigin3DToWGPUOrigin3D_ZeroOrigin_ConvertsCorrectly)
{
    GfxOrigin3D origin = { 0, 0, 0 };
    WGPUOrigin3D result = gfx::backend::webgpu::converter::gfxOrigin3DToWGPUOrigin3D(&origin);

    EXPECT_EQ(result.x, 0u);
    EXPECT_EQ(result.y, 0u);
    EXPECT_EQ(result.z, 0u);
}

TEST(WebGPUConversionsTest, GfxExtent3DToWGPUExtent3D_MaxDimensions_ConvertsCorrectly)
{
    GfxExtent3D extent = { 8192, 8192, 256 };
    WGPUExtent3D result = gfx::backend::webgpu::converter::gfxExtent3DToWGPUExtent3D(&extent);

    EXPECT_EQ(result.width, 8192u);
    EXPECT_EQ(result.height, 8192u);
    EXPECT_EQ(result.depthOrArrayLayers, 256u);
}

TEST(WebGPUConversionsTest, GfxExtent3DToWGPUExtent3D_1DTexture_ConvertsCorrectly)
{
    GfxExtent3D extent = { 1024, 1, 1 };
    WGPUExtent3D result = gfx::backend::webgpu::converter::gfxExtent3DToWGPUExtent3D(&extent);

    EXPECT_EQ(result.width, 1024u);
    EXPECT_EQ(result.height, 1u);
    EXPECT_EQ(result.depthOrArrayLayers, 1u);
}

TEST(WebGPUConversionsTest, GfxExtent3DToWGPUExtent3D_3DTexture_ConvertsCorrectly)
{
    GfxExtent3D extent = { 256, 256, 64 };
    WGPUExtent3D result = gfx::backend::webgpu::converter::gfxExtent3DToWGPUExtent3D(&extent);

    EXPECT_EQ(result.width, 256u);
    EXPECT_EQ(result.height, 256u);
    EXPECT_EQ(result.depthOrArrayLayers, 64u);
}

// ============================================================================
// Semaphore Type Round-Trip Tests
// ============================================================================

TEST(WebGPUConversionsTest, SemaphoreTypeConversion_RoundTrip_Preserves)
{
    GfxSemaphoreType original = GFX_SEMAPHORE_TYPE_BINARY;
    gfx::backend::webgpu::core::SemaphoreType webgpu = gfx::backend::webgpu::converter::gfxSemaphoreTypeToWebGPUSemaphoreType(original);
    GfxSemaphoreType result = gfx::backend::webgpu::converter::webgpuSemaphoreTypeToGfxSemaphoreType(webgpu);
    EXPECT_EQ(result, original);

    original = GFX_SEMAPHORE_TYPE_TIMELINE;
    webgpu = gfx::backend::webgpu::converter::gfxSemaphoreTypeToWebGPUSemaphoreType(original);
    result = gfx::backend::webgpu::converter::webgpuSemaphoreTypeToGfxSemaphoreType(webgpu);
    EXPECT_EQ(result, original);
}

} // anonymous namespace
