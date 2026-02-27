#include "../../common/CommonTest.h"

#include <core/resource/BindGroupLayout.h>
#include <core/system/Device.h>

#include <vector>

namespace gfx {

class BindGroupLayoutImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc{
            .backend = backend,
            .applicationName = "BindGroupLayoutImplTest"
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

TEST_P(BindGroupLayoutImplTest, CreateBindGroupLayout)
{
    DeviceImpl deviceWrapper(device);

    BindGroupLayoutEntry entry{
        .binding = 0,
        .visibility = ShaderStage::Compute,
        .resource = BindGroupLayoutEntry::BufferBinding{
            .hasDynamicOffset = false,
            .minBindingSize = 0 }
    };

    BindGroupLayoutDescriptor desc{
        .entries = { entry }
    };

    auto layout = deviceWrapper.createBindGroupLayout(desc);
    ASSERT_NE(layout, nullptr);
}

TEST_P(BindGroupLayoutImplTest, CreateBindGroupLayoutWithMultipleBindings)
{
    DeviceImpl deviceWrapper(device);

    BindGroupLayoutEntry bufferEntry{
        .binding = 0,
        .visibility = ShaderStage::Compute,
        .resource = BindGroupLayoutEntry::BufferBinding{ .hasDynamicOffset = false, .minBindingSize = 0 }
    };

    BindGroupLayoutEntry samplerEntry{
        .binding = 1,
        .visibility = ShaderStage::Fragment,
        .resource = BindGroupLayoutEntry::SamplerBinding{ .comparison = false }
    };

    BindGroupLayoutEntry textureEntry{
        .binding = 2,
        .visibility = ShaderStage::Fragment,
        .resource = BindGroupLayoutEntry::TextureBinding{
            .multisampled = false,
            .viewDimension = TextureViewType::View2D }
    };

    BindGroupLayoutDescriptor desc{
        .entries = { bufferEntry, samplerEntry, textureEntry }
    };

    auto layout = deviceWrapper.createBindGroupLayout(desc);
    ASSERT_NE(layout, nullptr);
}

TEST_P(BindGroupLayoutImplTest, CreateBindGroupLayoutWithStorageBuffer)
{
    DeviceImpl deviceWrapper(device);

    BindGroupLayoutEntry entry{
        .binding = 0,
        .visibility = ShaderStage::Compute,
        .resource = BindGroupLayoutEntry::BufferBinding{
            .hasDynamicOffset = false,
            .minBindingSize = 256 }
    };

    BindGroupLayoutDescriptor desc{
        .entries = { entry }
    };

    auto layout = deviceWrapper.createBindGroupLayout(desc);
    ASSERT_NE(layout, nullptr);
}

TEST_P(BindGroupLayoutImplTest, CreateBindGroupLayoutWithDynamicOffset)
{
    DeviceImpl deviceWrapper(device);

    BindGroupLayoutEntry entry{
        .binding = 0,
        .visibility = ShaderStage::Compute,
        .resource = BindGroupLayoutEntry::BufferBinding{
            .hasDynamicOffset = true,
            .minBindingSize = 256 }
    };

    BindGroupLayoutDescriptor desc{
        .entries = { entry }
    };

    auto layout = deviceWrapper.createBindGroupLayout(desc);
    ASSERT_NE(layout, nullptr);
}

TEST_P(BindGroupLayoutImplTest, CreateBindGroupLayoutWithStorageTexture)
{
    DeviceImpl deviceWrapper(device);

    BindGroupLayoutEntry entry{
        .binding = 0,
        .visibility = ShaderStage::Compute,
        .resource = BindGroupLayoutEntry::StorageTextureBinding{
            .format = Format::R8G8B8A8Unorm,
            .writeOnly = false,
            .viewDimension = TextureViewType::View2D }
    };

    BindGroupLayoutDescriptor desc{
        .entries = { entry }
    };

    auto layout = deviceWrapper.createBindGroupLayout(desc);
    ASSERT_NE(layout, nullptr);
}

TEST_P(BindGroupLayoutImplTest, MultipleLayouts_IndependentHandles)
{
    DeviceImpl deviceWrapper(device);

    BindGroupLayoutEntry entry{
        .binding = 0,
        .visibility = ShaderStage::Compute,
        .resource = BindGroupLayoutEntry::BufferBinding{
            .hasDynamicOffset = false,
            .minBindingSize = 0 }
    };

    BindGroupLayoutDescriptor desc{
        .entries = { entry }
    };

    auto layout1 = deviceWrapper.createBindGroupLayout(desc);
    auto layout2 = deviceWrapper.createBindGroupLayout(desc);

    ASSERT_NE(layout1, nullptr);
    ASSERT_NE(layout2, nullptr);
    EXPECT_NE(layout1.get(), layout2.get());
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    BindGroupLayoutImplTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

} // namespace gfx
