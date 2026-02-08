#include <backend/vulkan/converter/Conversions.h>
#include <backend/vulkan/core/CoreTypes.h>

#include <gtest/gtest.h>

// Test Vulkan conversion functions
// Tests pure conversion functions between C API types and Vulkan types

namespace {

// ============================================================================
// Format Conversion Tests
// ============================================================================

TEST(VulkanConversionsTest, GfxFormatToVkFormat_CommonFormats_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxFormatToVkFormat(GFX_TEXTURE_FORMAT_UNDEFINED), VK_FORMAT_UNDEFINED);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxFormatToVkFormat(GFX_TEXTURE_FORMAT_R8_UNORM), VK_FORMAT_R8_UNORM);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxFormatToVkFormat(GFX_TEXTURE_FORMAT_R8G8_UNORM), VK_FORMAT_R8G8_UNORM);
}

TEST(VulkanConversionsTest, GfxFormatToVkFormat_RGBA8Formats_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxFormatToVkFormat(GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM), VK_FORMAT_R8G8B8A8_UNORM);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxFormatToVkFormat(GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM_SRGB), VK_FORMAT_R8G8B8A8_SRGB);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxFormatToVkFormat(GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM), VK_FORMAT_B8G8R8A8_UNORM);
}

TEST(VulkanConversionsTest, GfxFormatToVkFormat_FloatFormats_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxFormatToVkFormat(GFX_TEXTURE_FORMAT_R32_FLOAT), VK_FORMAT_R32_SFLOAT);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxFormatToVkFormat(GFX_TEXTURE_FORMAT_R32G32_FLOAT), VK_FORMAT_R32G32_SFLOAT);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxFormatToVkFormat(GFX_TEXTURE_FORMAT_R32G32B32A32_FLOAT), VK_FORMAT_R32G32B32A32_SFLOAT);
}

TEST(VulkanConversionsTest, GfxFormatToVkFormat_DepthFormats_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxFormatToVkFormat(GFX_TEXTURE_FORMAT_DEPTH16_UNORM), VK_FORMAT_D16_UNORM);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxFormatToVkFormat(GFX_TEXTURE_FORMAT_DEPTH32_FLOAT), VK_FORMAT_D32_SFLOAT);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxFormatToVkFormat(GFX_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8), VK_FORMAT_D24_UNORM_S8_UINT);
}

TEST(VulkanConversionsTest, VkFormatToGfxFormat_RoundTrip_Preserves)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::vkFormatToGfxFormat(VK_FORMAT_R8G8B8A8_UNORM), GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkFormatToGfxFormat(VK_FORMAT_R8G8B8A8_SRGB), GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM_SRGB);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkFormatToGfxFormat(VK_FORMAT_D32_SFLOAT), GFX_TEXTURE_FORMAT_DEPTH32_FLOAT);
}

TEST(VulkanConversionsTest, IsDepthFormat_DepthFormats_ReturnsTrue)
{
    EXPECT_TRUE(gfx::backend::vulkan::converter::isDepthFormat(VK_FORMAT_D16_UNORM));
    EXPECT_TRUE(gfx::backend::vulkan::converter::isDepthFormat(VK_FORMAT_D32_SFLOAT));
    EXPECT_TRUE(gfx::backend::vulkan::converter::isDepthFormat(VK_FORMAT_D24_UNORM_S8_UINT));
    EXPECT_TRUE(gfx::backend::vulkan::converter::isDepthFormat(VK_FORMAT_D32_SFLOAT_S8_UINT));
}

TEST(VulkanConversionsTest, IsDepthFormat_ColorFormats_ReturnsFalse)
{
    EXPECT_FALSE(gfx::backend::vulkan::converter::isDepthFormat(VK_FORMAT_R8G8B8A8_UNORM));
    EXPECT_FALSE(gfx::backend::vulkan::converter::isDepthFormat(VK_FORMAT_R8G8B8A8_SRGB));
    EXPECT_FALSE(gfx::backend::vulkan::converter::isDepthFormat(VK_FORMAT_R32G32B32A32_SFLOAT));
}

// ============================================================================
// Buffer Usage Conversion Tests
// ============================================================================

