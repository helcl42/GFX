#include <backend/webgpu/core/resource/BindGroup.h>
#include <backend/webgpu/core/resource/BindGroupLayout.h>
#include <backend/webgpu/core/resource/Buffer.h>
#include <backend/webgpu/core/resource/Sampler.h>
#include <backend/webgpu/core/resource/Texture.h>
#include <backend/webgpu/core/resource/TextureView.h>
#include <backend/webgpu/core/system/Device.h>
#include <backend/webgpu/core/system/Instance.h>

#include <gtest/gtest.h>

#include <memory>

namespace {

class WebGPUBindGroupTest : public testing::Test {
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

TEST_F(WebGPUBindGroupTest, CreateBindGroup_WithUniformBuffer)
{
    // Create layout
    gfx::backend::webgpu::core::BindGroupLayoutCreateInfo layoutInfo{};
    gfx::backend::webgpu::core::BindGroupLayoutEntry layoutEntry{};
    layoutEntry.binding = 0;
    layoutEntry.visibility = WGPUShaderStage_Vertex;
    layoutEntry.bufferType = WGPUBufferBindingType_Uniform;
    layoutEntry.bufferHasDynamicOffset = false;
    layoutEntry.bufferMinBindingSize = 0;
    layoutInfo.entries.push_back(layoutEntry);
    auto layout = std::make_unique<gfx::backend::webgpu::core::BindGroupLayout>(device.get(), layoutInfo);

    // Create buffer
    gfx::backend::webgpu::core::BufferCreateInfo bufferInfo{};
    bufferInfo.size = 256;
    bufferInfo.usage = WGPUBufferUsage_Uniform;
    auto buffer = std::make_unique<gfx::backend::webgpu::core::Buffer>(device.get(), bufferInfo);

    // Create bind group
    gfx::backend::webgpu::core::BindGroupCreateInfo createInfo{};
    createInfo.layout = layout->handle();
    
    gfx::backend::webgpu::core::BindGroupEntry entry{};
    entry.binding = 0;
    entry.buffer = buffer->handle();
    entry.bufferOffset = 0;
    entry.bufferSize = 256;
    createInfo.entries.push_back(entry);

    auto bindGroup = std::make_unique<gfx::backend::webgpu::core::BindGroup>(device.get(), createInfo);

    EXPECT_NE(bindGroup->handle(), nullptr);
}

TEST_F(WebGPUBindGroupTest, Handle_ReturnsValidWGPUBindGroup)
{
    // Create layout
    gfx::backend::webgpu::core::BindGroupLayoutCreateInfo layoutInfo{};
    gfx::backend::webgpu::core::BindGroupLayoutEntry layoutEntry{};
    layoutEntry.binding = 0;
    layoutEntry.visibility = WGPUShaderStage_Compute;
    layoutEntry.bufferType = WGPUBufferBindingType_Storage;
    layoutEntry.bufferHasDynamicOffset = false;
    layoutEntry.bufferMinBindingSize = 0;
    layoutInfo.entries.push_back(layoutEntry);
    auto layout = std::make_unique<gfx::backend::webgpu::core::BindGroupLayout>(device.get(), layoutInfo);

    // Create buffer
    gfx::backend::webgpu::core::BufferCreateInfo bufferInfo{};
    bufferInfo.size = 1024;
    bufferInfo.usage = WGPUBufferUsage_Storage;
    auto buffer = std::make_unique<gfx::backend::webgpu::core::Buffer>(device.get(), bufferInfo);

    // Create bind group
    gfx::backend::webgpu::core::BindGroupCreateInfo createInfo{};
    createInfo.layout = layout->handle();
    
    gfx::backend::webgpu::core::BindGroupEntry entry{};
    entry.binding = 0;
    entry.buffer = buffer->handle();
    entry.bufferOffset = 0;
    entry.bufferSize = 1024;
    createInfo.entries.push_back(entry);

    auto bindGroup = std::make_unique<gfx::backend::webgpu::core::BindGroup>(device.get(), createInfo);

    WGPUBindGroup handle = bindGroup->handle();
    EXPECT_NE(handle, nullptr);
}

TEST_F(WebGPUBindGroupTest, CreateBindGroup_WithTextureAndSampler)
{
    // Create layout
    gfx::backend::webgpu::core::BindGroupLayoutCreateInfo layoutInfo{};
    
    gfx::backend::webgpu::core::BindGroupLayoutEntry textureEntry{};
    textureEntry.binding = 0;
    textureEntry.visibility = WGPUShaderStage_Fragment;
    textureEntry.textureSampleType = WGPUTextureSampleType_Float;
    textureEntry.textureViewDimension = WGPUTextureViewDimension_2D;
    textureEntry.textureMultisampled = false;
    layoutInfo.entries.push_back(textureEntry);

    gfx::backend::webgpu::core::BindGroupLayoutEntry samplerEntry{};
    samplerEntry.binding = 1;
    samplerEntry.visibility = WGPUShaderStage_Fragment;
    samplerEntry.samplerType = WGPUSamplerBindingType_Filtering;
    layoutInfo.entries.push_back(samplerEntry);

    auto layout = std::make_unique<gfx::backend::webgpu::core::BindGroupLayout>(device.get(), layoutInfo);

    // Create texture
    gfx::backend::webgpu::core::TextureCreateInfo texInfo{};
    texInfo.format = WGPUTextureFormat_RGBA8Unorm;
    texInfo.size = {256, 256, 1};
    texInfo.usage = WGPUTextureUsage_TextureBinding;
    texInfo.dimension = WGPUTextureDimension_2D;
    texInfo.mipLevelCount = 1;
    texInfo.sampleCount = 1;
    texInfo.arrayLayers = 1;
    auto texture = std::make_unique<gfx::backend::webgpu::core::Texture>(device.get(), texInfo);

    // Create texture view
    gfx::backend::webgpu::core::TextureViewCreateInfo viewInfo{};
    viewInfo.format = WGPUTextureFormat_RGBA8Unorm;
    viewInfo.viewDimension = WGPUTextureViewDimension_2D;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;
    auto textureView = std::make_unique<gfx::backend::webgpu::core::TextureView>(texture.get(), viewInfo);

    // Create sampler
    gfx::backend::webgpu::core::SamplerCreateInfo samplerInfo{};
    samplerInfo.minFilter = WGPUFilterMode_Linear;
    samplerInfo.magFilter = WGPUFilterMode_Linear;
    samplerInfo.mipmapFilter = WGPUMipmapFilterMode_Linear;
    samplerInfo.addressModeU = WGPUAddressMode_Repeat;
    samplerInfo.addressModeV = WGPUAddressMode_Repeat;
    samplerInfo.addressModeW = WGPUAddressMode_Repeat;
    auto sampler = std::make_unique<gfx::backend::webgpu::core::Sampler>(device.get(), samplerInfo);

    // Create bind group
    gfx::backend::webgpu::core::BindGroupCreateInfo createInfo{};
    createInfo.layout = layout->handle();
    
    gfx::backend::webgpu::core::BindGroupEntry texEntry{};
    texEntry.binding = 0;
    texEntry.textureView = textureView->handle();
    createInfo.entries.push_back(texEntry);

    gfx::backend::webgpu::core::BindGroupEntry sampEntry{};
    sampEntry.binding = 1;
    sampEntry.sampler = sampler->handle();
    createInfo.entries.push_back(sampEntry);

    auto bindGroup = std::make_unique<gfx::backend::webgpu::core::BindGroup>(device.get(), createInfo);

    EXPECT_NE(bindGroup->handle(), nullptr);
}

TEST_F(WebGPUBindGroupTest, Destructor_CleansUpResources)
{
    // Create layout
    gfx::backend::webgpu::core::BindGroupLayoutCreateInfo layoutInfo{};
    gfx::backend::webgpu::core::BindGroupLayoutEntry layoutEntry{};
    layoutEntry.binding = 0;
    layoutEntry.visibility = WGPUShaderStage_Compute;
    layoutEntry.bufferType = WGPUBufferBindingType_Storage;
    layoutEntry.bufferHasDynamicOffset = false;
    layoutEntry.bufferMinBindingSize = 0;
    layoutInfo.entries.push_back(layoutEntry);
    auto layout = std::make_unique<gfx::backend::webgpu::core::BindGroupLayout>(device.get(), layoutInfo);

    // Create buffer
    gfx::backend::webgpu::core::BufferCreateInfo bufferInfo{};
    bufferInfo.size = 512;
    bufferInfo.usage = WGPUBufferUsage_Storage;
    auto buffer = std::make_unique<gfx::backend::webgpu::core::Buffer>(device.get(), bufferInfo);

    {
        gfx::backend::webgpu::core::BindGroupCreateInfo createInfo{};
        createInfo.layout = layout->handle();
        
        gfx::backend::webgpu::core::BindGroupEntry entry{};
        entry.binding = 0;
        entry.buffer = buffer->handle();
        entry.bufferOffset = 0;
        entry.bufferSize = 512;
        createInfo.entries.push_back(entry);

        auto bindGroup = std::make_unique<gfx::backend::webgpu::core::BindGroup>(device.get(), createInfo);
        EXPECT_NE(bindGroup->handle(), nullptr);
    }

    // If we reach here without crashing, cleanup succeeded
    SUCCEED();
}

} // anonymous namespace
