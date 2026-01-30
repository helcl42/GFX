#include <backend/vulkan/core/system/Adapter.h>
#include <backend/vulkan/core/system/Instance.h>
#include <backend/vulkan/core/util/Utils.h>


#include <gtest/gtest.h>

// Test Vulkan core utility functions
// These tests verify the internal utility implementations

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class VulkanUtilsTest : public testing::Test {
protected:
    void SetUp() override
    {
        try {
            gfx::backend::vulkan::core::InstanceCreateInfo instInfo{};
            instance = std::make_unique<gfx::backend::vulkan::core::Instance>(instInfo);

            gfx::backend::vulkan::core::AdapterCreateInfo adapterInfo{};
            adapterInfo.adapterIndex = 0;
            adapter = instance->requestAdapter(adapterInfo);
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up Vulkan: " << e.what();
        }
    }

    std::unique_ptr<gfx::backend::vulkan::core::Instance> instance;
    gfx::backend::vulkan::core::Adapter* adapter = nullptr;
};

// ============================================================================
// Depth Format Tests
// ============================================================================

TEST_F(VulkanUtilsTest, IsDepthFormat_D32Sfloat_ReturnsTrue)
{
    bool result = gfx::backend::vulkan::core::isDepthFormat(VK_FORMAT_D32_SFLOAT);
    EXPECT_TRUE(result);
}

TEST_F(VulkanUtilsTest, IsDepthFormat_D24UnormS8Uint_ReturnsTrue)
{
    bool result = gfx::backend::vulkan::core::isDepthFormat(VK_FORMAT_D24_UNORM_S8_UINT);
    EXPECT_TRUE(result);
}

TEST_F(VulkanUtilsTest, IsDepthFormat_D32SfloatS8Uint_ReturnsTrue)
{
    bool result = gfx::backend::vulkan::core::isDepthFormat(VK_FORMAT_D32_SFLOAT_S8_UINT);
    EXPECT_TRUE(result);
}

TEST_F(VulkanUtilsTest, IsDepthFormat_D16Unorm_ReturnsTrue)
{
    bool result = gfx::backend::vulkan::core::isDepthFormat(VK_FORMAT_D16_UNORM);
    EXPECT_TRUE(result);
}

TEST_F(VulkanUtilsTest, IsDepthFormat_ColorFormat_ReturnsFalse)
{
    bool result = gfx::backend::vulkan::core::isDepthFormat(VK_FORMAT_R8G8B8A8_UNORM);
    EXPECT_FALSE(result);
}

TEST_F(VulkanUtilsTest, IsDepthFormat_UndefinedFormat_ReturnsFalse)
{
    bool result = gfx::backend::vulkan::core::isDepthFormat(VK_FORMAT_UNDEFINED);
    EXPECT_FALSE(result);
}

// ============================================================================
// Stencil Component Tests
// ============================================================================

TEST_F(VulkanUtilsTest, HasStencilComponent_D24UnormS8Uint_ReturnsTrue)
{
    bool result = gfx::backend::vulkan::core::hasStencilComponent(VK_FORMAT_D24_UNORM_S8_UINT);
    EXPECT_TRUE(result);
}

TEST_F(VulkanUtilsTest, HasStencilComponent_D32SfloatS8Uint_ReturnsTrue)
{
    bool result = gfx::backend::vulkan::core::hasStencilComponent(VK_FORMAT_D32_SFLOAT_S8_UINT);
    EXPECT_TRUE(result);
}

TEST_F(VulkanUtilsTest, HasStencilComponent_D32Sfloat_ReturnsFalse)
{
    bool result = gfx::backend::vulkan::core::hasStencilComponent(VK_FORMAT_D32_SFLOAT);
    EXPECT_FALSE(result);
}

TEST_F(VulkanUtilsTest, HasStencilComponent_D16Unorm_ReturnsFalse)
{
    bool result = gfx::backend::vulkan::core::hasStencilComponent(VK_FORMAT_D16_UNORM);
    EXPECT_FALSE(result);
}

TEST_F(VulkanUtilsTest, HasStencilComponent_ColorFormat_ReturnsFalse)
{
    bool result = gfx::backend::vulkan::core::hasStencilComponent(VK_FORMAT_R8G8B8A8_UNORM);
    EXPECT_FALSE(result);
}

