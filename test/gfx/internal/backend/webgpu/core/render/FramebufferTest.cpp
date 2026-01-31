#include <backend/webgpu/core/render/Framebuffer.h>
#include <backend/webgpu/core/resource/Texture.h>
#include <backend/webgpu/core/resource/TextureView.h>
#include <backend/webgpu/core/system/Device.h>
#include <backend/webgpu/core/system/Instance.h>

#include <gtest/gtest.h>

#include <memory>

namespace {

class WebGPUFramebufferTest : public testing::Test {
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

TEST_F(WebGPUFramebufferTest, CreateFramebuffer_WithColorAttachment)
{
    // Create texture
    gfx::backend::webgpu::core::TextureCreateInfo texInfo{};
    texInfo.format = WGPUTextureFormat_RGBA8Unorm;
    texInfo.size = {800, 600, 1};
    texInfo.usage = WGPUTextureUsage_RenderAttachment;
    texInfo.dimension = WGPUTextureDimension_2D;
    texInfo.mipLevelCount = 1;
    texInfo.sampleCount = 1;
    texInfo.arrayLayers = 1;
    auto texture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), texInfo);

    // Create texture view
    gfx::backend::webgpu::core::TextureViewCreateInfo viewInfo{};
    viewInfo.format = WGPUTextureFormat_RGBA8Unorm;
    viewInfo.viewDimension = WGPUTextureViewDimension_2D;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;
    auto textureView = std::make_unique<gfx::backend::webgpu::core::TextureView>(texture.get(), viewInfo);

    // Create framebuffer
    gfx::backend::webgpu::core::FramebufferCreateInfo createInfo{};
    createInfo.colorAttachmentViews.push_back(textureView.get());
    createInfo.width = 800;
    createInfo.height = 600;

    auto framebuffer = std::make_unique<gfx::backend::webgpu::core::Framebuffer>(device.get(), createInfo);

    EXPECT_EQ(framebuffer->getDevice(), device.get());
    EXPECT_EQ(framebuffer->getCreateInfo().width, 800);
    EXPECT_EQ(framebuffer->getCreateInfo().height, 600);
}

TEST_F(WebGPUFramebufferTest, GetDevice_ReturnsCorrectDevice)
{
    gfx::backend::webgpu::core::TextureCreateInfo texInfo{};
    texInfo.format = WGPUTextureFormat_BGRA8Unorm;
    texInfo.size = {1024, 768, 1};
    texInfo.usage = WGPUTextureUsage_RenderAttachment;
    texInfo.dimension = WGPUTextureDimension_2D;
    texInfo.mipLevelCount = 1;
    texInfo.sampleCount = 1;
    texInfo.arrayLayers = 1;
    auto texture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), texInfo);

    gfx::backend::webgpu::core::TextureViewCreateInfo viewInfo{};
    viewInfo.format = WGPUTextureFormat_BGRA8Unorm;
    viewInfo.viewDimension = WGPUTextureViewDimension_2D;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;
    auto textureView = std::make_unique<gfx::backend::webgpu::core::TextureView>(texture.get(), viewInfo);

    gfx::backend::webgpu::core::FramebufferCreateInfo createInfo{};
    createInfo.colorAttachmentViews.push_back(textureView.get());
    createInfo.width = 1024;
    createInfo.height = 768;

    auto framebuffer = std::make_unique<gfx::backend::webgpu::core::Framebuffer>(device.get(), createInfo);

    EXPECT_EQ(framebuffer->getDevice(), device.get());
}

TEST_F(WebGPUFramebufferTest, Destructor_CleansUpResources)
{
    gfx::backend::webgpu::core::TextureCreateInfo texInfo{};
    texInfo.format = WGPUTextureFormat_RGBA8Unorm;
    texInfo.size = {640, 480, 1};
    texInfo.usage = WGPUTextureUsage_RenderAttachment;
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
    auto textureView = std::make_unique<gfx::backend::webgpu::core::TextureView>(texture.get(), viewInfo);

    {
        gfx::backend::webgpu::core::FramebufferCreateInfo createInfo{};
        createInfo.colorAttachmentViews.push_back(textureView.get());
        createInfo.width = 640;
        createInfo.height = 480;

        auto framebuffer = std::make_unique<gfx::backend::webgpu::core::Framebuffer>(device.get(), createInfo);
        EXPECT_EQ(framebuffer->getDevice(), device.get());
    }

    // If we reach here without crashing, cleanup succeeded
    SUCCEED();
}

} // anonymous namespace
