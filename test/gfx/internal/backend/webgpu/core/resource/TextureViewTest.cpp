#include <backend/webgpu/core/resource/Texture.h>
#include <backend/webgpu/core/resource/TextureView.h>
#include <backend/webgpu/core/system/Device.h>
#include <backend/webgpu/core/system/Instance.h>

#include <gtest/gtest.h>

#include <memory>

namespace {

class WebGPUTextureViewTest : public testing::Test {
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

TEST_F(WebGPUTextureViewTest, CreateTextureView_FromTexture)
{
    gfx::backend::webgpu::core::TextureCreateInfo texInfo{};
    texInfo.format = WGPUTextureFormat_RGBA8Unorm;
    texInfo.size = { 256, 256, 1 };
    texInfo.usage = WGPUTextureUsage_TextureBinding;
    texInfo.dimension = WGPUTextureDimension_2D;
    texInfo.mipLevelCount = 1;
    texInfo.sampleCount = 1;
    texInfo.arrayLayers = 1;
    auto texture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), texInfo);

    gfx::backend::webgpu::core::TextureViewCreateInfo viewInfo{};
    viewInfo.format = WGPUTextureFormat_RGBA8Unorm;
    viewInfo.viewDimension = WGPUTextureViewDimension_2D;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;

    auto view = std::make_unique<gfx::backend::webgpu::core::TextureView>(texture.get(), viewInfo);

    EXPECT_NE(view->handle(), nullptr);
    EXPECT_EQ(view->getTexture(), texture.get());
}

TEST_F(WebGPUTextureViewTest, Handle_ReturnsValidWGPUTextureView)
{
    gfx::backend::webgpu::core::TextureCreateInfo texInfo{};
    texInfo.format = WGPUTextureFormat_RGBA8Unorm;
    texInfo.size = { 128, 128, 1 };
    texInfo.usage = WGPUTextureUsage_TextureBinding;
    texInfo.dimension = WGPUTextureDimension_2D;
    texInfo.mipLevelCount = 1;
    texInfo.sampleCount = 1;
    texInfo.arrayLayers = 1;
    auto texture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), texInfo);

    gfx::backend::webgpu::core::TextureViewCreateInfo viewInfo{};
    viewInfo.format = WGPUTextureFormat_RGBA8Unorm;
    viewInfo.viewDimension = WGPUTextureViewDimension_2D;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;

    auto view = std::make_unique<gfx::backend::webgpu::core::TextureView>(texture.get(), viewInfo);

    WGPUTextureView handle = view->handle();
    EXPECT_NE(handle, nullptr);
}

TEST_F(WebGPUTextureViewTest, CreateTextureView_WithMipLevel)
{
    gfx::backend::webgpu::core::TextureCreateInfo texInfo{};
    texInfo.format = WGPUTextureFormat_RGBA8Unorm;
    texInfo.size = { 256, 256, 1 };
    texInfo.usage = WGPUTextureUsage_TextureBinding;
    texInfo.dimension = WGPUTextureDimension_2D;
    texInfo.mipLevelCount = 4;
    texInfo.sampleCount = 1;
    texInfo.arrayLayers = 1;
    auto texture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), texInfo);

    gfx::backend::webgpu::core::TextureViewCreateInfo viewInfo{};
    viewInfo.format = WGPUTextureFormat_RGBA8Unorm;
    viewInfo.viewDimension = WGPUTextureViewDimension_2D;
    viewInfo.baseMipLevel = 1;
    viewInfo.mipLevelCount = 2;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;

    auto view = std::make_unique<gfx::backend::webgpu::core::TextureView>(texture.get(), viewInfo);

    EXPECT_NE(view->handle(), nullptr);
}

TEST_F(WebGPUTextureViewTest, MultipleViews_FromSameTexture)
{
    gfx::backend::webgpu::core::TextureCreateInfo texInfo{};
    texInfo.format = WGPUTextureFormat_RGBA8Unorm;
    texInfo.size = { 256, 256, 1 };
    texInfo.usage = WGPUTextureUsage_TextureBinding;
    texInfo.dimension = WGPUTextureDimension_2D;
    texInfo.mipLevelCount = 4;
    texInfo.sampleCount = 1;
    texInfo.arrayLayers = 1;
    auto texture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), texInfo);

    gfx::backend::webgpu::core::TextureViewCreateInfo viewInfo1{};
    viewInfo1.format = WGPUTextureFormat_RGBA8Unorm;
    viewInfo1.viewDimension = WGPUTextureViewDimension_2D;
    viewInfo1.baseMipLevel = 0;
    viewInfo1.mipLevelCount = 1;
    viewInfo1.baseArrayLayer = 0;
    viewInfo1.arrayLayerCount = 1;

    gfx::backend::webgpu::core::TextureViewCreateInfo viewInfo2{};
    viewInfo2.format = WGPUTextureFormat_RGBA8Unorm;
    viewInfo2.viewDimension = WGPUTextureViewDimension_2D;
    viewInfo2.baseMipLevel = 1;
    viewInfo2.mipLevelCount = 1;
    viewInfo2.baseArrayLayer = 0;
    viewInfo2.arrayLayerCount = 1;

    auto view1 = std::make_unique<gfx::backend::webgpu::core::TextureView>(texture.get(), viewInfo1);
    auto view2 = std::make_unique<gfx::backend::webgpu::core::TextureView>(texture.get(), viewInfo2);

    EXPECT_NE(view1->handle(), nullptr);
    EXPECT_NE(view2->handle(), nullptr);
    EXPECT_NE(view1->handle(), view2->handle());
    EXPECT_EQ(view1->getTexture(), texture.get());
    EXPECT_EQ(view2->getTexture(), texture.get());
}

TEST_F(WebGPUTextureViewTest, Destructor_CleansUpResources)
{
    gfx::backend::webgpu::core::TextureCreateInfo texInfo{};
    texInfo.format = WGPUTextureFormat_RGBA8Unorm;
    texInfo.size = { 128, 128, 1 };
    texInfo.usage = WGPUTextureUsage_TextureBinding;
    texInfo.dimension = WGPUTextureDimension_2D;
    texInfo.mipLevelCount = 1;
    texInfo.sampleCount = 1;
    texInfo.arrayLayers = 1;
    auto texture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), texInfo);

    {
        gfx::backend::webgpu::core::TextureViewCreateInfo viewInfo{};
        viewInfo.format = WGPUTextureFormat_RGBA8Unorm;
        viewInfo.viewDimension = WGPUTextureViewDimension_2D;
        viewInfo.baseMipLevel = 0;
        viewInfo.mipLevelCount = 1;
        viewInfo.baseArrayLayer = 0;
        viewInfo.arrayLayerCount = 1;

        auto view = std::make_unique<gfx::backend::webgpu::core::TextureView>(texture.get(), viewInfo);
        EXPECT_NE(view->handle(), nullptr);
    }

    // If we reach here without crashing, cleanup succeeded
    SUCCEED();
}

} // anonymous namespace