TEST(VulkanConversionsTest, GfxBufferUsageToVkBufferUsage_SingleFlags_ConvertsCorrectly)
{
    EXPECT_TRUE(gfx::backend::vulkan::converter::gfxBufferUsageToVkBufferUsage(GFX_BUFFER_USAGE_VERTEX) & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    EXPECT_TRUE(gfx::backend::vulkan::converter::gfxBufferUsageToVkBufferUsage(GFX_BUFFER_USAGE_INDEX) & VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    EXPECT_TRUE(gfx::backend::vulkan::converter::gfxBufferUsageToVkBufferUsage(GFX_BUFFER_USAGE_UNIFORM) & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    EXPECT_TRUE(gfx::backend::vulkan::converter::gfxBufferUsageToVkBufferUsage(GFX_BUFFER_USAGE_STORAGE) & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

TEST(VulkanConversionsTest, GfxBufferUsageToVkBufferUsage_MultipleFlags_CombinesCorrectly)
{
    VkBufferUsageFlags result = gfx::backend::vulkan::converter::gfxBufferUsageToVkBufferUsage(
        GFX_BUFFER_USAGE_VERTEX | GFX_BUFFER_USAGE_UNIFORM);

    EXPECT_TRUE(result & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    EXPECT_TRUE(result & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
}

TEST(VulkanConversionsTest, VkBufferUsageToGfxBufferUsage_RoundTrip_Preserves)
{
    GfxBufferUsageFlags original = GFX_BUFFER_USAGE_VERTEX | GFX_BUFFER_USAGE_UNIFORM;
    VkBufferUsageFlags vk = gfx::backend::vulkan::converter::gfxBufferUsageToVkBufferUsage(original);
    GfxBufferUsageFlags result = gfx::backend::vulkan::converter::vkBufferUsageToGfxBufferUsage(vk);

    EXPECT_TRUE(result & GFX_BUFFER_USAGE_VERTEX);
    EXPECT_TRUE(result & GFX_BUFFER_USAGE_UNIFORM);
}

// ============================================================================
// Texture Usage Conversion Tests
// ============================================================================

TEST(VulkanConversionsTest, GfxTextureUsageToVkImageUsage_SingleFlags_ConvertsCorrectly)
{
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    EXPECT_TRUE(gfx::backend::vulkan::converter::gfxTextureUsageToVkImageUsage(GFX_TEXTURE_USAGE_TEXTURE_BINDING, format) & VK_IMAGE_USAGE_SAMPLED_BIT);
    EXPECT_TRUE(gfx::backend::vulkan::converter::gfxTextureUsageToVkImageUsage(GFX_TEXTURE_USAGE_STORAGE_BINDING, format) & VK_IMAGE_USAGE_STORAGE_BIT);
    EXPECT_TRUE(gfx::backend::vulkan::converter::gfxTextureUsageToVkImageUsage(GFX_TEXTURE_USAGE_RENDER_ATTACHMENT, format) & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
}

TEST(VulkanConversionsTest, GfxTextureUsageToVkImageUsage_DepthFormat_AddsDepthBit)
{
    VkImageUsageFlags result = gfx::backend::vulkan::converter::gfxTextureUsageToVkImageUsage(
        GFX_TEXTURE_USAGE_RENDER_ATTACHMENT, VK_FORMAT_D32_SFLOAT);

    EXPECT_TRUE(result & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

TEST(VulkanConversionsTest, VkImageUsageToGfxTextureUsage_RoundTrip_Preserves)
{
    VkImageUsageFlags vk = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    GfxTextureUsageFlags result = gfx::backend::vulkan::converter::vkImageUsageToGfxTextureUsage(vk);

    EXPECT_TRUE(result & GFX_TEXTURE_USAGE_TEXTURE_BINDING);
    EXPECT_TRUE(result & GFX_TEXTURE_USAGE_STORAGE_BINDING);
}

// ============================================================================
// Index Format Conversion Tests
// ============================================================================

TEST(VulkanConversionsTest, GfxIndexFormatToVkIndexType_ValidFormats_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxIndexFormatToVkIndexType(GFX_INDEX_FORMAT_UINT16), VK_INDEX_TYPE_UINT16);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxIndexFormatToVkIndexType(GFX_INDEX_FORMAT_UINT32), VK_INDEX_TYPE_UINT32);
}

// ============================================================================
// Load/Store Op Conversion Tests
// ============================================================================

TEST(VulkanConversionsTest, GfxLoadOpToVkLoadOp_AllOps_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxLoadOpToVkLoadOp(GFX_LOAD_OP_LOAD), VK_ATTACHMENT_LOAD_OP_LOAD);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxLoadOpToVkLoadOp(GFX_LOAD_OP_CLEAR), VK_ATTACHMENT_LOAD_OP_CLEAR);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxLoadOpToVkLoadOp(GFX_LOAD_OP_DONT_CARE), VK_ATTACHMENT_LOAD_OP_DONT_CARE);
}

TEST(VulkanConversionsTest, GfxStoreOpToVkStoreOp_AllOps_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxStoreOpToVkStoreOp(GFX_STORE_OP_STORE), VK_ATTACHMENT_STORE_OP_STORE);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxStoreOpToVkStoreOp(GFX_STORE_OP_DONT_CARE), VK_ATTACHMENT_STORE_OP_DONT_CARE);
}

// ============================================================================
// Pipeline Stage Conversion Tests
// ============================================================================

TEST(VulkanConversionsTest, GfxPipelineStageFlagsToVkPipelineStageFlags_SingleFlags_ConvertsCorrectly)
{
    EXPECT_TRUE(gfx::backend::vulkan::converter::gfxPipelineStageFlagsToVkPipelineStageFlags(GFX_PIPELINE_STAGE_TOP_OF_PIPE) & VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    EXPECT_TRUE(gfx::backend::vulkan::converter::gfxPipelineStageFlagsToVkPipelineStageFlags(GFX_PIPELINE_STAGE_VERTEX_SHADER) & VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
    EXPECT_TRUE(gfx::backend::vulkan::converter::gfxPipelineStageFlagsToVkPipelineStageFlags(GFX_PIPELINE_STAGE_FRAGMENT_SHADER) & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    EXPECT_TRUE(gfx::backend::vulkan::converter::gfxPipelineStageFlagsToVkPipelineStageFlags(GFX_PIPELINE_STAGE_COMPUTE_SHADER) & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
}

TEST(VulkanConversionsTest, GfxPipelineStageFlagsToVkPipelineStageFlags_MultipleFlags_CombinesCorrectly)
{
    VkPipelineStageFlags result = gfx::backend::vulkan::converter::gfxPipelineStageFlagsToVkPipelineStageFlags(
        GFX_PIPELINE_STAGE_VERTEX_SHADER | GFX_PIPELINE_STAGE_FRAGMENT_SHADER);

    EXPECT_TRUE(result & VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
    EXPECT_TRUE(result & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

// ============================================================================
// Access Flags Conversion Tests
// ============================================================================

TEST(VulkanConversionsTest, GfxAccessFlagsToVkAccessFlags_SingleFlags_ConvertsCorrectly)
{
    EXPECT_TRUE(gfx::backend::vulkan::converter::gfxAccessFlagsToVkAccessFlags(GFX_ACCESS_SHADER_READ) & VK_ACCESS_SHADER_READ_BIT);
    EXPECT_TRUE(gfx::backend::vulkan::converter::gfxAccessFlagsToVkAccessFlags(GFX_ACCESS_SHADER_WRITE) & VK_ACCESS_SHADER_WRITE_BIT);
    EXPECT_TRUE(gfx::backend::vulkan::converter::gfxAccessFlagsToVkAccessFlags(GFX_ACCESS_TRANSFER_READ) & VK_ACCESS_TRANSFER_READ_BIT);
    EXPECT_TRUE(gfx::backend::vulkan::converter::gfxAccessFlagsToVkAccessFlags(GFX_ACCESS_TRANSFER_WRITE) & VK_ACCESS_TRANSFER_WRITE_BIT);
}

TEST(VulkanConversionsTest, GfxAccessFlagsToVkAccessFlags_MultipleFlags_CombinesCorrectly)
{
    VkAccessFlags result = gfx::backend::vulkan::converter::gfxAccessFlagsToVkAccessFlags(
        GFX_ACCESS_SHADER_READ | GFX_ACCESS_SHADER_WRITE);

    EXPECT_TRUE(result & VK_ACCESS_SHADER_READ_BIT);
    EXPECT_TRUE(result & VK_ACCESS_SHADER_WRITE_BIT);
}

// ============================================================================
// Adapter Type Conversion Tests
// ============================================================================

TEST(VulkanConversionsTest, VkDeviceTypeToGfxAdapterType_AllTypes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::vkDeviceTypeToGfxAdapterType(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU), GFX_ADAPTER_TYPE_DISCRETE_GPU);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkDeviceTypeToGfxAdapterType(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU), GFX_ADAPTER_TYPE_INTEGRATED_GPU);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkDeviceTypeToGfxAdapterType(VK_PHYSICAL_DEVICE_TYPE_CPU), GFX_ADAPTER_TYPE_CPU);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkDeviceTypeToGfxAdapterType(VK_PHYSICAL_DEVICE_TYPE_OTHER), GFX_ADAPTER_TYPE_UNKNOWN);
}

// ============================================================================
// Queue Flags Conversion Tests
// ============================================================================

TEST(VulkanConversionsTest, VkQueueFlagsToGfx_SingleFlags_ConvertsCorrectly)
{
    EXPECT_TRUE(gfx::backend::vulkan::converter::vkQueueFlagsToGfx(VK_QUEUE_GRAPHICS_BIT) & GFX_QUEUE_FLAG_GRAPHICS);
    EXPECT_TRUE(gfx::backend::vulkan::converter::vkQueueFlagsToGfx(VK_QUEUE_COMPUTE_BIT) & GFX_QUEUE_FLAG_COMPUTE);
    EXPECT_TRUE(gfx::backend::vulkan::converter::vkQueueFlagsToGfx(VK_QUEUE_TRANSFER_BIT) & GFX_QUEUE_FLAG_TRANSFER);
}

TEST(VulkanConversionsTest, VkQueueFlagsToGfx_MultipleFlags_CombinesCorrectly)
{
    GfxQueueFlags result = gfx::backend::vulkan::converter::vkQueueFlagsToGfx(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

    EXPECT_TRUE(result & GFX_QUEUE_FLAG_GRAPHICS);
    EXPECT_TRUE(result & GFX_QUEUE_FLAG_COMPUTE);
}

// ============================================================================
// Semaphore Type Conversion Tests
// ============================================================================

TEST(VulkanConversionsTest, GfxSemaphoreTypeToVulkanSemaphoreType_AllTypes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxSemaphoreTypeToVulkanSemaphoreType(GFX_SEMAPHORE_TYPE_BINARY), gfx::backend::vulkan::core::SemaphoreType::Binary);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxSemaphoreTypeToVulkanSemaphoreType(GFX_SEMAPHORE_TYPE_TIMELINE), gfx::backend::vulkan::core::SemaphoreType::Timeline);
}

// ============================================================================
// Handle Conversion Tests (Templates)
// ============================================================================

TEST(VulkanConversionsTest, ToGfx_NullPointer_ReturnsNullHandle)
{
    int* ptr = nullptr;
    GfxBuffer handle = gfx::backend::vulkan::converter::toGfx<GfxBuffer>(ptr);
    EXPECT_EQ(handle, nullptr);
}

TEST(VulkanConversionsTest, ToNative_NullHandle_ReturnsNullPointer)
{
    GfxBuffer handle = nullptr;
    int* ptr = gfx::backend::vulkan::converter::toNative<int>(handle);
    EXPECT_EQ(ptr, nullptr);
}

TEST(VulkanConversionsTest, ToGfxToNative_RoundTrip_Preserves)
{
    // Create a dummy pointer value (not dereferenced)
    int* originalPtr = reinterpret_cast<int*>(0x12345678);

    GfxBuffer handle = gfx::backend::vulkan::converter::toGfx<GfxBuffer>(originalPtr);
    int* resultPtr = gfx::backend::vulkan::converter::toNative<int>(handle);

    EXPECT_EQ(resultPtr, originalPtr);
}

// ============================================================================
// Memory Property Conversion Tests
// ============================================================================

TEST(VulkanConversionsTest, GfxMemoryPropertyToVkMemoryProperty_SingleFlags_ConvertsCorrectly)
{
    EXPECT_TRUE(gfx::backend::vulkan::converter::gfxMemoryPropertyToVkMemoryProperty(GFX_MEMORY_PROPERTY_DEVICE_LOCAL) & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    EXPECT_TRUE(gfx::backend::vulkan::converter::gfxMemoryPropertyToVkMemoryProperty(GFX_MEMORY_PROPERTY_HOST_VISIBLE) & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    EXPECT_TRUE(gfx::backend::vulkan::converter::gfxMemoryPropertyToVkMemoryProperty(GFX_MEMORY_PROPERTY_HOST_COHERENT) & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    EXPECT_TRUE(gfx::backend::vulkan::converter::gfxMemoryPropertyToVkMemoryProperty(GFX_MEMORY_PROPERTY_HOST_CACHED) & VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
}

TEST(VulkanConversionsTest, GfxMemoryPropertyToVkMemoryProperty_MultipleFlags_CombinesCorrectly)
{
    VkMemoryPropertyFlags result = gfx::backend::vulkan::converter::gfxMemoryPropertyToVkMemoryProperty(
        GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT);

    EXPECT_TRUE(result & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    EXPECT_TRUE(result & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

TEST(VulkanConversionsTest, VkMemoryPropertyToGfxMemoryProperty_RoundTrip_Preserves)
{
    GfxMemoryPropertyFlags original = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT;
    VkMemoryPropertyFlags vk = gfx::backend::vulkan::converter::gfxMemoryPropertyToVkMemoryProperty(original);
    GfxMemoryPropertyFlags result = gfx::backend::vulkan::converter::vkMemoryPropertyToGfxMemoryProperty(vk);

    EXPECT_TRUE(result & GFX_MEMORY_PROPERTY_HOST_VISIBLE);
    EXPECT_TRUE(result & GFX_MEMORY_PROPERTY_HOST_COHERENT);
}

TEST(VulkanConversionsTest, GfxMemoryPropertyToVkMemoryProperty_DeviceLocal_ConvertsCorrectly)
{
    VkMemoryPropertyFlags result = gfx::backend::vulkan::converter::gfxMemoryPropertyToVkMemoryProperty(GFX_MEMORY_PROPERTY_DEVICE_LOCAL);
    EXPECT_EQ(result, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

TEST(VulkanConversionsTest, GfxMemoryPropertyToVkMemoryProperty_HostVisibleCoherent_ConvertsCorrectly)
{
    VkMemoryPropertyFlags result = gfx::backend::vulkan::converter::gfxMemoryPropertyToVkMemoryProperty(
        GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT);

    EXPECT_TRUE(result & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    EXPECT_TRUE(result & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    EXPECT_FALSE(result & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

// ============================================================================
// Viewport and Scissor Rect Conversions
// ============================================================================

TEST(VulkanConversionsTest, GfxViewportToViewport_AllFields_ConvertsCorrectly)
{
    GfxViewport gfxViewport = { 10.0f, 20.0f, 800.0f, 600.0f, 0.0f, 1.0f };

    gfx::backend::vulkan::core::Viewport result = gfx::backend::vulkan::converter::gfxViewportToViewport(&gfxViewport);

    EXPECT_FLOAT_EQ(result.x, 10.0f);
    EXPECT_FLOAT_EQ(result.y, 20.0f);
    EXPECT_FLOAT_EQ(result.width, 800.0f);
    EXPECT_FLOAT_EQ(result.height, 600.0f);
    EXPECT_FLOAT_EQ(result.minDepth, 0.0f);
    EXPECT_FLOAT_EQ(result.maxDepth, 1.0f);
}

TEST(VulkanConversionsTest, GfxViewportToViewport_NegativeCoordinates_ConvertsCorrectly)
{
    GfxViewport gfxViewport = { -50.0f, -100.0f, 1920.0f, 1080.0f, 0.1f, 0.9f };

    gfx::backend::vulkan::core::Viewport result = gfx::backend::vulkan::converter::gfxViewportToViewport(&gfxViewport);

    EXPECT_FLOAT_EQ(result.x, -50.0f);
    EXPECT_FLOAT_EQ(result.y, -100.0f);
    EXPECT_FLOAT_EQ(result.width, 1920.0f);
    EXPECT_FLOAT_EQ(result.height, 1080.0f);
    EXPECT_FLOAT_EQ(result.minDepth, 0.1f);
    EXPECT_FLOAT_EQ(result.maxDepth, 0.9f);
}

TEST(VulkanConversionsTest, GfxScissorRectToScissorRect_AllFields_ConvertsCorrectly)
{
    GfxScissorRect gfxScissor = { 100, 200, 640, 480 };

    gfx::backend::vulkan::core::ScissorRect result = gfx::backend::vulkan::converter::gfxScissorRectToScissorRect(&gfxScissor);

    EXPECT_EQ(result.x, 100);
    EXPECT_EQ(result.y, 200);
    EXPECT_EQ(result.width, 640u);
    EXPECT_EQ(result.height, 480u);
}

TEST(VulkanConversionsTest, GfxScissorRectToScissorRect_NegativeOrigin_ConvertsCorrectly)
{
    GfxScissorRect gfxScissor = { -10, -20, 800, 600 };

    gfx::backend::vulkan::core::ScissorRect result = gfx::backend::vulkan::converter::gfxScissorRectToScissorRect(&gfxScissor);

    EXPECT_EQ(result.x, -10);
    EXPECT_EQ(result.y, -20);
    EXPECT_EQ(result.width, 800u);
    EXPECT_EQ(result.height, 600u);
}

// ============================================================================
// Barrier Conversions
// ============================================================================

TEST(VulkanConversionsTest, GfxMemoryBarrierToMemoryBarrier_AllFields_ConvertsCorrectly)
{
    GfxMemoryBarrier gfxBarrier = {
        GFX_PIPELINE_STAGE_VERTEX_SHADER,
        GFX_PIPELINE_STAGE_FRAGMENT_SHADER,
        GFX_ACCESS_SHADER_WRITE,
        GFX_ACCESS_SHADER_READ
    };

    gfx::backend::vulkan::core::MemoryBarrier result = gfx::backend::vulkan::converter::gfxMemoryBarrierToMemoryBarrier(gfxBarrier);

    // Check that the appropriate bits are set (converter may set additional flags like ALL_GRAPHICS)
    EXPECT_TRUE(result.srcStageMask & VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
    EXPECT_TRUE(result.dstStageMask & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    EXPECT_TRUE(result.srcAccessMask & VK_ACCESS_SHADER_WRITE_BIT);
    EXPECT_TRUE(result.dstAccessMask & VK_ACCESS_SHADER_READ_BIT);
}

TEST(VulkanConversionsTest, GfxMemoryBarrierToMemoryBarrier_MultipleStages_ConvertsCorrectly)
{
    GfxMemoryBarrier gfxBarrier = {
        GFX_PIPELINE_STAGE_COMPUTE_SHADER | GFX_PIPELINE_STAGE_TRANSFER,
        GFX_PIPELINE_STAGE_FRAGMENT_SHADER | GFX_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT,
        GFX_ACCESS_SHADER_WRITE | GFX_ACCESS_TRANSFER_WRITE,
        GFX_ACCESS_SHADER_READ | GFX_ACCESS_COLOR_ATTACHMENT_READ
    };

    gfx::backend::vulkan::core::MemoryBarrier result = gfx::backend::vulkan::converter::gfxMemoryBarrierToMemoryBarrier(gfxBarrier);

    EXPECT_TRUE(result.srcStageMask & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    EXPECT_TRUE(result.srcStageMask & VK_PIPELINE_STAGE_TRANSFER_BIT);
    EXPECT_TRUE(result.dstStageMask & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    EXPECT_TRUE(result.dstStageMask & VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
}

TEST(VulkanConversionsTest, GfxBufferBarrierToBufferBarrier_AllFields_ConvertsCorrectly)
{
    // Use a dummy buffer pointer for testing (not dereferenced)
    auto* dummyBuffer = reinterpret_cast<GfxBuffer>(0x12345678);

    GfxBufferBarrier gfxBarrier = {
        dummyBuffer,
        GFX_PIPELINE_STAGE_COMPUTE_SHADER,
        GFX_PIPELINE_STAGE_VERTEX_SHADER,
        GFX_ACCESS_SHADER_WRITE,
        GFX_ACCESS_VERTEX_ATTRIBUTE_READ,
        1024,
        2048
    };

    gfx::backend::vulkan::core::BufferBarrier result = gfx::backend::vulkan::converter::gfxBufferBarrierToBufferBarrier(gfxBarrier);

    EXPECT_EQ(result.buffer, reinterpret_cast<gfx::backend::vulkan::core::Buffer*>(dummyBuffer));
    EXPECT_TRUE(result.srcStageMask & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    EXPECT_TRUE(result.dstStageMask & VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
    EXPECT_TRUE(result.srcAccessMask & VK_ACCESS_SHADER_WRITE_BIT);
    EXPECT_TRUE(result.dstAccessMask & VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
    EXPECT_EQ(result.offset, 1024u);
    EXPECT_EQ(result.size, 2048u);
}

TEST(VulkanConversionsTest, GfxBufferBarrierToBufferBarrier_WholeBuffer_ConvertsCorrectly)
{
    auto* dummyBuffer = reinterpret_cast<GfxBuffer>(0x11223344);

    GfxBufferBarrier gfxBarrier = {
        dummyBuffer,
        GFX_PIPELINE_STAGE_TRANSFER,
        GFX_PIPELINE_STAGE_COMPUTE_SHADER,
        GFX_ACCESS_TRANSFER_WRITE,
        GFX_ACCESS_SHADER_READ,
        0,
        0 // 0 means whole buffer
    };

    gfx::backend::vulkan::core::BufferBarrier result = gfx::backend::vulkan::converter::gfxBufferBarrierToBufferBarrier(gfxBarrier);

    EXPECT_EQ(result.offset, 0u);
    EXPECT_EQ(result.size, 0u); // Backend interprets 0 as whole buffer
}

TEST(VulkanConversionsTest, GfxTextureBarrierToTextureBarrier_AllFields_ConvertsCorrectly)
{
    auto* dummyTexture = reinterpret_cast<GfxTexture>(0xABCDEF00);

    GfxTextureBarrier gfxBarrier = {
        dummyTexture,
        GFX_TEXTURE_LAYOUT_UNDEFINED,
        GFX_TEXTURE_LAYOUT_SHADER_READ_ONLY,
        GFX_PIPELINE_STAGE_TOP_OF_PIPE,
        GFX_PIPELINE_STAGE_FRAGMENT_SHADER,
        GFX_ACCESS_NONE,
        GFX_ACCESS_SHADER_READ,
        0,
        1,
        0,
        1
    };

    gfx::backend::vulkan::core::TextureBarrier result = gfx::backend::vulkan::converter::gfxTextureBarrierToTextureBarrier(gfxBarrier);

    EXPECT_EQ(result.texture, reinterpret_cast<gfx::backend::vulkan::core::Texture*>(dummyTexture));
    EXPECT_TRUE(result.srcStageMask & VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    EXPECT_TRUE(result.dstStageMask & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    EXPECT_EQ(result.srcAccessMask, 0u);
    EXPECT_TRUE(result.dstAccessMask & VK_ACCESS_SHADER_READ_BIT);
    EXPECT_EQ(result.oldLayout, VK_IMAGE_LAYOUT_UNDEFINED);
    EXPECT_EQ(result.newLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    EXPECT_EQ(result.baseMipLevel, 0u);
    EXPECT_EQ(result.mipLevelCount, 1u);
    EXPECT_EQ(result.baseArrayLayer, 0u);
    EXPECT_EQ(result.arrayLayerCount, 1u);
}

TEST(VulkanConversionsTest, GfxTextureBarrierToTextureBarrier_MipmapArrayTexture_ConvertsCorrectly)
{
    auto* dummyTexture = reinterpret_cast<GfxTexture>(0xDEADBEEF);

    GfxTextureBarrier gfxBarrier = {
        dummyTexture,
        GFX_TEXTURE_LAYOUT_TRANSFER_DST,
        GFX_TEXTURE_LAYOUT_SHADER_READ_ONLY,
        GFX_PIPELINE_STAGE_TRANSFER,
        GFX_PIPELINE_STAGE_FRAGMENT_SHADER,
        GFX_ACCESS_TRANSFER_WRITE,
        GFX_ACCESS_SHADER_READ,
        2, // baseMipLevel
        5, // mipLevelCount
        1, // baseArrayLayer
        6 // arrayLayerCount (cube map)
    };

    gfx::backend::vulkan::core::TextureBarrier result = gfx::backend::vulkan::converter::gfxTextureBarrierToTextureBarrier(gfxBarrier);

    EXPECT_EQ(result.oldLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    EXPECT_EQ(result.newLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    EXPECT_EQ(result.baseMipLevel, 2u);
    EXPECT_EQ(result.mipLevelCount, 5u);
    EXPECT_EQ(result.baseArrayLayer, 1u);
    EXPECT_EQ(result.arrayLayerCount, 6u);
}

// ============================================================================
// Device Limits Conversion
// ============================================================================

TEST(VulkanConversionsTest, VkPropertiesToGfxDeviceLimits_CommonLimits_ConvertsCorrectly)
{
    VkPhysicalDeviceProperties props{};
    props.limits.maxImageDimension1D = 16384;
    props.limits.maxImageDimension2D = 16384;
    props.limits.maxImageDimension3D = 2048;
    props.limits.maxImageArrayLayers = 2048;

    GfxDeviceLimits result = gfx::backend::vulkan::converter::vkPropertiesToGfxDeviceLimits(props);

    EXPECT_EQ(result.maxTextureDimension1D, 16384u);
    EXPECT_EQ(result.maxTextureDimension2D, 16384u);
    EXPECT_EQ(result.maxTextureDimension3D, 2048u);
    EXPECT_EQ(result.maxTextureArrayLayers, 2048u);
}

TEST(VulkanConversionsTest, VkPropertiesToGfxDeviceLimits_BufferLimits_ConvertsCorrectly)
{
    VkPhysicalDeviceProperties props{};
    props.limits.maxUniformBufferRange = 65536;
    props.limits.maxStorageBufferRange = 134217728;
    props.limits.minUniformBufferOffsetAlignment = 256;
    props.limits.minStorageBufferOffsetAlignment = 256;

    GfxDeviceLimits result = gfx::backend::vulkan::converter::vkPropertiesToGfxDeviceLimits(props);

    EXPECT_EQ(result.maxUniformBufferBindingSize, 65536u);
    EXPECT_EQ(result.maxStorageBufferBindingSize, 134217728u);
    EXPECT_EQ(result.minUniformBufferOffsetAlignment, 256u);
    EXPECT_EQ(result.minStorageBufferOffsetAlignment, 256u);
}

// ============================================================================
// Queue Family Conversion
// ============================================================================

TEST(VulkanConversionsTest, VkQueueFamilyPropertiesToGfx_AllFields_ConvertsCorrectly)
{
    VkQueueFamilyProperties vkProps{};
    vkProps.queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
    vkProps.queueCount = 16;
    vkProps.timestampValidBits = 64;
    vkProps.minImageTransferGranularity = { 1, 1, 1 };

    GfxQueueFamilyProperties result = gfx::backend::vulkan::converter::vkQueueFamilyPropertiesToGfx(vkProps);

    EXPECT_TRUE(result.flags & GFX_QUEUE_FLAG_GRAPHICS);
    EXPECT_TRUE(result.flags & GFX_QUEUE_FLAG_COMPUTE);
    EXPECT_TRUE(result.flags & GFX_QUEUE_FLAG_TRANSFER);
    EXPECT_EQ(result.queueCount, 16u);
}

TEST(VulkanConversionsTest, VkQueueFamilyPropertiesToGfx_ComputeOnly_ConvertsCorrectly)
{
    VkQueueFamilyProperties vkProps{};
    vkProps.queueFlags = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
    vkProps.queueCount = 8;
    vkProps.timestampValidBits = 64;
    vkProps.minImageTransferGranularity = { 1, 1, 1 };

    GfxQueueFamilyProperties result = gfx::backend::vulkan::converter::vkQueueFamilyPropertiesToGfx(vkProps);

    EXPECT_FALSE(result.flags & GFX_QUEUE_FLAG_GRAPHICS);
    EXPECT_TRUE(result.flags & GFX_QUEUE_FLAG_COMPUTE);
    EXPECT_TRUE(result.flags & GFX_QUEUE_FLAG_TRANSFER);
    EXPECT_EQ(result.queueCount, 8u);
}

// ============================================================================
// Adapter Info Conversion
// ============================================================================

TEST(VulkanConversionsTest, VkPropertiesToGfxAdapterInfo_DiscreteGPU_ConvertsCorrectly)
{
    VkPhysicalDeviceProperties vkProps{};
    vkProps.deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    vkProps.vendorID = 0x10DE; // NVIDIA
    vkProps.deviceID = 0x1234;
    strncpy(vkProps.deviceName, "NVIDIA GeForce RTX 4090", VK_MAX_PHYSICAL_DEVICE_NAME_SIZE - 1);

    GfxAdapterInfo result = gfx::backend::vulkan::converter::vkPropertiesToGfxAdapterInfo(vkProps);

    EXPECT_EQ(result.adapterType, GFX_ADAPTER_TYPE_DISCRETE_GPU);
    EXPECT_EQ(result.vendorID, 0x10DE);
    EXPECT_EQ(result.deviceID, 0x1234);
    EXPECT_STREQ(result.name, "NVIDIA GeForce RTX 4090");
}

TEST(VulkanConversionsTest, VkPropertiesToGfxAdapterInfo_IntegratedGPU_ConvertsCorrectly)
{
    VkPhysicalDeviceProperties vkProps{};
    vkProps.deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
    vkProps.vendorID = 0x8086; // Intel
    vkProps.deviceID = 0x5678;
    strncpy(vkProps.deviceName, "Intel Iris Xe Graphics", VK_MAX_PHYSICAL_DEVICE_NAME_SIZE - 1);

    GfxAdapterInfo result = gfx::backend::vulkan::converter::vkPropertiesToGfxAdapterInfo(vkProps);

    EXPECT_EQ(result.adapterType, GFX_ADAPTER_TYPE_INTEGRATED_GPU);
    EXPECT_EQ(result.vendorID, 0x8086);
    EXPECT_EQ(result.deviceID, 0x5678);
    EXPECT_STREQ(result.name, "Intel Iris Xe Graphics");
}

TEST(VulkanConversionsTest, VkPropertiesToGfxAdapterInfo_CPU_ConvertsCorrectly)
{
    VkPhysicalDeviceProperties vkProps{};
    vkProps.deviceType = VK_PHYSICAL_DEVICE_TYPE_CPU;
    vkProps.vendorID = 0xFFFF;
    vkProps.deviceID = 0x0001;
    strncpy(vkProps.deviceName, "SwiftShader", VK_MAX_PHYSICAL_DEVICE_NAME_SIZE - 1);

    GfxAdapterInfo result = gfx::backend::vulkan::converter::vkPropertiesToGfxAdapterInfo(vkProps);

    EXPECT_EQ(result.adapterType, GFX_ADAPTER_TYPE_CPU);
    EXPECT_EQ(result.vendorID, 0xFFFF);
    EXPECT_EQ(result.deviceID, 0x0001);
    EXPECT_STREQ(result.name, "SwiftShader");
}

// ============================================================================
// Layout Conversions
// ============================================================================

TEST(VulkanConversionsTest, GfxLayoutToVkImageLayout_AllLayouts_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxLayoutToVkImageLayout(GFX_TEXTURE_LAYOUT_UNDEFINED), VK_IMAGE_LAYOUT_UNDEFINED);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxLayoutToVkImageLayout(GFX_TEXTURE_LAYOUT_GENERAL), VK_IMAGE_LAYOUT_GENERAL);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxLayoutToVkImageLayout(GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxLayoutToVkImageLayout(GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxLayoutToVkImageLayout(GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY), VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxLayoutToVkImageLayout(GFX_TEXTURE_LAYOUT_SHADER_READ_ONLY), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxLayoutToVkImageLayout(GFX_TEXTURE_LAYOUT_TRANSFER_SRC), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxLayoutToVkImageLayout(GFX_TEXTURE_LAYOUT_TRANSFER_DST), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxLayoutToVkImageLayout(GFX_TEXTURE_LAYOUT_PRESENT_SRC), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

TEST(VulkanConversionsTest, VkImageLayoutToGfxLayout_AllLayouts_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::vkImageLayoutToGfxLayout(VK_IMAGE_LAYOUT_UNDEFINED), GFX_TEXTURE_LAYOUT_UNDEFINED);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkImageLayoutToGfxLayout(VK_IMAGE_LAYOUT_GENERAL), GFX_TEXTURE_LAYOUT_GENERAL);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkImageLayoutToGfxLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL), GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkImageLayoutToGfxLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL), GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkImageLayoutToGfxLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL), GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkImageLayoutToGfxLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL), GFX_TEXTURE_LAYOUT_SHADER_READ_ONLY);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkImageLayoutToGfxLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL), GFX_TEXTURE_LAYOUT_TRANSFER_SRC);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkImageLayoutToGfxLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL), GFX_TEXTURE_LAYOUT_TRANSFER_DST);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkImageLayoutToGfxLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR), GFX_TEXTURE_LAYOUT_PRESENT_SRC);
}

TEST(VulkanConversionsTest, LayoutConversion_RoundTrip_Preserves)
{
    GfxTextureLayout layouts[] = {
        GFX_TEXTURE_LAYOUT_UNDEFINED,
        GFX_TEXTURE_LAYOUT_GENERAL,
        GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT,
        GFX_TEXTURE_LAYOUT_SHADER_READ_ONLY,
        GFX_TEXTURE_LAYOUT_TRANSFER_SRC,
        GFX_TEXTURE_LAYOUT_TRANSFER_DST
    };

    for (auto layout : layouts) {
        VkImageLayout vkLayout = gfx::backend::vulkan::converter::gfxLayoutToVkImageLayout(layout);
        GfxTextureLayout result = gfx::backend::vulkan::converter::vkImageLayoutToGfxLayout(vkLayout);
        EXPECT_EQ(result, layout);
    }
}

// ============================================================================
// Texture Type Conversions
// ============================================================================

TEST(VulkanConversionsTest, GfxTextureTypeToVkImageType_AllTypes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxTextureTypeToVkImageType(GFX_TEXTURE_TYPE_1D), VK_IMAGE_TYPE_1D);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxTextureTypeToVkImageType(GFX_TEXTURE_TYPE_2D), VK_IMAGE_TYPE_2D);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxTextureTypeToVkImageType(GFX_TEXTURE_TYPE_3D), VK_IMAGE_TYPE_3D);
}

TEST(VulkanConversionsTest, VkImageTypeToGfxTextureType_AllTypes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::vkImageTypeToGfxTextureType(VK_IMAGE_TYPE_1D), GFX_TEXTURE_TYPE_1D);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkImageTypeToGfxTextureType(VK_IMAGE_TYPE_2D), GFX_TEXTURE_TYPE_2D);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkImageTypeToGfxTextureType(VK_IMAGE_TYPE_3D), GFX_TEXTURE_TYPE_3D);
}

TEST(VulkanConversionsTest, GfxTextureViewTypeToVkImageViewType_AllTypes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxTextureViewTypeToVkImageViewType(GFX_TEXTURE_VIEW_TYPE_1D), VK_IMAGE_VIEW_TYPE_1D);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxTextureViewTypeToVkImageViewType(GFX_TEXTURE_VIEW_TYPE_2D), VK_IMAGE_VIEW_TYPE_2D);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxTextureViewTypeToVkImageViewType(GFX_TEXTURE_VIEW_TYPE_3D), VK_IMAGE_VIEW_TYPE_3D);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxTextureViewTypeToVkImageViewType(GFX_TEXTURE_VIEW_TYPE_CUBE), VK_IMAGE_VIEW_TYPE_CUBE);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxTextureViewTypeToVkImageViewType(GFX_TEXTURE_VIEW_TYPE_1D_ARRAY), VK_IMAGE_VIEW_TYPE_1D_ARRAY);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxTextureViewTypeToVkImageViewType(GFX_TEXTURE_VIEW_TYPE_2D_ARRAY), VK_IMAGE_VIEW_TYPE_2D_ARRAY);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxTextureViewTypeToVkImageViewType(GFX_TEXTURE_VIEW_TYPE_CUBE_ARRAY), VK_IMAGE_VIEW_TYPE_CUBE_ARRAY);
}

// ============================================================================
// Sample Count Conversions
// ============================================================================

TEST(VulkanConversionsTest, SampleCountToVkSampleCount_AllCounts_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::sampleCountToVkSampleCount(GFX_SAMPLE_COUNT_1), VK_SAMPLE_COUNT_1_BIT);
    EXPECT_EQ(gfx::backend::vulkan::converter::sampleCountToVkSampleCount(GFX_SAMPLE_COUNT_2), VK_SAMPLE_COUNT_2_BIT);
    EXPECT_EQ(gfx::backend::vulkan::converter::sampleCountToVkSampleCount(GFX_SAMPLE_COUNT_4), VK_SAMPLE_COUNT_4_BIT);
    EXPECT_EQ(gfx::backend::vulkan::converter::sampleCountToVkSampleCount(GFX_SAMPLE_COUNT_8), VK_SAMPLE_COUNT_8_BIT);
    EXPECT_EQ(gfx::backend::vulkan::converter::sampleCountToVkSampleCount(GFX_SAMPLE_COUNT_16), VK_SAMPLE_COUNT_16_BIT);
    EXPECT_EQ(gfx::backend::vulkan::converter::sampleCountToVkSampleCount(GFX_SAMPLE_COUNT_32), VK_SAMPLE_COUNT_32_BIT);
    EXPECT_EQ(gfx::backend::vulkan::converter::sampleCountToVkSampleCount(GFX_SAMPLE_COUNT_64), VK_SAMPLE_COUNT_64_BIT);
}

TEST(VulkanConversionsTest, VkSampleCountToGfxSampleCount_AllCounts_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::vkSampleCountToGfxSampleCount(VK_SAMPLE_COUNT_1_BIT), GFX_SAMPLE_COUNT_1);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkSampleCountToGfxSampleCount(VK_SAMPLE_COUNT_2_BIT), GFX_SAMPLE_COUNT_2);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkSampleCountToGfxSampleCount(VK_SAMPLE_COUNT_4_BIT), GFX_SAMPLE_COUNT_4);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkSampleCountToGfxSampleCount(VK_SAMPLE_COUNT_8_BIT), GFX_SAMPLE_COUNT_8);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkSampleCountToGfxSampleCount(VK_SAMPLE_COUNT_16_BIT), GFX_SAMPLE_COUNT_16);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkSampleCountToGfxSampleCount(VK_SAMPLE_COUNT_32_BIT), GFX_SAMPLE_COUNT_32);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkSampleCountToGfxSampleCount(VK_SAMPLE_COUNT_64_BIT), GFX_SAMPLE_COUNT_64);
}

