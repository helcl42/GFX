#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

#include <memory>

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxCppRenderPassEncoderTest : public testing::TestWithParam<gfx::Backend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        try {
            instance = gfx::createInstance({ .backend = backend, .enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG } });
            adapter = instance->requestAdapter({ .adapterIndex = 0 });
            device = adapter->createDevice({ .label = "Test Device" });
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up: " << e.what();
        }
    }

    void TearDown() override
    {
        device.reset();
        adapter.reset();
        instance.reset();
    }

    gfx::Backend backend = gfx::Backend::Vulkan;
    std::shared_ptr<gfx::Instance> instance;
    std::shared_ptr<gfx::Adapter> adapter;
    std::shared_ptr<gfx::Device> device;
};

// NULL parameter validation tests
TEST_P(GfxCppRenderPassEncoderTest, SetPipelineWithNullPipeline)
{
    ASSERT_NE(device, nullptr);

    auto encoder = device->createCommandEncoder({});
    ASSERT_NE(encoder, nullptr);

    gfx::RenderPassCreateDescriptor rpDesc{
        .colorAttachments = {
            gfx::RenderPassColorAttachment{
                .target = {
                    .format = gfx::TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = gfx::SampleCount::Count1,
                    .ops = { gfx::LoadOp::Clear, gfx::StoreOp::Store },
                    .finalLayout = gfx::TextureLayout::ColorAttachment } } }
    };
    auto renderPass = device->createRenderPass(rpDesc);
    ASSERT_NE(renderPass, nullptr);

    auto texture = device->createTexture({ .type = gfx::TextureType::Texture2D, .size = { 256, 256, 1 }, .format = gfx::TextureFormat::R8G8B8A8Unorm, .usage = gfx::TextureUsage::RenderAttachment });
    ASSERT_NE(texture, nullptr);

    auto view = texture->createView({ .viewType = gfx::TextureViewType::View2D, .format = gfx::TextureFormat::R8G8B8A8Unorm });
    ASSERT_NE(view, nullptr);

    auto framebuffer = device->createFramebuffer({ .renderPass = renderPass, .colorAttachments = { gfx::FramebufferColorAttachment{ .view = view } }, .extent = { 256, 256 } });
    ASSERT_NE(framebuffer, nullptr);

    auto renderPassEncoder = encoder->beginRenderPass({ .framebuffer = framebuffer });
    ASSERT_NE(renderPassEncoder, nullptr);

    // Null pipeline should throw
    EXPECT_THROW(renderPassEncoder->setPipeline(nullptr), std::invalid_argument);
}

TEST_P(GfxCppRenderPassEncoderTest, SetBindGroupWithNullBindGroup)
{
    ASSERT_NE(device, nullptr);

    auto encoder = device->createCommandEncoder({});
    ASSERT_NE(encoder, nullptr);

    gfx::RenderPassCreateDescriptor rpDesc{
        .colorAttachments = { gfx::RenderPassColorAttachment{ .target = { .format = gfx::TextureFormat::R8G8B8A8Unorm, .sampleCount = gfx::SampleCount::Count1, .ops = { gfx::LoadOp::Clear, gfx::StoreOp::Store }, .finalLayout = gfx::TextureLayout::ColorAttachment } } }
    };
    auto renderPass = device->createRenderPass(rpDesc);
    ASSERT_NE(renderPass, nullptr);

    auto texture = device->createTexture({ .type = gfx::TextureType::Texture2D, .size = { 256, 256, 1 }, .format = gfx::TextureFormat::R8G8B8A8Unorm, .usage = gfx::TextureUsage::RenderAttachment });
    auto view = texture->createView({ .viewType = gfx::TextureViewType::View2D, .format = gfx::TextureFormat::R8G8B8A8Unorm });
    auto framebuffer = device->createFramebuffer({ .renderPass = renderPass, .colorAttachments = { gfx::FramebufferColorAttachment{ .view = view } }, .extent = { 256, 256 } });

    auto renderPassEncoder = encoder->beginRenderPass({ .framebuffer = framebuffer });
    ASSERT_NE(renderPassEncoder, nullptr);

    // Null bind group should throw
    EXPECT_THROW(renderPassEncoder->setBindGroup(0, nullptr), std::invalid_argument);
}

