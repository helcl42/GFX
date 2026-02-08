#include <backend/vulkan/core/render/Framebuffer.h>
#include <backend/vulkan/core/render/RenderPass.h>
#include <backend/vulkan/core/resource/Texture.h>
#include <backend/vulkan/core/resource/TextureView.h>
#include <backend/vulkan/core/system/Adapter.h>
#include <backend/vulkan/core/system/Device.h>
#include <backend/vulkan/core/system/Instance.h>

#include <gtest/gtest.h>

// Test Vulkan core Framebuffer class
// These tests verify the internal framebuffer implementation

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class VulkanFramebufferTest : public testing::Test {
protected:
    void SetUp() override
    {
        try {
            gfx::backend::vulkan::core::InstanceCreateInfo instInfo{};
            instance = std::make_unique<gfx::backend::vulkan::core::Instance>(instInfo);

            gfx::backend::vulkan::core::AdapterCreateInfo adapterInfo{};
            adapterInfo.adapterIndex = 0;
            adapter = instance->requestAdapter(adapterInfo);

            gfx::backend::vulkan::core::DeviceCreateInfo deviceInfo{};
            device = std::make_unique<gfx::backend::vulkan::core::Device>(adapter, deviceInfo);
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up Vulkan: " << e.what();
        }
    }

    std::unique_ptr<gfx::backend::vulkan::core::Instance> instance;
    gfx::backend::vulkan::core::Adapter* adapter = nullptr;
    std::unique_ptr<gfx::backend::vulkan::core::Device> device;
};

// ============================================================================
// Basic Creation Tests
// ============================================================================

TEST_F(VulkanFramebufferTest, CreateSingleColorAttachment_CreatesSuccessfully)
{
    // Create render pass
    gfx::backend::vulkan::core::RenderPassCreateInfo rpInfo{};
    gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
    colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    rpInfo.colorAttachments.push_back(colorAtt);
    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), rpInfo);

    // Create texture
    gfx::backend::vulkan::core::TextureCreateInfo texInfo{};
    texInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    texInfo.size = { 800, 600, 1 };
    texInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    texInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    texInfo.mipLevelCount = 1;
    texInfo.imageType = VK_IMAGE_TYPE_2D;
    texInfo.arrayLayers = 1;
    texInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), texInfo);

    // Create texture view
    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;
    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    // Create framebuffer
    gfx::backend::vulkan::core::FramebufferCreateInfo fbInfo{};
    fbInfo.renderPass = renderPass.handle();
    fbInfo.attachments.push_back(textureView.handle());
    fbInfo.width = 800;
    fbInfo.height = 600;
    fbInfo.colorAttachmentCount = 1;
    fbInfo.hasDepthResolve = false;

    gfx::backend::vulkan::core::Framebuffer framebuffer(device.get(), fbInfo);

    EXPECT_NE(framebuffer.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(framebuffer.width(), 800u);
    EXPECT_EQ(framebuffer.height(), 600u);
}

TEST_F(VulkanFramebufferTest, CreateMultipleColorAttachments_CreatesSuccessfully)
{
    // Create render pass
    gfx::backend::vulkan::core::RenderPassCreateInfo rpInfo{};
    for (int i = 0; i < 2; ++i) {
        gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
        colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
        colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
        colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        rpInfo.colorAttachments.push_back(colorAtt);
    }
    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), rpInfo);

    // Create textures and views
    std::vector<std::unique_ptr<gfx::backend::vulkan::core::Texture>> textures;
    std::vector<std::unique_ptr<gfx::backend::vulkan::core::TextureView>> views;
    std::vector<VkImageView> imageViews;

    for (int i = 0; i < 2; ++i) {
        gfx::backend::vulkan::core::TextureCreateInfo texInfo{};
        texInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        texInfo.size = { 1024, 768, 1 };
        texInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        texInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
        texInfo.mipLevelCount = 1;
        texInfo.imageType = VK_IMAGE_TYPE_2D;
        texInfo.arrayLayers = 1;
        texInfo.flags = 0;
        textures.push_back(std::make_unique<gfx::backend::vulkan::core::Texture>(device.get(), texInfo));

        gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.baseMipLevel = 0;
        viewInfo.mipLevelCount = 1;
        viewInfo.baseArrayLayer = 0;
        viewInfo.arrayLayerCount = 1;
        views.push_back(std::make_unique<gfx::backend::vulkan::core::TextureView>(textures[i].get(), viewInfo));
        imageViews.push_back(views[i]->handle());
    }

    // Create framebuffer
    gfx::backend::vulkan::core::FramebufferCreateInfo fbInfo{};
    fbInfo.renderPass = renderPass.handle();
    fbInfo.attachments = imageViews;
    fbInfo.width = 1024;
    fbInfo.height = 768;
    fbInfo.colorAttachmentCount = 2;
    fbInfo.hasDepthResolve = false;

    gfx::backend::vulkan::core::Framebuffer framebuffer(device.get(), fbInfo);

    EXPECT_NE(framebuffer.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(framebuffer.width(), 1024u);
    EXPECT_EQ(framebuffer.height(), 768u);
}

