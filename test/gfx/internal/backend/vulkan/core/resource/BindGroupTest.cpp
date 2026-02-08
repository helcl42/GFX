#include <backend/vulkan/core/resource/BindGroup.h>
#include <backend/vulkan/core/resource/BindGroupLayout.h>
#include <backend/vulkan/core/resource/Buffer.h>
#include <backend/vulkan/core/resource/Sampler.h>
#include <backend/vulkan/core/resource/Texture.h>
#include <backend/vulkan/core/resource/TextureView.h>
#include <backend/vulkan/core/system/Adapter.h>
#include <backend/vulkan/core/system/Device.h>
#include <backend/vulkan/core/system/Instance.h>

#include <gtest/gtest.h>

// Test Vulkan core BindGroup class
// These tests verify the internal bind group implementation, not the public API

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class VulkanBindGroupTest : public testing::Test {
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

TEST_F(VulkanBindGroupTest, CreateEmpty_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo layoutInfo{};
    layoutInfo.entries = {};
    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), layoutInfo);

    gfx::backend::vulkan::core::BindGroupCreateInfo createInfo{};
    createInfo.layout = layout.handle();
    createInfo.entries = {};

    gfx::backend::vulkan::core::BindGroup bindGroup(device.get(), createInfo);

    EXPECT_NE(bindGroup.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanBindGroupTest, CreateWithUniformBuffer_CreatesSuccessfully)
{
    // Create layout
    gfx::backend::vulkan::core::BindGroupLayoutEntry layoutEntry{};
    layoutEntry.binding = 0;
    layoutEntry.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutEntry.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo layoutInfo{};
    layoutInfo.entries = { layoutEntry };
    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), layoutInfo);

    // Create buffer
    gfx::backend::vulkan::core::BufferCreateInfo bufferInfo{};
    bufferInfo.size = 256;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    gfx::backend::vulkan::core::Buffer buffer(device.get(), bufferInfo);

    // Create bind group
    gfx::backend::vulkan::core::BindGroupEntry entry{};
    entry.binding = 0;
    entry.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    entry.buffer = buffer.handle();
    entry.bufferOffset = 0;
    entry.bufferSize = 256;

    gfx::backend::vulkan::core::BindGroupCreateInfo createInfo{};
    createInfo.layout = layout.handle();
    createInfo.entries = { entry };

    gfx::backend::vulkan::core::BindGroup bindGroup(device.get(), createInfo);

    EXPECT_NE(bindGroup.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanBindGroupTest, CreateWithStorageBuffer_CreatesSuccessfully)
{
    // Create layout
    gfx::backend::vulkan::core::BindGroupLayoutEntry layoutEntry{};
    layoutEntry.binding = 0;
    layoutEntry.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutEntry.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo layoutInfo{};
    layoutInfo.entries = { layoutEntry };
    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), layoutInfo);

    // Create buffer
    gfx::backend::vulkan::core::BufferCreateInfo bufferInfo{};
    bufferInfo.size = 1024;
    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    gfx::backend::vulkan::core::Buffer buffer(device.get(), bufferInfo);

    // Create bind group
    gfx::backend::vulkan::core::BindGroupEntry entry{};
    entry.binding = 0;
    entry.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    entry.buffer = buffer.handle();
    entry.bufferOffset = 0;
    entry.bufferSize = 1024;

    gfx::backend::vulkan::core::BindGroupCreateInfo createInfo{};
    createInfo.layout = layout.handle();
    createInfo.entries = { entry };

    gfx::backend::vulkan::core::BindGroup bindGroup(device.get(), createInfo);

    EXPECT_NE(bindGroup.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanBindGroupTest, CreateWithPartialBuffer_CreatesSuccessfully)
{
    // Create layout
    gfx::backend::vulkan::core::BindGroupLayoutEntry layoutEntry{};
    layoutEntry.binding = 0;
    layoutEntry.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutEntry.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo layoutInfo{};
    layoutInfo.entries = { layoutEntry };
    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), layoutInfo);

    // Create buffer
    gfx::backend::vulkan::core::BufferCreateInfo bufferInfo{};
    bufferInfo.size = 1024;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    gfx::backend::vulkan::core::Buffer buffer(device.get(), bufferInfo);

    // Create bind group with partial buffer range
    gfx::backend::vulkan::core::BindGroupEntry entry{};
    entry.binding = 0;
    entry.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    entry.buffer = buffer.handle();
    entry.bufferOffset = 256;
    entry.bufferSize = 256;

    gfx::backend::vulkan::core::BindGroupCreateInfo createInfo{};
    createInfo.layout = layout.handle();
    createInfo.entries = { entry };

    gfx::backend::vulkan::core::BindGroup bindGroup(device.get(), createInfo);

    EXPECT_NE(bindGroup.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Texture and Sampler Tests
// ============================================================================

TEST_F(VulkanBindGroupTest, CreateWithSampler_CreatesSuccessfully)
{
    // Create layout
    gfx::backend::vulkan::core::BindGroupLayoutEntry layoutEntry{};
    layoutEntry.binding = 0;
    layoutEntry.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    layoutEntry.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo layoutInfo{};
    layoutInfo.entries = { layoutEntry };
    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), layoutInfo);

    // Create sampler
    gfx::backend::vulkan::core::SamplerCreateInfo samplerInfo{};
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.lodMinClamp = 0.0f;
    samplerInfo.lodMaxClamp = 1.0f;
    samplerInfo.maxAnisotropy = 1;
    samplerInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;
    gfx::backend::vulkan::core::Sampler sampler(device.get(), samplerInfo);

    // Create bind group
    gfx::backend::vulkan::core::BindGroupEntry entry{};
    entry.binding = 0;
    entry.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    entry.sampler = sampler.handle();

    gfx::backend::vulkan::core::BindGroupCreateInfo createInfo{};
    createInfo.layout = layout.handle();
    createInfo.entries = { entry };

    gfx::backend::vulkan::core::BindGroup bindGroup(device.get(), createInfo);

    EXPECT_NE(bindGroup.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanBindGroupTest, CreateWithSampledImage_CreatesSuccessfully)
{
    // Create layout
    gfx::backend::vulkan::core::BindGroupLayoutEntry layoutEntry{};
    layoutEntry.binding = 0;
    layoutEntry.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    layoutEntry.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo layoutInfo{};
    layoutInfo.entries = { layoutEntry };
    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), layoutInfo);

    // Create texture
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.size = { 256, 256, 1 };
    textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 1;
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.arrayLayers = 1;
    textureInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    // Create texture view
    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_UNDEFINED; // Use texture's format
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;
    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    // Create bind group
    gfx::backend::vulkan::core::BindGroupEntry entry{};
    entry.binding = 0;
    entry.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    entry.imageView = textureView.handle();
    entry.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    gfx::backend::vulkan::core::BindGroupCreateInfo createInfo{};
    createInfo.layout = layout.handle();
    createInfo.entries = { entry };

    gfx::backend::vulkan::core::BindGroup bindGroup(device.get(), createInfo);

    EXPECT_NE(bindGroup.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanBindGroupTest, CreateWithStorageImage_CreatesSuccessfully)
{
    // Create layout
    gfx::backend::vulkan::core::BindGroupLayoutEntry layoutEntry{};
    layoutEntry.binding = 0;
    layoutEntry.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    layoutEntry.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo layoutInfo{};
    layoutInfo.entries = { layoutEntry };
    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), layoutInfo);

    // Create texture
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.size = { 512, 512, 1 };
    textureInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 1;
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.arrayLayers = 1;
    textureInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    // Create texture view
    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_UNDEFINED;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;
    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    // Create bind group
    gfx::backend::vulkan::core::BindGroupEntry entry{};
    entry.binding = 0;
    entry.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    entry.imageView = textureView.handle();
    entry.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    gfx::backend::vulkan::core::BindGroupCreateInfo createInfo{};
    createInfo.layout = layout.handle();
    createInfo.entries = { entry };

    gfx::backend::vulkan::core::BindGroup bindGroup(device.get(), createInfo);

    EXPECT_NE(bindGroup.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanBindGroupTest, CreateWithCombinedImageSampler_CreatesSuccessfully)
{
    // Create layout
    gfx::backend::vulkan::core::BindGroupLayoutEntry layoutEntry{};
    layoutEntry.binding = 0;
    layoutEntry.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutEntry.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo layoutInfo{};
    layoutInfo.entries = { layoutEntry };
    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), layoutInfo);

    // Create texture
    gfx::backend::vulkan::core::TextureCreateInfo textureInfo{};
    textureInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureInfo.size = { 256, 256, 1 };
    textureInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    textureInfo.mipLevelCount = 1;
    textureInfo.imageType = VK_IMAGE_TYPE_2D;
    textureInfo.arrayLayers = 1;
    textureInfo.flags = 0;
    gfx::backend::vulkan::core::Texture texture(device.get(), textureInfo);

    // Create texture view
    gfx::backend::vulkan::core::TextureViewCreateInfo viewInfo{};
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_UNDEFINED;
    viewInfo.baseMipLevel = 0;
    viewInfo.mipLevelCount = 1;
    viewInfo.baseArrayLayer = 0;
    viewInfo.arrayLayerCount = 1;
    gfx::backend::vulkan::core::TextureView textureView(&texture, viewInfo);

    // Create sampler
    gfx::backend::vulkan::core::SamplerCreateInfo samplerInfo{};
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.lodMinClamp = 0.0f;
    samplerInfo.lodMaxClamp = 1.0f;
    samplerInfo.maxAnisotropy = 1;
    samplerInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;
    gfx::backend::vulkan::core::Sampler sampler(device.get(), samplerInfo);

    // Create bind group
    gfx::backend::vulkan::core::BindGroupEntry entry{};
    entry.binding = 0;
    entry.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    entry.imageView = textureView.handle();
    entry.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    entry.sampler = sampler.handle();

    gfx::backend::vulkan::core::BindGroupCreateInfo createInfo{};
    createInfo.layout = layout.handle();
    createInfo.entries = { entry };

    gfx::backend::vulkan::core::BindGroup bindGroup(device.get(), createInfo);

    EXPECT_NE(bindGroup.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Multiple Binding Tests
// ============================================================================

TEST_F(VulkanBindGroupTest, CreateWithMultipleBuffers_CreatesSuccessfully)
{
    // Create layout
    gfx::backend::vulkan::core::BindGroupLayoutEntry layoutEntry0{};
    layoutEntry0.binding = 0;
    layoutEntry0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutEntry0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry layoutEntry1{};
    layoutEntry1.binding = 1;
    layoutEntry1.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutEntry1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo layoutInfo{};
    layoutInfo.entries = { layoutEntry0, layoutEntry1 };
    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), layoutInfo);

    // Create buffers
    gfx::backend::vulkan::core::BufferCreateInfo bufferInfo0{};
    bufferInfo0.size = 256;
    bufferInfo0.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo0.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    gfx::backend::vulkan::core::Buffer buffer0(device.get(), bufferInfo0);

    gfx::backend::vulkan::core::BufferCreateInfo bufferInfo1{};
    bufferInfo1.size = 128;
    bufferInfo1.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo1.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    gfx::backend::vulkan::core::Buffer buffer1(device.get(), bufferInfo1);

    // Create bind group
    gfx::backend::vulkan::core::BindGroupEntry entry0{};
    entry0.binding = 0;
    entry0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    entry0.buffer = buffer0.handle();
    entry0.bufferOffset = 0;
    entry0.bufferSize = 256;

    gfx::backend::vulkan::core::BindGroupEntry entry1{};
    entry1.binding = 1;
    entry1.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    entry1.buffer = buffer1.handle();
    entry1.bufferOffset = 0;
    entry1.bufferSize = 128;

    gfx::backend::vulkan::core::BindGroupCreateInfo createInfo{};
    createInfo.layout = layout.handle();
    createInfo.entries = { entry0, entry1 };

    gfx::backend::vulkan::core::BindGroup bindGroup(device.get(), createInfo);

    EXPECT_NE(bindGroup.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanBindGroupTest, CreateComplexGraphicsBindGroup_CreatesSuccessfully)
{
    // Create layout for: MVP buffer + material buffer + 2 textures + sampler
    gfx::backend::vulkan::core::BindGroupLayoutEntry mvpEntry{};
    mvpEntry.binding = 0;
    mvpEntry.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    mvpEntry.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry materialEntry{};
    materialEntry.binding = 1;
    materialEntry.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    materialEntry.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry albedoEntry{};
    albedoEntry.binding = 2;
    albedoEntry.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    albedoEntry.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry normalEntry{};
    normalEntry.binding = 3;
    normalEntry.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    normalEntry.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry samplerEntry{};
    samplerEntry.binding = 4;
    samplerEntry.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    samplerEntry.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo layoutInfo{};
    layoutInfo.entries = { mvpEntry, materialEntry, albedoEntry, normalEntry, samplerEntry };
    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), layoutInfo);

    // Create MVP buffer
    gfx::backend::vulkan::core::BufferCreateInfo mvpBufferInfo{};
    mvpBufferInfo.size = 256;
    mvpBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    mvpBufferInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    gfx::backend::vulkan::core::Buffer mvpBuffer(device.get(), mvpBufferInfo);

    // Create material buffer
    gfx::backend::vulkan::core::BufferCreateInfo materialBufferInfo{};
    materialBufferInfo.size = 128;
    materialBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    materialBufferInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    gfx::backend::vulkan::core::Buffer materialBuffer(device.get(), materialBufferInfo);

    // Create albedo texture
    gfx::backend::vulkan::core::TextureCreateInfo albedoTexInfo{};
    albedoTexInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    albedoTexInfo.size = { 512, 512, 1 };
    albedoTexInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    albedoTexInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    albedoTexInfo.mipLevelCount = 1;
    albedoTexInfo.imageType = VK_IMAGE_TYPE_2D;
    albedoTexInfo.arrayLayers = 1;
    albedoTexInfo.flags = 0;
    gfx::backend::vulkan::core::Texture albedoTexture(device.get(), albedoTexInfo);

    gfx::backend::vulkan::core::TextureViewCreateInfo albedoViewInfo{};
    albedoViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    albedoViewInfo.format = VK_FORMAT_UNDEFINED;
    albedoViewInfo.baseMipLevel = 0;
    albedoViewInfo.mipLevelCount = 1;
    albedoViewInfo.baseArrayLayer = 0;
    albedoViewInfo.arrayLayerCount = 1;
    gfx::backend::vulkan::core::TextureView albedoView(&albedoTexture, albedoViewInfo);

    // Create normal texture
    gfx::backend::vulkan::core::TextureCreateInfo normalTexInfo{};
    normalTexInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    normalTexInfo.size = { 512, 512, 1 };
    normalTexInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    normalTexInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
    normalTexInfo.mipLevelCount = 1;
    normalTexInfo.imageType = VK_IMAGE_TYPE_2D;
    normalTexInfo.arrayLayers = 1;
    normalTexInfo.flags = 0;
    gfx::backend::vulkan::core::Texture normalTexture(device.get(), normalTexInfo);

    gfx::backend::vulkan::core::TextureViewCreateInfo normalViewInfo{};
    normalViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    normalViewInfo.format = VK_FORMAT_UNDEFINED;
    normalViewInfo.baseMipLevel = 0;
    normalViewInfo.mipLevelCount = 1;
    normalViewInfo.baseArrayLayer = 0;
    normalViewInfo.arrayLayerCount = 1;
    gfx::backend::vulkan::core::TextureView normalView(&normalTexture, normalViewInfo);

    // Create sampler
    gfx::backend::vulkan::core::SamplerCreateInfo samplerInfo{};
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.lodMinClamp = 0.0f;
    samplerInfo.lodMaxClamp = 1.0f;
    samplerInfo.maxAnisotropy = 1;
    samplerInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;
    gfx::backend::vulkan::core::Sampler sampler(device.get(), samplerInfo);

    // Create bind group
    gfx::backend::vulkan::core::BindGroupEntry bgEntry0{};
    bgEntry0.binding = 0;
    bgEntry0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bgEntry0.buffer = mvpBuffer.handle();
    bgEntry0.bufferOffset = 0;
    bgEntry0.bufferSize = 256;

    gfx::backend::vulkan::core::BindGroupEntry bgEntry1{};
    bgEntry1.binding = 1;
    bgEntry1.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bgEntry1.buffer = materialBuffer.handle();
    bgEntry1.bufferOffset = 0;
    bgEntry1.bufferSize = 128;

    gfx::backend::vulkan::core::BindGroupEntry bgEntry2{};
    bgEntry2.binding = 2;
    bgEntry2.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    bgEntry2.imageView = albedoView.handle();
    bgEntry2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    gfx::backend::vulkan::core::BindGroupEntry bgEntry3{};
    bgEntry3.binding = 3;
    bgEntry3.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    bgEntry3.imageView = normalView.handle();
    bgEntry3.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    gfx::backend::vulkan::core::BindGroupEntry bgEntry4{};
    bgEntry4.binding = 4;
    bgEntry4.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    bgEntry4.sampler = sampler.handle();

    gfx::backend::vulkan::core::BindGroupCreateInfo createInfo{};
    createInfo.layout = layout.handle();
    createInfo.entries = { bgEntry0, bgEntry1, bgEntry2, bgEntry3, bgEntry4 };

    gfx::backend::vulkan::core::BindGroup bindGroup(device.get(), createInfo);

    EXPECT_NE(bindGroup.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanBindGroupTest, CreateComplexComputeBindGroup_CreatesSuccessfully)
{
    // Create layout for: 2 input storage buffers + 1 output storage buffer + params uniform buffer
    gfx::backend::vulkan::core::BindGroupLayoutEntry input0Entry{};
    input0Entry.binding = 0;
    input0Entry.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    input0Entry.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry input1Entry{};
    input1Entry.binding = 1;
    input1Entry.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    input1Entry.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry outputEntry{};
    outputEntry.binding = 2;
    outputEntry.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    outputEntry.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutEntry paramsEntry{};
    paramsEntry.binding = 3;
    paramsEntry.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    paramsEntry.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    gfx::backend::vulkan::core::BindGroupLayoutCreateInfo layoutInfo{};
    layoutInfo.entries = { input0Entry, input1Entry, outputEntry, paramsEntry };
    gfx::backend::vulkan::core::BindGroupLayout layout(device.get(), layoutInfo);

    // Create buffers
    gfx::backend::vulkan::core::BufferCreateInfo storageBufferInfo{};
    storageBufferInfo.size = 4096;
    storageBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    storageBufferInfo.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    gfx::backend::vulkan::core::Buffer inputBuffer0(device.get(), storageBufferInfo);
    gfx::backend::vulkan::core::Buffer inputBuffer1(device.get(), storageBufferInfo);
    gfx::backend::vulkan::core::Buffer outputBuffer(device.get(), storageBufferInfo);

    gfx::backend::vulkan::core::BufferCreateInfo uniformBufferInfo{};
    uniformBufferInfo.size = 256;
    uniformBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    uniformBufferInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    gfx::backend::vulkan::core::Buffer paramsBuffer(device.get(), uniformBufferInfo);

    // Create bind group
    gfx::backend::vulkan::core::BindGroupEntry bgEntry0{};
    bgEntry0.binding = 0;
    bgEntry0.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bgEntry0.buffer = inputBuffer0.handle();
    bgEntry0.bufferOffset = 0;
    bgEntry0.bufferSize = 4096;

    gfx::backend::vulkan::core::BindGroupEntry bgEntry1{};
    bgEntry1.binding = 1;
    bgEntry1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bgEntry1.buffer = inputBuffer1.handle();
    bgEntry1.bufferOffset = 0;
    bgEntry1.bufferSize = 4096;

    gfx::backend::vulkan::core::BindGroupEntry bgEntry2{};
    bgEntry2.binding = 2;
    bgEntry2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bgEntry2.buffer = outputBuffer.handle();
    bgEntry2.bufferOffset = 0;
    bgEntry2.bufferSize = 4096;

    gfx::backend::vulkan::core::BindGroupEntry bgEntry3{};
    bgEntry3.binding = 3;
    bgEntry3.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bgEntry3.buffer = paramsBuffer.handle();
    bgEntry3.bufferOffset = 0;
    bgEntry3.bufferSize = 256;

    gfx::backend::vulkan::core::BindGroupCreateInfo createInfo{};
    createInfo.layout = layout.handle();
    createInfo.entries = { bgEntry0, bgEntry1, bgEntry2, bgEntry3 };

    gfx::backend::vulkan::core::BindGroup bindGroup(device.get(), createInfo);

    EXPECT_NE(bindGroup.handle(), VK_NULL_HANDLE);
}

} // namespace
