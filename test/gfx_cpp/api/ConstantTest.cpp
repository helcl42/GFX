#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

// Basic sanity test

namespace {

TEST(GfxCppConstantTest, LibraryVersion)
{
    // Just verify we can link against the library
    EXPECT_TRUE(true);
}

TEST(GfxCppConstantTest, ResultEnumValues)
{
    // Verify enum values are correct
    EXPECT_EQ(static_cast<int32_t>(gfx::Result::Success), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::Result::Timeout), 1);
    EXPECT_EQ(static_cast<int32_t>(gfx::Result::NotReady), 2);
    EXPECT_LT(static_cast<int32_t>(gfx::Result::ErrorInvalidArgument), 0);
}

TEST(GfxCppConstantTest, BackendEnumValues)
{
    // Verify backend enum values
    EXPECT_EQ(static_cast<int32_t>(gfx::Backend::Vulkan), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::Backend::WebGPU), 1);
    EXPECT_EQ(static_cast<int32_t>(gfx::Backend::Auto), 2);
}

TEST(GfxCppConstantTest, AdapterTypeEnumValues)
{
    // Verify adapter type enum values
    EXPECT_EQ(static_cast<int32_t>(gfx::AdapterType::DiscreteGPU), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::AdapterType::IntegratedGPU), 1);
    EXPECT_EQ(static_cast<int32_t>(gfx::AdapterType::CPU), 2);
    EXPECT_EQ(static_cast<int32_t>(gfx::AdapterType::Unknown), 3);
}

TEST(GfxCppConstantTest, TextureFormatEnumValues)
{
    // Verify texture format enum values
    EXPECT_EQ(static_cast<int32_t>(gfx::TextureFormat::Undefined), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::TextureFormat::R8Unorm), 1);
    EXPECT_EQ(static_cast<int32_t>(gfx::TextureFormat::R8G8B8A8Unorm), 3);
    EXPECT_EQ(static_cast<int32_t>(gfx::TextureFormat::B8G8R8A8Unorm), 5);
}

TEST(GfxCppConstantTest, BufferUsageFlagValues)
{
    // Verify buffer usage flags are bitmasks
    EXPECT_EQ(static_cast<uint32_t>(gfx::BufferUsage::None), 0u);
    EXPECT_EQ(static_cast<uint32_t>(gfx::BufferUsage::MapRead), 1u << 0);
    EXPECT_EQ(static_cast<uint32_t>(gfx::BufferUsage::MapWrite), 1u << 1);
    EXPECT_EQ(static_cast<uint32_t>(gfx::BufferUsage::Vertex), 1u << 5);
    EXPECT_EQ(static_cast<uint32_t>(gfx::BufferUsage::Uniform), 1u << 6);
}

TEST(GfxCppConstantTest, TextureUsageFlagValues)
{
    // Verify texture usage flags are bitmasks
    EXPECT_EQ(static_cast<uint32_t>(gfx::TextureUsage::None), 0u);
    EXPECT_EQ(static_cast<uint32_t>(gfx::TextureUsage::CopySrc), 1u << 0);
    EXPECT_EQ(static_cast<uint32_t>(gfx::TextureUsage::CopyDst), 1u << 1);
    EXPECT_EQ(static_cast<uint32_t>(gfx::TextureUsage::TextureBinding), 1u << 2);
    EXPECT_EQ(static_cast<uint32_t>(gfx::TextureUsage::RenderAttachment), 1u << 4);
}

TEST(GfxCppConstantTest, ShaderStageFlags)
{
    // Verify shader stage flags are bitmasks
    EXPECT_EQ(static_cast<uint32_t>(gfx::ShaderStage::None), 0u);
    EXPECT_EQ(static_cast<uint32_t>(gfx::ShaderStage::Vertex), 1u << 0);
    EXPECT_EQ(static_cast<uint32_t>(gfx::ShaderStage::Fragment), 1u << 1);
    EXPECT_EQ(static_cast<uint32_t>(gfx::ShaderStage::Compute), 1u << 2);
}

