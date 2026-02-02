#include <core/render/Framebuffer.h>
#include <core/system/Device.h>

#include <gfx/gfx.h>

#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

namespace gfx {

class FramebufferImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc = {};
        instanceDesc.backend = backend;
        instanceDesc.applicationName = "FramebufferTest";
        instanceDesc.applicationVersion = 1;
        ASSERT_EQ(gfxCreateInstance(&instanceDesc, &instance), GFX_RESULT_SUCCESS);

        GfxAdapterDescriptor adapterDesc = {};
        adapterDesc.adapterIndex = 0;
        ASSERT_EQ(gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter), GFX_RESULT_SUCCESS);

        GfxDeviceDescriptor deviceDesc = {};
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
    RenderPassCreateDescriptor renderPassDesc;
    renderPassDesc.colorAttachments = {
        RenderPassColorAttachment{
            .target = {
                .format = TextureFormat::R8G8B8A8Unorm,
                .sampleCount = SampleCount::Count1,
                .loadOp = LoadOp::Clear,
                .storeOp = StoreOp::Store,
                .finalLayout = TextureLayout::ColorAttachment } }
    };

    auto renderPass = deviceWrapper.createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create texture for color attachment
    TextureDescriptor textureDesc;
    textureDesc.label = "Color Attachment Texture";
    textureDesc.type = TextureType::Texture2D;
    textureDesc.size = { 800, 600, 1 };
    textureDesc.arrayLayerCount = 1;
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = SampleCount::Count1;
    textureDesc.format = TextureFormat::R8G8B8A8Unorm;
    textureDesc.usage = TextureUsage::RenderAttachment;

    auto texture = deviceWrapper.createTexture(textureDesc);
    ASSERT_NE(texture, nullptr);

    // Create texture view
    TextureViewDescriptor viewDesc;
    viewDesc.label = "Color Attachment View";
    viewDesc.format = TextureFormat::R8G8B8A8Unorm;
    viewDesc.viewType = TextureViewType::View2D;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    auto textureView = texture->createView(viewDesc);
    ASSERT_NE(textureView, nullptr);

    // Create framebuffer
    FramebufferDescriptor framebufferDesc;
    framebufferDesc.label = "Test Framebuffer";
    framebufferDesc.renderPass = renderPass;
    framebufferDesc.colorAttachments = {
        FramebufferColorAttachment{
            .view = textureView }
    };
    framebufferDesc.width = 800;
    framebufferDesc.height = 600;

    auto framebuffer = deviceWrapper.createFramebuffer(framebufferDesc);
    EXPECT_NE(framebuffer, nullptr);
}

TEST_P(FramebufferImplTest, CreateFramebufferWithMultipleColorAttachments)
{
    DeviceImpl deviceWrapper(device);

    // Create render pass with multiple color attachments
    RenderPassCreateDescriptor renderPassDesc;
    renderPassDesc.colorAttachments = {
        RenderPassColorAttachment{
            .target = {
                .format = TextureFormat::R8G8B8A8Unorm,
                .sampleCount = SampleCount::Count1,
                .loadOp = LoadOp::Clear,
                .storeOp = StoreOp::Store,
                .finalLayout = TextureLayout::ColorAttachment } },
        RenderPassColorAttachment{ .target = { .format = TextureFormat::R16G16B16A16Float, .sampleCount = SampleCount::Count1, .loadOp = LoadOp::Clear, .storeOp = StoreOp::Store, .finalLayout = TextureLayout::ColorAttachment } }
    };

    auto renderPass = deviceWrapper.createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create first texture
    TextureDescriptor textureDesc1;
    textureDesc1.type = TextureType::Texture2D;
    textureDesc1.size = { 800, 600, 1 };
    textureDesc1.arrayLayerCount = 1;
    textureDesc1.mipLevelCount = 1;
    textureDesc1.sampleCount = SampleCount::Count1;
    textureDesc1.format = TextureFormat::R8G8B8A8Unorm;
    textureDesc1.usage = TextureUsage::RenderAttachment;

    auto texture1 = deviceWrapper.createTexture(textureDesc1);
    ASSERT_NE(texture1, nullptr);

    // Create second texture
    TextureDescriptor textureDesc2;
    textureDesc2.type = TextureType::Texture2D;
    textureDesc2.size = { 800, 600, 1 };
    textureDesc2.arrayLayerCount = 1;
    textureDesc2.mipLevelCount = 1;
    textureDesc2.sampleCount = SampleCount::Count1;
    textureDesc2.format = TextureFormat::R16G16B16A16Float;
    textureDesc2.usage = TextureUsage::RenderAttachment;

    auto texture2 = deviceWrapper.createTexture(textureDesc2);
    ASSERT_NE(texture2, nullptr);

    // Create texture views
    TextureViewDescriptor viewDesc1;
    viewDesc1.format = TextureFormat::R8G8B8A8Unorm;
    viewDesc1.viewType = TextureViewType::View2D;
    viewDesc1.baseMipLevel = 0;
    viewDesc1.mipLevelCount = 1;
    viewDesc1.baseArrayLayer = 0;
    viewDesc1.arrayLayerCount = 1;

    auto textureView1 = texture1->createView(viewDesc1);
    ASSERT_NE(textureView1, nullptr);

    TextureViewDescriptor viewDesc2;
    viewDesc2.format = TextureFormat::R16G16B16A16Float;
    viewDesc2.viewType = TextureViewType::View2D;
    viewDesc2.baseMipLevel = 0;
    viewDesc2.mipLevelCount = 1;
    viewDesc2.baseArrayLayer = 0;
    viewDesc2.arrayLayerCount = 1;

    auto textureView2 = texture2->createView(viewDesc2);
    ASSERT_NE(textureView2, nullptr);

    // Create framebuffer
    FramebufferDescriptor framebufferDesc;
    framebufferDesc.renderPass = renderPass;
    framebufferDesc.colorAttachments = {
        FramebufferColorAttachment{ .view = textureView1 },
        FramebufferColorAttachment{ .view = textureView2 }
    };
    framebufferDesc.width = 800;
    framebufferDesc.height = 600;

    auto framebuffer = deviceWrapper.createFramebuffer(framebufferDesc);
    EXPECT_NE(framebuffer, nullptr);
}