TEST_P(GfxCppRenderPassEncoderTest, SetVertexBufferWithNullBuffer)
{
    auto encoder = device->createCommandEncoder({});
    ASSERT_NE(encoder, nullptr);

    gfx::RenderPassCreateDescriptor rpDesc{
        .colorAttachments = { gfx::RenderPassColorAttachment{ .target = { .format = gfx::TextureFormat::R8G8B8A8Unorm, .sampleCount = gfx::SampleCount::Count1, .ops = { gfx::LoadOp::Clear, gfx::StoreOp::Store }, .finalLayout = gfx::TextureLayout::ColorAttachment } } }
    };
    auto renderPass = device->createRenderPass(rpDesc);

    auto texture = device->createTexture({ .type = gfx::TextureType::Texture2D, .size = { 256, 256, 1 }, .format = gfx::TextureFormat::R8G8B8A8Unorm, .usage = gfx::TextureUsage::RenderAttachment });
    auto view = texture->createView({ .viewType = gfx::TextureViewType::View2D, .format = gfx::TextureFormat::R8G8B8A8Unorm });
    auto framebuffer = device->createFramebuffer({ .renderPass = renderPass, .colorAttachments = { gfx::FramebufferColorAttachment{ .view = view } }, .extent = { 256, 256 } });

    auto renderPassEncoder = encoder->beginRenderPass({ .framebuffer = framebuffer });
    ASSERT_NE(renderPassEncoder, nullptr);

    // Null vertex buffer should throw
    EXPECT_THROW(renderPassEncoder->setVertexBuffer(0, nullptr), std::invalid_argument);
}

TEST_P(GfxCppRenderPassEncoderTest, SetIndexBufferWithNullBuffer)
{
    auto encoder = device->createCommandEncoder({});
    ASSERT_NE(encoder, nullptr);

    gfx::RenderPassCreateDescriptor rpDesc{
        .colorAttachments = { gfx::RenderPassColorAttachment{ .target = { .format = gfx::TextureFormat::R8G8B8A8Unorm, .sampleCount = gfx::SampleCount::Count1, .ops = { gfx::LoadOp::Clear, gfx::StoreOp::Store }, .finalLayout = gfx::TextureLayout::ColorAttachment } } }
    };
    auto renderPass = device->createRenderPass(rpDesc);

    auto texture = device->createTexture({ .type = gfx::TextureType::Texture2D, .size = { 256, 256, 1 }, .format = gfx::TextureFormat::R8G8B8A8Unorm, .usage = gfx::TextureUsage::RenderAttachment });
    auto view = texture->createView({ .viewType = gfx::TextureViewType::View2D, .format = gfx::TextureFormat::R8G8B8A8Unorm });
    auto framebuffer = device->createFramebuffer({ .renderPass = renderPass, .colorAttachments = { gfx::FramebufferColorAttachment{ .view = view } }, .extent = { 256, 256 } });

    auto renderPassEncoder = encoder->beginRenderPass({ .framebuffer = framebuffer });
    ASSERT_NE(renderPassEncoder, nullptr);

    // Null index buffer should throw
    EXPECT_THROW(renderPassEncoder->setIndexBuffer(nullptr, gfx::IndexFormat::Uint16), std::invalid_argument);
}

