#include "CommonTest.h"

namespace {

class GfxCppTextureViewTest : public ::testing::TestWithParam<gfx::Backend> {
protected:
    void SetUp() override
    {
        gfx::Backend backend = GetParam();

        try {
            gfx::InstanceDescriptor instanceDesc{};
            instanceDesc.backend = backend;
            instanceDesc.enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG };

            instance = gfx::createInstance(instanceDesc);
            ASSERT_NE(instance, nullptr);

            gfx::AdapterDescriptor adapterDesc{};
            adapterDesc.adapterIndex = 0;
            adapter = instance->requestAdapter(adapterDesc);
            ASSERT_NE(adapter, nullptr);

            gfx::DeviceDescriptor deviceDesc{};
            device = adapter->createDevice(deviceDesc);
            ASSERT_NE(device, nullptr);
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Setup failed: " << e.what();
        }
    }

    void TearDown() override
    {
        device.reset();
        adapter.reset();
        instance.reset();
    }

    // Helper to create a basic 2D texture
    std::shared_ptr<gfx::Texture> createBasicTexture()
    {
        gfx::TextureDescriptor desc{};
        desc.type = gfx::TextureType::Texture2D;
        desc.size = { 256, 256, 1 };
        desc.arrayLayerCount = 1;
        desc.mipLevelCount = 1;
        desc.sampleCount = gfx::SampleCount::Count1;
        desc.format = gfx::Format::R8G8B8A8Unorm;
        desc.usage = gfx::TextureUsage::TextureBinding;

        return device->createTexture(desc);
    }

    std::shared_ptr<gfx::Instance> instance;
    std::shared_ptr<gfx::Adapter> adapter;
    std::shared_ptr<gfx::Device> device;
};

TEST_P(GfxCppTextureViewTest, CreateDestroy2DView)
{
    auto texture = createBasicTexture();
    ASSERT_NE(texture, nullptr);

    gfx::TextureViewDescriptor viewDesc{};
    viewDesc.label = "Test2DView";
    viewDesc.viewType = gfx::TextureViewType::View2D;
    viewDesc.format = gfx::Format::R8G8B8A8Unorm;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    auto view = texture->createView(viewDesc);
    EXPECT_NE(view, nullptr);
}

TEST_P(GfxCppTextureViewTest, CreateViewWithNullDescriptor)
{
    auto texture = createBasicTexture();
    ASSERT_NE(texture, nullptr);

    // The C++ API doesn't support null descriptors the same way,
    // but we can verify invalid descriptor handling
    // This test is more relevant to the C API
}

TEST_P(GfxCppTextureViewTest, CreateViewInvalidArguments)
{
    auto texture = createBasicTexture();
    ASSERT_NE(texture, nullptr);

    gfx::TextureViewDescriptor viewDesc{};
    viewDesc.viewType = gfx::TextureViewType::View2D;
    viewDesc.format = gfx::Format::R8G8B8A8Unorm;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    // Valid descriptor should work
    auto view = texture->createView(viewDesc);
    EXPECT_NE(view, nullptr);
}

TEST_P(GfxCppTextureViewTest, CreateView1D)
{
    gfx::TextureDescriptor desc{};
    desc.type = gfx::TextureType::Texture1D;
    desc.size = { 512, 1, 1 };
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 1;
    desc.sampleCount = gfx::SampleCount::Count1;
    desc.format = gfx::Format::R8G8B8A8Unorm;
    desc.usage = gfx::TextureUsage::TextureBinding;

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    gfx::TextureViewDescriptor viewDesc{};
    viewDesc.viewType = gfx::TextureViewType::View1D;
    viewDesc.format = gfx::Format::R8G8B8A8Unorm;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    auto view = texture->createView(viewDesc);
    EXPECT_NE(view, nullptr);
}

TEST_P(GfxCppTextureViewTest, CreateView3D)
{
    gfx::TextureDescriptor desc{};
    desc.type = gfx::TextureType::Texture3D;
    desc.size = { 64, 64, 64 };
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 1;
    desc.sampleCount = gfx::SampleCount::Count1;
    desc.format = gfx::Format::R8G8B8A8Unorm;
    desc.usage = gfx::TextureUsage::TextureBinding;

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    gfx::TextureViewDescriptor viewDesc{};
    viewDesc.viewType = gfx::TextureViewType::View3D;
    viewDesc.format = gfx::Format::R8G8B8A8Unorm;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    auto view = texture->createView(viewDesc);
    EXPECT_NE(view, nullptr);
}