TEST_F(VulkanFramebufferTest, CreateWithDepthAttachment_CreatesSuccessfully)
{
    // Create render pass
    gfx::backend::vulkan::core::RenderPassCreateInfo rpInfo{};

    gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
    colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    rpInfo.colorAttachments.push_back(colorAtt);

    gfx::backend::vulkan::core::RenderPassDepthStencilAttachment depthAtt{};
    depthAtt.target.format = VK_FORMAT_D32_SFLOAT;
    depthAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    depthAtt.target.depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAtt.target.depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAtt.target.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAtt.target.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAtt.target.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    rpInfo.depthStencilAttachment = depthAtt;

    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), rpInfo);

    // Create color texture
    gfx::backend::vulkan::core::TextureCreateInfo colorTexInfo{};
    colorTexInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorTexInfo.size = { 640, 480, 1 };
    colorTexInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    colorTexInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    colorTexInfo.mipLevelCount = 1;
    colorTexInfo.imageType = VK_IMAGE_TYPE_2D;
    colorTexInfo.arrayLayers = 1;
    colorTexInfo.flags = 0;
    gfx::backend::vulkan::core::Texture colorTexture(device.get(), colorTexInfo);

    gfx::backend::vulkan::core::TextureViewCreateInfo colorViewInfo{};
    colorViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    colorViewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorViewInfo.baseMipLevel = 0;
    colorViewInfo.mipLevelCount = 1;
    colorViewInfo.baseArrayLayer = 0;
    colorViewInfo.arrayLayerCount = 1;
    gfx::backend::vulkan::core::TextureView colorView(&colorTexture, colorViewInfo);

    // Create depth texture
    gfx::backend::vulkan::core::TextureCreateInfo depthTexInfo{};
    depthTexInfo.format = VK_FORMAT_D32_SFLOAT;
    depthTexInfo.size = { 640, 480, 1 };
    depthTexInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthTexInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    depthTexInfo.mipLevelCount = 1;
    depthTexInfo.imageType = VK_IMAGE_TYPE_2D;
    depthTexInfo.arrayLayers = 1;
    depthTexInfo.flags = 0;
    gfx::backend::vulkan::core::Texture depthTexture(device.get(), depthTexInfo);

    gfx::backend::vulkan::core::TextureViewCreateInfo depthViewInfo{};
    depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthViewInfo.format = VK_FORMAT_D32_SFLOAT;
    depthViewInfo.baseMipLevel = 0;
    depthViewInfo.mipLevelCount = 1;
    depthViewInfo.baseArrayLayer = 0;
    depthViewInfo.arrayLayerCount = 1;
    gfx::backend::vulkan::core::TextureView depthView(&depthTexture, depthViewInfo);

    // Create framebuffer
    gfx::backend::vulkan::core::FramebufferCreateInfo fbInfo{};
    fbInfo.renderPass = renderPass.handle();
    fbInfo.attachments.push_back(colorView.handle());
    fbInfo.attachments.push_back(depthView.handle());
    fbInfo.width = 640;
    fbInfo.height = 480;
    fbInfo.colorAttachmentCount = 1;
    fbInfo.hasDepthResolve = false;

    gfx::backend::vulkan::core::Framebuffer framebuffer(device.get(), fbInfo);

    EXPECT_NE(framebuffer.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(framebuffer.width(), 640u);
    EXPECT_EQ(framebuffer.height(), 480u);
}

// ============================================================================
// Different Sizes
// ============================================================================

