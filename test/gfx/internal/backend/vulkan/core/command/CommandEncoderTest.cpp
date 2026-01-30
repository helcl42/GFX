#include <backend/vulkan/core/command/CommandEncoder.h>
#include <backend/vulkan/core/query/QuerySet.h>
#include <backend/vulkan/core/resource/Buffer.h>
#include <backend/vulkan/core/resource/Texture.h>
#include <backend/vulkan/core/system/Device.h>
#include <backend/vulkan/core/system/Instance.h>

#include <gtest/gtest.h>

#include <memory>

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class VulkanCommandEncoderTest : public testing::Test {
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
// Basic Creation Tests
// ============================================================================

TEST_F(VulkanCommandEncoderTest, CreateCommandEncoder_CreatesSuccessfully)
{
    auto encoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    EXPECT_NE(encoder->handle(), VK_NULL_HANDLE);
    EXPECT_EQ(encoder->device(), device->handle());
    EXPECT_EQ(encoder->getDevice(), device.get());
}

TEST_F(VulkanCommandEncoderTest, Handle_ReturnsValidVkCommandBuffer)
{
    auto encoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    VkCommandBuffer handle = encoder->handle();
    EXPECT_NE(handle, VK_NULL_HANDLE);
}

TEST_F(VulkanCommandEncoderTest, Handle_IsUnique)
{
    auto encoder1 = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());
    auto encoder2 = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    EXPECT_NE(encoder1->handle(), encoder2->handle());
}

// ============================================================================
// Device Tests
// ============================================================================

TEST_F(VulkanCommandEncoderTest, Device_ReturnsCorrectDevice)
{
    auto encoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    EXPECT_EQ(encoder->device(), device->handle());
}

TEST_F(VulkanCommandEncoderTest, GetDevice_ReturnsCorrectDevicePointer)
{
    auto encoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    EXPECT_EQ(encoder->getDevice(), device.get());
}

// ============================================================================
// Recording Tests
// ============================================================================

TEST_F(VulkanCommandEncoderTest, BeginEnd_WorksCorrectly)
{
    auto encoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    EXPECT_NO_THROW(encoder->begin());
    EXPECT_NO_THROW(encoder->end());
}

TEST_F(VulkanCommandEncoderTest, Reset_WorksAfterRecording)
{
    auto encoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    encoder->begin();
    encoder->end();
    EXPECT_NO_THROW(encoder->reset());
}

TEST_F(VulkanCommandEncoderTest, MultipleRecordCycles_WorkCorrectly)
{
    auto encoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    for (int i = 0; i < 3; ++i) {
        EXPECT_NO_THROW(encoder->begin());
        EXPECT_NO_THROW(encoder->end());
        EXPECT_NO_THROW(encoder->reset());
    }
}

// ============================================================================
// Pipeline Layout Tests
// ============================================================================

TEST_F(VulkanCommandEncoderTest, CurrentPipelineLayout_InitiallyNull)
{
    auto encoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    EXPECT_EQ(encoder->currentPipelineLayout(), VK_NULL_HANDLE);
}

TEST_F(VulkanCommandEncoderTest, SetCurrentPipelineLayout_UpdatesLayout)
{
    auto encoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    VkPipelineLayout layout;
    VkResult result = vkCreatePipelineLayout(device->handle(), &layoutInfo, nullptr, &layout);
    ASSERT_EQ(result, VK_SUCCESS);

    encoder->setCurrentPipelineLayout(layout);
    EXPECT_EQ(encoder->currentPipelineLayout(), layout);

    vkDestroyPipelineLayout(device->handle(), layout, nullptr);
}

// ============================================================================
// Buffer Copy Tests
// ============================================================================

TEST_F(VulkanCommandEncoderTest, CopyBufferToBuffer_WorksCorrectly)
{
    gfx::backend::vulkan::core::BufferCreateInfo srcInfo{};
    srcInfo.size = 1024;
    srcInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    auto srcBuffer = std::make_unique<gfx::backend::vulkan::core::Buffer>(device.get(), srcInfo);

    gfx::backend::vulkan::core::BufferCreateInfo dstInfo{};
    dstInfo.size = 1024;
    dstInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    auto dstBuffer = std::make_unique<gfx::backend::vulkan::core::Buffer>(device.get(), dstInfo);

    auto encoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    encoder->begin();
    EXPECT_NO_THROW(encoder->copyBufferToBuffer(srcBuffer.get(), 0, dstBuffer.get(), 0, 512));
    encoder->end();
}