// ============================================================================
// Present Mode Conversions
// ============================================================================

TEST(VulkanConversionsTest, GfxPresentModeToVkPresentMode_AllModes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxPresentModeToVkPresentMode(GFX_PRESENT_MODE_IMMEDIATE), VK_PRESENT_MODE_IMMEDIATE_KHR);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxPresentModeToVkPresentMode(GFX_PRESENT_MODE_FIFO), VK_PRESENT_MODE_FIFO_KHR);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxPresentModeToVkPresentMode(GFX_PRESENT_MODE_FIFO_RELAXED), VK_PRESENT_MODE_FIFO_RELAXED_KHR);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxPresentModeToVkPresentMode(GFX_PRESENT_MODE_MAILBOX), VK_PRESENT_MODE_MAILBOX_KHR);
}

TEST(VulkanConversionsTest, VkPresentModeToGfxPresentMode_AllModes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::vkPresentModeToGfxPresentMode(VK_PRESENT_MODE_IMMEDIATE_KHR), GFX_PRESENT_MODE_IMMEDIATE);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkPresentModeToGfxPresentMode(VK_PRESENT_MODE_FIFO_KHR), GFX_PRESENT_MODE_FIFO);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkPresentModeToGfxPresentMode(VK_PRESENT_MODE_FIFO_RELAXED_KHR), GFX_PRESENT_MODE_FIFO_RELAXED);
    EXPECT_EQ(gfx::backend::vulkan::converter::vkPresentModeToGfxPresentMode(VK_PRESENT_MODE_MAILBOX_KHR), GFX_PRESENT_MODE_MAILBOX);
}

