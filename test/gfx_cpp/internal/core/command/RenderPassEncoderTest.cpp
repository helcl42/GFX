#include <core/command/CommandEncoder.h>
#include <core/command/RenderPassEncoder.h>
#include <core/system/Device.h>

#include <gfx/gfx.h>

#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

namespace gfx {

class RenderPassEncoderImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc{
            .backend = backend,
            .applicationName = "RenderPassEncoderTest",
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

TEST_P(RenderPassEncoderImplTest, SetViewportAndScissor)
{
    DeviceImpl deviceWrapper(device);

    // Create render pass
    RenderPassColorAttachmentTarget colorTarget{
        .format = TextureFormat::R8G8B8A8Unorm,
        .sampleCount = SampleCount::Count1,
        .loadOp = LoadOp::Clear,
        .storeOp = StoreOp::Store,
        .finalLayout = TextureLayout::ColorAttachment
    };

    RenderPassColorAttachment colorAttachment{
        .target = colorTarget,
        .resolveTarget = std::nullopt
    };

    RenderPassCreateDescriptor renderPassDesc{
        .colorAttachments = { colorAttachment }
    };

    auto renderPass = deviceWrapper.createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create texture
    TextureDescriptor textureDesc{
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
        .renderPass = renderPass,
        .colorAttachments = { FramebufferColorAttachment{ .view = textureView } },
        .width = 800,
        .height = 600
    };

    auto framebuffer = deviceWrapper.createFramebuffer(framebufferDesc);
    ASSERT_NE(framebuffer, nullptr);

    // Create command encoder
    CommandEncoderDescriptor encoderDesc;
    auto encoder = deviceWrapper.createCommandEncoder(encoderDesc);
    ASSERT_NE(encoder, nullptr);
    encoder->begin();

    // Begin render pass
    {
        Color clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
        RenderPassBeginDescriptor rpBeginDesc{
            .framebuffer = framebuffer,
            .colorClearValues = { clearColor }
        };

        auto renderPassEncoder = encoder->beginRenderPass(rpBeginDesc);
        ASSERT_NE(renderPassEncoder, nullptr);

        // Set viewport
        renderPassEncoder->setViewport(0.0f, 0.0f, 800.0f, 600.0f, 0.0f, 1.0f);

        // Set scissor rect
        renderPassEncoder->setScissorRect(0, 0, 800, 600);
    } // Render pass encoder ends here (RAII)

    // End command encoder
    encoder->end();
}

// ============================================================================
// Null/Error Handling Tests - Skipped (test C API, not C++ implementation)
// ============================================================================

#if defined(GFX_ENABLE_VULKAN) && defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(AllBackends, RenderPassEncoderImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU));
#elif defined(GFX_ENABLE_VULKAN)
INSTANTIATE_TEST_SUITE_P(VulkanBackend, RenderPassEncoderImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN));
#elif defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(WebGPUBackend, RenderPassEncoderImplTest,
    ::testing::Values(GFX_BACKEND_WEBGPU));
#endif

} // namespace gfx
