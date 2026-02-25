#include "CommonTest.h"

#include <memory>

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxCppSamplerTest : public testing::TestWithParam<gfx::Backend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        try {
            instance = gfx::createInstance({ .backend = backend, .enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG } });
            adapter = instance->requestAdapter({ .adapterIndex = 0 });
            device = adapter->createDevice({ .label = "Test Device" });
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up: " << e.what();
        }
    }

    void TearDown() override
    {
        device.reset();
        adapter.reset();
        instance.reset();
    }

    gfx::Backend backend = gfx::Backend::Vulkan;
    std::shared_ptr<gfx::Instance> instance;
    std::shared_ptr<gfx::Adapter> adapter;
    std::shared_ptr<gfx::Device> device;
};

TEST_P(GfxCppSamplerTest, CreateSamplerWithValidDescriptor)
{
    auto sampler = device->createSampler({ .label = "Test Sampler",
        .addressModeU = gfx::AddressMode::Repeat,
        .addressModeV = gfx::AddressMode::Repeat,
        .addressModeW = gfx::AddressMode::Repeat,
        .magFilter = gfx::FilterMode::Linear,
        .minFilter = gfx::FilterMode::Linear,
        .mipmapFilter = gfx::FilterMode::Linear,
        .lodMinClamp = 0.0f,
        .lodMaxClamp = 1000.0f,
        .compare = gfx::CompareFunction::Undefined,
        .maxAnisotropy = 1 });

    EXPECT_NE(sampler, nullptr);
}

TEST_P(GfxCppSamplerTest, CreateSamplerWithClampToEdge)
{
    auto sampler = device->createSampler({ .label = "Clamp Sampler",
        .addressModeU = gfx::AddressMode::ClampToEdge,
        .addressModeV = gfx::AddressMode::ClampToEdge,
        .addressModeW = gfx::AddressMode::ClampToEdge,
        .magFilter = gfx::FilterMode::Linear,
        .minFilter = gfx::FilterMode::Linear,
        .mipmapFilter = gfx::FilterMode::Nearest,
        .lodMinClamp = 0.0f,
        .lodMaxClamp = 0.0f,
        .compare = gfx::CompareFunction::Undefined,
        .maxAnisotropy = 1 });

    EXPECT_NE(sampler, nullptr);
}

TEST_P(GfxCppSamplerTest, CreateSamplerWithNearestFiltering)
{
    auto sampler = device->createSampler({ .label = "Nearest Sampler",
        .addressModeU = gfx::AddressMode::Repeat,
        .addressModeV = gfx::AddressMode::Repeat,
        .addressModeW = gfx::AddressMode::Repeat,
        .magFilter = gfx::FilterMode::Nearest,
        .minFilter = gfx::FilterMode::Nearest,
        .mipmapFilter = gfx::FilterMode::Nearest,
        .lodMinClamp = 0.0f,
        .lodMaxClamp = 1000.0f,
        .compare = gfx::CompareFunction::Undefined,
        .maxAnisotropy = 1 });

    EXPECT_NE(sampler, nullptr);
}

TEST_P(GfxCppSamplerTest, CreateSamplerWithCompareFunction)
{
    auto sampler = device->createSampler({ .label = "Compare Sampler",
        .addressModeU = gfx::AddressMode::ClampToEdge,
        .addressModeV = gfx::AddressMode::ClampToEdge,
        .addressModeW = gfx::AddressMode::ClampToEdge,
        .magFilter = gfx::FilterMode::Linear,
        .minFilter = gfx::FilterMode::Linear,
        .mipmapFilter = gfx::FilterMode::Linear,
        .lodMinClamp = 0.0f,
        .lodMaxClamp = 1000.0f,
        .compare = gfx::CompareFunction::LessEqual, // For shadow mapping
        .maxAnisotropy = 1 });

    EXPECT_NE(sampler, nullptr);
}

TEST_P(GfxCppSamplerTest, CreateMultipleSamplers)
{
    const int samplerCount = 5;
    std::vector<std::shared_ptr<gfx::Sampler>> samplers;

    for (int i = 0; i < samplerCount; ++i) {
        auto sampler = device->createSampler({ .addressModeU = gfx::AddressMode::Repeat,
            .addressModeV = gfx::AddressMode::Repeat,
            .addressModeW = gfx::AddressMode::Repeat,
            .magFilter = gfx::FilterMode::Linear,
            .minFilter = gfx::FilterMode::Linear,
            .mipmapFilter = gfx::FilterMode::Linear,
            .lodMinClamp = 0.0f,
            .lodMaxClamp = 1000.0f,
            .compare = gfx::CompareFunction::Undefined,
            .maxAnisotropy = 1 });

        EXPECT_NE(sampler, nullptr);
        samplers.push_back(sampler);
    }

    EXPECT_EQ(samplers.size(), samplerCount);
}

TEST_P(GfxCppSamplerTest, CreateSamplerWithDefaultDescriptor)
{
    // Test that default descriptor values work
    auto sampler = device->createSampler({});

    EXPECT_NE(sampler, nullptr);
}

TEST_P(GfxCppSamplerTest, CreateSamplerWithMirrorRepeat)
{
    auto sampler = device->createSampler({ .label = "Mirror Sampler",
        .addressModeU = gfx::AddressMode::MirrorRepeat,
        .addressModeV = gfx::AddressMode::MirrorRepeat,
        .addressModeW = gfx::AddressMode::MirrorRepeat,
        .magFilter = gfx::FilterMode::Linear,
        .minFilter = gfx::FilterMode::Linear,
        .mipmapFilter = gfx::FilterMode::Linear });

    EXPECT_NE(sampler, nullptr);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppSamplerTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

} // namespace
