#include <gfx/gfx.h>

#include <gtest/gtest.h>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

class GfxDeviceTest : public testing::TestWithParam<GfxBackend> {
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
};

TEST_P(GfxDeviceTest, CreateDestroyDevice)
{
    GfxDeviceDescriptor desc = {};

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(device, nullptr);
}

TEST_P(GfxDeviceTest, CreateDeviceInvalidArguments)
{
    GfxDeviceDescriptor desc = {};

    // NULL output pointer
    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL adapter
    result = gfxAdapterCreateDevice(NULL, &desc, &device);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL descriptor
    result = gfxAdapterCreateDevice(adapter, NULL, &device);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxDeviceTest, GetDefaultQueue)
{
    GfxDeviceDescriptor desc = {};

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(device, nullptr);

    GfxQueue queue = NULL;
    result = gfxDeviceGetQueue(device, &queue);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(queue, nullptr);
}

TEST_P(GfxDeviceTest, GetQueueByIndex)
{
    // Get queue families first
    uint32_t queueFamilyCount = 0;
    GfxResult result = gfxAdapterEnumerateQueueFamilies(adapter, &queueFamilyCount, NULL);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    if (queueFamilyCount == 0) {
        GTEST_SKIP() << "No queue families available";
    }

    GfxQueueFamilyProperties* queueFamilies = new GfxQueueFamilyProperties[queueFamilyCount];
    result = gfxAdapterEnumerateQueueFamilies(adapter, &queueFamilyCount, queueFamilies);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create device
    GfxDeviceDescriptor desc = {};

    result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Try to get queue from first family
    GfxQueue queue = NULL;
    result = gfxDeviceGetQueueByIndex(device, 0, 0, &queue);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(queue, nullptr);

    delete[] queueFamilies;
}

TEST_P(GfxDeviceTest, GetQueueInvalidArguments)
{
    GfxDeviceDescriptor desc = {};

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // NULL device
    GfxQueue queue = NULL;
    result = gfxDeviceGetQueue(NULL, &queue);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL output pointer
    result = gfxDeviceGetQueue(device, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL device for GetQueueByIndex
    result = gfxDeviceGetQueueByIndex(NULL, 0, 0, &queue);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL output pointer for GetQueueByIndex
    result = gfxDeviceGetQueueByIndex(device, 0, 0, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxDeviceTest, GetQueueInvalidIndex)
{
    GfxDeviceDescriptor desc = {};

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Try to get queue with invalid family index
    GfxQueue queue = NULL;
    result = gfxDeviceGetQueueByIndex(device, 9999, 0, &queue);

    EXPECT_NE(result, GFX_RESULT_SUCCESS);
}

TEST_P(GfxDeviceTest, WaitIdle)
{
    GfxDeviceDescriptor desc = {};

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    result = gfxDeviceWaitIdle(device);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
}

TEST_P(GfxDeviceTest, GetLimits)
{
    GfxDeviceDescriptor desc = {};

    GfxResult result = gfxAdapterCreateDevice(adapter, &desc, &device);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxDeviceLimits limits = {};
    result = gfxDeviceGetLimits(device, &limits);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_GT(limits.maxBufferSize, 0u);
    EXPECT_GT(limits.maxTextureDimension2D, 0u);
}

TEST_P(GfxDeviceTest, MultipleDevices)
{
    // WebGPU backend doesn't support multiple devices from the same adapter
    if (backend == GFX_BACKEND_WEBGPU) {
        GTEST_SKIP() << "WebGPU doesn't support multiple devices from same adapter";
    }

    GfxDeviceDescriptor desc = {};

    GfxDevice device1 = NULL;
    GfxDevice device2 = NULL;

    GfxResult result1 = gfxAdapterCreateDevice(adapter, &desc, &device1);
    GfxResult result2 = gfxAdapterCreateDevice(adapter, &desc, &device2);

    EXPECT_EQ(result1, GFX_RESULT_SUCCESS);
    EXPECT_EQ(result2, GFX_RESULT_SUCCESS);
    EXPECT_NE(device1, nullptr);
    EXPECT_NE(device2, nullptr);
    EXPECT_NE(device1, device2);

    if (device1) {
        gfxDeviceDestroy(device1);
    }
    if (device2) {
        gfxDeviceDestroy(device2);
    }
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxDeviceTest,
    testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU),
    [](const testing::TestParamInfo<GfxBackend>& info) {
        return info.param == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU";
    });

// ===========================================================================
// Non-Parameterized Tests - Backend-independent functionality
// ===========================================================================

TEST(GfxDeviceTestNonParam, DestroyNullDevice)
{
    GfxResult result = gfxDeviceDestroy(NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}
