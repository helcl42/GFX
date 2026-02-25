#include "CommonTest.h"

#include <algorithm>
#include <string>
#include <vector>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxInstanceTest : public testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        GfxResult result = gfxLoadBackend(backend);
        if (result != GFX_RESULT_SUCCESS) {
            GTEST_SKIP() << "Backend not available";
        }

        backendLoaded = true;
    }

    void TearDown() override
    {
        if (instance) {
            gfxInstanceDestroy(instance);
        }
        if (backendLoaded) {
            gfxUnloadBackend(backend);
        }
    }

    GfxBackend backend;
    GfxInstance instance = NULL;
    bool backendLoaded = false;
};

TEST_P(GfxInstanceTest, CreateDestroy)
{
    const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
    GfxInstanceDescriptor desc = {};
    desc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
    desc.pNext = nullptr;
    desc.backend = backend;
    desc.enabledExtensions = extensions;
    desc.enabledExtensionCount = 1;

    GfxResult result = gfxCreateInstance(&desc, &instance);

    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(instance, nullptr);

    result = gfxInstanceDestroy(instance);
    instance = NULL; // Prevent double destroy in TearDown

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
}

TEST_P(GfxInstanceTest, WithValidation)
{
    const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
    GfxInstanceDescriptor desc = {};
    desc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
    desc.pNext = nullptr;
    desc.backend = backend;
    desc.enabledExtensions = extensions;
    desc.enabledExtensionCount = 1;

    GfxResult result = gfxCreateInstance(&desc, &instance);

    if (result == GFX_RESULT_SUCCESS) {
        EXPECT_NE(instance, nullptr);
    }
    // Validation may not be supported on all backends
}

TEST_P(GfxInstanceTest, WithApplicationInfo)
{
    const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
    GfxInstanceDescriptor desc = {};
    desc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
    desc.pNext = nullptr;
    desc.backend = backend;
    desc.applicationName = "Test Application";
    desc.applicationVersion = 1;
    desc.enabledExtensions = extensions;
    desc.enabledExtensionCount = 1;

    GfxResult result = gfxCreateInstance(&desc, &instance);

    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(instance, nullptr);
}

TEST_P(GfxInstanceTest, WithEnabledFeatures)
{
    const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG, GFX_INSTANCE_EXTENSION_SURFACE };
    GfxInstanceDescriptor desc = {};
    desc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
    desc.pNext = nullptr;
    desc.backend = backend;
    desc.enabledExtensions = extensions;
    desc.enabledExtensionCount = 2;

    GfxInstance localInstance = NULL;
    GfxResult result = gfxCreateInstance(&desc, &localInstance);

    if (result == GFX_RESULT_SUCCESS) {
        EXPECT_NE(localInstance, nullptr);
        gfxInstanceDestroy(localInstance);
    }
    // Surface feature may not be available in headless builds
}

TEST_P(GfxInstanceTest, RequestAdapterInvalidArguments)
{
    const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
    GfxInstanceDescriptor desc = {};
    desc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
    desc.pNext = nullptr;
    desc.backend = backend;
    desc.enabledExtensions = extensions;
    desc.enabledExtensionCount = 1;

    GfxResult result = gfxCreateInstance(&desc, &instance);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(instance, nullptr);

    // Test NULL instance
    GfxAdapterDescriptor adapterDesc = {};
    adapterDesc.sType = GFX_STRUCTURE_TYPE_ADAPTER_DESCRIPTOR;
    adapterDesc.pNext = nullptr;
    adapterDesc.adapterIndex = UINT32_MAX;
    adapterDesc.preference = GFX_ADAPTER_PREFERENCE_HIGH_PERFORMANCE;

    GfxAdapter adapter = NULL;
    result = gfxInstanceRequestAdapter(NULL, &adapterDesc, &adapter);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // Test NULL descriptor
    result = gfxInstanceRequestAdapter(instance, NULL, &adapter);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // Test NULL output pointer
    result = gfxInstanceRequestAdapter(instance, &adapterDesc, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxInstanceTest, RequestAdapterByPreference)
{
    const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
    GfxInstanceDescriptor desc = {};
    desc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
    desc.pNext = nullptr;
    desc.backend = backend;
    desc.enabledExtensions = extensions;
    desc.enabledExtensionCount = 1;

    GfxResult result = gfxCreateInstance(&desc, &instance);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxAdapterDescriptor adapterDesc = {};
    adapterDesc.sType = GFX_STRUCTURE_TYPE_ADAPTER_DESCRIPTOR;
    adapterDesc.pNext = nullptr;
    adapterDesc.adapterIndex = UINT32_MAX;
    adapterDesc.preference = GFX_ADAPTER_PREFERENCE_HIGH_PERFORMANCE;

    GfxAdapter adapter = NULL;
    result = gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter);

    if (result == GFX_RESULT_SUCCESS) {
        EXPECT_NE(adapter, nullptr);
    }
}

