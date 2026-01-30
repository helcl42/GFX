#include <backend/vulkan/core/resource/BindGroupLayout.h>
#include <backend/vulkan/core/system/Adapter.h>
#include <backend/vulkan/core/system/Device.h>
#include <backend/vulkan/core/system/Instance.h>


#include <gtest/gtest.h>

// Test Vulkan core BindGroupLayout class
// These tests verify the internal bind group layout implementation, not the public API

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class VulkanBindGroupLayoutTest : public testing::Test {
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

TEST_F(VulkanBindGroupLayoutTest, CreateEmpty_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo createInfo{};
    createInfo.entries = {};

    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), createInfo);

    EXPECT_NE(layout.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanBindGroupLayoutTest, CreateSingleUniformBuffer_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BindGroupLayoutEntry entry{};
    entry.binding = 0;
    entry.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    entry.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo createInfo{};
    createInfo.entries = { entry };

    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), createInfo);

    EXPECT_NE(layout.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(layout.getBindingType(0), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
}

TEST_F(VulkanBindGroupLayoutTest, CreateSingleStorageBuffer_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BindGroupLayoutEntry entry{};
    entry.binding = 0;
    entry.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    entry.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo createInfo{};
    createInfo.entries = { entry };

    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), createInfo);

    EXPECT_NE(layout.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(layout.getBindingType(0), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
}

TEST_F(VulkanBindGroupLayoutTest, CreateSingleSampler_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BindGroupLayoutEntry entry{};
    entry.binding = 0;
    entry.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    entry.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo createInfo{};
    createInfo.entries = { entry };

    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), createInfo);

    EXPECT_NE(layout.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(layout.getBindingType(0), VK_DESCRIPTOR_TYPE_SAMPLER);
}