TEST_P(GfxCppRenderPassEncoderTest, SetViewportValid)
{
    auto encoder = device->createCommandEncoder({});
    ASSERT_NE(encoder, nullptr);

    gfx::RenderPassCreateDescriptor rpDesc{
        .colorAttachments = { gfx::RenderPassColorAttachment{ .target = { .format = gfx::TextureFormat::R8G8B8A8Unorm, .sampleCount = gfx::SampleCount::Count1, .ops = { gfx::LoadOp::Clear, gfx::StoreOp::Store }, .finalLayout = gfx::TextureLayout::ColorAttachment } } }
    };
    auto renderPass = device->createRenderPass(rpDesc);

    auto texture = device->createTexture({ .type = gfx::TextureType::Texture2D, .size = { 256, 256, 1 }, .format = gfx::TextureFormat::R8G8B8A8Unorm, .usage = gfx::TextureUsage::RenderAttachment });
    auto view = texture->createView({ .viewType = gfx::TextureViewType::View2D, .format = gfx::TextureFormat::R8G8B8A8Unorm });
    auto framebuffer = device->createFramebuffer({ .renderPass = renderPass, .colorAttachments = { gfx::FramebufferColorAttachment{ .view = view } }, .extent = { 256, 256 } });

    auto renderPassEncoder = encoder->beginRenderPass({ .framebuffer = framebuffer });
    ASSERT_NE(renderPassEncoder, nullptr);

    EXPECT_NO_THROW(renderPassEncoder->setViewport({ 0, 0, 256, 256 }));
}

TEST_P(GfxCppRenderPassEncoderTest, SetScissorRectValid)
{
    auto encoder = device->createCommandEncoder({});
    ASSERT_NE(encoder, nullptr);

    gfx::RenderPassCreateDescriptor rpDesc{
        .colorAttachments = { gfx::RenderPassColorAttachment{ .target = { .format = gfx::TextureFormat::R8G8B8A8Unorm, .sampleCount = gfx::SampleCount::Count1, .ops = { gfx::LoadOp::Clear, gfx::StoreOp::Store }, .finalLayout = gfx::TextureLayout::ColorAttachment } } }
    };
    auto renderPass = device->createRenderPass(rpDesc);

    auto texture = device->createTexture({ .type = gfx::TextureType::Texture2D, .size = { 256, 256, 1 }, .format = gfx::TextureFormat::R8G8B8A8Unorm, .usage = gfx::TextureUsage::RenderAttachment });
    auto view = texture->createView({ .viewType = gfx::TextureViewType::View2D, .format = gfx::TextureFormat::R8G8B8A8Unorm });
    auto framebuffer = device->createFramebuffer({ .renderPass = renderPass, .colorAttachments = { gfx::FramebufferColorAttachment{ .view = view } }, .extent = { 256, 256 } });

    auto renderPassEncoder = encoder->beginRenderPass({ .framebuffer = framebuffer });
    ASSERT_NE(renderPassEncoder, nullptr);

    EXPECT_NO_THROW(renderPassEncoder->setScissorRect({ 0, 0, 256, 256 }));
}

TEST_P(GfxCppRenderPassEncoderTest, DrawIndirectWithNullBuffer)
{
    auto encoder = device->createCommandEncoder({});
    ASSERT_NE(encoder, nullptr);

    gfx::RenderPassCreateDescriptor rpDesc{
        .colorAttachments = { gfx::RenderPassColorAttachment{ .target = { .format = gfx::TextureFormat::R8G8B8A8Unorm, .sampleCount = gfx::SampleCount::Count1, .ops = { gfx::LoadOp::Clear, gfx::StoreOp::Store }, .finalLayout = gfx::TextureLayout::ColorAttachment } } }
    };
    auto renderPass = device->createRenderPass(rpDesc);

    auto texture = device->createTexture({ .type = gfx::TextureType::Texture2D, .size = { 256, 256, 1 }, .format = gfx::TextureFormat::R8G8B8A8Unorm, .usage = gfx::TextureUsage::RenderAttachment });
    auto view = texture->createView({ .viewType = gfx::TextureViewType::View2D, .format = gfx::TextureFormat::R8G8B8A8Unorm });
    auto framebuffer = device->createFramebuffer({ .renderPass = renderPass, .colorAttachments = { gfx::FramebufferColorAttachment{ .view = view } }, .extent = { 256, 256 } });

    auto renderPassEncoder = encoder->beginRenderPass({ .framebuffer = framebuffer });
    ASSERT_NE(renderPassEncoder, nullptr);

    // Null indirect buffer should throw
    EXPECT_THROW(renderPassEncoder->drawIndirect(nullptr, 0), std::invalid_argument);
}

