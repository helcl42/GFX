#include "../../common/CommonTest.h"

#include <core/render/Framebuffer.h>
#include <core/system/Device.h>

namespace gfx {

class FramebufferImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc{
            .backend = backend,
            .applicationName = "FramebufferTest",
            .applicationVersion = 1
        };
        ASSERT_EQ(gfxCreateInstance(&instanceDesc, &instance), GFX_RESULT_SUCCESS);

        GfxAdapterDescriptor adapterDesc{
            .sType = GFX_STRUCTURE_TYPE_ADAPTER_DESCRIPTOR,
            .pNext = nullptr,
            .adapterIndex = 0
        };
        ASSERT_EQ(gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter), GFX_RESULT_SUCCESS);

        GfxDeviceDescriptor deviceDesc{
            .sType = GFX_STRUCTURE_TYPE_DEVICE_DESCRIPTOR,
            .pNext = nullptr
        };
        ASSERT_EQ(gfxAdapterCreateDevice(adapter, &deviceDesc, &device), GFX_RESULT_SUCCESS);
    }

    void TearDown() override
    {
        if (device) {
            gfxDeviceDestroy(device);
        }
        if (instance) {
            gfxInstanceDestroy(instance);
        }
        gfxUnloadBackend(backend);
    }

    GfxBackend backend;
    GfxInstance instance = nullptr;
    GfxAdapter adapter = nullptr;
    GfxDevice device = nullptr;
};