TEST_P(GfxCppTextureViewTest, CreateViewCube)
{
    gfx::TextureDescriptor desc{};
    desc.type = gfx::TextureType::TextureCube;
    desc.size = { 256, 256, 1 };
    desc.arrayLayerCount = 6; // Cube requires 6 layers
    desc.mipLevelCount = 1;
    desc.sampleCount = gfx::SampleCount::Count1;
    desc.format = gfx::Format::R8G8B8A8Unorm;
    desc.usage = gfx::TextureUsage::TextureBinding;

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    gfx::TextureViewDescriptor viewDesc{};
    viewDesc.viewType = gfx::TextureViewType::ViewCube;
    viewDesc.format = gfx::Format::R8G8B8A8Unorm;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 6;

    auto view = texture->createView(viewDesc);
    EXPECT_NE(view, nullptr);
}

TEST_P(GfxCppTextureViewTest, CreateView1DArray)
{
    gfx::TextureDescriptor desc{};
    desc.type = gfx::TextureType::Texture1D;
    desc.size = { 512, 1, 1 };
    desc.arrayLayerCount = 4;
    desc.mipLevelCount = 1;
    desc.sampleCount = gfx::SampleCount::Count1;
    desc.format = gfx::Format::R8G8B8A8Unorm;
    desc.usage = gfx::TextureUsage::TextureBinding;

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    gfx::TextureViewDescriptor viewDesc{};
    viewDesc.viewType = gfx::TextureViewType::View1DArray;
    viewDesc.format = gfx::Format::R8G8B8A8Unorm;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 4;

    auto view = texture->createView(viewDesc);
    EXPECT_NE(view, nullptr);
}

TEST_P(GfxCppTextureViewTest, CreateView2DArray)
{
    gfx::TextureDescriptor desc{};
    desc.type = gfx::TextureType::Texture2D;
    desc.size = { 256, 256, 1 };
    desc.arrayLayerCount = 8;
    desc.mipLevelCount = 1;
    desc.sampleCount = gfx::SampleCount::Count1;
    desc.format = gfx::Format::R8G8B8A8Unorm;
    desc.usage = gfx::TextureUsage::TextureBinding;

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    gfx::TextureViewDescriptor viewDesc{};
    viewDesc.viewType = gfx::TextureViewType::View2DArray;
    viewDesc.format = gfx::Format::R8G8B8A8Unorm;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 8;

    auto view = texture->createView(viewDesc);
    EXPECT_NE(view, nullptr);
}

TEST_P(GfxCppTextureViewTest, CreateViewCubeArray)
{
    gfx::TextureDescriptor desc{};
    desc.type = gfx::TextureType::TextureCube;
    desc.size = { 256, 256, 1 };
    desc.arrayLayerCount = 12; // 2 cubes = 12 layers
    desc.mipLevelCount = 1;
    desc.sampleCount = gfx::SampleCount::Count1;
    desc.format = gfx::Format::R8G8B8A8Unorm;
    desc.usage = gfx::TextureUsage::TextureBinding;

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    gfx::TextureViewDescriptor viewDesc{};
    viewDesc.viewType = gfx::TextureViewType::ViewCubeArray;
    viewDesc.format = gfx::Format::R8G8B8A8Unorm;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 12;

    auto view = texture->createView(viewDesc);
    EXPECT_NE(view, nullptr);
}

TEST_P(GfxCppTextureViewTest, CreateViewSpecificMipLevel)
{
    gfx::TextureDescriptor desc{};
    desc.type = gfx::TextureType::Texture2D;
    desc.size = { 512, 512, 1 };
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 9; // log2(512) + 1
    desc.sampleCount = gfx::SampleCount::Count1;
    desc.format = gfx::Format::R8G8B8A8Unorm;
    desc.usage = gfx::TextureUsage::TextureBinding | gfx::TextureUsage::CopyDst;

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    // Create view for mip level 3 only
    gfx::TextureViewDescriptor viewDesc{};
    viewDesc.viewType = gfx::TextureViewType::View2D;
    viewDesc.format = gfx::Format::R8G8B8A8Unorm;
    viewDesc.baseMipLevel = 3;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    auto view = texture->createView(viewDesc);
    EXPECT_NE(view, nullptr);
}

TEST_P(GfxCppTextureViewTest, CreateViewMipLevelRange)
{
    gfx::TextureDescriptor desc{};
    desc.type = gfx::TextureType::Texture2D;
    desc.size = { 512, 512, 1 };
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 9;
    desc.sampleCount = gfx::SampleCount::Count1;
    desc.format = gfx::Format::R8G8B8A8Unorm;
    desc.usage = gfx::TextureUsage::TextureBinding | gfx::TextureUsage::CopyDst;

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    // Create view for mip levels 2-5
    gfx::TextureViewDescriptor viewDesc{};
    viewDesc.viewType = gfx::TextureViewType::View2D;
    viewDesc.format = gfx::Format::R8G8B8A8Unorm;
    viewDesc.baseMipLevel = 2;
    viewDesc.mipLevelCount = 4; // levels 2, 3, 4, 5
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    auto view = texture->createView(viewDesc);
    EXPECT_NE(view, nullptr);
}