// ============================================================================
// Texture Copy Tests
// ============================================================================

TEST_F(VulkanCommandEncoderTest, CopyBufferToTexture_WorksCorrectly)
{
    gfx::backend::vulkan::core::BufferCreateInfo bufferInfo{};
    bufferInfo.size = 1024 * 1024;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    auto buffer = std::make_unique<gfx::backend::vulkan::core::Buffer>(device.get(), bufferInfo);

    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.size = { 256, 256, 1 };
    textureInfo.mipLevelCount = 1;
    textureInfo.arrayLayers = 1;
    textureInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.flags = 0;
    auto texture = std::make_unique<gfx::backend::vulkan::core::Texture>(device.get(), textureInfo);

    auto encoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    encoder->begin();
    VkOffset3D origin = { 0, 0, 0 };
    VkExtent3D extent = { 256, 256, 1 };
    EXPECT_NO_THROW(encoder->copyBufferToTexture(buffer.get(), 0, texture.get(), origin, extent, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
    encoder->end();
}

TEST_F(VulkanCommandEncoderTest, CopyTextureToBuffer_WorksCorrectly)
{
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.size = { 256, 256, 1 };
    textureInfo.mipLevelCount = 1;
    textureInfo.arrayLayers = 1;
    textureInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.flags = 0;
    auto texture = std::make_unique<gfx::backend::vulkan::core::Texture>(device.get(), textureInfo);

    gfx::backend::vulkan::core::BufferCreateInfo bufferInfo{};
    bufferInfo.size = 1024 * 1024;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    auto buffer = std::make_unique<gfx::backend::vulkan::core::Buffer>(device.get(), bufferInfo);

    auto encoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    encoder->begin();
    VkOffset3D origin = { 0, 0, 0 };
    VkExtent3D extent = { 256, 256, 1 };
    EXPECT_NO_THROW(encoder->copyTextureToBuffer(texture.get(), origin, 0, buffer.get(), 0, extent, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL));
    encoder->end();
}

TEST_F(VulkanCommandEncoderTest, CopyTextureToTexture_WorksCorrectly)
{
    gfx::backend::vulkan::core::TextureCreateInfo srcInfo{};
    srcInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    srcInfo.size = { 256, 256, 1 };
    srcInfo.mipLevelCount = 1;
    srcInfo.arrayLayers = 1;
    srcInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    srcInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    srcInfo.imageType = VK_IMAGE_TYPE_2D;
    srcInfo.flags = 0;
    auto srcTexture = std::make_unique<gfx::backend::vulkan::core::Texture>(device.get(), srcInfo);

    gfx::backend::vulkan::core::TextureCreateInfo dstInfo{};
    dstInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    dstInfo.size = { 256, 256, 1 };
    dstInfo.mipLevelCount = 1;
    dstInfo.arrayLayers = 1;
    dstInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    dstInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    dstInfo.imageType = VK_IMAGE_TYPE_2D;
    dstInfo.flags = 0;
    auto dstTexture = std::make_unique<gfx::backend::vulkan::core::Texture>(device.get(), dstInfo);

    auto encoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    encoder->begin();
    VkOffset3D origin = { 0, 0, 0 };
    VkExtent3D extent = { 256, 256, 1 };
    EXPECT_NO_THROW(encoder->copyTextureToTexture(srcTexture.get(), origin, 0, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstTexture.get(), origin, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, extent));
    encoder->end();
}

TEST_F(VulkanCommandEncoderTest, BlitTextureToTexture_WorksCorrectly)
{
    gfx::backend::vulkan::core::TextureCreateInfo srcInfo{};
    srcInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    srcInfo.size = { 256, 256, 1 };
    srcInfo.mipLevelCount = 1;
    srcInfo.arrayLayers = 1;
    srcInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    srcInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    srcInfo.imageType = VK_IMAGE_TYPE_2D;
    srcInfo.flags = 0;
    auto srcTexture = std::make_unique<gfx::backend::vulkan::core::Texture>(device.get(), srcInfo);

    gfx::backend::vulkan::core::TextureCreateInfo dstInfo{};
    dstInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    dstInfo.size = { 128, 128, 1 };
    dstInfo.mipLevelCount = 1;
    dstInfo.arrayLayers = 1;
    dstInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    dstInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    dstInfo.imageType = VK_IMAGE_TYPE_2D;
    dstInfo.flags = 0;
    auto dstTexture = std::make_unique<gfx::backend::vulkan::core::Texture>(device.get(), dstInfo);

    auto encoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    encoder->begin();
    VkOffset3D origin = { 0, 0, 0 };
    VkExtent3D srcExtent = { 256, 256, 1 };
    VkExtent3D dstExtent = { 128, 128, 1 };
    EXPECT_NO_THROW(encoder->blitTextureToTexture(srcTexture.get(), origin, srcExtent, 0, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstTexture.get(), origin, dstExtent, 0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_FILTER_LINEAR));
    encoder->end();
}

// ============================================================================
// Pipeline Barrier Tests
// ============================================================================

TEST_F(VulkanCommandEncoderTest, PipelineBarrier_WithNoBarriers)
{
    auto encoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    encoder->begin();
    EXPECT_NO_THROW(encoder->pipelineBarrier(nullptr, 0, nullptr, 0, nullptr, 0));
    encoder->end();
}

// ============================================================================
// Query Tests
// ============================================================================

TEST_F(VulkanCommandEncoderTest, WriteTimestamp_WorksCorrectly)
{
    gfx::backend::vulkan::core::QuerySetCreateInfo queryInfo{};
    queryInfo.type = VK_QUERY_TYPE_TIMESTAMP;
    queryInfo.count = 2;
    auto querySet = std::make_unique<gfx::backend::vulkan::core::QuerySet>(device.get(), queryInfo);

    auto encoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    encoder->begin();
    EXPECT_NO_THROW(encoder->writeTimestamp(querySet->handle(), 0));
    encoder->end();
}

TEST_F(VulkanCommandEncoderTest, ResolveQuerySet_WorksCorrectly)
{
    gfx::backend::vulkan::core::QuerySetCreateInfo queryInfo{};
    queryInfo.type = VK_QUERY_TYPE_TIMESTAMP;
    queryInfo.count = 2;
    auto querySet = std::make_unique<gfx::backend::vulkan::core::QuerySet>(device.get(), queryInfo);

    gfx::backend::vulkan::core::BufferCreateInfo bufferInfo{};
    bufferInfo.size = 256;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    auto buffer = std::make_unique<gfx::backend::vulkan::core::Buffer>(device.get(), bufferInfo);

    auto encoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    encoder->begin();
    EXPECT_NO_THROW(encoder->resolveQuerySet(querySet->handle(), 0, 2, buffer->handle(), 0));
    encoder->end();
}

// ============================================================================
// Lifecycle Tests
// ============================================================================

TEST_F(VulkanCommandEncoderTest, Destructor_CleansUpResources)
{
    {
        auto encoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());
        EXPECT_NE(encoder->handle(), VK_NULL_HANDLE);
    }

    // If we reach here without crashing, cleanup succeeded
    SUCCEED();
}

TEST_F(VulkanCommandEncoderTest, MultipleCommandEncoders_CanCoexist)
{
    auto encoder1 = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());
    auto encoder2 = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());
    auto encoder3 = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    EXPECT_NE(encoder1->handle(), VK_NULL_HANDLE);
    EXPECT_NE(encoder2->handle(), VK_NULL_HANDLE);
    EXPECT_NE(encoder3->handle(), VK_NULL_HANDLE);

    EXPECT_NE(encoder1->handle(), encoder2->handle());
    EXPECT_NE(encoder2->handle(), encoder3->handle());
    EXPECT_NE(encoder1->handle(), encoder3->handle());
}

} // anonymous namespace
