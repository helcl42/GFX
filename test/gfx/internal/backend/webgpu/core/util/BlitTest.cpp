#include <backend/webgpu/core/command/CommandEncoder.h>
#include <backend/webgpu/core/resource/Texture.h>
#include <backend/webgpu/core/system/Adapter.h>
#include <backend/webgpu/core/system/Device.h>
#include <backend/webgpu/core/system/Instance.h>
#include <backend/webgpu/core/util/Blit.h>

#include <gtest/gtest.h>

namespace {

class WebGPUBlitTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        try {
            gfx::backend::webgpu::core::InstanceCreateInfo instInfo{};
            instance = std::make_unique<gfx::backend::webgpu::core::Instance>(instInfo);

            gfx::backend::webgpu::core::AdapterCreateInfo adapterInfo{};
            adapterInfo.adapterIndex = 0;
            adapter = instance->requestAdapter(adapterInfo);

            gfx::backend::webgpu::core::DeviceCreateInfo deviceInfo{};
            device = std::make_unique<gfx::backend::webgpu::core::Device>(adapter, deviceInfo);
        } catch (const std::exception& e) {
            GTEST_SKIP() << "WebGPU not available: " << e.what();
        }
    }

    void TearDown() override
    {
        device.reset();
        instance.reset();
    }

    std::unique_ptr<gfx::backend::webgpu::core::Instance> instance;
    gfx::backend::webgpu::core::Adapter* adapter = nullptr;
    std::unique_ptr<gfx::backend::webgpu::core::Device> device;
};

// ============================================================================
// Construction Tests
// ============================================================================

TEST_F(WebGPUBlitTest, Construction_WithValidDevice_CreatesBlitHelper)
{
    EXPECT_NO_THROW({
        auto blit = std::make_unique<gfx::backend::webgpu::core::Blit>(device->handle());
    });
}

TEST_F(WebGPUBlitTest, GetBlit_FromDevice_ReturnsNonNull)
{
    auto* blit = device->getBlit();
    EXPECT_NE(blit, nullptr);
}

// ============================================================================
// Basic Blit Tests
// ============================================================================

TEST_F(WebGPUBlitTest, Execute_SameSize_BlitsCorrectly)
{
    // Create source texture
    gfx::backend::webgpu::core::TextureCreateInfo srcInfo{};
    srcInfo.format = WGPUTextureFormat_RGBA8Unorm;
    srcInfo.size = { 256, 256, 1 };
    srcInfo.mipLevelCount = 1;
    srcInfo.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
    auto srcTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), srcInfo);

    // Create destination texture
    gfx::backend::webgpu::core::TextureCreateInfo dstInfo{};
    dstInfo.format = WGPUTextureFormat_RGBA8Unorm;
    dstInfo.size = { 256, 256, 1 };
    dstInfo.mipLevelCount = 1;
    dstInfo.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopyDst;
    auto dstTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), dstInfo);

    // Create command encoder
    gfx::backend::webgpu::core::CommandEncoderCreateInfo encoderInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), encoderInfo);

    WGPUOrigin3D origin = { 0, 0, 0 };
    WGPUExtent3D extent = { 256, 256, 1 };

    EXPECT_NO_THROW({
        device->getBlit()->execute(
            encoder->handle(),
            srcTexture->handle(),
            origin, extent, 0,
            dstTexture->handle(),
            origin, extent, 0,
            WGPUFilterMode_Linear);
    });
}

