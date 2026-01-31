#include <backend/webgpu/core/resource/Shader.h>
#include <backend/webgpu/core/system/Device.h>
#include <backend/webgpu/core/system/Instance.h>

#include <gtest/gtest.h>

#include <cstring>
#include <memory>
#include <string>

namespace {

// ============================================================================
// Test Shaders - WGSL
// ============================================================================

// Minimal WGSL compute shader
static const char* MINIMAL_COMPUTE_WGSL = R"(
@compute @workgroup_size(1)
fn main() {
}
)";

// Minimal WGSL vertex shader
static const char* MINIMAL_VERTEX_WGSL = R"(
@vertex
fn main(@builtin(vertex_index) idx: u32) -> @builtin(position) vec4<f32> {
    return vec4<f32>(0.0, 0.0, 0.0, 1.0);
}
)";

// Minimal WGSL fragment shader
static const char* MINIMAL_FRAGMENT_WGSL = R"(
@fragment
fn main() -> @location(0) vec4<f32> {
    return vec4<f32>(1.0, 0.0, 0.0, 1.0);
}
)";

// ============================================================================
// Test Shaders - SPIRV
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

class WebGPUShaderTest : public testing::Test {
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

TEST_F(WebGPUShaderTest, CreateShader_WithComputeWGSL)
{
    gfx::backend::webgpu::core::ShaderCreateInfo createInfo{};
    createInfo.code = MINIMAL_COMPUTE_WGSL;
    createInfo.codeSize = std::strlen(MINIMAL_COMPUTE_WGSL);
    createInfo.entryPoint = "main";

    auto shader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), createInfo);

    EXPECT_NE(shader->handle(), nullptr);
}

TEST_F(WebGPUShaderTest, Handle_ReturnsValidWGPUShaderModule)
{
    gfx::backend::webgpu::core::ShaderCreateInfo createInfo{};
    createInfo.code = MINIMAL_VERTEX_WGSL;
    createInfo.codeSize = std::strlen(MINIMAL_VERTEX_WGSL);
    createInfo.entryPoint = "main";

    auto shader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), createInfo);

    WGPUShaderModule handle = shader->handle();
    EXPECT_NE(handle, nullptr);
}

TEST_F(WebGPUShaderTest, CreateShader_WithVertexWGSL)
{
    gfx::backend::webgpu::core::ShaderCreateInfo createInfo{};
    createInfo.code = MINIMAL_VERTEX_WGSL;
    createInfo.codeSize = std::strlen(MINIMAL_VERTEX_WGSL);
    createInfo.entryPoint = "main";

    auto shader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), createInfo);

    EXPECT_NE(shader->handle(), nullptr);
}

TEST_F(WebGPUShaderTest, CreateShader_WithFragmentWGSL)
{
    gfx::backend::webgpu::core::ShaderCreateInfo createInfo{};
    createInfo.code = MINIMAL_FRAGMENT_WGSL;
    createInfo.codeSize = std::strlen(MINIMAL_FRAGMENT_WGSL);
    createInfo.entryPoint = "main";

    auto shader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), createInfo);

    EXPECT_NE(shader->handle(), nullptr);
}

TEST_F(WebGPUShaderTest, MultipleShaders_CanCoexist)
{
    gfx::backend::webgpu::core::ShaderCreateInfo computeInfo{};
    computeInfo.code = MINIMAL_COMPUTE_WGSL;
    computeInfo.codeSize = std::strlen(MINIMAL_COMPUTE_WGSL);
    computeInfo.entryPoint = "main";

    gfx::backend::webgpu::core::ShaderCreateInfo vertexInfo{};
    vertexInfo.code = MINIMAL_VERTEX_WGSL;
    vertexInfo.codeSize = std::strlen(MINIMAL_VERTEX_WGSL);
    vertexInfo.entryPoint = "main";

    auto computeShader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), computeInfo);
    auto vertexShader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), vertexInfo);

    EXPECT_NE(computeShader->handle(), nullptr);
    EXPECT_NE(vertexShader->handle(), nullptr);
    EXPECT_NE(computeShader->handle(), vertexShader->handle());
}

