#include <backend/vulkan/core/command/CommandEncoder.h>
#include <backend/vulkan/core/command/ComputePassEncoder.h>
#include <backend/vulkan/core/compute/ComputePipeline.h>
#include <backend/vulkan/core/resource/BindGroup.h>
#include <backend/vulkan/core/resource/BindGroupLayout.h>
#include <backend/vulkan/core/resource/Buffer.h>
#include <backend/vulkan/core/resource/Shader.h>
#include <backend/vulkan/core/system/Device.h>
#include <backend/vulkan/core/system/Instance.h>

#include <gtest/gtest.h>

#include <memory>

namespace {

// Minimal compute shader SPIR-V (empty main function, workgroup size 1,1,1)
static const uint32_t MINIMAL_COMPUTE_SPIRV[] = {
    0x07230203, 0x00010000, 0x00080001, 0x00000009, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0005000f, 0x00000005, 0x00000004, 0x6e69616d, 0x00000000, 0x00060010, 0x00000004, 0x00000011,
    0x00000001, 0x00000001, 0x00000001, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004,
    0x6e69616d, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00050036,
    0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000008, 0x000100fd, 0x00010038
};

// ============================================================================
// Test Fixture
// ============================================================================

class VulkanComputePassEncoderTest : public testing::Test {
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
// Basic Creation Tests
// ============================================================================

TEST_F(VulkanComputePassEncoderTest, CreateComputePassEncoder_CreatesSuccessfully)
{
    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    gfx::backend::vulkan::core::ComputePassEncoderCreateInfo createInfo{};
    createInfo.label = "TestComputePass";

    auto encoder = std::make_unique<gfx::backend::vulkan::core::ComputePassEncoder>(commandEncoder.get(), createInfo);

    EXPECT_NE(encoder->handle(), VK_NULL_HANDLE);
    EXPECT_EQ(encoder->device(), device.get());
    EXPECT_EQ(encoder->commandEncoder(), commandEncoder.get());

    commandEncoder->end();
}

TEST_F(VulkanComputePassEncoderTest, CreateComputePassEncoder_WithNullLabel)
{
    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    gfx::backend::vulkan::core::ComputePassEncoderCreateInfo createInfo{};
    createInfo.label = nullptr;

    auto encoder = std::make_unique<gfx::backend::vulkan::core::ComputePassEncoder>(commandEncoder.get(), createInfo);

    EXPECT_NE(encoder->handle(), VK_NULL_HANDLE);

    commandEncoder->end();
}

// ============================================================================
// Handle Tests
// ============================================================================

TEST_F(VulkanComputePassEncoderTest, Handle_ReturnsValidVkCommandBuffer)
{
    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    gfx::backend::vulkan::core::ComputePassEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::vulkan::core::ComputePassEncoder>(commandEncoder.get(), createInfo);

    VkCommandBuffer handle = encoder->handle();
    EXPECT_NE(handle, VK_NULL_HANDLE);
    EXPECT_EQ(handle, commandEncoder->handle());

    commandEncoder->end();
}

// ============================================================================
// Device Tests
// ============================================================================

TEST_F(VulkanComputePassEncoderTest, Device_ReturnsCorrectDevice)
{
    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    gfx::backend::vulkan::core::ComputePassEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::vulkan::core::ComputePassEncoder>(commandEncoder.get(), createInfo);

    EXPECT_EQ(encoder->device(), device.get());

    commandEncoder->end();
}

TEST_F(VulkanComputePassEncoderTest, CommandEncoder_ReturnsCorrectEncoder)
{
    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    gfx::backend::vulkan::core::ComputePassEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::vulkan::core::ComputePassEncoder>(commandEncoder.get(), createInfo);

    EXPECT_EQ(encoder->commandEncoder(), commandEncoder.get());

    commandEncoder->end();
}

// ============================================================================
// Pipeline Tests
// ============================================================================

TEST_F(VulkanComputePassEncoderTest, SetPipeline_WorksCorrectly)
{
    gfx::backend::vulkan::core::ShaderCreateInfo shaderInfo{};
    shaderInfo.code = MINIMAL_COMPUTE_SPIRV;
    shaderInfo.codeSize = sizeof(MINIMAL_COMPUTE_SPIRV);
    auto shader = std::make_unique<gfx::backend::vulkan::core::Shader>(device.get(), shaderInfo);

    gfx::backend::vulkan::core::ComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.module = shader->handle();
    pipelineInfo.entryPoint = "main";
    auto pipeline = std::make_unique<gfx::backend::vulkan::core::ComputePipeline>(device.get(), pipelineInfo);

    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    gfx::backend::vulkan::core::ComputePassEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::vulkan::core::ComputePassEncoder>(commandEncoder.get(), createInfo);

    EXPECT_NO_THROW(encoder->setPipeline(pipeline.get()));

    commandEncoder->end();
}

// ============================================================================
// Bind Group Tests
// ============================================================================

TEST_F(VulkanComputePassEncoderTest, SetBindGroup_WithoutDynamicOffsets)
{
    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo layoutInfo{};
    auto layout = std::make_unique<gfx::backend::vulkan::core::BindGroupLayout>(device.get(), layoutInfo);

    gfx::backend::vulkan::core::BindGroupCreateInfo bindGroupInfo{};
    bindGroupInfo.layout = layout->handle();
    auto bindGroup = std::make_unique<gfx::backend::vulkan::core::BindGroup>(device.get(), bindGroupInfo);

    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    gfx::backend::vulkan::core::ComputePassEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::vulkan::core::ComputePassEncoder>(commandEncoder.get(), createInfo);

    EXPECT_NO_THROW(encoder->setBindGroup(0, bindGroup.get(), nullptr, 0));

    commandEncoder->end();
}

TEST_F(VulkanComputePassEncoderTest, SetBindGroup_WithDynamicOffsets)
{
    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo layoutInfo{};
    auto layout = std::make_unique<gfx::backend::vulkan::core::BindGroupLayout>(device.get(), layoutInfo);

    gfx::backend::vulkan::core::BindGroupCreateInfo bindGroupInfo{};
    bindGroupInfo.layout = layout->handle();
    auto bindGroup = std::make_unique<gfx::backend::vulkan::core::BindGroup>(device.get(), bindGroupInfo);

    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    gfx::backend::vulkan::core::ComputePassEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::vulkan::core::ComputePassEncoder>(commandEncoder.get(), createInfo);

    uint32_t dynamicOffsets[] = { 0, 256 };
    EXPECT_NO_THROW(encoder->setBindGroup(0, bindGroup.get(), dynamicOffsets, 2));

    commandEncoder->end();
}

// ============================================================================
// Dispatch Tests
// ============================================================================

TEST_F(VulkanComputePassEncoderTest, DispatchWorkgroups_WorksCorrectly)
{
    gfx::backend::vulkan::core::ShaderCreateInfo shaderInfo{};
    shaderInfo.code = MINIMAL_COMPUTE_SPIRV;
    shaderInfo.codeSize = sizeof(MINIMAL_COMPUTE_SPIRV);
    auto shader = std::make_unique<gfx::backend::vulkan::core::Shader>(device.get(), shaderInfo);

    gfx::backend::vulkan::core::ComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.module = shader->handle();
    pipelineInfo.entryPoint = "main";
    auto pipeline = std::make_unique<gfx::backend::vulkan::core::ComputePipeline>(device.get(), pipelineInfo);

    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    gfx::backend::vulkan::core::ComputePassEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::vulkan::core::ComputePassEncoder>(commandEncoder.get(), createInfo);

    encoder->setPipeline(pipeline.get());
    EXPECT_NO_THROW(encoder->dispatchWorkgroups(1, 1, 1));

    commandEncoder->end();
}

TEST_F(VulkanComputePassEncoderTest, DispatchWorkgroups_WithMultipleWorkgroups)
{
    gfx::backend::vulkan::core::ShaderCreateInfo shaderInfo{};
    shaderInfo.code = MINIMAL_COMPUTE_SPIRV;
    shaderInfo.codeSize = sizeof(MINIMAL_COMPUTE_SPIRV);
    auto shader = std::make_unique<gfx::backend::vulkan::core::Shader>(device.get(), shaderInfo);

    gfx::backend::vulkan::core::ComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.module = shader->handle();
    pipelineInfo.entryPoint = "main";
    auto pipeline = std::make_unique<gfx::backend::vulkan::core::ComputePipeline>(device.get(), pipelineInfo);

    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    gfx::backend::vulkan::core::ComputePassEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::vulkan::core::ComputePassEncoder>(commandEncoder.get(), createInfo);

    encoder->setPipeline(pipeline.get());
    EXPECT_NO_THROW(encoder->dispatchWorkgroups(16, 16, 1));

    commandEncoder->end();
}

TEST_F(VulkanComputePassEncoderTest, DispatchIndirect_WorksCorrectly)
{
    gfx::backend::vulkan::core::ShaderCreateInfo shaderInfo{};
    shaderInfo.code = MINIMAL_COMPUTE_SPIRV;
    shaderInfo.codeSize = sizeof(MINIMAL_COMPUTE_SPIRV);
    auto shader = std::make_unique<gfx::backend::vulkan::core::Shader>(device.get(), shaderInfo);

    gfx::backend::vulkan::core::ComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.module = shader->handle();
    pipelineInfo.entryPoint = "main";
    auto pipeline = std::make_unique<gfx::backend::vulkan::core::ComputePipeline>(device.get(), pipelineInfo);

    gfx::backend::vulkan::core::BufferCreateInfo bufferInfo{};
    bufferInfo.size = 256;
    bufferInfo.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    auto buffer = std::make_unique<gfx::backend::vulkan::core::Buffer>(device.get(), bufferInfo);

    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    gfx::backend::vulkan::core::ComputePassEncoderCreateInfo createInfo{};
    auto encoder = std::make_unique<gfx::backend::vulkan::core::ComputePassEncoder>(commandEncoder.get(), createInfo);

    encoder->setPipeline(pipeline.get());
    EXPECT_NO_THROW(encoder->dispatchIndirect(buffer.get(), 0));

    commandEncoder->end();
}

// ============================================================================
// Lifecycle Tests
// ============================================================================

TEST_F(VulkanComputePassEncoderTest, Destructor_CleansUpResources)
{
    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    {
        gfx::backend::vulkan::core::ComputePassEncoderCreateInfo createInfo{};
        auto encoder = std::make_unique<gfx::backend::vulkan::core::ComputePassEncoder>(commandEncoder.get(), createInfo);
        EXPECT_NE(encoder->handle(), VK_NULL_HANDLE);
    }

    commandEncoder->end();

    // If we reach here without crashing, cleanup succeeded
    SUCCEED();
}

TEST_F(VulkanComputePassEncoderTest, MultipleComputePasses_Sequential)
{
    auto commandEncoder = std::make_unique<gfx::backend::vulkan::core::CommandEncoder>(device.get());

    commandEncoder->begin();

    for (int i = 0; i < 3; ++i) {
        gfx::backend::vulkan::core::ComputePassEncoderCreateInfo createInfo{};
        auto encoder = std::make_unique<gfx::backend::vulkan::core::ComputePassEncoder>(commandEncoder.get(), createInfo);
        EXPECT_NE(encoder->handle(), VK_NULL_HANDLE);
    }

    commandEncoder->end();

    SUCCEED();
}

} // anonymous namespace
