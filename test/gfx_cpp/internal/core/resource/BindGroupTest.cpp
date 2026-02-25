#include "../../common/CommonTest.h"
#include <core/resource/BindGroup.h>
#include <core/resource/BindGroupLayout.h>
#include <core/resource/Buffer.h>
#include <core/resource/Sampler.h>
#include <core/system/Device.h>

#include <vector>

namespace gfx {

class BindGroupImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc{
            .backend = backend,
            .applicationName = "BindGroupImplTest"
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

    BindGroupLayoutDescriptor layoutDesc{
        .entries = { layoutEntry }
    };

    auto layout = deviceWrapper.createBindGroupLayout(layoutDesc);
    ASSERT_NE(layout, nullptr);

    // Create buffer using C++ API
    BufferDescriptor bufferDesc{
        .size = 1024,
        .usage = BufferUsage::Uniform,
        .memoryProperties = MemoryProperty::DeviceLocal
    };

    auto buffer = deviceWrapper.createBuffer(bufferDesc);
    ASSERT_NE(buffer, nullptr);

    // Create bind group using C++ API
    BindGroupEntry entry{
        .binding = 0,
        .resource = buffer,
        .offset = 0,
        .size = 1024
    };

    BindGroupDescriptor bindGroupDesc{
        .layout = layout,
        .entries = { entry }
    };

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

    BindGroupLayoutDescriptor layoutDesc{
        .entries = { layoutEntry }
    };

    auto layout = deviceWrapper.createBindGroupLayout(layoutDesc);
    ASSERT_NE(layout, nullptr);

    // Create texture
    TextureDescriptor textureDesc{
        .type = TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = SampleCount::Count1,
        .format = TextureFormat::R8G8B8A8Unorm,
        .usage = TextureUsage::TextureBinding
    };

    auto texture = deviceWrapper.createTexture(textureDesc);
    ASSERT_NE(texture, nullptr);

    // Create texture view
    TextureViewDescriptor viewDesc{
        .viewType = TextureViewType::View2D,
        .format = TextureFormat::R8G8B8A8Unorm,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1
    };

    auto textureView = texture->createView(viewDesc);
    ASSERT_NE(textureView, nullptr);

    // Create bind group
    BindGroupEntry entry{
        .binding = 0,
        .resource = textureView
    };

    BindGroupDescriptor bindGroupDesc{
        .layout = layout,
        .entries = { entry }
    };

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

    BindGroupLayoutDescriptor layoutDesc{
        .entries = { layoutEntry }
    };

    auto layout = deviceWrapper.createBindGroupLayout(layoutDesc);
    ASSERT_NE(layout, nullptr);

    // Create sampler
    SamplerDescriptor samplerDesc{
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

    auto sampler = deviceWrapper.createSampler(samplerDesc);
    ASSERT_NE(sampler, nullptr);

    // Create bind group
    BindGroupEntry entry{
        .binding = 0,
        .resource = sampler
    };

    BindGroupDescriptor bindGroupDesc{
        .layout = layout,
        .entries = { entry }
    };

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
            .minBindingSize = 0 }
    };

    BindGroupLayoutEntry samplerLayoutEntry{
        .binding = 1,
        .visibility = ShaderStage::Compute,
        .resource = BindGroupLayoutEntry::SamplerBinding{
            .comparison = false }
    };

    BindGroupLayoutDescriptor layoutDesc{
        .entries = { bufferLayoutEntry, samplerLayoutEntry }
    };

    auto layout = deviceWrapper.createBindGroupLayout(layoutDesc);
    ASSERT_NE(layout, nullptr);

    // Create buffer
    BufferDescriptor bufferDesc{
        .size = 1024,
        .usage = BufferUsage::Uniform,
        .memoryProperties = MemoryProperty::DeviceLocal
    };

    auto buffer = deviceWrapper.createBuffer(bufferDesc);
    ASSERT_NE(buffer, nullptr);

    // Create sampler
    SamplerDescriptor samplerDesc{
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

    BindGroupDescriptor bindGroupDesc{
        .layout = layout,
        .entries = { bufferEntry, samplerEntry }
    };

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
            .minBindingSize = 0 }
    };

    BindGroupLayoutDescriptor layoutDesc{
        .entries = { layoutEntry }
    };

    auto layout = deviceWrapper.createBindGroupLayout(layoutDesc);
    ASSERT_NE(layout, nullptr);

    // Create buffer
    BufferDescriptor bufferDesc{
        .size = 1024,
        .usage = BufferUsage::Uniform,
        .memoryProperties = MemoryProperty::DeviceLocal
    };

    auto buffer = deviceWrapper.createBuffer(bufferDesc);
    ASSERT_NE(buffer, nullptr);

    // Create bind groups
    BindGroupEntry entry{
        .binding = 0,
        .resource = buffer,
        .offset = 0,
        .size = 1024
    };

    BindGroupDescriptor bindGroupDesc{
        .layout = layout,
        .entries = { entry }
    };

    auto bindGroup1 = deviceWrapper.createBindGroup(bindGroupDesc);
    auto bindGroup2 = deviceWrapper.createBindGroup(bindGroupDesc);

    ASSERT_NE(bindGroup1, nullptr);
    ASSERT_NE(bindGroup2, nullptr);
    EXPECT_NE(bindGroup1.get(), bindGroup2.get());
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    BindGroupImplTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

} // namespace gfx
