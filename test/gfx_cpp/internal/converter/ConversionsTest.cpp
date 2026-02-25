
#include "../common/CommonTest.h"

#include <converter/Conversions.h>

namespace gfx {

// =============================================================================
// Backend Conversions
// =============================================================================

TEST(ConversionsTest, CppBackendToCBackend_Vulkan)
{
    EXPECT_EQ(cppBackendToCBackend(Backend::Vulkan), GFX_BACKEND_VULKAN);
}

TEST(ConversionsTest, CppBackendToCBackend_WebGPU)
{
    EXPECT_EQ(cppBackendToCBackend(Backend::WebGPU), GFX_BACKEND_WEBGPU);
}

TEST(ConversionsTest, CppBackendToCBackend_Auto)
{
    EXPECT_EQ(cppBackendToCBackend(Backend::Auto), GFX_BACKEND_AUTO);
}

TEST(ConversionsTest, CBackendToCppBackend_Vulkan)
{
    EXPECT_EQ(cBackendToCppBackend(GFX_BACKEND_VULKAN), Backend::Vulkan);
}

TEST(ConversionsTest, CBackendToCppBackend_WebGPU)
{
    EXPECT_EQ(cBackendToCppBackend(GFX_BACKEND_WEBGPU), Backend::WebGPU);
}

TEST(ConversionsTest, CBackendToCppBackend_Auto)
{
    EXPECT_EQ(cBackendToCppBackend(GFX_BACKEND_AUTO), Backend::Auto);
}

TEST(ConversionsTest, BackendRoundTrip)
{
    EXPECT_EQ(cBackendToCppBackend(cppBackendToCBackend(Backend::Vulkan)), Backend::Vulkan);
    EXPECT_EQ(cBackendToCppBackend(cppBackendToCBackend(Backend::WebGPU)), Backend::WebGPU);
    EXPECT_EQ(cBackendToCppBackend(cppBackendToCBackend(Backend::Auto)), Backend::Auto);
}

// =============================================================================
// String Array Conversions
// =============================================================================

TEST(ConversionsTest, CStringArrayToCppStringVector_Empty)
{
    auto result = cStringArrayToCppStringVector(nullptr, 0);
    EXPECT_TRUE(result.empty());
}

TEST(ConversionsTest, CStringArrayToCppStringVector_SingleString)
{
    const char* strings[] = { "test" };
    auto result = cStringArrayToCppStringVector(strings, 1);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "test");
}

TEST(ConversionsTest, CStringArrayToCppStringVector_MultipleStrings)
{
    const char* strings[] = { "first", "second", "third" };
    auto result = cStringArrayToCppStringVector(strings, 3);
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "first");
    EXPECT_EQ(result[1], "second");
    EXPECT_EQ(result[2], "third");
}

TEST(ConversionsTest, CStringArrayToCppStringVector_WithNullEntry)
{
    const char* strings[] = { "first", nullptr, "third" };
    auto result = cStringArrayToCppStringVector(strings, 3);
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], "first");
    EXPECT_EQ(result[1], "third");
}

// =============================================================================
// Adapter Type Conversions
// =============================================================================

TEST(ConversionsTest, CAdapterTypeToCppAdapterType_DiscreteGPU)
{
    EXPECT_EQ(cAdapterTypeToCppAdapterType(GFX_ADAPTER_TYPE_DISCRETE_GPU), AdapterType::DiscreteGPU);
}

TEST(ConversionsTest, CAdapterTypeToCppAdapterType_IntegratedGPU)
{
    EXPECT_EQ(cAdapterTypeToCppAdapterType(GFX_ADAPTER_TYPE_INTEGRATED_GPU), AdapterType::IntegratedGPU);
}

TEST(ConversionsTest, CAdapterTypeToCppAdapterType_CPU)
{
    EXPECT_EQ(cAdapterTypeToCppAdapterType(GFX_ADAPTER_TYPE_CPU), AdapterType::CPU);
}

TEST(ConversionsTest, CAdapterTypeToCppAdapterType_Unknown)
{
    EXPECT_EQ(cAdapterTypeToCppAdapterType(GFX_ADAPTER_TYPE_UNKNOWN), AdapterType::Unknown);
}

// =============================================================================
// Texture Format Conversions
// =============================================================================

TEST(ConversionsTest, TextureFormatRoundTrip)
{
    std::vector<TextureFormat> formats = {
        TextureFormat::R8G8B8A8Unorm,
        TextureFormat::R8G8B8A8UnormSrgb,
        TextureFormat::B8G8R8A8Unorm,
        TextureFormat::R32Float,
        TextureFormat::R32G32B32A32Float,
        TextureFormat::Depth24Plus,
        TextureFormat::Depth32Float,
    };

    for (const auto& format : formats) {
        auto cFormat = cppFormatToCFormat(format);
        auto backToCpp = cFormatToCppFormat(cFormat);
        EXPECT_EQ(backToCpp, format) << "Failed roundtrip for format " << static_cast<int>(format);
    }
}

// =============================================================================
// Texture Layout Conversions
// =============================================================================

TEST(ConversionsTest, TextureLayoutRoundTrip)
{
    std::vector<TextureLayout> layouts = {
        TextureLayout::Undefined,
        TextureLayout::General,
        TextureLayout::ColorAttachment,
        TextureLayout::DepthStencilAttachment,
        TextureLayout::DepthStencilReadOnly,
        TextureLayout::ShaderReadOnly,
        TextureLayout::TransferSrc,
        TextureLayout::TransferDst,
        TextureLayout::PresentSrc,
    };

    for (const auto& layout : layouts) {
        auto cLayout = cppLayoutToCLayout(layout);
        auto backToCpp = cLayoutToCppLayout(cLayout);
        EXPECT_EQ(backToCpp, layout) << "Failed roundtrip for layout " << static_cast<int>(layout);
    }
}

// =============================================================================
// Present Mode Conversions
// =============================================================================

