#include <core/render/RenderPass.h>
#include <core/system/Device.h>

#include <gfx/gfx.h>

#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

namespace gfx {

class RenderPassImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc = {};
        instanceDesc.backend = backend;
        instanceDesc.applicationName = "RenderPassTest";
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

TEST_P(RenderPassImplTest, CreateRenderPassWithColorAttachment)
{
    DeviceImpl deviceWrapper(device);

    RenderPassCreateDescriptor renderPassDesc;
    renderPassDesc.label = "Test Render Pass";
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
    EXPECT_NE(renderPass, nullptr);
}

TEST_P(RenderPassImplTest, CreateRenderPassWithMultipleColorAttachments)
{
    DeviceImpl deviceWrapper(device);

    RenderPassCreateDescriptor renderPassDesc;
    renderPassDesc.label = "Multi-Attachment Render Pass";
    renderPassDesc.colorAttachments = {
        RenderPassColorAttachment{
            .target = {
                .format = TextureFormat::R8G8B8A8Unorm,
                .sampleCount = SampleCount::Count1,
                .loadOp = LoadOp::Clear,
                .storeOp = StoreOp::Store,
                .finalLayout = TextureLayout::ColorAttachment } },
        RenderPassColorAttachment{ .target = { .format = TextureFormat::R16G16B16A16Float, .sampleCount = SampleCount::Count1, .loadOp = LoadOp::Load, .storeOp = StoreOp::Store, .finalLayout = TextureLayout::ColorAttachment } }
    };

    auto renderPass = deviceWrapper.createRenderPass(renderPassDesc);
    EXPECT_NE(renderPass, nullptr);
}

TEST_P(RenderPassImplTest, CreateRenderPassWithDepthStencilAttachment)
{
    DeviceImpl deviceWrapper(device);

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
    renderPassDesc.label = "Depth-Stencil Render Pass";
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
    EXPECT_NE(renderPass, nullptr);
}

TEST_P(RenderPassImplTest, CreateMultipleRenderPasses_IndependentHandles)
{
    DeviceImpl deviceWrapper(device);

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

    auto renderPass1 = deviceWrapper.createRenderPass(renderPassDesc);
    auto renderPass2 = deviceWrapper.createRenderPass(renderPassDesc);

    EXPECT_NE(renderPass1, nullptr);
    EXPECT_NE(renderPass2, nullptr);
    EXPECT_NE(renderPass1, renderPass2);
}

TEST_P(RenderPassImplTest, CreateRenderPassWithMSAAAndResolve)
{
    DeviceImpl deviceWrapper(device);

    RenderPassColorAttachmentTarget resolveTarget;
    resolveTarget.format = TextureFormat::R8G8B8A8Unorm;
    resolveTarget.sampleCount = SampleCount::Count1;
    resolveTarget.loadOp = LoadOp::DontCare;
    resolveTarget.storeOp = StoreOp::Store;
    resolveTarget.finalLayout = TextureLayout::ColorAttachment;

    RenderPassCreateDescriptor renderPassDesc;
    renderPassDesc.label = "MSAA Render Pass with Resolve";
    renderPassDesc.colorAttachments = {
        RenderPassColorAttachment{
            .target = {
                .format = TextureFormat::R8G8B8A8Unorm,
                .sampleCount = SampleCount::Count4,
                .loadOp = LoadOp::Clear,
                .storeOp = StoreOp::Store,
                .finalLayout = TextureLayout::ColorAttachment },
            .resolveTarget = resolveTarget }
    };

    auto renderPass = deviceWrapper.createRenderPass(renderPassDesc);
    EXPECT_NE(renderPass, nullptr);
}

#if defined(GFX_ENABLE_VULKAN) && defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(AllBackends, RenderPassImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU));
#elif defined(GFX_ENABLE_VULKAN)
INSTANTIATE_TEST_SUITE_P(VulkanBackend, RenderPassImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN));
#elif defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(WebGPUBackend, RenderPassImplTest,
    ::testing::Values(GFX_BACKEND_WEBGPU));
#endif

} // namespace gfx
