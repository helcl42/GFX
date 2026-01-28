#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

#include <memory>

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

class GfxCppShaderTest : public testing::TestWithParam<gfx::Backend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        try {
            instance = gfx::createInstance({ .backend = backend, .enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG } });
            adapter = instance->requestAdapter({ .adapterIndex = 0 });
            device = adapter->createDevice({ .label = "Test Device" });
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up: " << e.what();
        }
    }

    void TearDown() override
    {
        device.reset();
        adapter.reset();
        instance.reset();
    }

    gfx::Backend backend = gfx::Backend::Vulkan;
    std::shared_ptr<gfx::Instance> instance;
    std::shared_ptr<gfx::Adapter> adapter;
    std::shared_ptr<gfx::Device> device;
};

// Simple WGSL compute shader for testing
static const char* wgslComputeShader = R"(
@group(0) @binding(0) var<storage, read_write> data: array<f32>;

@compute @workgroup_size(64)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let index = global_id.x;
    data[index] = data[index] * 2.0;
}
)";

// Simple WGSL vertex shader for testing
static const char* wgslVertexShader = R"(
struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec4<f32>,
}

@vertex
fn main(@location(0) position: vec3<f32>, @location(1) color: vec3<f32>) -> VertexOutput {
    var output: VertexOutput;
    output.position = vec4<f32>(position, 1.0);
    output.color = vec4<f32>(color, 1.0);
    return output;
}
)";

// Simple WGSL fragment shader for testing
static const char* wgslFragmentShader = R"(
@fragment
fn main(@location(0) color: vec4<f32>) -> @location(0) vec4<f32> {
    return color;
}
)";

// SPIR-V compute shader (simple passthrough)
static const uint32_t spirvComputeShader[] = {
    0x07230203, 0x00010000, 0x000d000a, 0x00000020, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0006000f, 0x00000005, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000d, 0x00060010, 0x00000004,
    0x00000011, 0x00000040, 0x00000001, 0x00000001, 0x00030003, 0x00000002, 0x000001c2, 0x00040005,
    0x00000004, 0x6e69616d, 0x00000000, 0x00080005, 0x0000000d, 0x5f6c6769, 0x61626f6c, 0x766e496c,
    0x7461636f, 0x496e6f69, 0x00000044, 0x00040005, 0x00000012, 0x61746164, 0x00000000, 0x00050005,
    0x00000019, 0x61746164, 0x7275745f, 0x0000006e, 0x00040047, 0x0000000d, 0x0000000b, 0x0000001c,
    0x00050048, 0x00000010, 0x00000000, 0x00000023, 0x00000000, 0x00030047, 0x00000010, 0x00000003,
    0x00040047, 0x00000012, 0x00000022, 0x00000000, 0x00040047, 0x00000012, 0x00000021, 0x00000000,
    0x00040047, 0x00000019, 0x00000022, 0x00000000, 0x00040047, 0x00000019, 0x00000021, 0x00000000,
    0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00040015, 0x00000006, 0x00000020,
    0x00000000, 0x00040017, 0x00000007, 0x00000006, 0x00000003, 0x00040020, 0x00000008, 0x00000001,
    0x00000007, 0x0004003b, 0x00000008, 0x0000000d, 0x00000001, 0x00040020, 0x0000000e, 0x00000001,
    0x00000006, 0x00030016, 0x0000000f, 0x00000020, 0x0003001d, 0x00000010, 0x0000000f, 0x0003001e,
    0x00000011, 0x00000010, 0x00040020, 0x00000012, 0x0000000c, 0x00000011, 0x0004003b, 0x00000012,
    0x00000013, 0x0000000c, 0x00040015, 0x00000014, 0x00000020, 0x00000001, 0x0004002b, 0x00000014,
    0x00000015, 0x00000000, 0x00040020, 0x00000017, 0x0000000c, 0x0000000f, 0x0004002b, 0x0000000f,
    0x0000001b, 0x40000000, 0x0003001d, 0x0000001d, 0x0000000f, 0x0003001e, 0x0000001e, 0x0000001d,
    0x00040020, 0x0000001f, 0x0000000c, 0x0000001e, 0x0004003b, 0x0000001f, 0x00000019, 0x0000000c,
    0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x00050041,
    0x0000000e, 0x00000016, 0x0000000d, 0x00000015, 0x0004003d, 0x00000006, 0x00000009, 0x00000016,
    0x00050041, 0x00000017, 0x00000018, 0x00000013, 0x00000009, 0x0004003d, 0x0000000f, 0x0000000a,
    0x00000018, 0x00050085, 0x0000000f, 0x0000001c, 0x0000000a, 0x0000001b, 0x00050041, 0x00000017,
    0x0000001a, 0x00000019, 0x00000009, 0x0003003e, 0x0000001a, 0x0000001c, 0x000100fd, 0x00010038
};

