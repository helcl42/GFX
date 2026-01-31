#include <backend/webgpu/core/command/CommandEncoder.h>
#include <backend/webgpu/core/command/ComputePassEncoder.h>
#include <backend/webgpu/core/resource/Buffer.h>
#include <backend/webgpu/core/system/Device.h>
#include <backend/webgpu/core/system/Instance.h>

#include <gtest/gtest.h>

#include <memory>

namespace {

class WebGPUComputePassEncoderTest : public testing::Test {
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

TEST_F(WebGPUComputePassEncoderTest, CreateComputePassEncoder_CreatesSuccessfully)
{
    gfx::backend::webgpu::core::CommandEncoderCreateInfo cmdCreateInfo{};
    auto commandEncoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), cmdCreateInfo);

    gfx::backend::webgpu::core::ComputePassEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::ComputePassEncoder>(commandEncoder.get(), createInfo);

    EXPECT_NE(encoder->handle(), nullptr);
}

TEST_F(WebGPUComputePassEncoderTest, Handle_ReturnsValidWGPUComputePassEncoder)
{
    gfx::backend::webgpu::core::CommandEncoderCreateInfo cmdCreateInfo{};
    auto commandEncoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), cmdCreateInfo);

    gfx::backend::webgpu::core::ComputePassEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::ComputePassEncoder>(commandEncoder.get(), createInfo);

    WGPUComputePassEncoder handle = encoder->handle();
    EXPECT_NE(handle, nullptr);
}

TEST_F(WebGPUComputePassEncoderTest, DispatchWorkgroups_WorksCorrectly)
{
    gfx::backend::webgpu::core::CommandEncoderCreateInfo cmdCreateInfo{};
    auto commandEncoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), cmdCreateInfo);

    gfx::backend::webgpu::core::ComputePassEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::ComputePassEncoder>(commandEncoder.get(), createInfo);

    EXPECT_NO_THROW(encoder->dispatchWorkgroups(8, 8, 1));
}

TEST_F(WebGPUComputePassEncoderTest, DispatchIndirect_WorksCorrectly)
{
    gfx::backend::webgpu::core::BufferCreateInfo bufferInfo{};
    bufferInfo.size = 256;
    bufferInfo.usage = WGPUBufferUsage_Indirect;
    auto buffer = std::make_unique<gfx::backend::webgpu::core::Buffer>(device.get(), bufferInfo);

    gfx::backend::webgpu::core::CommandEncoderCreateInfo cmdCreateInfo{};
    auto commandEncoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), cmdCreateInfo);

    gfx::backend::webgpu::core::ComputePassEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::webgpu::core::ComputePassEncoder>(commandEncoder.get(), createInfo);

    EXPECT_NO_THROW(encoder->dispatchIndirect(buffer->handle(), 0));
}

TEST_F(WebGPUComputePassEncoderTest, Destructor_CleansUpResources)
{
    gfx::backend::webgpu::core::CommandEncoderCreateInfo cmdCreateInfo{};
    auto commandEncoder = std::make_unique<gfx::backend::webgpu::core::CommandEncoder>(device.get(), cmdCreateInfo);

    {
        gfx::backend::webgpu::core::ComputePassEncoderCreateInfo createInfo{};
        auto encoder = std::make_unique<gfx::backend::webgpu::core::ComputePassEncoder>(commandEncoder.get(), createInfo);
        EXPECT_NE(encoder->handle(), nullptr);
    }

    // If we reach here without crashing, cleanup succeeded
    SUCCEED();
}

} // anonymous namespace