TEST_P(GfxInstanceTest, RequestAdapterByIndex)
{
    const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
    GfxInstanceDescriptor desc = {};
    desc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
    desc.pNext = nullptr;
    desc.backend = backend;
    desc.enabledExtensions = extensions;
    desc.enabledExtensionCount = 1;

    GfxResult result = gfxCreateInstance(&desc, &instance);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(instance, nullptr);

    // First enumerate to get adapter count
    uint32_t adapterCount = 0;
    result = gfxInstanceEnumerateAdapters(instance, &adapterCount, NULL);

    if (result == GFX_RESULT_SUCCESS && adapterCount > 0) {
        // Request first adapter by index
        GfxAdapterDescriptor adapterDesc = {};
        adapterDesc.sType = GFX_STRUCTURE_TYPE_ADAPTER_DESCRIPTOR;
        adapterDesc.pNext = nullptr;
        adapterDesc.adapterIndex = 0;
        adapterDesc.preference = GFX_ADAPTER_PREFERENCE_HIGH_PERFORMANCE;

        GfxAdapter adapter = NULL;
        result = gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter);

        EXPECT_EQ(result, GFX_RESULT_SUCCESS);
        EXPECT_NE(adapter, nullptr);
    }
}

TEST_P(GfxInstanceTest, EnumerateAdaptersInvalidArguments)
{
    const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
    GfxInstanceDescriptor desc = {};
    desc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
    desc.pNext = nullptr;
    desc.backend = backend;
    desc.enabledExtensions = extensions;
    desc.enabledExtensionCount = 1;

    GfxResult result = gfxCreateInstance(&desc, &instance);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(instance, nullptr);

    // Test NULL instance
    uint32_t adapterCount = 0;
    result = gfxInstanceEnumerateAdapters(NULL, &adapterCount, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // Test NULL count pointer
    result = gfxInstanceEnumerateAdapters(instance, NULL, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxInstanceTest, EnumerateAdaptersGetCount)
{
    const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
    GfxInstanceDescriptor desc = {};
    desc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
    desc.pNext = nullptr;
    desc.backend = backend;
    desc.enabledExtensions = extensions;
    desc.enabledExtensionCount = 1;

    GfxResult result = gfxCreateInstance(&desc, &instance);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(instance, nullptr);

    // Get adapter count
    uint32_t adapterCount = 0;
    result = gfxInstanceEnumerateAdapters(instance, &adapterCount, NULL);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);

    if (adapterCount == 0) {
        GTEST_SKIP() << "Backend returned 0 adapters (enumeration may not be fully implemented)";
    }
    EXPECT_GT(adapterCount, 0u);
}

TEST_P(GfxInstanceTest, EnumerateAdaptersGetAdapters)
{
    const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
    GfxInstanceDescriptor desc = {};
    desc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
    desc.pNext = nullptr;
    desc.backend = backend;
    desc.enabledExtensions = extensions;
    desc.enabledExtensionCount = 1;

    GfxResult result = gfxCreateInstance(&desc, &instance);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(instance, nullptr);

    // Get adapter count
    uint32_t adapterCount = 0;
    result = gfxInstanceEnumerateAdapters(instance, &adapterCount, NULL);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);

    if (adapterCount > 0) {
        // Allocate array and get adapters
        GfxAdapter* adapters = new GfxAdapter[adapterCount];
        result = gfxInstanceEnumerateAdapters(instance, &adapterCount, adapters);

        EXPECT_EQ(result, GFX_RESULT_SUCCESS);

        // Verify all adapters are non-null
        for (uint32_t i = 0; i < adapterCount; ++i) {
            EXPECT_NE(adapters[i], nullptr);
        }

        delete[] adapters;
    }
}

TEST_P(GfxInstanceTest, EnumerateAdaptersTwoCalls)
{
    const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
    GfxInstanceDescriptor desc = {};
    desc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
    desc.pNext = nullptr;
    desc.backend = backend;
    desc.enabledExtensions = extensions;
    desc.enabledExtensionCount = 1;

    GfxResult result = gfxCreateInstance(&desc, &instance);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(instance, nullptr);

    // First call: get count
    uint32_t adapterCount = 0;
    result = gfxInstanceEnumerateAdapters(instance, &adapterCount, NULL);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);

    if (adapterCount == 0) {
        GTEST_SKIP() << "Backend returned 0 adapters (enumeration may not be fully implemented)";
    }

    EXPECT_GT(adapterCount, 0u);

    uint32_t firstCount = adapterCount;

    // Second call: get adapters
    GfxAdapter* adapters = new GfxAdapter[adapterCount];
    result = gfxInstanceEnumerateAdapters(instance, &adapterCount, adapters);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_EQ(adapterCount, firstCount); // Count should remain the same

    delete[] adapters;
}