// ============================================================================
// Extent and Origin Conversions
// ============================================================================

TEST(VulkanConversionsTest, GfxExtent3DToVkExtent3D_AllFields_ConvertsCorrectly)
{
    GfxExtent3D gfxExtent = { 1920, 1080, 1 };

    VkExtent3D result = gfx::backend::vulkan::converter::gfxExtent3DToVkExtent3D(&gfxExtent);

    EXPECT_EQ(result.width, 1920u);
    EXPECT_EQ(result.height, 1080u);
    EXPECT_EQ(result.depth, 1u);
}

TEST(VulkanConversionsTest, VkExtent3DToGfxExtent3D_AllFields_ConvertsCorrectly)
{
    VkExtent3D vkExtent = { 2560, 1440, 16 };

    GfxExtent3D result = gfx::backend::vulkan::converter::vkExtent3DToGfxExtent3D(vkExtent);

    EXPECT_EQ(result.width, 2560u);
    EXPECT_EQ(result.height, 1440u);
    EXPECT_EQ(result.depth, 16u);
}

TEST(VulkanConversionsTest, GfxOrigin3DToVkOffset3D_AllFields_ConvertsCorrectly)
{
    GfxOrigin3D gfxOrigin = { 100, -50, 5 };

    VkOffset3D result = gfx::backend::vulkan::converter::gfxOrigin3DToVkOffset3D(&gfxOrigin);

    EXPECT_EQ(result.x, 100);
    EXPECT_EQ(result.y, -50);
    EXPECT_EQ(result.z, 5);
}

