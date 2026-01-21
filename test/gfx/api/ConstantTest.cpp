#include <gfx/gfx.h>

#include <gtest/gtest.h>

// C API tests compiled with C++ for GoogleTest compatibility

// Basic sanity test
TEST(GfxConstantTest, LibraryVersion) {
    // Just verify we can link against the library
    EXPECT_TRUE(true);
}

TEST(GfxConstantTest, ResultEnumValues) {
    // Verify enum values are correct
    EXPECT_EQ(GFX_RESULT_SUCCESS, 0);
    EXPECT_EQ(GFX_RESULT_TIMEOUT, 1);
    EXPECT_EQ(GFX_RESULT_NOT_READY, 2);
    EXPECT_LT(GFX_RESULT_ERROR_INVALID_ARGUMENT, 0);
}

TEST(GfxConstantTest, BackendEnumValues) {
    // Verify backend enum values
    EXPECT_EQ(GFX_BACKEND_VULKAN, 0);
    EXPECT_EQ(GFX_BACKEND_WEBGPU, 1);
    EXPECT_EQ(GFX_BACKEND_AUTO, 2);
}

TEST(GfxConstantTest, AdapterTypeEnumValues) {
    // Verify adapter type enum values
    EXPECT_EQ(GFX_ADAPTER_TYPE_DISCRETE_GPU, 0);
    EXPECT_EQ(GFX_ADAPTER_TYPE_INTEGRATED_GPU, 1);
    EXPECT_EQ(GFX_ADAPTER_TYPE_CPU, 2);
    EXPECT_EQ(GFX_ADAPTER_TYPE_UNKNOWN, 3);
}

TEST(GfxConstantTest, TextureFormatEnumValues) {
    // Verify texture format enum values
    EXPECT_EQ(GFX_TEXTURE_FORMAT_UNDEFINED, 0);
    EXPECT_EQ(GFX_TEXTURE_FORMAT_R8_UNORM, 1);
    EXPECT_EQ(GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM, 3);
    EXPECT_EQ(GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM, 5);
}

TEST(GfxConstantTest, BufferUsageFlagValues) {
    // Verify buffer usage flags are bitmasks
    EXPECT_EQ(GFX_BUFFER_USAGE_NONE, 0);
    EXPECT_EQ(GFX_BUFFER_USAGE_MAP_READ, 1 << 0);
    EXPECT_EQ(GFX_BUFFER_USAGE_MAP_WRITE, 1 << 1);
    EXPECT_EQ(GFX_BUFFER_USAGE_VERTEX, 1 << 5);
    EXPECT_EQ(GFX_BUFFER_USAGE_UNIFORM, 1 << 6);
}

TEST(GfxConstantTest, TextureUsageFlagValues) {
    // Verify texture usage flags are bitmasks
    EXPECT_EQ(GFX_TEXTURE_USAGE_NONE, 0);
    EXPECT_EQ(GFX_TEXTURE_USAGE_COPY_SRC, 1 << 0);
    EXPECT_EQ(GFX_TEXTURE_USAGE_COPY_DST, 1 << 1);
    EXPECT_EQ(GFX_TEXTURE_USAGE_TEXTURE_BINDING, 1 << 2);
    EXPECT_EQ(GFX_TEXTURE_USAGE_RENDER_ATTACHMENT, 1 << 4);
}

TEST(GfxConstantTest, ShaderStageFlags) {
    // Verify shader stage flags are bitmasks
    EXPECT_EQ(GFX_SHADER_STAGE_NONE, 0);
    EXPECT_EQ(GFX_SHADER_STAGE_VERTEX, 1 << 0);
    EXPECT_EQ(GFX_SHADER_STAGE_FRAGMENT, 1 << 1);
    EXPECT_EQ(GFX_SHADER_STAGE_COMPUTE, 1 << 2);
}

