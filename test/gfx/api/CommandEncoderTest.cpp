#include <gfx/gfx.h>

#include <gtest/gtest.h>

#include <cstring>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxCommandEncoderTest : public testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        if (gfxLoadBackend(backend) != GFX_RESULT_SUCCESS) {
            GTEST_SKIP() << "Backend not available";
        }

        const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
        GfxInstanceDescriptor instDesc = {};
        instDesc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
        instDesc.pNext = nullptr;
        instDesc.backend = backend;
        instDesc.enabledExtensions = extensions;
        instDesc.enabledExtensionCount = 1;

        if (gfxCreateInstance(&instDesc, &instance) != GFX_RESULT_SUCCESS) {
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to create instance";
        }

        GfxAdapterDescriptor adapterDesc = {};
        adapterDesc.sType = GFX_STRUCTURE_TYPE_ADAPTER_DESCRIPTOR;
        adapterDesc.pNext = nullptr;
        adapterDesc.adapterIndex = 0;

        if (gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter) != GFX_RESULT_SUCCESS) {
            gfxInstanceDestroy(instance);
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to request adapter";
        }

        GfxDeviceDescriptor deviceDesc = {};
        deviceDesc.sType = GFX_STRUCTURE_TYPE_DEVICE_DESCRIPTOR;
        deviceDesc.pNext = nullptr;
        deviceDesc.label = "Test Device";

        if (gfxAdapterCreateDevice(adapter, &deviceDesc, &device) != GFX_RESULT_SUCCESS) {
            gfxInstanceDestroy(instance);
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to create device";
        }
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

    GfxBackend backend = GFX_BACKEND_VULKAN;
    GfxInstance instance = nullptr;
    GfxAdapter adapter = nullptr;
    GfxDevice device = nullptr;
};

// NULL parameter validation tests
TEST_P(GfxCommandEncoderTest, CreateCommandEncoderWithNullDevice)
{
    GfxCommandEncoder encoder = nullptr;
    GfxCommandEncoderDescriptor desc = {};
    desc.label = "test_encoder";

    GfxResult result = gfxDeviceCreateCommandEncoder(nullptr, &desc, &encoder);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxCommandEncoderTest, CreateCommandEncoderWithNullDescriptor)
{
    GfxCommandEncoder encoder = nullptr;

    GfxResult result = gfxDeviceCreateCommandEncoder(device, nullptr, &encoder);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxCommandEncoderTest, CreateCommandEncoderWithNullOutput)
{
    GfxCommandEncoderDescriptor desc = {};
    desc.label = "test_encoder";

    GfxResult result = gfxDeviceCreateCommandEncoder(device, &desc, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Basic functionality tests
TEST_P(GfxCommandEncoderTest, CreateCommandEncoder)
{
    GfxCommandEncoder encoder = nullptr;
    GfxCommandEncoderDescriptor desc = {};
    desc.label = "test_encoder";

    GfxResult result = gfxDeviceCreateCommandEncoder(device, &desc, &encoder);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(encoder, nullptr);

    gfxCommandEncoderDestroy(encoder);
}

TEST_P(GfxCommandEncoderTest, CreateCommandEncoderWithoutLabel)
{
    GfxCommandEncoder encoder = nullptr;
    GfxCommandEncoderDescriptor desc = {};
    desc.label = nullptr;

    GfxResult result = gfxDeviceCreateCommandEncoder(device, &desc, &encoder);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(encoder, nullptr);

    gfxCommandEncoderDestroy(encoder);
}

TEST_P(GfxCommandEncoderTest, DestroyNullCommandEncoder)
{
    GfxResult result = gfxCommandEncoderDestroy(nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxCommandEncoderTest, DestroyCommandEncoder)
{
    GfxCommandEncoder encoder = nullptr;
    GfxCommandEncoderDescriptor desc = {};
    desc.label = "test_encoder";

    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &desc, &encoder), GFX_RESULT_SUCCESS);

    GfxResult result = gfxCommandEncoderDestroy(encoder);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
}

// Copy operations tests
TEST_P(GfxCommandEncoderTest, CopyBufferToBufferWithNullEncoder)
{
    GfxCopyBufferToBufferDescriptor copyDesc = {};

    GfxResult result = gfxCommandEncoderCopyBufferToBuffer(nullptr, &copyDesc);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxCommandEncoderTest, CopyBufferToBufferWithNullDescriptor)
{
    GfxCommandEncoder encoder = nullptr;
    GfxCommandEncoderDescriptor desc = {};
    desc.label = "test_encoder";
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &desc, &encoder), GFX_RESULT_SUCCESS);

    GfxResult result = gfxCommandEncoderCopyBufferToBuffer(encoder, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxCommandEncoderDestroy(encoder);
}

