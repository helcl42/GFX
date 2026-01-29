#include <gfx/gfx.h>

#include <gtest/gtest.h>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

class GfxBindGroupTest : public testing::TestWithParam<GfxBackend> {
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
// BindGroup Tests
// ===========================================================================

// Test: Create BindGroup with NULL device
TEST_P(GfxBindGroupTest, CreateBindGroupWithNullDevice)
{
    GfxBindGroupLayoutEntry layoutEntry = {};
    layoutEntry.binding = 0;
    layoutEntry.visibility = GFX_SHADER_STAGE_VERTEX;
    layoutEntry.type = GFX_BINDING_TYPE_BUFFER;
    layoutEntry.buffer.hasDynamicOffset = false;
    layoutEntry.buffer.minBindingSize = 0;

    GfxBindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.entries = &layoutEntry;
    layoutDesc.entryCount = 1;

    GfxBindGroupLayout layout = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(device, &layoutDesc, &layout);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(layout, nullptr);

    GfxBufferDescriptor bufferDesc = {};
    bufferDesc.size = 256;
    bufferDesc.usage = GFX_BUFFER_USAGE_UNIFORM;
    bufferDesc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT;

    GfxBuffer buffer = nullptr;
    result = gfxDeviceCreateBuffer(device, &bufferDesc, &buffer);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(buffer, nullptr);

    GfxBindGroupEntry entry = {};
    entry.binding = 0;
    entry.type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER;
    entry.resource.buffer.buffer = buffer;
    entry.resource.buffer.offset = 0;
    entry.resource.buffer.size = 256;

    GfxBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = layout;
    bindGroupDesc.entries = &entry;
    bindGroupDesc.entryCount = 1;

    GfxBindGroup bindGroup = nullptr;
    result = gfxDeviceCreateBindGroup(nullptr, &bindGroupDesc, &bindGroup);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxBufferDestroy(buffer);
    gfxBindGroupLayoutDestroy(layout);
}

// Test: Create BindGroup with NULL descriptor
TEST_P(GfxBindGroupTest, CreateBindGroupWithNullDescriptor)
{
    GfxBindGroup bindGroup = nullptr;
    GfxResult result = gfxDeviceCreateBindGroup(device, nullptr, &bindGroup);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Test: Create BindGroup with NULL output
TEST_P(GfxBindGroupTest, CreateBindGroupWithNullOutput)
{
    GfxBindGroupLayoutEntry layoutEntry = {};
    layoutEntry.binding = 0;
    layoutEntry.visibility = GFX_SHADER_STAGE_VERTEX;
    layoutEntry.type = GFX_BINDING_TYPE_BUFFER;
    layoutEntry.buffer.hasDynamicOffset = false;
    layoutEntry.buffer.minBindingSize = 0;

    GfxBindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.entries = &layoutEntry;
    layoutDesc.entryCount = 1;

    GfxBindGroupLayout layout = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(device, &layoutDesc, &layout);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(layout, nullptr);

    GfxBufferDescriptor bufferDesc = {};
    bufferDesc.size = 256;
    bufferDesc.usage = GFX_BUFFER_USAGE_UNIFORM;
    bufferDesc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT;

    GfxBuffer buffer = nullptr;
    result = gfxDeviceCreateBuffer(device, &bufferDesc, &buffer);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(buffer, nullptr);

    GfxBindGroupEntry entry = {};
    entry.binding = 0;
    entry.type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER;
    entry.resource.buffer.buffer = buffer;
    entry.resource.buffer.offset = 0;
    entry.resource.buffer.size = 256;

    GfxBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = layout;
    bindGroupDesc.entries = &entry;
    bindGroupDesc.entryCount = 1;

    result = gfxDeviceCreateBindGroup(device, &bindGroupDesc, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxBufferDestroy(buffer);
    gfxBindGroupLayoutDestroy(layout);
}

// Test: Create BindGroup with uniform buffer
TEST_P(GfxBindGroupTest, CreateBindGroupWithUniformBuffer)
{
    GfxBindGroupLayoutEntry layoutEntry = {};
    layoutEntry.binding = 0;
    layoutEntry.visibility = GFX_SHADER_STAGE_VERTEX | GFX_SHADER_STAGE_FRAGMENT;
    layoutEntry.type = GFX_BINDING_TYPE_BUFFER;
    layoutEntry.buffer.hasDynamicOffset = false;
    layoutEntry.buffer.minBindingSize = 0;

    GfxBindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.entries = &layoutEntry;
    layoutDesc.entryCount = 1;

    GfxBindGroupLayout layout = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(device, &layoutDesc, &layout);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(layout, nullptr);

    GfxBufferDescriptor bufferDesc = {};
    bufferDesc.size = 256;
    bufferDesc.usage = GFX_BUFFER_USAGE_UNIFORM;
    bufferDesc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT;

    GfxBuffer buffer = nullptr;
    result = gfxDeviceCreateBuffer(device, &bufferDesc, &buffer);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(buffer, nullptr);

    GfxBindGroupEntry entry = {};
    entry.binding = 0;
    entry.type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER;
    entry.resource.buffer.buffer = buffer;
    entry.resource.buffer.offset = 0;
    entry.resource.buffer.size = 256;

    GfxBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = layout;
    bindGroupDesc.entries = &entry;
    bindGroupDesc.entryCount = 1;

    GfxBindGroup bindGroup = nullptr;
    result = gfxDeviceCreateBindGroup(device, &bindGroupDesc, &bindGroup);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(bindGroup, nullptr);

    gfxBindGroupDestroy(bindGroup);
    gfxBufferDestroy(buffer);
    gfxBindGroupLayoutDestroy(layout);
}

// Test: Create BindGroup with sampler
TEST_P(GfxBindGroupTest, CreateBindGroupWithSampler)
{
    GfxBindGroupLayoutEntry layoutEntry = {};
    layoutEntry.binding = 0;
    layoutEntry.visibility = GFX_SHADER_STAGE_FRAGMENT;
    layoutEntry.type = GFX_BINDING_TYPE_SAMPLER;
    layoutEntry.sampler.comparison = false;

    GfxBindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.entries = &layoutEntry;
    layoutDesc.entryCount = 1;

    GfxBindGroupLayout layout = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(device, &layoutDesc, &layout);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(layout, nullptr);

    GfxSamplerDescriptor samplerDesc = {};
    samplerDesc.addressModeU = GFX_ADDRESS_MODE_REPEAT;
    samplerDesc.addressModeV = GFX_ADDRESS_MODE_REPEAT;
    samplerDesc.addressModeW = GFX_ADDRESS_MODE_REPEAT;
    samplerDesc.magFilter = GFX_FILTER_MODE_LINEAR;
    samplerDesc.minFilter = GFX_FILTER_MODE_LINEAR;
    samplerDesc.mipmapFilter = GFX_FILTER_MODE_LINEAR;

    GfxSampler sampler = nullptr;
    result = gfxDeviceCreateSampler(device, &samplerDesc, &sampler);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(sampler, nullptr);

    GfxBindGroupEntry entry = {};
    entry.binding = 0;
    entry.type = GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER;
    entry.resource.sampler = sampler;

    GfxBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = layout;
    bindGroupDesc.entries = &entry;
    bindGroupDesc.entryCount = 1;

    GfxBindGroup bindGroup = nullptr;
    result = gfxDeviceCreateBindGroup(device, &bindGroupDesc, &bindGroup);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(bindGroup, nullptr);

    gfxBindGroupDestroy(bindGroup);
    gfxSamplerDestroy(sampler);
    gfxBindGroupLayoutDestroy(layout);
}

// Test: Create BindGroup with texture view
TEST_P(GfxBindGroupTest, CreateBindGroupWithTextureView)
{
    GfxBindGroupLayoutEntry layoutEntry = {};
    layoutEntry.binding = 0;
    layoutEntry.visibility = GFX_SHADER_STAGE_FRAGMENT;
    layoutEntry.type = GFX_BINDING_TYPE_TEXTURE;
    layoutEntry.texture.sampleType = GFX_TEXTURE_SAMPLE_TYPE_FLOAT;
    layoutEntry.texture.viewDimension = GFX_TEXTURE_VIEW_TYPE_2D;
    layoutEntry.texture.multisampled = false;

    GfxBindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.entries = &layoutEntry;
    layoutDesc.entryCount = 1;

    GfxBindGroupLayout layout = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(device, &layoutDesc, &layout);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(layout, nullptr);

    GfxTextureDescriptor textureDesc = {};
    textureDesc.type = GFX_TEXTURE_TYPE_2D;
    textureDesc.size = { 256, 256, 1 };
    textureDesc.arrayLayerCount = 1;
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = GFX_SAMPLE_COUNT_1;
    textureDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    textureDesc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING;

    GfxTexture texture = nullptr;
    result = gfxDeviceCreateTexture(device, &textureDesc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(texture, nullptr);

    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    GfxTextureView textureView = nullptr;
    result = gfxTextureCreateView(texture, &viewDesc, &textureView);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(textureView, nullptr);

    GfxBindGroupEntry entry = {};
    entry.binding = 0;
    entry.type = GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW;
    entry.resource.textureView = textureView;

    GfxBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = layout;
    bindGroupDesc.entries = &entry;
    bindGroupDesc.entryCount = 1;

    GfxBindGroup bindGroup = nullptr;
    result = gfxDeviceCreateBindGroup(device, &bindGroupDesc, &bindGroup);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(bindGroup, nullptr);

    gfxBindGroupDestroy(bindGroup);
    gfxTextureViewDestroy(textureView);
    gfxTextureDestroy(texture);
    gfxBindGroupLayoutDestroy(layout);
}

// Test: Create BindGroup with storage buffer
TEST_P(GfxBindGroupTest, CreateBindGroupWithStorageBuffer)
{
    GfxBindGroupLayoutEntry layoutEntry = {};
    layoutEntry.binding = 0;
    layoutEntry.visibility = GFX_SHADER_STAGE_COMPUTE;
    layoutEntry.type = GFX_BINDING_TYPE_BUFFER;
    layoutEntry.buffer.hasDynamicOffset = false;
    layoutEntry.buffer.minBindingSize = 0;

    GfxBindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.entries = &layoutEntry;
    layoutDesc.entryCount = 1;

    GfxBindGroupLayout layout = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(device, &layoutDesc, &layout);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(layout, nullptr);

    GfxBufferDescriptor bufferDesc = {};
    bufferDesc.size = 1024;
    bufferDesc.usage = GFX_BUFFER_USAGE_STORAGE;
    bufferDesc.memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL;

    GfxBuffer buffer = nullptr;
    result = gfxDeviceCreateBuffer(device, &bufferDesc, &buffer);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(buffer, nullptr);

    GfxBindGroupEntry entry = {};
    entry.binding = 0;
    entry.type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER;
    entry.resource.buffer.buffer = buffer;
    entry.resource.buffer.offset = 0;
    entry.resource.buffer.size = 1024;

    GfxBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = layout;
    bindGroupDesc.entries = &entry;
    bindGroupDesc.entryCount = 1;

    GfxBindGroup bindGroup = nullptr;
    result = gfxDeviceCreateBindGroup(device, &bindGroupDesc, &bindGroup);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(bindGroup, nullptr);

    gfxBindGroupDestroy(bindGroup);
    gfxBufferDestroy(buffer);
    gfxBindGroupLayoutDestroy(layout);
}

// Test: Create BindGroup with multiple entries
TEST_P(GfxBindGroupTest, CreateBindGroupWithMultipleEntries)
{
    GfxBindGroupLayoutEntry layoutEntries[3] = {};

    // Binding 0: Uniform buffer
    layoutEntries[0].binding = 0;
    layoutEntries[0].visibility = GFX_SHADER_STAGE_VERTEX;
    layoutEntries[0].type = GFX_BINDING_TYPE_BUFFER;
    layoutEntries[0].buffer.hasDynamicOffset = false;
    layoutEntries[0].buffer.minBindingSize = 0;

    // Binding 1: Sampler
    layoutEntries[1].binding = 1;
    layoutEntries[1].visibility = GFX_SHADER_STAGE_FRAGMENT;
    layoutEntries[1].type = GFX_BINDING_TYPE_SAMPLER;
    layoutEntries[1].sampler.comparison = false;

    // Binding 2: Texture
    layoutEntries[2].binding = 2;
    layoutEntries[2].visibility = GFX_SHADER_STAGE_FRAGMENT;
    layoutEntries[2].type = GFX_BINDING_TYPE_TEXTURE;
    layoutEntries[2].texture.sampleType = GFX_TEXTURE_SAMPLE_TYPE_FLOAT;
    layoutEntries[2].texture.viewDimension = GFX_TEXTURE_VIEW_TYPE_2D;
    layoutEntries[2].texture.multisampled = false;

    GfxBindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.entries = layoutEntries;
    layoutDesc.entryCount = 3;

    GfxBindGroupLayout layout = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(device, &layoutDesc, &layout);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(layout, nullptr);

    // Create resources
    GfxBufferDescriptor bufferDesc = {};
    bufferDesc.size = 256;
    bufferDesc.usage = GFX_BUFFER_USAGE_UNIFORM;
    bufferDesc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT;

    GfxBuffer buffer = nullptr;
    result = gfxDeviceCreateBuffer(device, &bufferDesc, &buffer);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxSamplerDescriptor samplerDesc = {};
    samplerDesc.addressModeU = GFX_ADDRESS_MODE_REPEAT;
    samplerDesc.addressModeV = GFX_ADDRESS_MODE_REPEAT;
    samplerDesc.addressModeW = GFX_ADDRESS_MODE_REPEAT;
    samplerDesc.magFilter = GFX_FILTER_MODE_LINEAR;
    samplerDesc.minFilter = GFX_FILTER_MODE_LINEAR;
    samplerDesc.mipmapFilter = GFX_FILTER_MODE_LINEAR;

    GfxSampler sampler = nullptr;
    result = gfxDeviceCreateSampler(device, &samplerDesc, &sampler);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxTextureDescriptor textureDesc = {};
    textureDesc.type = GFX_TEXTURE_TYPE_2D;
    textureDesc.size = { 256, 256, 1 };
    textureDesc.arrayLayerCount = 1;
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = GFX_SAMPLE_COUNT_1;
    textureDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    textureDesc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING;

    GfxTexture texture = nullptr;
    result = gfxDeviceCreateTexture(device, &textureDesc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    GfxTextureView textureView = nullptr;
    result = gfxTextureCreateView(texture, &viewDesc, &textureView);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create bind group entries
    GfxBindGroupEntry entries[3] = {};

    entries[0].binding = 0;
    entries[0].type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER;
    entries[0].resource.buffer.buffer = buffer;
    entries[0].resource.buffer.offset = 0;
    entries[0].resource.buffer.size = 256;

    entries[1].binding = 1;
    entries[1].type = GFX_BIND_GROUP_ENTRY_TYPE_SAMPLER;
    entries[1].resource.sampler = sampler;

    entries[2].binding = 2;
    entries[2].type = GFX_BIND_GROUP_ENTRY_TYPE_TEXTURE_VIEW;
    entries[2].resource.textureView = textureView;

    GfxBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = layout;
    bindGroupDesc.entries = entries;
    bindGroupDesc.entryCount = 3;

    GfxBindGroup bindGroup = nullptr;
    result = gfxDeviceCreateBindGroup(device, &bindGroupDesc, &bindGroup);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(bindGroup, nullptr);

    gfxBindGroupDestroy(bindGroup);
    gfxTextureViewDestroy(textureView);
    gfxTextureDestroy(texture);
    gfxSamplerDestroy(sampler);
    gfxBufferDestroy(buffer);
    gfxBindGroupLayoutDestroy(layout);
}

// Test: Create BindGroup with buffer offset
TEST_P(GfxBindGroupTest, CreateBindGroupWithBufferOffset)
{
    GfxBindGroupLayoutEntry layoutEntry = {};
    layoutEntry.binding = 0;
    layoutEntry.visibility = GFX_SHADER_STAGE_VERTEX;
    layoutEntry.type = GFX_BINDING_TYPE_BUFFER;
    layoutEntry.buffer.hasDynamicOffset = false;
    layoutEntry.buffer.minBindingSize = 0;

    GfxBindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.entries = &layoutEntry;
    layoutDesc.entryCount = 1;

    GfxBindGroupLayout layout = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(device, &layoutDesc, &layout);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(layout, nullptr);

    GfxBufferDescriptor bufferDesc = {};
    bufferDesc.size = 512;
    bufferDesc.usage = GFX_BUFFER_USAGE_UNIFORM;
    bufferDesc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT;

    GfxBuffer buffer = nullptr;
    result = gfxDeviceCreateBuffer(device, &bufferDesc, &buffer);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(buffer, nullptr);

    GfxBindGroupEntry entry = {};
    entry.binding = 0;
    entry.type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER;
    entry.resource.buffer.buffer = buffer;
    entry.resource.buffer.offset = 256; // Offset into buffer
    entry.resource.buffer.size = 256;

    GfxBindGroupDescriptor bindGroupDesc = {};
    bindGroupDesc.layout = layout;
    bindGroupDesc.entries = &entry;
    bindGroupDesc.entryCount = 1;

    GfxBindGroup bindGroup = nullptr;
    result = gfxDeviceCreateBindGroup(device, &bindGroupDesc, &bindGroup);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(bindGroup, nullptr);

    gfxBindGroupDestroy(bindGroup);
    gfxBufferDestroy(buffer);
    gfxBindGroupLayoutDestroy(layout);
}

// Test: Create multiple BindGroups with same layout
TEST_P(GfxBindGroupTest, CreateMultipleBindGroupsWithSameLayout)
{
    GfxBindGroupLayoutEntry layoutEntry = {};
    layoutEntry.binding = 0;
    layoutEntry.visibility = GFX_SHADER_STAGE_VERTEX;
    layoutEntry.type = GFX_BINDING_TYPE_BUFFER;
    layoutEntry.buffer.hasDynamicOffset = false;
    layoutEntry.buffer.minBindingSize = 0;

    GfxBindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.entries = &layoutEntry;
    layoutDesc.entryCount = 1;

    GfxBindGroupLayout layout = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(device, &layoutDesc, &layout);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(layout, nullptr);

    // Create two buffers
    GfxBufferDescriptor bufferDesc = {};
    bufferDesc.size = 256;
    bufferDesc.usage = GFX_BUFFER_USAGE_UNIFORM;
    bufferDesc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT;

    GfxBuffer buffer1 = nullptr;
    result = gfxDeviceCreateBuffer(device, &bufferDesc, &buffer1);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxBuffer buffer2 = nullptr;
    result = gfxDeviceCreateBuffer(device, &bufferDesc, &buffer2);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create first bind group
    GfxBindGroupEntry entry1 = {};
    entry1.binding = 0;
    entry1.type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER;
    entry1.resource.buffer.buffer = buffer1;
    entry1.resource.buffer.offset = 0;
    entry1.resource.buffer.size = 256;

    GfxBindGroupDescriptor bindGroupDesc1 = {};
    bindGroupDesc1.layout = layout;
    bindGroupDesc1.entries = &entry1;
    bindGroupDesc1.entryCount = 1;

    GfxBindGroup bindGroup1 = nullptr;
    result = gfxDeviceCreateBindGroup(device, &bindGroupDesc1, &bindGroup1);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(bindGroup1, nullptr);

    // Create second bind group
    GfxBindGroupEntry entry2 = {};
    entry2.binding = 0;
    entry2.type = GFX_BIND_GROUP_ENTRY_TYPE_BUFFER;
    entry2.resource.buffer.buffer = buffer2;
    entry2.resource.buffer.offset = 0;
    entry2.resource.buffer.size = 256;

    GfxBindGroupDescriptor bindGroupDesc2 = {};
    bindGroupDesc2.layout = layout;
    bindGroupDesc2.entries = &entry2;
    bindGroupDesc2.entryCount = 1;

    GfxBindGroup bindGroup2 = nullptr;
    result = gfxDeviceCreateBindGroup(device, &bindGroupDesc2, &bindGroup2);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(bindGroup2, nullptr);

    gfxBindGroupDestroy(bindGroup1);
    gfxBindGroupDestroy(bindGroup2);
    gfxBufferDestroy(buffer1);
    gfxBufferDestroy(buffer2);
    gfxBindGroupLayoutDestroy(layout);
}

// Test: Destroy NULL BindGroup
TEST_P(GfxBindGroupTest, DestroyNullBindGroup)
{
    GfxResult result = gfxBindGroupDestroy(nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT); // NULL destroy returns invalid argument
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxBindGroupTest,
    testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU),
    [](const testing::TestParamInfo<GfxBackend>& info) {
        return info.param == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU";
    });