// ============================================================================
// Image Aspect Mask Tests
// ============================================================================

TEST_F(VulkanUtilsTest, GetImageAspectMask_ColorFormat_ReturnsColorBit)
{
    VkImageAspectFlags result = gfx::backend::vulkan::core::getImageAspectMask(VK_FORMAT_R8G8B8A8_UNORM);
    EXPECT_EQ(result, VK_IMAGE_ASPECT_COLOR_BIT);
}

TEST_F(VulkanUtilsTest, GetImageAspectMask_D32Sfloat_ReturnsDepthBit)
{
    VkImageAspectFlags result = gfx::backend::vulkan::core::getImageAspectMask(VK_FORMAT_D32_SFLOAT);
    EXPECT_EQ(result, VK_IMAGE_ASPECT_DEPTH_BIT);
}

TEST_F(VulkanUtilsTest, GetImageAspectMask_D16Unorm_ReturnsDepthBit)
{
    VkImageAspectFlags result = gfx::backend::vulkan::core::getImageAspectMask(VK_FORMAT_D16_UNORM);
    EXPECT_EQ(result, VK_IMAGE_ASPECT_DEPTH_BIT);
}

TEST_F(VulkanUtilsTest, GetImageAspectMask_D24UnormS8Uint_ReturnsDepthAndStencilBits)
{
    VkImageAspectFlags result = gfx::backend::vulkan::core::getImageAspectMask(VK_FORMAT_D24_UNORM_S8_UINT);
    EXPECT_EQ(result, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
}

TEST_F(VulkanUtilsTest, GetImageAspectMask_D32SfloatS8Uint_ReturnsDepthAndStencilBits)
{
    VkImageAspectFlags result = gfx::backend::vulkan::core::getImageAspectMask(VK_FORMAT_D32_SFLOAT_S8_UINT);
    EXPECT_EQ(result, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
}

TEST_F(VulkanUtilsTest, GetImageAspectMask_MultipleColorFormats_AllReturnColorBit)
{
    VkFormat colorFormats[] = {
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R32G32B32A32_SFLOAT
    };

    for (auto format : colorFormats) {
        VkImageAspectFlags result = gfx::backend::vulkan::core::getImageAspectMask(format);
        EXPECT_EQ(result, VK_IMAGE_ASPECT_COLOR_BIT) << "Format: " << format;
    }
}

// ============================================================================
// Access Flags for Layout Tests
// ============================================================================

TEST_F(VulkanUtilsTest, GetVkAccessFlagsForLayout_Undefined_ReturnsZero)
{
    VkAccessFlags result = gfx::backend::vulkan::core::getVkAccessFlagsForLayout(VK_IMAGE_LAYOUT_UNDEFINED);
    EXPECT_EQ(result, 0u);
}

TEST_F(VulkanUtilsTest, GetVkAccessFlagsForLayout_General_ReturnsReadWrite)
{
    VkAccessFlags result = gfx::backend::vulkan::core::getVkAccessFlagsForLayout(VK_IMAGE_LAYOUT_GENERAL);
    EXPECT_EQ(result, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT);
}

TEST_F(VulkanUtilsTest, GetVkAccessFlagsForLayout_ColorAttachment_ReturnsColorReadWrite)
{
    VkAccessFlags result = gfx::backend::vulkan::core::getVkAccessFlagsForLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    EXPECT_EQ(result, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
}

TEST_F(VulkanUtilsTest, GetVkAccessFlagsForLayout_DepthStencilAttachment_ReturnsDepthStencilReadWrite)
{
    VkAccessFlags result = gfx::backend::vulkan::core::getVkAccessFlagsForLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    EXPECT_EQ(result, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
}

TEST_F(VulkanUtilsTest, GetVkAccessFlagsForLayout_DepthStencilReadOnly_ReturnsDepthStencilRead)
{
    VkAccessFlags result = gfx::backend::vulkan::core::getVkAccessFlagsForLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
    EXPECT_EQ(result, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT);
}

TEST_F(VulkanUtilsTest, GetVkAccessFlagsForLayout_ShaderReadOnly_ReturnsShaderRead)
{
    VkAccessFlags result = gfx::backend::vulkan::core::getVkAccessFlagsForLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    EXPECT_EQ(result, VK_ACCESS_SHADER_READ_BIT);
}

TEST_F(VulkanUtilsTest, GetVkAccessFlagsForLayout_TransferSrc_ReturnsTransferRead)
{
    VkAccessFlags result = gfx::backend::vulkan::core::getVkAccessFlagsForLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    EXPECT_EQ(result, VK_ACCESS_TRANSFER_READ_BIT);
}

TEST_F(VulkanUtilsTest, GetVkAccessFlagsForLayout_TransferDst_ReturnsTransferWrite)
{
    VkAccessFlags result = gfx::backend::vulkan::core::getVkAccessFlagsForLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    EXPECT_EQ(result, VK_ACCESS_TRANSFER_WRITE_BIT);
}

TEST_F(VulkanUtilsTest, GetVkAccessFlagsForLayout_PresentSrc_ReturnsZero)
{
    VkAccessFlags result = gfx::backend::vulkan::core::getVkAccessFlagsForLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    EXPECT_EQ(result, 0u);
}

TEST_F(VulkanUtilsTest, GetVkAccessFlagsForLayout_DepthReadOnlyStencilAttachment_ReturnsDepthStencilRead)
{
    VkAccessFlags result = gfx::backend::vulkan::core::getVkAccessFlagsForLayout(VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL);
    EXPECT_EQ(result, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT);
}

TEST_F(VulkanUtilsTest, GetVkAccessFlagsForLayout_DepthAttachmentStencilReadOnly_ReturnsDepthStencilRead)
{
    VkAccessFlags result = gfx::backend::vulkan::core::getVkAccessFlagsForLayout(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL);
    EXPECT_EQ(result, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT);
}

// ============================================================================
// Memory Type Finding Tests
// ============================================================================

TEST_F(VulkanUtilsTest, FindMemoryType_DeviceLocal_FindsValidType)
{
    auto memProperties = adapter->getMemoryProperties();
    uint32_t memoryTypeBits = 0xFFFFFFFF; // All memory types allowed

    uint32_t result = gfx::backend::vulkan::core::findMemoryType(
        memProperties,
        memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    EXPECT_NE(result, UINT32_MAX);
    EXPECT_LT(result, memProperties.memoryTypeCount);
    EXPECT_TRUE(memProperties.memoryTypes[result].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

TEST_F(VulkanUtilsTest, FindMemoryType_HostVisible_FindsValidType)
{
    auto memProperties = adapter->getMemoryProperties();
    uint32_t memoryTypeBits = 0xFFFFFFFF;

    uint32_t result = gfx::backend::vulkan::core::findMemoryType(
        memProperties,
        memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    EXPECT_NE(result, UINT32_MAX);
    EXPECT_LT(result, memProperties.memoryTypeCount);
    EXPECT_TRUE(memProperties.memoryTypes[result].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
}

TEST_F(VulkanUtilsTest, FindMemoryType_HostVisibleCoherent_FindsValidType)
{
    auto memProperties = adapter->getMemoryProperties();
    uint32_t memoryTypeBits = 0xFFFFFFFF;

    uint32_t result = gfx::backend::vulkan::core::findMemoryType(
        memProperties,
        memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    EXPECT_NE(result, UINT32_MAX);
    EXPECT_LT(result, memProperties.memoryTypeCount);
    VkMemoryPropertyFlags flags = memProperties.memoryTypes[result].propertyFlags;
    EXPECT_TRUE(flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    EXPECT_TRUE(flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

TEST_F(VulkanUtilsTest, FindMemoryType_NoMatchingType_ReturnsMax)
{
    auto memProperties = adapter->getMemoryProperties();
    uint32_t memoryTypeBits = 0; // No memory types allowed

    uint32_t result = gfx::backend::vulkan::core::findMemoryType(
        memProperties,
        memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    EXPECT_EQ(result, UINT32_MAX);
}

TEST_F(VulkanUtilsTest, FindMemoryType_RestrictedBits_RespectsRestriction)
{
    auto memProperties = adapter->getMemoryProperties();

    // Find a valid device local type first
    uint32_t allBits = 0xFFFFFFFF;
    uint32_t validType = gfx::backend::vulkan::core::findMemoryType(
        memProperties,
        allBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    ASSERT_NE(validType, UINT32_MAX);

    // Now search with only that bit set
    uint32_t restrictedBits = (1 << validType);
    uint32_t result = gfx::backend::vulkan::core::findMemoryType(
        memProperties,
        restrictedBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    EXPECT_EQ(result, validType);
}

// ============================================================================
// VkResult to String Tests
// ============================================================================

TEST_F(VulkanUtilsTest, VkResultToString_Success_ReturnsSuccessString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_SUCCESS);
    EXPECT_STREQ(result, "VK_SUCCESS");
}

TEST_F(VulkanUtilsTest, VkResultToString_NotReady_ReturnsNotReadyString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_NOT_READY);
    EXPECT_STREQ(result, "VK_NOT_READY");
}

TEST_F(VulkanUtilsTest, VkResultToString_Timeout_ReturnsTimeoutString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_TIMEOUT);
    EXPECT_STREQ(result, "VK_TIMEOUT");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorOutOfHostMemory_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_OUT_OF_HOST_MEMORY);
    EXPECT_STREQ(result, "VK_ERROR_OUT_OF_HOST_MEMORY");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorOutOfDeviceMemory_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_OUT_OF_DEVICE_MEMORY);
    EXPECT_STREQ(result, "VK_ERROR_OUT_OF_DEVICE_MEMORY");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorInitializationFailed_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_INITIALIZATION_FAILED);
    EXPECT_STREQ(result, "VK_ERROR_INITIALIZATION_FAILED");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorDeviceLost_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_DEVICE_LOST);
    EXPECT_STREQ(result, "VK_ERROR_DEVICE_LOST");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorMemoryMapFailed_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_MEMORY_MAP_FAILED);
    EXPECT_STREQ(result, "VK_ERROR_MEMORY_MAP_FAILED");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorLayerNotPresent_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_LAYER_NOT_PRESENT);
    EXPECT_STREQ(result, "VK_ERROR_LAYER_NOT_PRESENT");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorExtensionNotPresent_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_EXTENSION_NOT_PRESENT);
    EXPECT_STREQ(result, "VK_ERROR_EXTENSION_NOT_PRESENT");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorFeatureNotPresent_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_FEATURE_NOT_PRESENT);
    EXPECT_STREQ(result, "VK_ERROR_FEATURE_NOT_PRESENT");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorIncompatibleDriver_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_INCOMPATIBLE_DRIVER);
    EXPECT_STREQ(result, "VK_ERROR_INCOMPATIBLE_DRIVER");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorTooManyObjects_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_TOO_MANY_OBJECTS);
    EXPECT_STREQ(result, "VK_ERROR_TOO_MANY_OBJECTS");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorFormatNotSupported_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_FORMAT_NOT_SUPPORTED);
    EXPECT_STREQ(result, "VK_ERROR_FORMAT_NOT_SUPPORTED");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorFragmentedPool_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_FRAGMENTED_POOL);
    EXPECT_STREQ(result, "VK_ERROR_FRAGMENTED_POOL");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorSurfaceLost_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_SURFACE_LOST_KHR);
    EXPECT_STREQ(result, "VK_ERROR_SURFACE_LOST_KHR");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorNativeWindowInUse_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
    EXPECT_STREQ(result, "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR");
}

