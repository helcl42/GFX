#include <backend/vulkan/core/resource/Texture.h>
#include <backend/vulkan/core/resource/TextureView.h>
#include <backend/vulkan/core/system/Adapter.h>
#include <backend/vulkan/core/system/Device.h>
#include <backend/vulkan/core/system/Instance.h>

#include <gtest/gtest.h>

// Test Vulkan core TextureView class
// These tests verify the internal texture view implementation, not the public API

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class VulkanTextureViewTest : public testing::Test {
protected:
    void SetUp() override
    {
        try {
            gfx::backend::vulkan::core::InstanceCreateInfo instInfo{};
            instInfo.enabledExtensions = {};
            instance = std::make_unique<gfx::backend::vulkan::core::Instance>(instInfo);

            gfx::backend::vulkan::core::AdapterCreateInfo adapterInfo{};
            adapterInfo.adapterIndex = 0;
            adapter = instance->requestAdapter(adapterInfo);

            gfx::backend::vulkan::core::DeviceCreateInfo deviceInfo{};
            device = std::make_unique<gfx::backend::vulkan::core::Device>(adapter, deviceInfo);
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up Vulkan: " << e.what();
        }
    }

    std::unique_ptr<gfx::backend::vulkan::core::Instance> instance;
    gfx::backend::vulkan::core::Adapter* adapter = nullptr;
    std::unique_ptr<gfx::backend::vulkan::core::Device> device;
};

// ============================================================================
// Basic 2D Texture View Tests
// ============================================================================

