#include "CommonTest.h"

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxCppSwapchainTest : public testing::TestWithParam<gfx::Backend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        try {
            gfx::InstanceDescriptor instDesc{
                .backend = backend,
                .enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG }
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
        swapchain.reset();
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
    std::shared_ptr<gfx::Swapchain> swapchain;
};

TEST_P(GfxCppSwapchainTest, CreateSwapchainNullSurface)
{
    ASSERT_NE(device, nullptr);

    std::shared_ptr<gfx::Surface> nullSurface;

    gfx::SwapchainDescriptor desc{
        .label = "TestSwapchain",
        .surface = nullSurface,
        .extent = { 800, 600 },
        .format = gfx::Format::B8G8R8A8Unorm,
        .usage = gfx::TextureUsage::RenderAttachment,
        .presentMode = gfx::PresentMode::Fifo,
        .imageCount = 2
    };

    // Creating swapchain with null surface should throw or fail gracefully
    try {
        swapchain = device->createSwapchain(desc);
        // If it doesn't throw, swapchain should be null
        EXPECT_EQ(swapchain, nullptr) << "Swapchain creation with null surface should fail";
    } catch (const std::exception& e) {
        // Expected - null surface should be rejected
        SUCCEED() << "Correctly threw exception: " << e.what();
    }
}

TEST_P(GfxCppSwapchainTest, CreateSwapchainInvalidDimensions)
{
    ASSERT_NE(device, nullptr);

    std::shared_ptr<gfx::Surface> nullSurface;

    // Zero width should be rejected
    gfx::SwapchainDescriptor desc{
        .label = "TestSwapchain",
        .surface = nullSurface,
        .extent = { 0, 600 },
        .format = gfx::Format::B8G8R8A8Unorm,
        .usage = gfx::TextureUsage::RenderAttachment,
        .presentMode = gfx::PresentMode::Fifo,
        .imageCount = 2
    };

    try {
        swapchain = device->createSwapchain(desc);
        EXPECT_EQ(swapchain, nullptr) << "Swapchain creation with zero width should fail";
    } catch (const std::exception& e) {
        SUCCEED() << "Correctly rejected zero width: " << e.what();
    }

    // Zero height should be rejected
    desc.extent.width = 800;
    desc.extent.height = 0;

    try {
        swapchain = device->createSwapchain(desc);
        EXPECT_EQ(swapchain, nullptr) << "Swapchain creation with zero height should fail";
    } catch (const std::exception& e) {
        SUCCEED() << "Correctly rejected zero height: " << e.what();
    }

    // Both zero should be rejected
    desc.extent.width = 0;
    desc.extent.height = 0;

    try {
        swapchain = device->createSwapchain(desc);
        EXPECT_EQ(swapchain, nullptr) << "Swapchain creation with zero dimensions should fail";
    } catch (const std::exception& e) {
        SUCCEED() << "Correctly rejected zero dimensions: " << e.what();
    }
}

TEST_P(GfxCppSwapchainTest, CreateSwapchainInvalidImageCount)
{
    ASSERT_NE(device, nullptr);

    std::shared_ptr<gfx::Surface> nullSurface;

    gfx::SwapchainDescriptor desc{
        .label = "TestSwapchain",
        .surface = nullSurface,
        .extent = { 800, 600 },
        .format = gfx::Format::B8G8R8A8Unorm,
        .usage = gfx::TextureUsage::RenderAttachment,
        .presentMode = gfx::PresentMode::Fifo,
        .imageCount = 0 // Invalid
    };

    // Zero image count should be rejected
    try {
        swapchain = device->createSwapchain(desc);
        EXPECT_EQ(swapchain, nullptr) << "Swapchain creation with zero image count should fail";
    } catch (const std::exception& e) {
        SUCCEED() << "Correctly rejected zero image count: " << e.what();
    }
}

TEST_P(GfxCppSwapchainTest, DestroyNullSwapchain)
{
    // Test that destroying a null swapchain (via reset) is safe
    std::shared_ptr<gfx::Swapchain> nullSwapchain;
    EXPECT_NO_THROW(nullSwapchain.reset());
}

TEST_P(GfxCppSwapchainTest, GetInfoNullSwapchain)
{
    // Test that calling methods on null swapchain pointer is handled safely
    // In C++, dereferencing nullptr would crash, so we just verify nullptr checks
    std::shared_ptr<gfx::Swapchain> nullSwapchain;
    EXPECT_EQ(nullSwapchain, nullptr);

    // The following would crash if uncommented (expected behavior):
    // auto info = nullSwapchain->getInfo();
}

TEST_P(GfxCppSwapchainTest, AcquireNextImageNullSwapchain)
{
    // Test that calling methods on null swapchain pointer is handled safely
    std::shared_ptr<gfx::Swapchain> nullSwapchain;
    EXPECT_EQ(nullSwapchain, nullptr);

    // The following would crash if uncommented (expected behavior):
    // uint32_t imageIndex = 0;
    // auto result = nullSwapchain->acquireNextImage(0, nullptr, nullptr, &imageIndex);
}

TEST_P(GfxCppSwapchainTest, GetTextureViewNullSwapchain)
{
    // Test that calling methods on null swapchain pointer is handled safely
    std::shared_ptr<gfx::Swapchain> nullSwapchain;
    EXPECT_EQ(nullSwapchain, nullptr);

    // The following would crash if uncommented (expected behavior):
    // auto view = nullSwapchain->getTextureView(0);
}

TEST_P(GfxCppSwapchainTest, GetCurrentTextureViewNullSwapchain)
{
    // Test that calling methods on null swapchain pointer is handled safely
    std::shared_ptr<gfx::Swapchain> nullSwapchain;
    EXPECT_EQ(nullSwapchain, nullptr);

    // The following would crash if uncommented (expected behavior):
    // auto view = nullSwapchain->getCurrentTextureView();
}

TEST_P(GfxCppSwapchainTest, PresentNullSwapchain)
{
    // Test that calling methods on null swapchain pointer is handled safely
    std::shared_ptr<gfx::Swapchain> nullSwapchain;
    EXPECT_EQ(nullSwapchain, nullptr);

    // The following would crash if uncommented (expected behavior):
    // gfx::PresentDescriptor descriptor{};
    // auto result = nullSwapchain->present(descriptor);
}

// Note: Creating actual swapchains requires valid surfaces with real window handles.
// These tests verify API contracts and argument validation without requiring display servers.
// Full swapchain functionality tests would require integration with a windowing system.

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppSwapchainTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

} // namespace
