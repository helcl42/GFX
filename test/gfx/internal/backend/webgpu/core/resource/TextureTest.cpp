#include <backend/webgpu/core/resource/Texture.h>
#include <backend/webgpu/core/system/Device.h>
#include <backend/webgpu/core/system/Instance.h>

#include <gtest/gtest.h>

#include <memory>

namespace {

class WebGPUTextureTest : public testing::Test {
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

TEST_F(WebGPUTextureTest, CreateTexture2D_WithBasicSettings)
{
    gfx::backend::webgpu::core::TextureCreateInfo createInfo{};
    createInfo.format = WGPUTextureFormat_RGBA8Unorm;
    createInfo.size = { 256, 256, 1 };
    createInfo.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    createInfo.dimension = WGPUTextureDimension_2D;
    createInfo.mipLevelCount = 1;
    createInfo.sampleCount = 1;
    createInfo.arrayLayers = 1;

    auto texture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), createInfo);

    EXPECT_NE(texture->handle(), nullptr);
    EXPECT_EQ(texture->getFormat(), WGPUTextureFormat_RGBA8Unorm);
}

TEST_F(WebGPUTextureTest, GetSize_ReturnsCorrectDimensions)
{
    gfx::backend::webgpu::core::TextureCreateInfo createInfo{};
    createInfo.format = WGPUTextureFormat_RGBA8Unorm;
    createInfo.size = { 512, 384, 1 };
    createInfo.usage = WGPUTextureUsage_TextureBinding;
    createInfo.dimension = WGPUTextureDimension_2D;
    createInfo.mipLevelCount = 1;
    createInfo.sampleCount = 1;
    createInfo.arrayLayers = 1;

    auto texture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), createInfo);

    WGPUExtent3D size = texture->getSize();
    EXPECT_EQ(size.width, 512);
    EXPECT_EQ(size.height, 384);
    EXPECT_EQ(size.depthOrArrayLayers, 1);
}

TEST_F(WebGPUTextureTest, CreateTexture_WithMipmaps)
{
    gfx::backend::webgpu::core::TextureCreateInfo createInfo{};
    createInfo.format = WGPUTextureFormat_RGBA8Unorm;
    createInfo.size = { 256, 256, 1 };
    createInfo.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment;
    createInfo.dimension = WGPUTextureDimension_2D;
    createInfo.mipLevelCount = 5;
    createInfo.sampleCount = 1;
    createInfo.arrayLayers = 1;

    auto texture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), createInfo);

    EXPECT_NE(texture->handle(), nullptr);
    EXPECT_EQ(texture->getMipLevels(), 5);
}

TEST_F(WebGPUTextureTest, MultipleTextures_CanCoexist)
{
    gfx::backend::webgpu::core::TextureCreateInfo createInfo{};
    createInfo.format = WGPUTextureFormat_RGBA8Unorm;
    createInfo.size = { 128, 128, 1 };
    createInfo.usage = WGPUTextureUsage_TextureBinding;
    createInfo.dimension = WGPUTextureDimension_2D;
    createInfo.mipLevelCount = 1;
    createInfo.sampleCount = 1;
    createInfo.arrayLayers = 1;

    auto texture1 = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), createInfo);
    auto texture2 = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), createInfo);

    EXPECT_NE(texture1->handle(), nullptr);
    EXPECT_NE(texture2->handle(), nullptr);
    EXPECT_NE(texture1->handle(), texture2->handle());
}

} // anonymous namespace
