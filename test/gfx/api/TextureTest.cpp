#include <gfx/gfx.h>

#include <gtest/gtest.h>

namespace {

class GfxTextureTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    GfxInstance instance = nullptr;
    GfxAdapter adapter = nullptr;
    GfxDevice device = nullptr;

    void SetUp() override
    {
        GfxBackend backend = GetParam();

        GfxResult result = gfxLoadBackend(backend);
        if (result != GFX_RESULT_SUCCESS) {
            GTEST_SKIP() << "Backend not available";
        }

        const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
        GfxInstanceDescriptor instanceDesc = {};
        instanceDesc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
        instanceDesc.pNext = nullptr;
        instanceDesc.backend = backend;
        instanceDesc.enabledExtensions = extensions;
        instanceDesc.enabledExtensionCount = 1;

        result = gfxCreateInstance(&instanceDesc, &instance);
        ASSERT_EQ(result, GFX_RESULT_SUCCESS);
        ASSERT_NE(instance, nullptr);

        GfxAdapterDescriptor adapterDesc = {};
        adapterDesc.sType = GFX_STRUCTURE_TYPE_ADAPTER_DESCRIPTOR;
        adapterDesc.pNext = nullptr;
        adapterDesc.adapterIndex = 0;
        result = gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter);
        ASSERT_EQ(result, GFX_RESULT_SUCCESS);
        ASSERT_NE(adapter, nullptr);

        GfxDeviceDescriptor deviceDesc = {};
        deviceDesc.sType = GFX_STRUCTURE_TYPE_DEVICE_DESCRIPTOR;
        deviceDesc.pNext = nullptr;
        result = gfxAdapterCreateDevice(adapter, &deviceDesc, &device);
        ASSERT_EQ(result, GFX_RESULT_SUCCESS);
        ASSERT_NE(device, nullptr);
    }

    void TearDown() override
    {
        if (device != nullptr) {
            gfxDeviceDestroy(device);
        }
        if (instance != nullptr) {
            gfxInstanceDestroy(instance);
        }
        gfxUnloadBackend(GetParam());
    }
};

TEST_P(GfxTextureTest, CreateDestroyTexture)
{
    GfxTextureDescriptor desc = {};
    desc.label = "TestTexture";
    desc.type = GFX_TEXTURE_TYPE_2D;
    desc.size = { 256, 256, 1 };
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_FLAGS(GFX_TEXTURE_USAGE_TEXTURE_BINDING | GFX_TEXTURE_USAGE_COPY_DST);

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &desc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(texture, nullptr);

    result = gfxTextureDestroy(texture);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
}

TEST_P(GfxTextureTest, CreateTextureInvalidArguments)
{
    GfxTexture texture = nullptr;

    // Null descriptor
    GfxResult result = gfxDeviceCreateTexture(device, nullptr, &texture);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(texture, nullptr);

    // Null output
    GfxTextureDescriptor desc = {};
    desc.type = GFX_TEXTURE_TYPE_2D;
    desc.size = { 256, 256, 1 };
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING;

    result = gfxDeviceCreateTexture(device, &desc, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxTextureTest, CreateTextureZeroSize)
{
    GfxTextureDescriptor desc = {};
    desc.type = GFX_TEXTURE_TYPE_2D;
    desc.size = { 0, 0, 0 }; // Invalid: zero size
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING;

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &desc, &texture);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(texture, nullptr);
}

TEST_P(GfxTextureTest, CreateTextureNoUsage)
{
    GfxTextureDescriptor desc = {};
    desc.type = GFX_TEXTURE_TYPE_2D;
    desc.size = { 256, 256, 1 };
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_NONE; // Invalid: no usage

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &desc, &texture);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(texture, nullptr);
}

