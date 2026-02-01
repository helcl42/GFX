#include <core/resource/Sampler.h>
#include <core/system/Device.h>

#include <gfx/gfx.h>

#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

namespace gfx {

class SamplerImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc = {};
        instanceDesc.backend = backend;
        instanceDesc.applicationName = "SamplerImplTest";
        ASSERT_EQ(gfxCreateInstance(&instanceDesc, &instance), GFX_RESULT_SUCCESS);

        GfxAdapterDescriptor adapterDesc = {};
        ASSERT_EQ(gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter), GFX_RESULT_SUCCESS);

        GfxDeviceDescriptor deviceDesc = {};
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

    SamplerDescriptor desc = {};
    desc.addressModeU = AddressMode::Repeat;
    desc.addressModeV = AddressMode::Repeat;
    desc.addressModeW = AddressMode::Repeat;
    desc.magFilter = FilterMode::Linear;
    desc.minFilter = FilterMode::Linear;
    desc.mipmapFilter = FilterMode::Linear;
    desc.lodMinClamp = 0.0f;
    desc.lodMaxClamp = 1000.0f;
    desc.compare = CompareFunction::Never;
    desc.maxAnisotropy = 1.0f;

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

    SamplerDescriptor desc = {};
    desc.addressModeU = AddressMode::ClampToEdge;
    desc.addressModeV = AddressMode::ClampToEdge;
    desc.addressModeW = AddressMode::ClampToEdge;
    desc.magFilter = FilterMode::Linear;
    desc.minFilter = FilterMode::Linear;
    desc.mipmapFilter = FilterMode::Linear;
    desc.lodMinClamp = 0.0f;
    desc.lodMaxClamp = 1000.0f;
    desc.compare = CompareFunction::Never;
    desc.maxAnisotropy = 16.0f;

    auto sampler = deviceWrapper.createSampler(desc);
    ASSERT_NE(sampler, nullptr);
}

TEST_P(SamplerImplTest, CreateSamplerWithComparison)
{
    DeviceImpl deviceWrapper(device);

    SamplerDescriptor desc = {};
    desc.addressModeU = AddressMode::ClampToEdge;
    desc.addressModeV = AddressMode::ClampToEdge;
    desc.addressModeW = AddressMode::ClampToEdge;
    desc.magFilter = FilterMode::Linear;
    desc.minFilter = FilterMode::Linear;
    desc.mipmapFilter = FilterMode::Linear;
    desc.lodMinClamp = 0.0f;
    desc.lodMaxClamp = 1000.0f;
    desc.compare = CompareFunction::LessEqual;
    desc.maxAnisotropy = 1.0f;

    auto sampler = deviceWrapper.createSampler(desc);
    ASSERT_NE(sampler, nullptr);
}

TEST_P(SamplerImplTest, MultipleSamplers_IndependentHandles)
{
    DeviceImpl deviceWrapper(device);

    SamplerDescriptor desc;
    desc.addressModeU = AddressMode::Repeat;
    desc.addressModeV = AddressMode::Repeat;
    desc.addressModeW = AddressMode::Repeat;
    desc.magFilter = FilterMode::Linear;
    desc.minFilter = FilterMode::Linear;
    desc.mipmapFilter = FilterMode::Linear;
    desc.lodMinClamp = 0.0f;
    desc.lodMaxClamp = 1000.0f;
    desc.compare = CompareFunction::Never;
    desc.maxAnisotropy = 1.0f;

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

#if defined(GFX_ENABLE_VULKAN) && defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(AllBackends, SamplerImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU));
#elif defined(GFX_ENABLE_VULKAN)
INSTANTIATE_TEST_SUITE_P(VulkanOnly, SamplerImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN));
#elif defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(WebGPUOnly, SamplerImplTest,
    ::testing::Values(GFX_BACKEND_WEBGPU));
#endif

} // namespace gfx
