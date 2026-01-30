#include <backend/vulkan/core/resource/Buffer.h>
#include <backend/vulkan/core/system/Adapter.h>
#include <backend/vulkan/core/system/Device.h>
#include <backend/vulkan/core/system/Instance.h>


#include <gtest/gtest.h>

#include <cstring>

// Test Vulkan core Buffer class
// These tests verify the internal buffer implementation, not the public API

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class VulkanBufferTest : public testing::Test {
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
// Buffer Creation Tests
// ============================================================================

TEST_F(VulkanBufferTest, CreateBuffer_VertexUsage_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BufferCreateInfo createInfo{};
    createInfo.size = 1024;
    createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    createInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    gfx::backend::vulkan::core::Buffer buffer(device.get(), createInfo);

    EXPECT_NE(buffer.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(buffer.size(), 1024u);
    EXPECT_TRUE(buffer.getUsage() & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

TEST_F(VulkanBufferTest, CreateBuffer_UniformUsage_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BufferCreateInfo createInfo{};
    createInfo.size = 256;
    createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    createInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    gfx::backend::vulkan::core::Buffer buffer(device.get(), createInfo);

    EXPECT_NE(buffer.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(buffer.size(), 256u);
    EXPECT_TRUE(buffer.getUsage() & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
}

TEST_F(VulkanBufferTest, CreateBuffer_StorageUsage_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BufferCreateInfo createInfo{};
    createInfo.size = 4096;
    createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    createInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    gfx::backend::vulkan::core::Buffer buffer(device.get(), createInfo);

    EXPECT_NE(buffer.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(buffer.size(), 4096u);
    EXPECT_TRUE(buffer.getUsage() & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

TEST_F(VulkanBufferTest, CreateBuffer_MultipleUsageFlags_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BufferCreateInfo createInfo{};
    createInfo.size = 2048;
    createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    createInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    gfx::backend::vulkan::core::Buffer buffer(device.get(), createInfo);

    EXPECT_NE(buffer.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(buffer.size(), 2048u);
    EXPECT_TRUE(buffer.getUsage() & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    EXPECT_TRUE(buffer.getUsage() & VK_BUFFER_USAGE_TRANSFER_DST_BIT);
}

TEST_F(VulkanBufferTest, CreateBuffer_LargeSize_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BufferCreateInfo createInfo{};
    createInfo.size = 1024 * 1024 * 16; // 16MB
    createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    createInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    gfx::backend::vulkan::core::Buffer buffer(device.get(), createInfo);

    EXPECT_NE(buffer.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(buffer.size(), 1024u * 1024u * 16u);
}

// ============================================================================
// Buffer Info Tests
// ============================================================================

TEST_F(VulkanBufferTest, GetInfo_AfterCreation_ReturnsCorrectInfo)
{
    gfx::backend::vulkan::core::BufferCreateInfo createInfo{};
    createInfo.size = 512;
    createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    createInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    gfx::backend::vulkan::core::Buffer buffer(device.get(), createInfo);

    const auto& info = buffer.getInfo();
    EXPECT_EQ(info.size, 512u);
    EXPECT_TRUE(info.usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    EXPECT_TRUE(info.usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    EXPECT_TRUE(info.memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    EXPECT_TRUE(info.memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

// ============================================================================
// Memory Property Tests
// ============================================================================

TEST_F(VulkanBufferTest, CreateBuffer_DeviceLocal_AllocatesCorrectly)
{
    gfx::backend::vulkan::core::BufferCreateInfo createInfo{};
    createInfo.size = 1024;
    createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    createInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    gfx::backend::vulkan::core::Buffer buffer(device.get(), createInfo);

    const auto& info = buffer.getInfo();
    EXPECT_TRUE(info.memoryProperties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

TEST_F(VulkanBufferTest, CreateBuffer_HostVisible_AllocatesCorrectly)
{
    gfx::backend::vulkan::core::BufferCreateInfo createInfo{};
    createInfo.size = 512;
    createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    createInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    gfx::backend::vulkan::core::Buffer buffer(device.get(), createInfo);

    const auto& info = buffer.getInfo();
    EXPECT_TRUE(info.memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    EXPECT_TRUE(info.memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

TEST_F(VulkanBufferTest, CreateBuffer_HostCached_AllocatesCorrectly)
{
    gfx::backend::vulkan::core::BufferCreateInfo createInfo{};
    createInfo.size = 256;
    createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    createInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

    gfx::backend::vulkan::core::Buffer buffer(device.get(), createInfo);

    const auto& info = buffer.getInfo();
    EXPECT_TRUE(info.memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    // Note: Not all hardware supports HOST_CACHED, so we don't strictly check for it
}

// ============================================================================
// Buffer Mapping Tests
// ============================================================================

TEST_F(VulkanBufferTest, MapBuffer_HostVisible_MapsSuccessfully)
{
    gfx::backend::vulkan::core::BufferCreateInfo createInfo{};
    createInfo.size = 256;
    createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    createInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    gfx::backend::vulkan::core::Buffer buffer(device.get(), createInfo);

    void* mappedPtr = buffer.map();
    EXPECT_NE(mappedPtr, nullptr);

    buffer.unmap();
}

TEST_F(VulkanBufferTest, MapBuffer_WriteData_SuccessfullyWrites)
{
    gfx::backend::vulkan::core::BufferCreateInfo createInfo{};
    createInfo.size = 64;
    createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    createInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    gfx::backend::vulkan::core::Buffer buffer(device.get(), createInfo);

    void* mappedPtr = buffer.map();
    ASSERT_NE(mappedPtr, nullptr);

    // Write test data
    uint32_t testData[] = { 1, 2, 3, 4 };
    std::memcpy(mappedPtr, testData, sizeof(testData));

    // Read back
    uint32_t readData[4];
    std::memcpy(readData, mappedPtr, sizeof(readData));

    EXPECT_EQ(readData[0], 1u);
    EXPECT_EQ(readData[1], 2u);
    EXPECT_EQ(readData[2], 3u);
    EXPECT_EQ(readData[3], 4u);

    buffer.unmap();
}

TEST_F(VulkanBufferTest, MapUnmapBuffer_MultipleTimes_WorksCorrectly)
{
    gfx::backend::vulkan::core::BufferCreateInfo createInfo{};
    createInfo.size = 128;
    createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    createInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    gfx::backend::vulkan::core::Buffer buffer(device.get(), createInfo);

    // First map/unmap
    void* mappedPtr1 = buffer.map();
    EXPECT_NE(mappedPtr1, nullptr);
    buffer.unmap();

    // Second map/unmap
    void* mappedPtr2 = buffer.map();
    EXPECT_NE(mappedPtr2, nullptr);
    buffer.unmap();

    // Should get the same pointer
    EXPECT_EQ(mappedPtr1, mappedPtr2);
}

// ============================================================================
// Buffer Flush/Invalidate Tests (Non-Coherent Memory)
// ============================================================================

TEST_F(VulkanBufferTest, FlushMappedRange_NonCoherentMemory_NoThrow)
{
    gfx::backend::vulkan::core::BufferCreateInfo createInfo{};
    createInfo.size = 1024;
    createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    createInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT; // Non-coherent

    gfx::backend::vulkan::core::Buffer buffer(device.get(), createInfo);

    void* mappedPtr = buffer.map();
    ASSERT_NE(mappedPtr, nullptr);

    // Write some data
    std::memset(mappedPtr, 0x42, 512);

    // Flush should not throw
    EXPECT_NO_THROW(buffer.flushMappedRange(0, 512));

    buffer.unmap();
}

TEST_F(VulkanBufferTest, FlushMappedRange_CoherentMemory_IsNoOp)
{
    gfx::backend::vulkan::core::BufferCreateInfo createInfo{};
    createInfo.size = 1024;
    createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    createInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    gfx::backend::vulkan::core::Buffer buffer(device.get(), createInfo);

    void* mappedPtr = buffer.map();
    ASSERT_NE(mappedPtr, nullptr);

    // Flush on coherent memory should be a no-op (returns immediately)
    EXPECT_NO_THROW(buffer.flushMappedRange(0, 1024));

    buffer.unmap();
}

TEST_F(VulkanBufferTest, InvalidateMappedRange_NonCoherentMemory_NoThrow)
{
    gfx::backend::vulkan::core::BufferCreateInfo createInfo{};
    createInfo.size = 2048;
    createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    createInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT; // Non-coherent

    gfx::backend::vulkan::core::Buffer buffer(device.get(), createInfo);

    // Invalidate should not throw
    EXPECT_NO_THROW(buffer.invalidateMappedRange(0, 2048));

    void* mappedPtr = buffer.map();
    EXPECT_NE(mappedPtr, nullptr);

    buffer.unmap();
}

TEST_F(VulkanBufferTest, FlushInvalidate_PartialRange_NoThrow)
{
    gfx::backend::vulkan::core::BufferCreateInfo createInfo{};
    createInfo.size = 4096;
    createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    createInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT; // Non-coherent

    gfx::backend::vulkan::core::Buffer buffer(device.get(), createInfo);

    void* mappedPtr = buffer.map();
    ASSERT_NE(mappedPtr, nullptr);

    // Flush/invalidate partial ranges
    EXPECT_NO_THROW(buffer.flushMappedRange(0, 1024));
    EXPECT_NO_THROW(buffer.flushMappedRange(1024, 1024));
    EXPECT_NO_THROW(buffer.invalidateMappedRange(2048, 2048));

    buffer.unmap();
}

// ============================================================================
// Buffer Import Tests
// ============================================================================

TEST_F(VulkanBufferTest, ImportBuffer_ValidHandle_CreatesSuccessfully)
{
    // First create a regular buffer
    gfx::backend::vulkan::core::BufferCreateInfo createInfo{};
    createInfo.size = 512;
    createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    createInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    gfx::backend::vulkan::core::Buffer sourceBuffer(device.get(), createInfo);
    VkBuffer handle = sourceBuffer.handle();
    ASSERT_NE(handle, VK_NULL_HANDLE);

    // Import the handle
    gfx::backend::vulkan::core::BufferImportInfo importInfo{};
    importInfo.size = 512;
    importInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    importInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    gfx::backend::vulkan::core::Buffer importedBuffer(device.get(), handle, importInfo);

    EXPECT_EQ(importedBuffer.handle(), handle);
    EXPECT_EQ(importedBuffer.size(), 512u);
}

// ============================================================================
// Buffer Usage Combination Tests
// ============================================================================

TEST_F(VulkanBufferTest, CreateBuffer_VertexIndexUsage_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BufferCreateInfo createInfo{};
    createInfo.size = 2048;
    createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    createInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    gfx::backend::vulkan::core::Buffer buffer(device.get(), createInfo);

    EXPECT_TRUE(buffer.getUsage() & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    EXPECT_TRUE(buffer.getUsage() & VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

TEST_F(VulkanBufferTest, CreateBuffer_TransferUsage_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BufferCreateInfo createInfo{};
    createInfo.size = 1024;
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    createInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    gfx::backend::vulkan::core::Buffer buffer(device.get(), createInfo);

    EXPECT_TRUE(buffer.getUsage() & VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    EXPECT_TRUE(buffer.getUsage() & VK_BUFFER_USAGE_TRANSFER_DST_BIT);
}

TEST_F(VulkanBufferTest, CreateBuffer_IndirectUsage_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BufferCreateInfo createInfo{};
    createInfo.size = 256;
    createInfo.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    createInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    gfx::backend::vulkan::core::Buffer buffer(device.get(), createInfo);

    EXPECT_TRUE(buffer.getUsage() & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
}

} // namespace
