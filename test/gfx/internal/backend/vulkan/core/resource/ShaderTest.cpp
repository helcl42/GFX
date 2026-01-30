#include <backend/vulkan/core/resource/Shader.h>
#include <backend/vulkan/core/system/Adapter.h>
#include <backend/vulkan/core/system/Device.h>
#include <backend/vulkan/core/system/Instance.h>


#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

// Test Vulkan core Shader class
// These tests verify the internal shader module implementation, not the public API

namespace {

// ============================================================================
// Minimal valid SPIR-V shader modules for testing
// ============================================================================

// Minimal vertex shader SPIR-V (empty main function)
static const uint32_t MINIMAL_VERTEX_SPIRV[] = {
    0x07230203, 0x00010000, 0x00080001, 0x0000000d, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0005000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x00060010, 0x00000004, 0x00000011,
    0x00000001, 0x00000001, 0x00000001, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004,
    0x6e69616d, 0x00000000, 0x00050048, 0x00000009, 0x00000000, 0x0000000b, 0x00000000, 0x00030047,
    0x00000009, 0x00000002, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00050015,
    0x00000006, 0x00000020, 0x00000000, 0x00000000, 0x00040017, 0x00000007, 0x00000006, 0x00000004,
    0x0004001e, 0x00000009, 0x00000007, 0x00000007, 0x00040020, 0x0000000a, 0x00000003, 0x00000009,
    0x0004003b, 0x0000000a, 0x0000000b, 0x00000003, 0x00050036, 0x00000002, 0x00000004, 0x00000000,
    0x00000003, 0x000200f8, 0x0000000c, 0x000100fd, 0x00010038
};

// Minimal fragment shader SPIR-V (empty main function)
static const uint32_t MINIMAL_FRAGMENT_SPIRV[] = {
    0x07230203, 0x00010000, 0x00080001, 0x00000008, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0005000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00060010, 0x00000004, 0x00000011,
    0x00000007, 0x00000001, 0x00000001, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004,
    0x6e69616d, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00050036,
    0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000007, 0x000100fd, 0x00010038
};

// Minimal compute shader SPIR-V (empty main function)
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

class VulkanShaderTest : public testing::Test {
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

TEST_F(VulkanShaderTest, CreateVertexShader_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::ShaderCreateInfo createInfo{};
    createInfo.code = MINIMAL_VERTEX_SPIRV;
    createInfo.codeSize = sizeof(MINIMAL_VERTEX_SPIRV);
    createInfo.entryPoint = "main";

    gfx::backend::vulkan::core::Shader shader(device.get(), createInfo);

    EXPECT_NE(shader.handle(), VK_NULL_HANDLE);
    EXPECT_STREQ(shader.entryPoint(), "main");
}

TEST_F(VulkanShaderTest, CreateFragmentShader_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::ShaderCreateInfo createInfo{};
    createInfo.code = MINIMAL_FRAGMENT_SPIRV;
    createInfo.codeSize = sizeof(MINIMAL_FRAGMENT_SPIRV);
    createInfo.entryPoint = "main";

    gfx::backend::vulkan::core::Shader shader(device.get(), createInfo);

    EXPECT_NE(shader.handle(), VK_NULL_HANDLE);
    EXPECT_STREQ(shader.entryPoint(), "main");
}

TEST_F(VulkanShaderTest, CreateComputeShader_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::ShaderCreateInfo createInfo{};
    createInfo.code = MINIMAL_COMPUTE_SPIRV;
    createInfo.codeSize = sizeof(MINIMAL_COMPUTE_SPIRV);
    createInfo.entryPoint = "main";

    gfx::backend::vulkan::core::Shader shader(device.get(), createInfo);

    EXPECT_NE(shader.handle(), VK_NULL_HANDLE);
    EXPECT_STREQ(shader.entryPoint(), "main");
}

TEST_F(VulkanShaderTest, CreateShaderNullEntryPoint_DefaultsToMain)
{
    gfx::backend::vulkan::core::ShaderCreateInfo createInfo{};
    createInfo.code = MINIMAL_VERTEX_SPIRV;
    createInfo.codeSize = sizeof(MINIMAL_VERTEX_SPIRV);
    createInfo.entryPoint = nullptr;

    gfx::backend::vulkan::core::Shader shader(device.get(), createInfo);

    EXPECT_NE(shader.handle(), VK_NULL_HANDLE);
    EXPECT_STREQ(shader.entryPoint(), "main");
}

TEST_F(VulkanShaderTest, CreateShaderCustomEntryPoint_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::ShaderCreateInfo createInfo{};
    createInfo.code = MINIMAL_VERTEX_SPIRV;
    createInfo.codeSize = sizeof(MINIMAL_VERTEX_SPIRV);
    createInfo.entryPoint = "customMain";

    gfx::backend::vulkan::core::Shader shader(device.get(), createInfo);

    EXPECT_NE(shader.handle(), VK_NULL_HANDLE);
    EXPECT_STREQ(shader.entryPoint(), "customMain");
}

// ============================================================================
// Multiple Shader Creation Tests
// ============================================================================

TEST_F(VulkanShaderTest, CreateMultipleVertexShaders_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::ShaderCreateInfo createInfo1{};
    createInfo1.code = MINIMAL_VERTEX_SPIRV;
    createInfo1.codeSize = sizeof(MINIMAL_VERTEX_SPIRV);
    createInfo1.entryPoint = "main";

    gfx::backend::vulkan::core::ShaderCreateInfo createInfo2{};
    createInfo2.code = MINIMAL_VERTEX_SPIRV;
    createInfo2.codeSize = sizeof(MINIMAL_VERTEX_SPIRV);
    createInfo2.entryPoint = "main";

    gfx::backend::vulkan::core::Shader shader1(device.get(), createInfo1);
    gfx::backend::vulkan::core::Shader shader2(device.get(), createInfo2);

    EXPECT_NE(shader1.handle(), VK_NULL_HANDLE);
    EXPECT_NE(shader2.handle(), VK_NULL_HANDLE);
    EXPECT_NE(shader1.handle(), shader2.handle());
}

TEST_F(VulkanShaderTest, CreateAllShaderStages_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::ShaderCreateInfo vertexInfo{};
    vertexInfo.code = MINIMAL_VERTEX_SPIRV;
    vertexInfo.codeSize = sizeof(MINIMAL_VERTEX_SPIRV);
    vertexInfo.entryPoint = "main";

    gfx::backend::vulkan::core::ShaderCreateInfo fragmentInfo{};
    fragmentInfo.code = MINIMAL_FRAGMENT_SPIRV;
    fragmentInfo.codeSize = sizeof(MINIMAL_FRAGMENT_SPIRV);
    fragmentInfo.entryPoint = "main";

    gfx::backend::vulkan::core::ShaderCreateInfo computeInfo{};
    computeInfo.code = MINIMAL_COMPUTE_SPIRV;
    computeInfo.codeSize = sizeof(MINIMAL_COMPUTE_SPIRV);
    computeInfo.entryPoint = "main";

    gfx::backend::vulkan::core::Shader vertexShader(device.get(), vertexInfo);
    gfx::backend::vulkan::core::Shader fragmentShader(device.get(), fragmentInfo);
    gfx::backend::vulkan::core::Shader computeShader(device.get(), computeInfo);

    EXPECT_NE(vertexShader.handle(), VK_NULL_HANDLE);
    EXPECT_NE(fragmentShader.handle(), VK_NULL_HANDLE);
    EXPECT_NE(computeShader.handle(), VK_NULL_HANDLE);
}

// ============================================================================
// Entry Point Tests
// ============================================================================

TEST_F(VulkanShaderTest, CreateShaderLongEntryPoint_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::ShaderCreateInfo createInfo{};
    createInfo.code = MINIMAL_VERTEX_SPIRV;
    createInfo.codeSize = sizeof(MINIMAL_VERTEX_SPIRV);
    createInfo.entryPoint = "veryLongEntryPointNameForTesting123";

    gfx::backend::vulkan::core::Shader shader(device.get(), createInfo);

    EXPECT_NE(shader.handle(), VK_NULL_HANDLE);
    EXPECT_STREQ(shader.entryPoint(), "veryLongEntryPointNameForTesting123");
}

TEST_F(VulkanShaderTest, CreateDifferentEntryPoints_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::ShaderCreateInfo createInfo1{};
    createInfo1.code = MINIMAL_VERTEX_SPIRV;
    createInfo1.codeSize = sizeof(MINIMAL_VERTEX_SPIRV);
    createInfo1.entryPoint = "vertex_main";

    gfx::backend::vulkan::core::ShaderCreateInfo createInfo2{};
    createInfo2.code = MINIMAL_VERTEX_SPIRV;
    createInfo2.codeSize = sizeof(MINIMAL_VERTEX_SPIRV);
    createInfo2.entryPoint = "vertex_alternative";

    gfx::backend::vulkan::core::Shader shader1(device.get(), createInfo1);
    gfx::backend::vulkan::core::Shader shader2(device.get(), createInfo2);

    EXPECT_STREQ(shader1.entryPoint(), "vertex_main");
    EXPECT_STREQ(shader2.entryPoint(), "vertex_alternative");
}

// ============================================================================
// Handle Uniqueness Tests
// ============================================================================

TEST_F(VulkanShaderTest, CreateShaderGetHandle_ReturnsValidHandle)
{
    gfx::backend::vulkan::core::ShaderCreateInfo createInfo{};
    createInfo.code = MINIMAL_VERTEX_SPIRV;
    createInfo.codeSize = sizeof(MINIMAL_VERTEX_SPIRV);
    createInfo.entryPoint = "main";

    gfx::backend::vulkan::core::Shader shader(device.get(), createInfo);

    VkShaderModule handle = shader.handle();
    EXPECT_NE(handle, VK_NULL_HANDLE);

    // Multiple calls should return same handle
    EXPECT_EQ(shader.handle(), handle);
}

TEST_F(VulkanShaderTest, CreateMultipleShaders_HaveUniqueHandles)
{
    gfx::backend::vulkan::core::ShaderCreateInfo createInfo{};
    createInfo.code = MINIMAL_VERTEX_SPIRV;
    createInfo.codeSize = sizeof(MINIMAL_VERTEX_SPIRV);
    createInfo.entryPoint = "main";

    gfx::backend::vulkan::core::Shader shader1(device.get(), createInfo);
    gfx::backend::vulkan::core::Shader shader2(device.get(), createInfo);
    gfx::backend::vulkan::core::Shader shader3(device.get(), createInfo);

    EXPECT_NE(shader1.handle(), shader2.handle());
    EXPECT_NE(shader2.handle(), shader3.handle());
    EXPECT_NE(shader1.handle(), shader3.handle());
}

// ============================================================================
// SPIR-V Size Tests
// ============================================================================

TEST_F(VulkanShaderTest, CreateSmallSPIRV_CreatesSuccessfully)
{
    gfx::backend::vulkan::core::ShaderCreateInfo createInfo{};
    createInfo.code = MINIMAL_VERTEX_SPIRV;
    createInfo.codeSize = sizeof(MINIMAL_VERTEX_SPIRV);
    createInfo.entryPoint = "main";

    gfx::backend::vulkan::core::Shader shader(device.get(), createInfo);

    EXPECT_NE(shader.handle(), VK_NULL_HANDLE);
}

TEST_F(VulkanShaderTest, CreateLargeSPIRV_CreatesSuccessfully)
{
    // Create a larger SPIR-V by duplicating the minimal shader multiple times
    std::vector<uint32_t> largeSPIRV;
    largeSPIRV.insert(largeSPIRV.end(), MINIMAL_VERTEX_SPIRV,
        MINIMAL_VERTEX_SPIRV + (sizeof(MINIMAL_VERTEX_SPIRV) / sizeof(uint32_t)));

    // Pad with NOPs to make it larger (OpNop = 0x00010000)
    for (int i = 0; i < 1000; ++i) {
        largeSPIRV.push_back(0x00010000);
    }

    gfx::backend::vulkan::core::ShaderCreateInfo createInfo{};
    createInfo.code = largeSPIRV.data();
    createInfo.codeSize = largeSPIRV.size() * sizeof(uint32_t);
    createInfo.entryPoint = "main";

    gfx::backend::vulkan::core::Shader shader(device.get(), createInfo);

    EXPECT_NE(shader.handle(), VK_NULL_HANDLE);
}

} // namespace
