#include <backend/webgpu/core/system/Adapter.h>
#include <backend/webgpu/core/system/Instance.h>
#include <backend/webgpu/core/util/Utils.h>

#include <gtest/gtest.h>

// Test WebGPU core utility functions
// These tests verify the internal utility implementations

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class WebGPUUtilsTest : public testing::Test {
protected:
    void SetUp() override
    {
        try {
            gfx::backend::webgpu::core::InstanceCreateInfo instInfo{};
            instance = std::make_unique<gfx::backend::webgpu::core::Instance>(instInfo);

            gfx::backend::webgpu::core::AdapterCreateInfo adapterInfo{};
            adapterInfo.adapterIndex = 0;
            adapter = instance->requestAdapter(adapterInfo);
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up WebGPU: " << e.what();
        }
    }

    std::unique_ptr<gfx::backend::webgpu::core::Instance> instance;
    gfx::backend::webgpu::core::Adapter* adapter = nullptr;
};

// ============================================================================
// Stencil Format Tests
// ============================================================================

TEST_F(WebGPUUtilsTest, HasStencil_Depth24PlusStencil8_ReturnsTrue)
{
    bool result = gfx::backend::webgpu::core::hasStencil(WGPUTextureFormat_Depth24PlusStencil8);
    EXPECT_TRUE(result);
}

TEST_F(WebGPUUtilsTest, HasStencil_Depth32FloatStencil8_ReturnsTrue)
{
    bool result = gfx::backend::webgpu::core::hasStencil(WGPUTextureFormat_Depth32FloatStencil8);
    EXPECT_TRUE(result);
}

TEST_F(WebGPUUtilsTest, HasStencil_Stencil8_ReturnsTrue)
{
    bool result = gfx::backend::webgpu::core::hasStencil(WGPUTextureFormat_Stencil8);
    EXPECT_TRUE(result);
}

TEST_F(WebGPUUtilsTest, HasStencil_Depth32Float_ReturnsFalse)
{
    bool result = gfx::backend::webgpu::core::hasStencil(WGPUTextureFormat_Depth32Float);
    EXPECT_FALSE(result);
}

TEST_F(WebGPUUtilsTest, HasStencil_Depth16Unorm_ReturnsFalse)
{
    bool result = gfx::backend::webgpu::core::hasStencil(WGPUTextureFormat_Depth16Unorm);
    EXPECT_FALSE(result);
}

TEST_F(WebGPUUtilsTest, HasStencil_Depth24Plus_ReturnsFalse)
{
    bool result = gfx::backend::webgpu::core::hasStencil(WGPUTextureFormat_Depth24Plus);
    EXPECT_FALSE(result);
}

TEST_F(WebGPUUtilsTest, HasStencil_ColorFormat_ReturnsFalse)
{
    bool result = gfx::backend::webgpu::core::hasStencil(WGPUTextureFormat_RGBA8Unorm);
    EXPECT_FALSE(result);
}

TEST_F(WebGPUUtilsTest, HasStencil_UndefinedFormat_ReturnsFalse)
{
    bool result = gfx::backend::webgpu::core::hasStencil(WGPUTextureFormat_Undefined);
    EXPECT_FALSE(result);
}

// ============================================================================
// Format Bytes Per Pixel Tests
// ============================================================================

TEST_F(WebGPUUtilsTest, GetFormatBytesPerPixel_R8Formats_Returns1)
{
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_R8Unorm), 1u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_R8Snorm), 1u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_R8Uint), 1u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_R8Sint), 1u);
}

TEST_F(WebGPUUtilsTest, GetFormatBytesPerPixel_R16Formats_Returns2)
{
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_R16Uint), 2u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_R16Sint), 2u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_R16Float), 2u);
}

TEST_F(WebGPUUtilsTest, GetFormatBytesPerPixel_RG8Formats_Returns2)
{
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RG8Unorm), 2u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RG8Snorm), 2u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RG8Uint), 2u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RG8Sint), 2u);
}

