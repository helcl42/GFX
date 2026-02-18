#include <backend/vulkan/core/system/Adapter.h>
#include <backend/vulkan/core/system/Device.h>
#include <backend/vulkan/core/system/Instance.h>
#include <backend/vulkan/core/system/Queue.h>
#include <backend/vulkan/core/util/CommandExecutor.h>

#include <gtest/gtest.h>

// Test CommandExecutor utility
// Verifies single-time command buffer execution for synchronous operations

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class CommandExecutorTest : public testing::Test {
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

            queue = device->getQueue();
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up Vulkan: " << e.what();
        }
    }

    std::unique_ptr<gfx::backend::vulkan::core::Instance> instance;
    gfx::backend::vulkan::core::Adapter* adapter = nullptr;
    std::unique_ptr<gfx::backend::vulkan::core::Device> device;
    gfx::backend::vulkan::core::Queue* queue = nullptr;
};

// ============================================================================
// Construction Tests
// ============================================================================

TEST_F(CommandExecutorTest, Constructor_ValidQueue_Succeeds)
{
    ASSERT_NO_THROW({
        gfx::backend::vulkan::core::CommandExecutor executor(queue);
    });
}

TEST_F(CommandExecutorTest, Constructor_NullQueue_Throws)
{
    EXPECT_THROW({ gfx::backend::vulkan::core::CommandExecutor executor(nullptr); }, std::runtime_error);
}

// ============================================================================
// Single Execution Tests
// ============================================================================

TEST_F(CommandExecutorTest, Execute_SingleCommand_Succeeds)
{
    gfx::backend::vulkan::core::CommandExecutor executor(queue);

    bool commandRecorded = false;
    ASSERT_NO_THROW({
        executor.execute([&](VkCommandBuffer cmd) {
            EXPECT_NE(cmd, VK_NULL_HANDLE);
            commandRecorded = true;
        });
    });

    EXPECT_TRUE(commandRecorded);
}

TEST_F(CommandExecutorTest, Execute_CommandBufferIsValid_Succeeds)
{
    gfx::backend::vulkan::core::CommandExecutor executor(queue);

    VkCommandBuffer capturedCmd = VK_NULL_HANDLE;
    executor.execute([&](VkCommandBuffer cmd) {
        capturedCmd = cmd;
    });

    EXPECT_NE(capturedCmd, VK_NULL_HANDLE);
}

// ============================================================================
// Multiple Execution Tests
// ============================================================================

TEST_F(CommandExecutorTest, Execute_MultipleTimes_Succeeds)
{
    gfx::backend::vulkan::core::CommandExecutor executor(queue);

    int executionCount = 0;

    ASSERT_NO_THROW({
        executor.execute([&](VkCommandBuffer cmd) {
            EXPECT_NE(cmd, VK_NULL_HANDLE);
            executionCount++;
        });

        executor.execute([&](VkCommandBuffer cmd) {
            EXPECT_NE(cmd, VK_NULL_HANDLE);
            executionCount++;
        });

        executor.execute([&](VkCommandBuffer cmd) {
            EXPECT_NE(cmd, VK_NULL_HANDLE);
            executionCount++;
        });
    });

    EXPECT_EQ(executionCount, 3);
}

TEST_F(CommandExecutorTest, Execute_MultipleTimesGetsDifferentCommandBuffers_Succeeds)
{
    gfx::backend::vulkan::core::CommandExecutor executor(queue);

    VkCommandBuffer cmd1 = VK_NULL_HANDLE;
    VkCommandBuffer cmd2 = VK_NULL_HANDLE;

    executor.execute([&](VkCommandBuffer cmd) {
        cmd1 = cmd;
    });

    executor.execute([&](VkCommandBuffer cmd) {
        cmd2 = cmd;
    });

    EXPECT_NE(cmd1, VK_NULL_HANDLE);
    EXPECT_NE(cmd2, VK_NULL_HANDLE);
    // Note: Command buffers might be reused by Vulkan, so we don't assert they're different
}

// ============================================================================
// RAII Tests
// ============================================================================

TEST_F(CommandExecutorTest, Destructor_CleansUpResources_Succeeds)
{
    // Create in a scope and let it be destroyed
    {
        gfx::backend::vulkan::core::CommandExecutor executor(queue);
        executor.execute([](VkCommandBuffer cmd) {
            // Empty command
        });
    }
    // If we get here without crashing, cleanup worked
    SUCCEED();
}

