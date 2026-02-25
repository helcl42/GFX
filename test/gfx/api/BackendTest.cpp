#include "CommonTest.h"

// C API tests compiled with C++ for GoogleTest compatibility

// Test version query function

namespace {

TEST(GfxBackendTest, GetVersion)
{
    uint32_t major = 0;
    uint32_t minor = 0;
    uint32_t patch = 0;

    GfxResult result = gfxGetVersion(&major, &minor, &patch);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);

    // Verify expected version (1.0.0)
    EXPECT_EQ(major, 0u);
    EXPECT_EQ(minor, 9u);
    EXPECT_EQ(patch, 0u);

    // Verify GFX_VERSION macro matches
    uint32_t expectedVersion = GFX_MAKE_VERSION(major, minor, patch);
    EXPECT_EQ(GFX_VERSION, expectedVersion);
}

// Test version query with NULL parameters
TEST(GfxBackendTest, GetVersionNullParams)
{
    uint32_t version = 0;

    // All NULL should fail
    GfxResult result = gfxGetVersion(NULL, NULL, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // Any NULL should fail (all three are required)
    result = gfxGetVersion(&version, NULL, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    result = gfxGetVersion(NULL, &version, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    result = gfxGetVersion(NULL, NULL, &version);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // All three non-null should succeed
    uint32_t major = 0, minor = 0, patch = 0;
    result = gfxGetVersion(&major, &minor, &patch);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_EQ(major, 0u);
    EXPECT_EQ(minor, 9u);
    EXPECT_EQ(patch, 0u);
}

// Test backend loading for Vulkan
TEST(GfxBackendTest, LoadVulkanBackend)
{
    GfxResult result = gfxLoadBackend(GFX_BACKEND_VULKAN);

    if (result == GFX_RESULT_SUCCESS) {
        EXPECT_EQ(result, GFX_RESULT_SUCCESS);

        // Cleanup
        result = gfxUnloadBackend(GFX_BACKEND_VULKAN);
        EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    } else {
        GTEST_SKIP() << "Vulkan backend not available on this system";
    }
}

// Test backend loading for WebGPU
TEST(GfxBackendTest, LoadWebGPUBackend)
{
    GfxResult result = gfxLoadBackend(GFX_BACKEND_WEBGPU);

    if (result == GFX_RESULT_SUCCESS) {
        EXPECT_EQ(result, GFX_RESULT_SUCCESS);

        // Cleanup
        result = gfxUnloadBackend(GFX_BACKEND_WEBGPU);
        EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    } else {
        GTEST_SKIP() << "WebGPU backend not available on this system";
    }
}

// Test unloading a backend that was never loaded
TEST(GfxBackendTest, UnloadNeverLoadedBackend)
{
    // Try to unload Vulkan without loading it first
    GfxResult result = gfxUnloadBackend(GFX_BACKEND_VULKAN);

    // Should either succeed (idempotent) or return error
    // Implementation-defined behavior, but shouldn't crash
    EXPECT_TRUE(result == GFX_RESULT_SUCCESS || result < 0);
}

// Test double loading the same backend
TEST(GfxBackendTest, DoubleLoadBackend)
{
    GfxResult result1 = gfxLoadBackend(GFX_BACKEND_VULKAN);

    if (result1 != GFX_RESULT_SUCCESS) {
        GTEST_SKIP() << "Vulkan backend not available";
    }

    // Load again
    GfxResult result2 = gfxLoadBackend(GFX_BACKEND_VULKAN);

    // Should succeed (idempotent) or return an appropriate error
    EXPECT_TRUE(result2 == GFX_RESULT_SUCCESS || result2 < 0);

    // Cleanup
    gfxUnloadBackend(GFX_BACKEND_VULKAN);
}

// Test double unloading the same backend
TEST(GfxBackendTest, DoubleUnloadBackend)
{
    GfxResult result = gfxLoadBackend(GFX_BACKEND_VULKAN);

    if (result != GFX_RESULT_SUCCESS) {
        GTEST_SKIP() << "Vulkan backend not available";
    }

    // First unload
    result = gfxUnloadBackend(GFX_BACKEND_VULKAN);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);

    // Second unload
    result = gfxUnloadBackend(GFX_BACKEND_VULKAN);

    // Should either succeed (idempotent) or return error
    EXPECT_TRUE(result == GFX_RESULT_SUCCESS || result < 0);
}

// Test loading all backends
TEST(GfxBackendTest, LoadAllBackends)
{
    GfxResult result = gfxLoadAllBackends();

    // Should succeed even if some backends aren't available
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);

    // Cleanup
    result = gfxUnloadAllBackends();
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
}

// Test unloading all backends
TEST(GfxBackendTest, UnloadAllBackends)
{
    // Load all backends first
    GfxResult result = gfxLoadAllBackends();
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);

    // Unload all
    result = gfxUnloadAllBackends();
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
}