TEST_P(GfxCppShaderTest, CreateComputeShaderWGSL)
{
    if (GetParam() == gfx::Backend::Vulkan) {
        GTEST_SKIP() << "WGSL is WebGPU only";
    }

    auto shader = device->createShader({ .label = "WGSL Compute Shader",
        .sourceType = gfx::ShaderSourceType::WGSL,
        .code = wgslComputeShader,
        .entryPoint = "main" });

    EXPECT_NE(shader, nullptr);
}

TEST_P(GfxCppShaderTest, CreateComputeShaderSPIRV)
{
    std::string spirvCode(reinterpret_cast<const char*>(spirvComputeShader), sizeof(spirvComputeShader));

    auto shader = device->createShader({ .label = "SPIR-V Compute Shader",
        .sourceType = gfx::ShaderSourceType::SPIRV,
        .code = spirvCode,
        .entryPoint = "main" });

    EXPECT_NE(shader, nullptr);
}

TEST_P(GfxCppShaderTest, CreateVertexShaderWGSL)
{
    if (GetParam() == gfx::Backend::Vulkan) {
        GTEST_SKIP() << "WGSL is WebGPU only";
    }

    auto shader = device->createShader({ .label = "WGSL Vertex Shader",
        .sourceType = gfx::ShaderSourceType::WGSL,
        .code = wgslVertexShader,
        .entryPoint = "main" });

    EXPECT_NE(shader, nullptr);
}

TEST_P(GfxCppShaderTest, CreateFragmentShaderWGSL)
{
    if (GetParam() == gfx::Backend::Vulkan) {
        GTEST_SKIP() << "WGSL is WebGPU only";
    }

    auto shader = device->createShader({ .label = "WGSL Fragment Shader",
        .sourceType = gfx::ShaderSourceType::WGSL,
        .code = wgslFragmentShader,
        .entryPoint = "main" });

    EXPECT_NE(shader, nullptr);
}

TEST_P(GfxCppShaderTest, CreateMultipleShaders)
{
    std::string spirvCode(reinterpret_cast<const char*>(spirvComputeShader), sizeof(spirvComputeShader));

    auto shader1 = device->createShader({ .label = "Compute Shader 1",
        .sourceType = gfx::ShaderSourceType::SPIRV,
        .code = spirvCode,
        .entryPoint = "main" });

    auto shader2 = device->createShader({ .label = "Compute Shader 2",
        .sourceType = gfx::ShaderSourceType::SPIRV,
        .code = spirvCode,
        .entryPoint = "main" });

    auto shader3 = device->createShader({ .label = "Compute Shader 3",
        .sourceType = gfx::ShaderSourceType::SPIRV,
        .code = spirvCode,
        .entryPoint = "main" });

    EXPECT_NE(shader1, nullptr);
    EXPECT_NE(shader2, nullptr);
    EXPECT_NE(shader3, nullptr);
}

TEST_P(GfxCppShaderTest, CreateShaderWithDefaultDescriptor)
{
    std::string spirvCode(reinterpret_cast<const char*>(spirvComputeShader), sizeof(spirvComputeShader));

    // Default descriptor uses SPIRV source type and "main" entry point
    auto shader = device->createShader({ .code = spirvCode });

    EXPECT_NE(shader, nullptr);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppShaderTest,
    testing::Values(gfx::Backend::Vulkan, gfx::Backend::WebGPU),
    [](const testing::TestParamInfo<gfx::Backend>& info) {
        return info.param == gfx::Backend::Vulkan ? "Vulkan" : "WebGPU";
    });
