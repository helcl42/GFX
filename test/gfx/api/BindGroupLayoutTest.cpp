#include <gfx/gfx.h>

#include <gtest/gtest.h>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

class GfxBindGroupLayoutTest : public testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        if (gfxLoadBackend(backend) != GFX_RESULT_SUCCESS) {
            GTEST_SKIP() << "Backend not available";
        }

        GfxInstanceDescriptor instDesc = {};
        instDesc.backend = backend;
        const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
        instDesc.enabledExtensions = extensions;
        instDesc.enabledExtensionCount = 1;

        if (gfxCreateInstance(&instDesc, &instance) != GFX_RESULT_SUCCESS) {
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to create instance";
        }

        GfxAdapterDescriptor adapterDesc = {};
        adapterDesc.adapterIndex = 0;

        if (gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter) != GFX_RESULT_SUCCESS) {
            gfxInstanceDestroy(instance);
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to request adapter";
        }

        GfxDeviceDescriptor deviceDesc = {};
        deviceDesc.label = "Test Device";

        if (gfxAdapterCreateDevice(adapter, &deviceDesc, &device) != GFX_RESULT_SUCCESS) {
            gfxInstanceDestroy(instance);
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to create device";
        }
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

    GfxBackend backend = GFX_BACKEND_VULKAN;
    GfxInstance instance = nullptr;
    GfxAdapter adapter = nullptr;
    GfxDevice device = nullptr;
};

// ===========================================================================
// BindGroupLayout Tests
// ===========================================================================

TEST_P(GfxBindGroupLayoutTest, CreateBindGroupLayoutWithValidDescriptor)
{
    GfxBindGroupLayoutEntry entry = {};
    entry.binding = 0;
    entry.visibility = GFX_SHADER_STAGE_VERTEX;
    entry.type = GFX_BINDING_TYPE_BUFFER;
    entry.buffer.hasDynamicOffset = false;
    entry.buffer.minBindingSize = 0;

    GfxBindGroupLayoutDescriptor desc = {};
    desc.label = "Test Bind Group Layout";
    desc.entries = &entry;
    desc.entryCount = 1;

    GfxBindGroupLayout layout = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(device, &desc, &layout);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(layout, nullptr);

    if (layout) {
        gfxBindGroupLayoutDestroy(layout);
    }
}

TEST_P(GfxBindGroupLayoutTest, CreateBindGroupLayoutWithNullDevice)
{
    GfxBindGroupLayoutEntry entry = {};
    entry.binding = 0;
    entry.visibility = GFX_SHADER_STAGE_VERTEX;
    entry.type = GFX_BINDING_TYPE_BUFFER;

    GfxBindGroupLayoutDescriptor desc = {};
    desc.entries = &entry;
    desc.entryCount = 1;

    GfxBindGroupLayout layout = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(nullptr, &desc, &layout);

    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxBindGroupLayoutTest, CreateBindGroupLayoutWithNullDescriptor)
{
    GfxBindGroupLayout layout = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(device, nullptr, &layout);

    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxBindGroupLayoutTest, CreateBindGroupLayoutWithNullOutput)
{
    GfxBindGroupLayoutEntry entry = {};
    entry.binding = 0;
    entry.visibility = GFX_SHADER_STAGE_VERTEX;
    entry.type = GFX_BINDING_TYPE_BUFFER;

    GfxBindGroupLayoutDescriptor desc = {};
    desc.entries = &entry;
    desc.entryCount = 1;

    GfxResult result = gfxDeviceCreateBindGroupLayout(device, &desc, nullptr);

    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxBindGroupLayoutTest, CreateBindGroupLayoutWithUniformBuffer)
{
    GfxBindGroupLayoutEntry entry = {};
    entry.binding = 0;
    entry.visibility = GFX_SHADER_STAGE_VERTEX | GFX_SHADER_STAGE_FRAGMENT;
    entry.type = GFX_BINDING_TYPE_BUFFER;
    entry.buffer.hasDynamicOffset = false;
    entry.buffer.minBindingSize = 256;

    GfxBindGroupLayoutDescriptor desc = {};
    desc.label = "Uniform Buffer Layout";
    desc.entries = &entry;
    desc.entryCount = 1;

    GfxBindGroupLayout layout = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(device, &desc, &layout);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(layout, nullptr);

    if (layout) {
        gfxBindGroupLayoutDestroy(layout);
    }
}

TEST_P(GfxBindGroupLayoutTest, CreateBindGroupLayoutWithSampler)
{
    GfxBindGroupLayoutEntry entry = {};
    entry.binding = 0;
    entry.visibility = GFX_SHADER_STAGE_FRAGMENT;
    entry.type = GFX_BINDING_TYPE_SAMPLER;
    entry.sampler.comparison = false;

    GfxBindGroupLayoutDescriptor desc = {};
    desc.label = "Sampler Layout";
    desc.entries = &entry;
    desc.entryCount = 1;

    GfxBindGroupLayout layout = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(device, &desc, &layout);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(layout, nullptr);

    if (layout) {
        gfxBindGroupLayoutDestroy(layout);
    }
}

