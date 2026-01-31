#include <backend/webgpu/core/resource/Buffer.h>
#include <backend/webgpu/core/system/Device.h>
#include <backend/webgpu/core/system/Instance.h>
#include <backend/webgpu/core/system/Queue.h>

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class WebGPUQueueTest : public testing::Test {
protected:
    void SetUp() override
    {
        try {
            gfx::backend::webgpu::core::InstanceCreateInfo instInfo{};
            instance = std::make_unique<gfx::backend::webgpu::core::Instance>(instInfo);

            gfx::backend::webgpu::core::AdapterCreateInfo adapterInfo{};
            adapterInfo.adapterIndex = 0;
            adapter = instance->requestAdapter(adapterInfo);

            gfx::backend::webgpu::core::DeviceCreateInfo deviceInfo{};
            device = std::make_unique<gfx::backend::webgpu::core::Device>(adapter, deviceInfo);

            queue = device->getQueue();
        } catch (const std::exception& e) {
            GTEST_SKIP() << "WebGPU not available: " << e.what();
        }
    }

    std::unique_ptr<gfx::backend::webgpu::core::Instance> instance;
    gfx::backend::webgpu::core::Adapter* adapter = nullptr;
    std::unique_ptr<gfx::backend::webgpu::core::Device> device;
    gfx::backend::webgpu::core::Queue* queue = nullptr;
};

// ============================================================================
// Basic Tests
// ============================================================================

TEST_F(WebGPUQueueTest, Handle_ReturnsValidWGPUQueue)
{
    WGPUQueue handle = queue->handle();
    EXPECT_NE(handle, nullptr);
}

TEST_F(WebGPUQueueTest, GetDevice_ReturnsCorrectDevice)
{
    auto* dev = queue->getDevice();
    EXPECT_EQ(dev, device.get());
}

// ============================================================================
// Submit Tests
// ============================================================================

TEST_F(WebGPUQueueTest, Submit_WithEmptySubmit)
{
    gfx::backend::webgpu::core::SubmitInfo submitInfo{};
    submitInfo.commandEncoders = {};

    bool success = queue->submit(submitInfo);
    EXPECT_TRUE(success);
}

// ============================================================================
// Write Operations Tests
// ============================================================================

TEST_F(WebGPUQueueTest, WriteBuffer_WithData)
{
    gfx::backend::webgpu::core::BufferCreateInfo bufferInfo{};
    bufferInfo.size = 256;
    bufferInfo.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc;
    auto buffer = std::make_unique<gfx::backend::webgpu::core::Buffer>(device.get(), bufferInfo);

    std::vector<uint32_t> data(64, 42);
    EXPECT_NO_THROW(queue->writeBuffer(buffer.get(), 0, data.data(), data.size() * sizeof(uint32_t)));
}

TEST_F(WebGPUQueueTest, WriteBuffer_WithOffset)
{
    gfx::backend::webgpu::core::BufferCreateInfo bufferInfo{};
    bufferInfo.size = 512;
    bufferInfo.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc;
    auto buffer = std::make_unique<gfx::backend::webgpu::core::Buffer>(device.get(), bufferInfo);

    std::vector<uint32_t> data(32, 123);
    EXPECT_NO_THROW(queue->writeBuffer(buffer.get(), 128, data.data(), data.size() * sizeof(uint32_t)));
}

// ============================================================================
// Synchronization Tests
// ============================================================================

TEST_F(WebGPUQueueTest, WaitIdle_CompletesSuccessfully)
{
    bool success = queue->waitIdle();
    EXPECT_TRUE(success);
}

TEST_F(WebGPUQueueTest, WaitIdle_AfterSubmit)
{
    gfx::backend::webgpu::core::SubmitInfo submitInfo{};
    submitInfo.commandEncoders = {};

    bool submitSuccess = queue->submit(submitInfo);
    EXPECT_TRUE(submitSuccess);

    bool waitSuccess = queue->waitIdle();
    EXPECT_TRUE(waitSuccess);
}

} // anonymous namespace