TEST_F(WebGPUBlitTest, Execute_Downscale_BlitsCorrectly)
{
    // Create source texture (512x512)
    gfx::backend::webgpu::core::TextureCreateInfo srcInfo{};
    srcInfo.format = WGPUTextureFormat_RGBA8Unorm;
    srcInfo.size = { 512, 512, 1 };
    srcInfo.mipLevelCount = 1;
    srcInfo.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
    auto srcTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), srcInfo);

    // Create destination texture (256x256)
    gfx::backend::webgpu::core::TextureCreateInfo dstInfo{};
    dstInfo.format = WGPUTextureFormat_RGBA8Unorm;
    dstInfo.size = { 256, 256, 1 };
    dstInfo.mipLevelCount = 1;
    dstInfo.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopyDst;
    auto dstTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), dstInfo);

    // Create command encoder
    gfx::backend::webgpu::core::CommandEncoderCreateInfo encoderInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), encoderInfo);

    WGPUOrigin3D origin = { 0, 0, 0 };
    WGPUExtent3D srcExtent = { 512, 512, 1 };
    WGPUExtent3D dstExtent = { 256, 256, 1 };

    EXPECT_NO_THROW({
        device->getBlit()->execute(
            encoder->handle(),
            srcTexture->handle(),
            origin, srcExtent, 0,
            dstTexture->handle(),
            origin, dstExtent, 0,
            WGPUFilterMode_Linear);
    });
}

TEST_F(WebGPUBlitTest, Execute_Upscale_BlitsCorrectly)
{
    // Create source texture (128x128)
    gfx::backend::webgpu::core::TextureCreateInfo srcInfo{};
    srcInfo.format = WGPUTextureFormat_RGBA8Unorm;
    srcInfo.size = { 128, 128, 1 };
    srcInfo.mipLevelCount = 1;
    srcInfo.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
    auto srcTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), srcInfo);

    // Create destination texture (512x512)
    gfx::backend::webgpu::core::TextureCreateInfo dstInfo{};
    dstInfo.format = WGPUTextureFormat_RGBA8Unorm;
    dstInfo.size = { 512, 512, 1 };
    dstInfo.mipLevelCount = 1;
    dstInfo.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopyDst;
    auto dstTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), dstInfo);

    // Create command encoder
    gfx::backend::webgpu::core::CommandEncoderCreateInfo encoderInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), encoderInfo);

    WGPUOrigin3D origin = { 0, 0, 0 };
    WGPUExtent3D srcExtent = { 128, 128, 1 };
    WGPUExtent3D dstExtent = { 512, 512, 1 };

    EXPECT_NO_THROW({
        device->getBlit()->execute(
            encoder->handle(),
            srcTexture->handle(),
            origin, srcExtent, 0,
            dstTexture->handle(),
            origin, dstExtent, 0,
            WGPUFilterMode_Linear);
    });
}

// ============================================================================
// Region Blit Tests
// ============================================================================

TEST_F(WebGPUBlitTest, Execute_SourceRegion_BlitsCorrectly)
{
    // Create source texture
    gfx::backend::webgpu::core::TextureCreateInfo srcInfo{};
    srcInfo.format = WGPUTextureFormat_RGBA8Unorm;
    srcInfo.size = { 512, 512, 1 };
    srcInfo.mipLevelCount = 1;
    srcInfo.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
    auto srcTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), srcInfo);

    // Create destination texture
    gfx::backend::webgpu::core::TextureCreateInfo dstInfo{};
    dstInfo.format = WGPUTextureFormat_RGBA8Unorm;
    dstInfo.size = { 256, 256, 1 };
    dstInfo.mipLevelCount = 1;
    dstInfo.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopyDst;
    auto dstTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), dstInfo);

    // Create command encoder
    gfx::backend::webgpu::core::CommandEncoderCreateInfo encoderInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), encoderInfo);

    // Blit from region [128, 128] to [384, 384] (256x256 region)
    WGPUOrigin3D srcOrigin = { 128, 128, 0 };
    WGPUOrigin3D dstOrigin = { 0, 0, 0 };
    WGPUExtent3D srcExtent = { 256, 256, 1 };
    WGPUExtent3D dstExtent = { 256, 256, 1 };

    EXPECT_NO_THROW({
        device->getBlit()->execute(
            encoder->handle(),
            srcTexture->handle(),
            srcOrigin, srcExtent, 0,
            dstTexture->handle(),
            dstOrigin, dstExtent, 0,
            WGPUFilterMode_Linear);
    });
}