TEST(ConversionsTest, PresentModeRoundTrip)
{
    std::vector<PresentMode> modes = {
        PresentMode::Immediate,
        PresentMode::Fifo,
        PresentMode::FifoRelaxed,
        PresentMode::Mailbox,
    };

    for (const auto& mode : modes) {
        auto cMode = cppPresentModeToCPresentMode(mode);
        auto backToCpp = cPresentModeToCppPresentMode(cMode);
        EXPECT_EQ(backToCpp, mode) << "Failed roundtrip for mode " << static_cast<int>(mode);
    }
}

// =============================================================================
// Sample Count Conversions
// =============================================================================

TEST(ConversionsTest, SampleCountRoundTrip)
{
    std::vector<SampleCount> counts = {
        SampleCount::Count1,
        SampleCount::Count2,
        SampleCount::Count4,
        SampleCount::Count8,
        SampleCount::Count16,
        SampleCount::Count32,
        SampleCount::Count64,
    };

    for (const auto& count : counts) {
        auto cCount = cppSampleCountToCCount(count);
        auto backToCpp = cSampleCountToCppCount(cCount);
        EXPECT_EQ(backToCpp, count) << "Failed roundtrip for count " << static_cast<int>(count);
    }
}

// =============================================================================
// Buffer Usage Conversions
// =============================================================================

TEST(ConversionsTest, BufferUsageRoundTrip_SingleFlags)
{
    std::vector<BufferUsage> usages = {
        BufferUsage::MapRead,
        BufferUsage::MapWrite,
        BufferUsage::CopySrc,
        BufferUsage::CopyDst,
        BufferUsage::Index,
        BufferUsage::Vertex,
        BufferUsage::Uniform,
        BufferUsage::Storage,
        BufferUsage::Indirect,
    };

    for (const auto& usage : usages) {
        auto cUsage = cppBufferUsageToCUsage(usage);
        auto backToCpp = cBufferUsageToCppUsage(cUsage);
        EXPECT_EQ(backToCpp, usage) << "Failed roundtrip for usage " << static_cast<uint32_t>(usage);
    }
}

TEST(ConversionsTest, BufferUsageRoundTrip_CombinedFlags)
{
    auto combined = static_cast<BufferUsage>(
        static_cast<uint32_t>(BufferUsage::Vertex) | static_cast<uint32_t>(BufferUsage::CopyDst));
    auto cUsage = cppBufferUsageToCUsage(combined);
    auto backToCpp = cBufferUsageToCppUsage(cUsage);
    EXPECT_EQ(backToCpp, combined);
}

// =============================================================================
// Texture Usage Conversions
// =============================================================================

TEST(ConversionsTest, TextureUsageRoundTrip_SingleFlags)
{
    std::vector<TextureUsage> usages = {
        TextureUsage::CopySrc,
        TextureUsage::CopyDst,
        TextureUsage::TextureBinding,
        TextureUsage::StorageBinding,
        TextureUsage::RenderAttachment,
    };

    for (const auto& usage : usages) {
        auto cUsage = cppTextureUsageToCUsage(usage);
        auto backToCpp = cTextureUsageToCppUsage(cUsage);
        EXPECT_EQ(backToCpp, usage) << "Failed roundtrip for usage " << static_cast<uint32_t>(usage);
    }
}

TEST(ConversionsTest, TextureUsageRoundTrip_CombinedFlags)
{
    auto combined = static_cast<TextureUsage>(
        static_cast<uint32_t>(TextureUsage::TextureBinding) | static_cast<uint32_t>(TextureUsage::RenderAttachment));
    auto cUsage = cppTextureUsageToCUsage(combined);
    auto backToCpp = cTextureUsageToCppUsage(cUsage);
    EXPECT_EQ(backToCpp, combined);
}

// =============================================================================
// Filter Mode Conversions
// =============================================================================

TEST(ConversionsTest, CppFilterModeToCFilterMode)
{
    EXPECT_EQ(cppFilterModeToCFilterMode(FilterMode::Nearest), GFX_FILTER_MODE_NEAREST);
    EXPECT_EQ(cppFilterModeToCFilterMode(FilterMode::Linear), GFX_FILTER_MODE_LINEAR);
}

// =============================================================================
// Pipeline Stage Conversions
// =============================================================================

TEST(ConversionsTest, CppPipelineStageToCPipelineStage)
{
    auto stage = static_cast<PipelineStage>(
        static_cast<uint32_t>(PipelineStage::VertexShader) | static_cast<uint32_t>(PipelineStage::FragmentShader));
    auto cStage = cppPipelineStageToCPipelineStage(stage);
    EXPECT_NE(cStage, 0);
}

// =============================================================================
// Access Flags Conversions
// =============================================================================

TEST(ConversionsTest, AccessFlagsRoundTrip)
{
    auto flags = static_cast<AccessFlags>(
        static_cast<uint32_t>(AccessFlags::ShaderRead) | static_cast<uint32_t>(AccessFlags::ShaderWrite));
    auto cFlags = cppAccessFlagsToCAccessFlags(flags);
    auto backToCpp = cAccessFlagsToCppAccessFlags(cFlags);
    EXPECT_EQ(backToCpp, flags);
}

// =============================================================================
// Device Limits Conversions
// =============================================================================

TEST(ConversionsTest, CDeviceLimitsToCppDeviceLimits)
{
    GfxDeviceLimits cLimits{
        .minUniformBufferOffsetAlignment = 256,
        .minStorageBufferOffsetAlignment = 128,
        .maxUniformBufferBindingSize = 65536,
        .maxStorageBufferBindingSize = 134217728,
        .maxBufferSize = 1073741824,
        .maxTextureDimension1D = 16384,
        .maxTextureDimension2D = 16384,
        .maxTextureDimension3D = 2048,
        .maxTextureArrayLayers = 2048
    };

    auto cppLimits = cDeviceLimitsToCppDeviceLimits(cLimits);

    EXPECT_EQ(cppLimits.minUniformBufferOffsetAlignment, 256);
    EXPECT_EQ(cppLimits.minStorageBufferOffsetAlignment, 128);
    EXPECT_EQ(cppLimits.maxUniformBufferBindingSize, 65536);
    EXPECT_EQ(cppLimits.maxStorageBufferBindingSize, 134217728);
    EXPECT_EQ(cppLimits.maxBufferSize, 1073741824);
    EXPECT_EQ(cppLimits.maxTextureDimension1D, 16384);
    EXPECT_EQ(cppLimits.maxTextureDimension2D, 16384);
    EXPECT_EQ(cppLimits.maxTextureDimension3D, 2048);
    EXPECT_EQ(cppLimits.maxTextureArrayLayers, 2048);
}