TEST_P(GfxCppRenderPassEncoderTest, DrawIndexedIndirectWithNullBuffer)
{
    auto encoder = device->createCommandEncoder({});
    ASSERT_NE(encoder, nullptr);

    gfx::RenderPassCreateDescriptor rpDesc{
        .colorAttachments = { gfx::RenderPassColorAttachment{ .target = { .format = gfx::TextureFormat::R8G8B8A8Unorm, .sampleCount = gfx::SampleCount::Count1, .ops = { gfx::LoadOp::Clear, gfx::StoreOp::Store }, .finalLayout = gfx::TextureLayout::ColorAttachment } } }
    };
    auto renderPass = device->createRenderPass(rpDesc);

    auto texture = device->createTexture({ .type = gfx::TextureType::Texture2D, .size = { 256, 256, 1 }, .format = gfx::TextureFormat::R8G8B8A8Unorm, .usage = gfx::TextureUsage::RenderAttachment });
    auto view = texture->createView({ .viewType = gfx::TextureViewType::View2D, .format = gfx::TextureFormat::R8G8B8A8Unorm });
    auto framebuffer = device->createFramebuffer({ .renderPass = renderPass, .colorAttachments = { gfx::FramebufferColorAttachment{ .view = view } }, .extent = { 256, 256 } });

    auto renderPassEncoder = encoder->beginRenderPass({ .framebuffer = framebuffer });
    ASSERT_NE(renderPassEncoder, nullptr);

    // Null indirect buffer should throw
    EXPECT_THROW(renderPassEncoder->drawIndexedIndirect(nullptr, 0), std::invalid_argument);
}

TEST_P(GfxCppRenderPassEncoderTest, BeginRenderPassAndEnd)
{
    auto encoder = device->createCommandEncoder({});
    ASSERT_NE(encoder, nullptr);

    gfx::RenderPassCreateDescriptor rpDesc{
        .label = "Test",
        .colorAttachments = { gfx::RenderPassColorAttachment{ .target = { .format = gfx::TextureFormat::R8G8B8A8Unorm, .sampleCount = gfx::SampleCount::Count1, .ops = { gfx::LoadOp::Clear, gfx::StoreOp::Store }, .finalLayout = gfx::TextureLayout::ColorAttachment } } }
    };
    auto renderPass = device->createRenderPass(rpDesc);

    auto texture = device->createTexture({ .type = gfx::TextureType::Texture2D, .size = { 256, 256, 1 }, .format = gfx::TextureFormat::R8G8B8A8Unorm, .usage = gfx::TextureUsage::RenderAttachment });
    auto view = texture->createView({ .viewType = gfx::TextureViewType::View2D, .format = gfx::TextureFormat::R8G8B8A8Unorm });
    auto framebuffer = device->createFramebuffer({ .renderPass = renderPass, .colorAttachments = { gfx::FramebufferColorAttachment{ .view = view } }, .extent = { 256, 256 } });

    auto renderPassEncoder = encoder->beginRenderPass({ .framebuffer = framebuffer });
    ASSERT_NE(renderPassEncoder, nullptr);

    // Should be able to end without any operations
    EXPECT_NO_THROW(renderPassEncoder.reset());
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppRenderPassEncoderTest,
    testing::Values(gfx::Backend::Vulkan, gfx::Backend::WebGPU),
    [](const testing::TestParamInfo<gfx::Backend>& info) {
        return info.param == gfx::Backend::Vulkan ? "Vulkan" : "WebGPU";
    });

} // namespace
