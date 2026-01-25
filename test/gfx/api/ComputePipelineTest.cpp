#include <gfx/gfx.h>

#include <cstring>
#include <gtest/gtest.h>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

class GfxComputePipelineTest : public testing::TestWithParam<GfxBackend> {
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

// Simple WGSL compute shader - fills output with red
static const char* wgslComputeShader = R"(
@group(0) @binding(0) var<storage, read_write> output: array<f32>;

@compute @workgroup_size(64)
fn main(@builtin(global_invocation_id) globalId: vec3<u32>) {
    let index = globalId.x;
    output[index] = 1.0;
}
)";

// Simple SPIR-V compute shader binary - minimal shader for testing
// Does nothing, just tests pipeline creation
static const uint32_t spirvComputeShader[] = {
    0x07230203, 0x00010000, 0x0008000b, 0x0000000b, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0005000f, 0x00000005, 0x00000004, 0x6e69616d, 0x00000000, 0x00060010, 0x00000004, 0x00000011,
    0x00000040, 0x00000001, 0x00000001, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004,
    0x6e69616d, 0x00000000, 0x00040047, 0x0000000a, 0x0000000b, 0x00000019, 0x00020013, 0x00000002,
    0x00030021, 0x00000003, 0x00000002, 0x00040015, 0x00000006, 0x00000020, 0x00000000, 0x00040017,
    0x00000007, 0x00000006, 0x00000003, 0x0004002b, 0x00000006, 0x00000008, 0x00000040, 0x0004002b,
    0x00000006, 0x00000009, 0x00000001, 0x0006002c, 0x00000007, 0x0000000a, 0x00000008, 0x00000009,
    0x00000009, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005,
    0x000100fd, 0x00010038,
};

// ===========================================================================
// ComputePipeline Tests
// ===========================================================================

// Test: Create ComputePipeline with NULL device
TEST_P(GfxComputePipelineTest, CreateComputePipelineWithNullDevice)
{
    GfxShaderDescriptor shaderDesc = {};
    shaderDesc.label = "Test Compute Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        shaderDesc.code = spirvComputeShader;
        shaderDesc.codeSize = sizeof(spirvComputeShader);
    } else {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        shaderDesc.code = wgslComputeShader;
        shaderDesc.codeSize = strlen(wgslComputeShader) + 1;
    }
    shaderDesc.entryPoint = "main";

    GfxShader computeShader = nullptr;
    GfxResult result = gfxDeviceCreateShader(device, &shaderDesc, &computeShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxComputePipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = "Test Compute Pipeline";
    pipelineDesc.compute = computeShader;
    pipelineDesc.entryPoint = "main";

    GfxComputePipeline pipeline = nullptr;
    result = gfxDeviceCreateComputePipeline(nullptr, &pipelineDesc, &pipeline);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxShaderDestroy(computeShader);
}

