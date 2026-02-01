#include <core/resource/BindGroup.h>
#include <core/resource/BindGroupLayout.h>
#include <core/resource/Buffer.h>
#include <core/resource/Sampler.h>
#include <core/system/Device.h>

#include <gfx/gfx.h>

#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

#include <vector>

namespace gfx {

class BindGroupImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc = {};
        instanceDesc.backend = backend;
        instanceDesc.applicationName = "BindGroupImplTest";
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

TEST_P(BindGroupImplTest, CreateBindGroupWithBuffer)
{
    DeviceImpl deviceWrapper(device);

    // Create bind group layout using C++ API
    BindGroupLayoutEntry layoutEntry{
        .binding = 0,
        .visibility = ShaderStage::Compute,
        .resource = BindGroupLayoutEntry::BufferBinding{
            .hasDynamicOffset = false,
            .minBindingSize = 0 }
    };

    BindGroupLayoutDescriptor layoutDesc;
    layoutDesc.entries = { layoutEntry };

    auto layout = deviceWrapper.createBindGroupLayout(layoutDesc);
    ASSERT_NE(layout, nullptr);

    // Create buffer using C++ API
    BufferDescriptor bufferDesc;
    bufferDesc.size = 1024;
    bufferDesc.usage = BufferUsage::Uniform;
    bufferDesc.memoryProperties = MemoryProperty::DeviceLocal;

    auto buffer = deviceWrapper.createBuffer(bufferDesc);
    ASSERT_NE(buffer, nullptr);

    // Create bind group using C++ API
    BindGroupEntry entry{
        .binding = 0,
        .resource = buffer,
        .offset = 0,
        .size = 1024
    };

    BindGroupDescriptor bindGroupDesc;
    bindGroupDesc.layout = layout;
    bindGroupDesc.entries = { entry };

    auto bindGroup = deviceWrapper.createBindGroup(bindGroupDesc);
    EXPECT_NE(bindGroup, nullptr);
}

TEST_P(BindGroupImplTest, CreateBindGroupWithTextureView)
{
    DeviceImpl deviceWrapper(device);

    // Create bind group layout
    BindGroupLayoutEntry layoutEntry{
        .binding = 0,
        .visibility = ShaderStage::Fragment,
        .resource = BindGroupLayoutEntry::TextureBinding{
            .multisampled = false,
            .viewDimension = TextureViewType::View2D }
    };

    BindGroupLayoutDescriptor layoutDesc;
    layoutDesc.entries = { layoutEntry };

    auto layout = deviceWrapper.createBindGroupLayout(layoutDesc);
    ASSERT_NE(layout, nullptr);

    // Create texture
    TextureDescriptor textureDesc;
    textureDesc.type = TextureType::Texture2D;
    textureDesc.size = { 256, 256, 1 };
    textureDesc.arrayLayerCount = 1;
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = SampleCount::Count1;
    textureDesc.format = TextureFormat::R8G8B8A8Unorm;
    textureDesc.usage = TextureUsage::TextureBinding;

    auto texture = deviceWrapper.createTexture(textureDesc);
    ASSERT_NE(texture, nullptr);

    // Create texture view
    TextureViewDescriptor viewDesc;
    viewDesc.format = TextureFormat::R8G8B8A8Unorm;
    viewDesc.viewType = TextureViewType::View2D;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    auto textureView = texture->createView(viewDesc);
    ASSERT_NE(textureView, nullptr);

    // Create bind group
    BindGroupEntry entry{
        .binding = 0,
        .resource = textureView
    };

    BindGroupDescriptor bindGroupDesc;
    bindGroupDesc.layout = layout;
    bindGroupDesc.entries = { entry };

    auto bindGroup = deviceWrapper.createBindGroup(bindGroupDesc);
    EXPECT_NE(bindGroup, nullptr);
}

TEST_P(BindGroupImplTest, CreateBindGroupWithSampler)
{
    DeviceImpl deviceWrapper(device);

    // Create bind group layout
    BindGroupLayoutEntry layoutEntry{
        .binding = 0,
        .visibility = ShaderStage::Fragment,
        .resource = BindGroupLayoutEntry::SamplerBinding{
            .comparison = false }
    };

    BindGroupLayoutDescriptor layoutDesc;
    layoutDesc.entries = { layoutEntry };

    auto layout = deviceWrapper.createBindGroupLayout(layoutDesc);
    ASSERT_NE(layout, nullptr);

    // Create sampler
    SamplerDescriptor samplerDesc;
    samplerDesc.addressModeU = AddressMode::Repeat;
    samplerDesc.addressModeV = AddressMode::Repeat;
    samplerDesc.addressModeW = AddressMode::Repeat;
    samplerDesc.magFilter = FilterMode::Linear;
    samplerDesc.minFilter = FilterMode::Linear;
    samplerDesc.mipmapFilter = FilterMode::Linear;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = 1000.0f;
    samplerDesc.compare = CompareFunction::Never;
    samplerDesc.maxAnisotropy = 1.0f;

    auto sampler = deviceWrapper.createSampler(samplerDesc);
    ASSERT_NE(sampler, nullptr);

    // Create bind group
    BindGroupEntry entry{
        .binding = 0,
        .resource = sampler
    };

    BindGroupDescriptor bindGroupDesc;
    bindGroupDesc.layout = layout;
    bindGroupDesc.entries = { entry };

    auto bindGroup = deviceWrapper.createBindGroup(bindGroupDesc);
    EXPECT_NE(bindGroup, nullptr);
}

TEST_P(BindGroupImplTest, CreateBindGroupWithMultipleBindings)
{
    DeviceImpl deviceWrapper(device);

    // Create bind group layout with multiple bindings
    BindGroupLayoutEntry bufferLayoutEntry{
        .binding = 0,
        .visibility = ShaderStage::Compute,
        .resource = BindGroupLayoutEntry::BufferBinding{
            .hasDynamicOffset = false,
            .minBindingSize = 0
        }
    };

    BindGroupLayoutEntry samplerLayoutEntry{
        .binding = 1,
        .visibility = ShaderStage::Compute,
        .resource = BindGroupLayoutEntry::SamplerBinding{
            .comparison = false
        }
    };

    BindGroupLayoutDescriptor layoutDesc;
    layoutDesc.entries = {bufferLayoutEntry, samplerLayoutEntry};

    auto layout = deviceWrapper.createBindGroupLayout(layoutDesc);
    ASSERT_NE(layout, nullptr);

    // Create buffer
    BufferDescriptor bufferDesc;
    bufferDesc.size = 1024;
    bufferDesc.usage = BufferUsage::Uniform;
    bufferDesc.memoryProperties = MemoryProperty::DeviceLocal;

    auto buffer = deviceWrapper.createBuffer(bufferDesc);
    ASSERT_NE(buffer, nullptr);

    // Create sampler
    SamplerDescriptor samplerDesc;
    samplerDesc.addressModeU = AddressMode::Repeat;
    samplerDesc.addressModeV = AddressMode::Repeat;
    samplerDesc.addressModeW = AddressMode::Repeat;
    samplerDesc.magFilter = FilterMode::Linear;
    samplerDesc.minFilter = FilterMode::Linear;
    samplerDesc.mipmapFilter = FilterMode::Linear;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = 1000.0f;
    samplerDesc.compare = CompareFunction::Never;
    samplerDesc.maxAnisotropy = 1.0f;

    auto sampler = deviceWrapper.createSampler(samplerDesc);
    ASSERT_NE(sampler, nullptr);

    // Create bind group
    BindGroupEntry bufferEntry{
        .binding = 0,
        .resource = buffer,
        .offset = 0,
        .size = 1024
    };

    BindGroupEntry samplerEntry{
        .binding = 1,
        .resource = sampler
    };

    BindGroupDescriptor bindGroupDesc;
    bindGroupDesc.layout = layout;
    bindGroupDesc.entries = {bufferEntry, samplerEntry};

    auto bindGroup = deviceWrapper.createBindGroup(bindGroupDesc);
    EXPECT_NE(bindGroup, nullptr);
}

TEST_P(BindGroupImplTest, MultipleBindGroups_IndependentHandles)
{
    DeviceImpl deviceWrapper(device);

    // Create bind group layout
    BindGroupLayoutEntry layoutEntry{
        .binding = 0,
        .visibility = ShaderStage::Compute,
        .resource = BindGroupLayoutEntry::BufferBinding{
            .hasDynamicOffset = false,
            .minBindingSize = 0
        }
    };

    BindGroupLayoutDescriptor layoutDesc;
    layoutDesc.entries = {layoutEntry};

    auto layout = deviceWrapper.createBindGroupLayout(layoutDesc);
    ASSERT_NE(layout, nullptr);

    // Create buffer
    BufferDescriptor bufferDesc;
    bufferDesc.size = 1024;
    bufferDesc.usage = BufferUsage::Uniform;
    bufferDesc.memoryProperties = MemoryProperty::DeviceLocal;

    auto buffer = deviceWrapper.createBuffer(bufferDesc);
    ASSERT_NE(buffer, nullptr);

    // Create bind groups
    BindGroupEntry entry{
        .binding = 0,
        .resource = buffer,
        .offset = 0,
        .size = 1024
    };

    BindGroupDescriptor bindGroupDesc;
    bindGroupDesc.layout = layout;
    bindGroupDesc.entries = {entry};

    auto bindGroup1 = deviceWrapper.createBindGroup(bindGroupDesc);
    auto bindGroup2 = deviceWrapper.createBindGroup(bindGroupDesc);

    ASSERT_NE(bindGroup1, nullptr);
    ASSERT_NE(bindGroup2, nullptr);
    EXPECT_NE(bindGroup1.get(), bindGroup2.get());
}

#if defined(GFX_ENABLE_VULKAN) && defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(AllBackends, BindGroupImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU));
#elif defined(GFX_ENABLE_VULKAN)
INSTANTIATE_TEST_SUITE_P(VulkanOnly, BindGroupImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN));
#elif defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(WebGPUOnly, BindGroupImplTest,
    ::testing::Values(GFX_BACKEND_WEBGPU));
#endif

} // namespace gfx
