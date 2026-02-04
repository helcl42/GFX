#include <gfx/gfx.h>

#include <gtest/gtest.h>

#include <cstring>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxQuerySetTest : public testing::TestWithParam<GfxBackend> {
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

// ===========================================================================
// NULL Parameter Validation Tests
// ===========================================================================

TEST_P(GfxQuerySetTest, CreateWithNullDevice)
{
    GfxQuerySetDescriptor querySetDesc = {};
    querySetDesc.type = GFX_QUERY_TYPE_OCCLUSION;
    querySetDesc.count = 16;
    GfxQuerySet querySet = nullptr;
    GfxResult result = gfxDeviceCreateQuerySet(nullptr, &querySetDesc, &querySet);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxQuerySetTest, CreateWithNullDescriptor)
{
    GfxQuerySet querySet = nullptr;
    GfxResult result = gfxDeviceCreateQuerySet(device, nullptr, &querySet);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxQuerySetTest, CreateWithNullOutput)
{
    GfxQuerySetDescriptor querySetDesc = {};
    querySetDesc.type = GFX_QUERY_TYPE_OCCLUSION;
    querySetDesc.count = 16;
    GfxResult result = gfxDeviceCreateQuerySet(device, &querySetDesc, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxQuerySetTest, CreateWithZeroCount)
{
    GfxQuerySetDescriptor querySetDesc = {};
    querySetDesc.type = GFX_QUERY_TYPE_OCCLUSION;
    querySetDesc.count = 0;
    GfxQuerySet querySet = nullptr;
    GfxResult result = gfxDeviceCreateQuerySet(device, &querySetDesc, &querySet);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxQuerySetTest, DestroyWithNullQuerySet)
{
    GfxResult result = gfxQuerySetDestroy(nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ===========================================================================
// Query Set Creation and Destruction Tests
// ===========================================================================

TEST_P(GfxQuerySetTest, CreateAndDestroyOcclusionQuerySet)
{
    GfxQuerySetDescriptor querySetDesc = {};
    querySetDesc.label = "Occlusion Query Set";
    querySetDesc.type = GFX_QUERY_TYPE_OCCLUSION;
    querySetDesc.count = 16;

    GfxQuerySet querySet = nullptr;
    GfxResult result = gfxDeviceCreateQuerySet(device, &querySetDesc, &querySet);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(querySet, nullptr);

    result = gfxQuerySetDestroy(querySet);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
}

TEST_P(GfxQuerySetTest, CreateAndDestroyTimestampQuerySet)
{
    GfxQuerySetDescriptor querySetDesc = {};
    querySetDesc.label = "Timestamp Query Set";
    querySetDesc.type = GFX_QUERY_TYPE_TIMESTAMP;
    querySetDesc.count = 32;

    GfxQuerySet querySet = nullptr;
    GfxResult result = gfxDeviceCreateQuerySet(device, &querySetDesc, &querySet);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(querySet, nullptr);

    result = gfxQuerySetDestroy(querySet);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
}

TEST_P(GfxQuerySetTest, CreateMultipleQuerySets)
{
    GfxQuerySetDescriptor occlusionDesc = {};
    occlusionDesc.type = GFX_QUERY_TYPE_OCCLUSION;
    occlusionDesc.count = 8;

    GfxQuerySetDescriptor timestampDesc = {};
    timestampDesc.type = GFX_QUERY_TYPE_TIMESTAMP;
    timestampDesc.count = 8;

    GfxQuerySet occlusionQuerySet = nullptr;
    GfxQuerySet timestampQuerySet = nullptr;

    ASSERT_EQ(gfxDeviceCreateQuerySet(device, &occlusionDesc, &occlusionQuerySet), GFX_RESULT_SUCCESS);
    ASSERT_EQ(gfxDeviceCreateQuerySet(device, &timestampDesc, &timestampQuerySet), GFX_RESULT_SUCCESS);

    EXPECT_NE(occlusionQuerySet, nullptr);
    EXPECT_NE(timestampQuerySet, nullptr);

    EXPECT_EQ(gfxQuerySetDestroy(occlusionQuerySet), GFX_RESULT_SUCCESS);
    EXPECT_EQ(gfxQuerySetDestroy(timestampQuerySet), GFX_RESULT_SUCCESS);
}

// ===========================================================================
// Command Encoder Query Operations - Validation Tests
// ===========================================================================

TEST_P(GfxQuerySetTest, WriteTimestampWithNullEncoder)
{
    GfxQuerySetDescriptor querySetDesc = {};
    querySetDesc.type = GFX_QUERY_TYPE_TIMESTAMP;
    querySetDesc.count = 8;

    GfxQuerySet querySet = nullptr;
    ASSERT_EQ(gfxDeviceCreateQuerySet(device, &querySetDesc, &querySet), GFX_RESULT_SUCCESS);

    GfxResult result = gfxCommandEncoderWriteTimestamp(nullptr, querySet, 0);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxQuerySetDestroy(querySet);
}

TEST_P(GfxQuerySetTest, WriteTimestampWithNullQuerySet)
{
    GfxCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.label = "Test Encoder";

    GfxCommandEncoder encoder = nullptr;
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &encoderDesc, &encoder), GFX_RESULT_SUCCESS);

    GfxResult result = gfxCommandEncoderWriteTimestamp(encoder, nullptr, 0);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxCommandEncoderDestroy(encoder);
}

TEST_P(GfxQuerySetTest, ResolveQuerySetWithNullEncoder)
{
    GfxQuerySetDescriptor querySetDesc = {};
    querySetDesc.type = GFX_QUERY_TYPE_TIMESTAMP;
    querySetDesc.count = 8;

    GfxQuerySet querySet = nullptr;
    ASSERT_EQ(gfxDeviceCreateQuerySet(device, &querySetDesc, &querySet), GFX_RESULT_SUCCESS);

    GfxBufferDescriptor bufferDesc = {};
    bufferDesc.size = 8 * sizeof(uint64_t);
    bufferDesc.usage = GFX_BUFFER_USAGE_COPY_SRC | GFX_BUFFER_USAGE_COPY_DST;
    bufferDesc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT;

    GfxBuffer buffer = nullptr;
    ASSERT_EQ(gfxDeviceCreateBuffer(device, &bufferDesc, &buffer), GFX_RESULT_SUCCESS);

    GfxResult result = gfxCommandEncoderResolveQuerySet(nullptr, querySet, 0, 8, buffer, 0);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxBufferDestroy(buffer);
    gfxQuerySetDestroy(querySet);
}

TEST_P(GfxQuerySetTest, ResolveQuerySetWithNullQuerySet)
{
    GfxCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.label = "Test Encoder";

    GfxCommandEncoder encoder = nullptr;
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &encoderDesc, &encoder), GFX_RESULT_SUCCESS);

    GfxBufferDescriptor bufferDesc = {};
    bufferDesc.size = 8 * sizeof(uint64_t);
    bufferDesc.usage = GFX_BUFFER_USAGE_COPY_SRC | GFX_BUFFER_USAGE_COPY_DST;
    bufferDesc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT;

    GfxBuffer buffer = nullptr;
    ASSERT_EQ(gfxDeviceCreateBuffer(device, &bufferDesc, &buffer), GFX_RESULT_SUCCESS);

    GfxResult result = gfxCommandEncoderResolveQuerySet(encoder, nullptr, 0, 8, buffer, 0);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxBufferDestroy(buffer);
    gfxCommandEncoderDestroy(encoder);
}

