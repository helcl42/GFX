#include "CommonTest.h"

namespace {

class GfxTextureViewTest : public ::testing::TestWithParam<GfxBackend> {
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

    // Helper to create a basic 2D texture
    GfxTexture createBasicTexture()
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
        EXPECT_EQ(result, GFX_RESULT_SUCCESS);
        return texture;
    }
};

TEST_P(GfxTextureViewTest, CreateDestroy2DView)
{
    GfxTexture texture = createBasicTexture();
    ASSERT_NE(texture, nullptr);

    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.label = "Test2DView";
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    GfxTextureView view = nullptr;
    GfxResult result = gfxTextureCreateView(texture, &viewDesc, &view);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(view, nullptr);

    result = gfxTextureViewDestroy(view);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);

    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureViewTest, CreateViewWithNullDescriptor)
{
    GfxTexture texture = createBasicTexture();
    ASSERT_NE(texture, nullptr);

    // NULL descriptor should be rejected
    GfxTextureView view = nullptr;
    GfxResult result = gfxTextureCreateView(texture, nullptr, &view);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureViewTest, CreateViewInvalidArguments)
{
    GfxTexture texture = createBasicTexture();
    ASSERT_NE(texture, nullptr);

    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    // Null output pointer
    GfxResult result = gfxTextureCreateView(texture, &viewDesc, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // Null texture
    GfxTextureView view = nullptr;
    result = gfxTextureCreateView(nullptr, &viewDesc, &view);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureViewTest, CreateView1D)
{
    GfxTextureDescriptor desc = {};
    desc.type = GFX_TEXTURE_TYPE_1D;
    desc.size = { 512, 1, 1 };
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING;

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &desc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_1D;
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

TEST_P(GfxTextureViewTest, CreateView3D)
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

    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_3D;
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

TEST_P(GfxTextureViewTest, CreateViewCube)
{
    GfxTextureDescriptor desc = {};
    desc.type = GFX_TEXTURE_TYPE_CUBE;
    desc.size = { 256, 256, 1 };
    desc.arrayLayerCount = 6; // Cube requires 6 layers
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING;

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &desc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_CUBE;
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 6;

    GfxTextureView view = nullptr;
    result = gfxTextureCreateView(texture, &viewDesc, &view);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(view, nullptr);

    gfxTextureViewDestroy(view);
    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureViewTest, CreateView1DArray)
{
    GfxTextureDescriptor desc = {};
    desc.type = GFX_TEXTURE_TYPE_1D;
    desc.size = { 512, 1, 1 };
    desc.arrayLayerCount = 4;
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING;

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &desc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_1D_ARRAY;
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 4;

    GfxTextureView view = nullptr;
    result = gfxTextureCreateView(texture, &viewDesc, &view);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(view, nullptr);

    gfxTextureViewDestroy(view);
    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureViewTest, CreateView2DArray)
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

    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D_ARRAY;
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 8;

    GfxTextureView view = nullptr;
    result = gfxTextureCreateView(texture, &viewDesc, &view);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(view, nullptr);

    gfxTextureViewDestroy(view);
    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureViewTest, CreateViewCubeArray)
{
    GfxTextureDescriptor desc = {};
    desc.type = GFX_TEXTURE_TYPE_CUBE;
    desc.size = { 256, 256, 1 };
    desc.arrayLayerCount = 12; // 2 cubes = 12 layers
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING;

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &desc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_CUBE_ARRAY;
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 12;

    GfxTextureView view = nullptr;
    result = gfxTextureCreateView(texture, &viewDesc, &view);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(view, nullptr);

    gfxTextureViewDestroy(view);
    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureViewTest, CreateViewSpecificMipLevel)
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

    // Create view for mip level 3 only
    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.baseMipLevel = 3;
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

TEST_P(GfxTextureViewTest, CreateViewMipLevelRange)
{
    GfxTextureDescriptor desc = {};
    desc.type = GFX_TEXTURE_TYPE_2D;
    desc.size = { 512, 512, 1 };
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 9;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_FLAGS(GFX_TEXTURE_USAGE_TEXTURE_BINDING | GFX_TEXTURE_USAGE_COPY_DST);

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &desc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create view for mip levels 2-5
    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.baseMipLevel = 2;
    viewDesc.mipLevelCount = 4; // levels 2, 3, 4, 5
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    GfxTextureView view = nullptr;
    result = gfxTextureCreateView(texture, &viewDesc, &view);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(view, nullptr);

    gfxTextureViewDestroy(view);
    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureViewTest, CreateViewSpecificArrayLayer)
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

    // Create 2D view of layer 3
    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 3;
    viewDesc.arrayLayerCount = 1;

    GfxTextureView view = nullptr;
    result = gfxTextureCreateView(texture, &viewDesc, &view);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(view, nullptr);

    gfxTextureViewDestroy(view);
    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureViewTest, CreateViewArrayLayerRange)
{
    GfxTextureDescriptor desc = {};
    desc.type = GFX_TEXTURE_TYPE_2D;
    desc.size = { 256, 256, 1 };
    desc.arrayLayerCount = 10;
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING;

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &desc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create 2D array view of layers 3-6
    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D_ARRAY;
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 3;
    viewDesc.arrayLayerCount = 4; // layers 3, 4, 5, 6

    GfxTextureView view = nullptr;
    result = gfxTextureCreateView(texture, &viewDesc, &view);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(view, nullptr);

    gfxTextureViewDestroy(view);
    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureViewTest, CreateMultipleViewsOfSameTexture)
{
    GfxTexture texture = createBasicTexture();
    ASSERT_NE(texture, nullptr);

    const int viewCount = 5;
    GfxTextureView views[viewCount] = {};

    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    // Create multiple views of the same texture
    for (int i = 0; i < viewCount; ++i) {
        GfxResult result = gfxTextureCreateView(texture, &viewDesc, &views[i]);
        ASSERT_EQ(result, GFX_RESULT_SUCCESS);
        ASSERT_NE(views[i], nullptr);
    }

    // Destroy all views
    for (int i = 0; i < viewCount; ++i) {
        GfxResult result = gfxTextureViewDestroy(views[i]);
        EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    }

    gfxTextureDestroy(texture);
}

TEST_P(GfxTextureViewTest, CreateViewForDepthTexture)
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

    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    viewDesc.format = GFX_TEXTURE_FORMAT_DEPTH32_FLOAT;
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

TEST_P(GfxTextureViewTest, CreateView2DFromSingleCubeFace)
{
    GfxTextureDescriptor desc = {};
    desc.type = GFX_TEXTURE_TYPE_CUBE;
    desc.size = { 256, 256, 1 };
    desc.arrayLayerCount = 6;
    desc.mipLevelCount = 1;
    desc.sampleCount = GFX_SAMPLE_COUNT_1;
    desc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.usage = GFX_TEXTURE_USAGE_TEXTURE_BINDING;

    GfxTexture texture = nullptr;
    GfxResult result = gfxDeviceCreateTexture(device, &desc, &texture);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create a 2D view of face 2 (one face of the cube)
    GfxTextureViewDescriptor viewDesc = {};
    viewDesc.viewType = GFX_TEXTURE_VIEW_TYPE_2D;
    viewDesc.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 2;
    viewDesc.arrayLayerCount = 1;

    GfxTextureView view = nullptr;
    result = gfxTextureCreateView(texture, &viewDesc, &view);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(view, nullptr);

    gfxTextureViewDestroy(view);
    gfxTextureDestroy(texture);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxTextureViewTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

} // namespace