TEST_P(FramebufferImplTest, CreateFramebufferWithDepthStencilAttachment)
{
    DeviceImpl deviceWrapper(device);

    // Create render pass with depth-stencil attachment
    RenderPassDepthStencilAttachmentTarget depthStencilTarget;
    depthStencilTarget.format = TextureFormat::Depth24PlusStencil8;
    depthStencilTarget.sampleCount = SampleCount::Count1;
    depthStencilTarget.depthLoadOp = LoadOp::Clear;
    depthStencilTarget.depthStoreOp = StoreOp::Store;
    depthStencilTarget.stencilLoadOp = LoadOp::Clear;
    depthStencilTarget.stencilStoreOp = StoreOp::Store;
    depthStencilTarget.finalLayout = TextureLayout::DepthStencilAttachment;

    RenderPassDepthStencilAttachment depthStencilAttachment;
    depthStencilAttachment.target = depthStencilTarget;
    depthStencilAttachment.resolveTarget = std::nullopt;

    RenderPassCreateDescriptor renderPassDesc;
    renderPassDesc.colorAttachments = {
        RenderPassColorAttachment{
            .target = {
                .format = TextureFormat::R8G8B8A8Unorm,
                .sampleCount = SampleCount::Count1,
                .loadOp = LoadOp::Clear,
                .storeOp = StoreOp::Store,
                .finalLayout = TextureLayout::ColorAttachment } }
    };
    renderPassDesc.depthStencilAttachment = depthStencilAttachment;

    auto renderPass = deviceWrapper.createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create color texture
    TextureDescriptor colorTextureDesc;
    colorTextureDesc.type = TextureType::Texture2D;
    colorTextureDesc.size = { 800, 600, 1 };
    colorTextureDesc.arrayLayerCount = 1;
    colorTextureDesc.mipLevelCount = 1;
    colorTextureDesc.sampleCount = SampleCount::Count1;
    colorTextureDesc.format = TextureFormat::R8G8B8A8Unorm;
    colorTextureDesc.usage = TextureUsage::RenderAttachment;

    auto colorTexture = deviceWrapper.createTexture(colorTextureDesc);
    ASSERT_NE(colorTexture, nullptr);

    // Create depth-stencil texture
    TextureDescriptor depthTextureDesc;
    depthTextureDesc.type = TextureType::Texture2D;
    depthTextureDesc.size = { 800, 600, 1 };
    depthTextureDesc.arrayLayerCount = 1;
    depthTextureDesc.mipLevelCount = 1;
    depthTextureDesc.sampleCount = SampleCount::Count1;
    depthTextureDesc.format = TextureFormat::Depth24PlusStencil8;
    depthTextureDesc.usage = TextureUsage::RenderAttachment;

    auto depthTexture = deviceWrapper.createTexture(depthTextureDesc);
    ASSERT_NE(depthTexture, nullptr);

    // Create texture views
    TextureViewDescriptor colorViewDesc;
    colorViewDesc.format = TextureFormat::R8G8B8A8Unorm;
    colorViewDesc.viewType = TextureViewType::View2D;
    colorViewDesc.baseMipLevel = 0;
    colorViewDesc.mipLevelCount = 1;
    colorViewDesc.baseArrayLayer = 0;
    colorViewDesc.arrayLayerCount = 1;

    auto colorView = colorTexture->createView(colorViewDesc);
    ASSERT_NE(colorView, nullptr);

    TextureViewDescriptor depthViewDesc;
    depthViewDesc.format = TextureFormat::Depth24PlusStencil8;
    depthViewDesc.viewType = TextureViewType::View2D;
    depthViewDesc.baseMipLevel = 0;
    depthViewDesc.mipLevelCount = 1;
    depthViewDesc.baseArrayLayer = 0;
    depthViewDesc.arrayLayerCount = 1;

    auto depthView = depthTexture->createView(depthViewDesc);
    ASSERT_NE(depthView, nullptr);

    // Create framebuffer
    FramebufferDepthStencilAttachment fbDepthStencilAttachment{
        .view = depthView
    };

    FramebufferDescriptor framebufferDesc;
    framebufferDesc.renderPass = renderPass;
    framebufferDesc.colorAttachments = {
        FramebufferColorAttachment{ .view = colorView }
    };
    framebufferDesc.depthStencilAttachment = fbDepthStencilAttachment;
    framebufferDesc.width = 800;
    framebufferDesc.height = 600;

    auto framebuffer = deviceWrapper.createFramebuffer(framebufferDesc);
    EXPECT_NE(framebuffer, nullptr);
}