TEST(VulkanConversionsTest, ExtentConversion_RoundTrip_Preserves)
{
    VkExtent3D original = { 4096, 2160, 32 };

    GfxExtent3D gfxExtent = gfx::backend::vulkan::converter::vkExtent3DToGfxExtent3D(original);
    VkExtent3D result = gfx::backend::vulkan::converter::gfxExtent3DToVkExtent3D(&gfxExtent);

    EXPECT_EQ(result.width, original.width);
    EXPECT_EQ(result.height, original.height);
    EXPECT_EQ(result.depth, original.depth);
}

// ============================================================================
// Access Flags Reverse Conversion
// ============================================================================

TEST(VulkanConversionsTest, VkAccessFlagsToGfxAccessFlags_SingleFlags_ConvertsCorrectly)
{
    EXPECT_TRUE(gfx::backend::vulkan::converter::vkAccessFlagsToGfxAccessFlags(VK_ACCESS_INDIRECT_COMMAND_READ_BIT) & GFX_ACCESS_INDIRECT_COMMAND_READ);
    EXPECT_TRUE(gfx::backend::vulkan::converter::vkAccessFlagsToGfxAccessFlags(VK_ACCESS_INDEX_READ_BIT) & GFX_ACCESS_INDEX_READ);
    EXPECT_TRUE(gfx::backend::vulkan::converter::vkAccessFlagsToGfxAccessFlags(VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT) & GFX_ACCESS_VERTEX_ATTRIBUTE_READ);
    EXPECT_TRUE(gfx::backend::vulkan::converter::vkAccessFlagsToGfxAccessFlags(VK_ACCESS_SHADER_READ_BIT) & GFX_ACCESS_SHADER_READ);
    EXPECT_TRUE(gfx::backend::vulkan::converter::vkAccessFlagsToGfxAccessFlags(VK_ACCESS_SHADER_WRITE_BIT) & GFX_ACCESS_SHADER_WRITE);
}