TEST_P(GfxCommandEncoderTest, CopyBufferToBuffer)
{
    // Create buffers
    GfxBufferDescriptor srcBufferDesc = {};
    srcBufferDesc.label = "source_buffer";
    srcBufferDesc.size = 256;
    srcBufferDesc.usage = GFX_BUFFER_USAGE_COPY_SRC;
    srcBufferDesc.memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL;
    GfxBuffer srcBuffer = nullptr;
    ASSERT_EQ(gfxDeviceCreateBuffer(device, &srcBufferDesc, &srcBuffer), GFX_RESULT_SUCCESS);

    GfxBufferDescriptor dstBufferDesc = {};
    dstBufferDesc.label = "destination_buffer";
    dstBufferDesc.size = 256;
    dstBufferDesc.usage = GFX_BUFFER_USAGE_COPY_DST;
    dstBufferDesc.memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL;
    GfxBuffer dstBuffer = nullptr;
    ASSERT_EQ(gfxDeviceCreateBuffer(device, &dstBufferDesc, &dstBuffer), GFX_RESULT_SUCCESS);

    // Create command encoder
    GfxCommandEncoder encoder = nullptr;
    GfxCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.label = "copy_encoder";
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &encoderDesc, &encoder), GFX_RESULT_SUCCESS);

    // Set up copy operation
    GfxCopyBufferToBufferDescriptor copyDesc = {};
    copyDesc.source = srcBuffer;
    copyDesc.sourceOffset = 0;
    copyDesc.destination = dstBuffer;
    copyDesc.destinationOffset = 0;
    copyDesc.size = 256;

    // Test that copy operation succeeds
    GfxResult result = gfxCommandEncoderCopyBufferToBuffer(encoder, &copyDesc);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);

    gfxCommandEncoderDestroy(encoder);
    gfxBufferDestroy(srcBuffer);
    gfxBufferDestroy(dstBuffer);
}

TEST_P(GfxCommandEncoderTest, CopyBufferToTextureWithNullEncoder)
{
    GfxCopyBufferToTextureDescriptor copyDesc = {};

    GfxResult result = gfxCommandEncoderCopyBufferToTexture(nullptr, &copyDesc);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxCommandEncoderTest, CopyBufferToTextureWithNullDescriptor)
{
    GfxCommandEncoder encoder = nullptr;
    GfxCommandEncoderDescriptor desc = {};
    desc.label = "test_encoder";
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &desc, &encoder), GFX_RESULT_SUCCESS);

    GfxResult result = gfxCommandEncoderCopyBufferToTexture(encoder, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxCommandEncoderDestroy(encoder);
}

TEST_P(GfxCommandEncoderTest, CopyTextureToBufferWithNullEncoder)
{
    GfxCopyTextureToBufferDescriptor copyDesc = {};

    GfxResult result = gfxCommandEncoderCopyTextureToBuffer(nullptr, &copyDesc);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxCommandEncoderTest, CopyTextureToBufferWithNullDescriptor)
{
    GfxCommandEncoder encoder = nullptr;
    GfxCommandEncoderDescriptor desc = {};
    desc.label = "test_encoder";
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &desc, &encoder), GFX_RESULT_SUCCESS);

    GfxResult result = gfxCommandEncoderCopyTextureToBuffer(encoder, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxCommandEncoderDestroy(encoder);
}

TEST_P(GfxCommandEncoderTest, CopyTextureToTextureWithNullEncoder)
{
    GfxCopyTextureToTextureDescriptor copyDesc = {};

    GfxResult result = gfxCommandEncoderCopyTextureToTexture(nullptr, &copyDesc);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxCommandEncoderTest, CopyTextureToTextureWithNullDescriptor)
{
    GfxCommandEncoder encoder = nullptr;
    GfxCommandEncoderDescriptor desc = {};
    desc.label = "test_encoder";
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &desc, &encoder), GFX_RESULT_SUCCESS);

    GfxResult result = gfxCommandEncoderCopyTextureToTexture(encoder, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxCommandEncoderDestroy(encoder);
}

TEST_P(GfxCommandEncoderTest, BlitTextureToTextureWithNullEncoder)
{
    GfxBlitTextureToTextureDescriptor blitDesc = {};

    GfxResult result = gfxCommandEncoderBlitTextureToTexture(nullptr, &blitDesc);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxCommandEncoderTest, BlitTextureToTextureWithNullDescriptor)
{
    GfxCommandEncoder encoder = nullptr;
    GfxCommandEncoderDescriptor desc = {};
    desc.label = "test_encoder";
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &desc, &encoder), GFX_RESULT_SUCCESS);

    GfxResult result = gfxCommandEncoderBlitTextureToTexture(encoder, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxCommandEncoderDestroy(encoder);
}

// Pass encoder tests
TEST_P(GfxCommandEncoderTest, BeginRenderPassWithNullEncoder)
{
    GfxRenderPassBeginDescriptor beginDesc = {};
    GfxRenderPassEncoder passEncoder = nullptr;

    GfxResult result = gfxCommandEncoderBeginRenderPass(nullptr, &beginDesc, &passEncoder);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxCommandEncoderTest, BeginRenderPassWithNullDescriptor)
{
    GfxCommandEncoder encoder = nullptr;
    GfxCommandEncoderDescriptor desc = {};
    desc.label = "test_encoder";
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &desc, &encoder), GFX_RESULT_SUCCESS);

    GfxRenderPassEncoder passEncoder = nullptr;
    GfxResult result = gfxCommandEncoderBeginRenderPass(encoder, nullptr, &passEncoder);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxCommandEncoderDestroy(encoder);
}