// Test unloading all backends when none are loaded
TEST(GfxBackendTest, UnloadAllBackendsWhenNoneLoaded)
{
    // Ensure nothing is loaded
    gfxUnloadAllBackends();

    // Try unloading again
    GfxResult result = gfxUnloadAllBackends();

    // Should succeed (idempotent)
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
}

// Test loading multiple backends individually
TEST(GfxBackendTest, LoadMultipleBackends)
{
    GfxResult vulkanResult = gfxLoadBackend(GFX_BACKEND_VULKAN);
    GfxResult webgpuResult = gfxLoadBackend(GFX_BACKEND_WEBGPU);

    // At least one should succeed on most systems
    bool anyLoaded = (vulkanResult == GFX_RESULT_SUCCESS || webgpuResult == GFX_RESULT_SUCCESS);

    if (!anyLoaded) {
        GTEST_SKIP() << "No backends available on this system";
    }

    // Cleanup
    if (vulkanResult == GFX_RESULT_SUCCESS) {
        gfxUnloadBackend(GFX_BACKEND_VULKAN);
    }
    if (webgpuResult == GFX_RESULT_SUCCESS) {
        gfxUnloadBackend(GFX_BACKEND_WEBGPU);
    }
}

// Test that instance creation requires loaded backend
TEST(GfxBackendTest, InstanceCreationRequiresLoadedBackend)
{
    // Ensure backend is unloaded
    gfxUnloadBackend(GFX_BACKEND_VULKAN);

    // Try to create instance without loading backend
    const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
    GfxInstanceDescriptor desc = {};
    desc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
    desc.pNext = nullptr;
    desc.backend = GFX_BACKEND_VULKAN;
    desc.enabledExtensions = extensions;
    desc.enabledExtensionCount = 1;

    GfxInstance instance = nullptr;
    GfxResult result = gfxCreateInstance(&desc, &instance);

    // Should fail with backend not loaded error
    EXPECT_EQ(result, GFX_RESULT_ERROR_BACKEND_NOT_LOADED);
    EXPECT_EQ(instance, nullptr);
}

// Test backend load/unload cycle
TEST(GfxBackendTest, LoadUnloadCycle)
{
    for (int i = 0; i < 3; ++i) {
        GfxResult loadResult = gfxLoadBackend(GFX_BACKEND_VULKAN);

        if (loadResult != GFX_RESULT_SUCCESS) {
            GTEST_SKIP() << "Vulkan backend not available";
        }

        EXPECT_EQ(loadResult, GFX_RESULT_SUCCESS);

        GfxResult unloadResult = gfxUnloadBackend(GFX_BACKEND_VULKAN);
        EXPECT_EQ(unloadResult, GFX_RESULT_SUCCESS);
    }
}

// Test invalid backend enum
TEST(GfxBackendTest, LoadInvalidBackend)
{
    // Try to load with invalid backend value
    GfxBackend invalidBackend = static_cast<GfxBackend>(999);
    GfxResult result = gfxLoadBackend(invalidBackend);

    // Should return an error
    EXPECT_LT(result, 0);
}

// Test AUTO backend
TEST(GfxBackendTest, LoadAutoBackend)
{
    GfxResult result = gfxLoadBackend(GFX_BACKEND_AUTO);

    // AUTO might not be supported for explicit loading
    // Implementation-defined behavior
    EXPECT_TRUE(result == GFX_RESULT_SUCCESS || result < 0);

    if (result == GFX_RESULT_SUCCESS) {
        gfxUnloadBackend(GFX_BACKEND_AUTO);
    }
}

} // namespace