TEST(VulkanConversionsTest, VkAccessFlagsToGfxAccessFlags_MultipleFlags_CombinesCorrectly)
{
    GfxAccessFlags result = gfx::backend::vulkan::converter::vkAccessFlagsToGfxAccessFlags(
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT);

    EXPECT_TRUE(result & GFX_ACCESS_SHADER_READ);
    EXPECT_TRUE(result & GFX_ACCESS_SHADER_WRITE);
    EXPECT_TRUE(result & GFX_ACCESS_TRANSFER_READ);
}

// ============================================================================
// Rendering Pipeline Conversions
// ============================================================================

TEST(VulkanConversionsTest, GfxPrimitiveTopologyToVkPrimitiveTopology_AllTopologies_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxPrimitiveTopologyToVkPrimitiveTopology(GFX_PRIMITIVE_TOPOLOGY_POINT_LIST), VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxPrimitiveTopologyToVkPrimitiveTopology(GFX_PRIMITIVE_TOPOLOGY_LINE_LIST), VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxPrimitiveTopologyToVkPrimitiveTopology(GFX_PRIMITIVE_TOPOLOGY_LINE_STRIP), VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxPrimitiveTopologyToVkPrimitiveTopology(GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxPrimitiveTopologyToVkPrimitiveTopology(GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
}

TEST(VulkanConversionsTest, GfxCullModeToVkCullMode_AllModes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxCullModeToVkCullMode(GFX_CULL_MODE_NONE), VK_CULL_MODE_NONE);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxCullModeToVkCullMode(GFX_CULL_MODE_FRONT), VK_CULL_MODE_FRONT_BIT);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxCullModeToVkCullMode(GFX_CULL_MODE_BACK), VK_CULL_MODE_BACK_BIT);
}

TEST(VulkanConversionsTest, GfxFrontFaceToVkFrontFace_BothDirections_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxFrontFaceToVkFrontFace(GFX_FRONT_FACE_COUNTER_CLOCKWISE), VK_FRONT_FACE_COUNTER_CLOCKWISE);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxFrontFaceToVkFrontFace(GFX_FRONT_FACE_CLOCKWISE), VK_FRONT_FACE_CLOCKWISE);
}

TEST(VulkanConversionsTest, GfxPolygonModeToVkPolygonMode_AllModes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxPolygonModeToVkPolygonMode(GFX_POLYGON_MODE_FILL), VK_POLYGON_MODE_FILL);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxPolygonModeToVkPolygonMode(GFX_POLYGON_MODE_LINE), VK_POLYGON_MODE_LINE);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxPolygonModeToVkPolygonMode(GFX_POLYGON_MODE_POINT), VK_POLYGON_MODE_POINT);
}

// ============================================================================
// Blend State Conversions
// ============================================================================

TEST(VulkanConversionsTest, GfxBlendFactorToVkBlendFactor_CommonFactors_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxBlendFactorToVkBlendFactor(GFX_BLEND_FACTOR_ZERO), VK_BLEND_FACTOR_ZERO);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxBlendFactorToVkBlendFactor(GFX_BLEND_FACTOR_ONE), VK_BLEND_FACTOR_ONE);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxBlendFactorToVkBlendFactor(GFX_BLEND_FACTOR_SRC), VK_BLEND_FACTOR_SRC_COLOR);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxBlendFactorToVkBlendFactor(GFX_BLEND_FACTOR_ONE_MINUS_SRC), VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxBlendFactorToVkBlendFactor(GFX_BLEND_FACTOR_DST), VK_BLEND_FACTOR_DST_COLOR);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxBlendFactorToVkBlendFactor(GFX_BLEND_FACTOR_ONE_MINUS_DST), VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxBlendFactorToVkBlendFactor(GFX_BLEND_FACTOR_SRC_ALPHA), VK_BLEND_FACTOR_SRC_ALPHA);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxBlendFactorToVkBlendFactor(GFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA), VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
}

TEST(VulkanConversionsTest, GfxBlendOpToVkBlendOp_AllOperations_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxBlendOpToVkBlendOp(GFX_BLEND_OPERATION_ADD), VK_BLEND_OP_ADD);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxBlendOpToVkBlendOp(GFX_BLEND_OPERATION_SUBTRACT), VK_BLEND_OP_SUBTRACT);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxBlendOpToVkBlendOp(GFX_BLEND_OPERATION_REVERSE_SUBTRACT), VK_BLEND_OP_REVERSE_SUBTRACT);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxBlendOpToVkBlendOp(GFX_BLEND_OPERATION_MIN), VK_BLEND_OP_MIN);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxBlendOpToVkBlendOp(GFX_BLEND_OPERATION_MAX), VK_BLEND_OP_MAX);
}

