#include <core/resource/Texture.h>
#include <core/resource/TextureView.h>
#include <core/system/Device.h>

#include <gfx/gfx.h>

#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

namespace gfx {

class TextureViewImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc = {};
        instanceDesc.backend = backend;
        instanceDesc.applicationName = "TextureViewImplTest";
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

TEST_P(TextureViewImplTest, CreateTextureView)
{
    DeviceImpl deviceWrapper(device);

    // Create texture using DeviceImpl
    TextureDescriptor textureDesc;
    textureDesc.type = TextureType::Texture2D;
    textureDesc.size = { 256, 256, 1 };
    textureDesc.arrayLayerCount = 1;
    textureDesc.format = TextureFormat::R8G8B8A8Unorm;
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = SampleCount::Count1;
    textureDesc.usage = TextureUsage::TextureBinding;

    auto texture = deviceWrapper.createTexture(textureDesc);
    ASSERT_NE(texture, nullptr);

    // Create texture view using C++ API
    TextureViewDescriptor viewDesc;
    viewDesc.format = TextureFormat::R8G8B8A8Unorm;
    viewDesc.viewType = TextureViewType::View2D;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    auto view = texture->createView(viewDesc);
    EXPECT_NE(view, nullptr);
}

TEST_P(TextureViewImplTest, CreateTextureViewWithMipLevel)
{
    DeviceImpl deviceWrapper(device);

    // Create texture with multiple mip levels
    TextureDescriptor textureDesc;
    textureDesc.type = TextureType::Texture2D;
    textureDesc.size = { 256, 256, 1 };
    textureDesc.arrayLayerCount = 1;
    textureDesc.format = TextureFormat::R8G8B8A8Unorm;
    textureDesc.mipLevelCount = 4;
    textureDesc.sampleCount = SampleCount::Count1;
    textureDesc.usage = TextureUsage::TextureBinding;

    auto texture = deviceWrapper.createTexture(textureDesc);
    ASSERT_NE(texture, nullptr);

    // Create texture view for a specific mip level
    TextureViewDescriptor viewDesc;
    viewDesc.format = TextureFormat::R8G8B8A8Unorm;
    viewDesc.viewType = TextureViewType::View2D;
    viewDesc.baseMipLevel = 2;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    auto view = texture->createView(viewDesc);
    EXPECT_NE(view, nullptr);
}

TEST_P(TextureViewImplTest, CreateMultipleViews_SameTexture)
{
    DeviceImpl deviceWrapper(device);

    // Create texture
    TextureDescriptor textureDesc;
    textureDesc.type = TextureType::Texture2D;
    textureDesc.size = { 256, 256, 1 };
    textureDesc.arrayLayerCount = 1;
    textureDesc.format = TextureFormat::R8G8B8A8Unorm;
    textureDesc.mipLevelCount = 4;
    textureDesc.sampleCount = SampleCount::Count1;
    textureDesc.usage = TextureUsage::TextureBinding;

    auto texture = deviceWrapper.createTexture(textureDesc);
    ASSERT_NE(texture, nullptr);

    // Create first view
    TextureViewDescriptor viewDesc1;
    viewDesc1.format = TextureFormat::R8G8B8A8Unorm;
    viewDesc1.viewType = TextureViewType::View2D;
    viewDesc1.baseMipLevel = 0;
    viewDesc1.mipLevelCount = 2;
    viewDesc1.baseArrayLayer = 0;
    viewDesc1.arrayLayerCount = 1;

    auto view1 = texture->createView(viewDesc1);
    ASSERT_NE(view1, nullptr);

    // Create second view
    TextureViewDescriptor viewDesc2;
    viewDesc2.format = TextureFormat::R8G8B8A8Unorm;
    viewDesc2.viewType = TextureViewType::View2D;
    viewDesc2.baseMipLevel = 2;
    viewDesc2.mipLevelCount = 2;
    viewDesc2.baseArrayLayer = 0;
    viewDesc2.arrayLayerCount = 1;

    auto view2 = texture->createView(viewDesc2);
    ASSERT_NE(view2, nullptr);

    EXPECT_NE(view1, view2);
}
TEST_P(TextureViewImplTest, CreateView1DArray)
{
    DeviceImpl deviceWrapper(device);

    // Create 1D array texture
    TextureDescriptor textureDesc;
    textureDesc.type = TextureType::Texture1D;
    textureDesc.size = { 256, 1, 1 };
    textureDesc.arrayLayerCount = 4;
    textureDesc.format = TextureFormat::R8G8B8A8Unorm;
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = SampleCount::Count1;
    textureDesc.usage = TextureUsage::TextureBinding;

    auto texture = deviceWrapper.createTexture(textureDesc);
    ASSERT_NE(texture, nullptr);

    // Create 1D array view
    TextureViewDescriptor viewDesc;
    viewDesc.format = TextureFormat::R8G8B8A8Unorm;
    viewDesc.viewType = TextureViewType::View1DArray;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 4;

    auto view = texture->createView(viewDesc);
    EXPECT_NE(view, nullptr);
}

TEST_P(TextureViewImplTest, CreateView2DArray)
{
    DeviceImpl deviceWrapper(device);

    // Create 2D array texture
    TextureDescriptor textureDesc;
    textureDesc.type = TextureType::Texture2D;
    textureDesc.size = { 256, 256, 1 };
    textureDesc.arrayLayerCount = 6;
    textureDesc.format = TextureFormat::R8G8B8A8Unorm;
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = SampleCount::Count1;
    textureDesc.usage = TextureUsage::TextureBinding;

    auto texture = deviceWrapper.createTexture(textureDesc);
    ASSERT_NE(texture, nullptr);

    // Create 2D array view
    TextureViewDescriptor viewDesc;
    viewDesc.format = TextureFormat::R8G8B8A8Unorm;
    viewDesc.viewType = TextureViewType::View2DArray;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 6;

    auto view = texture->createView(viewDesc);
    EXPECT_NE(view, nullptr);
}
TEST_P(TextureViewImplTest, CreateCubeTextureView)
{
    DeviceImpl deviceWrapper(device);

    // Create cube texture
    TextureDescriptor textureDesc;
    textureDesc.type = TextureType::TextureCube;
    textureDesc.size = { 256, 256, 1 };
    textureDesc.arrayLayerCount = 6;
    textureDesc.format = TextureFormat::R8G8B8A8Unorm;
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = SampleCount::Count1;
    textureDesc.usage = TextureUsage::TextureBinding;

    auto texture = deviceWrapper.createTexture(textureDesc);
    ASSERT_NE(texture, nullptr);

    // Create cube view
    TextureViewDescriptor viewDesc;
    viewDesc.format = TextureFormat::R8G8B8A8Unorm;
    viewDesc.viewType = TextureViewType::ViewCube;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 6;

    auto view = texture->createView(viewDesc);
    EXPECT_NE(view, nullptr);
}

#if defined(GFX_ENABLE_VULKAN) && defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(AllBackends, TextureViewImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU));
#elif defined(GFX_ENABLE_VULKAN)
INSTANTIATE_TEST_SUITE_P(VulkanOnly, TextureViewImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN));
#elif defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(WebGPUOnly, TextureViewImplTest,
    ::testing::Values(GFX_BACKEND_WEBGPU));
#endif

} // namespace gfx
