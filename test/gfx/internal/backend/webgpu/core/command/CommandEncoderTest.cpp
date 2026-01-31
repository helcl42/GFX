#include <backend/webgpu/core/command/CommandEncoder.h>
#include <backend/webgpu/core/resource/Buffer.h>
#include <backend/webgpu/core/resource/Texture.h>
#include <backend/webgpu/core/system/Device.h>
#include <backend/webgpu/core/system/Instance.h>

#include <gtest/gtest.h>

#include <memory>

namespace {

class WebGPUCommandEncoderTest : public testing::Test {
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

    std::unique_ptr<gfx::backend::webgpu::core::Instance> instance;
    gfx::backend::webgpu::core::Adapter* adapter = nullptr;
    std::unique_ptr<gfx::backend::webgpu::core::Device> device;
};

TEST_F(WebGPUCommandEncoderTest, CreateCommandEncoder_CreatesSuccessfully)
{
    gfx::backend::webgpu::core::CommandEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), createInfo);

    EXPECT_NE(encoder->handle(), nullptr);
}

TEST_F(WebGPUCommandEncoderTest, Handle_ReturnsValidWGPUCommandEncoder)
{
    gfx::backend::webgpu::core::CommandEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), createInfo);

    WGPUCommandEncoder handle = encoder->handle();
    EXPECT_NE(handle, nullptr);
}

TEST_F(WebGPUCommandEncoderTest, GetDevice_ReturnsCorrectDevice)
{
    gfx::backend::webgpu::core::CommandEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), createInfo);

    EXPECT_EQ(encoder->getDevice(), device.get());
}

TEST_F(WebGPUCommandEncoderTest, IsFinished_InitiallyFalse)
{
    gfx::backend::webgpu::core::CommandEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), createInfo);

    EXPECT_FALSE(encoder->isFinished());
}

TEST_F(WebGPUCommandEncoderTest, MarkFinished_SetsFinishedFlag)
{
    gfx::backend::webgpu::core::CommandEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), createInfo);

    encoder->markFinished();
    EXPECT_TRUE(encoder->isFinished());
}

TEST_F(WebGPUCommandEncoderTest, RecreateIfNeeded_RecreatesAfterFinished)
{
    gfx::backend::webgpu::core::CommandEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), createInfo);

    encoder->markFinished();
    EXPECT_TRUE(encoder->isFinished());

    bool recreated = encoder->recreateIfNeeded();
    EXPECT_TRUE(recreated);
    EXPECT_FALSE(encoder->isFinished());
}

TEST_F(WebGPUCommandEncoderTest, CopyBufferToBuffer_WorksCorrectly)
{
    gfx::backend::webgpu::core::BufferCreateInfo bufferInfo{};
    bufferInfo.size = 1024;
    bufferInfo.usage = WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst;

    auto srcBuffer = std::make_unique<gfx::backend::webgpu::core::Buffer>(device.get(), bufferInfo);
    auto dstBuffer = std::make_unique<gfx::backend::webgpu::core::Buffer>(device.get(), bufferInfo);

    gfx::backend::webgpu::core::CommandEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), createInfo);

    EXPECT_NO_THROW(encoder->copyBufferToBuffer(srcBuffer.get(), 0, dstBuffer.get(), 0, 512));
}

TEST_F(WebGPUCommandEncoderTest, CopyBufferToTexture_WorksCorrectly)
{
    gfx::backend::webgpu::core::BufferCreateInfo bufferInfo{};
    bufferInfo.size = 256 * 256 * 4;
    bufferInfo.usage = WGPUBufferUsage_CopySrc;
    auto buffer = std::make_unique<gfx::backend::webgpu::core::Buffer>(device.get(), bufferInfo);

    gfx::backend::webgpu::core::TextureCreateInfo textureInfo{};
    textureInfo.format = WGPUTextureFormat_RGBA8Unorm;
    textureInfo.size = { 256, 256, 1 };
    textureInfo.usage = WGPUTextureUsage_CopyDst;
    textureInfo.dimension = WGPUTextureDimension_2D;
    textureInfo.mipLevelCount = 1;
    textureInfo.sampleCount = 1;
    textureInfo.arrayLayers = 1;
    auto texture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), textureInfo);

    gfx::backend::webgpu::core::CommandEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), createInfo);

    WGPUOrigin3D origin = { 0, 0, 0 };
    WGPUExtent3D extent = { 256, 256, 1 };
    EXPECT_NO_THROW(encoder->copyBufferToTexture(buffer.get(), 0, texture.get(), origin, extent, 0));
}

TEST_F(WebGPUCommandEncoderTest, CopyTextureToBuffer_WorksCorrectly)
{
    gfx::backend::webgpu::core::TextureCreateInfo textureInfo{};
    textureInfo.format = WGPUTextureFormat_RGBA8Unorm;
    textureInfo.size = { 256, 256, 1 };
    textureInfo.usage = WGPUTextureUsage_CopySrc;
    textureInfo.dimension = WGPUTextureDimension_2D;
    textureInfo.mipLevelCount = 1;
    textureInfo.sampleCount = 1;
    textureInfo.arrayLayers = 1;
    auto texture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), textureInfo);

    gfx::backend::webgpu::core::BufferCreateInfo bufferInfo{};
    bufferInfo.size = 256 * 256 * 4;
    bufferInfo.usage = WGPUBufferUsage_CopyDst;
    auto buffer = std::make_unique<gfx::backend::webgpu::core::Buffer>(device.get(), bufferInfo);

    gfx::backend::webgpu::core::CommandEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), createInfo);

    WGPUOrigin3D origin = { 0, 0, 0 };
    WGPUExtent3D extent = { 256, 256, 1 };
    EXPECT_NO_THROW(encoder->copyTextureToBuffer(texture.get(), origin, 0, buffer.get(), 0, extent));
}

TEST_F(WebGPUCommandEncoderTest, CopyTextureToTexture_WorksCorrectly)
{
    gfx::backend::webgpu::core::TextureCreateInfo textureInfo{};
    textureInfo.format = WGPUTextureFormat_RGBA8Unorm;
    textureInfo.size = { 256, 256, 1 };
    textureInfo.usage = WGPUTextureUsage_CopySrc | WGPUTextureUsage_CopyDst;
    textureInfo.dimension = WGPUTextureDimension_2D;
    textureInfo.mipLevelCount = 1;
    textureInfo.sampleCount = 1;
    textureInfo.arrayLayers = 1;

    auto srcTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), textureInfo);
    auto dstTexture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), textureInfo);

    gfx::backend::webgpu::core::CommandEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), createInfo);

    WGPUOrigin3D srcOrigin = { 0, 0, 0 };
    WGPUOrigin3D dstOrigin = { 0, 0, 0 };
    WGPUExtent3D extent = { 256, 256, 1 };
    EXPECT_NO_THROW(encoder->copyTextureToTexture(srcTexture.get(), srcOrigin, 0, dstTexture.get(), dstOrigin, 0, extent));
}

TEST_F(WebGPUCommandEncoderTest, MultipleCommandEncoders_CanCoexist)
{
    gfx::backend::webgpu::core::CommandEncoderCreateInfo createInfo{};
    auto encoder1 = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), createInfo);
    auto encoder2 = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), createInfo);

    EXPECT_NE(encoder1->handle(), nullptr);
    EXPECT_NE(encoder2->handle(), nullptr);
    EXPECT_NE(encoder1->handle(), encoder2->handle());
}

} // anonymous namespace