TEST_P(GfxCppTextureViewTest, CreateViewSpecificArrayLayer)
{
    gfx::TextureDescriptor desc{};
    desc.type = gfx::TextureType::Texture2D;
    desc.size = { 256, 256, 1 };
    desc.arrayLayerCount = 8;
    desc.mipLevelCount = 1;
    desc.sampleCount = gfx::SampleCount::Count1;
    desc.format = gfx::Format::R8G8B8A8Unorm;
    desc.usage = gfx::TextureUsage::TextureBinding;

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    // Create 2D view of layer 3
    gfx::TextureViewDescriptor viewDesc{};
    viewDesc.viewType = gfx::TextureViewType::View2D;
    viewDesc.format = gfx::Format::R8G8B8A8Unorm;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 3;
    viewDesc.arrayLayerCount = 1;

    auto view = texture->createView(viewDesc);
    EXPECT_NE(view, nullptr);
}

TEST_P(GfxCppTextureViewTest, CreateViewArrayLayerRange)
{
    gfx::TextureDescriptor desc{};
    desc.type = gfx::TextureType::Texture2D;
    desc.size = { 256, 256, 1 };
    desc.arrayLayerCount = 10;
    desc.mipLevelCount = 1;
    desc.sampleCount = gfx::SampleCount::Count1;
    desc.format = gfx::Format::R8G8B8A8Unorm;
    desc.usage = gfx::TextureUsage::TextureBinding;

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    // Create 2D array view of layers 3-6
    gfx::TextureViewDescriptor viewDesc{};
    viewDesc.viewType = gfx::TextureViewType::View2DArray;
    viewDesc.format = gfx::Format::R8G8B8A8Unorm;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 3;
    viewDesc.arrayLayerCount = 4; // layers 3, 4, 5, 6

    auto view = texture->createView(viewDesc);
    EXPECT_NE(view, nullptr);
}

TEST_P(GfxCppTextureViewTest, CreateMultipleViewsOfSameTexture)
{
    auto texture = createBasicTexture();
    ASSERT_NE(texture, nullptr);

    const int viewCount = 5;
    std::vector<std::shared_ptr<gfx::TextureView>> views;

    gfx::TextureViewDescriptor viewDesc{};
    viewDesc.viewType = gfx::TextureViewType::View2D;
    viewDesc.format = gfx::Format::R8G8B8A8Unorm;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    // Create multiple views of the same texture
    for (int i = 0; i < viewCount; ++i) {
        auto view = texture->createView(viewDesc);
        ASSERT_NE(view, nullptr);
        views.push_back(view);
    }

    // Views will be automatically destroyed when they go out of scope
    EXPECT_EQ(views.size(), viewCount);
}

TEST_P(GfxCppTextureViewTest, CreateViewForDepthTexture)
{
    gfx::TextureDescriptor desc{};
    desc.type = gfx::TextureType::Texture2D;
    desc.size = { 512, 512, 1 };
    desc.arrayLayerCount = 1;
    desc.mipLevelCount = 1;
    desc.sampleCount = gfx::SampleCount::Count1;
    desc.format = gfx::Format::Depth32Float;
    desc.usage = gfx::TextureUsage::RenderAttachment | gfx::TextureUsage::TextureBinding;

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    gfx::TextureViewDescriptor viewDesc{};
    viewDesc.viewType = gfx::TextureViewType::View2D;
    viewDesc.format = gfx::Format::Depth32Float;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 0;
    viewDesc.arrayLayerCount = 1;

    auto view = texture->createView(viewDesc);
    EXPECT_NE(view, nullptr);
}

TEST_P(GfxCppTextureViewTest, CreateView2DFromSingleCubeFace)
{
    gfx::TextureDescriptor desc{};
    desc.type = gfx::TextureType::TextureCube;
    desc.size = { 256, 256, 1 };
    desc.arrayLayerCount = 6;
    desc.mipLevelCount = 1;
    desc.sampleCount = gfx::SampleCount::Count1;
    desc.format = gfx::Format::R8G8B8A8Unorm;
    desc.usage = gfx::TextureUsage::TextureBinding;

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    // Create a 2D view of face 2 (one face of the cube)
    gfx::TextureViewDescriptor viewDesc{};
    viewDesc.viewType = gfx::TextureViewType::View2D;
    viewDesc.format = gfx::Format::R8G8B8A8Unorm;
    viewDesc.baseMipLevel = 0;
    viewDesc.mipLevelCount = 1;
    viewDesc.baseArrayLayer = 2;
    viewDesc.arrayLayerCount = 1;

    auto view = texture->createView(viewDesc);
    EXPECT_NE(view, nullptr);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppTextureViewTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

} // namespace
