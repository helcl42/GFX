#include <backend/webgpu/core/resource/Sampler.h>
#include <backend/webgpu/core/system/Device.h>
#include <backend/webgpu/core/system/Instance.h>

#include <gtest/gtest.h>

#include <memory>

namespace {

class WebGPUSamplerTest : public testing::Test {
protected:
    void SetUp() override
    {
        try {
            gfx::backend::webgpu::core::InstanceCreateInfo instInfo{};
            instance = std::make_unique<gfx::backend::webgpu::core::Instance>(instInfo);

            gfx::backend::webgpu::core::AdapterCreateInfo adapterInfo{};
            adapterInfo.adapterIndex = 0;
            adapter = instance->requestAdapter(adapterInfo);

            gfx::backend::webgpu::core::DeviceCreateInfo deviceInfo{};
            device = std::make_unique<gfx::backend::webgpu::core::Device>(adapter, deviceInfo);
        } catch (const std::exception& e) {
            GTEST_SKIP() << "WebGPU not available: " << e.what();
        }
    }

    std::unique_ptr<gfx::backend::webgpu::core::Instance> instance;
    gfx::backend::webgpu::core::Adapter* adapter = nullptr;
    std::unique_ptr<gfx::backend::webgpu::core::Device> device;
};

TEST_F(WebGPUSamplerTest, CreateSampler_WithDefaultSettings)
{
    gfx::backend::webgpu::core::SamplerCreateInfo createInfo{};
    createInfo.minFilter = WGPUFilterMode_Linear;
    createInfo.magFilter = WGPUFilterMode_Linear;
    createInfo.mipmapFilter = WGPUMipmapFilterMode_Linear;
    createInfo.addressModeU = WGPUAddressMode_Repeat;
    createInfo.addressModeV = WGPUAddressMode_Repeat;
    createInfo.addressModeW = WGPUAddressMode_Repeat;

    auto sampler = std::make_unique<gfx::backend::webgpu::core::Sampler>(device.get(), createInfo);

    EXPECT_NE(sampler->handle(), nullptr);
}

TEST_F(WebGPUSamplerTest, Handle_ReturnsValidWGPUSampler)
{
    gfx::backend::webgpu::core::SamplerCreateInfo createInfo{};
    createInfo.minFilter = WGPUFilterMode_Nearest;
    createInfo.magFilter = WGPUFilterMode_Nearest;
    createInfo.mipmapFilter = WGPUMipmapFilterMode_Nearest;
    createInfo.addressModeU = WGPUAddressMode_ClampToEdge;
    createInfo.addressModeV = WGPUAddressMode_ClampToEdge;
    createInfo.addressModeW = WGPUAddressMode_ClampToEdge;

    auto sampler = std::make_unique<gfx::backend::webgpu::core::Sampler>(device.get(), createInfo);

    WGPUSampler handle = sampler->handle();
    EXPECT_NE(handle, nullptr);
}

TEST_F(WebGPUSamplerTest, CreateSampler_WithAnisotropy)
{
    gfx::backend::webgpu::core::SamplerCreateInfo createInfo{};
    createInfo.minFilter = WGPUFilterMode_Linear;
    createInfo.magFilter = WGPUFilterMode_Linear;
    createInfo.mipmapFilter = WGPUMipmapFilterMode_Linear;
    createInfo.addressModeU = WGPUAddressMode_Repeat;
    createInfo.addressModeV = WGPUAddressMode_Repeat;
    createInfo.addressModeW = WGPUAddressMode_Repeat;
    createInfo.maxAnisotropy = 16;

    auto sampler = std::make_unique<gfx::backend::webgpu::core::Sampler>(device.get(), createInfo);

    EXPECT_NE(sampler->handle(), nullptr);
}

TEST_F(WebGPUSamplerTest, CreateSampler_WithComparison)
{
    gfx::backend::webgpu::core::SamplerCreateInfo createInfo{};
    createInfo.minFilter = WGPUFilterMode_Linear;
    createInfo.magFilter = WGPUFilterMode_Linear;
    createInfo.mipmapFilter = WGPUMipmapFilterMode_Linear;
    createInfo.addressModeU = WGPUAddressMode_ClampToEdge;
    createInfo.addressModeV = WGPUAddressMode_ClampToEdge;
    createInfo.addressModeW = WGPUAddressMode_ClampToEdge;
    createInfo.compareFunction = WGPUCompareFunction_Less;

    auto sampler = std::make_unique<gfx::backend::webgpu::core::Sampler>(device.get(), createInfo);

    EXPECT_NE(sampler->handle(), nullptr);
}

TEST_F(WebGPUSamplerTest, MultipleSamplers_CanCoexist)
{
    gfx::backend::webgpu::core::SamplerCreateInfo createInfo1{};
    createInfo1.minFilter = WGPUFilterMode_Linear;
    createInfo1.magFilter = WGPUFilterMode_Linear;
    createInfo1.mipmapFilter = WGPUMipmapFilterMode_Linear;
    createInfo1.addressModeU = WGPUAddressMode_Repeat;
    createInfo1.addressModeV = WGPUAddressMode_Repeat;
    createInfo1.addressModeW = WGPUAddressMode_Repeat;

    gfx::backend::webgpu::core::SamplerCreateInfo createInfo2{};
    createInfo2.minFilter = WGPUFilterMode_Nearest;
    createInfo2.magFilter = WGPUFilterMode_Nearest;
    createInfo2.mipmapFilter = WGPUMipmapFilterMode_Nearest;
    createInfo2.addressModeU = WGPUAddressMode_ClampToEdge;
    createInfo2.addressModeV = WGPUAddressMode_ClampToEdge;
    createInfo2.addressModeW = WGPUAddressMode_ClampToEdge;

    auto sampler1 = std::make_unique<gfx::backend::webgpu::core::Sampler>(device.get(), createInfo1);
    auto sampler2 = std::make_unique<gfx::backend::webgpu::core::Sampler>(device.get(), createInfo2);

    EXPECT_NE(sampler1->handle(), nullptr);
    EXPECT_NE(sampler2->handle(), nullptr);
    EXPECT_NE(sampler1->handle(), sampler2->handle());
}

TEST_F(WebGPUSamplerTest, Destructor_CleansUpResources)
{
    {
        gfx::backend::webgpu::core::SamplerCreateInfo createInfo{};
        createInfo.minFilter = WGPUFilterMode_Linear;
        createInfo.magFilter = WGPUFilterMode_Linear;
        createInfo.mipmapFilter = WGPUMipmapFilterMode_Linear;
        createInfo.addressModeU = WGPUAddressMode_Repeat;
        createInfo.addressModeV = WGPUAddressMode_Repeat;
        createInfo.addressModeW = WGPUAddressMode_Repeat;

        auto sampler = std::make_unique<gfx::backend::webgpu::core::Sampler>(device.get(), createInfo);
        EXPECT_NE(sampler->handle(), nullptr);
    }

    // If we reach here without crashing, cleanup succeeded
    SUCCEED();
}

} // anonymous namespace