TEST_F(WebGPUBlitTest, Execute_DestinationRegion_BlitsCorrectly)
{
    // Create source texture
    gfx::backend::webgpu::core::TextureCreateInfo srcInfo{};
    srcInfo.format = WGPUTextureFormat_RGBA8Unorm;
    srcInfo.size = { 256, 256, 1 };
    srcInfo.mipLevelCount = 1;
    srcInfo.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
    auto srcTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), srcInfo);

    // Create destination texture
    gfx::backend::webgpu::core::TextureCreateInfo dstInfo{};
    dstInfo.format = WGPUTextureFormat_RGBA8Unorm;
    dstInfo.size = { 512, 512, 1 };
    dstInfo.mipLevelCount = 1;
    dstInfo.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopyDst;
    auto dstTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), dstInfo);

    // Create command encoder
    gfx::backend::webgpu::core::CommandEncoderCreateInfo encoderInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), encoderInfo);

    // Blit to region [128, 128] to [384, 384] (256x256 region)
    WGPUOrigin3D srcOrigin = { 0, 0, 0 };
    WGPUOrigin3D dstOrigin = { 128, 128, 0 };
    WGPUExtent3D srcExtent = { 256, 256, 1 };
    WGPUExtent3D dstExtent = { 256, 256, 1 };

    EXPECT_NO_THROW({
        device->getBlit()->execute(
            encoder->handle(),
            srcTexture->handle(),
            srcOrigin, srcExtent, 0,
            dstTexture->handle(),
            dstOrigin, dstExtent, 0,
            WGPUFilterMode_Linear);
    });
}

TEST_F(WebGPUBlitTest, Execute_BothRegions_BlitsCorrectly)
{
    // Create source texture
    gfx::backend::webgpu::core::TextureCreateInfo srcInfo{};
    srcInfo.format = WGPUTextureFormat_RGBA8Unorm;
    srcInfo.size = { 1024, 1024, 1 };
    srcInfo.mipLevelCount = 1;
    srcInfo.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
    auto srcTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), srcInfo);

    // Create destination texture
    gfx::backend::webgpu::core::TextureCreateInfo dstInfo{};
    dstInfo.format = WGPUTextureFormat_RGBA8Unorm;
    dstInfo.size = { 1024, 1024, 1 };
    dstInfo.mipLevelCount = 1;
    dstInfo.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopyDst;
    auto dstTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), dstInfo);

    // Create command encoder
    gfx::backend::webgpu::core::CommandEncoderCreateInfo encoderInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), encoderInfo);

    // Blit from region [256, 256, 512x512] to [512, 512, 256x256]
    WGPUOrigin3D srcOrigin = { 256, 256, 0 };
    WGPUOrigin3D dstOrigin = { 512, 512, 0 };
    WGPUExtent3D srcExtent = { 512, 512, 1 };
    WGPUExtent3D dstExtent = { 256, 256, 1 };

    EXPECT_NO_THROW({
        device->getBlit()->execute(
            encoder->handle(),
            srcTexture->handle(),
            srcOrigin, srcExtent, 0,
            dstTexture->handle(),
            dstOrigin, dstExtent, 0,
            WGPUFilterMode_Linear);
    });
}

// ============================================================================
// Filter Mode Tests
// ============================================================================

TEST_F(WebGPUBlitTest, Execute_NearestFilter_BlitsCorrectly)
{
    // Create source texture
    gfx::backend::webgpu::core::TextureCreateInfo srcInfo{};
    srcInfo.format = WGPUTextureFormat_RGBA8Unorm;
    srcInfo.size = { 512, 512, 1 };
    srcInfo.mipLevelCount = 1;
    srcInfo.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
    auto srcTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), srcInfo);

    // Create destination texture
    gfx::backend::webgpu::core::TextureCreateInfo dstInfo{};
    dstInfo.format = WGPUTextureFormat_RGBA8Unorm;
    dstInfo.size = { 256, 256, 1 };
    dstInfo.mipLevelCount = 1;
    dstInfo.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopyDst;
    auto dstTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), dstInfo);

    // Create command encoder
    gfx::backend::webgpu::core::CommandEncoderCreateInfo encoderInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), encoderInfo);

    WGPUOrigin3D origin = { 0, 0, 0 };
    WGPUExtent3D srcExtent = { 512, 512, 1 };
    WGPUExtent3D dstExtent = { 256, 256, 1 };

    EXPECT_NO_THROW({
        device->getBlit()->execute(
            encoder->handle(),
            srcTexture->handle(),
            origin, srcExtent, 0,
            dstTexture->handle(),
            origin, dstExtent, 0,
            WGPUFilterMode_Nearest);
    });
}