// Test: Create ComputePipeline with NULL descriptor
TEST_P(GfxComputePipelineTest, CreateComputePipelineWithNullDescriptor)
{
    GfxComputePipeline pipeline = nullptr;
    GfxResult result = gfxDeviceCreateComputePipeline(device, nullptr, &pipeline);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Test: Create ComputePipeline with NULL output
TEST_P(GfxComputePipelineTest, CreateComputePipelineWithNullOutput)
{
    GfxShaderDescriptor shaderDesc = {};
    shaderDesc.label = "Test Compute Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        shaderDesc.code = spirvComputeShader;
        shaderDesc.codeSize = sizeof(spirvComputeShader);
    } else {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        shaderDesc.code = wgslComputeShader;
        shaderDesc.codeSize = strlen(wgslComputeShader) + 1;
    }
    shaderDesc.entryPoint = "main";

    GfxShader computeShader = nullptr;
    GfxResult result = gfxDeviceCreateShader(device, &shaderDesc, &computeShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxComputePipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = "Test Compute Pipeline";
    pipelineDesc.compute = computeShader;
    pipelineDesc.entryPoint = "main";

    result = gfxDeviceCreateComputePipeline(device, &pipelineDesc, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxShaderDestroy(computeShader);
}

// Test: Create basic ComputePipeline
TEST_P(GfxComputePipelineTest, CreateBasicComputePipeline)
{
    GfxShaderDescriptor shaderDesc = {};
    shaderDesc.label = "Test Compute Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        shaderDesc.code = spirvComputeShader;
        shaderDesc.codeSize = sizeof(spirvComputeShader);
    } else {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        shaderDesc.code = wgslComputeShader;
        shaderDesc.codeSize = strlen(wgslComputeShader) + 1;
    }
    shaderDesc.entryPoint = "main";

    GfxShader computeShader = nullptr;
    GfxResult result = gfxDeviceCreateShader(device, &shaderDesc, &computeShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(computeShader, nullptr);

    GfxComputePipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = "Basic Compute Pipeline";
    pipelineDesc.compute = computeShader;
    pipelineDesc.entryPoint = nullptr;  // Let it use shader's entry point
    pipelineDesc.bindGroupLayouts = nullptr;
    pipelineDesc.bindGroupLayoutCount = 0;

    GfxComputePipeline pipeline = nullptr;
    result = gfxDeviceCreateComputePipeline(device, &pipelineDesc, &pipeline);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(pipeline, nullptr);

    gfxComputePipelineDestroy(pipeline);
    gfxShaderDestroy(computeShader);
}

// Test: Create ComputePipeline with bind group layouts
TEST_P(GfxComputePipelineTest, CreateComputePipelineWithBindGroupLayouts)
{
    // Create a storage buffer bind group layout
    GfxBindGroupLayoutEntry entry = {};
    entry.binding = 0;
    entry.visibility = GFX_SHADER_STAGE_COMPUTE;
    entry.type = GFX_BINDING_TYPE_BUFFER;
    entry.buffer.hasDynamicOffset = false;
    entry.buffer.minBindingSize = 0;

    GfxBindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.label = "Compute Bind Group Layout";
    layoutDesc.entries = &entry;
    layoutDesc.entryCount = 1;

    GfxBindGroupLayout bindGroupLayout = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(device, &layoutDesc, &bindGroupLayout);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(bindGroupLayout, nullptr);

    // Create compute shader
    GfxShaderDescriptor shaderDesc = {};
    shaderDesc.label = "Test Compute Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        shaderDesc.code = spirvComputeShader;
        shaderDesc.codeSize = sizeof(spirvComputeShader);
    } else {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        shaderDesc.code = wgslComputeShader;
        shaderDesc.codeSize = strlen(wgslComputeShader) + 1;
    }
    shaderDesc.entryPoint = "main";

    GfxShader computeShader = nullptr;
    result = gfxDeviceCreateShader(device, &shaderDesc, &computeShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(computeShader, nullptr);

    // Create compute pipeline with bind group layout
    GfxComputePipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = "Compute Pipeline with Bind Groups";
    pipelineDesc.compute = computeShader;
    pipelineDesc.entryPoint = "main";
    pipelineDesc.bindGroupLayouts = &bindGroupLayout;
    pipelineDesc.bindGroupLayoutCount = 1;

    GfxComputePipeline pipeline = nullptr;
    result = gfxDeviceCreateComputePipeline(device, &pipelineDesc, &pipeline);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(pipeline, nullptr);

    gfxComputePipelineDestroy(pipeline);
    gfxShaderDestroy(computeShader);
    gfxBindGroupLayoutDestroy(bindGroupLayout);
}

// Test: Create ComputePipeline with multiple bind group layouts
TEST_P(GfxComputePipelineTest, CreateComputePipelineWithMultipleBindGroupLayouts)
{
    // Create first bind group layout (storage buffer)
    GfxBindGroupLayoutEntry entry1 = {};
    entry1.binding = 0;
    entry1.visibility = GFX_SHADER_STAGE_COMPUTE;
    entry1.type = GFX_BINDING_TYPE_BUFFER;
    entry1.buffer.hasDynamicOffset = false;
    entry1.buffer.minBindingSize = 0;

    GfxBindGroupLayoutDescriptor layoutDesc1 = {};
    layoutDesc1.label = "Storage Buffer Layout";
    layoutDesc1.entries = &entry1;
    layoutDesc1.entryCount = 1;

    GfxBindGroupLayout bindGroupLayout1 = nullptr;
    GfxResult result = gfxDeviceCreateBindGroupLayout(device, &layoutDesc1, &bindGroupLayout1);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create second bind group layout (uniform buffer)
    GfxBindGroupLayoutEntry entry2 = {};
    entry2.binding = 0;
    entry2.visibility = GFX_SHADER_STAGE_COMPUTE;
    entry2.type = GFX_BINDING_TYPE_BUFFER;
    entry2.buffer.hasDynamicOffset = false;
    entry2.buffer.minBindingSize = 0;

    GfxBindGroupLayoutDescriptor layoutDesc2 = {};
    layoutDesc2.label = "Uniform Buffer Layout";
    layoutDesc2.entries = &entry2;
    layoutDesc2.entryCount = 1;

    GfxBindGroupLayout bindGroupLayout2 = nullptr;
    result = gfxDeviceCreateBindGroupLayout(device, &layoutDesc2, &bindGroupLayout2);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create compute shader
    GfxShaderDescriptor shaderDesc = {};
    shaderDesc.label = "Test Compute Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        shaderDesc.code = spirvComputeShader;
        shaderDesc.codeSize = sizeof(spirvComputeShader);
    } else {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        shaderDesc.code = wgslComputeShader;
        shaderDesc.codeSize = strlen(wgslComputeShader) + 1;
    }
    shaderDesc.entryPoint = "main";

    GfxShader computeShader = nullptr;
    result = gfxDeviceCreateShader(device, &shaderDesc, &computeShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create pipeline with multiple bind group layouts
    GfxBindGroupLayout layouts[] = { bindGroupLayout1, bindGroupLayout2 };

    GfxComputePipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = "Compute Pipeline with Multiple Bind Groups";
    pipelineDesc.compute = computeShader;
    pipelineDesc.entryPoint = "main";
    pipelineDesc.bindGroupLayouts = layouts;
    pipelineDesc.bindGroupLayoutCount = 2;

    GfxComputePipeline pipeline = nullptr;
    result = gfxDeviceCreateComputePipeline(device, &pipelineDesc, &pipeline);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(pipeline, nullptr);

    gfxComputePipelineDestroy(pipeline);
    gfxShaderDestroy(computeShader);
    gfxBindGroupLayoutDestroy(bindGroupLayout2);
    gfxBindGroupLayoutDestroy(bindGroupLayout1);
}

// Test: Destroy NULL ComputePipeline
TEST_P(GfxComputePipelineTest, DestroyNullComputePipeline)
{
    GfxResult result = gfxComputePipelineDestroy(nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxComputePipelineTest,
    testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU),
    [](const testing::TestParamInfo<GfxBackend>& info) {
        return info.param == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU";
    });