TEST(GfxCppConstantTest, QueueFlags)
{
    // Verify queue flags are bitmasks
    EXPECT_EQ(static_cast<uint32_t>(gfx::QueueFlags::None), 0u);
    EXPECT_EQ(static_cast<uint32_t>(gfx::QueueFlags::Graphics), 1u << 0);
    EXPECT_EQ(static_cast<uint32_t>(gfx::QueueFlags::Compute), 1u << 1);
    EXPECT_EQ(static_cast<uint32_t>(gfx::QueueFlags::Transfer), 1u << 2);
}

TEST(GfxCppConstantTest, PrimitiveTopologyValues)
{
    // Verify primitive topology values
    EXPECT_EQ(static_cast<int32_t>(gfx::PrimitiveTopology::PointList), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::PrimitiveTopology::LineList), 1);
    EXPECT_EQ(static_cast<int32_t>(gfx::PrimitiveTopology::TriangleList), 3);
}

TEST(GfxCppConstantTest, IndexFormatValues)
{
    // Verify index format values
    EXPECT_EQ(static_cast<int32_t>(gfx::IndexFormat::Undefined), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::IndexFormat::Uint16), 1);
    EXPECT_EQ(static_cast<int32_t>(gfx::IndexFormat::Uint32), 2);
}

TEST(GfxCppConstantTest, SampleCountValues)
{
    // Verify sample count values
    EXPECT_EQ(static_cast<int32_t>(gfx::SampleCount::Count1), 1);
    EXPECT_EQ(static_cast<int32_t>(gfx::SampleCount::Count2), 2);
    EXPECT_EQ(static_cast<int32_t>(gfx::SampleCount::Count4), 4);
    EXPECT_EQ(static_cast<int32_t>(gfx::SampleCount::Count8), 8);
}

TEST(GfxCppConstantTest, CompareFunctionValues)
{
    // Verify compare function values
    EXPECT_EQ(static_cast<int32_t>(gfx::CompareFunction::Undefined), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::CompareFunction::Never), 1);
    EXPECT_EQ(static_cast<int32_t>(gfx::CompareFunction::Less), 2);
    EXPECT_EQ(static_cast<int32_t>(gfx::CompareFunction::Always), 8);
}

TEST(GfxCppConstantTest, LoadStoreOpValues)
{
    // Verify load/store operation values
    EXPECT_EQ(static_cast<int32_t>(gfx::LoadOp::Load), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::LoadOp::Clear), 1);
    EXPECT_EQ(static_cast<int32_t>(gfx::LoadOp::DontCare), 2);
    EXPECT_EQ(static_cast<int32_t>(gfx::StoreOp::Store), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::StoreOp::DontCare), 1);
}

TEST(GfxCppConstantTest, PresentModeValues)
{
    // Verify present mode values
    EXPECT_EQ(static_cast<int32_t>(gfx::PresentMode::Immediate), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::PresentMode::Fifo), 1);
    EXPECT_EQ(static_cast<int32_t>(gfx::PresentMode::FifoRelaxed), 2);
    EXPECT_EQ(static_cast<int32_t>(gfx::PresentMode::Mailbox), 3);
}

TEST(GfxCppConstantTest, CullModeValues)
{
    // Verify cull mode values
    EXPECT_EQ(static_cast<int32_t>(gfx::CullMode::None), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::CullMode::Front), 1);
    EXPECT_EQ(static_cast<int32_t>(gfx::CullMode::Back), 2);
    EXPECT_EQ(static_cast<int32_t>(gfx::CullMode::FrontAndBack), 3);
}

TEST(GfxCppConstantTest, FrontFaceValues)
{
    // Verify front face values
    EXPECT_EQ(static_cast<int32_t>(gfx::FrontFace::CounterClockwise), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::FrontFace::Clockwise), 1);
}

TEST(GfxCppConstantTest, BlendOperationValues)
{
    // Verify blend operation values
    EXPECT_EQ(static_cast<int32_t>(gfx::BlendOperation::Add), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::BlendOperation::Subtract), 1);
    EXPECT_EQ(static_cast<int32_t>(gfx::BlendOperation::ReverseSubtract), 2);
    EXPECT_EQ(static_cast<int32_t>(gfx::BlendOperation::Min), 3);
    EXPECT_EQ(static_cast<int32_t>(gfx::BlendOperation::Max), 4);
}