TEST_F(WebGPUBlitTest, Execute_LinearFilter_BlitsCorrectly)
{
    // Create source texture
    gfx::backend::webgpu::core::TextureCreateInfo srcInfo{};
    srcInfo.format = WGPUTextureFormat_RGBA8Unorm;
    srcInfo.size = { 512, 512, 1 };
    srcInfo.mipLevelCount = 1;
    srcInfo.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
    auto srcTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), srcInfo);

    // Create destination texture
    gfx::backend::webgpu::core::TextureCreateInfo dstInfo{};
    dstInfo.format = WGPUTextureFormat_RGBA8Unorm;
    dstInfo.size = { 256, 256, 1 };
    dstInfo.mipLevelCount = 1;
    dstInfo.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopyDst;
    auto dstTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), dstInfo);

    // Create command encoder
    gfx::backend::webgpu::core::CommandEncoderCreateInfo encoderInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), encoderInfo);

    WGPUOrigin3D origin = { 0, 0, 0 };
    WGPUExtent3D srcExtent = { 512, 512, 1 };
    WGPUExtent3D dstExtent = { 256, 256, 1 };

    EXPECT_NO_THROW({
        device->getBlit()->execute(
            encoder->handle(),
            srcTexture->handle(),
            origin, srcExtent, 0,
            dstTexture->handle(),
            origin, dstExtent, 0,
            WGPUFilterMode_Linear);
    });
}

// ============================================================================
// Format Tests
// ============================================================================

TEST_F(WebGPUBlitTest, Execute_RGBA8Unorm_BlitsCorrectly)
{
    // Create source texture
    gfx::backend::webgpu::core::TextureCreateInfo srcInfo{};
    srcInfo.format = WGPUTextureFormat_RGBA8Unorm;
    srcInfo.size = { 256, 256, 1 };
    srcInfo.mipLevelCount = 1;
    srcInfo.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
    auto srcTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), srcInfo);

    // Create destination texture
    gfx::backend::webgpu::core::TextureCreateInfo dstInfo{};
    dstInfo.format = WGPUTextureFormat_RGBA8Unorm;
    dstInfo.size = { 256, 256, 1 };
    dstInfo.mipLevelCount = 1;
    dstInfo.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopyDst;
    auto dstTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), dstInfo);

    gfx::backend::webgpu::core::CommandEncoderCreateInfo encoderInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), encoderInfo);
    WGPUOrigin3D origin = { 0, 0, 0 };
    WGPUExtent3D extent = { 256, 256, 1 };

    EXPECT_NO_THROW({
        device->getBlit()->execute(
            encoder->handle(),
            srcTexture->handle(),
            origin, extent, 0,
            dstTexture->handle(),
            origin, extent, 0,
            WGPUFilterMode_Linear);
    });
}

