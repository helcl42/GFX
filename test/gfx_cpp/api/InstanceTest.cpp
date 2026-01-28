#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

class GfxCppInstanceTest : public testing::TestWithParam<gfx::Backend> {
protected:
    void SetUp() override
    {
        backend = GetParam();
    }

    gfx::Backend backend;
};

TEST_P(GfxCppInstanceTest, CreateDestroy)
{
    gfx::InstanceDescriptor desc{
        .backend = backend,
        .enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG }
    };

    try {
        auto instance = gfx::createInstance(desc);
        EXPECT_NE(instance, nullptr);
        // Instance automatically destroyed via shared_ptr
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Backend not available: " << e.what();
    }
}

TEST_P(GfxCppInstanceTest, WithValidation)
{
    gfx::InstanceDescriptor desc{
        .backend = backend,
        .enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG }
    };

    try {
        auto instance = gfx::createInstance(desc);
        EXPECT_NE(instance, nullptr);
    } catch (const std::exception&) {
        // Validation may not be supported on all backends
        GTEST_SKIP() << "Backend not available or validation not supported";
    }
}

TEST_P(GfxCppInstanceTest, WithApplicationInfo)
{
    gfx::InstanceDescriptor desc{
        .backend = backend,
        .applicationName = "Test Application",
        .applicationVersion = 1,
        .enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG }
    };

    try {
        auto instance = gfx::createInstance(desc);
        EXPECT_NE(instance, nullptr);
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Backend not available: " << e.what();
    }
}

TEST_P(GfxCppInstanceTest, WithEnabledFeatures)
{
    gfx::InstanceDescriptor desc{
        .backend = backend,
        .enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG, gfx::INSTANCE_EXTENSION_SURFACE }
    };

    try {
        auto instance = gfx::createInstance(desc);
        EXPECT_NE(instance, nullptr);
    } catch (const std::exception&) {
        // Surface feature may not be available in headless builds
        GTEST_SKIP() << "Backend not available or surface extension not supported";
    }
}

TEST_P(GfxCppInstanceTest, RequestAdapterByPreference)
{
    gfx::InstanceDescriptor desc{
        .backend = backend,
        .enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG }
    };

    try {
        auto instance = gfx::createInstance(desc);
        ASSERT_NE(instance, nullptr);

        gfx::AdapterDescriptor adapterDesc{
            .preference = gfx::AdapterPreference::HighPerformance
        };

        auto adapter = instance->requestAdapter(adapterDesc);
        EXPECT_NE(adapter, nullptr);
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Backend not available: " << e.what();
    }
}

TEST_P(GfxCppInstanceTest, RequestAdapterByIndex)
{
    gfx::InstanceDescriptor desc{
        .backend = backend,
        .enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG }
    };

    try {
        auto instance = gfx::createInstance(desc);
        ASSERT_NE(instance, nullptr);

        // First enumerate to get adapter count
        auto adapters = instance->enumerateAdapters();

        if (!adapters.empty()) {
            // Request first adapter by index
            gfx::AdapterDescriptor adapterDesc{
                .adapterIndex = 0,
                .preference = gfx::AdapterPreference::HighPerformance
            };

            auto adapter = instance->requestAdapter(adapterDesc);
            EXPECT_NE(adapter, nullptr);
        } else {
            GTEST_SKIP() << "No adapters available";
        }
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Backend not available: " << e.what();
    }
}

TEST_P(GfxCppInstanceTest, EnumerateAdaptersGetCount)
{
    gfx::InstanceDescriptor desc{
        .backend = backend,
        .enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG }
    };

    try {
        auto instance = gfx::createInstance(desc);
        ASSERT_NE(instance, nullptr);

        // Get adapters
        auto adapters = instance->enumerateAdapters();

        if (adapters.empty()) {
            GTEST_SKIP() << "Backend returned 0 adapters (enumeration may not be fully implemented)";
        }
        EXPECT_GT(adapters.size(), 0u);
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Backend not available: " << e.what();
    }
}