// ============================================================================
// Compare Operation Conversion
// ============================================================================

TEST(VulkanConversionsTest, GfxCompareOpToVkCompareOp_AllOperations_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxCompareOpToVkCompareOp(GFX_COMPARE_FUNCTION_NEVER), VK_COMPARE_OP_NEVER);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxCompareOpToVkCompareOp(GFX_COMPARE_FUNCTION_LESS), VK_COMPARE_OP_LESS);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxCompareOpToVkCompareOp(GFX_COMPARE_FUNCTION_EQUAL), VK_COMPARE_OP_EQUAL);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxCompareOpToVkCompareOp(GFX_COMPARE_FUNCTION_LESS_EQUAL), VK_COMPARE_OP_LESS_OR_EQUAL);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxCompareOpToVkCompareOp(GFX_COMPARE_FUNCTION_GREATER), VK_COMPARE_OP_GREATER);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxCompareOpToVkCompareOp(GFX_COMPARE_FUNCTION_NOT_EQUAL), VK_COMPARE_OP_NOT_EQUAL);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxCompareOpToVkCompareOp(GFX_COMPARE_FUNCTION_GREATER_EQUAL), VK_COMPARE_OP_GREATER_OR_EQUAL);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxCompareOpToVkCompareOp(GFX_COMPARE_FUNCTION_ALWAYS), VK_COMPARE_OP_ALWAYS);
}

// ============================================================================
// Query Type Conversion
// ============================================================================

TEST(VulkanConversionsTest, GfxQueryTypeToVkQueryType_AllTypes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxQueryTypeToVkQueryType(GFX_QUERY_TYPE_OCCLUSION), VK_QUERY_TYPE_OCCLUSION);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxQueryTypeToVkQueryType(GFX_QUERY_TYPE_TIMESTAMP), VK_QUERY_TYPE_TIMESTAMP);
}

// ============================================================================
// Sampler State Conversions
// ============================================================================

TEST(VulkanConversionsTest, GfxAddressModeToVkAddressMode_AllModes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxAddressModeToVkAddressMode(GFX_ADDRESS_MODE_REPEAT), VK_SAMPLER_ADDRESS_MODE_REPEAT);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxAddressModeToVkAddressMode(GFX_ADDRESS_MODE_MIRROR_REPEAT), VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxAddressModeToVkAddressMode(GFX_ADDRESS_MODE_CLAMP_TO_EDGE), VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

TEST(VulkanConversionsTest, GfxFilterToVkFilter_BothFilters_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxFilterToVkFilter(GFX_FILTER_MODE_NEAREST), VK_FILTER_NEAREST);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxFilterToVkFilter(GFX_FILTER_MODE_LINEAR), VK_FILTER_LINEAR);
}

TEST(VulkanConversionsTest, GfxFilterModeToVkMipMapFilterMode_BothModes_ConvertsCorrectly)
{
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxFilterModeToVkMipMapFilterMode(GFX_FILTER_MODE_NEAREST), VK_SAMPLER_MIPMAP_MODE_NEAREST);
    EXPECT_EQ(gfx::backend::vulkan::converter::gfxFilterModeToVkMipMapFilterMode(GFX_FILTER_MODE_LINEAR), VK_SAMPLER_MIPMAP_MODE_LINEAR);
}

// ============================================================================
// Format Utility Functions
// ============================================================================

TEST(VulkanConversionsTest, HasStencilComponent_StencilFormats_ReturnsTrue)
{
    EXPECT_TRUE(gfx::backend::vulkan::converter::hasStencilComponent(VK_FORMAT_D32_SFLOAT_S8_UINT));
    EXPECT_TRUE(gfx::backend::vulkan::converter::hasStencilComponent(VK_FORMAT_D24_UNORM_S8_UINT));
    EXPECT_TRUE(gfx::backend::vulkan::converter::hasStencilComponent(VK_FORMAT_S8_UINT));
}

TEST(VulkanConversionsTest, HasStencilComponent_NonStencilFormats_ReturnsFalse)
{
    EXPECT_FALSE(gfx::backend::vulkan::converter::hasStencilComponent(VK_FORMAT_D32_SFLOAT));
    EXPECT_FALSE(gfx::backend::vulkan::converter::hasStencilComponent(VK_FORMAT_R8G8B8A8_UNORM));
    EXPECT_FALSE(gfx::backend::vulkan::converter::hasStencilComponent(VK_FORMAT_D16_UNORM));
}

TEST(VulkanConversionsTest, GetImageAspectMask_ColorFormat_ReturnsColorBit)
{
    VkImageAspectFlags result = gfx::backend::vulkan::converter::getImageAspectMask(VK_FORMAT_R8G8B8A8_UNORM);
    EXPECT_EQ(result, VK_IMAGE_ASPECT_COLOR_BIT);
}

TEST(VulkanConversionsTest, GetImageAspectMask_DepthFormat_ReturnsDepthBit)
{
    VkImageAspectFlags result = gfx::backend::vulkan::converter::getImageAspectMask(VK_FORMAT_D32_SFLOAT);
    EXPECT_EQ(result, VK_IMAGE_ASPECT_DEPTH_BIT);
}

TEST(VulkanConversionsTest, GetImageAspectMask_DepthStencilFormat_ReturnsBothBits)
{
    VkImageAspectFlags result = gfx::backend::vulkan::converter::getImageAspectMask(VK_FORMAT_D24_UNORM_S8_UINT);
    EXPECT_TRUE(result & VK_IMAGE_ASPECT_DEPTH_BIT);
    EXPECT_TRUE(result & VK_IMAGE_ASPECT_STENCIL_BIT);
}

// ============================================================================
// Layout Access Flags Utility
// ============================================================================

TEST(VulkanConversionsTest, GetVkAccessFlagsForLayout_UndefinedLayout_ReturnsZero)
{
    VkAccessFlags result = gfx::backend::vulkan::converter::getVkAccessFlagsForLayout(VK_IMAGE_LAYOUT_UNDEFINED);
    EXPECT_EQ(result, 0u);
}

TEST(VulkanConversionsTest, GetVkAccessFlagsForLayout_GeneralLayout_ReturnsReadWrite)
{
    VkAccessFlags result = gfx::backend::vulkan::converter::getVkAccessFlagsForLayout(VK_IMAGE_LAYOUT_GENERAL);
    EXPECT_TRUE(result & VK_ACCESS_MEMORY_READ_BIT);
    EXPECT_TRUE(result & VK_ACCESS_MEMORY_WRITE_BIT);
}

