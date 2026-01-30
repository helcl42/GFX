#include <backend/vulkan/core/command/CommandEncoder.h>
#include <backend/vulkan/core/command/RenderPassEncoder.h>
#include <backend/vulkan/core/query/QuerySet.h>
#include <backend/vulkan/core/render/Framebuffer.h>
#include <backend/vulkan/core/render/RenderPass.h>
#include <backend/vulkan/core/resource/Buffer.h>
#include <backend/vulkan/core/resource/Texture.h>
#include <backend/vulkan/core/resource/TextureView.h>
#include <backend/vulkan/core/system/Device.h>
#include <backend/vulkan/core/system/Instance.h>

#include <gtest/gtest.h>

#include <memory>

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class VulkanRenderPassEncoderTest : public testing::Test {
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

            // Create a simple render pass for reuse
            gfx::backend::vulkan::core::RenderPassCreateInfo rpInfo{};
            gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
            colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
            colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
            colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            rpInfo.colorAttachments.push_back(colorAtt);
            renderPass = std::make_unique<gfx::backend::vulkan::core::RenderPass>(device.get(), rpInfo);

            // Create texture
            gfx::backend::vulkan::core::TextureCreateInfo texInfo{};
            texInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
            texInfo.size = { 800, 600, 1 };
            texInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            texInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
            texInfo.mipLevelCount = 1;
            texInfo.imageType = VK_IMAGE_TYPE_2D;
            texInfo.arrayLayers = 1;
            texInfo.flags = 0;
            texture = std::make_unique<gfx::backend::vulkan::core::Texture>(device.get(), texInfo);

            // Create texture view
            gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
            viewInfo.baseMipLevel = 0;
            viewInfo.mipLevelCount = 1;
            viewInfo.baseArrayLayer = 0;
            viewInfo.arrayLayerCount = 1;
            textureView = std::make_unique<gfx::backend::vulkan::core::TextureView>(texture.get(), viewInfo);

            // Create framebuffer
            gfx::backend::vulkan::core::FramebufferCreateInfo fbInfo{};
            fbInfo.renderPass = renderPass->handle();
            fbInfo.attachments.push_back(textureView->handle());
            fbInfo.width = 800;
            fbInfo.height = 600;
            fbInfo.colorAttachmentCount = 1;
            fbInfo.hasDepthResolve = false;
            framebuffer = std::make_unique<gfx::backend::vulkan::core::Framebuffer>(device.get(), fbInfo);

        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up Vulkan: " << e.what();
        }
    }

    std::unique_ptr<gfx::backend::vulkan::core::Instance> instance;
    gfx::backend::vulkan::core::Adapter* adapter = nullptr;
    std::unique_ptr<gfx::backend::vulkan::core::Device> device;
    std::unique_ptr<gfx::backend::vulkan::core::RenderPass> renderPass;
    std::unique_ptr<gfx::backend::vulkan::core::Texture> texture;
    std::unique_ptr<gfx::backend::vulkan::core::TextureView> textureView;
    std::unique_ptr<gfx::backend::vulkan::core::Framebuffer> framebuffer;
};

// ============================================================================
// Basic Creation Tests
// ============================================================================

TEST_F(VulkanRenderPassEncoderTest, CreateRenderPassEncoder_CreatesSuccessfully)
{
    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    gfx::backend::vulkan::core::RenderPassEncoderBeginInfo beginInfo{};
    VkClearColorValue clearColor = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    beginInfo.colorClearValues = { clearColor };

    auto encoder = std::make_unique<gfx::backend::vulkan::core::RenderPassEncoder>(commandEncoder.get(), renderPass.get(), framebuffer.get(), beginInfo);

    EXPECT_NE(encoder->handle(), VK_NULL_HANDLE);
    EXPECT_EQ(encoder->device(), device.get());
    EXPECT_EQ(encoder->commandEncoder(), commandEncoder.get());

    commandEncoder->end();
}

// ============================================================================
// Handle Tests
// ============================================================================

TEST_F(VulkanRenderPassEncoderTest, Handle_ReturnsValidVkCommandBuffer)
{
    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    gfx::backend::vulkan::core::RenderPassEncoderBeginInfo beginInfo{};
    VkClearColorValue clearColor = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    beginInfo.colorClearValues = { clearColor };

    auto encoder = std::make_unique<gfx::backend::vulkan::core::RenderPassEncoder>(commandEncoder.get(), renderPass.get(), framebuffer.get(), beginInfo);

    VkCommandBuffer handle = encoder->handle();
    EXPECT_NE(handle, VK_NULL_HANDLE);
    EXPECT_EQ(handle, commandEncoder->handle());

    commandEncoder->end();
}

// ============================================================================
// Buffer Binding Tests
// ============================================================================

