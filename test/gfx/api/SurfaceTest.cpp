#include <gfx/gfx.h>

#include <gtest/gtest.h>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

class GfxSurfaceTest : public testing::TestWithParam<GfxBackend> {
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
            gfxAdapterDestroy(adapter);
            gfxInstanceDestroy(instance);
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to create device";
        }
    }
    
    void TearDown() override
    {
        if (surface) {
            gfxSurfaceDestroy(surface);
        }
        if (device) {
            gfxDeviceDestroy(device);
        }
        if (adapter) {
            gfxAdapterDestroy(adapter);
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
};

TEST_P(GfxSurfaceTest, CreateSurfaceInvalidArguments)
{
    GfxSurfaceDescriptor desc = {};
    desc.label = "TestSurface";
    desc.windowHandle.windowingSystem = GFX_WINDOWING_SYSTEM_XLIB;
    desc.windowHandle.xlib.display = NULL; // Invalid display
    desc.windowHandle.xlib.window = 0;     // Invalid window

    // NULL device
    GfxResult result = gfxDeviceCreateSurface(NULL, &desc, &surface);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL descriptor
    result = gfxDeviceCreateSurface(device, NULL, &surface);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL output pointer
    result = gfxDeviceCreateSurface(device, &desc, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSurfaceTest, DestroyNullSurface)
{
    GfxResult result = gfxSurfaceDestroy(NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSurfaceTest, EnumerateSupportedFormatsInvalidArguments)
{
    // Create a dummy surface pointer (won't be valid, but testing argument validation)
    GfxSurface dummySurface = (GfxSurface)0x1;
    uint32_t formatCount = 0;

    // NULL surface
    GfxResult result = gfxSurfaceEnumerateSupportedFormats(NULL, &formatCount, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL count pointer
    result = gfxSurfaceEnumerateSupportedFormats(dummySurface, NULL, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSurfaceTest, EnumerateSupportedPresentModesInvalidArguments)
{
    // Create a dummy surface pointer (won't be valid, but testing argument validation)
    GfxSurface dummySurface = (GfxSurface)0x1;
    uint32_t presentModeCount = 0;

    // NULL surface
    GfxResult result = gfxSurfaceEnumerateSupportedPresentModes(NULL, &presentModeCount, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL count pointer
    result = gfxSurfaceEnumerateSupportedPresentModes(dummySurface, NULL, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSurfaceTest, GetQueueFamilySurfaceSupportInvalidArguments)
{
    // Create a dummy surface pointer (won't be valid, but testing argument validation)
    GfxSurface dummySurface = (GfxSurface)0x1;
    bool supported = false;

    // NULL adapter
    GfxResult result = gfxAdapterGetQueueFamilySurfaceSupport(NULL, 0, dummySurface, &supported);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL surface
    result = gfxAdapterGetQueueFamilySurfaceSupport(adapter, 0, NULL, &supported);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL output pointer
    result = gfxAdapterGetQueueFamilySurfaceSupport(adapter, 0, dummySurface, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Note: Creating actual surfaces requires real window handles from X11/Wayland/etc.
// These tests verify API contracts and argument validation without requiring a display server

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxSurfaceTest,
    testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU),
    [](const testing::TestParamInfo<GfxBackend>& info) {
        return info.param == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU";
    }
);
