#include <backend/vulkan/core/render/RenderPass.h>
#include <backend/vulkan/core/system/Adapter.h>
#include <backend/vulkan/core/system/Device.h>
#include <backend/vulkan/core/system/Instance.h>


#include <gtest/gtest.h>

// Test Vulkan core RenderPass class
// These tests verify the internal render pass implementation

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class VulkanRenderPassTest : public testing::Test {
protected:
    void SetUp() override
    {
        try {
            gfx::backend::vulkan::core::InstanceCreateInfo instInfo{};
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

TEST_F(VulkanRenderPassTest, CreateSingleColorAttachment_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::RenderPassCreateInfo createInfo{};

    gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
    colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    createInfo.colorAttachments.push_back(colorAtt);

    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), createInfo);

    EXPECT_NE(renderPass.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(renderPass.colorAttachmentCount(), 1u);
    EXPECT_FALSE(renderPass.hasDepthStencil());
}

TEST_F(VulkanRenderPassTest, CreateMultipleColorAttachments_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::RenderPassCreateInfo createInfo{};

    for (int i = 0; i < 3; ++i) {
        gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
        colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
        colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
        colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        createInfo.colorAttachments.push_back(colorAtt);
    }

    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), createInfo);

    EXPECT_NE(renderPass.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(renderPass.colorAttachmentCount(), 3u);
    EXPECT_FALSE(renderPass.hasDepthStencil());
}

TEST_F(VulkanRenderPassTest, CreateDepthOnlyRenderPass_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::RenderPassCreateInfo createInfo{};

    gfx::backend::vulkan::core::RenderPassDepthStencilAttachment depthAtt{};
    depthAtt.target.format = VK_FORMAT_D32_SFLOAT;
    depthAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    depthAtt.target.depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAtt.target.depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAtt.target.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAtt.target.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAtt.target.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    createInfo.depthStencilAttachment = depthAtt;

    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), createInfo);

    EXPECT_NE(renderPass.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(renderPass.colorAttachmentCount(), 0u);
    EXPECT_TRUE(renderPass.hasDepthStencil());
}

TEST_F(VulkanRenderPassTest, CreateColorAndDepthRenderPass_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::RenderPassCreateInfo createInfo{};

    gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
    colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    createInfo.colorAttachments.push_back(colorAtt);

    gfx::backend::vulkan::core::RenderPassDepthStencilAttachment depthAtt{};
    depthAtt.target.format = VK_FORMAT_D32_SFLOAT;
    depthAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    depthAtt.target.depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAtt.target.depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAtt.target.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAtt.target.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAtt.target.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    createInfo.depthStencilAttachment = depthAtt;

    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), createInfo);

    EXPECT_NE(renderPass.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(renderPass.colorAttachmentCount(), 1u);
    EXPECT_TRUE(renderPass.hasDepthStencil());
}

// ============================================================================
// Different Load/Store Operations
// ============================================================================

TEST_F(VulkanRenderPassTest, LoadOpLoad_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::RenderPassCreateInfo createInfo{};

    gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
    colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    createInfo.colorAttachments.push_back(colorAtt);

    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), createInfo);

    EXPECT_NE(renderPass.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanRenderPassTest, LoadOpDontCare_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::RenderPassCreateInfo createInfo{};

    gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
    colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    createInfo.colorAttachments.push_back(colorAtt);

    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), createInfo);

    EXPECT_NE(renderPass.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Different Formats
// ============================================================================

TEST_F(VulkanRenderPassTest, DifferentColorFormats_CreateSuccessfully)
{
    VkFormat formats[] = {
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R32G32B32A32_SFLOAT
    };

    for (auto format : formats) {
        gfx::backend::vulkan::core::RenderPassCreateInfo createInfo{};

        gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
        colorAtt.target.format = format;
        colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
        colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        createInfo.colorAttachments.push_back(colorAtt);

        gfx::backend::vulkan::core::RenderPass renderPass(device.get(), createInfo);

        EXPECT_NE(renderPass.handle(), VK_NULL_HANDLE);
    }
}

TEST_F(VulkanRenderPassTest, DifferentDepthFormats_CreateSuccessfully)
{
    VkFormat formats[] = {
        VK_FORMAT_D16_UNORM,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT
    };

    for (auto format : formats) {
        gfx::backend::vulkan::core::RenderPassCreateInfo createInfo{};

        gfx::backend::vulkan::core::RenderPassDepthStencilAttachment depthAtt{};
        depthAtt.target.format = format;
        depthAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
        depthAtt.target.depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAtt.target.depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAtt.target.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAtt.target.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAtt.target.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        createInfo.depthStencilAttachment = depthAtt;

        gfx::backend::vulkan::core::RenderPass renderPass(device.get(), createInfo);

        EXPECT_NE(renderPass.handle(), VK_NULL_HANDLE);
    }
}

// ============================================================================
// MSAA Tests
// ============================================================================

TEST_F(VulkanRenderPassTest, MSAAColorAttachment_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::RenderPassCreateInfo createInfo{};

    gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
    colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.target.sampleCount = VK_SAMPLE_COUNT_4_BIT;
    colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    createInfo.colorAttachments.push_back(colorAtt);

    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), createInfo);

    EXPECT_NE(renderPass.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanRenderPassTest, MSAAWithResolve_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::RenderPassCreateInfo createInfo{};

    gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
    colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.target.sampleCount = VK_SAMPLE_COUNT_4_BIT;
    colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    gfx::backend::vulkan::core::RenderPassColorAttachmentTarget resolveTarget{};
    resolveTarget.format = VK_FORMAT_R8G8B8A8_UNORM;
    resolveTarget.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    resolveTarget.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveTarget.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resolveTarget.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAtt.resolveTarget = resolveTarget;

    createInfo.colorAttachments.push_back(colorAtt);

    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), createInfo);

    EXPECT_NE(renderPass.handle(), VK_NULL_HANDLE);
    EXPECT_TRUE(renderPass.colorHasResolve()[0]);
}

