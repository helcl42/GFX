#include <gfx/gfx.h>

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

class GfxShaderTest : public testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        if (gfxLoadBackend(backend) != GFX_RESULT_SUCCESS) {
            GTEST_SKIP() << "Backend not available";
        }

        GfxInstanceDescriptor instDesc = {};
        instDesc.backend = backend;
        const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
        instDesc.enabledExtensions = extensions;
        instDesc.enabledExtensionCount = 1;

        if (gfxCreateInstance(&instDesc, &instance) != GFX_RESULT_SUCCESS) {
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to create instance";
        }

        GfxAdapterDescriptor adapterDesc = {};
        adapterDesc.adapterIndex = 0;

        if (gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter) != GFX_RESULT_SUCCESS) {
            gfxInstanceDestroy(instance);
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to request adapter";
        }

        GfxDeviceDescriptor deviceDesc = {};
        deviceDesc.label = "Test Device";

        if (gfxAdapterCreateDevice(adapter, &deviceDesc, &device) != GFX_RESULT_SUCCESS) {
            gfxAdapterDestroy(adapter);
            gfxInstanceDestroy(instance);
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to create device";
        }
    }

    void TearDown() override
    {
        if (device) {
            gfxDeviceDestroy(device);
        }
        if (adapter) {
            gfxAdapterDestroy(adapter);
        }
        if (instance) {
            gfxInstanceDestroy(instance);
        }
        gfxUnloadBackend(backend);
    }

    GfxBackend backend = GFX_BACKEND_VULKAN;
    GfxInstance instance = nullptr;
    GfxAdapter adapter = nullptr;
    GfxDevice device = nullptr;
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

// Simple SPIR-V compute shader (generated from GLSL)
// Equivalent GLSL:
// #version 450
// layout(set = 0, binding = 0) buffer Data { float values[]; };
// void main() { values[gl_GlobalInvocationID.x] *= 2.0; }
static const uint32_t spirvComputeShader[] = {
    0x07230203, 0x00010000, 0x000d000a, 0x00000028,
    0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
    0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0006000f, 0x00000005, 0x00000004, 0x6e69616d,
    0x00000000, 0x0000000d, 0x00060010, 0x00000004,
    0x00000011, 0x00000040, 0x00000001, 0x00000001,
    0x00030003, 0x00000002, 0x000001c2, 0x00040005,
    0x00000004, 0x6e69616d, 0x00000000, 0x00050005,
    0x00000009, 0x65646e69, 0x00000078, 0x00000000,
    0x00080005, 0x0000000d, 0x475f6c67, 0x61626f6c,
    0x766e496c, 0x7461636f, 0x496e6f69, 0x00000044,
    0x00040005, 0x00000011, 0x61746144, 0x00000000,
    0x00060006, 0x00000011, 0x00000000, 0x756c6176,
    0x00007365, 0x00000000, 0x00030005, 0x00000013,
    0x00000000, 0x00040047, 0x0000000d, 0x0000000b,
    0x0000001c, 0x00040047, 0x00000010, 0x00000006,
    0x00000004, 0x00040048, 0x00000011, 0x00000000,
    0x00000018, 0x00050048, 0x00000011, 0x00000000,
    0x00000023, 0x00000000, 0x00030047, 0x00000011,
    0x00000003, 0x00040047, 0x00000013, 0x00000022,
    0x00000000, 0x00040047, 0x00000013, 0x00000021,
    0x00000000, 0x00020013, 0x00000002, 0x00030021,
    0x00000003, 0x00000002, 0x00040015, 0x00000006,
    0x00000020, 0x00000000, 0x00040020, 0x00000007,
    0x00000007, 0x00000006, 0x00040015, 0x0000000a,
    0x00000020, 0x00000001, 0x00040017, 0x0000000b,
    0x0000000a, 0x00000003, 0x00040020, 0x0000000c,
    0x00000001, 0x0000000b, 0x0004003b, 0x0000000c,
    0x0000000d, 0x00000001, 0x0004002b, 0x00000006,
    0x0000000e, 0x00000000, 0x00040020, 0x0000000f,
    0x00000001, 0x00000006, 0x0003001d, 0x00000010,
    0x00000016, 0x0003001e, 0x00000011, 0x00000010,
    0x00040020, 0x00000012, 0x00000002, 0x00000011,
    0x0004003b, 0x00000012, 0x00000013, 0x00000002,
    0x0004002b, 0x0000000a, 0x00000014, 0x00000000,
    0x00030016, 0x00000016, 0x00000020, 0x00040020,
    0x00000017, 0x00000002, 0x00000016, 0x0004002b,
    0x00000016, 0x0000001a, 0x40000000, 0x00050036,
    0x00000002, 0x00000004, 0x00000000, 0x00000003,
    0x000200f8, 0x00000005, 0x0004003b, 0x00000007,
    0x00000008, 0x00000007, 0x00050041, 0x0000000f,
    0x00000015, 0x0000000d, 0x0000000e, 0x0004003d,
    0x00000006, 0x00000019, 0x00000015, 0x0003003e,
    0x00000008, 0x00000019, 0x0004003d, 0x00000006,
    0x0000001b, 0x00000008, 0x00060041, 0x00000017,
    0x0000001c, 0x00000013, 0x00000014, 0x0000001b,
    0x0004003d, 0x00000016, 0x0000001d, 0x0000001c,
    0x00050085, 0x00000016, 0x0000001e, 0x0000001d,
    0x0000001a, 0x0004003d, 0x00000006, 0x0000001f,
    0x00000008, 0x00060041, 0x00000017, 0x00000020,
    0x00000013, 0x00000014, 0x0000001f, 0x0003003e,
    0x00000020, 0x0000001e, 0x000100fd, 0x00010038
};