TEST_F(VulkanBindGroupLayoutTest, CreateSingleSampledTexture_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BindGroupLayoutEntry entry{};
    entry.binding = 0;
    entry.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    entry.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo createInfo{};
    createInfo.entries = { entry };

    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), createInfo);

    EXPECT_NE(layout.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(layout.getBindingType(0), VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
}

TEST_F(VulkanBindGroupLayoutTest, CreateSingleStorageTexture_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BindGroupLayoutEntry entry{};
    entry.binding = 0;
    entry.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    entry.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo createInfo{};
    createInfo.entries = { entry };

    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), createInfo);

    EXPECT_NE(layout.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(layout.getBindingType(0), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
}

TEST_F(VulkanBindGroupLayoutTest, CreateCombinedImageSampler_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BindGroupLayoutEntry entry{};
    entry.binding = 0;
    entry.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    entry.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo createInfo{};
    createInfo.entries = { entry };

    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), createInfo);

    EXPECT_NE(layout.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(layout.getBindingType(0), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
}

// ============================================================================
// Multiple Bindings Tests
// ============================================================================

TEST_F(VulkanBindGroupLayoutTest, CreateMultipleBindings_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BindGroupLayoutEntry entry0{};
    entry0.binding = 0;
    entry0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    entry0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry entry1{};
    entry1.binding = 1;
    entry1.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    entry1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry entry2{};
    entry2.binding = 2;
    entry2.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    entry2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo createInfo{};
    createInfo.entries = { entry0, entry1, entry2 };

    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), createInfo);

    EXPECT_NE(layout.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(layout.getBindingType(0), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    EXPECT_EQ(layout.getBindingType(1), VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    EXPECT_EQ(layout.getBindingType(2), VK_DESCRIPTOR_TYPE_SAMPLER);
}

TEST_F(VulkanBindGroupLayoutTest, CreateNonSequentialBindings_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BindGroupLayoutEntry entry0{};
    entry0.binding = 0;
    entry0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    entry0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry entry5{};
    entry5.binding = 5;
    entry5.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    entry5.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo createInfo{};
    createInfo.entries = { entry0, entry5 };

    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), createInfo);

    EXPECT_NE(layout.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(layout.getBindingType(0), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    EXPECT_EQ(layout.getBindingType(5), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
}

// ============================================================================
// Shader Stage Tests
// ============================================================================

TEST_F(VulkanBindGroupLayoutTest, CreateVertexStageOnly_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BindGroupLayoutEntry entry{};
    entry.binding = 0;
    entry.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    entry.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo createInfo{};
    createInfo.entries = { entry };

    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), createInfo);

    EXPECT_NE(layout.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanBindGroupLayoutTest, CreateFragmentStageOnly_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BindGroupLayoutEntry entry{};
    entry.binding = 0;
    entry.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    entry.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo createInfo{};
    createInfo.entries = { entry };

    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), createInfo);

    EXPECT_NE(layout.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanBindGroupLayoutTest, CreateComputeStageOnly_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BindGroupLayoutEntry entry{};
    entry.binding = 0;
    entry.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    entry.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo createInfo{};
    createInfo.entries = { entry };

    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), createInfo);

    EXPECT_NE(layout.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanBindGroupLayoutTest, CreateMultipleStages_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BindGroupLayoutEntry entry{};
    entry.binding = 0;
    entry.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    entry.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo createInfo{};
    createInfo.entries = { entry };

    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), createInfo);

    EXPECT_NE(layout.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanBindGroupLayoutTest, CreateAllGraphicsStages_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BindGroupLayoutEntry entry{};
    entry.binding = 0;
    entry.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    entry.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo createInfo{};
    createInfo.entries = { entry };

    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), createInfo);

    EXPECT_NE(layout.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Complex Layout Tests
// ============================================================================

TEST_F(VulkanBindGroupLayoutTest, CreateComplexGraphicsLayout_CreatesSuccessfully)
{
    // Typical graphics layout: MVP uniform buffer + textures + sampler
    gfx::backend::vulkan::core::BindGroupLayoutEntry mvpBuffer{};
    mvpBuffer.binding = 0;
    mvpBuffer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    mvpBuffer.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry materialBuffer{};
    materialBuffer.binding = 1;
    materialBuffer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    materialBuffer.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry albedoTexture{};
    albedoTexture.binding = 2;
    albedoTexture.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    albedoTexture.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry normalTexture{};
    normalTexture.binding = 3;
    normalTexture.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    normalTexture.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry sampler{};
    sampler.binding = 4;
    sampler.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    sampler.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo createInfo{};
    createInfo.entries = { mvpBuffer, materialBuffer, albedoTexture, normalTexture, sampler };

    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), createInfo);

    EXPECT_NE(layout.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(layout.getBindingType(0), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    EXPECT_EQ(layout.getBindingType(1), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    EXPECT_EQ(layout.getBindingType(2), VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    EXPECT_EQ(layout.getBindingType(3), VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    EXPECT_EQ(layout.getBindingType(4), VK_DESCRIPTOR_TYPE_SAMPLER);
}

TEST_F(VulkanBindGroupLayoutTest, CreateComplexComputeLayout_CreatesSuccessfully)
{
    // Typical compute layout: input buffers, output buffer, params
    gfx::backend::vulkan::core::BindGroupLayoutEntry inputBuffer0{};
    inputBuffer0.binding = 0;
    inputBuffer0.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    inputBuffer0.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry inputBuffer1{};
    inputBuffer1.binding = 1;
    inputBuffer1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    inputBuffer1.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry outputBuffer{};
    outputBuffer.binding = 2;
    outputBuffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    outputBuffer.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry paramsBuffer{};
    paramsBuffer.binding = 3;
    paramsBuffer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    paramsBuffer.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo createInfo{};
    createInfo.entries = { inputBuffer0, inputBuffer1, outputBuffer, paramsBuffer };

    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), createInfo);

    EXPECT_NE(layout.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(layout.getBindingType(0), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    EXPECT_EQ(layout.getBindingType(1), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    EXPECT_EQ(layout.getBindingType(2), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    EXPECT_EQ(layout.getBindingType(3), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
}

TEST_F(VulkanBindGroupLayoutTest, CreateImageProcessingLayout_CreatesSuccessfully)
{
    // Image processing: input texture, output storage image, params
    gfx::backend::vulkan::core::BindGroupLayoutEntry inputImage{};
    inputImage.binding = 0;
    inputImage.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    inputImage.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry outputImage{};
    outputImage.binding = 1;
    outputImage.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    outputImage.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry paramsBuffer{};
    paramsBuffer.binding = 2;
    paramsBuffer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    paramsBuffer.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry sampler{};
    sampler.binding = 3;
    sampler.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    sampler.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo createInfo{};
    createInfo.entries = { inputImage, outputImage, paramsBuffer, sampler };

    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), createInfo);

    EXPECT_NE(layout.handle(), VK_NULL_HANDLE);
    EXPECT_EQ(layout.getBindingType(0), VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    EXPECT_EQ(layout.getBindingType(1), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    EXPECT_EQ(layout.getBindingType(2), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    EXPECT_EQ(layout.getBindingType(3), VK_DESCRIPTOR_TYPE_SAMPLER);
}

} // namespace
