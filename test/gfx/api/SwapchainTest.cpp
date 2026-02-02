#include <gfx/gfx.h>

#include <gtest/gtest.h>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxSwapchainTest : public testing::TestWithParam<GfxBackend> {
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
        adapterDesc.adapterIndex = UINT32_MAX;
        adapterDesc.preference = GFX_ADAPTER_PREFERENCE_HIGH_PERFORMANCE;

        if (gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter) != GFX_RESULT_SUCCESS) {
            gfxInstanceDestroy(instance);
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to get adapter";
        }

        GfxDeviceDescriptor deviceDesc = {};

        if (gfxAdapterCreateDevice(adapter, &deviceDesc, &device) != GFX_RESULT_SUCCESS) {
            gfxInstanceDestroy(instance);
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to create device";
        }
    }

    void TearDown() override
    {
        if (swapchain) {
            gfxSwapchainDestroy(swapchain);
        }
        if (surface) {
            gfxSurfaceDestroy(surface);
        }
        if (device) {
            gfxDeviceDestroy(device);
        }
        if (instance) {
            gfxInstanceDestroy(instance);
        }
        gfxUnloadBackend(backend);
    }

    GfxBackend backend;
    GfxInstance instance = NULL;
    GfxAdapter adapter = NULL;
    GfxDevice device = NULL;
    GfxSurface surface = NULL;
    GfxSwapchain swapchain = NULL;
};

TEST_P(GfxSwapchainTest, CreateSwapchainInvalidArguments)
{
    // Create a dummy surface pointer for testing (won't be valid, but testing argument validation)
    GfxSurface dummySurface = (GfxSurface)0x1;

    GfxSwapchainDescriptor desc = {};
    desc.label = "TestSwapchain";
    desc.surface = dummySurface;
    desc.width = 800;
    desc.height = 600;
    desc.format = GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;
    desc.presentMode = GFX_PRESENT_MODE_FIFO;
    desc.imageCount = 2;

    // NULL device
    GfxResult result = gfxDeviceCreateSwapchain(NULL, &desc, &swapchain);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL surface in descriptor
    desc.surface = NULL;
    result = gfxDeviceCreateSwapchain(device, &desc, &swapchain);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL descriptor
    result = gfxDeviceCreateSwapchain(device, NULL, &swapchain);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL output pointer
    desc.surface = dummySurface;
    result = gfxDeviceCreateSwapchain(device, &desc, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSwapchainTest, CreateSwapchainInvalidDimensions)
{
    // Test that dimension validation happens (should reject before trying to use surface)
    GfxSwapchainDescriptor desc = {};
    desc.label = "TestSwapchain";
    desc.width = 0; // Invalid
    desc.surface = NULL;
    desc.height = 600;
    desc.format = GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;
    desc.presentMode = GFX_PRESENT_MODE_FIFO;
    desc.imageCount = 2;

    // Zero width should be rejected before surface is accessed
    GfxResult result = gfxDeviceCreateSwapchain(device, &desc, &swapchain);
    EXPECT_NE(result, GFX_RESULT_SUCCESS);
    EXPECT_TRUE(result == GFX_RESULT_ERROR_INVALID_ARGUMENT || result < 0);

    // Zero height should be rejected
    desc.width = 800;
    desc.height = 0;
    result = gfxDeviceCreateSwapchain(device, &desc, &swapchain);
    EXPECT_NE(result, GFX_RESULT_SUCCESS);
    EXPECT_TRUE(result == GFX_RESULT_ERROR_INVALID_ARGUMENT || result < 0);

    // Both zero should be rejected
    desc.width = 0;
    desc.height = 0;
    result = gfxDeviceCreateSwapchain(device, &desc, &swapchain);
    EXPECT_NE(result, GFX_RESULT_SUCCESS);
    EXPECT_TRUE(result == GFX_RESULT_ERROR_INVALID_ARGUMENT || result < 0);
}

TEST_P(GfxSwapchainTest, CreateSwapchainInvalidImageCount)
{
    // Test that imageCount validation happens
    GfxSwapchainDescriptor desc = {};
    desc.label = "TestSwapchain";
    desc.surface = NULL;
    desc.width = 800;
    desc.height = 600;
    desc.format = GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_RENDER_ATTACHMENT;
    desc.presentMode = GFX_PRESENT_MODE_FIFO;
    desc.imageCount = 0; // Invalid

    // Zero image count should be rejected
    GfxResult result = gfxDeviceCreateSwapchain(device, &desc, &swapchain);
    EXPECT_NE(result, GFX_RESULT_SUCCESS);
    EXPECT_TRUE(result == GFX_RESULT_ERROR_INVALID_ARGUMENT || result < 0);
}

TEST_P(GfxSwapchainTest, DestroyNullSwapchain)
{
    GfxResult result = gfxSwapchainDestroy(NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSwapchainTest, GetInfoInvalidArguments)
{
    GfxSwapchain dummySwapchain = (GfxSwapchain)0x1;
    GfxSwapchainInfo info = {};

    // NULL swapchain
    GfxResult result = gfxSwapchainGetInfo(NULL, &info);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL output pointer
    result = gfxSwapchainGetInfo(dummySwapchain, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSwapchainTest, AcquireNextImageInvalidArguments)
{
    GfxSwapchain dummySwapchain = (GfxSwapchain)0x1;
    uint32_t imageIndex = 0;

    // NULL swapchain
    GfxResult result = gfxSwapchainAcquireNextImage(NULL, 0, NULL, NULL, &imageIndex);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL output pointer
    result = gfxSwapchainAcquireNextImage(dummySwapchain, 0, NULL, NULL, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSwapchainTest, GetTextureViewInvalidArguments)
{
    GfxSwapchain dummySwapchain = (GfxSwapchain)0x1;
    GfxTextureView view = NULL;

    // NULL swapchain
    GfxResult result = gfxSwapchainGetTextureView(NULL, 0, &view);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL output pointer
    result = gfxSwapchainGetTextureView(dummySwapchain, 0, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSwapchainTest, GetCurrentTextureViewInvalidArguments)
{
    GfxSwapchain dummySwapchain = (GfxSwapchain)0x1;
    GfxTextureView view = NULL;

    // NULL swapchain
    GfxResult result = gfxSwapchainGetCurrentTextureView(NULL, &view);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL output pointer
    result = gfxSwapchainGetCurrentTextureView(dummySwapchain, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSwapchainTest, PresentInvalidArguments)
{
    // NULL swapchain
    GfxResult result = gfxSwapchainPresent(NULL, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Note: Creating actual swapchains requires valid surfaces with real window handles.
// These tests verify API contracts and argument validation without requiring display servers.

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxSwapchainTest,
    testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU),
    [](const testing::TestParamInfo<GfxBackend>& info) {
        return info.param == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU";
    });

} // namespace
