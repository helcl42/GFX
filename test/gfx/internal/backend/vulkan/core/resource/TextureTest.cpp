#include <backend/vulkan/core/resource/Texture.h>
#include <backend/vulkan/core/system/Adapter.h>
#include <backend/vulkan/core/system/Device.h>
#include <backend/vulkan/core/system/Instance.h>


#include <gtest/gtest.h>

// Test Vulkan core Texture class
// These tests verify the internal texture implementation, not the public API

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class VulkanTextureTest : public testing::Test {
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
// Texture Creation Tests
// ============================================================================

TEST_F(VulkanTextureTest, CreateTexture_2D_RGBA8_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo createInfo{};
    createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    createInfo.size = { 512, 512, 1 };
    createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    createInfo.mipLevelCount = 1;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.arrayLayers = 1;
    createInfo.flags = 0;

    gfx::backend::vulkan::core::Texture texture(device.get(), createInfo);

    EXPECT_NE(texture.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(texture.getFormat(), VK_FORMAT_R8G8B8A8_UNORM);
    EXPECT_EQ(texture.getSize().width, 512u);
    EXPECT_EQ(texture.getSize().height, 512u);
    EXPECT_EQ(texture.getImageType(), VK_IMAGE_TYPE_2D);
}

TEST_F(VulkanTextureTest, CreateTexture_1D_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo createInfo{};
    createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    createInfo.size = { 256, 1, 1 };
    createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    createInfo.mipLevelCount = 1;
    createInfo.imageType = VK_IMAGE_TYPE_1D;
    createInfo.arrayLayers = 1;
    createInfo.flags = 0;

    gfx::backend::vulkan::core::Texture texture(device.get(), createInfo);

    EXPECT_NE(texture.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(texture.getImageType(), VK_IMAGE_TYPE_1D);
    EXPECT_EQ(texture.getSize().width, 256u);
}

TEST_F(VulkanTextureTest, CreateTexture_3D_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo createInfo{};
    createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    createInfo.size = { 64, 64, 64 };
    createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    createInfo.mipLevelCount = 1;
    createInfo.imageType = VK_IMAGE_TYPE_3D;
    createInfo.arrayLayers = 1;
    createInfo.flags = 0;

    gfx::backend::vulkan::core::Texture texture(device.get(), createInfo);

    EXPECT_NE(texture.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(texture.getImageType(), VK_IMAGE_TYPE_3D);
    EXPECT_EQ(texture.getSize().depth, 64u);
}

TEST_F(VulkanTextureTest, CreateTexture_WithMipmaps_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo createInfo{};
    createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    createInfo.size = { 1024, 1024, 1 };
    createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    createInfo.mipLevelCount = 11; // log2(1024) + 1
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.arrayLayers = 1;
    createInfo.flags = 0;

    gfx::backend::vulkan::core::Texture texture(device.get(), createInfo);

    EXPECT_NE(texture.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(texture.getMipLevelCount(), 11u);
}

TEST_F(VulkanTextureTest, CreateTexture_ArrayLayers_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo createInfo{};
    createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    createInfo.size = { 256, 256, 1 };
    createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    createInfo.mipLevelCount = 1;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.arrayLayers = 6;
    createInfo.flags = 0;

    gfx::backend::vulkan::core::Texture texture(device.get(), createInfo);

    EXPECT_NE(texture.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(texture.getArrayLayers(), 6u);
}

TEST_F(VulkanTextureTest, CreateTexture_CubeMap_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo createInfo{};
    createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    createInfo.size = { 512, 512, 1 };
    createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    createInfo.mipLevelCount = 1;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.arrayLayers = 6;
    createInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    gfx::backend::vulkan::core::Texture texture(device.get(), createInfo);

    EXPECT_NE(texture.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(texture.getArrayLayers(), 6u);
}

// ============================================================================
// Texture Format Tests
// ============================================================================

TEST_F(VulkanTextureTest, CreateTexture_FloatFormat_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo createInfo{};
    createInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    createInfo.size = { 256, 256, 1 };
    createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    createInfo.mipLevelCount = 1;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.arrayLayers = 1;
    createInfo.flags = 0;

    gfx::backend::vulkan::core::Texture texture(device.get(), createInfo);

    EXPECT_NE(texture.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(texture.getFormat(), VK_FORMAT_R32G32B32A32_SFLOAT);
}

TEST_F(VulkanTextureTest, CreateTexture_DepthFormat_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo createInfo{};
    createInfo.format = VK_FORMAT_D32_SFLOAT;
    createInfo.size = { 1024, 768, 1 };
    createInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    createInfo.mipLevelCount = 1;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.arrayLayers = 1;
    createInfo.flags = 0;

    gfx::backend::vulkan::core::Texture texture(device.get(), createInfo);

    EXPECT_NE(texture.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(texture.getFormat(), VK_FORMAT_D32_SFLOAT);
}

TEST_F(VulkanTextureTest, CreateTexture_DepthStencilFormat_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo createInfo{};
    createInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
    createInfo.size = { 800, 600, 1 };
    createInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    createInfo.mipLevelCount = 1;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.arrayLayers = 1;
    createInfo.flags = 0;

    gfx::backend::vulkan::core::Texture texture(device.get(), createInfo);

    EXPECT_NE(texture.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(texture.getFormat(), VK_FORMAT_D24_UNORM_S8_UINT);
}

TEST_F(VulkanTextureTest, CreateTexture_SRGBFormat_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo createInfo{};
    createInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    createInfo.size = { 512, 512, 1 };
    createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    createInfo.mipLevelCount = 1;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.arrayLayers = 1;
    createInfo.flags = 0;

    gfx::backend::vulkan::core::Texture texture(device.get(), createInfo);

    EXPECT_NE(texture.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(texture.getFormat(), VK_FORMAT_R8G8B8A8_SRGB);
}

// ============================================================================
// Texture Usage Tests
// ============================================================================

TEST_F(VulkanTextureTest, CreateTexture_ColorAttachment_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo createInfo{};
    createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    createInfo.size = { 1920, 1080, 1 };
    createInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    createInfo.mipLevelCount = 1;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.arrayLayers = 1;
    createInfo.flags = 0;

    gfx::backend::vulkan::core::Texture texture(device.get(), createInfo);

    EXPECT_TRUE(texture.getUsage() & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    EXPECT_TRUE(texture.getUsage() & VK_IMAGE_USAGE_SAMPLED_BIT);
}

TEST_F(VulkanTextureTest, CreateTexture_StorageUsage_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo createInfo{};
    createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    createInfo.size = { 512, 512, 1 };
    createInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    createInfo.mipLevelCount = 1;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.arrayLayers = 1;
    createInfo.flags = 0;

    gfx::backend::vulkan::core::Texture texture(device.get(), createInfo);

    EXPECT_TRUE(texture.getUsage() & VK_IMAGE_USAGE_STORAGE_BIT);
}

// ============================================================================
// Texture Sample Count Tests
// ============================================================================

TEST_F(VulkanTextureTest, CreateTexture_MSAA4x_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo createInfo{};
    createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    createInfo.size = { 1920, 1080, 1 };
    createInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.sampleCount = VK_SAMPLE_COUNT_4_BIT;
    createInfo.mipLevelCount = 1;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.arrayLayers = 1;
    createInfo.flags = 0;

    gfx::backend::vulkan::core::Texture texture(device.get(), createInfo);

    EXPECT_NE(texture.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(texture.getSampleCount(), VK_SAMPLE_COUNT_4_BIT);
}

// ============================================================================
// Texture Info Tests
// ============================================================================

TEST_F(VulkanTextureTest, GetInfo_AfterCreation_ReturnsCorrectInfo)
{
    gfx::backend::vulkan::core::TextureCreateInfo createInfo{};
    createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    createInfo.size = { 640, 480, 1 };
    createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    createInfo.mipLevelCount = 4;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.arrayLayers = 2;
    createInfo.flags = 0;

    gfx::backend::vulkan::core::Texture texture(device.get(), createInfo);

    const auto& info = texture.getInfo();
    EXPECT_EQ(info.imageType, VK_IMAGE_TYPE_2D);
    EXPECT_EQ(info.size.width, 640u);
    EXPECT_EQ(info.size.height, 480u);
    EXPECT_EQ(info.format, VK_FORMAT_R8G8B8A8_UNORM);
    EXPECT_EQ(info.mipLevelCount, 4u);
    EXPECT_EQ(info.arrayLayers, 2u);
    EXPECT_EQ(info.sampleCount, VK_SAMPLE_COUNT_1_BIT);
}

// ============================================================================
// Texture Layout Tests
// ============================================================================

TEST_F(VulkanTextureTest, GetLayout_InitialLayout_ReturnsUndefined)
{
    gfx::backend::vulkan::core::TextureCreateInfo createInfo{};
    createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    createInfo.size = { 256, 256, 1 };
    createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    createInfo.mipLevelCount = 1;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.arrayLayers = 1;
    createInfo.flags = 0;

    gfx::backend::vulkan::core::Texture texture(device.get(), createInfo);

    EXPECT_EQ(texture.getLayout(), VK_IMAGE_LAYOUT_UNDEFINED);
}

TEST_F(VulkanTextureTest, SetLayout_UpdatesLayout_Correctly)
{
    gfx::backend::vulkan::core::TextureCreateInfo createInfo{};
    createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    createInfo.size = { 256, 256, 1 };
    createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    createInfo.mipLevelCount = 1;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.arrayLayers = 1;
    createInfo.flags = 0;

    gfx::backend::vulkan::core::Texture texture(device.get(), createInfo);

    texture.setLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    EXPECT_EQ(texture.getLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    texture.setLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    EXPECT_EQ(texture.getLayout(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
}

// ============================================================================
// Texture Import Tests
// ============================================================================

TEST_F(VulkanTextureTest, ImportTexture_ValidHandle_CreatesSuccessfully)
{
    // First create a regular texture
    gfx::backend::vulkan::core::TextureCreateInfo createInfo{};
    createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    createInfo.size = { 512, 512, 1 };
    createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    createInfo.mipLevelCount = 1;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.arrayLayers = 1;
    createInfo.flags = 0;

    gfx::backend::vulkan::core::Texture sourceTexture(device.get(), createInfo);
    VkImage handle = sourceTexture.handle();
    ASSERT_NE(handle, VK_NULL_HANDLE);

    // Import the handle
    gfx::backend::vulkan::core::TextureImportInfo importInfo{};
    importInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    importInfo.size = { 512, 512, 1 };
    importInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    importInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    importInfo.mipLevelCount = 1;
    importInfo.imageType = VK_IMAGE_TYPE_2D;
    importInfo.arrayLayers = 1;
    importInfo.flags = 0;

    gfx::backend::vulkan::core::Texture importedTexture(device.get(), handle, importInfo);

    EXPECT_EQ(importedTexture.handle(), handle);
    EXPECT_EQ(importedTexture.getSize().width, 512u);
}

// ============================================================================
// Large Texture Tests
// ============================================================================

TEST_F(VulkanTextureTest, CreateTexture_4K_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo createInfo{};
    createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    createInfo.size = { 3840, 2160, 1 };
    createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    createInfo.mipLevelCount = 1;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.arrayLayers = 1;
    createInfo.flags = 0;

    gfx::backend::vulkan::core::Texture texture(device.get(), createInfo);

    EXPECT_NE(texture.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(texture.getSize().width, 3840u);
    EXPECT_EQ(texture.getSize().height, 2160u);
}

TEST_F(VulkanTextureTest, CreateTexture_8K_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::TextureCreateInfo createInfo{};
    createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    createInfo.size = { 7680, 4320, 1 };
    createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    createInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    createInfo.mipLevelCount = 1;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.arrayLayers = 1;
    createInfo.flags = 0;

    gfx::backend::vulkan::core::Texture texture(device.get(), createInfo);

    EXPECT_NE(texture.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(texture.getSize().width, 7680u);
    EXPECT_EQ(texture.getSize().height, 4320u);
}

} // namespace