TEST_F(VulkanRenderPassEncoderTest, SetVertexBuffer_WorksCorrectly)
{
    gfx::backend::vulkan::core::BufferCreateInfo bufferInfo{};
    bufferInfo.size = 1024;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    auto buffer = std::make_unique<gfx::backend::vulkan::core::Buffer>(device.get(), bufferInfo);

    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    gfx::backend::vulkan::core::RenderPassEncoderBeginInfo beginInfo{};
    VkClearColorValue clearColor = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    beginInfo.colorClearValues = { clearColor };

    auto encoder = std::make_unique<gfx::backend::vulkan::core::RenderPassEncoder>(commandEncoder.get(), renderPass.get(), framebuffer.get(), beginInfo);

    EXPECT_NO_THROW(encoder->setVertexBuffer(0, buffer.get(), 0));

    commandEncoder->end();
}

TEST_F(VulkanRenderPassEncoderTest, SetIndexBuffer_WorksCorrectly)
{
    gfx::backend::vulkan::core::BufferCreateInfo bufferInfo{};
    bufferInfo.size = 1024;
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    auto buffer = std::make_unique<gfx::backend::vulkan::core::Buffer>(device.get(), bufferInfo);

    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    gfx::backend::vulkan::core::RenderPassEncoderBeginInfo beginInfo{};
    VkClearColorValue clearColor = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    beginInfo.colorClearValues = { clearColor };

    auto encoder = std::make_unique<gfx::backend::vulkan::core::RenderPassEncoder>(commandEncoder.get(), renderPass.get(), framebuffer.get(), beginInfo);

    EXPECT_NO_THROW(encoder->setIndexBuffer(buffer.get(), VK_INDEX_TYPE_UINT16, 0));

    commandEncoder->end();
}

// ============================================================================
// Viewport and Scissor Tests
// ============================================================================

TEST_F(VulkanRenderPassEncoderTest, SetViewport_WorksCorrectly)
{
    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    gfx::backend::vulkan::core::RenderPassEncoderBeginInfo beginInfo{};
    VkClearColorValue clearColor = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    beginInfo.colorClearValues = { clearColor };

    auto encoder = std::make_unique<gfx::backend::vulkan::core::RenderPassEncoder>(commandEncoder.get(), renderPass.get(), framebuffer.get(), beginInfo);

    gfx::backend::vulkan::core::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = 800.0f;
    viewport.height = 600.0f;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    EXPECT_NO_THROW(encoder->setViewport(viewport));

    commandEncoder->end();
}

TEST_F(VulkanRenderPassEncoderTest, SetScissorRect_WorksCorrectly)
{
    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    gfx::backend::vulkan::core::RenderPassEncoderBeginInfo beginInfo{};
    VkClearColorValue clearColor = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    beginInfo.colorClearValues = { clearColor };

    auto encoder = std::make_unique<gfx::backend::vulkan::core::RenderPassEncoder>(commandEncoder.get(), renderPass.get(), framebuffer.get(), beginInfo);

    gfx::backend::vulkan::core::ScissorRect scissor{};
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = 800;
    scissor.height = 600;

    EXPECT_NO_THROW(encoder->setScissorRect(scissor));

    commandEncoder->end();
}

// ============================================================================
// Query Tests
// ============================================================================

TEST_F(VulkanRenderPassEncoderTest, OcclusionQuery_WorksCorrectly)
{
    gfx::backend::vulkan::core::QuerySetCreateInfo queryInfo{};
    queryInfo.type = VK_QUERY_TYPE_OCCLUSION;
    queryInfo.count = 2;
    auto querySet = std::make_unique<gfx::backend::vulkan::core::QuerySet>(device.get(), queryInfo);

    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    gfx::backend::vulkan::core::RenderPassEncoderBeginInfo beginInfo{};
    VkClearColorValue clearColor = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    beginInfo.colorClearValues = { clearColor };

    auto encoder = std::make_unique<gfx::backend::vulkan::core::RenderPassEncoder>(commandEncoder.get(), renderPass.get(), framebuffer.get(), beginInfo);

    EXPECT_NO_THROW(encoder->beginOcclusionQuery(querySet->handle(), 0));
    EXPECT_NO_THROW(encoder->endOcclusionQuery());

    commandEncoder->end();
}

// ============================================================================
// Lifecycle Tests
// ============================================================================

TEST_F(VulkanRenderPassEncoderTest, Destructor_CleansUpResources)
{
    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    {
        gfx::backend::vulkan::core::RenderPassEncoderBeginInfo beginInfo{};
        VkClearColorValue clearColor = { { 0.0f, 0.0f, 0.0f, 1.0f } };
        beginInfo.colorClearValues = { clearColor };

        auto encoder = std::make_unique<gfx::backend::vulkan::core::RenderPassEncoder>(commandEncoder.get(), renderPass.get(), framebuffer.get(), beginInfo);
        EXPECT_NE(encoder->handle(), VK_NULL_HANDLE);
    }

    commandEncoder->end();

    // If we reach here without crashing, cleanup succeeded
    SUCCEED();
}

} // anonymous namespace
