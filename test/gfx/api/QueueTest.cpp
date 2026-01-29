#include <gfx/gfx.h>

#include <cstring>
#include <gtest/gtest.h>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

class GfxQueueTest : public testing::TestWithParam<GfxBackend> {
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

// ===========================================================================
// Queue Tests
// ===========================================================================

// Test: Get default queue with NULL device
TEST_P(GfxQueueTest, GetQueueWithNullDevice)
{
    GfxQueue queue = nullptr;
    GfxResult result = gfxDeviceGetQueue(nullptr, &queue);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Test: Get default queue with NULL output
TEST_P(GfxQueueTest, GetQueueWithNullOutput)
{
    GfxResult result = gfxDeviceGetQueue(device, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Test: Get default queue
TEST_P(GfxQueueTest, GetDefaultQueue)
{
    GfxQueue queue = nullptr;
    GfxResult result = gfxDeviceGetQueue(device, &queue);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(queue, nullptr);
}

// Test: Get queue by index with NULL device
TEST_P(GfxQueueTest, GetQueueByIndexWithNullDevice)
{
    GfxQueue queue = nullptr;
    GfxResult result = gfxDeviceGetQueueByIndex(nullptr, 0, 0, &queue);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Test: Get queue by index with NULL output
TEST_P(GfxQueueTest, GetQueueByIndexWithNullOutput)
{
    GfxResult result = gfxDeviceGetQueueByIndex(device, 0, 0, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Test: Get queue by index
TEST_P(GfxQueueTest, GetQueueByIndex)
{
    GfxQueue queue = nullptr;
    GfxResult result = gfxDeviceGetQueueByIndex(device, 0, 0, &queue);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(queue, nullptr);
}

// Test: Queue submit with NULL queue
TEST_P(GfxQueueTest, SubmitWithNullQueue)
{
    GfxSubmitDescriptor submitDesc = {};
    GfxResult result = gfxQueueSubmit(nullptr, &submitDesc);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Test: Queue submit with NULL descriptor
TEST_P(GfxQueueTest, SubmitWithNullDescriptor)
{
    GfxQueue queue = nullptr;
    GfxResult result = gfxDeviceGetQueue(device, &queue);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    result = gfxQueueSubmit(queue, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Test: Queue submit with empty descriptor
TEST_P(GfxQueueTest, SubmitWithEmptyDescriptor)
{
    GfxQueue queue = nullptr;
    GfxResult result = gfxDeviceGetQueue(device, &queue);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxSubmitDescriptor submitDesc = {};
    submitDesc.commandEncoders = nullptr;
    submitDesc.commandEncoderCount = 0;

    result = gfxQueueSubmit(queue, &submitDesc);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
}

// Test: Queue write buffer with NULL queue
TEST_P(GfxQueueTest, WriteBufferWithNullQueue)
{
    uint32_t data = 42;
    GfxResult result = gfxQueueWriteBuffer(nullptr, nullptr, 0, &data, sizeof(data));
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Test: Queue write buffer with NULL buffer
TEST_P(GfxQueueTest, WriteBufferWithNullBuffer)
{
    GfxQueue queue = nullptr;
    GfxResult result = gfxDeviceGetQueue(device, &queue);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    uint32_t data = 42;
    result = gfxQueueWriteBuffer(queue, nullptr, 0, &data, sizeof(data));
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Test: Queue write buffer with NULL data
TEST_P(GfxQueueTest, WriteBufferWithNullData)
{
    GfxQueue queue = nullptr;
    GfxResult result = gfxDeviceGetQueue(device, &queue);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create a small buffer
    GfxBufferDescriptor bufferDesc = {};
    bufferDesc.label = "Test Buffer";
    bufferDesc.size = 256;
    bufferDesc.usage = GFX_BUFFER_USAGE_COPY_DST;
    bufferDesc.memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL;

    GfxBuffer buffer = nullptr;
    result = gfxDeviceCreateBuffer(device, &bufferDesc, &buffer);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    result = gfxQueueWriteBuffer(queue, buffer, 0, nullptr, 256);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxBufferDestroy(buffer);
}

// Test: Queue write buffer
TEST_P(GfxQueueTest, WriteBuffer)
{
    GfxQueue queue = nullptr;
    GfxResult result = gfxDeviceGetQueue(device, &queue);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create a buffer
    GfxBufferDescriptor bufferDesc = {};
    bufferDesc.label = "Test Buffer";
    bufferDesc.size = 256;
    bufferDesc.usage = GFX_BUFFER_USAGE_COPY_DST;
    bufferDesc.memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL;

    GfxBuffer buffer = nullptr;
    result = gfxDeviceCreateBuffer(device, &bufferDesc, &buffer);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Write data to buffer
    uint32_t data[64];
    for (int i = 0; i < 64; i++) {
        data[i] = i;
    }

    result = gfxQueueWriteBuffer(queue, buffer, 0, data, sizeof(data));
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);

    gfxBufferDestroy(buffer);
}

// Test: Queue wait idle with NULL queue
TEST_P(GfxQueueTest, WaitIdleWithNullQueue)
{
    GfxResult result = gfxQueueWaitIdle(nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Test: Queue wait idle
TEST_P(GfxQueueTest, WaitIdle)
{
    GfxQueue queue = nullptr;
    GfxResult result = gfxDeviceGetQueue(device, &queue);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    result = gfxQueueWaitIdle(queue);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
}

// Test: Queue write buffer with offset
TEST_P(GfxQueueTest, WriteBufferWithOffset)
{
    GfxQueue queue = nullptr;
    GfxResult result = gfxDeviceGetQueue(device, &queue);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create a buffer
    GfxBufferDescriptor bufferDesc = {};
    bufferDesc.label = "Test Buffer";
    bufferDesc.size = 256;
    bufferDesc.usage = GFX_BUFFER_USAGE_COPY_DST;
    bufferDesc.memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL;

    GfxBuffer buffer = nullptr;
    result = gfxDeviceCreateBuffer(device, &bufferDesc, &buffer);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Write data at offset 64
    uint32_t data[16];
    for (int i = 0; i < 16; i++) {
        data[i] = i + 100;
    }

    result = gfxQueueWriteBuffer(queue, buffer, 64, data, sizeof(data));
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);

    gfxBufferDestroy(buffer);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxQueueTest,
    testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU),
    [](const testing::TestParamInfo<GfxBackend>& info) {
        return info.param == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU";
    });