TEST(VulkanConversionsTest, GetVkAccessFlagsForLayout_ColorAttachmentLayout_ReturnsColorAccess)
{
    VkAccessFlags result = gfx::backend::vulkan::converter::getVkAccessFlagsForLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    EXPECT_TRUE(result & VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);
    EXPECT_TRUE(result & VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
}

TEST(VulkanConversionsTest, GetVkAccessFlagsForLayout_DepthStencilAttachmentLayout_ReturnsDepthStencilAccess)
{
    VkAccessFlags result = gfx::backend::vulkan::converter::getVkAccessFlagsForLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    EXPECT_TRUE(result & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT);
    EXPECT_TRUE(result & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
}

TEST(VulkanConversionsTest, GetVkAccessFlagsForLayout_ShaderReadOnlyLayout_ReturnsShaderRead)
{
    VkAccessFlags result = gfx::backend::vulkan::converter::getVkAccessFlagsForLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    EXPECT_EQ(result, VK_ACCESS_SHADER_READ_BIT);
}

TEST(VulkanConversionsTest, GetVkAccessFlagsForLayout_TransferSrcLayout_ReturnsTransferRead)
{
    VkAccessFlags result = gfx::backend::vulkan::converter::getVkAccessFlagsForLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    EXPECT_EQ(result, VK_ACCESS_TRANSFER_READ_BIT);
}

TEST(VulkanConversionsTest, GetVkAccessFlagsForLayout_TransferDstLayout_ReturnsTransferWrite)
{
    VkAccessFlags result = gfx::backend::vulkan::converter::getVkAccessFlagsForLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    EXPECT_EQ(result, VK_ACCESS_TRANSFER_WRITE_BIT);
}

TEST(VulkanConversionsTest, GetVkAccessFlagsForLayout_PresentSrcLayout_ReturnsMemoryRead)
{
    VkAccessFlags result = gfx::backend::vulkan::converter::getVkAccessFlagsForLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    EXPECT_EQ(result, VK_ACCESS_MEMORY_READ_BIT);
}

// ============================================================================
// Info Struct Conversions
// ============================================================================

TEST(VulkanConversionsTest, VkTextureInfoToGfxTextureInfo_AllFields_ConvertsCorrectly)
{
    gfx::backend::vulkan::core::TextureInfo vkInfo{};
    vkInfo.imageType = VK_IMAGE_TYPE_2D;
    vkInfo.size = { 1920, 1080, 1 };
    vkInfo.arrayLayers = 6;
    vkInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    vkInfo.mipLevelCount = 5;
    vkInfo.sampleCount = VK_SAMPLE_COUNT_4_BIT;
    vkInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    GfxTextureInfo result = gfx::backend::vulkan::converter::vkTextureInfoToGfxTextureInfo(vkInfo);

    EXPECT_EQ(result.type, GFX_TEXTURE_TYPE_2D);
    EXPECT_EQ(result.size.width, 1920u);
    EXPECT_EQ(result.size.height, 1080u);
    EXPECT_EQ(result.size.depth, 1u);
    EXPECT_EQ(result.arrayLayerCount, 6u);
    EXPECT_EQ(result.format, GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM);
    EXPECT_EQ(result.mipLevelCount, 5u);
    EXPECT_EQ(result.sampleCount, GFX_SAMPLE_COUNT_4);
    EXPECT_TRUE(result.usage & GFX_TEXTURE_USAGE_TEXTURE_BINDING);
    EXPECT_TRUE(result.usage & GFX_TEXTURE_USAGE_RENDER_ATTACHMENT);
}

TEST(VulkanConversionsTest, VkTextureInfoToGfxTextureInfo_3DTexture_ConvertsCorrectly)
{
    gfx::backend::vulkan::core::TextureInfo vkInfo{};
    vkInfo.imageType = VK_IMAGE_TYPE_3D;
    vkInfo.size = { 512, 512, 256 };
    vkInfo.arrayLayers = 1;
    vkInfo.format = VK_FORMAT_R32_SFLOAT;
    vkInfo.mipLevelCount = 1;
    vkInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    vkInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;

    GfxTextureInfo result = gfx::backend::vulkan::converter::vkTextureInfoToGfxTextureInfo(vkInfo);

    EXPECT_EQ(result.type, GFX_TEXTURE_TYPE_3D);
    EXPECT_EQ(result.size.width, 512u);
    EXPECT_EQ(result.size.height, 512u);
    EXPECT_EQ(result.size.depth, 256u);
    EXPECT_EQ(result.format, GFX_TEXTURE_FORMAT_R32_FLOAT);
    EXPECT_TRUE(result.usage & GFX_TEXTURE_USAGE_STORAGE_BINDING);
}

TEST(VulkanConversionsTest, VkSwapchainInfoToGfxSwapchainInfo_AllFields_ConvertsCorrectly)
{
    gfx::backend::vulkan::core::SwapchainInfo vkInfo{};
    vkInfo.width = 2560;
    vkInfo.height = 1440;
    vkInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
    vkInfo.imageCount = 3;
    vkInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;

    GfxSwapchainInfo result = gfx::backend::vulkan::converter::vkSwapchainInfoToGfxSwapchainInfo(vkInfo);

    EXPECT_EQ(result.extent.width, 2560u);
    EXPECT_EQ(result.extent.height, 1440u);
    EXPECT_EQ(result.format, GFX_TEXTURE_FORMAT_B8G8R8A8_UNORM_SRGB);
    EXPECT_EQ(result.imageCount, 3u);
    EXPECT_EQ(result.presentMode, GFX_PRESENT_MODE_MAILBOX);
}

TEST(VulkanConversionsTest, VkSwapchainInfoToGfxSwapchainInfo_FifoMode_ConvertsCorrectly)
{
    gfx::backend::vulkan::core::SwapchainInfo vkInfo{};
    vkInfo.width = 1920;
    vkInfo.height = 1080;
    vkInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    vkInfo.imageCount = 2;
    vkInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;

    GfxSwapchainInfo result = gfx::backend::vulkan::converter::vkSwapchainInfoToGfxSwapchainInfo(vkInfo);

    EXPECT_EQ(result.extent.width, 1920u);
    EXPECT_EQ(result.extent.height, 1080u);
    EXPECT_EQ(result.presentMode, GFX_PRESENT_MODE_FIFO);
}

TEST(VulkanConversionsTest, VkBufferToGfxBufferInfo_AllFields_ConvertsCorrectly)
{
    gfx::backend::vulkan::core::BufferInfo vkInfo{};
    vkInfo.size = 1024 * 1024; // 1MB
    vkInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vkInfo.originalUsage = GFX_BUFFER_USAGE_VERTEX | GFX_BUFFER_USAGE_COPY_DST;
    vkInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    GfxBufferInfo result = gfx::backend::vulkan::converter::vkBufferToGfxBufferInfo(vkInfo);

    EXPECT_EQ(result.size, 1024u * 1024u);
    EXPECT_TRUE(result.usage & GFX_BUFFER_USAGE_VERTEX);
    EXPECT_TRUE(result.usage & GFX_BUFFER_USAGE_COPY_DST);
    EXPECT_TRUE(result.memoryProperties & GFX_MEMORY_PROPERTY_DEVICE_LOCAL);
}

TEST(VulkanConversionsTest, VkBufferToGfxBufferInfo_UniformBuffer_ConvertsCorrectly)
{
    gfx::backend::vulkan::core::BufferInfo vkInfo{};
    vkInfo.size = 256;
    vkInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vkInfo.originalUsage = GFX_BUFFER_USAGE_UNIFORM | GFX_BUFFER_USAGE_COPY_DST;
    vkInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    GfxBufferInfo result = gfx::backend::vulkan::converter::vkBufferToGfxBufferInfo(vkInfo);

    EXPECT_EQ(result.size, 256u);
    EXPECT_TRUE(result.usage & GFX_BUFFER_USAGE_UNIFORM);
    EXPECT_TRUE(result.usage & GFX_BUFFER_USAGE_COPY_DST);
    EXPECT_TRUE(result.memoryProperties & GFX_MEMORY_PROPERTY_HOST_VISIBLE);
    EXPECT_TRUE(result.memoryProperties & GFX_MEMORY_PROPERTY_HOST_COHERENT);
}

TEST(VulkanConversionsTest, VkBufferToGfxBufferInfo_StorageBuffer_ConvertsCorrectly)
{
    gfx::backend::vulkan::core::BufferInfo vkInfo{};
    vkInfo.size = 4096;
    vkInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    vkInfo.originalUsage = GFX_BUFFER_USAGE_STORAGE | GFX_BUFFER_USAGE_COPY_SRC;
    vkInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    GfxBufferInfo result = gfx::backend::vulkan::converter::vkBufferToGfxBufferInfo(vkInfo);

    EXPECT_EQ(result.size, 4096u);
    EXPECT_TRUE(result.usage & GFX_BUFFER_USAGE_STORAGE);
    EXPECT_TRUE(result.usage & GFX_BUFFER_USAGE_COPY_SRC);
    EXPECT_TRUE(result.memoryProperties & GFX_MEMORY_PROPERTY_DEVICE_LOCAL);
}

// ============================================================================
// Surface Info Conversion Tests
// ============================================================================

TEST(VulkanConversionsTest, VkSurfaceCapabilitiesToGfxSurfaceInfo_ConvertsCorrectly)
{
    VkSurfaceCapabilitiesKHR vkCaps{};
    vkCaps.minImageCount = 2;
    vkCaps.maxImageCount = 3;
    vkCaps.minImageExtent = { 1, 1 };
    vkCaps.maxImageExtent = { 4096, 4096 };

    GfxSurfaceInfo result = gfx::backend::vulkan::converter::vkSurfaceCapabilitiesToGfxSurfaceInfo(vkCaps);

    EXPECT_EQ(result.minImageCount, 2u);
    EXPECT_EQ(result.maxImageCount, 3u);
    EXPECT_EQ(result.minExtent.width, 1u);
    EXPECT_EQ(result.minExtent.height, 1u);
    EXPECT_EQ(result.maxExtent.width, 4096u);
    EXPECT_EQ(result.maxExtent.height, 4096u);
}

TEST(VulkanConversionsTest, VkSurfaceCapabilitiesToGfxSurfaceInfo_LargeValues_ConvertsCorrectly)
{
    VkSurfaceCapabilitiesKHR vkCaps{};
    vkCaps.minImageCount = 1;
    vkCaps.maxImageCount = 8;
    vkCaps.minImageExtent = { 16, 16 };
    vkCaps.maxImageExtent = { 8192, 8192 };

    GfxSurfaceInfo result = gfx::backend::vulkan::converter::vkSurfaceCapabilitiesToGfxSurfaceInfo(vkCaps);

    EXPECT_EQ(result.minImageCount, 1u);
    EXPECT_EQ(result.maxImageCount, 8u);
    EXPECT_EQ(result.minExtent.width, 16u);
    EXPECT_EQ(result.minExtent.height, 16u);
    EXPECT_EQ(result.maxExtent.width, 8192u);
    EXPECT_EQ(result.maxExtent.height, 8192u);
}

} // namespace