TEST_F(WebGPUBlitTest, Execute_BGRA8Unorm_BlitsCorrectly)
{
    // Create source texture
    gfx::backend::webgpu::core::TextureCreateInfo srcInfo{};
    srcInfo.format = WGPUTextureFormat_BGRA8Unorm;
    srcInfo.size = { 256, 256, 1 };
    srcInfo.mipLevelCount = 1;
    srcInfo.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
    auto srcTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), srcInfo);

    // Create destination texture
    gfx::backend::webgpu::core::TextureCreateInfo dstInfo{};
    dstInfo.format = WGPUTextureFormat_BGRA8Unorm;
    dstInfo.size = { 256, 256, 1 };
    dstInfo.mipLevelCount = 1;
    dstInfo.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopyDst;
    auto dstTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), dstInfo);

    gfx::backend::webgpu::core::CommandEncoderCreateInfo encoderInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), encoderInfo);
    WGPUOrigin3D origin = { 0, 0, 0 };
    WGPUExtent3D extent = { 256, 256, 1 };

    EXPECT_NO_THROW({
        device->getBlit()->execute(
            encoder->handle(),
            srcTexture->handle(),
            origin, extent, 0,
            dstTexture->handle(),
            origin, extent, 0,
            WGPUFilterMode_Linear);
    });
}

TEST_F(WebGPUBlitTest, Execute_RGBA16Float_BlitsCorrectly)
{
    // Create source texture
    gfx::backend::webgpu::core::TextureCreateInfo srcInfo{};
    srcInfo.format = WGPUTextureFormat_RGBA16Float;
    srcInfo.size = { 256, 256, 1 };
    srcInfo.mipLevelCount = 1;
    srcInfo.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
    auto srcTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), srcInfo);

    // Create destination texture
    gfx::backend::webgpu::core::TextureCreateInfo dstInfo{};
    dstInfo.format = WGPUTextureFormat_RGBA16Float;
    dstInfo.size = { 256, 256, 1 };
    dstInfo.mipLevelCount = 1;
    dstInfo.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopyDst;
    auto dstTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), dstInfo);

    gfx::backend::webgpu::core::CommandEncoderCreateInfo encoderInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), encoderInfo);
    WGPUOrigin3D origin = { 0, 0, 0 };
    WGPUExtent3D extent = { 256, 256, 1 };

    EXPECT_NO_THROW({
        device->getBlit()->execute(
            encoder->handle(),
            srcTexture->handle(),
            origin, extent, 0,
            dstTexture->handle(),
            origin, extent, 0,
            WGPUFilterMode_Linear);
    });
}

// ============================================================================
// Mip Level Tests
// ============================================================================

TEST_F(WebGPUBlitTest, Execute_SourceMipLevel1_BlitsCorrectly)
{
    // Create source texture with mip levels
    gfx::backend::webgpu::core::TextureCreateInfo srcInfo{};
    srcInfo.format = WGPUTextureFormat_RGBA8Unorm;
    srcInfo.size = { 512, 512, 1 };
    srcInfo.mipLevelCount = 3; // 512x512, 256x256, 128x128
    srcInfo.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
    auto srcTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), srcInfo);

    // Create destination texture
    gfx::backend::webgpu::core::TextureCreateInfo dstInfo{};
    dstInfo.format = WGPUTextureFormat_RGBA8Unorm;
    dstInfo.size = { 256, 256, 1 };
    dstInfo.mipLevelCount = 1;
    dstInfo.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopyDst;
    auto dstTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), dstInfo);

    gfx::backend::webgpu::core::CommandEncoderCreateInfo encoderInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), encoderInfo);
    WGPUOrigin3D origin = { 0, 0, 0 };
    WGPUExtent3D extent = { 256, 256, 1 };

    // Blit from mip level 1 (256x256)
    EXPECT_NO_THROW({
        device->getBlit()->execute(
            encoder->handle(),
            srcTexture->handle(),
            origin, extent, 1, // Source mip level 1
            dstTexture->handle(),
            origin, extent, 0,
            WGPUFilterMode_Linear);
    });
}