TEST_P(FramebufferImplTest, CreateMultipleFramebuffers_IndependentHandles)
{
    DeviceImpl deviceWrapper(device);

    // Create render pass
    RenderPassCreateDescriptor renderPassDesc;
    renderPassDesc.colorAttachments = {
        RenderPassColorAttachment{
            .target = {
                .format = TextureFormat::R8G8B8A8Unorm,
                .sampleCount = SampleCount::Count1,
                .loadOp = LoadOp::Clear,
                .storeOp = StoreOp::Store,
                .finalLayout = TextureLayout::ColorAttachment } }
    };

    auto renderPass = deviceWrapper.createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create textures and views
    TextureDescriptor textureDesc;
    textureDesc.type = TextureType::Texture2D;
    textureDesc.size = { 800, 600, 1 };
    textureDesc.arrayLayerCount = 1;
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = SampleCount::Count1;
    textureDesc.format = TextureFormat::R8G8B8A8Unorm;
    textureDesc.usage = TextureUsage::RenderAttachment;

    auto texture1 = deviceWrapper.createTexture(textureDesc);
    auto texture2 = deviceWrapper.createTexture(textureDesc);
    ASSERT_NE(texture1, nullptr);
    ASSERT_NE(texture2, nullptr);

    TextureViewDescriptor viewDesc;
    viewDesc.format = TextureFormat::R8G8B8A8Unorm;
    viewDesc.viewType = TextureViewType::View2D;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    auto textureView1 = texture1->createView(viewDesc);
    auto textureView2 = texture2->createView(viewDesc);
    ASSERT_NE(textureView1, nullptr);
    ASSERT_NE(textureView2, nullptr);

    // Create two framebuffers
    FramebufferDescriptor framebufferDesc1;
    framebufferDesc1.renderPass = renderPass;
    framebufferDesc1.colorAttachments = {
        FramebufferColorAttachment{ .view = textureView1 }
    };
    framebufferDesc1.width = 800;
    framebufferDesc1.height = 600;

    FramebufferDescriptor framebufferDesc2;
    framebufferDesc2.renderPass = renderPass;
    framebufferDesc2.colorAttachments = {
        FramebufferColorAttachment{ .view = textureView2 }
    };
    framebufferDesc2.width = 800;
    framebufferDesc2.height = 600;

    auto framebuffer1 = deviceWrapper.createFramebuffer(framebufferDesc1);
    auto framebuffer2 = deviceWrapper.createFramebuffer(framebufferDesc2);

    EXPECT_NE(framebuffer1, nullptr);
    EXPECT_NE(framebuffer2, nullptr);
    EXPECT_NE(framebuffer1, framebuffer2);
}

#if defined(GFX_ENABLE_VULKAN) && defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(AllBackends, FramebufferImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU));
#elif defined(GFX_ENABLE_VULKAN)
INSTANTIATE_TEST_SUITE_P(VulkanBackend, FramebufferImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN));
#elif defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(WebGPUBackend, FramebufferImplTest,
    ::testing::Values(GFX_BACKEND_WEBGPU));
#endif

} // namespace gfx