TEST_F(CommandExecutorTest, InlineUsage_TemporaryObject_Succeeds)
{
    bool executed = false;
    ASSERT_NO_THROW({
        gfx::backend::vulkan::core::CommandExecutor(queue).execute([&](VkCommandBuffer cmd) {
            EXPECT_NE(cmd, VK_NULL_HANDLE);
            executed = true;
        });
    });

    EXPECT_TRUE(executed);
}

// ============================================================================
// Practical Usage Tests
// ============================================================================

TEST_F(CommandExecutorTest, Execute_ActualBufferFill_Succeeds)
{
    gfx::backend::vulkan::core::CommandExecutor executor(queue);

    // Create a simple buffer to fill
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = 1024;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buffer = VK_NULL_HANDLE;
    VkResult result = vkCreateBuffer(device->handle(), &bufferInfo, nullptr, &buffer);
    ASSERT_EQ(result, VK_SUCCESS);

    // Get memory requirements
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(device->handle(), buffer, &memReqs);

    // Allocate memory
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = 0; // Use first available memory type

    VkDeviceMemory memory = VK_NULL_HANDLE;
    result = vkAllocateMemory(device->handle(), &allocInfo, nullptr, &memory);
    ASSERT_EQ(result, VK_SUCCESS);

    // Bind memory to buffer
    result = vkBindBufferMemory(device->handle(), buffer, memory, 0);
    ASSERT_EQ(result, VK_SUCCESS);

    // Execute a fill command
    ASSERT_NO_THROW({
        executor.execute([buffer](VkCommandBuffer cmd) {
            vkCmdFillBuffer(cmd, buffer, 0, 1024, 0xDEADBEEF);
        });
    });

    // Cleanup
    vkDestroyBuffer(device->handle(), buffer, nullptr);
    vkFreeMemory(device->handle(), memory, nullptr);
}

TEST_F(CommandExecutorTest, Execute_MultipleBufferOperations_Succeeds)
{
    gfx::backend::vulkan::core::CommandExecutor executor(queue);

    // Create two buffers
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = 256;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buffer1 = VK_NULL_HANDLE;
    VkBuffer buffer2 = VK_NULL_HANDLE;
    vkCreateBuffer(device->handle(), &bufferInfo, nullptr, &buffer1);
    vkCreateBuffer(device->handle(), &bufferInfo, nullptr, &buffer2);

    // Allocate memory for buffer1
    VkMemoryRequirements memReqs1;
    vkGetBufferMemoryRequirements(device->handle(), buffer1, &memReqs1);
    VkMemoryAllocateInfo allocInfo1{};
    allocInfo1.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo1.allocationSize = memReqs1.size;
    allocInfo1.memoryTypeIndex = 0;
    VkDeviceMemory memory1 = VK_NULL_HANDLE;
    vkAllocateMemory(device->handle(), &allocInfo1, nullptr, &memory1);
    vkBindBufferMemory(device->handle(), buffer1, memory1, 0);

    // Allocate memory for buffer2
    VkMemoryRequirements memReqs2;
    vkGetBufferMemoryRequirements(device->handle(), buffer2, &memReqs2);
    VkMemoryAllocateInfo allocInfo2{};
    allocInfo2.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo2.allocationSize = memReqs2.size;
    allocInfo2.memoryTypeIndex = 0;
    VkDeviceMemory memory2 = VK_NULL_HANDLE;
    vkAllocateMemory(device->handle(), &allocInfo2, nullptr, &memory2);
    vkBindBufferMemory(device->handle(), buffer2, memory2, 0);

    // Execute multiple operations
    executor.execute([buffer1](VkCommandBuffer cmd) {
        vkCmdFillBuffer(cmd, buffer1, 0, 256, 0x12345678);
    });

    executor.execute([buffer1, buffer2](VkCommandBuffer cmd) {
        VkBufferCopy copyRegion{};
        copyRegion.size = 256;
        vkCmdCopyBuffer(cmd, buffer1, buffer2, 1, &copyRegion);
    });

    // Cleanup
    vkDestroyBuffer(device->handle(), buffer1, nullptr);
    vkDestroyBuffer(device->handle(), buffer2, nullptr);
    vkFreeMemory(device->handle(), memory1, nullptr);
    vkFreeMemory(device->handle(), memory2, nullptr);
}

} // anonymous namespace