TEST_F(WebGPUBlitTest, Execute_DestinationMipLevel1_BlitsCorrectly)
{
    // Create source texture
    gfx::backend::webgpu::core::TextureCreateInfo srcInfo{};
    srcInfo.format = WGPUTextureFormat_RGBA8Unorm;
    srcInfo.size = { 256, 256, 1 };
    srcInfo.mipLevelCount = 1;
    srcInfo.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
    auto srcTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), srcInfo);

    // Create destination texture with mip levels
    gfx::backend::webgpu::core::TextureCreateInfo dstInfo{};
    dstInfo.format = WGPUTextureFormat_RGBA8Unorm;
    dstInfo.size = { 512, 512, 1 };
    dstInfo.mipLevelCount = 3; // 512x512, 256x256, 128x128
    dstInfo.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopyDst;
    auto dstTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), dstInfo);

    gfx::backend::webgpu::core::CommandEncoderCreateInfo encoderInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), encoderInfo);
    WGPUOrigin3D origin = { 0, 0, 0 };
    WGPUExtent3D extent = { 256, 256, 1 };

    // Blit to mip level 1 (256x256)
    EXPECT_NO_THROW({
        device->getBlit()->execute(
            encoder->handle(),
            srcTexture->handle(),
            origin, extent, 0,
            dstTexture->handle(),
            origin, extent, 1, // Destination mip level 1
            WGPUFilterMode_Linear);
    });
}

TEST_F(WebGPUBlitTest, Execute_MipToMip_BlitsCorrectly)
{
    // Create source texture with mip levels
    gfx::backend::webgpu::core::TextureCreateInfo srcInfo{};
    srcInfo.format = WGPUTextureFormat_RGBA8Unorm;
    srcInfo.size = { 1024, 1024, 1 };
    srcInfo.mipLevelCount = 4; // 1024, 512, 256, 128
    srcInfo.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
    auto srcTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), srcInfo);

    // Create destination texture with mip levels
    gfx::backend::webgpu::core::TextureCreateInfo dstInfo{};
    dstInfo.format = WGPUTextureFormat_RGBA8Unorm;
    dstInfo.size = { 512, 512, 1 };
    dstInfo.mipLevelCount = 3; // 512, 256, 128
    dstInfo.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopyDst;
    auto dstTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), dstInfo);

    gfx::backend::webgpu::core::CommandEncoderCreateInfo encoderInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), encoderInfo);
    WGPUOrigin3D origin = { 0, 0, 0 };
    WGPUExtent3D extent = { 256, 256, 1 };

    // Blit from mip level 2 (256x256) to mip level 1 (256x256)
    EXPECT_NO_THROW({
        device->getBlit()->execute(
            encoder->handle(),
            srcTexture->handle(),
            origin, extent, 2, // Source mip level 2
            dstTexture->handle(),
            origin, extent, 1, // Destination mip level 1
            WGPUFilterMode_Linear);
    });
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(WebGPUBlitTest, Execute_SmallTexture_BlitsCorrectly)
{
    // Create 16x16 textures
    gfx::backend::webgpu::core::TextureCreateInfo srcInfo{};
    srcInfo.format = WGPUTextureFormat_RGBA8Unorm;
    srcInfo.size = { 16, 16, 1 };
    srcInfo.mipLevelCount = 1;
    srcInfo.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
    auto srcTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), srcInfo);

    gfx::backend::webgpu::core::TextureCreateInfo dstInfo{};
    dstInfo.format = WGPUTextureFormat_RGBA8Unorm;
    dstInfo.size = { 16, 16, 1 };
    dstInfo.mipLevelCount = 1;
    dstInfo.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopyDst;
    auto dstTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), dstInfo);

    gfx::backend::webgpu::core::CommandEncoderCreateInfo encoderInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), encoderInfo);
    WGPUOrigin3D origin = { 0, 0, 0 };
    WGPUExtent3D extent = { 16, 16, 1 };

    EXPECT_NO_THROW({
        device->getBlit()->execute(
            encoder->handle(),
            srcTexture->handle(),
            origin, extent, 0,
            dstTexture->handle(),
            origin, extent, 0,
            WGPUFilterMode_Linear);
    });
}

