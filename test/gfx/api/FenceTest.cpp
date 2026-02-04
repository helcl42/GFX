#include <gfx/gfx.h>

#include <gtest/gtest.h>

#include <cstring>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxFenceTest : public testing::TestWithParam<GfxBackend> {
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
TEST_P(GfxFenceTest, CreateWithNullDevice)
{
    GfxFenceDescriptor fenceDesc = {};
    GfxFence fence = nullptr;
    GfxResult result = gfxDeviceCreateFence(nullptr, &fenceDesc, &fence);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxFenceTest, CreateWithNullDescriptor)
{
    GfxFence fence = nullptr;
    GfxResult result = gfxDeviceCreateFence(device, nullptr, &fence);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxFenceTest, CreateWithNullOutput)
{
    GfxFenceDescriptor fenceDesc = {};
    GfxResult result = gfxDeviceCreateFence(device, &fenceDesc, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxFenceTest, DestroyWithNullFence)
{
    GfxResult result = gfxFenceDestroy(nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxFenceTest, GetStatusWithNullFence)
{
    bool isSignaled = false;
    GfxResult result = gfxFenceGetStatus(nullptr, &isSignaled);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxFenceTest, GetStatusWithNullOutput)
{
    GfxFenceDescriptor fenceDesc = {};
    GfxFence fence = nullptr;
    ASSERT_EQ(gfxDeviceCreateFence(device, &fenceDesc, &fence), GFX_RESULT_SUCCESS);

    GfxResult result = gfxFenceGetStatus(fence, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxFenceDestroy(fence);
}

TEST_P(GfxFenceTest, WaitWithNullFence)
{
    GfxResult result = gfxFenceWait(nullptr, 0);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxFenceTest, ResetWithNullFence)
{
    GfxResult result = gfxFenceReset(nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Functional tests
TEST_P(GfxFenceTest, CreateAndDestroy)
{
    GfxFenceDescriptor fenceDesc = {};
    fenceDesc.label = "Test Fence";

    GfxFence fence = nullptr;
    ASSERT_EQ(gfxDeviceCreateFence(device, &fenceDesc, &fence), GFX_RESULT_SUCCESS);
    EXPECT_NE(fence, nullptr);

    EXPECT_EQ(gfxFenceDestroy(fence), GFX_RESULT_SUCCESS);
}

TEST_P(GfxFenceTest, InitialStatusUnsignaled)
{
    GfxFenceDescriptor fenceDesc = {};
    fenceDesc.signaled = false;

    GfxFence fence = nullptr;
    ASSERT_EQ(gfxDeviceCreateFence(device, &fenceDesc, &fence), GFX_RESULT_SUCCESS);

    bool isSignaled = true;
    ASSERT_EQ(gfxFenceGetStatus(fence, &isSignaled), GFX_RESULT_SUCCESS);
    EXPECT_FALSE(isSignaled);

    gfxFenceDestroy(fence);
}

TEST_P(GfxFenceTest, InitialStatusSignaled)
{
    GfxFenceDescriptor fenceDesc = {};
    fenceDesc.signaled = true;

    GfxFence fence = nullptr;
    ASSERT_EQ(gfxDeviceCreateFence(device, &fenceDesc, &fence), GFX_RESULT_SUCCESS);

    bool isSignaled = false;
    ASSERT_EQ(gfxFenceGetStatus(fence, &isSignaled), GFX_RESULT_SUCCESS);
    EXPECT_TRUE(isSignaled);

    gfxFenceDestroy(fence);
}

TEST_P(GfxFenceTest, WaitOnSignaledFence)
{
    GfxFenceDescriptor fenceDesc = {};
    fenceDesc.signaled = true;

    GfxFence fence = nullptr;
    ASSERT_EQ(gfxDeviceCreateFence(device, &fenceDesc, &fence), GFX_RESULT_SUCCESS);

    // Should return immediately since fence is already signaled
    EXPECT_EQ(gfxFenceWait(fence, 0), GFX_RESULT_SUCCESS);

    gfxFenceDestroy(fence);
}

TEST_P(GfxFenceTest, ResetSignaledFence)
{
    GfxFenceDescriptor fenceDesc = {};
    fenceDesc.signaled = true;

    GfxFence fence = nullptr;
    ASSERT_EQ(gfxDeviceCreateFence(device, &fenceDesc, &fence), GFX_RESULT_SUCCESS);

    bool isSignaled = false;
    ASSERT_EQ(gfxFenceGetStatus(fence, &isSignaled), GFX_RESULT_SUCCESS);
    EXPECT_TRUE(isSignaled);

    ASSERT_EQ(gfxFenceReset(fence), GFX_RESULT_SUCCESS);

    ASSERT_EQ(gfxFenceGetStatus(fence, &isSignaled), GFX_RESULT_SUCCESS);
    EXPECT_FALSE(isSignaled);

    gfxFenceDestroy(fence);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxFenceTest,
    testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU),
    [](const testing::TestParamInfo<GfxBackend>& info) {
        return info.param == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU";
    });

} // namespace