TEST_P(GfxQuerySetTest, ResolveQuerySetWithNullBuffer)
{
    GfxCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.label = "Test Encoder";

    GfxCommandEncoder encoder = nullptr;
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &encoderDesc, &encoder), GFX_RESULT_SUCCESS);

    GfxQuerySetDescriptor querySetDesc = {};
    querySetDesc.type = GFX_QUERY_TYPE_TIMESTAMP;
    querySetDesc.count = 8;

    GfxQuerySet querySet = nullptr;
    ASSERT_EQ(gfxDeviceCreateQuerySet(device, &querySetDesc, &querySet), GFX_RESULT_SUCCESS);

    GfxResult result = gfxCommandEncoderResolveQuerySet(encoder, querySet, 0, 8, nullptr, 0);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxQuerySetDestroy(querySet);
    gfxCommandEncoderDestroy(encoder);
}

// ===========================================================================
// Render Pass Encoder Query Operations - Validation Tests
// ===========================================================================

TEST_P(GfxQuerySetTest, BeginOcclusionQueryWithNullEncoder)
{
    GfxQuerySetDescriptor querySetDesc = {};
    querySetDesc.type = GFX_QUERY_TYPE_OCCLUSION;
    querySetDesc.count = 8;

    GfxQuerySet querySet = nullptr;
    ASSERT_EQ(gfxDeviceCreateQuerySet(device, &querySetDesc, &querySet), GFX_RESULT_SUCCESS);

    GfxResult result = gfxRenderPassEncoderBeginOcclusionQuery(nullptr, querySet, 0);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxQuerySetDestroy(querySet);
}