TEST_F(WebGPUUtilsTest, GetFormatBytesPerPixel_R32Formats_Returns4)
{
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_R32Float), 4u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_R32Uint), 4u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_R32Sint), 4u);
}

TEST_F(WebGPUUtilsTest, GetFormatBytesPerPixel_RGBA8Formats_Returns4)
{
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RGBA8Unorm), 4u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RGBA8UnormSrgb), 4u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RGBA8Snorm), 4u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RGBA8Uint), 4u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RGBA8Sint), 4u);
}

TEST_F(WebGPUUtilsTest, GetFormatBytesPerPixel_BGRA8Formats_Returns4)
{
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_BGRA8Unorm), 4u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_BGRA8UnormSrgb), 4u);
}

TEST_F(WebGPUUtilsTest, GetFormatBytesPerPixel_SpecialRGBFormats_Returns4)
{
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RGB10A2Unorm), 4u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RG11B10Ufloat), 4u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RGB9E5Ufloat), 4u);
}

TEST_F(WebGPUUtilsTest, GetFormatBytesPerPixel_RG32Formats_Returns8)
{
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RG32Float), 8u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RG32Uint), 8u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RG32Sint), 8u);
}

TEST_F(WebGPUUtilsTest, GetFormatBytesPerPixel_RGBA16Formats_Returns8)
{
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RGBA16Uint), 8u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RGBA16Sint), 8u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RGBA16Float), 8u);
}

TEST_F(WebGPUUtilsTest, GetFormatBytesPerPixel_RGBA32Formats_Returns16)
{
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RGBA32Float), 16u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RGBA32Uint), 16u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_RGBA32Sint), 16u);
}

TEST_F(WebGPUUtilsTest, GetFormatBytesPerPixel_DepthFormats_ReturnsCorrectSize)
{
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_Depth16Unorm), 2u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_Depth24Plus), 4u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_Depth24PlusStencil8), 4u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_Depth32Float), 4u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_Depth32FloatStencil8), 8u);
}

TEST_F(WebGPUUtilsTest, GetFormatBytesPerPixel_Stencil8_Returns1)
{
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_Stencil8), 1u);
}

TEST_F(WebGPUUtilsTest, GetFormatBytesPerPixel_BC1Formats_Returns8)
{
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_BC1RGBAUnorm), 8u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_BC1RGBAUnormSrgb), 8u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_BC4RUnorm), 8u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_BC4RSnorm), 8u);
}

TEST_F(WebGPUUtilsTest, GetFormatBytesPerPixel_BC2BC3Formats_Returns16)
{
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_BC2RGBAUnorm), 16u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_BC2RGBAUnormSrgb), 16u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_BC3RGBAUnorm), 16u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_BC3RGBAUnormSrgb), 16u);
}

TEST_F(WebGPUUtilsTest, GetFormatBytesPerPixel_BC5BC6BC7Formats_Returns16)
{
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_BC5RGUnorm), 16u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_BC5RGSnorm), 16u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_BC6HRGBUfloat), 16u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_BC6HRGBFloat), 16u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_BC7RGBAUnorm), 16u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_BC7RGBAUnormSrgb), 16u);
}

TEST_F(WebGPUUtilsTest, GetFormatBytesPerPixel_ETC2Formats_ReturnsCorrectSize)
{
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_ETC2RGB8Unorm), 8u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_ETC2RGB8UnormSrgb), 8u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_ETC2RGB8A1Unorm), 8u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_ETC2RGB8A1UnormSrgb), 8u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_ETC2RGBA8Unorm), 16u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_ETC2RGBA8UnormSrgb), 16u);
}

TEST_F(WebGPUUtilsTest, GetFormatBytesPerPixel_EACFormats_ReturnsCorrectSize)
{
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_EACR11Unorm), 8u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_EACR11Snorm), 8u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_EACRG11Unorm), 16u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_EACRG11Snorm), 16u);
}

TEST_F(WebGPUUtilsTest, GetFormatBytesPerPixel_ASTCFormats_Returns16)
{
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_ASTC4x4Unorm), 16u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_ASTC4x4UnormSrgb), 16u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_ASTC8x8Unorm), 16u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_ASTC8x8UnormSrgb), 16u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_ASTC12x12Unorm), 16u);
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_ASTC12x12UnormSrgb), 16u);
}