TEST_F(VulkanFramebufferTest, DifferentSizes_CreateSuccessfully)
{
    uint32_t sizes[][2] = {
        { 256, 256 },
        { 1920, 1080 },
        { 4096, 2160 },
        { 128, 1024 }
    };

    for (auto& size : sizes) {
        // Create render pass
        gfx::backend::vulkan::core::RenderPassCreateInfo rpInfo{};
        gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
        colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
        colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
        colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        rpInfo.colorAttachments.push_back(colorAtt);
        gfx::backend::vulkan::core::RenderPass renderPass(device.get(), rpInfo);

        // Create texture
        gfx::backend::vulkan::core::TextureCreateInfo texInfo{};
        texInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        texInfo.size = { size[0], size[1], 1 };
        texInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        texInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
        texInfo.mipLevelCount = 1;
        texInfo.imageType = VK_IMAGE_TYPE_2D;
        texInfo.arrayLayers = 1;
        texInfo.flags = 0;
        gfx::backend::vulkan::core::Texture texture(device.get(), texInfo);

        gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.baseMipLevel = 0;
        viewInfo.mipLevelCount = 1;
        viewInfo.baseArrayLayer = 0;
        viewInfo.arrayLayerCount = 1;
        gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

        // Create framebuffer
        gfx::backend::vulkan::core::FramebufferCreateInfo fbInfo{};
        fbInfo.renderPass = renderPass.handle();
        fbInfo.attachments.push_back(textureView.handle());
        fbInfo.width = size[0];
        fbInfo.height = size[1];
        fbInfo.colorAttachmentCount = 1;
        fbInfo.hasDepthResolve = false;

        gfx::backend::vulkan::core::Framebuffer framebuffer(device.get(), fbInfo);

        EXPECT_NE(framebuffer.handle(), VK_NULL_HANDLE);
        EXPECT_EQ(framebuffer.width(), size[0]);
        EXPECT_EQ(framebuffer.height(), size[1]);
    }
}

// ============================================================================
// Handle Tests
// ============================================================================

TEST_F(VulkanFramebufferTest, GetHandle_ReturnsValidHandle)
{
    // Create render pass
    gfx::backend::vulkan::core::RenderPassCreateInfo rpInfo{};
    gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
    colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    rpInfo.colorAttachments.push_back(colorAtt);
    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), rpInfo);

    // Create texture
    gfx::backend::vulkan::core::TextureCreateInfo texInfo{};
    texInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    texInfo.size = { 512, 512, 1 };
    texInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    texInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    texInfo.mipLevelCount = 1;
    texInfo.imageType = VK_IMAGE_TYPE_2D;
    texInfo.arrayLayers = 1;
    texInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), texInfo);

    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;
    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    // Create framebuffer
    gfx::backend::vulkan::core::FramebufferCreateInfo fbInfo{};
    fbInfo.renderPass = renderPass.handle();
    fbInfo.attachments.push_back(textureView.handle());
    fbInfo.width = 512;
    fbInfo.height = 512;
    fbInfo.colorAttachmentCount = 1;
    fbInfo.hasDepthResolve = false;

    gfx::backend::vulkan::core::Framebuffer framebuffer(device.get(), fbInfo);

    VkFramebuffer handle = framebuffer.handle();
    EXPECT_NE(handle, VK_NULL_HANDLE);
    EXPECT_EQ(framebuffer.handle(), handle);
}