TEST_P(GfxCommandEncoderTest, BeginRenderPassWithNullOutput)
{
    GfxCommandEncoder encoder = nullptr;
    GfxCommandEncoderDescriptor desc = {};
    desc.label = "test_encoder";
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &desc, &encoder), GFX_RESULT_SUCCESS);

    GfxRenderPassBeginDescriptor beginDesc = {};
    GfxResult result = gfxCommandEncoderBeginRenderPass(encoder, &beginDesc, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxCommandEncoderDestroy(encoder);
}

TEST_P(GfxCommandEncoderTest, BeginComputePassWithNullEncoder)
{
    GfxComputePassBeginDescriptor beginDesc = {};
    GfxComputePassEncoder passEncoder = nullptr;

    GfxResult result = gfxCommandEncoderBeginComputePass(nullptr, &beginDesc, &passEncoder);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxCommandEncoderTest, BeginComputePassWithNullDescriptor)
{
    GfxCommandEncoder encoder = nullptr;
    GfxCommandEncoderDescriptor desc = {};
    desc.label = "test_encoder";
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &desc, &encoder), GFX_RESULT_SUCCESS);

    GfxComputePassEncoder passEncoder = nullptr;
    GfxResult result = gfxCommandEncoderBeginComputePass(encoder, nullptr, &passEncoder);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxCommandEncoderDestroy(encoder);
}

TEST_P(GfxCommandEncoderTest, BeginComputePassWithNullOutput)
{
    GfxCommandEncoder encoder = nullptr;
    GfxCommandEncoderDescriptor desc = {};
    desc.label = "test_encoder";
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &desc, &encoder), GFX_RESULT_SUCCESS);

    GfxComputePassBeginDescriptor beginDesc = {};
    GfxResult result = gfxCommandEncoderBeginComputePass(encoder, &beginDesc, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxCommandEncoderDestroy(encoder);
}

TEST_P(GfxCommandEncoderTest, GenerateMipmapsWithNullEncoder)
{
    GfxResult result = gfxCommandEncoderGenerateMipmaps(nullptr, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxCommandEncoderTest, GenerateMipmapsWithNullTexture)
{
    GfxCommandEncoder encoder = nullptr;
    GfxCommandEncoderDescriptor desc = {};
    desc.label = "test_encoder";
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &desc, &encoder), GFX_RESULT_SUCCESS);

    GfxResult result = gfxCommandEncoderGenerateMipmaps(encoder, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxCommandEncoderDestroy(encoder);
}

TEST_P(GfxCommandEncoderTest, GenerateMipmapsRangeWithNullEncoder)
{
    GfxResult result = gfxCommandEncoderGenerateMipmapsRange(nullptr, nullptr, 0, 1);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxCommandEncoderTest, GenerateMipmapsRangeWithNullTexture)
{
    GfxCommandEncoder encoder = nullptr;
    GfxCommandEncoderDescriptor desc = {};
    desc.label = "test_encoder";
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &desc, &encoder), GFX_RESULT_SUCCESS);

    GfxResult result = gfxCommandEncoderGenerateMipmapsRange(encoder, nullptr, 0, 1);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxCommandEncoderDestroy(encoder);
}

TEST_P(GfxCommandEncoderTest, PipelineBarrierWithNullEncoder)
{
    GfxPipelineBarrierDescriptor barrierDesc = {};

    GfxResult result = gfxCommandEncoderPipelineBarrier(nullptr, &barrierDesc);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxCommandEncoderTest, PipelineBarrierWithNullDescriptor)
{
    GfxCommandEncoder encoder = nullptr;
    GfxCommandEncoderDescriptor desc = {};
    desc.label = "test_encoder";
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &desc, &encoder), GFX_RESULT_SUCCESS);

    GfxResult result = gfxCommandEncoderPipelineBarrier(encoder, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxCommandEncoderDestroy(encoder);
}

TEST_P(GfxCommandEncoderTest, PipelineBarrierEmpty)
{
    GfxCommandEncoder encoder = nullptr;
    GfxCommandEncoderDescriptor desc = {};
    desc.label = "test_encoder";
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &desc, &encoder), GFX_RESULT_SUCCESS);

    GfxPipelineBarrierDescriptor barrierDesc = {};
    barrierDesc.memoryBarriers = nullptr;
    barrierDesc.memoryBarrierCount = 0;
    barrierDesc.bufferBarriers = nullptr;
    barrierDesc.bufferBarrierCount = 0;
    barrierDesc.textureBarriers = nullptr;
    barrierDesc.textureBarrierCount = 0;

    GfxResult result = gfxCommandEncoderPipelineBarrier(encoder, &barrierDesc);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);

    gfxCommandEncoderDestroy(encoder);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCommandEncoderTest,
    testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU),
    [](const testing::TestParamInfo<GfxBackend>& info) {
        return info.param == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU";
    });

} // namespace