TEST_F(WebGPUUtilsTest, GetFormatBytesPerPixel_UndefinedFormat_Returns0)
{
    EXPECT_EQ(gfx::backend::webgpu::core::getFormatBytesPerPixel(WGPUTextureFormat_Undefined), 0u);
}

// ============================================================================
// Alignment Tests
// ============================================================================

TEST_F(WebGPUUtilsTest, AlignUp_AlreadyAligned_ReturnsOriginal)
{
    EXPECT_EQ(gfx::backend::webgpu::core::alignUp(256, 256), 256u);
    EXPECT_EQ(gfx::backend::webgpu::core::alignUp(512, 256), 512u);
    EXPECT_EQ(gfx::backend::webgpu::core::alignUp(1024, 256), 1024u);
}

TEST_F(WebGPUUtilsTest, AlignUp_NotAligned_RoundsUp)
{
    EXPECT_EQ(gfx::backend::webgpu::core::alignUp(1, 256), 256u);
    EXPECT_EQ(gfx::backend::webgpu::core::alignUp(255, 256), 256u);
    EXPECT_EQ(gfx::backend::webgpu::core::alignUp(257, 256), 512u);
    EXPECT_EQ(gfx::backend::webgpu::core::alignUp(500, 256), 512u);
}

TEST_F(WebGPUUtilsTest, AlignUp_ZeroAlignment_ReturnsOriginal)
{
    EXPECT_EQ(gfx::backend::webgpu::core::alignUp(123, 0), 123u);
    EXPECT_EQ(gfx::backend::webgpu::core::alignUp(0, 0), 0u);
}

TEST_F(WebGPUUtilsTest, AlignUp_SmallAlignments_Works)
{
    EXPECT_EQ(gfx::backend::webgpu::core::alignUp(5, 4), 8u);
    EXPECT_EQ(gfx::backend::webgpu::core::alignUp(7, 8), 8u);
    EXPECT_EQ(gfx::backend::webgpu::core::alignUp(9, 8), 16u);
    EXPECT_EQ(gfx::backend::webgpu::core::alignUp(15, 16), 16u);
    EXPECT_EQ(gfx::backend::webgpu::core::alignUp(17, 16), 32u);
}

TEST_F(WebGPUUtilsTest, AlignUp_PowerOfTwo_AlwaysWorks)
{
    for (uint32_t alignment : { 1u, 2u, 4u, 8u, 16u, 32u, 64u, 128u, 256u }) {
        uint32_t value = 123;
        uint32_t result = gfx::backend::webgpu::core::alignUp(value, alignment);
        EXPECT_EQ(result % alignment, 0u) << "Alignment: " << alignment;
        EXPECT_GE(result, value) << "Alignment: " << alignment;
        EXPECT_LT(result, value + alignment) << "Alignment: " << alignment;
    }
}

// ============================================================================
// Calculate Bytes Per Row Tests
// ============================================================================

TEST_F(WebGPUUtilsTest, CalculateBytesPerRow_RGBA8_AlignsTo256)
{
    // 1 pixel * 4 bytes = 4, rounds up to 256
    EXPECT_EQ(gfx::backend::webgpu::core::calculateBytesPerRow(WGPUTextureFormat_RGBA8Unorm, 1), 256u);

    // 64 pixels * 4 bytes = 256, already aligned
    EXPECT_EQ(gfx::backend::webgpu::core::calculateBytesPerRow(WGPUTextureFormat_RGBA8Unorm, 64), 256u);

    // 65 pixels * 4 bytes = 260, rounds up to 512
    EXPECT_EQ(gfx::backend::webgpu::core::calculateBytesPerRow(WGPUTextureFormat_RGBA8Unorm, 65), 512u);

    // 128 pixels * 4 bytes = 512, already aligned
    EXPECT_EQ(gfx::backend::webgpu::core::calculateBytesPerRow(WGPUTextureFormat_RGBA8Unorm, 128), 512u);
}

