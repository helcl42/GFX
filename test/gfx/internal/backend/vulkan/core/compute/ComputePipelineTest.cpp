#include <backend/vulkan/core/compute/ComputePipeline.h>
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

class VulkanComputePipelineTest : public testing::Test {
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

TEST_F(VulkanComputePipelineTest, CreateComputePipeline_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::ShaderCreateInfo shaderInfo{};
    shaderInfo.code = MINIMAL_COMPUTE_SPIRV;
    shaderInfo.codeSize = sizeof(MINIMAL_COMPUTE_SPIRV);
    auto shader = std::make_unique<gfx::backend::vulkan::core::Shader>(device.get(), shaderInfo);

    gfx::backend::vulkan::core::ComputePipelineCreateInfo createInfo{};
    createInfo.module = shader->handle();
    createInfo.entryPoint = "main";

    auto pipeline = std::make_unique<gfx::backend::vulkan::core::ComputePipeline>(device.get(), createInfo);

    EXPECT_NE(pipeline->handle(), VK_NULL_HANDLE);
    EXPECT_NE(pipeline->layout(), VK_NULL_HANDLE);
}

TEST_F(VulkanComputePipelineTest, CreateComputePipeline_WithEmptyBindGroupLayouts)
{
    gfx::backend::vulkan::core::ShaderCreateInfo shaderInfo{};
    shaderInfo.code = MINIMAL_COMPUTE_SPIRV;
    shaderInfo.codeSize = sizeof(MINIMAL_COMPUTE_SPIRV);
    auto shader = std::make_unique<gfx::backend::vulkan::core::Shader>(device.get(), shaderInfo);

    gfx::backend::vulkan::core::ComputePipelineCreateInfo createInfo{};
    createInfo.bindGroupLayouts = {};
    createInfo.module = shader->handle();
    createInfo.entryPoint = "main";

    auto pipeline = std::make_unique<gfx::backend::vulkan::core::ComputePipeline>(device.get(), createInfo);

    EXPECT_NE(pipeline->handle(), VK_NULL_HANDLE);
    EXPECT_NE(pipeline->layout(), VK_NULL_HANDLE);
}

// ============================================================================
// Handle Tests
// ============================================================================

TEST_F(VulkanComputePipelineTest, Handle_ReturnsValidVkPipeline)
{
    gfx::backend::vulkan::core::ShaderCreateInfo shaderInfo{};
    shaderInfo.code = MINIMAL_COMPUTE_SPIRV;
    shaderInfo.codeSize = sizeof(MINIMAL_COMPUTE_SPIRV);
    auto shader = std::make_unique<gfx::backend::vulkan::core::Shader>(device.get(), shaderInfo);

    gfx::backend::vulkan::core::ComputePipelineCreateInfo createInfo{};
    createInfo.module = shader->handle();
    createInfo.entryPoint = "main";

    auto pipeline = std::make_unique<gfx::backend::vulkan::core::ComputePipeline>(device.get(), createInfo);

    VkPipeline handle = pipeline->handle();
    EXPECT_NE(handle, VK_NULL_HANDLE);
}

TEST_F(VulkanComputePipelineTest, Handle_IsUnique)
{
    gfx::backend::vulkan::core::ShaderCreateInfo shaderInfo{};
    shaderInfo.code = MINIMAL_COMPUTE_SPIRV;
    shaderInfo.codeSize = sizeof(MINIMAL_COMPUTE_SPIRV);
    auto shader = std::make_unique<gfx::backend::vulkan::core::Shader>(device.get(), shaderInfo);

    gfx::backend::vulkan::core::ComputePipelineCreateInfo createInfo{};
    createInfo.module = shader->handle();
    createInfo.entryPoint = "main";

    auto pipeline1 = std::make_unique<gfx::backend::vulkan::core::ComputePipeline>(device.get(), createInfo);
    auto pipeline2 = std::make_unique<gfx::backend::vulkan::core::ComputePipeline>(device.get(), createInfo);

    EXPECT_NE(pipeline1->handle(), pipeline2->handle());
}

// ============================================================================
// Layout Tests
// ============================================================================

TEST_F(VulkanComputePipelineTest, Layout_ReturnsValidVkPipelineLayout)
{
    gfx::backend::vulkan::core::ShaderCreateInfo shaderInfo{};
    shaderInfo.code = MINIMAL_COMPUTE_SPIRV;
    shaderInfo.codeSize = sizeof(MINIMAL_COMPUTE_SPIRV);
    auto shader = std::make_unique<gfx::backend::vulkan::core::Shader>(device.get(), shaderInfo);

    gfx::backend::vulkan::core::ComputePipelineCreateInfo createInfo{};
    createInfo.module = shader->handle();
    createInfo.entryPoint = "main";

    auto pipeline = std::make_unique<gfx::backend::vulkan::core::ComputePipeline>(device.get(), createInfo);

    VkPipelineLayout layout = pipeline->layout();
    EXPECT_NE(layout, VK_NULL_HANDLE);
}

TEST_F(VulkanComputePipelineTest, Layout_IsUnique)
{
    gfx::backend::vulkan::core::ShaderCreateInfo shaderInfo{};
    shaderInfo.code = MINIMAL_COMPUTE_SPIRV;
    shaderInfo.codeSize = sizeof(MINIMAL_COMPUTE_SPIRV);
    auto shader = std::make_unique<gfx::backend::vulkan::core::Shader>(device.get(), shaderInfo);

    gfx::backend::vulkan::core::ComputePipelineCreateInfo createInfo{};
    createInfo.module = shader->handle();
    createInfo.entryPoint = "main";

    auto pipeline1 = std::make_unique<gfx::backend::vulkan::core::ComputePipeline>(device.get(), createInfo);
    auto pipeline2 = std::make_unique<gfx::backend::vulkan::core::ComputePipeline>(device.get(), createInfo);

    EXPECT_NE(pipeline1->layout(), pipeline2->layout());
}

// ============================================================================
// Entry Point Tests
// ============================================================================

TEST_F(VulkanComputePipelineTest, CreateComputePipeline_WithMainEntryPoint)
{
    gfx::backend::vulkan::core::ShaderCreateInfo shaderInfo{};
    shaderInfo.code = MINIMAL_COMPUTE_SPIRV;
    shaderInfo.codeSize = sizeof(MINIMAL_COMPUTE_SPIRV);
    auto shader = std::make_unique<gfx::backend::vulkan::core::Shader>(device.get(), shaderInfo);

    gfx::backend::vulkan::core::ComputePipelineCreateInfo createInfo{};
    createInfo.module = shader->handle();
    createInfo.entryPoint = "main";

    auto pipeline = std::make_unique<gfx::backend::vulkan::core::ComputePipeline>(device.get(), createInfo);

    EXPECT_NE(pipeline->handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Lifecycle Tests
// ============================================================================

TEST_F(VulkanComputePipelineTest, Destructor_CleansUpResources)
{
    gfx::backend::vulkan::core::ShaderCreateInfo shaderInfo{};
    shaderInfo.code = MINIMAL_COMPUTE_SPIRV;
    shaderInfo.codeSize = sizeof(MINIMAL_COMPUTE_SPIRV);
    auto shader = std::make_unique<gfx::backend::vulkan::core::Shader>(device.get(), shaderInfo);

    gfx::backend::vulkan::core::ComputePipelineCreateInfo createInfo{};
    createInfo.module = shader->handle();
    createInfo.entryPoint = "main";

    {
        auto pipeline = std::make_unique<gfx::backend::vulkan::core::ComputePipeline>(device.get(), createInfo);
        EXPECT_NE(pipeline->handle(), VK_NULL_HANDLE);
    }

    // If we reach here without crashing, cleanup succeeded
    SUCCEED();
}

TEST_F(VulkanComputePipelineTest, MultipleComputePipelines_CanCoexist)
{
    gfx::backend::vulkan::core::ShaderCreateInfo shaderInfo{};
    shaderInfo.code = MINIMAL_COMPUTE_SPIRV;
    shaderInfo.codeSize = sizeof(MINIMAL_COMPUTE_SPIRV);
    auto shader = std::make_unique<gfx::backend::vulkan::core::Shader>(device.get(), shaderInfo);

    gfx::backend::vulkan::core::ComputePipelineCreateInfo createInfo{};
    createInfo.module = shader->handle();
    createInfo.entryPoint = "main";

    auto pipeline1 = std::make_unique<gfx::backend::vulkan::core::ComputePipeline>(device.get(), createInfo);
    auto pipeline2 = std::make_unique<gfx::backend::vulkan::core::ComputePipeline>(device.get(), createInfo);
    auto pipeline3 = std::make_unique<gfx::backend::vulkan::core::ComputePipeline>(device.get(), createInfo);

    EXPECT_NE(pipeline1->handle(), VK_NULL_HANDLE);
    EXPECT_NE(pipeline2->handle(), VK_NULL_HANDLE);
    EXPECT_NE(pipeline3->handle(), VK_NULL_HANDLE);

    EXPECT_NE(pipeline1->handle(), pipeline2->handle());
    EXPECT_NE(pipeline2->handle(), pipeline3->handle());
    EXPECT_NE(pipeline1->handle(), pipeline3->handle());
}

// ============================================================================
// Bind Group Layout Tests
// ============================================================================

TEST_F(VulkanComputePipelineTest, CreateComputePipeline_WithBindGroupLayouts)
{
    // Create a bind group layout
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;

    VkDescriptorSetLayout setLayout;
    VkResult result = vkCreateDescriptorSetLayout(device->handle(), &layoutInfo, nullptr, &setLayout);
    ASSERT_EQ(result, VK_SUCCESS);

    gfx::backend::vulkan::core::ShaderCreateInfo shaderInfo{};
    shaderInfo.code = MINIMAL_COMPUTE_SPIRV;
    shaderInfo.codeSize = sizeof(MINIMAL_COMPUTE_SPIRV);
    auto shader = std::make_unique<gfx::backend::vulkan::core::Shader>(device.get(), shaderInfo);

    gfx::backend::vulkan::core::ComputePipelineCreateInfo createInfo{};
    createInfo.bindGroupLayouts = { setLayout };
    createInfo.module = shader->handle();
    createInfo.entryPoint = "main";

    auto pipeline = std::make_unique<gfx::backend::vulkan::core::ComputePipeline>(device.get(), createInfo);

    EXPECT_NE(pipeline->handle(), VK_NULL_HANDLE);
    EXPECT_NE(pipeline->layout(), VK_NULL_HANDLE);

    vkDestroyDescriptorSetLayout(device->handle(), setLayout, nullptr);
}

TEST_F(VulkanComputePipelineTest, CreateComputePipeline_WithMultipleBindGroupLayouts)
{
    // Create first bind group layout
    VkDescriptorSetLayoutBinding binding1{};
    binding1.binding = 0;
    binding1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding1.descriptorCount = 1;
    binding1.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo1{};
    layoutInfo1.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo1.bindingCount = 1;
    layoutInfo1.pBindings = &binding1;

    VkDescriptorSetLayout setLayout1;
    VkResult result = vkCreateDescriptorSetLayout(device->handle(), &layoutInfo1, nullptr, &setLayout1);
    ASSERT_EQ(result, VK_SUCCESS);

    // Create second bind group layout
    VkDescriptorSetLayoutBinding binding2{};
    binding2.binding = 0;
    binding2.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding2.descriptorCount = 1;
    binding2.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo2{};
    layoutInfo2.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo2.bindingCount = 1;
    layoutInfo2.pBindings = &binding2;

    VkDescriptorSetLayout setLayout2;
    result = vkCreateDescriptorSetLayout(device->handle(), &layoutInfo2, nullptr, &setLayout2);
    ASSERT_EQ(result, VK_SUCCESS);

    gfx::backend::vulkan::core::ShaderCreateInfo shaderInfo{};
    shaderInfo.code = MINIMAL_COMPUTE_SPIRV;
    shaderInfo.codeSize = sizeof(MINIMAL_COMPUTE_SPIRV);
    auto shader = std::make_unique<gfx::backend::vulkan::core::Shader>(device.get(), shaderInfo);

    gfx::backend::vulkan::core::ComputePipelineCreateInfo createInfo{};
    createInfo.bindGroupLayouts = { setLayout1, setLayout2 };
    createInfo.module = shader->handle();
    createInfo.entryPoint = "main";

    auto pipeline = std::make_unique<gfx::backend::vulkan::core::ComputePipeline>(device.get(), createInfo);

    EXPECT_NE(pipeline->handle(), VK_NULL_HANDLE);
    EXPECT_NE(pipeline->layout(), VK_NULL_HANDLE);

    vkDestroyDescriptorSetLayout(device->handle(), setLayout1, nullptr);
    vkDestroyDescriptorSetLayout(device->handle(), setLayout2, nullptr);
}

} // anonymous namespace
