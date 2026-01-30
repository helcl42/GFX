#include <backend/vulkan/core/resource/Sampler.h>
#include <backend/vulkan/core/system/Adapter.h>
#include <backend/vulkan/core/system/Device.h>
#include <backend/vulkan/core/system/Instance.h>

#include <gtest/gtest.h>

// Test Vulkan core Sampler class
// These tests verify the internal sampler implementation, not the public API

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class VulkanSamplerTest : public testing::Test {
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

TEST_F(VulkanSamplerTest, CreateBasicSampler_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 1.0f;
    createInfo.maxAnisotropy = 1;
    createInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanSamplerTest, CreateNearestSampler_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.magFilter = VK_FILTER_NEAREST;
    createInfo.minFilter = VK_FILTER_NEAREST;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 1.0f;
    createInfo.maxAnisotropy = 1;
    createInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Address Mode Tests
// ============================================================================

TEST_F(VulkanSamplerTest, CreateRepeatAddressMode_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 1.0f;
    createInfo.maxAnisotropy = 1;
    createInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanSamplerTest, CreateMirroredRepeatAddressMode_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 1.0f;
    createInfo.maxAnisotropy = 1;
    createInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanSamplerTest, CreateClampToEdgeAddressMode_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 1.0f;
    createInfo.maxAnisotropy = 1;
    createInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanSamplerTest, CreateClampToBorderAddressMode_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 1.0f;
    createInfo.maxAnisotropy = 1;
    createInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanSamplerTest, CreateMixedAddressModes_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 1.0f;
    createInfo.maxAnisotropy = 1;
    createInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Filter Tests
// ============================================================================

TEST_F(VulkanSamplerTest, CreateLinearFilter_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 1.0f;
    createInfo.maxAnisotropy = 1;
    createInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanSamplerTest, CreateNearestFilter_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.magFilter = VK_FILTER_NEAREST;
    createInfo.minFilter = VK_FILTER_NEAREST;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 1.0f;
    createInfo.maxAnisotropy = 1;
    createInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanSamplerTest, CreateMixedFilters_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_NEAREST;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 1.0f;
    createInfo.maxAnisotropy = 1;
    createInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Mipmap Tests
// ============================================================================

TEST_F(VulkanSamplerTest, CreateLinearMipmapMode_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 10.0f;
    createInfo.maxAnisotropy = 1;
    createInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanSamplerTest, CreateNearestMipmapMode_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 10.0f;
    createInfo.maxAnisotropy = 1;
    createInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanSamplerTest, CreateCustomLodRange_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.lodMinClamp = 2.0f;
    createInfo.lodMaxClamp = 8.0f;
    createInfo.maxAnisotropy = 1;
    createInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Anisotropic Filtering Tests
// ============================================================================

TEST_F(VulkanSamplerTest, CreateAnisotropic2x_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 10.0f;
    createInfo.maxAnisotropy = 2;
    createInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanSamplerTest, CreateAnisotropic4x_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 10.0f;
    createInfo.maxAnisotropy = 4;
    createInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanSamplerTest, CreateAnisotropic8x_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 10.0f;
    createInfo.maxAnisotropy = 8;
    createInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanSamplerTest, CreateAnisotropic16x_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 10.0f;
    createInfo.maxAnisotropy = 16;
    createInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Comparison Sampler Tests
// ============================================================================

TEST_F(VulkanSamplerTest, CreateComparisonLess_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 1.0f;
    createInfo.maxAnisotropy = 1;
    createInfo.compareOp = VK_COMPARE_OP_LESS;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanSamplerTest, CreateComparisonLessOrEqual_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 1.0f;
    createInfo.maxAnisotropy = 1;
    createInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Common Use Case Tests
// ============================================================================

TEST_F(VulkanSamplerTest, CreatePixelArtSampler_CreatesSuccessfully)
{
    // Nearest filtering, clamp to edge
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.magFilter = VK_FILTER_NEAREST;
    createInfo.minFilter = VK_FILTER_NEAREST;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 0.0f;
    createInfo.maxAnisotropy = 1;
    createInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanSamplerTest, CreateSmoothTextureSampler_CreatesSuccessfully)
{
    // Trilinear filtering with anisotropy
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 16.0f;
    createInfo.maxAnisotropy = 16;
    createInfo.compareOp = VK_COMPARE_OP_MAX_ENUM;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanSamplerTest, CreateShadowMapSampler_CreatesSuccessfully)
{
    // Depth comparison sampler
    gfx::backend::vulkan::core::SamplerCreateInfo createInfo{};
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    createInfo.lodMinClamp = 0.0f;
    createInfo.lodMaxClamp = 1.0f;
    createInfo.maxAnisotropy = 1;
    createInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    gfx::backend::vulkan::core::Sampler sampler(device.get(), createInfo);

    EXPECT_NE(sampler.handle(), VK_NULL_HANDLE);
}

} // namespace
