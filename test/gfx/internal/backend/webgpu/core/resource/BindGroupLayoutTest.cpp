#include <backend/webgpu/core/resource/BindGroupLayout.h>
#include <backend/webgpu/core/system/Device.h>
#include <backend/webgpu/core/system/Instance.h>

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace {

class WebGPUBindGroupLayoutTest : public testing::Test {
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

TEST_F(WebGPUBindGroupLayoutTest, CreateBindGroupLayout_WithUniformBuffer)
{
    gfx::backend::webgpu::core::BindGroupLayoutCreateInfo createInfo{};
    gfx::backend::webgpu::core::BindGroupLayoutEntry entry{};
    entry.binding = 0;
    entry.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    entry.bufferType = WGPUBufferBindingType_Uniform;
    entry.bufferHasDynamicOffset = false;
    entry.bufferMinBindingSize = 0;

    createInfo.entries.push_back(entry);

    auto layout = std::make_unique<gfx::backend::webgpu::core::BindGroupLayout>(device.get(), createInfo);

    EXPECT_NE(layout->handle(), nullptr);
}

TEST_F(WebGPUBindGroupLayoutTest, Handle_ReturnsValidWGPUBindGroupLayout)
{
    gfx::backend::webgpu::core::BindGroupLayoutCreateInfo createInfo{};
    gfx::backend::webgpu::core::BindGroupLayoutEntry entry{};
    entry.binding = 0;
    entry.visibility = WGPUShaderStage_Compute;
    entry.bufferType = WGPUBufferBindingType_Storage;
    entry.bufferHasDynamicOffset = false;
    entry.bufferMinBindingSize = 0;

    createInfo.entries.push_back(entry);

    auto layout = std::make_unique<gfx::backend::webgpu::core::BindGroupLayout>(device.get(), createInfo);

    WGPUBindGroupLayout handle = layout->handle();
    EXPECT_NE(handle, nullptr);
}

TEST_F(WebGPUBindGroupLayoutTest, CreateBindGroupLayout_WithTexture)
{
    gfx::backend::webgpu::core::BindGroupLayoutCreateInfo createInfo{};
    gfx::backend::webgpu::core::BindGroupLayoutEntry entry{};
    entry.binding = 0;
    entry.visibility = WGPUShaderStage_Fragment;
    entry.textureSampleType = WGPUTextureSampleType_Float;
    entry.textureViewDimension = WGPUTextureViewDimension_2D;
    entry.textureMultisampled = false;

    createInfo.entries.push_back(entry);

    auto layout = std::make_unique<gfx::backend::webgpu::core::BindGroupLayout>(device.get(), createInfo);

    EXPECT_NE(layout->handle(), nullptr);
}

TEST_F(WebGPUBindGroupLayoutTest, CreateBindGroupLayout_WithSampler)
{
    gfx::backend::webgpu::core::BindGroupLayoutCreateInfo createInfo{};
    gfx::backend::webgpu::core::BindGroupLayoutEntry entry{};
    entry.binding = 0;
    entry.visibility = WGPUShaderStage_Fragment;
    entry.samplerType = WGPUSamplerBindingType_Filtering;

    createInfo.entries.push_back(entry);

    auto layout = std::make_unique<gfx::backend::webgpu::core::BindGroupLayout>(device.get(), createInfo);

    EXPECT_NE(layout->handle(), nullptr);
}

TEST_F(WebGPUBindGroupLayoutTest, CreateBindGroupLayout_WithMultipleEntries)
{
    gfx::backend::webgpu::core::BindGroupLayoutCreateInfo createInfo{};

    gfx::backend::webgpu::core::BindGroupLayoutEntry entry0{};
    entry0.binding = 0;
    entry0.visibility = WGPUShaderStage_Vertex;
    entry0.bufferType = WGPUBufferBindingType_Uniform;
    entry0.bufferHasDynamicOffset = false;
    entry0.bufferMinBindingSize = 0;

    gfx::backend::webgpu::core::BindGroupLayoutEntry entry1{};
    entry1.binding = 1;
    entry1.visibility = WGPUShaderStage_Fragment;
    entry1.textureSampleType = WGPUTextureSampleType_Float;
    entry1.textureViewDimension = WGPUTextureViewDimension_2D;
    entry1.textureMultisampled = false;

    gfx::backend::webgpu::core::BindGroupLayoutEntry entry2{};
    entry2.binding = 2;
    entry2.visibility = WGPUShaderStage_Fragment;
    entry2.samplerType = WGPUSamplerBindingType_Filtering;

    createInfo.entries.push_back(entry0);
    createInfo.entries.push_back(entry1);
    createInfo.entries.push_back(entry2);

    auto layout = std::make_unique<gfx::backend::webgpu::core::BindGroupLayout>(device.get(), createInfo);

    EXPECT_NE(layout->handle(), nullptr);
}

TEST_F(WebGPUBindGroupLayoutTest, MultipleLayouts_CanCoexist)
{
    gfx::backend::webgpu::core::BindGroupLayoutCreateInfo createInfo1{};
    gfx::backend::webgpu::core::BindGroupLayoutEntry entry1{};
    entry1.binding = 0;
    entry1.visibility = WGPUShaderStage_Compute;
    entry1.bufferType = WGPUBufferBindingType_Storage;
    entry1.bufferHasDynamicOffset = false;
    entry1.bufferMinBindingSize = 0;
    createInfo1.entries.push_back(entry1);

    gfx::backend::webgpu::core::BindGroupLayoutCreateInfo createInfo2{};
    gfx::backend::webgpu::core::BindGroupLayoutEntry entry2{};
    entry2.binding = 0;
    entry2.visibility = WGPUShaderStage_Fragment;
    entry2.textureSampleType = WGPUTextureSampleType_Float;
    entry2.textureViewDimension = WGPUTextureViewDimension_2D;
    entry2.textureMultisampled = false;
    createInfo2.entries.push_back(entry2);

    auto layout1 = std::make_unique<gfx::backend::webgpu::core::BindGroupLayout>(device.get(), createInfo1);
    auto layout2 = std::make_unique<gfx::backend::webgpu::core::BindGroupLayout>(device.get(), createInfo2);

    EXPECT_NE(layout1->handle(), nullptr);
    EXPECT_NE(layout2->handle(), nullptr);
    EXPECT_NE(layout1->handle(), layout2->handle());
}

TEST_F(WebGPUBindGroupLayoutTest, Destructor_CleansUpResources)
{
    {
        gfx::backend::webgpu::core::BindGroupLayoutCreateInfo createInfo{};
        gfx::backend::webgpu::core::BindGroupLayoutEntry entry{};
        entry.binding = 0;
        entry.visibility = WGPUShaderStage_Compute;
        entry.bufferType = WGPUBufferBindingType_Storage;
        entry.bufferHasDynamicOffset = false;
        entry.bufferMinBindingSize = 0;
        createInfo.entries.push_back(entry);

        auto layout = std::make_unique<gfx::backend::webgpu::core::BindGroupLayout>(device.get(), createInfo);
        EXPECT_NE(layout->handle(), nullptr);
    }

    // If we reach here without crashing, cleanup succeeded
    SUCCEED();
}

} // anonymous namespace