// ============================================================================
// Final Layout Tests
// ============================================================================

TEST_F(VulkanRenderPassTest, PresentLayout_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::RenderPassCreateInfo createInfo{};

    gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
    colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    createInfo.colorAttachments.push_back(colorAtt);

    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), createInfo);

    EXPECT_NE(renderPass.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanRenderPassTest, ShaderReadOnlyLayout_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::RenderPassCreateInfo createInfo{};

    gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
    colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    createInfo.colorAttachments.push_back(colorAtt);

    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), createInfo);

    EXPECT_NE(renderPass.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Depth Stencil Separate Operations
// ============================================================================

TEST_F(VulkanRenderPassTest, DepthStencilSeparateOps_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::RenderPassCreateInfo createInfo{};

    gfx::backend::vulkan::core::RenderPassDepthStencilAttachment depthAtt{};
    depthAtt.target.format = VK_FORMAT_D24_UNORM_S8_UINT;
    depthAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    depthAtt.target.depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAtt.target.depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAtt.target.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depthAtt.target.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAtt.target.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    createInfo.depthStencilAttachment = depthAtt;

    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), createInfo);

    EXPECT_NE(renderPass.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Handle Tests
// ============================================================================

TEST_F(VulkanRenderPassTest, GetHandle_ReturnsValidHandle)
{
    gfx::backend::vulkan::core::RenderPassCreateInfo createInfo{};

    gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
    colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    createInfo.colorAttachments.push_back(colorAtt);

    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), createInfo);

    VkRenderPass handle = renderPass.handle();
    EXPECT_NE(handle, VK_NULL_HANDLE);
    EXPECT_EQ(renderPass.handle(), handle);
}

TEST_F(VulkanRenderPassTest, MultipleRenderPasses_HaveUniqueHandles)
{
    gfx::backend::vulkan::core::RenderPassCreateInfo createInfo{};

    gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
    colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    createInfo.colorAttachments.push_back(colorAtt);

    gfx::backend::vulkan::core::RenderPass renderPass1(device.get(), createInfo);
    gfx::backend::vulkan::core::RenderPass renderPass2(device.get(), createInfo);

    EXPECT_NE(renderPass1.handle(), renderPass2.handle());
}

// ============================================================================
// Property Tests
// ============================================================================

TEST_F(VulkanRenderPassTest, ColorAttachmentCount_ReturnsCorrectCount)
{
    gfx::backend::vulkan::core::RenderPassCreateInfo createInfo{};

    for (int i = 0; i < 4; ++i) {
        gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
        colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
        colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
        colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        createInfo.colorAttachments.push_back(colorAtt);
    }

    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), createInfo);

    EXPECT_EQ(renderPass.colorAttachmentCount(), 4u);
}

TEST_F(VulkanRenderPassTest, HasDepthStencil_ReturnsTrueWhenPresent)
{
    gfx::backend::vulkan::core::RenderPassCreateInfo createInfo{};

    gfx::backend::vulkan::core::RenderPassDepthStencilAttachment depthAtt{};
    depthAtt.target.format = VK_FORMAT_D32_SFLOAT;
    depthAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    depthAtt.target.depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAtt.target.depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAtt.target.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAtt.target.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAtt.target.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    createInfo.depthStencilAttachment = depthAtt;

    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), createInfo);

    EXPECT_TRUE(renderPass.hasDepthStencil());
}

TEST_F(VulkanRenderPassTest, HasDepthStencil_ReturnsFalseWhenAbsent)
{
    gfx::backend::vulkan::core::RenderPassCreateInfo createInfo{};

    gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
    colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    createInfo.colorAttachments.push_back(colorAtt);

    gfx::backend::vulkan::core::RenderPass renderPass(device.get(), createInfo);

    EXPECT_FALSE(renderPass.hasDepthStencil());
}

// ============================================================================
// Lifecycle Tests
// ============================================================================

TEST_F(VulkanRenderPassTest, CreateAndDestroy_WorksCorrectly)
{
    {
        gfx::backend::vulkan::core::RenderPassCreateInfo createInfo{};

        gfx::backend::vulkan::core::RenderPassColorAttachment colorAtt{};
        colorAtt.target.format = VK_FORMAT_R8G8B8A8_UNORM;
        colorAtt.target.sampleCount = VK_SAMPLE_COUNT_1_BIT;
        colorAtt.target.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAtt.target.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAtt.target.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        createInfo.colorAttachments.push_back(colorAtt);

        gfx::backend::vulkan::core::RenderPass renderPass(device.get(), createInfo);

        EXPECT_NE(renderPass.handle(), VK_NULL_HANDLE);
    }
    // RenderPass destroyed, no crash
}

} // namespace