// =============================================================================
// Adapter Info Conversions
// =============================================================================

TEST(ConversionsTest, CAdapterInfoToCppAdapterInfo_WithStrings)
{
    const char* name = "Test GPU";
    const char* description = "Test Driver";
    GfxAdapterInfo cInfo{
        .name = name,
        .driverDescription = description,
        .vendorID = 0x10DE,
        .deviceID = 0x1234,
        .adapterType = GFX_ADAPTER_TYPE_DISCRETE_GPU,
        .backend = GFX_BACKEND_VULKAN
    };

    auto cppInfo = cAdapterInfoToCppAdapterInfo(cInfo);

    EXPECT_EQ(cppInfo.name, "Test GPU");
    EXPECT_EQ(cppInfo.driverDescription, "Test Driver");
    EXPECT_EQ(cppInfo.vendorID, 0x10DE);
    EXPECT_EQ(cppInfo.deviceID, 0x1234);
    EXPECT_EQ(cppInfo.adapterType, AdapterType::DiscreteGPU);
    EXPECT_EQ(cppInfo.backend, Backend::Vulkan);
}

TEST(ConversionsTest, CAdapterInfoToCppAdapterInfo_WithNullStrings)
{
    GfxAdapterInfo cInfo{
        .name = nullptr,
        .driverDescription = nullptr,
        .vendorID = 0,
        .deviceID = 0,
        .adapterType = GFX_ADAPTER_TYPE_UNKNOWN,
        .backend = GFX_BACKEND_AUTO
    };

    auto cppInfo = cAdapterInfoToCppAdapterInfo(cInfo);

    EXPECT_EQ(cppInfo.name, "Unknown");
    EXPECT_EQ(cppInfo.driverDescription, "");
    EXPECT_EQ(cppInfo.adapterType, AdapterType::Unknown);
    EXPECT_EQ(cppInfo.backend, Backend::Auto);
}

// =============================================================================
// Index Format Conversions
// =============================================================================

TEST(ConversionsTest, CppIndexFormatToCIndexFormat)
{
    EXPECT_EQ(cppIndexFormatToCIndexFormat(IndexFormat::Undefined), GFX_INDEX_FORMAT_UNDEFINED);
    EXPECT_EQ(cppIndexFormatToCIndexFormat(IndexFormat::Uint16), GFX_INDEX_FORMAT_UINT16);
    EXPECT_EQ(cppIndexFormatToCIndexFormat(IndexFormat::Uint32), GFX_INDEX_FORMAT_UINT32);
}

// =============================================================================
// Address Mode Conversions
// =============================================================================

TEST(ConversionsTest, CppAddressModeToCAddressMode)
{
    EXPECT_EQ(cppAddressModeToCAddressMode(AddressMode::Repeat), GFX_ADDRESS_MODE_REPEAT);
    EXPECT_EQ(cppAddressModeToCAddressMode(AddressMode::MirrorRepeat), GFX_ADDRESS_MODE_MIRROR_REPEAT);
    EXPECT_EQ(cppAddressModeToCAddressMode(AddressMode::ClampToEdge), GFX_ADDRESS_MODE_CLAMP_TO_EDGE);
}

// =============================================================================
// Filter Mode Reverse Conversions (missing C to C++)
// =============================================================================

TEST(ConversionsTest, CFilterModeToCppFilterMode)
{
    // Note: FilterMode conversion appears to only exist in C++ to C direction
    // If needed, add the reverse conversion function
    EXPECT_EQ(cppFilterModeToCFilterMode(FilterMode::Nearest), GFX_FILTER_MODE_NEAREST);
    EXPECT_EQ(cppFilterModeToCFilterMode(FilterMode::Linear), GFX_FILTER_MODE_LINEAR);
}

// =============================================================================
// Texture Type Conversions
// =============================================================================

TEST(ConversionsTest, TextureTypeRoundTrip)
{
    std::vector<TextureType> types = {
        TextureType::Texture1D,
        TextureType::Texture2D,
        TextureType::Texture3D,
    };

    for (const auto& type : types) {
        auto cType = cppTextureTypeToCType(type);
        auto backToCpp = cTextureTypeToCppType(cType);
        EXPECT_EQ(backToCpp, type) << "Failed roundtrip for type " << static_cast<int>(type);
    }
}

// =============================================================================
// Texture View Type Conversions
// =============================================================================

TEST(ConversionsTest, CppTextureViewTypeToCType)
{
    EXPECT_EQ(cppTextureViewTypeToCType(TextureViewType::View1D), GFX_TEXTURE_VIEW_TYPE_1D);
    EXPECT_EQ(cppTextureViewTypeToCType(TextureViewType::View2D), GFX_TEXTURE_VIEW_TYPE_2D);
    EXPECT_EQ(cppTextureViewTypeToCType(TextureViewType::View3D), GFX_TEXTURE_VIEW_TYPE_3D);
    EXPECT_EQ(cppTextureViewTypeToCType(TextureViewType::ViewCube), GFX_TEXTURE_VIEW_TYPE_CUBE);
    EXPECT_EQ(cppTextureViewTypeToCType(TextureViewType::View1DArray), GFX_TEXTURE_VIEW_TYPE_1D_ARRAY);
    EXPECT_EQ(cppTextureViewTypeToCType(TextureViewType::View2DArray), GFX_TEXTURE_VIEW_TYPE_2D_ARRAY);
    EXPECT_EQ(cppTextureViewTypeToCType(TextureViewType::ViewCubeArray), GFX_TEXTURE_VIEW_TYPE_CUBE_ARRAY);
}