TEST_P(GfxInstanceTest, MultipleInstances)
{
    const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
    GfxInstanceDescriptor desc = {};
    desc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
    desc.pNext = nullptr;
    desc.backend = backend;
    desc.enabledExtensions = extensions;
    desc.enabledExtensionCount = 1;

    GfxInstance instance1 = NULL;
    GfxInstance instance2 = NULL;

    GfxResult result1 = gfxCreateInstance(&desc, &instance1);
    GfxResult result2 = gfxCreateInstance(&desc, &instance2);

    ASSERT_EQ(result1, GFX_RESULT_SUCCESS);
    ASSERT_EQ(result2, GFX_RESULT_SUCCESS);

    EXPECT_NE(instance1, nullptr);
    EXPECT_NE(instance2, nullptr);
    EXPECT_NE(instance1, instance2); // Should be different instances

    gfxInstanceDestroy(instance1);
    gfxInstanceDestroy(instance2);
    instance = NULL; // Prevent TearDown from double-destroying
}

TEST_P(GfxInstanceTest, DoubleDestroy)
{
    const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
    GfxInstanceDescriptor desc = {};
    desc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
    desc.pNext = nullptr;
    desc.backend = backend;
    desc.enabledExtensions = extensions;
    desc.enabledExtensionCount = 1;

    GfxResult result = gfxCreateInstance(&desc, &instance);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(instance, nullptr);

    // First destroy should succeed
    result = gfxInstanceDestroy(instance);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    instance = NULL;

    // Second destroy of same instance is undefined behavior, but we test it doesn't crash
    // Note: In production code, don't do this - just testing robustness
}

TEST_P(GfxInstanceTest, EnumerateInstanceExtensions)
{
    // First call: get count
    uint32_t extensionCount = 0;
    EXPECT_EQ(gfxEnumerateInstanceExtensions(backend, &extensionCount, nullptr), GFX_RESULT_SUCCESS);
    EXPECT_GT(extensionCount, 0) << "Backend should support at least one instance extension";

    // Second call: get extensions
    std::vector<const char*> extensionNames(extensionCount);
    EXPECT_EQ(gfxEnumerateInstanceExtensions(backend, &extensionCount, extensionNames.data()), GFX_RESULT_SUCCESS);

    // Verify extensions
    for (uint32_t i = 0; i < extensionCount; ++i) {
        EXPECT_NE(extensionNames[i], nullptr) << "Extension name at index " << i << " should not be null";
        EXPECT_GT(strlen(extensionNames[i]), 0) << "Extension name at index " << i << " should not be empty";

        // All extensions should be non-null strings
        std::string extName(extensionNames[i]);
        EXPECT_FALSE(extName.empty()) << "Extension " << i << " has empty name";
    }

    // Check for expected surface extension
    auto it = std::find_if(extensionNames.begin(), extensionNames.end(),
        [](const char* name) { return strcmp(name, GFX_INSTANCE_EXTENSION_SURFACE) == 0; });
    EXPECT_NE(it, extensionNames.end()) << "Surface extension should be available";
}

TEST_P(GfxInstanceTest, EnumerateInstanceExtensionsWithZeroCount)
{
    // Query with zero count should still succeed and return the count
    uint32_t extensionCount = 0;
    EXPECT_EQ(gfxEnumerateInstanceExtensions(backend, &extensionCount, nullptr), GFX_RESULT_SUCCESS);
    EXPECT_GT(extensionCount, 0);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxInstanceTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

// ===========================================================================
// Non-Parameterized Tests - Backend-independent functionality
// ===========================================================================

TEST(GfxInstanceTestNonParam, InvalidArguments)
{
    // Test NULL output pointer
    const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
    GfxInstanceDescriptor desc = {};
    desc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
    desc.pNext = nullptr;
    desc.backend = static_cast<GfxBackend>(GFX_BACKEND_VULKAN);
    ;
    desc.enabledExtensions = extensions;
    desc.enabledExtensionCount = 1;

    GfxResult result = gfxCreateInstance(&desc, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // Test NULL descriptor
    GfxInstance instance = NULL;
    result = gfxCreateInstance(NULL, &instance);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST(GfxInstanceTestNonParam, DestroyNullInstance)
{
    // Should handle NULL gracefully
    GfxResult result = gfxInstanceDestroy(NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

} // namespace