TEST_F(VulkanFramebufferTest, MultipleFramebuffers_HaveUniqueHandles)
{
    // Create render pass
    gfx::backend::vulkan::core::RenderPassCreateInfo rpInfo{};
    gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
    colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    rpInfo.colorAttachments.push_back(colorAtt);
    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), rpInfo);

    // Create first texture and framebuffer
    gfx::backend::vulkan::core::TextureCreateInfo texInfo1{};
    texInfo1.format = VK_FORMAT_R8G8B8A8_UNORM;
    texInfo1.size = { 256, 256, 1 };
    texInfo1.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    texInfo1.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    texInfo1.mipLevelCount = 1;
    texInfo1.imageType = VK_IMAGE_TYPE_2D;
    texInfo1.arrayLayers = 1;
    texInfo1.flags = 0;
    gfx::backend::vulkan::core::Texture texture1(device.get(), texInfo1);

    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo1{};
    viewInfo1.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo1.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo1.baseMipLevel = 0;
    viewInfo1.mipLevelCount = 1;
    viewInfo1.baseArrayLayer = 0;
    viewInfo1.arrayLayerCount = 1;
    gfx::backend::vulkan::core::TextureView textureView1(&texture1, viewInfo1);

    gfx::backend::vulkan::core::FramebufferCreateInfo fbInfo1{};
    fbInfo1.renderPass = renderPass.handle();
    fbInfo1.attachments.push_back(textureView1.handle());
    fbInfo1.width = 256;
    fbInfo1.height = 256;
    fbInfo1.colorAttachmentCount = 1;
    fbInfo1.hasDepthResolve = false;
    gfx::backend::vulkan::core::Framebuffer framebuffer1(device.get(), fbInfo1);

    // Create second texture and framebuffer
    gfx::backend::vulkan::core::TextureCreateInfo texInfo2{};
    texInfo2.format = VK_FORMAT_R8G8B8A8_UNORM;
    texInfo2.size = { 256, 256, 1 };
    texInfo2.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    texInfo2.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    texInfo2.mipLevelCount = 1;
    texInfo2.imageType = VK_IMAGE_TYPE_2D;
    texInfo2.arrayLayers = 1;
    texInfo2.flags = 0;
    gfx::backend::vulkan::core::Texture texture2(device.get(), texInfo2);

    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo2{};
    viewInfo2.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo2.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo2.baseMipLevel = 0;
    viewInfo2.mipLevelCount = 1;
    viewInfo2.baseArrayLayer = 0;
    viewInfo2.arrayLayerCount = 1;
    gfx::backend::vulkan::core::TextureView textureView2(&texture2, viewInfo2);

    gfx::backend::vulkan::core::FramebufferCreateInfo fbInfo2{};
    fbInfo2.renderPass = renderPass.handle();
    fbInfo2.attachments.push_back(textureView2.handle());
    fbInfo2.width = 256;
    fbInfo2.height = 256;
    fbInfo2.colorAttachmentCount = 1;
    fbInfo2.hasDepthResolve = false;
    gfx::backend::vulkan::core::Framebuffer framebuffer2(device.get(), fbInfo2);

    EXPECT_NE(framebuffer1.handle(), framebuffer2.handle());
}

// ============================================================================
// Property Tests
// ============================================================================

TEST_F(VulkanFramebufferTest, WidthAndHeight_ReturnCorrectValues)
{
    // Create render pass
    gfx::backend::vulkan::core::RenderPassCreateInfo rpInfo{};
    gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
    colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    rpInfo.colorAttachments.push_back(colorAtt);
    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), rpInfo);

    // Create texture
    gfx::backend::vulkan::core::TextureCreateInfo texInfo{};
    texInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    texInfo.size = { 1280, 720, 1 };
    texInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    texInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    texInfo.mipLevelCount = 1;
    texInfo.imageType = VK_IMAGE_TYPE_2D;
    texInfo.arrayLayers = 1;
    texInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), texInfo);

    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;
    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    // Create framebuffer
    gfx::backend::vulkan::core::FramebufferCreateInfo fbInfo{};
    fbInfo.renderPass = renderPass.handle();
    fbInfo.attachments.push_back(textureView.handle());
    fbInfo.width = 1280;
    fbInfo.height = 720;
    fbInfo.colorAttachmentCount = 1;
    fbInfo.hasDepthResolve = false;

    gfx::backend::vulkan::core::Framebuffer framebuffer(device.get(), fbInfo);

    EXPECT_EQ(framebuffer.width(), 1280u);
    EXPECT_EQ(framebuffer.height(), 720u);
}

// ============================================================================
// Lifecycle Tests
// ============================================================================

TEST_F(VulkanFramebufferTest, CreateAndDestroy_WorksCorrectly)
{
    // Create render pass
    gfx::backend::vulkan::core::RenderPassCreateInfo rpInfo{};
    gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
    colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    rpInfo.colorAttachments.push_back(colorAtt);
    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), rpInfo);

    // Create texture
    gfx::backend::vulkan::core::TextureCreateInfo texInfo{};
    texInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    texInfo.size = { 400, 300, 1 };
    texInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    texInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    texInfo.mipLevelCount = 1;
    texInfo.imageType = VK_IMAGE_TYPE_2D;
    texInfo.arrayLayers = 1;
    texInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), texInfo);

    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;
    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    {
        gfx::backend::vulkan::core::FramebufferCreateInfo fbInfo{};
        fbInfo.renderPass = renderPass.handle();
        fbInfo.attachments.push_back(textureView.handle());
        fbInfo.width = 400;
        fbInfo.height = 300;
        fbInfo.colorAttachmentCount = 1;
        fbInfo.hasDepthResolve = false;

        gfx::backend::vulkan::core::Framebuffer framebuffer(device.get(), fbInfo);

        EXPECT_NE(framebuffer.handle(), VK_NULL_HANDLE);
    }
    // Framebuffer destroyed, no crash
}

} // namespace
