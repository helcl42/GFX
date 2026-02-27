#include "CommonTest.h"

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxCppBindGroupLayoutTest : public testing::TestWithParam<gfx::Backend> {
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
                .adapterIndex = 0
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

    gfx::Backend backend;
    std::shared_ptr<gfx::Instance> instance;
    std::shared_ptr<gfx::Adapter> adapter;
    std::shared_ptr<gfx::Device> device;
};

// ===========================================================================
// BindGroupLayout Tests
// ===========================================================================

TEST_P(GfxCppBindGroupLayoutTest, CreateBindGroupLayoutWithValidDescriptor)
{
    ASSERT_NE(device, nullptr);

    gfx::BindGroupLayoutEntry entry{
        .binding = 0,
        .visibility = gfx::ShaderStage::Vertex,
        .resource = gfx::BindGroupLayoutEntry::BufferBinding{
            .hasDynamicOffset = false,
            .minBindingSize = 0 }
    };

    gfx::BindGroupLayoutDescriptor desc{
        .label = "Test Bind Group Layout",
        .entries = { entry }
    };

    auto layout = device->createBindGroupLayout(desc);
    EXPECT_NE(layout, nullptr);
}

TEST_P(GfxCppBindGroupLayoutTest, CreateBindGroupLayoutWithUniformBuffer)
{
    ASSERT_NE(device, nullptr);

    gfx::BindGroupLayoutEntry entry{
        .binding = 0,
        .visibility = gfx::ShaderStage::Vertex | gfx::ShaderStage::Fragment,
        .resource = gfx::BindGroupLayoutEntry::BufferBinding{
            .hasDynamicOffset = false,
            .minBindingSize = 256 }
    };

    gfx::BindGroupLayoutDescriptor desc{
        .label = "Uniform Buffer Layout",
        .entries = { entry }
    };

    auto layout = device->createBindGroupLayout(desc);
    EXPECT_NE(layout, nullptr);
}

TEST_P(GfxCppBindGroupLayoutTest, CreateBindGroupLayoutWithSampler)
{
    ASSERT_NE(device, nullptr);

    gfx::BindGroupLayoutEntry entry{
        .binding = 0,
        .visibility = gfx::ShaderStage::Fragment,
        .resource = gfx::BindGroupLayoutEntry::SamplerBinding{
            .comparison = false }
    };

    gfx::BindGroupLayoutDescriptor desc{
        .label = "Sampler Layout",
        .entries = { entry }
    };

    auto layout = device->createBindGroupLayout(desc);
    EXPECT_NE(layout, nullptr);
}

TEST_P(GfxCppBindGroupLayoutTest, CreateBindGroupLayoutWithTexture)
{
    ASSERT_NE(device, nullptr);

    gfx::BindGroupLayoutEntry entry{
        .binding = 0,
        .visibility = gfx::ShaderStage::Fragment,
        .resource = gfx::BindGroupLayoutEntry::TextureBinding{
            .multisampled = false,
            .viewDimension = gfx::TextureViewType::View2D }
    };

    gfx::BindGroupLayoutDescriptor desc{
        .label = "Texture Layout",
        .entries = { entry }
    };

    auto layout = device->createBindGroupLayout(desc);
    EXPECT_NE(layout, nullptr);
}

TEST_P(GfxCppBindGroupLayoutTest, CreateBindGroupLayoutWithStorageTexture)
{
    ASSERT_NE(device, nullptr);

    gfx::BindGroupLayoutEntry entry{
        .binding = 0,
        .visibility = gfx::ShaderStage::Compute,
        .resource = gfx::BindGroupLayoutEntry::StorageTextureBinding{
            .format = gfx::Format::R32G32B32A32Float,
            .writeOnly = true,
            .viewDimension = gfx::TextureViewType::View2D }
    };

    gfx::BindGroupLayoutDescriptor desc{
        .label = "Storage Texture Layout",
        .entries = { entry }
    };

    auto layout = device->createBindGroupLayout(desc);
    EXPECT_NE(layout, nullptr);
}

TEST_P(GfxCppBindGroupLayoutTest, CreateBindGroupLayoutWithMultipleEntries)
{
    ASSERT_NE(device, nullptr);

    std::vector<gfx::BindGroupLayoutEntry> entries;

    // Uniform buffer at binding 0
    entries.push_back(gfx::BindGroupLayoutEntry{
        .binding = 0,
        .visibility = gfx::ShaderStage::Vertex,
        .resource = gfx::BindGroupLayoutEntry::BufferBinding{
            .hasDynamicOffset = false,
            .minBindingSize = 256 } });

    // Texture at binding 1
    entries.push_back(gfx::BindGroupLayoutEntry{
        .binding = 1,
        .visibility = gfx::ShaderStage::Fragment,
        .resource = gfx::BindGroupLayoutEntry::TextureBinding{
            .multisampled = false,
            .viewDimension = gfx::TextureViewType::View2D } });

    // Sampler at binding 2
    entries.push_back(gfx::BindGroupLayoutEntry{
        .binding = 2,
        .visibility = gfx::ShaderStage::Fragment,
        .resource = gfx::BindGroupLayoutEntry::SamplerBinding{
            .comparison = false } });

    gfx::BindGroupLayoutDescriptor desc{
        .label = "Multi-Entry Layout",
        .entries = entries
    };

    auto layout = device->createBindGroupLayout(desc);
    EXPECT_NE(layout, nullptr);
}

TEST_P(GfxCppBindGroupLayoutTest, CreateBindGroupLayoutWithDynamicOffset)
{
    ASSERT_NE(device, nullptr);

    gfx::BindGroupLayoutEntry entry{
        .binding = 0,
        .visibility = gfx::ShaderStage::Compute,
        .resource = gfx::BindGroupLayoutEntry::BufferBinding{
            .hasDynamicOffset = true,
            .minBindingSize = 64 }
    };

    gfx::BindGroupLayoutDescriptor desc{
        .label = "Dynamic Offset Layout",
        .entries = { entry }
    };

    auto layout = device->createBindGroupLayout(desc);
    EXPECT_NE(layout, nullptr);
}

TEST_P(GfxCppBindGroupLayoutTest, CreateMultipleBindGroupLayouts)
{
    ASSERT_NE(device, nullptr);

    const int layoutCount = 3;
    std::vector<std::shared_ptr<gfx::BindGroupLayout>> layouts;

    for (int i = 0; i < layoutCount; ++i) {
        gfx::BindGroupLayoutEntry entry{
            .binding = 0,
            .visibility = gfx::ShaderStage::Compute,
            .resource = gfx::BindGroupLayoutEntry::BufferBinding{
                .hasDynamicOffset = false,
                .minBindingSize = 0 }
        };

        gfx::BindGroupLayoutDescriptor desc{};
        desc.entries = { entry };

        auto layout = device->createBindGroupLayout(desc);
        EXPECT_NE(layout, nullptr);
        layouts.push_back(layout);
    }

    EXPECT_EQ(layouts.size(), layoutCount);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppBindGroupLayoutTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

} // namespace
