#include <core/command/CommandEncoder.h>
#include <core/system/Device.h>

#include <gfx/gfx.h>

#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

namespace gfx {

class CommandEncoderImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc{
            .backend = backend,
            .applicationName = "CommandEncoderTest",
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

TEST_P(CommandEncoderImplTest, CreateCommandEncoder)
{
    DeviceImpl deviceWrapper(device);

    CommandEncoderDescriptor encoderDesc{
        .label = "Test Command Encoder"
    };

    auto encoder = deviceWrapper.createCommandEncoder(encoderDesc);
    EXPECT_NE(encoder, nullptr);
}

TEST_P(CommandEncoderImplTest, CreateMultipleCommandEncoders_IndependentHandles)
{
    DeviceImpl deviceWrapper(device);

    CommandEncoderDescriptor encoderDesc{
        .label = "Test Encoder"
    };

    auto encoder1 = deviceWrapper.createCommandEncoder(encoderDesc);
    auto encoder2 = deviceWrapper.createCommandEncoder(encoderDesc);

    EXPECT_NE(encoder1, nullptr);
    EXPECT_NE(encoder2, nullptr);
    EXPECT_NE(encoder1, encoder2);
}

TEST_P(CommandEncoderImplTest, BeginEndCommandEncoder)
{
    DeviceImpl deviceWrapper(device);

    CommandEncoderDescriptor encoderDesc{
        .label = "Begin/End Test Encoder"
    };

    auto encoder = deviceWrapper.createCommandEncoder(encoderDesc);
    ASSERT_NE(encoder, nullptr);

    // Begin recording
    encoder->begin();

    // End recording
    encoder->end();
}

TEST_P(CommandEncoderImplTest, CopyBufferToBuffer)
{
    DeviceImpl deviceWrapper(device);

    // Create source buffer
    BufferDescriptor srcBufferDesc{
        .label = "Source Buffer",
        .size = 256,
        .usage = BufferUsage::CopySrc,
        .memoryProperties = MemoryProperty::DeviceLocal
    };

    auto srcBuffer = deviceWrapper.createBuffer(srcBufferDesc);
    ASSERT_NE(srcBuffer, nullptr);

    // Create destination buffer
    BufferDescriptor dstBufferDesc{
        .label = "Destination Buffer",
        .size = 256,
        .usage = BufferUsage::CopyDst,
        .memoryProperties = MemoryProperty::DeviceLocal
    };

    auto dstBuffer = deviceWrapper.createBuffer(dstBufferDesc);
    ASSERT_NE(dstBuffer, nullptr);

    // Create command encoder
    CommandEncoderDescriptor encoderDesc{
        .label = "Copy Test Encoder"
    };

    auto encoder = deviceWrapper.createCommandEncoder(encoderDesc);
    ASSERT_NE(encoder, nullptr);

    // Begin recording
    encoder->begin();

    // Copy buffer
    CopyBufferToBufferDescriptor copyDesc{
        .source = srcBuffer,
        .sourceOffset = 0,
        .destination = dstBuffer,
        .destinationOffset = 0,
        .size = 256
    };

    encoder->copyBufferToBuffer(copyDesc);

    // End recording
    encoder->end();
}

TEST_P(CommandEncoderImplTest, BeginRenderPass)
{
    DeviceImpl deviceWrapper(device);

    // Create render pass
    RenderPassColorAttachmentTarget colorTarget{
        .format = TextureFormat::R8G8B8A8Unorm,
        .sampleCount = SampleCount::Count1,
        .ops = { LoadOp::Clear, StoreOp::Store },
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

    // Create texture for framebuffer
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
        .extent = { 800, 600 }
    };

    auto framebuffer = deviceWrapper.createFramebuffer(framebufferDesc);
    ASSERT_NE(framebuffer, nullptr);

    // Create command encoder
    CommandEncoderDescriptor encoderDesc{
        .label = "Render Pass Test Encoder"
    };

    auto encoder = deviceWrapper.createCommandEncoder(encoderDesc);
    ASSERT_NE(encoder, nullptr);

    // Begin recording
    encoder->begin();

    // Begin render pass
    {
        Color clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
        RenderPassBeginDescriptor rpBeginDesc{
            .framebuffer = framebuffer,
            .colorClearValues = { clearColor }
        };

        auto renderPassEncoder = encoder->beginRenderPass(rpBeginDesc);
        EXPECT_NE(renderPassEncoder, nullptr);
    } // Render pass encoder ends here (RAII)

    // End recording
    encoder->end();
}

TEST_P(CommandEncoderImplTest, BeginComputePass)
{
    DeviceImpl deviceWrapper(device);

    // Create command encoder
    CommandEncoderDescriptor encoderDesc{
        .label = "Compute Pass Test Encoder"
    };

    auto encoder = deviceWrapper.createCommandEncoder(encoderDesc);
    ASSERT_NE(encoder, nullptr);

    // Begin recording
    encoder->begin();

    // Begin compute pass
    {
        ComputePassBeginDescriptor cpBeginDesc{
            .label = "Test Compute Pass"
        };

        auto computePassEncoder = encoder->beginComputePass(cpBeginDesc);
        EXPECT_NE(computePassEncoder, nullptr);
    } // Compute pass encoder ends here (RAII)

    // End recording
    encoder->end();
}

// ============================================================================
// Null/Error Handling Tests - Skipped (test C API, not C++ implementation)
// ============================================================================

#if defined(GFX_ENABLE_VULKAN) && defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(AllBackends, CommandEncoderImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU));
#elif defined(GFX_ENABLE_VULKAN)
INSTANTIATE_TEST_SUITE_P(VulkanBackend, CommandEncoderImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN));
#elif defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(WebGPUBackend, CommandEncoderImplTest,
    ::testing::Values(GFX_BACKEND_WEBGPU));
#endif

} // namespace gfx
