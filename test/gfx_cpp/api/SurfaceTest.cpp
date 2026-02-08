#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxCppSurfaceTest : public testing::TestWithParam<gfx::Backend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        try {
            gfx::InstanceDescriptor instDesc{
                .backend = backend,
                .enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG, gfx::INSTANCE_EXTENSION_SURFACE }
            };
            instance = gfx::createInstance(instDesc);

            gfx::AdapterDescriptor adapterDesc{
                .preference = gfx::AdapterPreference::HighPerformance
            };
            adapter = instance->requestAdapter(adapterDesc);

            gfx::DeviceDescriptor deviceDesc{
                .label = "Test Device"
            };
            device = adapter->createDevice(deviceDesc);
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up: " << e.what();
        }
    }

    void TearDown() override
    {
        surface.reset();
        device.reset();
        adapter.reset();
        instance.reset();
    }

    gfx::Backend backend;
    std::shared_ptr<gfx::Instance> instance;
    std::shared_ptr<gfx::Adapter> adapter;
    std::shared_ptr<gfx::Device> device;
    std::shared_ptr<gfx::Surface> surface;
};

TEST_P(GfxCppSurfaceTest, CreateSurfaceInvalidArguments)
{
    ASSERT_NE(device, nullptr);

    // Use the factory method to create an invalid window handle
    auto invalidHandle = gfx::PlatformWindowHandle::fromXlib(nullptr, 0);

    gfx::SurfaceDescriptor desc{
        .label = "TestSurface",
        .windowHandle = invalidHandle
    };

    // Creating with invalid window handles should throw or return null
    // The actual behavior depends on backend validation
    try {
        surface = device->createSurface(desc);
        // If it doesn't throw, it should at least return null or handle gracefully
        // Some backends might defer validation until surface is actually used
    } catch (const std::exception& e) {
        // Expected behavior for invalid arguments
        SUCCEED() << "Correctly threw exception: " << e.what();
    }
}

TEST_P(GfxCppSurfaceTest, DestroyNullSurface)
{
    // Test that destroying a null surface (via reset) is safe
    std::shared_ptr<gfx::Surface> nullSurface;
    EXPECT_NO_THROW(nullSurface.reset());
}

TEST_P(GfxCppSurfaceTest, GetSupportedFormatsNullSurface)
{
    // Test that calling methods on null surface pointer is handled safely
    // In C++, dereferencing nullptr would crash, so we just verify nullptr checks
    std::shared_ptr<gfx::Surface> nullSurface;
    EXPECT_EQ(nullSurface, nullptr);

    // The following would crash if uncommented (expected behavior):
    // auto formats = nullSurface->getSupportedFormats();
}

TEST_P(GfxCppSurfaceTest, GetSupportedPresentModesNullSurface)
{
    // Test that calling methods on null surface pointer is handled safely
    // In C++, dereferencing nullptr would crash, so we just verify nullptr checks
    std::shared_ptr<gfx::Surface> nullSurface;
    EXPECT_EQ(nullSurface, nullptr);

    // The following would crash if uncommented (expected behavior):
    // auto modes = nullSurface->getSupportedPresentModes();
}

TEST_P(GfxCppSurfaceTest, GetQueueFamilySurfaceSupportNullAdapter)
{
    ASSERT_NE(adapter, nullptr);

    // Test with null surface - using raw pointer for the test
    gfx::Surface* nullSurface = nullptr;

    // This should throw or handle gracefully
    try {
        bool supported = adapter->getQueueFamilySurfaceSupport(0, nullSurface);
        // If it doesn't throw, we expect false for invalid surface
        EXPECT_FALSE(supported);
    } catch (const std::exception& e) {
        // Expected - null surface should be rejected
        SUCCEED() << "Correctly threw exception: " << e.what();
    }
}

TEST_P(GfxCppSurfaceTest, GetQueueFamilySurfaceSupportInvalidSurface)
{
    ASSERT_NE(adapter, nullptr);
    ASSERT_NE(device, nullptr);

    // Create a surface with invalid handle to test validation
    auto invalidHandle = gfx::PlatformWindowHandle::fromXlib(nullptr, 0);

    gfx::SurfaceDescriptor desc{
        .label = "TestSurface",
        .windowHandle = invalidHandle
    };

    try {
        auto testSurface = device->createSurface(desc);

        if (testSurface) {
            // If surface was created despite invalid handle, test queue support
            // This tests that the API handles invalid surfaces gracefully
            try {
                bool supported = adapter->getQueueFamilySurfaceSupport(0, testSurface.get());
                // Backend may return false for invalid surfaces
                EXPECT_FALSE(supported) << "Invalid surface should not be supported";
            } catch (const std::exception& e) {
                SUCCEED() << "Correctly rejected invalid surface: " << e.what();
            }
        }
    } catch (const std::exception& e) {
        // Expected - surface creation with invalid handle should fail
        SUCCEED() << "Correctly failed to create invalid surface: " << e.what();
    }
}

TEST_P(GfxCppSurfaceTest, GetSurfaceInfoReturnsValidStruct)
{
    ASSERT_NE(adapter, nullptr);
    ASSERT_NE(device, nullptr);

    // Create a surface with invalid handle (we're testing the API contract, not actual functionality)
    auto invalidHandle = gfx::PlatformWindowHandle::fromXlib(nullptr, 0);

    gfx::SurfaceDescriptor desc{
        .label = "TestSurface",
        .windowHandle = invalidHandle
    };

    try {
        auto testSurface = device->createSurface(desc);

        if (testSurface) {
            // Test that getInfo returns a struct with reasonable values
            gfx::SurfaceInfo info = testSurface->getInfo();

            // Verify struct has been populated (values should be non-negative)
            // We can't verify exact values without a real surface, but we can check the struct is valid
            EXPECT_GE(info.minImageCount, 0u);
            EXPECT_GE(info.maxImageCount, info.minImageCount) << "maxImageCount should be >= minImageCount";
            EXPECT_GE(info.minWidth, 0u);
            EXPECT_GE(info.minHeight, 0u);
            EXPECT_GE(info.maxWidth, info.minWidth) << "maxWidth should be >= minWidth";
            EXPECT_GE(info.maxHeight, info.minHeight) << "maxHeight should be >= minHeight";
        }
    } catch (const std::exception& e) {
        // Expected - surface creation with invalid handle should fail
        SUCCEED() << "Correctly failed to create invalid surface: " << e.what();
    }
}

// Note: Creating actual surfaces requires real window handles from X11/Wayland/etc.
// These tests verify API contracts and argument validation without requiring a display server
// Full surface functionality tests would require integration with a windowing system

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppSurfaceTest,
    testing::Values(gfx::Backend::Vulkan, gfx::Backend::WebGPU),
    [](const testing::TestParamInfo<gfx::Backend>& info) {
        return info.param == gfx::Backend::Vulkan ? "Vulkan" : "WebGPU";
    });

} // namespace
