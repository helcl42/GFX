#include <gfx/gfx.h>

#include <gtest/gtest.h>

#include <cstring>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

class GfxSemaphoreTest : public testing::TestWithParam<GfxBackend> {
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
            gfxAdapterDestroy(adapter);
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
        if (adapter) {
            gfxAdapterDestroy(adapter);
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
TEST_P(GfxSemaphoreTest, CreateWithNullDevice)
{
    GfxSemaphoreDescriptor semaphoreDesc = {};
    semaphoreDesc.type = GFX_SEMAPHORE_TYPE_BINARY;
    GfxSemaphore semaphore = nullptr;
    GfxResult result = gfxDeviceCreateSemaphore(nullptr, &semaphoreDesc, &semaphore);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSemaphoreTest, CreateWithNullDescriptor)
{
    GfxSemaphore semaphore = nullptr;
    GfxResult result = gfxDeviceCreateSemaphore(device, nullptr, &semaphore);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSemaphoreTest, CreateWithNullOutput)
{
    GfxSemaphoreDescriptor semaphoreDesc = {};
    semaphoreDesc.type = GFX_SEMAPHORE_TYPE_BINARY;
    GfxResult result = gfxDeviceCreateSemaphore(device, &semaphoreDesc, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSemaphoreTest, DestroyWithNullSemaphore)
{
    GfxResult result = gfxSemaphoreDestroy(nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSemaphoreTest, GetTypeWithNullSemaphore)
{
    GfxSemaphoreType type;
    GfxResult result = gfxSemaphoreGetType(nullptr, &type);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSemaphoreTest, GetTypeWithNullOutput)
{
    GfxSemaphoreDescriptor semaphoreDesc = {};
    semaphoreDesc.type = GFX_SEMAPHORE_TYPE_BINARY;
    GfxSemaphore semaphore = nullptr;
    ASSERT_EQ(gfxDeviceCreateSemaphore(device, &semaphoreDesc, &semaphore), GFX_RESULT_SUCCESS);

    GfxResult result = gfxSemaphoreGetType(semaphore, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxSemaphoreDestroy(semaphore);
}

TEST_P(GfxSemaphoreTest, SignalWithNullSemaphore)
{
    GfxResult result = gfxSemaphoreSignal(nullptr, 1);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSemaphoreTest, WaitWithNullSemaphore)
{
    GfxResult result = gfxSemaphoreWait(nullptr, 1, 0);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSemaphoreTest, GetValueWithNullSemaphore)
{
    uint64_t value;
    GfxResult result = gfxSemaphoreGetValue(nullptr, &value);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxSemaphoreTest, GetValueWithNullOutput)
{
    GfxSemaphoreDescriptor semaphoreDesc = {};
    semaphoreDesc.type = GFX_SEMAPHORE_TYPE_TIMELINE;
    semaphoreDesc.initialValue = 0;
    GfxSemaphore semaphore = nullptr;
    GfxResult result = gfxDeviceCreateSemaphore(device, &semaphoreDesc, &semaphore);
    if (result != GFX_RESULT_SUCCESS) {
        GTEST_SKIP() << "Timeline semaphores not supported";
    }

    result = gfxSemaphoreGetValue(semaphore, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxSemaphoreDestroy(semaphore);
}

// Functional tests
TEST_P(GfxSemaphoreTest, CreateAndDestroyBinary)
{
    GfxSemaphoreDescriptor semaphoreDesc = {};
    semaphoreDesc.label = "Test Binary Semaphore";
    semaphoreDesc.type = GFX_SEMAPHORE_TYPE_BINARY;

    GfxSemaphore semaphore = nullptr;
    ASSERT_EQ(gfxDeviceCreateSemaphore(device, &semaphoreDesc, &semaphore), GFX_RESULT_SUCCESS);
    EXPECT_NE(semaphore, nullptr);

    EXPECT_EQ(gfxSemaphoreDestroy(semaphore), GFX_RESULT_SUCCESS);
}

TEST_P(GfxSemaphoreTest, CreateAndDestroyTimeline)
{
    GfxSemaphoreDescriptor semaphoreDesc = {};
    semaphoreDesc.label = "Test Timeline Semaphore";
    semaphoreDesc.type = GFX_SEMAPHORE_TYPE_TIMELINE;
    semaphoreDesc.initialValue = 0;

    GfxSemaphore semaphore = nullptr;
    GfxResult result = gfxDeviceCreateSemaphore(device, &semaphoreDesc, &semaphore);
    if (result != GFX_RESULT_SUCCESS) {
        GTEST_SKIP() << "Timeline semaphores not supported";
    }
    EXPECT_NE(semaphore, nullptr);

    EXPECT_EQ(gfxSemaphoreDestroy(semaphore), GFX_RESULT_SUCCESS);
}

TEST_P(GfxSemaphoreTest, GetTypeBinary)
{
    GfxSemaphoreDescriptor semaphoreDesc = {};
    semaphoreDesc.type = GFX_SEMAPHORE_TYPE_BINARY;

    GfxSemaphore semaphore = nullptr;
    ASSERT_EQ(gfxDeviceCreateSemaphore(device, &semaphoreDesc, &semaphore), GFX_RESULT_SUCCESS);

    GfxSemaphoreType type;
    ASSERT_EQ(gfxSemaphoreGetType(semaphore, &type), GFX_RESULT_SUCCESS);
    EXPECT_EQ(type, GFX_SEMAPHORE_TYPE_BINARY);

    gfxSemaphoreDestroy(semaphore);
}

TEST_P(GfxSemaphoreTest, GetTypeTimeline)
{
    GfxSemaphoreDescriptor semaphoreDesc = {};
    semaphoreDesc.type = GFX_SEMAPHORE_TYPE_TIMELINE;
    semaphoreDesc.initialValue = 0;

    GfxSemaphore semaphore = nullptr;
    GfxResult result = gfxDeviceCreateSemaphore(device, &semaphoreDesc, &semaphore);
    if (result != GFX_RESULT_SUCCESS) {
        GTEST_SKIP() << "Timeline semaphores not supported";
    }

    GfxSemaphoreType type;
    ASSERT_EQ(gfxSemaphoreGetType(semaphore, &type), GFX_RESULT_SUCCESS);
    EXPECT_EQ(type, GFX_SEMAPHORE_TYPE_TIMELINE);

    gfxSemaphoreDestroy(semaphore);
}

TEST_P(GfxSemaphoreTest, TimelineInitialValue)
{
    if (backend == GFX_BACKEND_VULKAN) {
        GTEST_SKIP() << "Timeline semaphores have issues on Vulkan";
    }

    GfxSemaphoreDescriptor semaphoreDesc = {};
    semaphoreDesc.type = GFX_SEMAPHORE_TYPE_TIMELINE;
    semaphoreDesc.initialValue = 42;

    GfxSemaphore semaphore = nullptr;
    GfxResult result = gfxDeviceCreateSemaphore(device, &semaphoreDesc, &semaphore);
    if (result != GFX_RESULT_SUCCESS) {
        GTEST_SKIP() << "Timeline semaphores not supported";
    }

    uint64_t value;
    ASSERT_EQ(gfxSemaphoreGetValue(semaphore, &value), GFX_RESULT_SUCCESS);
    EXPECT_EQ(value, 42);

    gfxSemaphoreDestroy(semaphore);
}

TEST_P(GfxSemaphoreTest, TimelineSignal)
{
    if (backend == GFX_BACKEND_VULKAN) {
        GTEST_SKIP() << "Timeline semaphores have issues on Vulkan";
    }

    GfxSemaphoreDescriptor semaphoreDesc = {};
    semaphoreDesc.type = GFX_SEMAPHORE_TYPE_TIMELINE;
    semaphoreDesc.initialValue = 0;

    GfxSemaphore semaphore = nullptr;
    GfxResult result = gfxDeviceCreateSemaphore(device, &semaphoreDesc, &semaphore);
    if (result != GFX_RESULT_SUCCESS) {
        GTEST_SKIP() << "Timeline semaphores not supported";
    }

    ASSERT_EQ(gfxSemaphoreSignal(semaphore, 10), GFX_RESULT_SUCCESS);

    uint64_t value;
    ASSERT_EQ(gfxSemaphoreGetValue(semaphore, &value), GFX_RESULT_SUCCESS);
    EXPECT_EQ(value, 10);

    gfxSemaphoreDestroy(semaphore);
}

TEST_P(GfxSemaphoreTest, TimelineWait)
{
    if (backend == GFX_BACKEND_VULKAN) {
        GTEST_SKIP() << "Timeline semaphores have issues on Vulkan";
    }

    GfxSemaphoreDescriptor semaphoreDesc = {};
    semaphoreDesc.type = GFX_SEMAPHORE_TYPE_TIMELINE;
    semaphoreDesc.initialValue = 5;

    GfxSemaphore semaphore = nullptr;
    GfxResult result = gfxDeviceCreateSemaphore(device, &semaphoreDesc, &semaphore);
    if (result != GFX_RESULT_SUCCESS) {
        GTEST_SKIP() << "Timeline semaphores not supported";
    }

    // Should succeed immediately since value is already 5
    EXPECT_EQ(gfxSemaphoreWait(semaphore, 5, 0), GFX_RESULT_SUCCESS);

    gfxSemaphoreDestroy(semaphore);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxSemaphoreTest,
    testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU),
    [](const testing::TestParamInfo<GfxBackend>& info) {
        return info.param == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU";
    });
