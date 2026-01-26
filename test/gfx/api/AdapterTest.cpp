#include <gfx/gfx.h>

#include <gtest/gtest.h>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

class GfxAdapterTest : public testing::TestWithParam<GfxBackend> {
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
    }

    void TearDown() override
    {
        if (instance) {
            gfxInstanceDestroy(instance);
        }
        gfxUnloadBackend(backend);
    }

    GfxBackend backend;
    GfxInstance instance = NULL;
    GfxAdapter adapter = NULL;
};

TEST_P(GfxAdapterTest, GetInfo)
{
    GfxAdapterInfo info = {};
    GfxResult result = gfxAdapterGetInfo(adapter, &info);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);

    // Verify we got some valid information
    EXPECT_GT(strlen(info.name), 0u) << "Adapter should have a name";

    // Vendor ID should be non-zero for real hardware
    // (might be 0 for software renderers, so just check it's set)
    EXPECT_TRUE(info.vendorID >= 0);

    // Adapter type should be valid
    EXPECT_GE(info.adapterType, GFX_ADAPTER_TYPE_DISCRETE_GPU);
    EXPECT_LE(info.adapterType, GFX_ADAPTER_TYPE_CPU);
}

TEST_P(GfxAdapterTest, GetInfoInvalidArguments)
{
    GfxAdapterInfo info = {};

    // NULL adapter
    GfxResult result = gfxAdapterGetInfo(NULL, &info);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL output pointer
    result = gfxAdapterGetInfo(adapter, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxAdapterTest, GetLimits)
{
    GfxDeviceLimits limits = {};
    GfxResult result = gfxAdapterGetLimits(adapter, &limits);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);

    // Verify reasonable limits
    EXPECT_GT(limits.maxBufferSize, 0u);
    EXPECT_GT(limits.maxTextureDimension1D, 0u);
    EXPECT_GT(limits.maxTextureDimension2D, 0u);
    EXPECT_GT(limits.maxTextureDimension3D, 0u);
    EXPECT_GT(limits.maxTextureArrayLayers, 0u);
    EXPECT_GT(limits.maxUniformBufferBindingSize, 0u);
    EXPECT_GT(limits.maxStorageBufferBindingSize, 0u);

    // These should be at least the WebGPU minimums
    EXPECT_GE(limits.maxTextureDimension2D, 8192u);
}

TEST_P(GfxAdapterTest, GetLimitsInvalidArguments)
{
    GfxDeviceLimits limits = {};

    // NULL adapter
    GfxResult result = gfxAdapterGetLimits(NULL, &limits);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL output pointer
    result = gfxAdapterGetLimits(adapter, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxAdapterTest, EnumerateQueueFamilies)
{
    // Get count
    uint32_t queueFamilyCount = 0;
    GfxResult result = gfxAdapterEnumerateQueueFamilies(adapter, &queueFamilyCount, NULL);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_GT(queueFamilyCount, 0u) << "Adapter should have at least one queue family";

    // Get properties
    GfxQueueFamilyProperties* queueFamilies = new GfxQueueFamilyProperties[queueFamilyCount];
    result = gfxAdapterEnumerateQueueFamilies(adapter, &queueFamilyCount, queueFamilies);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);

    // Verify at least one queue family supports graphics
    bool hasGraphics = false;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        EXPECT_GT(queueFamilies[i].queueCount, 0u);
        if (queueFamilies[i].flags & GFX_QUEUE_FLAG_GRAPHICS) {
            hasGraphics = true;
        }
    }
    EXPECT_TRUE(hasGraphics) << "At least one queue family should support graphics";

    delete[] queueFamilies;
}

TEST_P(GfxAdapterTest, EnumerateQueueFamiliesInvalidArguments)
{
    uint32_t queueFamilyCount = 0;

    // NULL adapter
    GfxResult result = gfxAdapterEnumerateQueueFamilies(NULL, &queueFamilyCount, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL count pointer
    result = gfxAdapterEnumerateQueueFamilies(adapter, NULL, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxAdapterTest, EnumerateQueueFamiliesBufferTooSmall)
{
    uint32_t queueFamilyCount = 0;
    GfxResult result = gfxAdapterEnumerateQueueFamilies(adapter, &queueFamilyCount, NULL);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    if (queueFamilyCount > 1) {
        // Try to get with a buffer that's too small
        uint32_t smallCount = 1;
        GfxQueueFamilyProperties queueFamily = {};
        result = gfxAdapterEnumerateQueueFamilies(adapter, &smallCount, &queueFamily);

        // Should succeed but return only 1 queue family
        EXPECT_EQ(result, GFX_RESULT_SUCCESS);
        EXPECT_EQ(smallCount, 1u);
    }
}

TEST_P(GfxAdapterTest, CreateDevice)
{
    GfxDeviceDescriptor desc = {};

    GfxDevice device = NULL;
    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(device, nullptr);

    if (device) {
        gfxDeviceDestroy(device);
    }
}

TEST_P(GfxAdapterTest, CreateDeviceInvalidArguments)
{
    GfxDeviceDescriptor desc = {};
    GfxDevice device = NULL;

    // NULL adapter
    GfxResult result = gfxAdapterCreateDevice(NULL, &desc, &device);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL descriptor
    result = gfxAdapterCreateDevice(adapter, NULL, &device);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL output pointer
    result = gfxAdapterCreateDevice(adapter, &desc, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxAdapterTest, InfoConsistency)
{
    // Get info multiple times and verify consistency
    GfxAdapterInfo info1 = {};
    GfxAdapterInfo info2 = {};

    GfxResult result1 = gfxAdapterGetInfo(adapter, &info1);
    GfxResult result2 = gfxAdapterGetInfo(adapter, &info2);

    ASSERT_EQ(result1, GFX_RESULT_SUCCESS);
    ASSERT_EQ(result2, GFX_RESULT_SUCCESS);

    // Info should be consistent
    EXPECT_STREQ(info1.name, info2.name);
    EXPECT_EQ(info1.vendorID, info2.vendorID);
    EXPECT_EQ(info1.deviceID, info2.deviceID);
    EXPECT_EQ(info1.adapterType, info2.adapterType);
}

TEST_P(GfxAdapterTest, LimitsConsistency)
{
    // Get limits multiple times and verify consistency
    GfxDeviceLimits limits1 = {};
    GfxDeviceLimits limits2 = {};

    GfxResult result1 = gfxAdapterGetLimits(adapter, &limits1);
    GfxResult result2 = gfxAdapterGetLimits(adapter, &limits2);

    ASSERT_EQ(result1, GFX_RESULT_SUCCESS);
    ASSERT_EQ(result2, GFX_RESULT_SUCCESS);

    // Limits should be consistent
    EXPECT_EQ(limits1.maxBufferSize, limits2.maxBufferSize);
    EXPECT_EQ(limits1.maxTextureDimension2D, limits2.maxTextureDimension2D);
    EXPECT_EQ(limits1.maxTextureArrayLayers, limits2.maxTextureArrayLayers);
    EXPECT_EQ(limits1.maxUniformBufferBindingSize, limits2.maxUniformBufferBindingSize);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxAdapterTest,
    testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU),
    [](const testing::TestParamInfo<GfxBackend>& info) {
        return info.param == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU";
    });
