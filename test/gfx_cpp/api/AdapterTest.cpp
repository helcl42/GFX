#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

class GfxCppAdapterTest : public testing::TestWithParam<gfx::Backend> {
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
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up adapter: " << e.what();
        }
    }

    gfx::Backend backend;
    std::shared_ptr<gfx::Instance> instance;
    std::shared_ptr<gfx::Adapter> adapter;
};

TEST_P(GfxCppAdapterTest, GetInfo)
{
    ASSERT_NE(adapter, nullptr);

    auto info = adapter->getInfo();

    // Verify we got some valid information
    EXPECT_FALSE(info.name.empty()) << "Adapter should have a name";

    // Vendor ID should be non-zero for real hardware
    // (might be 0 for software renderers, so just check it's set)
    EXPECT_TRUE(info.vendorID >= 0);

    // Adapter type should be valid
    EXPECT_GE(info.adapterType, gfx::AdapterType::DiscreteGPU);
    EXPECT_LE(info.adapterType, gfx::AdapterType::CPU);
}

TEST_P(GfxCppAdapterTest, GetLimits)
{
    ASSERT_NE(adapter, nullptr);

    auto limits = adapter->getLimits();

    // Verify reasonable limits
    EXPECT_GT(limits.maxBufferSize, 0u);
    EXPECT_GT(limits.maxTextureDimension1D, 0u);
    EXPECT_GT(limits.maxTextureDimension2D, 0u);
    EXPECT_GT(limits.maxTextureDimension3D, 0u);
    EXPECT_GT(limits.maxTextureArrayLayers, 0u);
    EXPECT_GT(limits.maxUniformBufferBindingSize, 0u);
    EXPECT_GT(limits.maxStorageBufferBindingSize, 0u);

    // These should be at least the WebGPU minimums
    EXPECT_GE(limits.maxTextureDimension2D, 8192u);
}

TEST_P(GfxCppAdapterTest, EnumerateQueueFamilies)
{
    ASSERT_NE(adapter, nullptr);

    auto queueFamilies = adapter->enumerateQueueFamilies();

    EXPECT_GT(queueFamilies.size(), 0u) << "Adapter should have at least one queue family";

    // Verify at least one queue family supports graphics
    bool hasGraphics = false;
    for (const auto& family : queueFamilies) {
        EXPECT_GT(family.queueCount, 0u);
        if ((family.flags & gfx::QueueFlags::Graphics) != gfx::QueueFlags::None) {
            hasGraphics = true;
        }
    }
    EXPECT_TRUE(hasGraphics) << "At least one queue family should support graphics";
}

TEST_P(GfxCppAdapterTest, CreateDevice)
{
    ASSERT_NE(adapter, nullptr);

    gfx::DeviceDescriptor desc{
        .label = "Test Device"
    };

    auto device = adapter->createDevice(desc);
    EXPECT_NE(device, nullptr);
    // Device automatically destroyed via shared_ptr
}

TEST_P(GfxCppAdapterTest, InfoConsistency)
{
    ASSERT_NE(adapter, nullptr);

    // Get info multiple times and verify consistency
    auto info1 = adapter->getInfo();
    auto info2 = adapter->getInfo();

    // Info should be consistent
    EXPECT_EQ(info1.name, info2.name);
    EXPECT_EQ(info1.vendorID, info2.vendorID);
    EXPECT_EQ(info1.deviceID, info2.deviceID);
    EXPECT_EQ(info1.adapterType, info2.adapterType);
}

TEST_P(GfxCppAdapterTest, LimitsConsistency)
{
    ASSERT_NE(adapter, nullptr);

    // Get limits multiple times and verify consistency
    auto limits1 = adapter->getLimits();
    auto limits2 = adapter->getLimits();

    // Limits should be consistent
    EXPECT_EQ(limits1.maxBufferSize, limits2.maxBufferSize);
    EXPECT_EQ(limits1.maxTextureDimension2D, limits2.maxTextureDimension2D);
    EXPECT_EQ(limits1.maxTextureArrayLayers, limits2.maxTextureArrayLayers);
    EXPECT_EQ(limits1.maxUniformBufferBindingSize, limits2.maxUniformBufferBindingSize);
}

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppAdapterTest,
    testing::Values(gfx::Backend::Vulkan, gfx::Backend::WebGPU),
    [](const testing::TestParamInfo<gfx::Backend>& info) {
        return info.param == gfx::Backend::Vulkan ? "Vulkan" : "WebGPU";
    });
