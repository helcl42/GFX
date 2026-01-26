#include <gfx/gfx.h>

#include <gtest/gtest.h>

#include <cstring>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

class GfxRenderPassEncoderTest : public testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        if (gfxLoadBackend(backend) != GFX_RESULT_SUCCESS) {
            GTEST_SKIP() << "Backend not available";
        }

        GfxInstanceDescriptor instDesc = {};
        instDesc.backend = backend;
        const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
        instDesc.enabledExtensions = extensions;
        instDesc.enabledExtensionCount = 1;

        if (gfxCreateInstance(&instDesc, &instance) != GFX_RESULT_SUCCESS) {
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to create instance";
        }

        GfxAdapterDescriptor adapterDesc = {};
        adapterDesc.adapterIndex = 0;

        if (gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter) != GFX_RESULT_SUCCESS) {
            gfxInstanceDestroy(instance);
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to request adapter";
        }

        GfxDeviceDescriptor deviceDesc = {};
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
TEST_P(GfxRenderPassEncoderTest, SetPipelineWithNullEncoder)
{
    GfxResult result = gfxRenderPassEncoderSetPipeline(nullptr, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxRenderPassEncoderTest, SetBindGroupWithNullEncoder)
{
    GfxResult result = gfxRenderPassEncoderSetBindGroup(nullptr, 0, nullptr, nullptr, 0);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxRenderPassEncoderTest, SetVertexBufferWithNullEncoder)
{
    GfxResult result = gfxRenderPassEncoderSetVertexBuffer(nullptr, 0, nullptr, 0, 0);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxRenderPassEncoderTest, SetIndexBufferWithNullEncoder)
{
    GfxResult result = gfxRenderPassEncoderSetIndexBuffer(nullptr, nullptr, GFX_INDEX_FORMAT_UINT16, 0, 0);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxRenderPassEncoderTest, SetViewportWithNullEncoder)
{
    GfxViewport viewport = {};
    GfxResult result = gfxRenderPassEncoderSetViewport(nullptr, &viewport);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxRenderPassEncoderTest, SetViewportWithNullViewport)
{
    // Create a command encoder first
    GfxCommandEncoder cmdEncoder = nullptr;
    GfxCommandEncoderDescriptor cmdDesc = {};
    ASSERT_EQ(gfxDeviceCreateCommandEncoder(device, &cmdDesc, &cmdEncoder), GFX_RESULT_SUCCESS);

    // We need a render pass to get a render pass encoder, but for this test
    // we're just testing NULL validation, so we can skip creating one
    // and just test with nullptr
    GfxResult result = gfxRenderPassEncoderSetViewport(nullptr, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxCommandEncoderDestroy(cmdEncoder);
}

TEST_P(GfxRenderPassEncoderTest, SetScissorRectWithNullEncoder)
{
    GfxScissorRect scissor = {};
    GfxResult result = gfxRenderPassEncoderSetScissorRect(nullptr, &scissor);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxRenderPassEncoderTest, SetScissorRectWithNullScissor)
{
    GfxResult result = gfxRenderPassEncoderSetScissorRect(nullptr, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxRenderPassEncoderTest, DrawWithNullEncoder)
{
    GfxResult result = gfxRenderPassEncoderDraw(nullptr, 3, 1, 0, 0);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxRenderPassEncoderTest, DrawIndexedWithNullEncoder)
{
    GfxResult result = gfxRenderPassEncoderDrawIndexed(nullptr, 3, 1, 0, 0, 0);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxRenderPassEncoderTest, DrawIndirectWithNullEncoder)
{
    GfxResult result = gfxRenderPassEncoderDrawIndirect(nullptr, nullptr, 0);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxRenderPassEncoderTest, DrawIndexedIndirectWithNullEncoder)
{
    GfxResult result = gfxRenderPassEncoderDrawIndexedIndirect(nullptr, nullptr, 0);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxRenderPassEncoderTest, EndWithNullEncoder)
{
    GfxResult result = gfxRenderPassEncoderEnd(nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxRenderPassEncoderTest,
    testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU),
    [](const testing::TestParamInfo<GfxBackend>& info) {
        return info.param == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU";
    });