TEST(GfxConstantTest, QueueFlags) {
    // Verify queue flags are bitmasks
    EXPECT_EQ(GFX_QUEUE_FLAG_NONE, 0);
    EXPECT_EQ(GFX_QUEUE_FLAG_GRAPHICS, 1 << 0);
    EXPECT_EQ(GFX_QUEUE_FLAG_COMPUTE, 1 << 1);
    EXPECT_EQ(GFX_QUEUE_FLAG_TRANSFER, 1 << 2);
}

TEST(GfxConstantTest, PrimitiveTopologyValues) {
    // Verify primitive topology values
    EXPECT_EQ(GFX_PRIMITIVE_TOPOLOGY_POINT_LIST, 0);
    EXPECT_EQ(GFX_PRIMITIVE_TOPOLOGY_LINE_LIST, 1);
    EXPECT_EQ(GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 3);
}

TEST(GfxConstantTest, IndexFormatValues) {
    // Verify index format values
    EXPECT_EQ(GFX_INDEX_FORMAT_UNDEFINED, 0);
    EXPECT_EQ(GFX_INDEX_FORMAT_UINT16, 1);
    EXPECT_EQ(GFX_INDEX_FORMAT_UINT32, 2);
}

TEST(GfxConstantTest, SampleCountValues) {
    // Verify sample count values
    EXPECT_EQ(GFX_SAMPLE_COUNT_1, 1);
    EXPECT_EQ(GFX_SAMPLE_COUNT_2, 2);
    EXPECT_EQ(GFX_SAMPLE_COUNT_4, 4);
    EXPECT_EQ(GFX_SAMPLE_COUNT_8, 8);
}

TEST(GfxConstantTest, CompareFunctionValues) {
    // Verify compare function values
    EXPECT_EQ(GFX_COMPARE_FUNCTION_UNDEFINED, 0);
    EXPECT_EQ(GFX_COMPARE_FUNCTION_NEVER, 1);
    EXPECT_EQ(GFX_COMPARE_FUNCTION_LESS, 2);
    EXPECT_EQ(GFX_COMPARE_FUNCTION_ALWAYS, 8);
}

TEST(GfxConstantTest, LoadStoreOpValues) {
    // Verify load/store operation values
    EXPECT_EQ(GFX_LOAD_OP_LOAD, 0);
    EXPECT_EQ(GFX_LOAD_OP_CLEAR, 1);
    EXPECT_EQ(GFX_LOAD_OP_DONT_CARE, 2);
    EXPECT_EQ(GFX_STORE_OP_STORE, 0);
    EXPECT_EQ(GFX_STORE_OP_DONT_CARE, 1);
}

TEST(GfxConstantTest, PresentModeValues) {
    // Verify present mode values
    EXPECT_EQ(GFX_PRESENT_MODE_IMMEDIATE, 0);
    EXPECT_EQ(GFX_PRESENT_MODE_FIFO, 1);
    EXPECT_EQ(GFX_PRESENT_MODE_FIFO_RELAXED, 2);
    EXPECT_EQ(GFX_PRESENT_MODE_MAILBOX, 3);
}

TEST(GfxConstantTest, CullModeValues) {
    // Verify cull mode values
    EXPECT_EQ(GFX_CULL_MODE_NONE, 0);
    EXPECT_EQ(GFX_CULL_MODE_FRONT, 1);
    EXPECT_EQ(GFX_CULL_MODE_BACK, 2);
    EXPECT_EQ(GFX_CULL_MODE_FRONT_AND_BACK, 3);
}

TEST(GfxConstantTest, FrontFaceValues) {
    // Verify front face values
    EXPECT_EQ(GFX_FRONT_FACE_COUNTER_CLOCKWISE, 0);
    EXPECT_EQ(GFX_FRONT_FACE_CLOCKWISE, 1);
}

TEST(GfxConstantTest, BlendOperationValues) {
    // Verify blend operation values
    EXPECT_EQ(GFX_BLEND_OPERATION_ADD, 0);
    EXPECT_EQ(GFX_BLEND_OPERATION_SUBTRACT, 1);
    EXPECT_EQ(GFX_BLEND_OPERATION_REVERSE_SUBTRACT, 2);
    EXPECT_EQ(GFX_BLEND_OPERATION_MIN, 3);
    EXPECT_EQ(GFX_BLEND_OPERATION_MAX, 4);
}

