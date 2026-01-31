#include <backend/webgpu/core/system/Device.h>
#include <backend/webgpu/core/system/Instance.h>
#include <backend/webgpu/core/system/Queue.h>

#include <gtest/gtest.h>

#include <memory>

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class WebGPUDeviceTest : public testing::Test {
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
        } catch (const std::exception& e) {
            GTEST_SKIP() << "WebGPU not available: " << e.what();
        }
    }

    std::unique_ptr<gfx::backend::webgpu::core::Instance> instance;
    gfx::backend::webgpu::core::Adapter* adapter = nullptr;
    std::unique_ptr<gfx::backend::webgpu::core::Device> device;
};

// ============================================================================
// Basic Creation Tests
// ============================================================================

TEST_F(WebGPUDeviceTest, CreateDevice_CreatesSuccessfully)
{
    EXPECT_NE(device->handle(), nullptr);
}

TEST_F(WebGPUDeviceTest, Handle_ReturnsValidWGPUDevice)
{
    WGPUDevice handle = device->handle();
    EXPECT_NE(handle, nullptr);
}

TEST_F(WebGPUDeviceTest, GetAdapter_ReturnsCorrectAdapter)
{
    auto* adpt = device->getAdapter();
    EXPECT_EQ(adpt, adapter);
}

// ============================================================================
// Queue Tests
// ============================================================================

TEST_F(WebGPUDeviceTest, GetQueue_ReturnsValidQueue)
{
    auto* queue = device->getQueue();
    EXPECT_NE(queue, nullptr);
    EXPECT_NE(queue->handle(), nullptr);
}

TEST_F(WebGPUDeviceTest, GetQueue_ReturnsSameQueue)
{
    auto* queue1 = device->getQueue();
    auto* queue2 = device->getQueue();
    EXPECT_EQ(queue1, queue2);
}

// ============================================================================
// Limits Tests
// ============================================================================

TEST_F(WebGPUDeviceTest, GetLimits_ReturnsValidLimits)
{
    WGPULimits limits = device->getLimits();

    EXPECT_GT(limits.maxTextureDimension1D, 0);
    EXPECT_GT(limits.maxTextureDimension2D, 0);
    EXPECT_GT(limits.maxBindGroups, 0);
}

// ============================================================================
// Synchronization Tests
// ============================================================================

TEST_F(WebGPUDeviceTest, WaitIdle_CompletesSuccessfully)
{
    EXPECT_NO_THROW(device->waitIdle());
}

// ============================================================================
// Blit Tests
// ============================================================================

TEST_F(WebGPUDeviceTest, GetBlit_ReturnsValidBlit)
{
    auto* blit = device->getBlit();
    EXPECT_NE(blit, nullptr);
}

TEST_F(WebGPUDeviceTest, GetBlit_ReturnsSameBlit)
{
    auto* blit1 = device->getBlit();
    auto* blit2 = device->getBlit();
    EXPECT_EQ(blit1, blit2);
}

// ============================================================================
// Lifecycle Tests
// ============================================================================

TEST_F(WebGPUDeviceTest, Destructor_CleansUpResources)
{
    // Note: WebGPU/Dawn may not support multiple devices from the same adapter
    // Test device cleanup using a separate instance and adapter
    gfx::backend::webgpu::core::InstanceCreateInfo instInfo{};
    auto tempInstance = std::make_unique<gfx::backend::webgpu::core::Instance>(instInfo);

    gfx::backend::webgpu::core::AdapterCreateInfo adapterInfo{};
    adapterInfo.adapterIndex = 0;
    auto* tempAdapter = tempInstance->requestAdapter(adapterInfo);

    {
        gfx::backend::webgpu::core::DeviceCreateInfo deviceInfo{};
        auto tempDevice = std::make_unique<gfx::backend::webgpu::core::Device>(tempAdapter, deviceInfo);
        EXPECT_NE(tempDevice->handle(), nullptr);
    }
    // If we reach here without crashing, cleanup succeeded
    SUCCEED();
}

TEST_F(WebGPUDeviceTest, MultipleDevices_FromDifferentAdapters)
{
    // Note: WebGPU/Dawn may not support multiple devices from the same adapter
    // Test multiple devices by using separate instance/adapter pairs
    gfx::backend::webgpu::core::InstanceCreateInfo instInfo{};
    auto instance2 = std::make_unique<gfx::backend::webgpu::core::Instance>(instInfo);

    gfx::backend::webgpu::core::AdapterCreateInfo adapterInfo{};
    adapterInfo.adapterIndex = 0;
    auto* adapter2 = instance2->requestAdapter(adapterInfo);

    gfx::backend::webgpu::core::DeviceCreateInfo deviceInfo{};
    auto device2 = std::make_unique<gfx::backend::webgpu::core::Device>(adapter2, deviceInfo);

    EXPECT_NE(device->handle(), nullptr);
    EXPECT_NE(device2->handle(), nullptr);
    EXPECT_NE(device->handle(), device2->handle());
}

} // anonymous namespace