TEST_F(VulkanUtilsTest, VkResultToString_SuboptimalKhr_ReturnsSuboptimalString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_SUBOPTIMAL_KHR);
    EXPECT_STREQ(result, "VK_SUBOPTIMAL_KHR");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorOutOfDate_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_OUT_OF_DATE_KHR);
    EXPECT_STREQ(result, "VK_ERROR_OUT_OF_DATE_KHR");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorIncompatibleDisplay_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
    EXPECT_STREQ(result, "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorValidationFailed_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_VALIDATION_FAILED_EXT);
    EXPECT_STREQ(result, "VK_ERROR_VALIDATION_FAILED_EXT");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorInvalidShader_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_INVALID_SHADER_NV);
    EXPECT_STREQ(result, "VK_ERROR_INVALID_SHADER_NV");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorOutOfPoolMemory_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_OUT_OF_POOL_MEMORY);
    EXPECT_STREQ(result, "VK_ERROR_OUT_OF_POOL_MEMORY");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorInvalidExternalHandle_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_INVALID_EXTERNAL_HANDLE);
    EXPECT_STREQ(result, "VK_ERROR_INVALID_EXTERNAL_HANDLE");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorFragmentation_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_FRAGMENTATION);
    EXPECT_STREQ(result, "VK_ERROR_FRAGMENTATION");
}