// =============================================================================
// Semaphore Type Conversions
// =============================================================================

TEST(ConversionsTest, SemaphoreTypeRoundTrip)
{
    std::vector<SemaphoreType> types = {
        SemaphoreType::Binary,
        SemaphoreType::Timeline,
    };

    for (const auto& type : types) {
        auto cType = cppSemaphoreTypeToCSemaphoreType(type);
        auto backToCpp = cSemaphoreTypeToCppSemaphoreType(cType);
        EXPECT_EQ(backToCpp, type) << "Failed roundtrip for type " << static_cast<int>(type);
    }
}

// =============================================================================
// Query Type Conversions
// =============================================================================

TEST(ConversionsTest, QueryTypeRoundTrip)
{
    std::vector<QueryType> types = {
        QueryType::Occlusion,
        QueryType::Timestamp,
    };

    for (const auto& type : types) {
        auto cType = cppQueryTypeToCQueryType(type);
        auto backToCpp = cQueryTypeToCppQueryType(cType);
        EXPECT_EQ(backToCpp, type) << "Failed roundtrip for type " << static_cast<int>(type);
    }
}

// =============================================================================
// Shader Source Type Conversions
// =============================================================================

TEST(ConversionsTest, CppShaderSourceTypeToCShaderSourceType)
{
    EXPECT_EQ(cppShaderSourceTypeToCShaderSourceType(ShaderSourceType::SPIRV), GFX_SHADER_SOURCE_SPIRV);
    EXPECT_EQ(cppShaderSourceTypeToCShaderSourceType(ShaderSourceType::WGSL), GFX_SHADER_SOURCE_WGSL);
}

// =============================================================================
// Blend Operation Conversions
// =============================================================================

TEST(ConversionsTest, CppBlendOperationToCBlendOperation)
{
    EXPECT_EQ(cppBlendOperationToCBlendOperation(BlendOperation::Add), GFX_BLEND_OPERATION_ADD);
    EXPECT_EQ(cppBlendOperationToCBlendOperation(BlendOperation::Subtract), GFX_BLEND_OPERATION_SUBTRACT);
    EXPECT_EQ(cppBlendOperationToCBlendOperation(BlendOperation::ReverseSubtract), GFX_BLEND_OPERATION_REVERSE_SUBTRACT);
    EXPECT_EQ(cppBlendOperationToCBlendOperation(BlendOperation::Min), GFX_BLEND_OPERATION_MIN);
    EXPECT_EQ(cppBlendOperationToCBlendOperation(BlendOperation::Max), GFX_BLEND_OPERATION_MAX);
}

// =============================================================================
// Blend Factor Conversions
// =============================================================================

TEST(ConversionsTest, CppBlendFactorToCBlendFactor)
{
    EXPECT_EQ(cppBlendFactorToCBlendFactor(BlendFactor::Zero), GFX_BLEND_FACTOR_ZERO);
    EXPECT_EQ(cppBlendFactorToCBlendFactor(BlendFactor::One), GFX_BLEND_FACTOR_ONE);
    EXPECT_EQ(cppBlendFactorToCBlendFactor(BlendFactor::Src), GFX_BLEND_FACTOR_SRC);
    EXPECT_EQ(cppBlendFactorToCBlendFactor(BlendFactor::OneMinusSrc), GFX_BLEND_FACTOR_ONE_MINUS_SRC);
}

// =============================================================================
// Primitive Topology Conversions
// =============================================================================