TEST_P(FramebufferImplTest, CreateFramebufferWithColorAttachment)
{
    DeviceImpl deviceWrapper(device);

    // Create render pass
    RenderPassCreateDescriptor renderPassDesc{
        .colorAttachments = {
            RenderPassColorAttachment{
                .target = {
                    .format = TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = SampleCount::Count1,
                    .ops = { LoadOp::Clear, StoreOp::Store },
                    .finalLayout = TextureLayout::ColorAttachment } } }
    };

    auto renderPass = deviceWrapper.createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create texture for color attachment
    TextureDescriptor textureDesc{
        .label = "Color Attachment Texture",
        .type = TextureType::Texture2D,
        .size = { 800, 600, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = SampleCount::Count1,
        .format = TextureFormat::R8G8B8A8Unorm,
        .usage = TextureUsage::RenderAttachment
    };

    auto texture = deviceWrapper.createTexture(textureDesc);
    ASSERT_NE(texture, nullptr);

    // Create texture view
    TextureViewDescriptor viewDesc{
        .label = "Color Attachment View",
        .viewType = TextureViewType::View2D,
        .format = TextureFormat::R8G8B8A8Unorm,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1
    };

    auto textureView = texture->createView(viewDesc);
    ASSERT_NE(textureView, nullptr);

    // Create framebuffer
    FramebufferDescriptor framebufferDesc{
        .label = "Test Framebuffer",
        .renderPass = renderPass,
        .colorAttachments = {
            FramebufferColorAttachment{
                .view = textureView } },
        .extent = { 800, 600 }
    };

    auto framebuffer = deviceWrapper.createFramebuffer(framebufferDesc);
    EXPECT_NE(framebuffer, nullptr);
}

TEST_P(FramebufferImplTest, CreateFramebufferWithMultipleColorAttachments)
{
    DeviceImpl deviceWrapper(device);

    // Create render pass with multiple color attachments
    RenderPassCreateDescriptor renderPassDesc{
        .colorAttachments = {
            RenderPassColorAttachment{
                .target = {
                    .format = TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = SampleCount::Count1,
                    .ops = { LoadOp::Clear, StoreOp::Store },
                    .finalLayout = TextureLayout::ColorAttachment } },
            RenderPassColorAttachment{ .target = { .format = TextureFormat::R16G16B16A16Float, .sampleCount = SampleCount::Count1, .ops = { LoadOp::Clear, StoreOp::Store }, .finalLayout = TextureLayout::ColorAttachment } } }
    };

    auto renderPass = deviceWrapper.createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create first texture
    TextureDescriptor textureDesc1{
        .type = TextureType::Texture2D,
        .size = { 800, 600, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = SampleCount::Count1,
        .format = TextureFormat::R8G8B8A8Unorm,
        .usage = TextureUsage::RenderAttachment
    };

    auto texture1 = deviceWrapper.createTexture(textureDesc1);
    ASSERT_NE(texture1, nullptr);

    // Create second texture
    TextureDescriptor textureDesc2{
        .type = TextureType::Texture2D,
        .size = { 800, 600, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = SampleCount::Count1,
        .format = TextureFormat::R16G16B16A16Float,
        .usage = TextureUsage::RenderAttachment
    };

    auto texture2 = deviceWrapper.createTexture(textureDesc2);
    ASSERT_NE(texture2, nullptr);

    // Create texture views
    TextureViewDescriptor viewDesc1{
        .viewType = TextureViewType::View2D,
        .format = TextureFormat::R8G8B8A8Unorm,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1
    };

    auto textureView1 = texture1->createView(viewDesc1);
    ASSERT_NE(textureView1, nullptr);

    TextureViewDescriptor viewDesc2{
        .viewType = TextureViewType::View2D,
        .format = TextureFormat::R16G16B16A16Float,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1
    };

    auto textureView2 = texture2->createView(viewDesc2);
    ASSERT_NE(textureView2, nullptr);

    // Create framebuffer
    FramebufferDescriptor framebufferDesc{
        .renderPass = renderPass,
        .colorAttachments = {
            FramebufferColorAttachment{ .view = textureView1 },
            FramebufferColorAttachment{ .view = textureView2 } },
        .extent = { 800, 600 }
    };

    auto framebuffer = deviceWrapper.createFramebuffer(framebufferDesc);
    EXPECT_NE(framebuffer, nullptr);
}

TEST_P(FramebufferImplTest, CreateFramebufferWithDepthStencilAttachment)
{
    DeviceImpl deviceWrapper(device);

    // Create render pass with depth-stencil attachment
    RenderPassDepthStencilAttachmentTarget depthStencilTarget{
        .format = TextureFormat::Depth24PlusStencil8,
        .sampleCount = SampleCount::Count1,
        .depthOps = { LoadOp::Clear, StoreOp::Store },
        .stencilOps = { LoadOp::Clear, StoreOp::Store },
        .finalLayout = TextureLayout::DepthStencilAttachment
    };

    RenderPassDepthStencilAttachment depthStencilAttachment{
        .target = depthStencilTarget,
        .resolveTarget = std::nullopt
    };

    RenderPassCreateDescriptor renderPassDesc{
        .colorAttachments = {
            RenderPassColorAttachment{
                .target = {
                    .format = TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = SampleCount::Count1,
                    .ops = { LoadOp::Clear, StoreOp::Store },
                    .finalLayout = TextureLayout::ColorAttachment } } },
        .depthStencilAttachment = depthStencilAttachment
    };

    auto renderPass = deviceWrapper.createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create color texture
    TextureDescriptor colorTextureDesc{
        .type = TextureType::Texture2D,
        .size = { 800, 600, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = SampleCount::Count1,
        .format = TextureFormat::R8G8B8A8Unorm,
        .usage = TextureUsage::RenderAttachment
    };

    auto colorTexture = deviceWrapper.createTexture(colorTextureDesc);
    ASSERT_NE(colorTexture, nullptr);

    // Create depth-stencil texture
    TextureDescriptor depthTextureDesc{
        .type = TextureType::Texture2D,
        .size = { 800, 600, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = SampleCount::Count1,
        .format = TextureFormat::Depth24PlusStencil8,
        .usage = TextureUsage::RenderAttachment
    };

    auto depthTexture = deviceWrapper.createTexture(depthTextureDesc);
    ASSERT_NE(depthTexture, nullptr);

    // Create texture views
    TextureViewDescriptor colorViewDesc{
        .viewType = TextureViewType::View2D,
        .format = TextureFormat::R8G8B8A8Unorm,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1
    };

    auto colorView = colorTexture->createView(colorViewDesc);
    ASSERT_NE(colorView, nullptr);

    TextureViewDescriptor depthViewDesc{
        .viewType = TextureViewType::View2D,
        .format = TextureFormat::Depth24PlusStencil8,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1
    };

    auto depthView = depthTexture->createView(depthViewDesc);
    ASSERT_NE(depthView, nullptr);

    // Create framebuffer
    FramebufferDepthStencilAttachment fbDepthStencilAttachment{
        .view = depthView
    };

    FramebufferDescriptor framebufferDesc{
        .renderPass = renderPass,
        .colorAttachments = {
            FramebufferColorAttachment{ .view = colorView } },
        .depthStencilAttachment = fbDepthStencilAttachment,
        .extent = { 800, 600 }
    };

    auto framebuffer = deviceWrapper.createFramebuffer(framebufferDesc);
    EXPECT_NE(framebuffer, nullptr);
}

TEST_P(FramebufferImplTest, CreateMultipleFramebuffers_IndependentHandles)
{
    DeviceImpl deviceWrapper(device);

    // Create render pass
    RenderPassCreateDescriptor renderPassDesc{
        .colorAttachments = {
            RenderPassColorAttachment{
                .target = {
                    .format = TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = SampleCount::Count1,
                    .ops = { LoadOp::Clear, StoreOp::Store },
                    .finalLayout = TextureLayout::ColorAttachment } } }
    };

    auto renderPass = deviceWrapper.createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create textures and views
    TextureDescriptor textureDesc{
        .type = TextureType::Texture2D,
        .size = { 800, 600, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = SampleCount::Count1,
        .format = TextureFormat::R8G8B8A8Unorm,
        .usage = TextureUsage::RenderAttachment
    };

    auto texture1 = deviceWrapper.createTexture(textureDesc);
    auto texture2 = deviceWrapper.createTexture(textureDesc);
    ASSERT_NE(texture1, nullptr);
    ASSERT_NE(texture2, nullptr);

    TextureViewDescriptor viewDesc{
        .viewType = TextureViewType::View2D,
        .format = TextureFormat::R8G8B8A8Unorm,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1
    };

    auto textureView1 = texture1->createView(viewDesc);
    auto textureView2 = texture2->createView(viewDesc);
    ASSERT_NE(textureView1, nullptr);
    ASSERT_NE(textureView2, nullptr);

    // Create two framebuffers
    FramebufferDescriptor framebufferDesc1{
        .renderPass = renderPass,
        .colorAttachments = {
            FramebufferColorAttachment{ .view = textureView1 } },
        .extent = { 800, 600 }
    };

    FramebufferDescriptor framebufferDesc2{
        .renderPass = renderPass,
        .colorAttachments = {
            FramebufferColorAttachment{ .view = textureView2 } },
        .extent = { 800, 600 }
    };

    auto framebuffer1 = deviceWrapper.createFramebuffer(framebufferDesc1);
    auto framebuffer2 = deviceWrapper.createFramebuffer(framebufferDesc2);

    EXPECT_NE(framebuffer1, nullptr);
    EXPECT_NE(framebuffer2, nullptr);
    EXPECT_NE(framebuffer1, framebuffer2);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    FramebufferImplTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

} // namespace gfx