TEST_P(GfxQuerySetTest, BeginOcclusionQueryWithNullQuerySet)
{
    // Create render pass
    GfxRenderPassColorAttachmentTarget colorTarget = {};
    colorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTarget.ops.loadOp = GFX_LOAD_OP_CLEAR;
    colorTarget.ops.storeOp = GFX_STORE_OP_STORE;
    colorTarget.finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachment = {};
    colorAttachment.target = colorTarget;
    colorAttachment.resolveTarget = nullptr;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;

    GfxRenderPass renderPass = nullptr;
    ASSERT_EQ(gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass), GFX_RESULT_SUCCESS);

    // Create texture and view for framebuffer
    GfxTextureDescriptor colorTextureDesc = {};
    colorTextureDesc.type = GFX_TEXTURE_TYPE_2D;
    colorTextureDesc.size = { 256, 256, 1 };
    colorTextureDesc.arrayLayerCount = 1;
    colorTextureDesc.mipLevelCount = 1;
    colorTextureDesc.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTextureDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTextureDesc.usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;

    GfxTexture colorTexture = nullptr;
    ASSERT_EQ(gfxDeviceCreateTexture(device, &colorTextureDesc, &colorTexture), GFX_RESULT_SUCCESS);

    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    GfxTextureView colorView = nullptr;
    ASSERT_EQ(gfxTextureCreateView(colorTexture, &viewDesc, &colorView), GFX_RESULT_SUCCESS);

    // Create framebuffer
    GfxFramebufferAttachment colorFbAttachment = {};
    colorFbAttachment.view = colorView;
    colorFbAttachment.resolveTarget = nullptr;

    GfxFramebufferDescriptor framebufferDesc = {};
    framebufferDesc.renderPass = renderPass;
    framebufferDesc.colorAttachmentCount = 1;
    framebufferDesc.colorAttachments = &colorFbAttachment;
    framebufferDesc.width = 256;
    framebufferDesc.height = 256;

    GfxFramebuffer framebuffer = nullptr;
    ASSERT_EQ(gfxDeviceCreateFramebuffer(device, &framebufferDesc, &framebuffer), GFX_RESULT_SUCCESS);

    GfxCommandEncoderDescriptor encoderDesc = {};
    GfxCommandEncoder encoder = nullptr;
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &encoderDesc, &encoder), GFX_RESULT_SUCCESS);

    GfxRenderPassBeginDescriptor beginDesc = {};
    beginDesc.renderPass = renderPass;
    beginDesc.framebuffer = framebuffer;

    GfxRenderPassEncoder renderPassEncoder = nullptr;
    ASSERT_EQ(gfxCommandEncoderBeginRenderPass(encoder, &beginDesc, &renderPassEncoder), GFX_RESULT_SUCCESS);

    GfxResult result = gfxRenderPassEncoderBeginOcclusionQuery(renderPassEncoder, nullptr, 0);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxRenderPassEncoderEnd(renderPassEncoder);
    gfxCommandEncoderDestroy(encoder);
    gfxFramebufferDestroy(framebuffer);
    gfxRenderPassDestroy(renderPass);
    gfxTextureViewDestroy(colorView);
    gfxTextureDestroy(colorTexture);
}