TEST_F(WebGPUUtilsTest, CalculateBytesPerRow_R8_AlignsTo256)
{
    // 1 pixel * 1 byte = 1, rounds up to 256
    EXPECT_EQ(gfx::backend::webgpu::core::calculateBytesPerRow(WGPUTextureFormat_R8Unorm, 1), 256u);

    // 256 pixels * 1 byte = 256, already aligned
    EXPECT_EQ(gfx::backend::webgpu::core::calculateBytesPerRow(WGPUTextureFormat_R8Unorm, 256), 256u);

    // 257 pixels * 1 byte = 257, rounds up to 512
    EXPECT_EQ(gfx::backend::webgpu::core::calculateBytesPerRow(WGPUTextureFormat_R8Unorm, 257), 512u);
}

TEST_F(WebGPUUtilsTest, CalculateBytesPerRow_RGBA32Float_AlignsTo256)
{
    // 1 pixel * 16 bytes = 16, rounds up to 256
    EXPECT_EQ(gfx::backend::webgpu::core::calculateBytesPerRow(WGPUTextureFormat_RGBA32Float, 1), 256u);

    // 16 pixels * 16 bytes = 256, already aligned
    EXPECT_EQ(gfx::backend::webgpu::core::calculateBytesPerRow(WGPUTextureFormat_RGBA32Float, 16), 256u);

    // 17 pixels * 16 bytes = 272, rounds up to 512
    EXPECT_EQ(gfx::backend::webgpu::core::calculateBytesPerRow(WGPUTextureFormat_RGBA32Float, 17), 512u);
}

TEST_F(WebGPUUtilsTest, CalculateBytesPerRow_CommonWidths_AllAlignedTo256)
{
    WGPUTextureFormat formats[] = {
        WGPUTextureFormat_RGBA8Unorm,
        WGPUTextureFormat_BGRA8Unorm,
        WGPUTextureFormat_R32Float,
        WGPUTextureFormat_RG16Float
    };

    uint32_t widths[] = { 1, 64, 128, 256, 512, 1024, 1920, 2048 };

    for (auto format : formats) {
        for (auto width : widths) {
            uint32_t result = gfx::backend::webgpu::core::calculateBytesPerRow(format, width);
            EXPECT_EQ(result % 256, 0u) << "Format: " << format << ", Width: " << width;

            uint32_t bytesPerPixel = gfx::backend::webgpu::core::getFormatBytesPerPixel(format);
            uint32_t minBytes = width * bytesPerPixel;
            EXPECT_GE(result, minBytes) << "Format: " << format << ", Width: " << width;
        }
    }
}

TEST_F(WebGPUUtilsTest, CalculateBytesPerRow_ZeroWidth_Returns0)
{
    // Edge case: 0 width returns 0
    EXPECT_EQ(gfx::backend::webgpu::core::calculateBytesPerRow(WGPUTextureFormat_RGBA8Unorm, 0), 0u);
}

// ============================================================================
// String View Tests
// ============================================================================

TEST_F(WebGPUUtilsTest, ToStringView_ValidString_ReturnsCorrectView)
{
    const char* str = "test";
    WGPUStringView view = gfx::backend::webgpu::core::toStringView(str);

    EXPECT_EQ(view.data, str);
    EXPECT_EQ(view.length, WGPU_STRLEN);
}

TEST_F(WebGPUUtilsTest, ToStringView_NullString_ReturnsNullView)
{
    WGPUStringView view = gfx::backend::webgpu::core::toStringView(nullptr);

    EXPECT_EQ(view.data, nullptr);
    EXPECT_EQ(view.length, WGPU_STRLEN);
}

TEST_F(WebGPUUtilsTest, ToStringView_EmptyString_ReturnsValidView)
{
    const char* str = "";
    WGPUStringView view = gfx::backend::webgpu::core::toStringView(str);

    EXPECT_EQ(view.data, str);
    EXPECT_EQ(view.length, WGPU_STRLEN);
}

} // anonymous namespace