TEST_F(WebGPUShaderTest, Destructor_CleansUpResources)
{
    {
        gfx::backend::webgpu::core::ShaderCreateInfo createInfo{};
        createInfo.code = MINIMAL_COMPUTE_WGSL;
        createInfo.codeSize = std::strlen(MINIMAL_COMPUTE_WGSL);
        createInfo.entryPoint = "main";

        auto shader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), createInfo);
        EXPECT_NE(shader->handle(), nullptr);
    }

    // If we reach here without crashing, cleanup succeeded
    SUCCEED();
}

// ============================================================================
// SPIRV Shader Tests
// ============================================================================

TEST_F(WebGPUShaderTest, CreateShader_WithVertexSPIRV)
{
    gfx::backend::webgpu::core::ShaderCreateInfo createInfo{};
    createInfo.sourceType = gfx::backend::webgpu::core::ShaderSourceType::SPIRV;
    createInfo.code = reinterpret_cast<const char*>(MINIMAL_VERTEX_SPIRV);
    createInfo.codeSize = sizeof(MINIMAL_VERTEX_SPIRV);
    createInfo.entryPoint = "main";

    auto shader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), createInfo);

    EXPECT_NE(shader->handle(), nullptr);
}

TEST_F(WebGPUShaderTest, CreateShader_WithFragmentSPIRV)
{
    gfx::backend::webgpu::core::ShaderCreateInfo createInfo{};
    createInfo.sourceType = gfx::backend::webgpu::core::ShaderSourceType::SPIRV;
    createInfo.code = reinterpret_cast<const char*>(MINIMAL_FRAGMENT_SPIRV);
    createInfo.codeSize = sizeof(MINIMAL_FRAGMENT_SPIRV);
    createInfo.entryPoint = "main";

    auto shader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), createInfo);

    EXPECT_NE(shader->handle(), nullptr);
}

TEST_F(WebGPUShaderTest, CreateShader_WithComputeSPIRV)
{
    gfx::backend::webgpu::core::ShaderCreateInfo createInfo{};
    createInfo.sourceType = gfx::backend::webgpu::core::ShaderSourceType::SPIRV;
    createInfo.code = reinterpret_cast<const char*>(MINIMAL_COMPUTE_SPIRV);
    createInfo.codeSize = sizeof(MINIMAL_COMPUTE_SPIRV);
    createInfo.entryPoint = "main";

    auto shader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), createInfo);

    EXPECT_NE(shader->handle(), nullptr);
}

TEST_F(WebGPUShaderTest, SPIRV_Handle_ReturnsValidWGPUShaderModule)
{
    gfx::backend::webgpu::core::ShaderCreateInfo createInfo{};
    createInfo.sourceType = gfx::backend::webgpu::core::ShaderSourceType::SPIRV;
    createInfo.code = reinterpret_cast<const char*>(MINIMAL_VERTEX_SPIRV);
    createInfo.codeSize = sizeof(MINIMAL_VERTEX_SPIRV);
    createInfo.entryPoint = "main";

    auto shader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), createInfo);

    WGPUShaderModule handle = shader->handle();
    EXPECT_NE(handle, nullptr);
}

TEST_F(WebGPUShaderTest, MixedShaderSources_WGSLAndSPIRV_CanCoexist)
{
    gfx::backend::webgpu::core::ShaderCreateInfo wgslInfo{};
    wgslInfo.sourceType = gfx::backend::webgpu::core::ShaderSourceType::WGSL;
    wgslInfo.code = MINIMAL_COMPUTE_WGSL;
    wgslInfo.codeSize = std::strlen(MINIMAL_COMPUTE_WGSL);
    wgslInfo.entryPoint = "main";

    gfx::backend::webgpu::core::ShaderCreateInfo spirvInfo{};
    spirvInfo.sourceType = gfx::backend::webgpu::core::ShaderSourceType::SPIRV;
    spirvInfo.code = reinterpret_cast<const char*>(MINIMAL_COMPUTE_SPIRV);
    spirvInfo.codeSize = sizeof(MINIMAL_COMPUTE_SPIRV);
    spirvInfo.entryPoint = "main";

    auto wgslShader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), wgslInfo);
    auto spirvShader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), spirvInfo);

    EXPECT_NE(wgslShader->handle(), nullptr);
    EXPECT_NE(spirvShader->handle(), nullptr);
    EXPECT_NE(wgslShader->handle(), spirvShader->handle());
}

} // anonymous namespace