TEST_P(GfxBindGroupLayoutTest, CreateBindGroupLayoutWithTexture)
{
    GfxBindGroupLayoutEntry entry = {};
    entry.binding = 0;
    entry.visibility = GFX_SHADER_STAGE_FRAGMENT;
    entry.type = GFX_BINDING_TYPE_TEXTURE;
    entry.texture.sampleType = GFX_TEXTURE_SAMPLE_TYPE_FLOAT;
    entry.texture.viewDimension = GFX_TEXTURE_VIEW_TYPE_2D;
    entry.texture.multisampled = false;

    GfxBindGroupLayoutDescriptor desc = {};
    desc.label = "Texture Layout";
    desc.entries = &entry;
    desc.entryCount = 1;

    GfxBindGroupLayout layout = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(device, &desc, &layout);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(layout, nullptr);

    if (layout) {
        gfxBindGroupLayoutDestroy(layout);
    }
}

TEST_P(GfxBindGroupLayoutTest, CreateBindGroupLayoutWithStorageTexture)
{
    GfxBindGroupLayoutEntry entry = {};
    entry.binding = 0;
    entry.visibility = GFX_SHADER_STAGE_COMPUTE;
    entry.type = GFX_BINDING_TYPE_STORAGE_TEXTURE;
    entry.storageTexture.format = GFX_TEXTURE_FORMAT_R32G32B32A32_FLOAT;
    entry.storageTexture.viewDimension = GFX_TEXTURE_VIEW_TYPE_2D;
    entry.storageTexture.writeOnly = true;

    GfxBindGroupLayoutDescriptor desc = {};
    desc.label = "Storage Texture Layout";
    desc.entries = &entry;
    desc.entryCount = 1;

    GfxBindGroupLayout layout = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(device, &desc, &layout);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(layout, nullptr);

    if (layout) {
        gfxBindGroupLayoutDestroy(layout);
    }
}

TEST_P(GfxBindGroupLayoutTest, CreateBindGroupLayoutWithMultipleEntries)
{
    GfxBindGroupLayoutEntry entries[3] = {};

    // Uniform buffer at binding 0
    entries[0].binding = 0;
    entries[0].visibility = GFX_SHADER_STAGE_VERTEX;
    entries[0].type = GFX_BINDING_TYPE_BUFFER;
    entries[0].buffer.hasDynamicOffset = false;
    entries[0].buffer.minBindingSize = 256;

    // Texture at binding 1
    entries[1].binding = 1;
    entries[1].visibility = GFX_SHADER_STAGE_FRAGMENT;
    entries[1].type = GFX_BINDING_TYPE_TEXTURE;
    entries[1].texture.sampleType = GFX_TEXTURE_SAMPLE_TYPE_FLOAT;
    entries[1].texture.viewDimension = GFX_TEXTURE_VIEW_TYPE_2D;
    entries[1].texture.multisampled = false;

    // Sampler at binding 2
    entries[2].binding = 2;
    entries[2].visibility = GFX_SHADER_STAGE_FRAGMENT;
    entries[2].type = GFX_BINDING_TYPE_SAMPLER;
    entries[2].sampler.comparison = false;

    GfxBindGroupLayoutDescriptor desc = {};
    desc.label = "Multi-Entry Layout";
    desc.entries = entries;
    desc.entryCount = 3;

    GfxBindGroupLayout layout = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(device, &desc, &layout);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(layout, nullptr);

    if (layout) {
        gfxBindGroupLayoutDestroy(layout);
    }
}

TEST_P(GfxBindGroupLayoutTest, CreateBindGroupLayoutWithDynamicOffset)
{
    GfxBindGroupLayoutEntry entry = {};
    entry.binding = 0;
    entry.visibility = GFX_SHADER_STAGE_COMPUTE;
    entry.type = GFX_BINDING_TYPE_BUFFER;
    entry.buffer.hasDynamicOffset = true;
    entry.buffer.minBindingSize = 64;

    GfxBindGroupLayoutDescriptor desc = {};
    desc.label = "Dynamic Offset Layout";
    desc.entries = &entry;
    desc.entryCount = 1;

    GfxBindGroupLayout layout = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(device, &desc, &layout);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(layout, nullptr);

    if (layout) {
        gfxBindGroupLayoutDestroy(layout);
    }
}

TEST_P(GfxBindGroupLayoutTest, CreateMultipleBindGroupLayouts)
{
    const int layoutCount = 3;
    GfxBindGroupLayout layouts[layoutCount] = {};

    for (int i = 0; i < layoutCount; ++i) {
        GfxBindGroupLayoutEntry entry = {};
        entry.binding = 0;
        entry.visibility = GFX_SHADER_STAGE_COMPUTE;
        entry.type = GFX_BINDING_TYPE_BUFFER;
        entry.buffer.hasDynamicOffset = false;
        entry.buffer.minBindingSize = 0;

        GfxBindGroupLayoutDescriptor desc = {};
        desc.entries = &entry;
        desc.entryCount = 1;

        GfxResult result = gfxDeviceCreateBindGroupLayout(device, &desc, &layouts[i]);
        EXPECT_EQ(result, GFX_RESULT_SUCCESS);
        EXPECT_NE(layouts[i], nullptr);
    }

    for (int i = 0; i < layoutCount; ++i) {
        if (layouts[i]) {
            gfxBindGroupLayoutDestroy(layouts[i]);
        }
    }
}

TEST_P(GfxBindGroupLayoutTest, DestroyBindGroupLayoutWithNull)
{
    // Destroying NULL should not crash
    GfxResult result = gfxBindGroupLayoutDestroy(nullptr);
    // The API may return an error or succeed - just ensure it doesn't crash
    EXPECT_TRUE(result == GFX_RESULT_SUCCESS || result == GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxBindGroupLayoutTest,
    testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU),
    [](const testing::TestParamInfo<GfxBackend>& info) {
        return info.param == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU";
    });
