#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

#include <memory>

// ===========================================================================
// Framebuffer Test Suite
// ===========================================================================

namespace {

class GfxCppFramebufferTest : public testing::TestWithParam<gfx::Backend> {
protected:
    void SetUp() override
    {
        backend = GetParam();
        try {
            gfx::InstanceDescriptor instanceDesc{
                .backend = backend
            };
            instance = gfx::createInstance(instanceDesc);

            gfx::AdapterDescriptor adapterDesc{
                .adapterIndex = 0
            };
            adapter = instance->requestAdapter(adapterDesc);

            gfx::DeviceDescriptor deviceDesc{
                .label = "Test Device"
            };
            device = adapter->createDevice(deviceDesc);
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up GFX device for backend "
                         << (backend == gfx::Backend::Vulkan ? "Vulkan" : "WebGPU")
                         << ": " << e.what();
        }
    }

    void TearDown() override
    {
        device.reset();
        adapter.reset();
        instance.reset();
    }

    gfx::Backend backend;
    std::shared_ptr<gfx::Instance> instance;
    std::shared_ptr<gfx::Adapter> adapter;
    std::shared_ptr<gfx::Device> device;
};

// ===========================================================================
// Test Cases
// ===========================================================================

// Test: Create basic Framebuffer with single color attachment
TEST_P(GfxCppFramebufferTest, CreateBasicFramebuffer)
{
    // Create render pass
    gfx::RenderPassCreateDescriptor renderPassDesc{
        .label = "Basic Render Pass",
        .colorAttachments = {
            gfx::RenderPassColorAttachment{
                .target = {
                    .format = gfx::TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = gfx::SampleCount::Count1,
                    .ops = { gfx::LoadOp::Clear, gfx::StoreOp::Store },
                    .finalLayout = gfx::TextureLayout::ColorAttachment } } }
    };
    auto renderPass = device->createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create texture
    auto texture = device->createTexture({ .type = gfx::TextureType::Texture2D, .size = { 256, 256, 1 }, .format = gfx::TextureFormat::R8G8B8A8Unorm, .usage = gfx::TextureUsage::RenderAttachment });
    ASSERT_NE(texture, nullptr);

    // Create texture view
    auto textureView = texture->createView({ .viewType = gfx::TextureViewType::View2D, .format = gfx::TextureFormat::R8G8B8A8Unorm });
    ASSERT_NE(textureView, nullptr);

    // Create framebuffer
    auto framebuffer = device->createFramebuffer({ .label = "Basic Framebuffer", .renderPass = renderPass, .colorAttachments = { gfx::FramebufferColorAttachment{ .view = textureView } }, .extent = { 256, 256 } });
    EXPECT_NE(framebuffer, nullptr);
}

// Test: Create Framebuffer with empty label
TEST_P(GfxCppFramebufferTest, CreateFramebufferWithEmptyLabel)
{
    gfx::RenderPassCreateDescriptor renderPassDesc{
        .colorAttachments = {
            gfx::RenderPassColorAttachment{
                .target = {
                    .format = gfx::TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = gfx::SampleCount::Count1,
                    .ops = { gfx::LoadOp::Clear, gfx::StoreOp::Store },
                    .finalLayout = gfx::TextureLayout::ColorAttachment } } }
    };
    auto renderPass = device->createRenderPass(renderPassDesc);

    auto texture = device->createTexture({ .type = gfx::TextureType::Texture2D, .size = { 256, 256, 1 }, .format = gfx::TextureFormat::R8G8B8A8Unorm, .usage = gfx::TextureUsage::RenderAttachment });
    auto textureView = texture->createView({ .viewType = gfx::TextureViewType::View2D, .format = gfx::TextureFormat::R8G8B8A8Unorm });

    auto framebuffer = device->createFramebuffer({ .label = "", .renderPass = renderPass, .colorAttachments = { gfx::FramebufferColorAttachment{ .view = textureView } }, .extent = { 256, 256 } });
    EXPECT_NE(framebuffer, nullptr);
}

// Test: Create Framebuffer with multiple color attachments
TEST_P(GfxCppFramebufferTest, CreateFramebufferWithMultipleColorAttachments)
{
    // Create render pass with two color attachments
    gfx::RenderPassCreateDescriptor renderPassDesc{
        .colorAttachments = {
            gfx::RenderPassColorAttachment{
                .target = {
                    .format = gfx::TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = gfx::SampleCount::Count1,
                    .ops = { gfx::LoadOp::Clear, gfx::StoreOp::Store },
                    .finalLayout = gfx::TextureLayout::ColorAttachment } },
            gfx::RenderPassColorAttachment{ .target = { .format = gfx::TextureFormat::R16G16B16A16Float, .sampleCount = gfx::SampleCount::Count1, .ops = { gfx::LoadOp::Clear, gfx::StoreOp::Store }, .finalLayout = gfx::TextureLayout::ColorAttachment } } }
    };
    auto renderPass = device->createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create first texture
    auto texture1 = device->createTexture({ .type = gfx::TextureType::Texture2D, .size = { 512, 512, 1 }, .format = gfx::TextureFormat::R8G8B8A8Unorm, .usage = gfx::TextureUsage::RenderAttachment });
    ASSERT_NE(texture1, nullptr);
    auto textureView1 = texture1->createView({ .viewType = gfx::TextureViewType::View2D, .format = gfx::TextureFormat::R8G8B8A8Unorm });
    ASSERT_NE(textureView1, nullptr);

    // Create second texture
    auto texture2 = device->createTexture({ .type = gfx::TextureType::Texture2D, .size = { 512, 512, 1 }, .format = gfx::TextureFormat::R16G16B16A16Float, .usage = gfx::TextureUsage::RenderAttachment });
    ASSERT_NE(texture2, nullptr);
    auto textureView2 = texture2->createView({ .viewType = gfx::TextureViewType::View2D, .format = gfx::TextureFormat::R16G16B16A16Float });
    ASSERT_NE(textureView2, nullptr);

    // Create framebuffer with multiple attachments
    auto framebuffer = device->createFramebuffer({ .label = "Multiple Attachments Framebuffer",
        .renderPass = renderPass,
        .colorAttachments = {
            gfx::FramebufferColorAttachment{ .view = textureView1 },
            gfx::FramebufferColorAttachment{ .view = textureView2 } },
        .extent = { 512, 512 } });
    EXPECT_NE(framebuffer, nullptr);
}

// Test: Create Framebuffer with depth attachment
TEST_P(GfxCppFramebufferTest, CreateFramebufferWithDepthAttachment)
{
    // Create render pass with color and depth attachments
    gfx::RenderPassDepthStencilAttachment depthAttachment = {
        .target = {
            .format = gfx::TextureFormat::Depth32Float,
            .sampleCount = gfx::SampleCount::Count1,
            .depthOps = { gfx::LoadOp::Clear, gfx::StoreOp::Store },
            .stencilOps = { gfx::LoadOp::DontCare, gfx::StoreOp::DontCare },
            .finalLayout = gfx::TextureLayout::DepthStencilAttachment }
    };

    gfx::RenderPassCreateDescriptor renderPassDesc{
        .colorAttachments = {
            gfx::RenderPassColorAttachment{
                .target = {
                    .format = gfx::TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = gfx::SampleCount::Count1,
                    .ops = { gfx::LoadOp::Clear, gfx::StoreOp::Store },
                    .finalLayout = gfx::TextureLayout::ColorAttachment } } },
        .depthStencilAttachment = depthAttachment
    };
    auto renderPass = device->createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create color texture
    auto colorTexture = device->createTexture({ .type = gfx::TextureType::Texture2D, .size = { 1024, 768, 1 }, .format = gfx::TextureFormat::R8G8B8A8Unorm, .usage = gfx::TextureUsage::RenderAttachment });
    ASSERT_NE(colorTexture, nullptr);
    auto colorView = colorTexture->createView({ .viewType = gfx::TextureViewType::View2D, .format = gfx::TextureFormat::R8G8B8A8Unorm });
    ASSERT_NE(colorView, nullptr);

    // Create depth texture
    auto depthTexture = device->createTexture({ .type = gfx::TextureType::Texture2D, .size = { 1024, 768, 1 }, .format = gfx::TextureFormat::Depth32Float, .usage = gfx::TextureUsage::RenderAttachment });
    ASSERT_NE(depthTexture, nullptr);
    auto depthView = depthTexture->createView({ .viewType = gfx::TextureViewType::View2D, .format = gfx::TextureFormat::Depth32Float });
    ASSERT_NE(depthView, nullptr);

    // Create framebuffer with depth attachment
    gfx::FramebufferDepthStencilAttachment fbDepthAttachment = { .view = depthView };

    auto framebuffer = device->createFramebuffer({ .label = "Depth Framebuffer",
        .renderPass = renderPass,
        .colorAttachments = { gfx::FramebufferColorAttachment{ .view = colorView } },
        .depthStencilAttachment = fbDepthAttachment,
        .extent = { 1024, 768 } });
    EXPECT_NE(framebuffer, nullptr);
}

// Test: Create Framebuffer with different sizes
TEST_P(GfxCppFramebufferTest, CreateFramebufferWithDifferentSizes)
{
    // Create render pass once
    gfx::RenderPassCreateDescriptor renderPassDesc{
        .colorAttachments = {
            gfx::RenderPassColorAttachment{
                .target = {
                    .format = gfx::TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = gfx::SampleCount::Count1,
                    .ops = { gfx::LoadOp::Clear, gfx::StoreOp::Store },
                    .finalLayout = gfx::TextureLayout::ColorAttachment } } }
    };
    auto renderPass = device->createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    struct {
        uint32_t width;
        uint32_t height;
    } sizes[] = {
        { 128, 128 },
        { 256, 256 },
        { 512, 512 },
        { 1920, 1080 },
        { 3840, 2160 }
    };

    for (auto size : sizes) {
        auto texture = device->createTexture({ .type = gfx::TextureType::Texture2D, .size = { size.width, size.height, 1 }, .format = gfx::TextureFormat::R8G8B8A8Unorm, .usage = gfx::TextureUsage::RenderAttachment });
        ASSERT_NE(texture, nullptr);

        auto textureView = texture->createView({ .viewType = gfx::TextureViewType::View2D, .format = gfx::TextureFormat::R8G8B8A8Unorm });
        ASSERT_NE(textureView, nullptr);

        auto framebuffer = device->createFramebuffer({ .renderPass = renderPass, .colorAttachments = { gfx::FramebufferColorAttachment{ .view = textureView } }, .extent = { size.width, size.height } });
        EXPECT_NE(framebuffer, nullptr);
    }
}

// Test: Create Framebuffer with null render pass (should throw exception)
TEST_P(GfxCppFramebufferTest, CreateFramebufferWithNullRenderPass)
{
    auto texture = device->createTexture({ .type = gfx::TextureType::Texture2D, .size = { 256, 256, 1 }, .format = gfx::TextureFormat::R8G8B8A8Unorm, .usage = gfx::TextureUsage::RenderAttachment });
    auto textureView = texture->createView({ .viewType = gfx::TextureViewType::View2D, .format = gfx::TextureFormat::R8G8B8A8Unorm });

    EXPECT_THROW(
        device->createFramebuffer({ .renderPass = nullptr, .colorAttachments = { gfx::FramebufferColorAttachment{ .view = textureView } }, .extent = { 256, 256 } }),
        std::exception);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppFramebufferTest,
    testing::Values(gfx::Backend::Vulkan, gfx::Backend::WebGPU),
    [](const testing::TestParamInfo<gfx::Backend>& info) {
        return info.param == gfx::Backend::Vulkan ? "Vulkan" : "WebGPU";
    });

} // namespace