TEST_F(WebGPUBlitTest, Execute_LargeTexture_BlitsCorrectly)
{
    // Create 4096x4096 textures
    gfx::backend::webgpu::core::TextureCreateInfo srcInfo{};
    srcInfo.format = WGPUTextureFormat_RGBA8Unorm;
    srcInfo.size = { 4096, 4096, 1 };
    srcInfo.mipLevelCount = 1;
    srcInfo.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
    auto srcTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), srcInfo);

    gfx::backend::webgpu::core::TextureCreateInfo dstInfo{};
    dstInfo.format = WGPUTextureFormat_RGBA8Unorm;
    dstInfo.size = { 2048, 2048, 1 };
    dstInfo.mipLevelCount = 1;
    dstInfo.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopyDst;
    auto dstTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), dstInfo);

    gfx::backend::webgpu::core::CommandEncoderCreateInfo encoderInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), encoderInfo);
    WGPUOrigin3D origin = { 0, 0, 0 };
    WGPUExtent3D srcExtent = { 4096, 4096, 1 };
    WGPUExtent3D dstExtent = { 2048, 2048, 1 };

    EXPECT_NO_THROW({
        device->getBlit()->execute(
            encoder->handle(),
            srcTexture->handle(),
            origin, srcExtent, 0,
            dstTexture->handle(),
            origin, dstExtent, 0,
            WGPUFilterMode_Linear);
    });
}

TEST_F(WebGPUBlitTest, Execute_NonSquareTextures_BlitsCorrectly)
{
    // Create 512x256 source and 256x128 destination
    gfx::backend::webgpu::core::TextureCreateInfo srcInfo{};
    srcInfo.format = WGPUTextureFormat_RGBA8Unorm;
    srcInfo.size = { 512, 256, 1 };
    srcInfo.mipLevelCount = 1;
    srcInfo.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
    auto srcTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), srcInfo);

    gfx::backend::webgpu::core::TextureCreateInfo dstInfo{};
    dstInfo.format = WGPUTextureFormat_RGBA8Unorm;
    dstInfo.size = { 256, 128, 1 };
    dstInfo.mipLevelCount = 1;
    dstInfo.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopyDst;
    auto dstTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), dstInfo);

    gfx::backend::webgpu::core::CommandEncoderCreateInfo encoderInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), encoderInfo);
    WGPUOrigin3D origin = { 0, 0, 0 };
    WGPUExtent3D srcExtent = { 512, 256, 1 };
    WGPUExtent3D dstExtent = { 256, 128, 1 };

    EXPECT_NO_THROW({
        device->getBlit()->execute(
            encoder->handle(),
            srcTexture->handle(),
            origin, srcExtent, 0,
            dstTexture->handle(),
            origin, dstExtent, 0,
            WGPUFilterMode_Linear);
    });
}

TEST_F(WebGPUBlitTest, Execute_MultipleBlit_WorksCorrectly)
{
    // Create source texture
    gfx::backend::webgpu::core::TextureCreateInfo srcInfo{};
    srcInfo.format = WGPUTextureFormat_RGBA8Unorm;
    srcInfo.size = { 256, 256, 1 };
    srcInfo.mipLevelCount = 1;
    srcInfo.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
    auto srcTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), srcInfo);

    // Create destination texture
    gfx::backend::webgpu::core::TextureCreateInfo dstInfo{};
    dstInfo.format = WGPUTextureFormat_RGBA8Unorm;
    dstInfo.size = { 256, 256, 1 };
    dstInfo.mipLevelCount = 1;
    dstInfo.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopyDst;
    auto dstTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), dstInfo);

    gfx::backend::webgpu::core::CommandEncoderCreateInfo encoderInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), encoderInfo);
    WGPUOrigin3D origin = { 0, 0, 0 };
    WGPUExtent3D extent = { 256, 256, 1 };

    // Execute multiple blits
    EXPECT_NO_THROW({
        device->getBlit()->execute(
            encoder->handle(),
            srcTexture->handle(),
            origin, extent, 0,
            dstTexture->handle(),
            origin, extent, 0,
            WGPUFilterMode_Linear);

        device->getBlit()->execute(
            encoder->handle(),
            srcTexture->handle(),
            origin, extent, 0,
            dstTexture->handle(),
            origin, extent, 0,
            WGPUFilterMode_Nearest);
    });
}

} // namespace