TEST_F(VulkanTextureViewTest, CreateBasic2DView_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.size = { 512, 512, 1 };
    textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 1;
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.arrayLayers = 1;
    textureInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_UNDEFINED; // Use texture's format
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;

    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    EXPECT_NE(textureView.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(textureView.getTexture(), &texture);
    EXPECT_EQ(textureView.getFormat(), VK_FORMAT_R8G8B8A8_UNORM);
}

TEST_F(VulkanTextureViewTest, CreateViewExplicitFormat_UsesSpecifiedFormat)
{
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.size = { 512, 512, 1 };
    textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 1;
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.arrayLayers = 1;
    textureInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB; // Different format from texture
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;

    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    EXPECT_NE(textureView.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(textureView.getFormat(), VK_FORMAT_R8G8B8A8_SRGB);
}

// ============================================================================
// 1D Texture View Tests
// ============================================================================

TEST_F(VulkanTextureViewTest, Create1DView_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.size = { 256, 1, 1 };
    textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 1;
    textureInfo.imageType = VK_IMAGE_TYPE_1D;
    textureInfo.arrayLayers = 1;
    textureInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_1D;
    viewInfo.format = VK_FORMAT_UNDEFINED;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;

    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    EXPECT_NE(textureView.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanTextureViewTest, Create1DArrayView_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.size = { 256, 1, 1 };
    textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 1;
    textureInfo.imageType = VK_IMAGE_TYPE_1D;
    textureInfo.arrayLayers = 4;
    textureInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
    viewInfo.format = VK_FORMAT_UNDEFINED;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 4;

    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    EXPECT_NE(textureView.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// 2D Array Texture View Tests
// ============================================================================

TEST_F(VulkanTextureViewTest, Create2DArrayView_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.size = { 256, 256, 1 };
    textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 1;
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.arrayLayers = 6;
    textureInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    viewInfo.format = VK_FORMAT_UNDEFINED;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 6;

    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    EXPECT_NE(textureView.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanTextureViewTest, CreatePartialArrayView_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.size = { 256, 256, 1 };
    textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 1;
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.arrayLayers = 10;
    textureInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    // View only layers 2-5
    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    viewInfo.format = VK_FORMAT_UNDEFINED;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 2;
    viewInfo.arrayLayerCount = 4;

    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    EXPECT_NE(textureView.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// 3D Texture View Tests
// ============================================================================

TEST_F(VulkanTextureViewTest, Create3DView_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.size = { 64, 64, 64 };
    textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 1;
    textureInfo.imageType = VK_IMAGE_TYPE_3D;
    textureInfo.arrayLayers = 1;
    textureInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
    viewInfo.format = VK_FORMAT_UNDEFINED;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;

    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    EXPECT_NE(textureView.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Cube Map Texture View Tests
// ============================================================================

TEST_F(VulkanTextureViewTest, CreateCubeView_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.size = { 512, 512, 1 };
    textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 1;
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.arrayLayers = 6;
    textureInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = VK_FORMAT_UNDEFINED;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 6;

    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    EXPECT_NE(textureView.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanTextureViewTest, CreateCubeArrayView_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.size = { 256, 256, 1 };
    textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 1;
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.arrayLayers = 12; // 2 cube maps
    textureInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    viewInfo.format = VK_FORMAT_UNDEFINED;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 12;

    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    EXPECT_NE(textureView.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Mipmap Level Tests
// ============================================================================

TEST_F(VulkanTextureViewTest, CreateViewAllMipLevels_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.size = { 1024, 1024, 1 };
    textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 11; // log2(1024) + 1
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.arrayLayers = 1;
    textureInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_UNDEFINED;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 11;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;

    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    EXPECT_NE(textureView.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanTextureViewTest, CreateViewSingleMipLevel_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.size = { 1024, 1024, 1 };
    textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 11;
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.arrayLayers = 1;
    textureInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    // View only mip level 3
    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_UNDEFINED;
    viewInfo.baseMipLevel = 3;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;

    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    EXPECT_NE(textureView.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanTextureViewTest, CreateViewPartialMipRange_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.size = { 1024, 1024, 1 };
    textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 11;
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.arrayLayers = 1;
    textureInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    // View mip levels 2-5
    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_UNDEFINED;
    viewInfo.baseMipLevel = 2;
    viewInfo.mipLevelCount = 4;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;

    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    EXPECT_NE(textureView.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Different Format Tests
// ============================================================================

TEST_F(VulkanTextureViewTest, CreateViewDepthFormat_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_D32_SFLOAT;
    textureInfo.size = { 1024, 768, 1 };
    textureInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 1;
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.arrayLayers = 1;
    textureInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_UNDEFINED;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;

    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    EXPECT_NE(textureView.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(textureView.getFormat(), VK_FORMAT_D32_SFLOAT);
}

TEST_F(VulkanTextureViewTest, CreateViewFloatFormat_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    textureInfo.size = { 256, 256, 1 };
    textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 1;
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.arrayLayers = 1;
    textureInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_UNDEFINED;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;

    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    EXPECT_NE(textureView.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(textureView.getFormat(), VK_FORMAT_R32G32B32A32_SFLOAT);
}

// ============================================================================
// Multiple Views from Same Texture Tests
// ============================================================================

TEST_F(VulkanTextureViewTest, CreateMultipleViewsSameTexture_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.size = { 512, 512, 1 };
    textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 5;
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.arrayLayers = 1;
    textureInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    // Create view for all mips
    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo1{};
    viewInfo1.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo1.format = VK_FORMAT_UNDEFINED;
    viewInfo1.baseMipLevel = 0;
    viewInfo1.mipLevelCount = 5;
    viewInfo1.baseArrayLayer = 0;
    viewInfo1.arrayLayerCount = 1;

    // Create view for first mip only
    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo2{};
    viewInfo2.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo2.format = VK_FORMAT_UNDEFINED;
    viewInfo2.baseMipLevel = 0;
    viewInfo2.mipLevelCount = 1;
    viewInfo2.baseArrayLayer = 0;
    viewInfo2.arrayLayerCount = 1;

    gfx::backend::vulkan::core::TextureView textureView1(&texture, viewInfo1);
    gfx::backend::vulkan::core::TextureView textureView2(&texture, viewInfo2);

    EXPECT_NE(textureView1.handle(), VK_NULL_HANDLE);
    EXPECT_NE(textureView2.handle(), VK_NULL_HANDLE);
    EXPECT_NE(textureView1.handle(), textureView2.handle());
    EXPECT_EQ(textureView1.getTexture(), &texture);
    EXPECT_EQ(textureView2.getTexture(), &texture);
}

TEST_F(VulkanTextureViewTest, CreateViewsWithDifferentFormats_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.size = { 256, 256, 1 };
    textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 1;
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.arrayLayers = 1;
    textureInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    // UNORM view
    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo1{};
    viewInfo1.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo1.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo1.baseMipLevel = 0;
    viewInfo1.mipLevelCount = 1;
    viewInfo1.baseArrayLayer = 0;
    viewInfo1.arrayLayerCount = 1;

    // SRGB view (reinterpretation)
    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo2{};
    viewInfo2.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo2.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo2.baseMipLevel = 0;
    viewInfo2.mipLevelCount = 1;
    viewInfo2.baseArrayLayer = 0;
    viewInfo2.arrayLayerCount = 1;

    gfx::backend::vulkan::core::TextureView textureView1(&texture, viewInfo1);
    gfx::backend::vulkan::core::TextureView textureView2(&texture, viewInfo2);

    EXPECT_NE(textureView1.handle(), VK_NULL_HANDLE);
    EXPECT_NE(textureView2.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(textureView1.getFormat(), VK_FORMAT_R8G8B8A8_UNORM);
    EXPECT_EQ(textureView2.getFormat(), VK_FORMAT_R8G8B8A8_SRGB);
}

// ============================================================================
// Getter Tests
// ============================================================================

TEST_F(VulkanTextureViewTest, GetTexture_ReturnsCorrectTexture)
{
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.size = { 256, 256, 1 };
    textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 1;
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.arrayLayers = 1;
    textureInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_UNDEFINED;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;

    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    EXPECT_EQ(textureView.getTexture(), &texture);
}

TEST_F(VulkanTextureViewTest, GetFormat_ReturnsCorrectFormat)
{
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    textureInfo.size = { 512, 512, 1 };
    textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 1;
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.arrayLayers = 1;
    textureInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_UNDEFINED;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;

    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    EXPECT_EQ(textureView.getFormat(), VK_FORMAT_R16G16B16A16_SFLOAT);
}

} // namespace