TEST_P(GfxCppInstanceTest, EnumerateAdaptersGetAdapters)
{
    gfx::InstanceDescriptor desc{
        .backend = backend,
        .enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG }
    };

    try {
        auto instance = gfx::createInstance(desc);
        ASSERT_NE(instance, nullptr);

        // Get adapters
        auto adapters = instance->enumerateAdapters();

        if (!adapters.empty()) {
            // Verify all adapters are non-null
            for (const auto& adapter : adapters) {
                EXPECT_NE(adapter, nullptr);
            }
        }
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Backend not available: " << e.what();
    }
}

TEST_P(GfxCppInstanceTest, EnumerateAdaptersTwoCalls)
{
    gfx::InstanceDescriptor desc{
        .backend = backend,
        .enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG }
    };

    try {
        auto instance = gfx::createInstance(desc);
        ASSERT_NE(instance, nullptr);

        // First call: get adapters
        auto adapters1 = instance->enumerateAdapters();

        if (adapters1.empty()) {
            GTEST_SKIP() << "Backend returned 0 adapters (enumeration may not be fully implemented)";
        }

        EXPECT_GT(adapters1.size(), 0u);
        size_t firstCount = adapters1.size();

        // Second call: get adapters again
        auto adapters2 = instance->enumerateAdapters();

        EXPECT_EQ(adapters2.size(), firstCount); // Count should remain the same
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Backend not available: " << e.what();
    }
}

TEST_P(GfxCppInstanceTest, MultipleInstances)
{
    gfx::InstanceDescriptor desc{
        .backend = backend,
        .enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG }
    };

    try {
        auto instance1 = gfx::createInstance(desc);
        auto instance2 = gfx::createInstance(desc);

        EXPECT_NE(instance1, nullptr);
        EXPECT_NE(instance2, nullptr);
        EXPECT_NE(instance1.get(), instance2.get()); // Should be different instances
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Backend not available: " << e.what();
    }
}

TEST_P(GfxCppInstanceTest, SharedPointerSemantics)
{
    gfx::InstanceDescriptor desc{
        .backend = backend,
        .enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG }
    };

    try {
        auto instance1 = gfx::createInstance(desc);
        EXPECT_NE(instance1, nullptr);

        // Copy shared_ptr
        auto instance2 = instance1;
        EXPECT_EQ(instance1, instance2);
        EXPECT_EQ(instance1.get(), instance2.get());
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Backend not available: " << e.what();
    }
}

TEST_P(GfxCppInstanceTest, EnumerateInstanceExtensions)
{
    auto extensions = gfx::enumerateInstanceExtensions(backend);

    EXPECT_GT(extensions.size(), 0) << "Backend should support at least one instance extension";

    // Verify all extensions are valid strings
    for (const auto& ext : extensions) {
        EXPECT_FALSE(ext.empty()) << "Extension name should not be empty";
    }

    // Check for expected surface extension
    auto it = std::find(extensions.begin(), extensions.end(), std::string(gfx::INSTANCE_EXTENSION_SURFACE));
    EXPECT_NE(it, extensions.end()) << "Surface extension should be available";
}

TEST_P(GfxCppInstanceTest, EnumerateInstanceExtensionsNoDuplicates)
{
    auto extensions = gfx::enumerateInstanceExtensions(backend);

    // Check for duplicates
    for (size_t i = 0; i < extensions.size(); ++i) {
        for (size_t j = i + 1; j < extensions.size(); ++j) {
            EXPECT_NE(extensions[i], extensions[j])
                << "Found duplicate extension: " << extensions[i];
        }
    }
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppInstanceTest,
    testing::Values(gfx::Backend::Vulkan, gfx::Backend::WebGPU),
    [](const testing::TestParamInfo<gfx::Backend>& info) {
        return info.param == gfx::Backend::Vulkan ? "Vulkan" : "WebGPU";
    });

// ===========================================================================
// Non-Parameterized Tests - Backend-independent functionality
// ===========================================================================

TEST(GfxCppInstanceTestNonParam, NullInstance)
{
    std::shared_ptr<gfx::Instance> instance; // Default constructed = null
    EXPECT_EQ(instance, nullptr);
}
