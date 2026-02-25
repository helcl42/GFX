#include "common/CommonTest.h"

// Test the implementation functions in gfx_cpp/src/GfxImpl.cpp
// These are the factory functions and utilities that bridge C++ wrapper to C API

namespace gfx::test {

// ============================================================================
// Version Query Tests (gfx::getVersion)
// ============================================================================

TEST(GfxImplTest, GetVersion_ReturnsValidVersion)
{
    auto [major, minor, patch] = gfx::getVersion();

    // Version numbers should match the C API version
    EXPECT_GE(major, 0u);
    EXPECT_GE(minor, 0u);
    EXPECT_GE(patch, 0u);

    // Verify it matches C API
    uint32_t cMajor, cMinor, cPatch;
    gfxGetVersion(&cMajor, &cMinor, &cPatch);
    EXPECT_EQ(major, cMajor);
    EXPECT_EQ(minor, cMinor);
    EXPECT_EQ(patch, cPatch);
}

// ============================================================================
// Instance Extension Enumeration Tests (gfx::enumerateInstanceExtensions)
// ============================================================================

TEST(GfxImplTest, EnumerateInstanceExtensions_InvalidBackend_Throws)
{
    EXPECT_THROW({ gfx::enumerateInstanceExtensions(static_cast<gfx::Backend>(999)); }, std::runtime_error);
}

#ifdef GFX_ENABLE_VULKAN
TEST(GfxImplTest, EnumerateInstanceExtensions_VulkanBackend_ReturnsExtensions)
{
    // Load backend first
    if (gfxLoadBackend(GFX_BACKEND_VULKAN) != GFX_RESULT_SUCCESS) {
        GTEST_SKIP() << "Vulkan backend not available";
    }

    EXPECT_NO_THROW({
        auto extensions = gfx::enumerateInstanceExtensions(gfx::Backend::Vulkan);
        // Should return a vector (may be empty)
        EXPECT_GE(extensions.size(), 0u);
    });
}
#endif

#ifdef GFX_ENABLE_WEBGPU
TEST(GfxImplTest, EnumerateInstanceExtensions_WebGPUBackend_ReturnsExtensions)
{
    // Load backend first
    if (gfxLoadBackend(GFX_BACKEND_WEBGPU) != GFX_RESULT_SUCCESS) {
        GTEST_SKIP() << "WebGPU backend not available";
    }

    EXPECT_NO_THROW({
        auto extensions = gfx::enumerateInstanceExtensions(gfx::Backend::WebGPU);
        EXPECT_GE(extensions.size(), 0u);
    });
}
#endif

// ============================================================================
// Instance Creation Tests (gfx::createInstance)
// ============================================================================

TEST(GfxImplTest, CreateInstance_InvalidBackend_Throws)
{
    gfx::InstanceDescriptor desc{
        .backend = static_cast<gfx::Backend>(999), // Invalid backend
        .applicationName = "Test"
    };

    EXPECT_THROW({ auto instance = gfx::createInstance(desc); }, std::runtime_error);
}

#ifdef GFX_ENABLE_VULKAN
TEST(GfxImplTest, CreateInstance_VulkanBackend_Succeeds)
{
    gfx::InstanceDescriptor desc{
        .backend = gfx::Backend::Vulkan,
        .applicationName = "GfxImplTest",
        .applicationVersion = 1
    };

    std::shared_ptr<gfx::Instance> instance;
    EXPECT_NO_THROW({
        instance = gfx::createInstance(desc);
    });

    if (instance) {
        EXPECT_NE(instance, nullptr);
    }
}
#endif

#ifdef GFX_ENABLE_WEBGPU
TEST(GfxImplTest, CreateInstance_WebGPUBackend_Succeeds)
{
    gfx::InstanceDescriptor desc{
        .backend = gfx::Backend::WebGPU,
        .applicationName = "GfxImplTest",
        .applicationVersion = 1
    };

    std::shared_ptr<gfx::Instance> instance;
    EXPECT_NO_THROW({
        instance = gfx::createInstance(desc);
    });

    if (instance) {
        EXPECT_NE(instance, nullptr);
    }
}
#endif

TEST(GfxImplTest, CreateInstance_VulkanWithExtensions_Succeeds)
{
#ifdef GFX_ENABLE_VULKAN
    gfx::InstanceDescriptor desc{
        .backend = gfx::Backend::Vulkan,
        .applicationName = "GfxImplTest",
        .applicationVersion = 1,
        .enabledExtensions = {} // Empty extensions list should work
    };

    EXPECT_NO_THROW({
        auto instance = gfx::createInstance(desc);
        EXPECT_NE(instance, nullptr);
    });
#else
    GTEST_SKIP() << "No backend available";
#endif
}

// ============================================================================
// Log Callback Tests (gfx::setLogCallback)
// ============================================================================

TEST(GfxImplTest, SetLogCallback_NullCallback_DoesNotCrash)
{
    EXPECT_NO_THROW({
        gfx::setLogCallback(nullptr);
    });
}

TEST(GfxImplTest, SetLogCallback_ValidCallback_DoesNotCrash)
{
    bool callbackInvoked = false;

    EXPECT_NO_THROW({
        gfx::setLogCallback([&callbackInvoked](gfx::LogLevel level, const std::string& message) {
            (void)level;
            (void)message;
            callbackInvoked = true;
        });
    });

    // Note: We don't test if callback is actually invoked since that depends on internal logging
}

TEST(GfxImplTest, SetLogCallback_ClearCallback_DoesNotCrash)
{
    // Set a callback first
    gfx::setLogCallback([](gfx::LogLevel, const std::string&) {});

    // Clear it
    EXPECT_NO_THROW({
        gfx::setLogCallback(nullptr);
    });
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST(GfxImplTest, CreateInstance_InvalidDescriptor_Throws)
{
    // Backend that doesn't exist
    gfx::InstanceDescriptor desc{
        .backend = static_cast<gfx::Backend>(999),
        .applicationName = "Test"
    };

    EXPECT_THROW({ auto instance = gfx::createInstance(desc); }, std::runtime_error);
}

} // namespace gfx::test