TEST(GfxCppConstantTest, BlendFactorValues)
{
    // Verify blend factor values
    EXPECT_EQ(static_cast<int32_t>(gfx::BlendFactor::Zero), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::BlendFactor::One), 1);
    EXPECT_EQ(static_cast<int32_t>(gfx::BlendFactor::SrcAlpha), 4);
    EXPECT_EQ(static_cast<int32_t>(gfx::BlendFactor::DstAlpha), 8);
}

TEST(GfxCppConstantTest, ColorWriteMaskValues)
{
    // Verify color write mask flags
    EXPECT_EQ(static_cast<uint32_t>(gfx::ColorWriteMask::None), 0x0u);
    EXPECT_EQ(static_cast<uint32_t>(gfx::ColorWriteMask::Red), 0x1u);
    EXPECT_EQ(static_cast<uint32_t>(gfx::ColorWriteMask::Green), 0x2u);
    EXPECT_EQ(static_cast<uint32_t>(gfx::ColorWriteMask::Blue), 0x4u);
    EXPECT_EQ(static_cast<uint32_t>(gfx::ColorWriteMask::Alpha), 0x8u);
    EXPECT_EQ(static_cast<uint32_t>(gfx::ColorWriteMask::All), 0xFu);
}

TEST(GfxCppConstantTest, TextureTypeValues)
{
    // Verify texture type values
    EXPECT_EQ(static_cast<int32_t>(gfx::TextureType::Texture1D), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::TextureType::Texture2D), 1);
    EXPECT_EQ(static_cast<int32_t>(gfx::TextureType::Texture3D), 2);
    EXPECT_EQ(static_cast<int32_t>(gfx::TextureType::TextureCube), 3);
}

TEST(GfxCppConstantTest, FilterModeValues)
{
    // Verify filter mode values
    EXPECT_EQ(static_cast<int32_t>(gfx::FilterMode::Nearest), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::FilterMode::Linear), 1);
}

TEST(GfxCppConstantTest, AddressModeValues)
{
    // Verify address mode values
    EXPECT_EQ(static_cast<int32_t>(gfx::AddressMode::Repeat), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::AddressMode::MirrorRepeat), 1);
    EXPECT_EQ(static_cast<int32_t>(gfx::AddressMode::ClampToEdge), 2);
}

TEST(GfxCppConstantTest, PolygonModeValues)
{
    // Verify polygon mode values
    EXPECT_EQ(static_cast<int32_t>(gfx::PolygonMode::Fill), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::PolygonMode::Line), 1);
    EXPECT_EQ(static_cast<int32_t>(gfx::PolygonMode::Point), 2);
}

TEST(GfxCppConstantTest, TextureLayoutValues)
{
    // Verify texture layout values
    EXPECT_EQ(static_cast<int32_t>(gfx::TextureLayout::Undefined), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::TextureLayout::General), 1);
    EXPECT_EQ(static_cast<int32_t>(gfx::TextureLayout::ColorAttachment), 2);
    EXPECT_EQ(static_cast<int32_t>(gfx::TextureLayout::ShaderReadOnly), 5);
    EXPECT_EQ(static_cast<int32_t>(gfx::TextureLayout::PresentSrc), 8);
}

TEST(GfxCppConstantTest, ShaderSourceTypeValues)
{
    // Verify shader source type values
    EXPECT_EQ(static_cast<int32_t>(gfx::ShaderSourceType::WGSL), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::ShaderSourceType::SPIRV), 1);
}

TEST(GfxCppConstantTest, AdapterPreferenceValues)
{
    // Verify adapter preference values
    EXPECT_EQ(static_cast<int32_t>(gfx::AdapterPreference::Undefined), 0);
    EXPECT_EQ(static_cast<int32_t>(gfx::AdapterPreference::LowPower), 1);
    EXPECT_EQ(static_cast<int32_t>(gfx::AdapterPreference::HighPerformance), 2);
    EXPECT_EQ(static_cast<int32_t>(gfx::AdapterPreference::Software), 3);
}

} // namespace
