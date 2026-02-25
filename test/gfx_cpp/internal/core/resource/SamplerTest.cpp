#include "../../common/CommonTest.h"

#include <core/resource/Sampler.h>
#include <core/system/Device.h>

namespace gfx {

class SamplerImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc{
            .backend = backend,
            .applicationName = "SamplerImplTest"
        };
        ASSERT_EQ(gfxCreateInstance(&instanceDesc, &instance), GFX_RESULT_SUCCESS);

        GfxAdapterDescriptor adapterDesc{
            .sType = GFX_STRUCTURE_TYPE_ADAPTER_DESCRIPTOR,
            .pNext = nullptr,
            .adapterIndex = 0
        };
        ASSERT_EQ(gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter), GFX_RESULT_SUCCESS);

        GfxDeviceDescriptor deviceDesc{
            .sType = GFX_STRUCTURE_TYPE_DEVICE_DESCRIPTOR,
            .pNext = nullptr,
            .label = nullptr,
            .queueRequests = nullptr,
            .queueRequestCount = 0,
            .enabledExtensions = nullptr,
            .enabledExtensionCount = 0
        };
        ASSERT_EQ(gfxAdapterCreateDevice(adapter, &deviceDesc, &device), GFX_RESULT_SUCCESS);
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

    GfxBackend backend;
    GfxInstance instance = nullptr;
    GfxAdapter adapter = nullptr;
    GfxDevice device = nullptr;
};

TEST_P(SamplerImplTest, CreateSampler)
{
    DeviceImpl deviceWrapper(device);

    SamplerDescriptor desc{
        .addressModeU = AddressMode::Repeat,
        .addressModeV = AddressMode::Repeat,
        .addressModeW = AddressMode::Repeat,
        .magFilter = FilterMode::Linear,
        .minFilter = FilterMode::Linear,
        .mipmapFilter = FilterMode::Linear,
        .lodMinClamp = 0.0f,
        .lodMaxClamp = 1000.0f,
        .compare = CompareFunction::Never,
        .maxAnisotropy = 1
    };

    auto sampler = deviceWrapper.createSampler(desc);
    ASSERT_NE(sampler, nullptr);

    // Verify we can get the handle
    auto* samplerImpl = dynamic_cast<SamplerImpl*>(sampler.get());
    ASSERT_NE(samplerImpl, nullptr);
    EXPECT_NE(samplerImpl->getHandle(), nullptr);
}

TEST_P(SamplerImplTest, CreateSamplerWithAnisotropy)
{
    DeviceImpl deviceWrapper(device);

    SamplerDescriptor desc{
        .addressModeU = AddressMode::ClampToEdge,
        .addressModeV = AddressMode::ClampToEdge,
        .addressModeW = AddressMode::ClampToEdge,
        .magFilter = FilterMode::Linear,
        .minFilter = FilterMode::Linear,
        .mipmapFilter = FilterMode::Linear,
        .lodMinClamp = 0.0f,
        .lodMaxClamp = 1000.0f,
        .compare = CompareFunction::Never,
        .maxAnisotropy = 16
    };

    auto sampler = deviceWrapper.createSampler(desc);
    ASSERT_NE(sampler, nullptr);
}

TEST_P(SamplerImplTest, CreateSamplerWithComparison)
{
    DeviceImpl deviceWrapper(device);

    SamplerDescriptor desc{
        .addressModeU = AddressMode::ClampToEdge,
        .addressModeV = AddressMode::ClampToEdge,
        .addressModeW = AddressMode::ClampToEdge,
        .magFilter = FilterMode::Linear,
        .minFilter = FilterMode::Linear,
        .mipmapFilter = FilterMode::Linear,
        .lodMinClamp = 0.0f,
        .lodMaxClamp = 1000.0f,
        .compare = CompareFunction::LessEqual,
        .maxAnisotropy = 1
    };

    auto sampler = deviceWrapper.createSampler(desc);
    ASSERT_NE(sampler, nullptr);
}

TEST_P(SamplerImplTest, MultipleSamplers_IndependentHandles)
{
    DeviceImpl deviceWrapper(device);

    SamplerDescriptor desc{
        .addressModeU = AddressMode::Repeat,
        .addressModeV = AddressMode::Repeat,
        .addressModeW = AddressMode::Repeat,
        .magFilter = FilterMode::Linear,
        .minFilter = FilterMode::Linear,
        .mipmapFilter = FilterMode::Linear,
        .lodMinClamp = 0.0f,
        .lodMaxClamp = 1000.0f,
        .compare = CompareFunction::Never,
        .maxAnisotropy = 1
    };

    auto sampler1 = deviceWrapper.createSampler(desc);
    auto sampler2 = deviceWrapper.createSampler(desc);

    ASSERT_NE(sampler1, nullptr);
    ASSERT_NE(sampler2, nullptr);
    EXPECT_NE(sampler1.get(), sampler2.get());

    // Verify handles are different
    auto* impl1 = dynamic_cast<SamplerImpl*>(sampler1.get());
    auto* impl2 = dynamic_cast<SamplerImpl*>(sampler2.get());
    ASSERT_NE(impl1, nullptr);
    ASSERT_NE(impl2, nullptr);
    EXPECT_NE(impl1->getHandle(), impl2->getHandle());
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    SamplerImplTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

} // namespace gfx