TEST_P(GfxShaderTest, CreateShaderWithNullDevice)
{
    const char* shaderCode = (backend == GFX_BACKEND_VULKAN) ? "" : wgslComputeShader;
    GfxShaderSourceType sourceType = (backend == GFX_BACKEND_VULKAN) ? GFX_SHADER_SOURCE_SPIRV : GFX_SHADER_SOURCE_WGSL;

    GfxShaderDescriptor desc = {};
    desc.label = "Test Shader";
    desc.sourceType = sourceType;
    desc.code = shaderCode;
    desc.codeSize = (backend == GFX_BACKEND_VULKAN) ? sizeof(spirvComputeShader) : 0;
    desc.entryPoint = "main";

    GfxShader shader = nullptr;
    GfxResult result = gfxDeviceCreateShader(nullptr, &desc, &shader);

    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxShaderTest, CreateShaderWithNullDescriptor)
{
    GfxShader shader = nullptr;
    GfxResult result = gfxDeviceCreateShader(device, nullptr, &shader);

    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxShaderTest, CreateShaderWithNullOutput)
{
    const char* shaderCode = (backend == GFX_BACKEND_VULKAN) ? "" : wgslComputeShader;
    GfxShaderSourceType sourceType = (backend == GFX_BACKEND_VULKAN) ? GFX_SHADER_SOURCE_SPIRV : GFX_SHADER_SOURCE_WGSL;

    GfxShaderDescriptor desc = {};
    desc.label = "Test Shader";
    desc.sourceType = sourceType;
    desc.code = shaderCode;
    desc.codeSize = (backend == GFX_BACKEND_VULKAN) ? sizeof(spirvComputeShader) : 0;
    desc.entryPoint = "main";

    GfxResult result = gfxDeviceCreateShader(device, &desc, nullptr);

    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxShaderTest, CreateComputeShaderWGSL)
{
    if (backend == GFX_BACKEND_VULKAN) {
        GTEST_SKIP() << "WGSL is WebGPU only";
    }

    GfxShaderDescriptor desc = {};
    desc.label = "WGSL Compute Shader";
    desc.sourceType = GFX_SHADER_SOURCE_WGSL;
    desc.code = wgslComputeShader;
    desc.codeSize = strlen(wgslComputeShader) + 1; // Include null terminator
    desc.entryPoint = "main";

    GfxShader shader = nullptr;
    GfxResult result = gfxDeviceCreateShader(device, &desc, &shader);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(shader, nullptr);

    if (shader) {
        gfxShaderDestroy(shader);
    }
}

