#include <core/util/Utils.h>

#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

namespace gfx::utils {

// =============================================================================
// Alignment Tests
// =============================================================================

TEST(UtilsTest, AlignUp_AlreadyAligned)
{
    EXPECT_EQ(alignUp(256, 256), 256);
    EXPECT_EQ(alignUp(512, 256), 512);
    EXPECT_EQ(alignUp(1024, 256), 1024);
}

TEST(UtilsTest, AlignUp_NeedsAlignment)
{
    EXPECT_EQ(alignUp(1, 256), 256);
    EXPECT_EQ(alignUp(100, 256), 256);
    EXPECT_EQ(alignUp(257, 256), 512);
    EXPECT_EQ(alignUp(500, 256), 512);
}

TEST(UtilsTest, AlignUp_PowerOfTwo)
{
    EXPECT_EQ(alignUp(15, 16), 16);
    EXPECT_EQ(alignUp(17, 16), 32);
    EXPECT_EQ(alignUp(31, 32), 32);
    EXPECT_EQ(alignUp(33, 32), 64);
}

TEST(UtilsTest, AlignUp_Zero)
{
    EXPECT_EQ(alignUp(0, 256), 0);
}

TEST(UtilsTest, AlignDown_AlreadyAligned)
{
    EXPECT_EQ(alignDown(256, 256), 256);
    EXPECT_EQ(alignDown(512, 256), 512);
    EXPECT_EQ(alignDown(1024, 256), 1024);
}

TEST(UtilsTest, AlignDown_NeedsAlignment)
{
    EXPECT_EQ(alignDown(1, 256), 0);
    EXPECT_EQ(alignDown(100, 256), 0);
    EXPECT_EQ(alignDown(257, 256), 256);
    EXPECT_EQ(alignDown(500, 256), 256);
}

TEST(UtilsTest, AlignDown_PowerOfTwo)
{
    EXPECT_EQ(alignDown(15, 16), 0);
    EXPECT_EQ(alignDown(17, 16), 16);
    EXPECT_EQ(alignDown(31, 32), 0);
    EXPECT_EQ(alignDown(33, 32), 32);
}

TEST(UtilsTest, AlignDown_Zero)
{
    EXPECT_EQ(alignDown(0, 256), 0);
}

// =============================================================================
// Access Flags for Layout Tests
// =============================================================================

TEST(UtilsTest, GetAccessFlagsForLayout_Undefined)
{
    auto flags = getAccessFlagsForLayout(TextureLayout::Undefined);
    EXPECT_EQ(flags, AccessFlags::None);
}

TEST(UtilsTest, GetAccessFlagsForLayout_General)
{
    auto flags = getAccessFlagsForLayout(TextureLayout::General);
    // General layout allows all accesses
    EXPECT_NE(flags, AccessFlags::None);
}

TEST(UtilsTest, GetAccessFlagsForLayout_ColorAttachment)
{
    auto flags = getAccessFlagsForLayout(TextureLayout::ColorAttachment);
    auto expected = static_cast<AccessFlags>(
        static_cast<uint32_t>(AccessFlags::ColorAttachmentRead) | static_cast<uint32_t>(AccessFlags::ColorAttachmentWrite));
    EXPECT_EQ(flags, expected);
}

TEST(UtilsTest, GetAccessFlagsForLayout_DepthStencilAttachment)
{
    auto flags = getAccessFlagsForLayout(TextureLayout::DepthStencilAttachment);
    auto expected = static_cast<AccessFlags>(
        static_cast<uint32_t>(AccessFlags::DepthStencilAttachmentRead) | static_cast<uint32_t>(AccessFlags::DepthStencilAttachmentWrite));
    EXPECT_EQ(flags, expected);
}

TEST(UtilsTest, GetAccessFlagsForLayout_DepthStencilReadOnly)
{
    auto flags = getAccessFlagsForLayout(TextureLayout::DepthStencilReadOnly);
    EXPECT_EQ(flags, AccessFlags::DepthStencilAttachmentRead);
}

TEST(UtilsTest, GetAccessFlagsForLayout_ShaderReadOnly)
{
    auto flags = getAccessFlagsForLayout(TextureLayout::ShaderReadOnly);
    EXPECT_EQ(flags, AccessFlags::ShaderRead);
}

TEST(UtilsTest, GetAccessFlagsForLayout_TransferSrc)
{
    auto flags = getAccessFlagsForLayout(TextureLayout::TransferSrc);
    EXPECT_EQ(flags, AccessFlags::TransferRead);
}

TEST(UtilsTest, GetAccessFlagsForLayout_TransferDst)
{
    auto flags = getAccessFlagsForLayout(TextureLayout::TransferDst);
    EXPECT_EQ(flags, AccessFlags::TransferWrite);
}

TEST(UtilsTest, GetAccessFlagsForLayout_PresentSrc)
{
    auto flags = getAccessFlagsForLayout(TextureLayout::PresentSrc);
    // PresentSrc doesn't require specific access flags, can be None or memory read
    // Just verify it doesn't crash
    (void)flags;
}

// =============================================================================
// Format Bytes Per Pixel Tests
// =============================================================================

TEST(UtilsTest, GetFormatBytesPerPixel_Undefined)
{
    EXPECT_EQ(getFormatBytesPerPixel(TextureFormat::Undefined), 0);
}

TEST(UtilsTest, GetFormatBytesPerPixel_8Bit)
{
    EXPECT_EQ(getFormatBytesPerPixel(TextureFormat::R8Unorm), 1);
}

TEST(UtilsTest, GetFormatBytesPerPixel_16Bit)
{
    EXPECT_EQ(getFormatBytesPerPixel(TextureFormat::R8G8Unorm), 2);
    EXPECT_EQ(getFormatBytesPerPixel(TextureFormat::R16Float), 2);
}

TEST(UtilsTest, GetFormatBytesPerPixel_32Bit)
{
    EXPECT_EQ(getFormatBytesPerPixel(TextureFormat::R8G8B8A8Unorm), 4);
    EXPECT_EQ(getFormatBytesPerPixel(TextureFormat::R8G8B8A8UnormSrgb), 4);
    EXPECT_EQ(getFormatBytesPerPixel(TextureFormat::B8G8R8A8Unorm), 4);
    EXPECT_EQ(getFormatBytesPerPixel(TextureFormat::B8G8R8A8UnormSrgb), 4);
    EXPECT_EQ(getFormatBytesPerPixel(TextureFormat::R32Float), 4);
    EXPECT_EQ(getFormatBytesPerPixel(TextureFormat::R16G16Float), 4);
}

TEST(UtilsTest, GetFormatBytesPerPixel_64Bit)
{
    EXPECT_EQ(getFormatBytesPerPixel(TextureFormat::R16G16B16A16Float), 8);
    EXPECT_EQ(getFormatBytesPerPixel(TextureFormat::R32G32Float), 8);
}

TEST(UtilsTest, GetFormatBytesPerPixel_96Bit)
{
    EXPECT_EQ(getFormatBytesPerPixel(TextureFormat::R32G32B32Float), 12);
}

TEST(UtilsTest, GetFormatBytesPerPixel_128Bit)
{
    EXPECT_EQ(getFormatBytesPerPixel(TextureFormat::R32G32B32A32Float), 16);
}

TEST(UtilsTest, GetFormatBytesPerPixel_DepthFormats)
{
    EXPECT_EQ(getFormatBytesPerPixel(TextureFormat::Depth16Unorm), 2);
    EXPECT_EQ(getFormatBytesPerPixel(TextureFormat::Depth24Plus), 4);
    EXPECT_EQ(getFormatBytesPerPixel(TextureFormat::Depth32Float), 4);
    EXPECT_EQ(getFormatBytesPerPixel(TextureFormat::Depth24PlusStencil8), 4);
    EXPECT_EQ(getFormatBytesPerPixel(TextureFormat::Depth32FloatStencil8), 8);
}

TEST(UtilsTest, GetFormatBytesPerPixel_StencilFormat)
{
    EXPECT_EQ(getFormatBytesPerPixel(TextureFormat::Stencil8), 1);
}

} // namespace gfx::utils
