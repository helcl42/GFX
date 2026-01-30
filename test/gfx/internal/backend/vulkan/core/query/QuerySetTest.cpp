#include <backend/vulkan/core/query/QuerySet.h>
#include <backend/vulkan/core/system/Adapter.h>
#include <backend/vulkan/core/system/Device.h>
#include <backend/vulkan/core/system/Instance.h>

#include <gtest/gtest.h>

// Test Vulkan core QuerySet class
// These tests verify the internal query set implementation

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class VulkanQuerySetTest : public testing::Test {
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

TEST_F(VulkanQuerySetTest, CreateOcclusionQuerySet_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::QuerySetCreateInfo createInfo{};
    createInfo.type = VK_QUERY_TYPE_OCCLUSION;
    createInfo.count = 16;

    gfx::backend::vulkan::core::QuerySet querySet(device.get(), createInfo);

    EXPECT_NE(querySet.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(querySet.getType(), VK_QUERY_TYPE_OCCLUSION);
    EXPECT_EQ(querySet.getCount(), 16u);
}

TEST_F(VulkanQuerySetTest, CreateTimestampQuerySet_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::QuerySetCreateInfo createInfo{};
    createInfo.type = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.count = 8;

    gfx::backend::vulkan::core::QuerySet querySet(device.get(), createInfo);

    EXPECT_NE(querySet.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(querySet.getType(), VK_QUERY_TYPE_TIMESTAMP);
    EXPECT_EQ(querySet.getCount(), 8u);
}

TEST_F(VulkanQuerySetTest, CreatePipelineStatisticsQuerySet_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::QuerySetCreateInfo createInfo{};
    createInfo.type = VK_QUERY_TYPE_PIPELINE_STATISTICS;
    createInfo.count = 4;

    gfx::backend::vulkan::core::QuerySet querySet(device.get(), createInfo);

    EXPECT_NE(querySet.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(querySet.getType(), VK_QUERY_TYPE_PIPELINE_STATISTICS);
    EXPECT_EQ(querySet.getCount(), 4u);
}

// ============================================================================
// Query Count Tests
// ============================================================================

TEST_F(VulkanQuerySetTest, CreateWithSingleQuery_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::QuerySetCreateInfo createInfo{};
    createInfo.type = VK_QUERY_TYPE_OCCLUSION;
    createInfo.count = 1;

    gfx::backend::vulkan::core::QuerySet querySet(device.get(), createInfo);

    EXPECT_NE(querySet.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(querySet.getCount(), 1u);
}

TEST_F(VulkanQuerySetTest, CreateWithMultipleQueries_CreatesSuccessfully)
{
    uint32_t counts[] = { 2, 4, 8, 16, 32, 64, 128 };

    for (auto count : counts) {
        gfx::backend::vulkan::core::QuerySetCreateInfo createInfo{};
        createInfo.type = VK_QUERY_TYPE_OCCLUSION;
        createInfo.count = count;

        gfx::backend::vulkan::core::QuerySet querySet(device.get(), createInfo);

        EXPECT_NE(querySet.handle(), VK_NULL_HANDLE);
        EXPECT_EQ(querySet.getCount(), count);
    }
}

TEST_F(VulkanQuerySetTest, CreateWithLargeQueryCount_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::QuerySetCreateInfo createInfo{};
    createInfo.type = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.count = 1024;

    gfx::backend::vulkan::core::QuerySet querySet(device.get(), createInfo);

    EXPECT_NE(querySet.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(querySet.getCount(), 1024u);
}

// ============================================================================
// Handle Tests
// ============================================================================

TEST_F(VulkanQuerySetTest, GetHandle_ReturnsValidHandle)
{
    gfx::backend::vulkan::core::QuerySetCreateInfo createInfo{};
    createInfo.type = VK_QUERY_TYPE_OCCLUSION;
    createInfo.count = 8;

    gfx::backend::vulkan::core::QuerySet querySet(device.get(), createInfo);

    VkQueryPool handle = querySet.handle();
    EXPECT_NE(handle, VK_NULL_HANDLE);
    EXPECT_EQ(querySet.handle(), handle);
}

TEST_F(VulkanQuerySetTest, MultipleQuerySets_HaveUniqueHandles)
{
    gfx::backend::vulkan::core::QuerySetCreateInfo createInfo{};
    createInfo.type = VK_QUERY_TYPE_OCCLUSION;
    createInfo.count = 8;

    gfx::backend::vulkan::core::QuerySet querySet1(device.get(), createInfo);
    gfx::backend::vulkan::core::QuerySet querySet2(device.get(), createInfo);

    EXPECT_NE(querySet1.handle(), querySet2.handle());
}

// ============================================================================
// Device Accessor Tests
// ============================================================================

TEST_F(VulkanQuerySetTest, GetDevice_ReturnsCorrectDevice)
{
    gfx::backend::vulkan::core::QuerySetCreateInfo createInfo{};
    createInfo.type = VK_QUERY_TYPE_OCCLUSION;
    createInfo.count = 8;

    gfx::backend::vulkan::core::QuerySet querySet(device.get(), createInfo);

    EXPECT_EQ(querySet.getDevice(), device.get());
}

// ============================================================================
// Type Accessor Tests
// ============================================================================

TEST_F(VulkanQuerySetTest, GetType_ReturnsCorrectType)
{
    VkQueryType types[] = {
        VK_QUERY_TYPE_OCCLUSION,
        VK_QUERY_TYPE_TIMESTAMP,
        VK_QUERY_TYPE_PIPELINE_STATISTICS
    };

    for (auto type : types) {
        gfx::backend::vulkan::core::QuerySetCreateInfo createInfo{};
        createInfo.type = type;
        createInfo.count = 8;

        gfx::backend::vulkan::core::QuerySet querySet(device.get(), createInfo);

        EXPECT_EQ(querySet.getType(), type);
    }
}

// ============================================================================
// Count Accessor Tests
// ============================================================================

TEST_F(VulkanQuerySetTest, GetCount_ReturnsCorrectCount)
{
    uint32_t counts[] = { 1, 4, 16, 64, 256 };

    for (auto count : counts) {
        gfx::backend::vulkan::core::QuerySetCreateInfo createInfo{};
        createInfo.type = VK_QUERY_TYPE_OCCLUSION;
        createInfo.count = count;

        gfx::backend::vulkan::core::QuerySet querySet(device.get(), createInfo);

        EXPECT_EQ(querySet.getCount(), count);
    }
}

// ============================================================================
// Lifecycle Tests
// ============================================================================

TEST_F(VulkanQuerySetTest, CreateAndDestroy_WorksCorrectly)
{
    {
        gfx::backend::vulkan::core::QuerySetCreateInfo createInfo{};
        createInfo.type = VK_QUERY_TYPE_OCCLUSION;
        createInfo.count = 8;

        gfx::backend::vulkan::core::QuerySet querySet(device.get(), createInfo);

        EXPECT_NE(querySet.handle(), VK_NULL_HANDLE);
    }
    // QuerySet destroyed, no crash
}

TEST_F(VulkanQuerySetTest, MultipleLifecycles_WorkCorrectly)
{
    for (int i = 0; i < 10; ++i) {
        gfx::backend::vulkan::core::QuerySetCreateInfo createInfo{};
        createInfo.type = VK_QUERY_TYPE_TIMESTAMP;
        createInfo.count = 16;

        gfx::backend::vulkan::core::QuerySet querySet(device.get(), createInfo);

        EXPECT_NE(querySet.handle(), VK_NULL_HANDLE);
    }
}

// ============================================================================
// Label Tests
// ============================================================================

TEST_F(VulkanQuerySetTest, CreateWithLabel_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::QuerySetCreateInfo createInfo{};
    createInfo.label = "Test Query Set";
    createInfo.type = VK_QUERY_TYPE_OCCLUSION;
    createInfo.count = 8;

    gfx::backend::vulkan::core::QuerySet querySet(device.get(), createInfo);

    EXPECT_NE(querySet.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanQuerySetTest, CreateWithoutLabel_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::QuerySetCreateInfo createInfo{};
    createInfo.label = nullptr;
    createInfo.type = VK_QUERY_TYPE_OCCLUSION;
    createInfo.count = 8;

    gfx::backend::vulkan::core::QuerySet querySet(device.get(), createInfo);

    EXPECT_NE(querySet.handle(), VK_NULL_HANDLE);
}

} // namespace