TEST_P(GfxShaderTest, CreateComputeShaderSPIRV)
{
    GfxShaderDescriptor desc = {};
    desc.label = "SPIR-V Compute Shader";
    desc.sourceType = GFX_SHADER_SOURCE_SPIRV;
    desc.code = spirvComputeShader;
    desc.codeSize = sizeof(spirvComputeShader);
    desc.entryPoint = "main";

    GfxShader shader = nullptr;
    GfxResult result = gfxDeviceCreateShader(device, &desc, &shader);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(shader, nullptr);

    if (shader) {
        gfxShaderDestroy(shader);
    }
}

TEST_P(GfxShaderTest, CreateVertexShaderWGSL)
{
    if (backend == GFX_BACKEND_VULKAN) {
        GTEST_SKIP() << "WGSL is WebGPU only";
    }

    GfxShaderDescriptor desc = {};
    desc.label = "WGSL Vertex Shader";
    desc.sourceType = GFX_SHADER_SOURCE_WGSL;
    desc.code = wgslVertexShader;
    desc.codeSize = strlen(wgslVertexShader) + 1; // Include null terminator
    desc.entryPoint = "main";

    GfxShader shader = nullptr;
    GfxResult result = gfxDeviceCreateShader(device, &desc, &shader);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(shader, nullptr);

    if (shader) {
        gfxShaderDestroy(shader);
    }
}

TEST_P(GfxShaderTest, CreateFragmentShaderWGSL)
{
    if (backend == GFX_BACKEND_VULKAN) {
        GTEST_SKIP() << "WGSL is WebGPU only";
    }

    GfxShaderDescriptor desc = {};
    desc.label = "WGSL Fragment Shader";
    desc.sourceType = GFX_SHADER_SOURCE_WGSL;
    desc.code = wgslFragmentShader;
    desc.codeSize = strlen(wgslFragmentShader) + 1; // Include null terminator
    desc.entryPoint = "main";

    GfxShader shader = nullptr;
    GfxResult result = gfxDeviceCreateShader(device, &desc, &shader);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(shader, nullptr);

    if (shader) {
        gfxShaderDestroy(shader);
    }
}

TEST_P(GfxShaderTest, CreateMultipleShaders)
{
    if (backend == GFX_BACKEND_VULKAN) {
        GTEST_SKIP() << "Using WGSL for simplicity";
    }

    const int shaderCount = 3;
    GfxShader shaders[shaderCount] = {};

    const char* shaderCodes[] = { wgslComputeShader, wgslVertexShader, wgslFragmentShader };

    for (int i = 0; i < shaderCount; ++i) {
        GfxShaderDescriptor desc = {};
        desc.sourceType = GFX_SHADER_SOURCE_WGSL;
        desc.code = shaderCodes[i];
        desc.codeSize = strlen(shaderCodes[i]) + 1; // Include null terminator
        desc.entryPoint = "main";

        GfxResult result = gfxDeviceCreateShader(device, &desc, &shaders[i]);
        EXPECT_EQ(result, GFX_RESULT_SUCCESS);
        EXPECT_NE(shaders[i], nullptr);
    }

    for (int i = 0; i < shaderCount; ++i) {
        if (shaders[i]) {
            gfxShaderDestroy(shaders[i]);
        }
    }
}

TEST_P(GfxShaderTest, DestroyShaderWithNull)
{
    // Destroying NULL should return an error
    GfxResult result = gfxShaderDestroy(nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxShaderTest,
    testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU),
    [](const testing::TestParamInfo<GfxBackend>& info) {
        return info.param == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU";
    });