TEST(ConversionsTest, CppPrimitiveTopologyToCPrimitiveTopology)
{
    EXPECT_EQ(cppPrimitiveTopologyToCPrimitiveTopology(PrimitiveTopology::PointList), GFX_PRIMITIVE_TOPOLOGY_POINT_LIST);
    EXPECT_EQ(cppPrimitiveTopologyToCPrimitiveTopology(PrimitiveTopology::LineList), GFX_PRIMITIVE_TOPOLOGY_LINE_LIST);
    EXPECT_EQ(cppPrimitiveTopologyToCPrimitiveTopology(PrimitiveTopology::LineStrip), GFX_PRIMITIVE_TOPOLOGY_LINE_STRIP);
    EXPECT_EQ(cppPrimitiveTopologyToCPrimitiveTopology(PrimitiveTopology::TriangleList), GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    EXPECT_EQ(cppPrimitiveTopologyToCPrimitiveTopology(PrimitiveTopology::TriangleStrip), GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
}

// =============================================================================
// Front Face Conversions
// =============================================================================

TEST(ConversionsTest, CppFrontFaceToCFrontFace)
{
    EXPECT_EQ(cppFrontFaceToCFrontFace(FrontFace::CounterClockwise), GFX_FRONT_FACE_COUNTER_CLOCKWISE);
    EXPECT_EQ(cppFrontFaceToCFrontFace(FrontFace::Clockwise), GFX_FRONT_FACE_CLOCKWISE);
}

// =============================================================================
// Cull Mode Conversions
// =============================================================================

TEST(ConversionsTest, CppCullModeToCCullMode)
{
    EXPECT_EQ(cppCullModeToCCullMode(CullMode::None), GFX_CULL_MODE_NONE);
    EXPECT_EQ(cppCullModeToCCullMode(CullMode::Front), GFX_CULL_MODE_FRONT);
    EXPECT_EQ(cppCullModeToCCullMode(CullMode::Back), GFX_CULL_MODE_BACK);
}

// =============================================================================
// Polygon Mode Conversions
// =============================================================================

TEST(ConversionsTest, CppPolygonModeToCPolygonMode)
{
    EXPECT_EQ(cppPolygonModeToCPolygonMode(PolygonMode::Fill), GFX_POLYGON_MODE_FILL);
    EXPECT_EQ(cppPolygonModeToCPolygonMode(PolygonMode::Line), GFX_POLYGON_MODE_LINE);
    EXPECT_EQ(cppPolygonModeToCPolygonMode(PolygonMode::Point), GFX_POLYGON_MODE_POINT);
}

// =============================================================================
// Compare Function Conversions
// =============================================================================

TEST(ConversionsTest, CppCompareFunctionToCCompareFunction)
{
    EXPECT_EQ(cppCompareFunctionToCCompareFunction(CompareFunction::Never), GFX_COMPARE_FUNCTION_NEVER);
    EXPECT_EQ(cppCompareFunctionToCCompareFunction(CompareFunction::Less), GFX_COMPARE_FUNCTION_LESS);
    EXPECT_EQ(cppCompareFunctionToCCompareFunction(CompareFunction::Equal), GFX_COMPARE_FUNCTION_EQUAL);
    EXPECT_EQ(cppCompareFunctionToCCompareFunction(CompareFunction::LessEqual), GFX_COMPARE_FUNCTION_LESS_EQUAL);
    EXPECT_EQ(cppCompareFunctionToCCompareFunction(CompareFunction::Greater), GFX_COMPARE_FUNCTION_GREATER);
    EXPECT_EQ(cppCompareFunctionToCCompareFunction(CompareFunction::NotEqual), GFX_COMPARE_FUNCTION_NOT_EQUAL);
    EXPECT_EQ(cppCompareFunctionToCCompareFunction(CompareFunction::GreaterEqual), GFX_COMPARE_FUNCTION_GREATER_EQUAL);
    EXPECT_EQ(cppCompareFunctionToCCompareFunction(CompareFunction::Always), GFX_COMPARE_FUNCTION_ALWAYS);
}

// =============================================================================
// Stencil Operation Conversions
// =============================================================================

TEST(ConversionsTest, CppStencilOperationToCStencilOperation)
{
    EXPECT_EQ(cppStencilOperationToCStencilOperation(StencilOperation::Keep), GFX_STENCIL_OPERATION_KEEP);
    EXPECT_EQ(cppStencilOperationToCStencilOperation(StencilOperation::Zero), GFX_STENCIL_OPERATION_ZERO);
    EXPECT_EQ(cppStencilOperationToCStencilOperation(StencilOperation::Replace), GFX_STENCIL_OPERATION_REPLACE);
    EXPECT_EQ(cppStencilOperationToCStencilOperation(StencilOperation::IncrementClamp), GFX_STENCIL_OPERATION_INCREMENT_CLAMP);
    EXPECT_EQ(cppStencilOperationToCStencilOperation(StencilOperation::DecrementClamp), GFX_STENCIL_OPERATION_DECREMENT_CLAMP);
    EXPECT_EQ(cppStencilOperationToCStencilOperation(StencilOperation::Invert), GFX_STENCIL_OPERATION_INVERT);
    EXPECT_EQ(cppStencilOperationToCStencilOperation(StencilOperation::IncrementWrap), GFX_STENCIL_OPERATION_INCREMENT_WRAP);
    EXPECT_EQ(cppStencilOperationToCStencilOperation(StencilOperation::DecrementWrap), GFX_STENCIL_OPERATION_DECREMENT_WRAP);
}

// =============================================================================
// Load/Store Op Conversions
// =============================================================================

TEST(ConversionsTest, CppLoadOpToCLoadOp)
{
    EXPECT_EQ(cppLoadOpToCLoadOp(LoadOp::Load), GFX_LOAD_OP_LOAD);
    EXPECT_EQ(cppLoadOpToCLoadOp(LoadOp::Clear), GFX_LOAD_OP_CLEAR);
    EXPECT_EQ(cppLoadOpToCLoadOp(LoadOp::DontCare), GFX_LOAD_OP_DONT_CARE);
}

TEST(ConversionsTest, CppStoreOpToCStoreOp)
{
    EXPECT_EQ(cppStoreOpToCStoreOp(StoreOp::Store), GFX_STORE_OP_STORE);
    EXPECT_EQ(cppStoreOpToCStoreOp(StoreOp::DontCare), GFX_STORE_OP_DONT_CARE);
}

// =============================================================================
// Adapter Preference Conversions
// =============================================================================

TEST(ConversionsTest, CppAdapterPreferenceToCAdapterPreference)
{
    EXPECT_EQ(cppAdapterPreferenceToCAdapterPreference(AdapterPreference::Undefined), GFX_ADAPTER_PREFERENCE_UNDEFINED);
    EXPECT_EQ(cppAdapterPreferenceToCAdapterPreference(AdapterPreference::HighPerformance), GFX_ADAPTER_PREFERENCE_HIGH_PERFORMANCE);
    EXPECT_EQ(cppAdapterPreferenceToCAdapterPreference(AdapterPreference::LowPower), GFX_ADAPTER_PREFERENCE_LOW_POWER);
}

// =============================================================================
// Shader Stage Conversions
// =============================================================================

TEST(ConversionsTest, CppShaderStageToCShaderStage_SingleFlags)
{
    EXPECT_EQ(cppShaderStageToCShaderStage(ShaderStage::Vertex), GFX_SHADER_STAGE_VERTEX);
    EXPECT_EQ(cppShaderStageToCShaderStage(ShaderStage::Fragment), GFX_SHADER_STAGE_FRAGMENT);
    EXPECT_EQ(cppShaderStageToCShaderStage(ShaderStage::Compute), GFX_SHADER_STAGE_COMPUTE);
}

TEST(ConversionsTest, CppShaderStageToCShaderStage_CombinedFlags)
{
    auto combined = static_cast<ShaderStage>(
        static_cast<uint32_t>(ShaderStage::Vertex) | static_cast<uint32_t>(ShaderStage::Fragment));
    auto cStage = cppShaderStageToCShaderStage(combined);
    EXPECT_EQ(cStage, GFX_FLAGS(GFX_SHADER_STAGE_VERTEX | GFX_SHADER_STAGE_FRAGMENT));
}

// =============================================================================
// Windowing System Conversions
// =============================================================================

TEST(ConversionsTest, CppWindowingSystemToC)
{
#ifdef GFX_HAS_WIN32
    EXPECT_EQ(cppWindowingSystemToC(WindowingSystem::Win32), GFX_WINDOWING_SYSTEM_WIN32);
#endif
#ifdef GFX_HAS_METAL
    EXPECT_EQ(cppWindowingSystemToC(WindowingSystem::Metal), GFX_WINDOWING_SYSTEM_METAL);
#endif
#ifdef GFX_HAS_X11
    EXPECT_EQ(cppWindowingSystemToC(WindowingSystem::Xlib), GFX_WINDOWING_SYSTEM_XLIB);
#endif
#ifdef GFX_HAS_WAYLAND
    EXPECT_EQ(cppWindowingSystemToC(WindowingSystem::Wayland), GFX_WINDOWING_SYSTEM_WAYLAND);
#endif
#ifdef GFX_HAS_XCB
    EXPECT_EQ(cppWindowingSystemToC(WindowingSystem::XCB), GFX_WINDOWING_SYSTEM_XCB);
#endif
#ifdef GFX_HAS_ANDROID
    EXPECT_EQ(cppWindowingSystemToC(WindowingSystem::Android), GFX_WINDOWING_SYSTEM_ANDROID);
#endif
#ifdef GFX_HAS_EMSCRIPTEN
    EXPECT_EQ(cppWindowingSystemToC(WindowingSystem::Emscripten), GFX_WINDOWING_SYSTEM_EMSCRIPTEN);
#endif
}

// =============================================================================
// Result Conversions
// =============================================================================

TEST(ConversionsTest, CResultToCppResult)
{
    EXPECT_EQ(cResultToCppResult(GFX_RESULT_SUCCESS), Result::Success);
    EXPECT_EQ(cResultToCppResult(GFX_RESULT_ERROR_UNKNOWN), Result::ErrorUnknown);
    EXPECT_EQ(cResultToCppResult(GFX_RESULT_ERROR_INVALID_ARGUMENT), Result::ErrorInvalidArgument);
    EXPECT_EQ(cResultToCppResult(GFX_RESULT_ERROR_OUT_OF_MEMORY), Result::ErrorOutOfMemory);
}

// =============================================================================
// Log Level Conversions
// =============================================================================

TEST(ConversionsTest, CLogLevelToCppLogLevel)
{
    EXPECT_EQ(cLogLevelToCppLogLevel(GFX_LOG_LEVEL_DEBUG), LogLevel::Debug);
    EXPECT_EQ(cLogLevelToCppLogLevel(GFX_LOG_LEVEL_INFO), LogLevel::Info);
    EXPECT_EQ(cLogLevelToCppLogLevel(GFX_LOG_LEVEL_WARNING), LogLevel::Warning);
    EXPECT_EQ(cLogLevelToCppLogLevel(GFX_LOG_LEVEL_ERROR), LogLevel::Error);
}

// =============================================================================
// Platform Window Handle Conversions
// =============================================================================

#ifdef GFX_HAS_X11
TEST(ConversionsTest, CppHandleToCHandle_Xlib)
{
    PlatformWindowHandle cppHandle = PlatformWindowHandle::fromXlib((void*)0x1234, 5678);
    GfxPlatformWindowHandle cHandle = cppHandleToCHandle(cppHandle);

    EXPECT_EQ(cHandle.windowingSystem, GFX_WINDOWING_SYSTEM_XLIB);
    EXPECT_EQ(cHandle.xlib.display, (void*)0x1234);
    EXPECT_EQ(cHandle.xlib.window, 5678);
}
#endif // GFX_HAS_X11

#ifdef GFX_HAS_WAYLAND
TEST(ConversionsTest, CppHandleToCHandle_Wayland)
{
    // fromWayland takes (display, surface) - display first
    PlatformWindowHandle cppHandle = PlatformWindowHandle::fromWayland((void*)0x1234, (void*)0x5678);
    GfxPlatformWindowHandle cHandle = cppHandleToCHandle(cppHandle);

    EXPECT_EQ(cHandle.windowingSystem, GFX_WINDOWING_SYSTEM_WAYLAND);
    EXPECT_EQ(cHandle.wayland.display, (void*)0x1234);
    EXPECT_EQ(cHandle.wayland.surface, (void*)0x5678);
}
#endif // GFX_HAS_WAYLAND

#ifdef GFX_HAS_XCB
TEST(ConversionsTest, CppHandleToCHandle_XCB)
{
    PlatformWindowHandle cppHandle = PlatformWindowHandle::fromXCB((void*)0x1234, 5678);
    GfxPlatformWindowHandle cHandle = cppHandleToCHandle(cppHandle);

    EXPECT_EQ(cHandle.windowingSystem, GFX_WINDOWING_SYSTEM_XCB);
    EXPECT_EQ(cHandle.xcb.connection, (void*)0x1234);
    EXPECT_EQ(cHandle.xcb.window, 5678);
}
#endif // GFX_HAS_XCB

// =============================================================================
// Queue Family Properties Conversions
// =============================================================================

TEST(ConversionsTest, CQueueFamilyPropertiesToCppQueueFamilyProperties)
{
    GfxQueueFamilyProperties cProps{
        .flags = GFX_FLAGS(GFX_QUEUE_FLAG_GRAPHICS | GFX_QUEUE_FLAG_COMPUTE),
        .queueCount = 4
    };

    auto cppProps = cQueueFamilyPropertiesToCppQueueFamilyProperties(cProps);

    EXPECT_TRUE(static_cast<bool>(cppProps.flags & QueueFlags::Graphics));
    EXPECT_TRUE(static_cast<bool>(cppProps.flags & QueueFlags::Compute));
    EXPECT_EQ(cppProps.queueCount, 4);
}

// =============================================================================
// Queue Request Conversions
// =============================================================================

TEST(ConversionsTest, CppQueueRequestToCQueueRequest)
{
    QueueRequest cppReq{
        .queueFamilyIndex = 2,
        .queueIndex = 1
    };

    GfxQueueRequest cReq = cppQueueRequestToCQueueRequest(cppReq);

    EXPECT_EQ(cReq.queueFamilyIndex, 2);
    EXPECT_EQ(cReq.queueIndex, 1);
}

// =============================================================================
// Buffer/Texture/Swapchain Info Conversions
// =============================================================================

TEST(ConversionsTest, CBufferInfoToCppBufferInfo)
{
    GfxBufferInfo cInfo{
        .size = 4096,
        .usage = GFX_FLAGS(GFX_BUFFER_USAGE_VERTEX | GFX_BUFFER_USAGE_COPY_DST)
    };

    auto cppInfo = cBufferInfoToCppBufferInfo(cInfo);

    EXPECT_EQ(cppInfo.size, 4096);
    EXPECT_TRUE(static_cast<bool>(cppInfo.usage & BufferUsage::Vertex));
    EXPECT_TRUE(static_cast<bool>(cppInfo.usage & BufferUsage::CopyDst));
}

TEST(ConversionsTest, CTextureInfoToCppTextureInfo)
{
    GfxTextureInfo cInfo{
        .type = GFX_TEXTURE_TYPE_2D,
        .size = { 512, 512, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = GFX_SAMPLE_COUNT_1,
        .format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM,
        .usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT
    };

    auto cppInfo = cTextureInfoToCppTextureInfo(cInfo);

    EXPECT_EQ(cppInfo.type, TextureType::Texture2D);
    EXPECT_EQ(cppInfo.format, TextureFormat::R8G8B8A8Unorm);
    EXPECT_EQ(cppInfo.size.width, 512);
    EXPECT_EQ(cppInfo.size.height, 512);
    EXPECT_EQ(cppInfo.mipLevelCount, 1);
    EXPECT_TRUE(static_cast<bool>(cppInfo.usage & TextureUsage::RenderAttachment));
}

TEST(ConversionsTest, CSwapchainInfoToCppSwapchainInfo)
{
    GfxSwapchainInfo cInfo{
        .extent = { 1920, 1080 },
        .format = GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM,
        .imageCount = 3,
        .presentMode = GFX_PRESENT_MODE_MAILBOX
    };

    auto cppInfo = cSwapchainInfoToCppSwapchainInfo(cInfo);

    EXPECT_EQ(cppInfo.format, TextureFormat::B8G8R8A8Unorm);
    EXPECT_EQ(cppInfo.extent.width, 1920);
    EXPECT_EQ(cppInfo.extent.height, 1080);
    EXPECT_EQ(cppInfo.imageCount, 3);
    EXPECT_EQ(cppInfo.presentMode, PresentMode::Mailbox);
}

// =============================================================================
// Barrier Conversions
// =============================================================================

TEST(ConversionsTest, ConvertMemoryBarrier)
{
    MemoryBarrier cppBarrier{
        .srcAccessMask = AccessFlags::ShaderWrite,
        .dstAccessMask = AccessFlags::ShaderRead
    };

    GfxMemoryBarrier cBarrier;
    convertMemoryBarrier(cppBarrier, cBarrier);

    EXPECT_EQ(cBarrier.srcAccessMask, GFX_ACCESS_SHADER_WRITE);
    EXPECT_EQ(cBarrier.dstAccessMask, GFX_ACCESS_SHADER_READ);
}

// ConvertBufferBarrier test removed - requires BufferImpl implementation class

// ConvertTextureBarrier test removed - requires TextureImpl implementation class

// =============================================================================
// Descriptor Conversions - Basic Descriptors
// =============================================================================

TEST(ConversionsTest, ConvertCommandEncoderDescriptor)
{
    CommandEncoderDescriptor cppDesc{
        .label = "TestEncoder"
    };

    GfxCommandEncoderDescriptor cDesc;
    convertCommandEncoderDescriptor(cppDesc, cDesc);

    EXPECT_STREQ(cDesc.label, "TestEncoder");
}

TEST(ConversionsTest, ConvertFenceDescriptor)
{
    FenceDescriptor cppDesc{
        .signaled = true
    };

    GfxFenceDescriptor cDesc;
    convertFenceDescriptor(cppDesc, cDesc);

    EXPECT_EQ(cDesc.signaled, true);
}

TEST(ConversionsTest, ConvertSemaphoreDescriptor)
{
    SemaphoreDescriptor cppDesc{
        .type = SemaphoreType::Timeline,
        .initialValue = 42
    };

    GfxSemaphoreDescriptor cDesc;
    convertSemaphoreDescriptor(cppDesc, cDesc);

    EXPECT_EQ(cDesc.type, GFX_SEMAPHORE_TYPE_TIMELINE);
    EXPECT_EQ(cDesc.initialValue, 42);
}

TEST(ConversionsTest, ConvertQuerySetDescriptor)
{
    QuerySetDescriptor cppDesc{
        .type = QueryType::Timestamp,
        .count = 8
    };

    GfxQuerySetDescriptor cDesc;
    convertQuerySetDescriptor(cppDesc, cDesc);

    EXPECT_EQ(cDesc.type, GFX_QUERY_TYPE_TIMESTAMP);
    EXPECT_EQ(cDesc.count, 8);
}

TEST(ConversionsTest, ConvertBufferDescriptor)
{
    BufferDescriptor cppDesc{
        .label = "UniformBuffer",
        .size = 2048,
        .usage = BufferUsage::Uniform | BufferUsage::CopyDst
    };

    GfxBufferDescriptor cDesc;
    convertBufferDescriptor(cppDesc, cDesc);

    EXPECT_EQ(cDesc.size, 2048);
    EXPECT_EQ(cDesc.usage, GFX_BUFFER_USAGE_UNIFORM | GFX_BUFFER_USAGE_COPY_DST);
    EXPECT_STREQ(cDesc.label, "UniformBuffer");
}

TEST(ConversionsTest, ConvertTextureDescriptor)
{
    TextureDescriptor cppDesc{
        .label = "TestTexture",
        .type = TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = SampleCount::Count1,
        .format = TextureFormat::R8G8B8A8Unorm,
        .usage = TextureUsage::TextureBinding | TextureUsage::CopyDst
    };

    GfxTextureDescriptor cDesc;
    convertTextureDescriptor(cppDesc, cDesc);

    EXPECT_EQ(cDesc.type, GFX_TEXTURE_TYPE_2D);
    EXPECT_EQ(cDesc.size.width, 256);
    EXPECT_EQ(cDesc.size.height, 256);
    EXPECT_EQ(cDesc.format, GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM);
    EXPECT_EQ(cDesc.usage, GFX_TEXTURE_USAGE_TEXTURE_BINDING | GFX_TEXTURE_USAGE_COPY_DST);
    EXPECT_STREQ(cDesc.label, "TestTexture");
}

TEST(ConversionsTest, ConvertTextureViewDescriptor)
{
    TextureViewDescriptor cppDesc{
        .viewType = TextureViewType::View2D,
        .format = TextureFormat::R8G8B8A8Unorm,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1
    };

    GfxTextureViewDescriptor cDesc;
    convertTextureViewDescriptor(cppDesc, cDesc);

    EXPECT_EQ(cDesc.viewType, GFX_TEXTURE_VIEW_TYPE_2D);
    EXPECT_EQ(cDesc.format, GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM);
    EXPECT_EQ(cDesc.baseMipLevel, 0);
    EXPECT_EQ(cDesc.mipLevelCount, 1);
}

TEST(ConversionsTest, ConvertSamplerDescriptor)
{
    SamplerDescriptor cppDesc{
        .addressModeU = AddressMode::Repeat,
        .addressModeV = AddressMode::Repeat,
        .addressModeW = AddressMode::Repeat,
        .magFilter = FilterMode::Linear,
        .minFilter = FilterMode::Linear,
        .mipmapFilter = FilterMode::Linear,
        .maxAnisotropy = 16
    };

    GfxSamplerDescriptor cDesc;
    convertSamplerDescriptor(cppDesc, cDesc);

    EXPECT_EQ(cDesc.magFilter, GFX_FILTER_MODE_LINEAR);
    EXPECT_EQ(cDesc.minFilter, GFX_FILTER_MODE_LINEAR);
    EXPECT_EQ(cDesc.addressModeU, GFX_ADDRESS_MODE_REPEAT);
    EXPECT_EQ(cDesc.maxAnisotropy, 16);
}

// ConvertShaderDescriptor test removed - ShaderDescriptor structure doesn't have source/sourceSize fields

TEST(ConversionsTest, ConvertComputePassBeginDescriptor)
{
    ComputePassBeginDescriptor cppDesc{
        .label = "ComputePass"
    };

    GfxComputePassBeginDescriptor cDesc;
    convertComputePassBeginDescriptor(cppDesc, cDesc);

    EXPECT_STREQ(cDesc.label, "ComputePass");
}

TEST(ConversionsTest, ConvertPrimitiveState)
{
    PrimitiveState cppState{
        .topology = PrimitiveTopology::TriangleList,
        .frontFace = FrontFace::CounterClockwise,
        .cullMode = CullMode::Back
    };

    GfxPrimitiveState cState;
    convertPrimitiveState(cppState, cState);

    EXPECT_EQ(cState.topology, GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    EXPECT_EQ(cState.frontFace, GFX_FRONT_FACE_COUNTER_CLOCKWISE);
    EXPECT_EQ(cState.cullMode, GFX_CULL_MODE_BACK);
}

TEST(ConversionsTest, ConvertDepthStencilState)
{
    DepthStencilState cppState{
        .depthWriteEnabled = true,
        .depthCompare = CompareFunction::Less,
        .stencilReadMask = 0xFF,
        .stencilWriteMask = 0xFF
    };

    GfxDepthStencilState cState;
    convertDepthStencilState(cppState, cState);

    EXPECT_EQ(cState.depthWriteEnabled, true);
    EXPECT_EQ(cState.depthCompare, GFX_COMPARE_FUNCTION_LESS);
    EXPECT_EQ(cState.stencilReadMask, 0xFF);
}

// =============================================================================
// Surface Info Conversion Tests
// =============================================================================

TEST(GfxCppConversionsTest, CSurfaceInfoToCppSurfaceInfo_ConvertsCorrectly)
{
    GfxSurfaceInfo cInfo{};
    cInfo.minImageCount = 2;
    cInfo.maxImageCount = 3;
    cInfo.minExtent.width = 1;
    cInfo.minExtent.height = 1;
    cInfo.maxExtent.width = 4096;
    cInfo.maxExtent.height = 4096;

    gfx::SurfaceInfo result = gfx::cSurfaceInfoToCppSurfaceInfo(cInfo);

    EXPECT_EQ(result.minImageCount, 2u);
    EXPECT_EQ(result.maxImageCount, 3u);
    EXPECT_EQ(result.minExtent.width, 1u);
    EXPECT_EQ(result.minExtent.height, 1u);
    EXPECT_EQ(result.maxExtent.width, 4096u);
    EXPECT_EQ(result.maxExtent.height, 4096u);
}

TEST(GfxCppConversionsTest, CSurfaceInfoToCppSurfaceInfo_ZeroValues_ConvertsCorrectly)
{
    GfxSurfaceInfo cInfo{};
    cInfo.minImageCount = 0;
    cInfo.maxImageCount = 0;
    cInfo.minExtent.width = 0;
    cInfo.minExtent.height = 0;
    cInfo.maxExtent.width = 0;
    cInfo.maxExtent.height = 0;

    gfx::SurfaceInfo result = gfx::cSurfaceInfoToCppSurfaceInfo(cInfo);

    EXPECT_EQ(result.minImageCount, 0u);
    EXPECT_EQ(result.maxImageCount, 0u);
    EXPECT_EQ(result.minExtent.width, 0u);
    EXPECT_EQ(result.minExtent.height, 0u);
    EXPECT_EQ(result.maxExtent.width, 0u);
    EXPECT_EQ(result.maxExtent.height, 0u);
}

// =============================================================================
// Pipeline Layout and Bind Group Conversions
// =============================================================================

// TOOD

// =============================================================================
// Render Pipeline Conversions
// =============================================================================

// TOOD

// =============================================================================
// Render Pass Conversions
// =============================================================================

// TOOD

// =============================================================================
// Submit and Copy Conversions
// =============================================================================

// TOOD

// =============================================================================
// Instance and Device Conversions
// =============================================================================

// TOOD

} // namespace gfx