TEST_F(VulkanUtilsTest, VkResultToString_ErrorInvalidOpaqueCaptureAddress_ReturnsErrorString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
    EXPECT_STREQ(result, "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS");
}

TEST_F(VulkanUtilsTest, VkResultToString_EventSet_ReturnsEventSetString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_EVENT_SET);
    EXPECT_STREQ(result, "VK_EVENT_SET");
}

TEST_F(VulkanUtilsTest, VkResultToString_EventReset_ReturnsEventResetString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_EVENT_RESET);
    EXPECT_STREQ(result, "VK_EVENT_RESET");
}

TEST_F(VulkanUtilsTest, VkResultToString_Incomplete_ReturnsIncompleteString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(VK_INCOMPLETE);
    EXPECT_STREQ(result, "VK_INCOMPLETE");
}

TEST_F(VulkanUtilsTest, VkResultToString_UnknownValue_ReturnsUnknownString)
{
    const char* result = gfx::backend::vulkan::core::vkResultToString(static_cast<VkResult>(-999999));
    EXPECT_STREQ(result, "VK_UNKNOWN_ERROR");
}

// ============================================================================
// Combined Use Case Tests
// ============================================================================

TEST_F(VulkanUtilsTest, DepthStencilFormatWorkflow_WorksCorrectly)
{
    VkFormat format = VK_FORMAT_D24_UNORM_S8_UINT;

    // Check format properties
    EXPECT_TRUE(gfx::backend::vulkan::core::isDepthFormat(format));
    EXPECT_TRUE(gfx::backend::vulkan::core::hasStencilComponent(format));

    // Get aspect mask
    VkImageAspectFlags aspectMask = gfx::backend::vulkan::core::getImageAspectMask(format);
    EXPECT_EQ(aspectMask, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);

    // Get access flags for attachment layout
    VkAccessFlags accessFlags = gfx::backend::vulkan::core::getVkAccessFlagsForLayout(
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    EXPECT_EQ(accessFlags, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
}

TEST_F(VulkanUtilsTest, ColorFormatWorkflow_WorksCorrectly)
{
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    // Check format properties
    EXPECT_FALSE(gfx::backend::vulkan::core::isDepthFormat(format));
    EXPECT_FALSE(gfx::backend::vulkan::core::hasStencilComponent(format));

    // Get aspect mask
    VkImageAspectFlags aspectMask = gfx::backend::vulkan::core::getImageAspectMask(format);
    EXPECT_EQ(aspectMask, VK_IMAGE_ASPECT_COLOR_BIT);

    // Get access flags for attachment layout
    VkAccessFlags accessFlags = gfx::backend::vulkan::core::getVkAccessFlagsForLayout(
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    EXPECT_EQ(accessFlags, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
}

TEST_F(VulkanUtilsTest, MemoryAllocationWorkflow_WorksCorrectly)
{
    auto memProperties = adapter->getMemoryProperties();

    // Find device local memory for textures/buffers
    uint32_t deviceLocalType = gfx::backend::vulkan::core::findMemoryType(
        memProperties,
        0xFFFFFFFF,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    EXPECT_NE(deviceLocalType, UINT32_MAX);

    // Find host visible memory for staging
    uint32_t hostVisibleType = gfx::backend::vulkan::core::findMemoryType(
        memProperties,
        0xFFFFFFFF,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    EXPECT_NE(hostVisibleType, UINT32_MAX);
}

} // namespace