TEST_P(GfxTextureTest, GetTextureInfo)
{
    GfxTextureDescriptor desc = {};
    desc.label = "TestTexture";
    desc.type = GFX_TEXTURE_TYPE_2D;
    desc.size = { 512, 256, 1 };
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING;

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &desc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxTextureInfo info = {};
    result = gfxTextureGetInfo(texture, &info);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_EQ(info.type, GFX_TEXTURE_TYPE_2D);
    EXPECT_EQ(info.size.width, 512u);
    EXPECT_EQ(info.size.height, 256u);
    EXPECT_EQ(info.size.depth, 1u);
    EXPECT_EQ(info.format, GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM);
    EXPECT_EQ(info.usage, GFX_TEXTURE_USAGE_TEXTURE_BINDING);

    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureTest, CreateTextureView)
{
    GfxTextureDescriptor desc = {};
    desc.label = "TestTexture";
    desc.type = GFX_TEXTURE_TYPE_2D;
    desc.size = { 256, 256, 1 };
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING;

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &desc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.label = "TestTextureView";
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    GfxTextureView view = nullptr;
    result = gfxTextureCreateView(texture, &viewDesc, &view);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(view, nullptr);

    gfxTextureViewDestroy(view);
    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureTest, CreateTextureViewInvalidArguments)
{
    GfxTextureDescriptor desc = {};
    desc.type = GFX_TEXTURE_TYPE_2D;
    desc.size = { 256, 256, 1 };
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING;

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &desc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Null output pointer should fail
    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    result = gfxTextureCreateView(texture, &viewDesc, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureTest, CreateTexture1D)
{
    GfxTextureDescriptor desc = {};
    desc.type = GFX_TEXTURE_TYPE_1D;
    desc.size = { 256, 1, 1 };
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING;

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &desc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(texture, nullptr);

    GfxTextureInfo info = {};
    result = gfxTextureGetInfo(texture, &info);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_EQ(info.type, GFX_TEXTURE_TYPE_1D);

    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureTest, CreateTexture3D)
{
    GfxTextureDescriptor desc = {};
    desc.type = GFX_TEXTURE_TYPE_3D;
    desc.size = { 64, 64, 64 };
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING;

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &desc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(texture, nullptr);

    GfxTextureInfo info = {};
    result = gfxTextureGetInfo(texture, &info);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_EQ(info.type, GFX_TEXTURE_TYPE_3D);
    EXPECT_EQ(info.size.depth, 64u);

    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureTest, CreateTextureCube)
{
    GfxTextureDescriptor desc = {};
    desc.type = GFX_TEXTURE_TYPE_CUBE;
    desc.size = { 256, 256, 1 };
    desc.arrayLayerCount = 6; // Cube must have 6 layers
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING;

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &desc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(texture, nullptr);

    GfxTextureInfo info = {};
    result = gfxTextureGetInfo(texture, &info);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    // Note: Some backends may represent cube textures as 2D arrays with 6 layers
    // The important thing is that it has 6 array layers
    EXPECT_EQ(info.arrayLayerCount, 6u);

    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureTest, CreateTextureWithMipmaps)
{
    GfxTextureDescriptor desc = {};
    desc.type = GFX_TEXTURE_TYPE_2D;
    desc.size = { 512, 512, 1 };
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 9; // log2(512) + 1
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_FLAGS(GFX_TEXTURE_USAGE_TEXTURE_BINDING | GFX_TEXTURE_USAGE_COPY_DST);

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &desc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(texture, nullptr);

    GfxTextureInfo info = {};
    result = gfxTextureGetInfo(texture, &info);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_EQ(info.mipLevelCount, 9u);

    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureTest, CreateTextureArray)
{
    GfxTextureDescriptor desc = {};
    desc.type = GFX_TEXTURE_TYPE_2D;
    desc.size = { 256, 256, 1 };
    desc.arrayLayerCount = 8;
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING;

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &desc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(texture, nullptr);

    GfxTextureInfo info = {};
    result = gfxTextureGetInfo(texture, &info);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_EQ(info.arrayLayerCount, 8u);

    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureTest, CreateMultipleTextures)
{
    const uint32_t textureCount = 10;
    GfxTexture textures[textureCount] = {};

    for (uint32_t i = 0; i < textureCount; ++i) {
        GfxTextureDescriptor desc = {};
        desc.type = GFX_TEXTURE_TYPE_2D;
        desc.size = { 128, 128, 1 };
        desc.arrayLayerCount = 1;
        desc.mipLevelCount = 1;
        desc.sampleCount = GFX_SAMPLE_COUNT_1;
        desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
        desc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING;

        GfxResult result = gfxDeviceCreateTexture(device, &desc, &textures[i]);
        ASSERT_EQ(result, GFX_RESULT_SUCCESS);
        ASSERT_NE(textures[i], nullptr);
    }

    // Destroy all textures
    for (uint32_t i = 0; i < textureCount; ++i) {
        GfxResult result = gfxTextureDestroy(textures[i]);
        EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    }
}

TEST_P(GfxTextureTest, CreateTextureWithAllUsageFlags)
{
    GfxTextureDescriptor desc = {};
    desc.type = GFX_TEXTURE_TYPE_2D;
    desc.size = { 256, 256, 1 };
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_FLAGS(GFX_TEXTURE_USAGE_COPY_SRC | GFX_TEXTURE_USAGE_COPY_DST | GFX_TEXTURE_USAGE_TEXTURE_BINDING | GFX_TEXTURE_USAGE_STORAGE_BINDING | GFX_TEXTURE_USAGE_RENDER_ATTACHMENT);

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &desc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(texture, nullptr);

    GfxTextureInfo info = {};
    result = gfxTextureGetInfo(texture, &info);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_EQ(info.usage, desc.usage);

    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureTest, CreateDepthTexture)
{
    GfxTextureDescriptor desc = {};
    desc.type = GFX_TEXTURE_TYPE_2D;
    desc.size = { 512, 512, 1 };
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_DEPTH32_FLOAT;
    desc.usage = GFX_FLAGS(GFX_TEXTURE_USAGE_RENDER_ATTACHMENT | GFX_TEXTURE_USAGE_TEXTURE_BINDING);

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &desc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(texture, nullptr);

    GfxTextureInfo info = {};
    result = gfxTextureGetInfo(texture, &info);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_EQ(info.format, GFX_TEXTURE_FORMAT_DEPTH32_FLOAT);

    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureTest, ImportTextureInvalidArguments)
{
    GfxTextureImportDescriptor desc = {};
    desc.nativeHandle = nullptr; // Invalid handle
    desc.type = GFX_TEXTURE_TYPE_2D;
    desc.size = { 256, 256, 1 };
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING;
    desc.currentLayout = GFX_TEXTURE_LAYOUT_UNDEFINED;

    // NULL device
    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceImportTexture(nullptr, &desc, &texture);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL descriptor
    result = gfxDeviceImportTexture(device, nullptr, &texture);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL output
    result = gfxDeviceImportTexture(device, &desc, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL native handle
    result = gfxDeviceImportTexture(device, &desc, &texture);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxTextureTest, ImportTextureZeroSize)
{
    GfxTextureImportDescriptor desc = {};
    desc.nativeHandle = (void*)0x1; // Dummy non-null pointer
    desc.type = GFX_TEXTURE_TYPE_2D;
    desc.size = { 0, 0, 0 }; // Invalid: zero size
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING;
    desc.currentLayout = GFX_TEXTURE_LAYOUT_UNDEFINED;

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceImportTexture(device, &desc, &texture);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxTextureTest, ImportTextureNoUsage)
{
    GfxTextureImportDescriptor desc = {};
    desc.nativeHandle = (void*)0x1; // Dummy non-null pointer
    desc.type = GFX_TEXTURE_TYPE_2D;
    desc.size = { 256, 256, 1 };
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_NONE; // Invalid: no usage
    desc.currentLayout = GFX_TEXTURE_LAYOUT_UNDEFINED;

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceImportTexture(device, &desc, &texture);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxTextureTest, ImportTextureFromNativeHandle)
{
    // First, create a normal texture
    GfxTextureDescriptor createDesc = {};
    createDesc.label = "Source Texture";
    createDesc.type = GFX_TEXTURE_TYPE_2D;
    createDesc.size = { 256, 256, 1 };
    createDesc.arrayLayerCount = 1;
    createDesc.mipLevelCount = 1;
    createDesc.sampleCount = GFX_SAMPLE_COUNT_1;
    createDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    createDesc.usage = GFX_FLAGS(GFX_TEXTURE_USAGE_COPY_SRC | GFX_TEXTURE_USAGE_COPY_DST);

    GfxTexture sourceTexture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &createDesc, &sourceTexture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(sourceTexture, nullptr);

    // Get texture info to verify properties
    GfxTextureInfo info = {};
    result = gfxTextureGetInfo(sourceTexture, &info);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Extract native handle using the API
    void* nativeHandle = nullptr;
    result = gfxTextureGetNativeHandle(sourceTexture, &nativeHandle);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(nativeHandle, nullptr);

    // Now import the native handle
    GfxTextureImportDescriptor importDesc = {};
    importDesc.nativeHandle = nativeHandle;
    importDesc.type = info.type;
    importDesc.size = info.size;
    importDesc.arrayLayerCount = info.arrayLayerCount;
    importDesc.mipLevelCount = info.mipLevelCount;
    importDesc.sampleCount = info.sampleCount;
    importDesc.format = info.format;
    importDesc.usage = info.usage;
    importDesc.currentLayout = GFX_TEXTURE_LAYOUT_UNDEFINED;

    GfxTexture importedTexture = nullptr;
    result = gfxDeviceImportTexture(device, &importDesc, &importedTexture);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(importedTexture, nullptr);

    // Verify imported texture has correct properties
    if (importedTexture) {
        GfxTextureInfo importedInfo = {};
        result = gfxTextureGetInfo(importedTexture, &importedInfo);
        EXPECT_EQ(result, GFX_RESULT_SUCCESS);
        EXPECT_EQ(importedInfo.type, info.type);
        EXPECT_EQ(importedInfo.size.width, info.size.width);
        EXPECT_EQ(importedInfo.size.height, info.size.height);
        EXPECT_EQ(importedInfo.size.depth, info.size.depth);
        EXPECT_EQ(importedInfo.arrayLayerCount, info.arrayLayerCount);
        EXPECT_EQ(importedInfo.mipLevelCount, info.mipLevelCount);
        EXPECT_EQ(importedInfo.sampleCount, info.sampleCount);
        EXPECT_EQ(importedInfo.format, info.format);
        EXPECT_EQ(importedInfo.usage, info.usage);

        // Clean up imported texture (doesn't own the native handle)
        gfxTextureDestroy(importedTexture);
    }

    // Clean up source texture
    gfxTextureDestroy(sourceTexture);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    GfxTextureTests,
    GfxTextureTest,
    ::testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU),
    [](const ::testing::TestParamInfo<GfxBackend>& info) {
        return info.param == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU";
    });

} // namespace