TEST_P(GfxQuerySetTest, EndOcclusionQueryWithNullEncoder)
{
    GfxResult result = gfxRenderPassEncoderEndOcclusionQuery(nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ===========================================================================
// Command Encoder Timestamp Query Operations - Functional Tests
// ===========================================================================

TEST_P(GfxQuerySetTest, WriteTimestampOperation)
{
    GfxQuerySetDescriptor querySetDesc = {};
    querySetDesc.label = "Timestamp Query Set";
    querySetDesc.type = GFX_QUERY_TYPE_TIMESTAMP;
    querySetDesc.count = 2;

    GfxQuerySet querySet = nullptr;
    ASSERT_EQ(gfxDeviceCreateQuerySet(device, &querySetDesc, &querySet), GFX_RESULT_SUCCESS);

    GfxCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.label = "Test Encoder";

    GfxCommandEncoder encoder = nullptr;
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &encoderDesc, &encoder), GFX_RESULT_SUCCESS);

    // Write timestamps at beginning and end
    GfxResult result = gfxCommandEncoderWriteTimestamp(encoder, querySet, 0);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);

    result = gfxCommandEncoderWriteTimestamp(encoder, querySet, 1);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);

    gfxCommandEncoderDestroy(encoder);
    gfxQuerySetDestroy(querySet);
}

TEST_P(GfxQuerySetTest, ResolveQuerySetOperation)
{
    GfxQuerySetDescriptor querySetDesc = {};
    querySetDesc.type = GFX_QUERY_TYPE_TIMESTAMP;
    querySetDesc.count = 2;

    GfxQuerySet querySet = nullptr;
    ASSERT_EQ(gfxDeviceCreateQuerySet(device, &querySetDesc, &querySet), GFX_RESULT_SUCCESS);

    GfxBufferDescriptor bufferDesc = {};
    bufferDesc.size = 2 * sizeof(uint64_t);
    bufferDesc.usage = GFX_BUFFER_USAGE_COPY_SRC | GFX_BUFFER_USAGE_COPY_DST;
    bufferDesc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT;

    GfxBuffer buffer = nullptr;
    ASSERT_EQ(gfxDeviceCreateBuffer(device, &bufferDesc, &buffer), GFX_RESULT_SUCCESS);

    GfxCommandEncoderDescriptor encoderDesc = {};
    GfxCommandEncoder encoder = nullptr;
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &encoderDesc, &encoder), GFX_RESULT_SUCCESS);

    // Write timestamps
    ASSERT_EQ(gfxCommandEncoderWriteTimestamp(encoder, querySet, 0), GFX_RESULT_SUCCESS);
    ASSERT_EQ(gfxCommandEncoderWriteTimestamp(encoder, querySet, 1), GFX_RESULT_SUCCESS);

    // Resolve queries to buffer
    GfxResult result = gfxCommandEncoderResolveQuerySet(encoder, querySet, 0, 2, buffer, 0);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);

    gfxCommandEncoderDestroy(encoder);
    gfxBufferDestroy(buffer);
    gfxQuerySetDestroy(querySet);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxQuerySetTest,
    testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU),
    [](const testing::TestParamInfo<GfxBackend>& info) {
        return info.param == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU";
    });

} // namespace