TEST(GfxConstantTest, BlendFactorValues) {
    // Verify blend factor values
    EXPECT_EQ(GFX_BLEND_FACTOR_ZERO, 0);
    EXPECT_EQ(GFX_BLEND_FACTOR_ONE, 1);
    EXPECT_EQ(GFX_BLEND_FACTOR_SRC_ALPHA, 4);
    EXPECT_EQ(GFX_BLEND_FACTOR_DST_ALPHA, 8);
}

TEST(GfxConstantTest, ColorWriteMaskValues) {
    // Verify color write mask flags
    EXPECT_EQ(GFX_COLOR_WRITE_MASK_NONE, 0x0);
    EXPECT_EQ(GFX_COLOR_WRITE_MASK_RED, 0x1);
    EXPECT_EQ(GFX_COLOR_WRITE_MASK_GREEN, 0x2);
    EXPECT_EQ(GFX_COLOR_WRITE_MASK_BLUE, 0x4);
    EXPECT_EQ(GFX_COLOR_WRITE_MASK_ALPHA, 0x8);
    EXPECT_EQ(GFX_COLOR_WRITE_MASK_ALL, 0xF);
}

TEST(GfxConstantTest, TextureTypeValues) {
    // Verify texture type values
    EXPECT_EQ(GFX_TEXTURE_TYPE_1D, 0);
    EXPECT_EQ(GFX_TEXTURE_TYPE_2D, 1);
    EXPECT_EQ(GFX_TEXTURE_TYPE_3D, 2);
    EXPECT_EQ(GFX_TEXTURE_TYPE_CUBE, 3);
}

TEST(GfxConstantTest, FilterModeValues) {
    // Verify filter mode values
    EXPECT_EQ(GFX_FILTER_MODE_NEAREST, 0);
    EXPECT_EQ(GFX_FILTER_MODE_LINEAR, 1);
}

TEST(GfxConstantTest, AddressModeValues) {
    // Verify address mode values
    EXPECT_EQ(GFX_ADDRESS_MODE_REPEAT, 0);
    EXPECT_EQ(GFX_ADDRESS_MODE_MIRROR_REPEAT, 1);
    EXPECT_EQ(GFX_ADDRESS_MODE_CLAMP_TO_EDGE, 2);
}

TEST(GfxConstantTest, PolygonModeValues) {
    // Verify polygon mode values
    EXPECT_EQ(GFX_POLYGON_MODE_FILL, 0);
    EXPECT_EQ(GFX_POLYGON_MODE_LINE, 1);
    EXPECT_EQ(GFX_POLYGON_MODE_POINT, 2);
}

TEST(GfxConstantTest, TextureLayoutValues) {
    // Verify texture layout values
    EXPECT_EQ(GFX_TEXTURE_LAYOUT_UNDEFINED, 0);
    EXPECT_EQ(GFX_TEXTURE_LAYOUT_GENERAL, 1);
    EXPECT_EQ(GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT, 2);
    EXPECT_EQ(GFX_TEXTURE_LAYOUT_SHADER_READ_ONLY, 5);
    EXPECT_EQ(GFX_TEXTURE_LAYOUT_PRESENT_SRC, 8);
}

TEST(GfxConstantTest, ShaderSourceTypeValues) {
    // Verify shader source type values
    EXPECT_EQ(GFX_SHADER_SOURCE_WGSL, 0);
    EXPECT_EQ(GFX_SHADER_SOURCE_SPIRV, 1);
}

TEST(GfxConstantTest, AdapterPreferenceValues) {
    // Verify adapter preference values
    EXPECT_EQ(GFX_ADAPTER_PREFERENCE_UNDEFINED, 0);
    EXPECT_EQ(GFX_ADAPTER_PREFERENCE_LOW_POWER, 1);
    EXPECT_EQ(GFX_ADAPTER_PREFERENCE_HIGH_PERFORMANCE, 2);
    EXPECT_EQ(GFX_ADAPTER_PREFERENCE_SOFTWARE, 3);
}
