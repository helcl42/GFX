#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

#include <memory>

// ===========================================================================
// Queue Test Suite
// ===========================================================================

class GfxCppQueueTest : public testing::TestWithParam<gfx::Backend> {
protected:
    void SetUp() override
    {
        backend = GetParam();
        try {
            gfx::InstanceDescriptor instanceDesc{
                .backend = backend
            };
            instance = gfx::createInstance(instanceDesc);

            gfx::AdapterDescriptor adapterDesc{
                .adapterIndex = 0
            };
            adapter = instance->requestAdapter(adapterDesc);

            gfx::DeviceDescriptor deviceDesc{
                .label = "Test Device"
            };
            device = adapter->createDevice(deviceDesc);
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up GFX device for backend "
                         << (backend == gfx::Backend::Vulkan ? "Vulkan" : "WebGPU")
                         << ": " << e.what();
        }
    }

    void TearDown() override
    {
        device.reset();
        adapter.reset();
        instance.reset();
    }

    gfx::Backend backend;
    std::shared_ptr<gfx::Instance> instance;
    std::shared_ptr<gfx::Adapter> adapter;
    std::shared_ptr<gfx::Device> device;
};

// ===========================================================================
// Test Cases
// ===========================================================================

// Test: Get default queue
TEST_P(GfxCppQueueTest, GetDefaultQueue)
{
    auto queue = device->getQueue();
    EXPECT_NE(queue, nullptr);
}

// Test: Get queue by index
TEST_P(GfxCppQueueTest, GetQueueByIndex)
{
    auto queue = device->getQueueByIndex(0, 0);
    EXPECT_NE(queue, nullptr);
}

// Test: Queue submit with empty descriptor
TEST_P(GfxCppQueueTest, SubmitWithEmptyDescriptor)
{
    auto queue = device->getQueue();
    ASSERT_NE(queue, nullptr);

    gfx::SubmitDescriptor submitDesc{};
    queue->submit(submitDesc);
}

// Test: Queue write buffer
TEST_P(GfxCppQueueTest, WriteBuffer)
{
    auto queue = device->getQueue();
    ASSERT_NE(queue, nullptr);

    // Create a buffer
    auto buffer = device->createBuffer({ .size = 256, .usage = gfx::BufferUsage::CopyDst });
    ASSERT_NE(buffer, nullptr);

    // Write data to buffer
    uint32_t data[64];
    for (int i = 0; i < 64; i++) {
        data[i] = i;
    }

    queue->writeBuffer(buffer, 0, data, sizeof(data));
}

// Test: Queue write buffer with null buffer (should throw exception)
TEST_P(GfxCppQueueTest, WriteBufferWithNullBuffer)
{
    auto queue = device->getQueue();
    ASSERT_NE(queue, nullptr);

    uint32_t data = 42;
    EXPECT_THROW(
        queue->writeBuffer(nullptr, 0, &data, sizeof(data)),
        std::exception);
}

// Test: Queue wait idle
TEST_P(GfxCppQueueTest, WaitIdle)
{
    auto queue = device->getQueue();
    ASSERT_NE(queue, nullptr);

    queue->waitIdle();
}

// Test: Queue write buffer with offset
TEST_P(GfxCppQueueTest, WriteBufferWithOffset)
{
    auto queue = device->getQueue();
    ASSERT_NE(queue, nullptr);

    // Create a buffer
    auto buffer = device->createBuffer({ .size = 256, .usage = gfx::BufferUsage::CopyDst });
    ASSERT_NE(buffer, nullptr);

    // Write data at offset 64
    uint32_t data[16];
    for (int i = 0; i < 16; i++) {
        data[i] = i + 100;
    }

    queue->writeBuffer(buffer, 64, data, sizeof(data));
}

// Test: Queue write buffer using template helper
TEST_P(GfxCppQueueTest, WriteBufferTemplateHelper)
{
    auto queue = device->getQueue();
    ASSERT_NE(queue, nullptr);

    // Create a buffer
    auto buffer = device->createBuffer({ .size = 256, .usage = gfx::BufferUsage::CopyDst });
    ASSERT_NE(buffer, nullptr);

    // Write data using vector template helper
    std::vector<uint32_t> data;
    for (int i = 0; i < 64; i++) {
        data.push_back(i);
    }

    queue->writeBuffer(buffer, 0, data);
}

// Test: Write buffer and sync
TEST_P(GfxCppQueueTest, WriteBufferAndSync)
{
    auto queue = device->getQueue();
    ASSERT_NE(queue, nullptr);

    // Create a buffer
    auto buffer = device->createBuffer({ .size = 1024, .usage = gfx::BufferUsage::CopyDst });
    ASSERT_NE(buffer, nullptr);

    // Write data
    uint32_t data1[64];
    for (int i = 0; i < 64; i++) {
        data1[i] = i;
    }
    queue->writeBuffer(buffer, 0, data1, sizeof(data1));

    // Submit empty descriptor (synchronization point)
    gfx::SubmitDescriptor submitDesc{};
    queue->submit(submitDesc);

    // Write more data
    uint32_t data2[64];
    for (int i = 0; i < 64; i++) {
        data2[i] = i + 100;
    }
    queue->writeBuffer(buffer, 256, data2, sizeof(data2));

    queue->waitIdle();
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppQueueTest,
    testing::Values(gfx::Backend::Vulkan, gfx::Backend::WebGPU),
    [](const testing::TestParamInfo<gfx::Backend>& info) {
        return info.param == gfx::Backend::Vulkan ? "Vulkan" : "WebGPU";
    });
