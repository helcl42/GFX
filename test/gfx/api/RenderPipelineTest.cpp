#include "CommonTest.h"

#include <cstring>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxRenderPipelineTest : public testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        if (gfxLoadBackend(backend) != GFX_RESULT_SUCCESS) {
            GTEST_SKIP() << "Backend not available";
        }

        const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
        GfxInstanceDescriptor instDesc = {};
        instDesc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
        instDesc.pNext = nullptr;
        instDesc.backend = backend;
        instDesc.enabledExtensions = extensions;
        instDesc.enabledExtensionCount = 1;

        if (gfxCreateInstance(&instDesc, &instance) != GFX_RESULT_SUCCESS) {
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to create isnstance";
        }

        GfxAdapterDescriptor adapterDesc = {};
        adapterDesc.sType = GFX_STRUCTURE_TYPE_ADAPTER_DESCRIPTOR;
        adapterDesc.pNext = nullptr;
        adapterDesc.adapterIndex = 0;

        if (gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter) != GFX_RESULT_SUCCESS) {
            gfxInstanceDestroy(instance);
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to request adapter";
        }

        GfxDeviceDescriptor deviceDesc = {};
        deviceDesc.sType = GFX_STRUCTURE_TYPE_DEVICE_DESCRIPTOR;
        deviceDesc.pNext = nullptr;
        deviceDesc.label = "Test Device";

        if (gfxAdapterCreateDevice(adapter, &deviceDesc, &device) != GFX_RESULT_SUCCESS) {
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

// Simple WGSL vertex shader
static const char* wgslVertexShader = R"(
@vertex
fn main(@location(0) position: vec3<f32>) -> @builtin(position) vec4<f32> {
    return vec4<f32>(position, 1.0);
}
)";

// Simple WGSL fragment shader
static const char* wgslFragmentShader = R"(
@fragment
fn main() -> @location(0) vec4<f32> {
    return vec4<f32>(1.0, 0.0, 0.0, 1.0);
}
)";

// Simple SPIR-V vertex shader binary
// Equivalent GLSL: void main() { gl_Position = vec4(position, 1.0); }
static const uint32_t spirvVertexShader[] = {
    0x07230203, 0x00010000, 0x0008000b, 0x0000001b, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0007000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000d, 0x00000012, 0x00030003,
    0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00060005, 0x0000000b,
    0x505f6c67, 0x65567265, 0x78657472, 0x00000000, 0x00060006, 0x0000000b, 0x00000000, 0x505f6c67,
    0x7469736f, 0x006e6f69, 0x00070006, 0x0000000b, 0x00000001, 0x505f6c67, 0x746e696f, 0x657a6953,
    0x00000000, 0x00070006, 0x0000000b, 0x00000002, 0x435f6c67, 0x4470696c, 0x61747369, 0x0065636e,
    0x00070006, 0x0000000b, 0x00000003, 0x435f6c67, 0x446c6c75, 0x61747369, 0x0065636e, 0x00030005,
    0x0000000d, 0x00000000, 0x00050005, 0x00000012, 0x69736f70, 0x6e6f6974, 0x00000000, 0x00030047,
    0x0000000b, 0x00000002, 0x00050048, 0x0000000b, 0x00000000, 0x0000000b, 0x00000000, 0x00050048,
    0x0000000b, 0x00000001, 0x0000000b, 0x00000001, 0x00050048, 0x0000000b, 0x00000002, 0x0000000b,
    0x00000003, 0x00050048, 0x0000000b, 0x00000003, 0x0000000b, 0x00000004, 0x00040047, 0x00000012,
    0x0000001e, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016,
    0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040015, 0x00000008,
    0x00000020, 0x00000000, 0x0004002b, 0x00000008, 0x00000009, 0x00000001, 0x0004001c, 0x0000000a,
    0x00000006, 0x00000009, 0x0006001e, 0x0000000b, 0x00000007, 0x00000006, 0x0000000a, 0x0000000a,
    0x00040020, 0x0000000c, 0x00000003, 0x0000000b, 0x0004003b, 0x0000000c, 0x0000000d, 0x00000003,
    0x00040015, 0x0000000e, 0x00000020, 0x00000001, 0x0004002b, 0x0000000e, 0x0000000f, 0x00000000,
    0x00040017, 0x00000010, 0x00000006, 0x00000003, 0x00040020, 0x00000011, 0x00000001, 0x00000010,
    0x0004003b, 0x00000011, 0x00000012, 0x00000001, 0x0004002b, 0x00000006, 0x00000014, 0x3f800000,
    0x00040020, 0x00000019, 0x00000003, 0x00000007, 0x00050036, 0x00000002, 0x00000004, 0x00000000,
    0x00000003, 0x000200f8, 0x00000005, 0x0004003d, 0x00000010, 0x00000013, 0x00000012, 0x00050051,
    0x00000006, 0x00000015, 0x00000013, 0x00000000, 0x00050051, 0x00000006, 0x00000016, 0x00000013,
    0x00000001, 0x00050051, 0x00000006, 0x00000017, 0x00000013, 0x00000002, 0x00070050, 0x00000007,
    0x00000018, 0x00000015, 0x00000016, 0x00000017, 0x00000014, 0x00050041, 0x00000019, 0x0000001a,
    0x0000000d, 0x0000000f, 0x0003003e, 0x0000001a, 0x00000018, 0x000100fd, 0x00010038
};

// Simple SPIR-V fragment shader binary
// Equivalent GLSL: void main() { fragColor = vec4(1.0, 0.0, 0.0, 1.0); }
static const uint32_t spirvFragmentShader[] = {
    0x07230203, 0x00010000, 0x0008000b, 0x0000000d, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0006000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x00030010, 0x00000004,
    0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000,
    0x00050005, 0x00000009, 0x67617266, 0x6f6c6f43, 0x00000072, 0x00040047, 0x00000009, 0x0000001e,
    0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006,
    0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020, 0x00000008, 0x00000003,
    0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x0004002b, 0x00000006, 0x0000000a,
    0x3f800000, 0x0004002b, 0x00000006, 0x0000000b, 0x00000000, 0x0007002c, 0x00000007, 0x0000000c,
    0x0000000a, 0x0000000b, 0x0000000b, 0x0000000a, 0x00050036, 0x00000002, 0x00000004, 0x00000000,
    0x00000003, 0x000200f8, 0x00000005, 0x0003003e, 0x00000009, 0x0000000c, 0x000100fd, 0x00010038
};

// ===========================================================================
// RenderPipeline Tests
// ===========================================================================

// Test: Create RenderPipeline with NULL device
TEST_P(GfxRenderPipelineTest, CreateRenderPipelineWithNullDevice)
{
    // Create a simple render pass for the pipeline
    GfxRenderPassColorAttachmentTarget colorTarget = {};
    colorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTarget.ops.loadOp = GFX_LOAD_OP_CLEAR;
    colorTarget.ops.storeOp = GFX_STORE_OP_STORE;
    colorTarget.finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachment = {};
    colorAttachment.target = colorTarget;
    colorAttachment.resolveTarget = nullptr;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(renderPass, nullptr);

    // Create shader with backend-appropriate code
    GfxShaderDescriptor shaderDesc = {};
    shaderDesc.label = "Test Vertex Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        shaderDesc.code = spirvVertexShader;
        shaderDesc.codeSize = sizeof(spirvVertexShader);
    } else {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        shaderDesc.code = wgslVertexShader;
        shaderDesc.codeSize = strlen(wgslVertexShader) + 1; // Include null terminator
    }
    shaderDesc.entryPoint = "main";

    GfxShader vertexShader = nullptr;
    result = gfxDeviceCreateShader(device, &shaderDesc, &vertexShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Setup vertex state
    GfxVertexAttribute vertexAttr = {};
    vertexAttr.format = GFX_TEXTURE_FORMAT_R32G32B32_FLOAT;
    vertexAttr.offset = 0;
    vertexAttr.shaderLocation = 0;

    GfxVertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.arrayStride = 12;
    vertexBufferLayout.attributes = &vertexAttr;
    vertexBufferLayout.attributeCount = 1;
    vertexBufferLayout.stepMode = GFX_VERTEX_STEP_MODE_VERTEX;

    GfxVertexState vertexState = {};
    vertexState.module = vertexShader;
    vertexState.entryPoint = "main";
    vertexState.buffers = &vertexBufferLayout;
    vertexState.bufferCount = 1;

    // Setup primitive state
    GfxPrimitiveState primitiveState = {};
    primitiveState.topology = GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    primitiveState.stripIndexFormat = GFX_INDEX_FORMAT_UNDEFINED;
    primitiveState.frontFace = GFX_FRONT_FACE_COUNTER_CLOCKWISE;
    primitiveState.cullMode = GFX_CULL_MODE_NONE;
    primitiveState.polygonMode = GFX_POLYGON_MODE_FILL;

    GfxRenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = "Test Pipeline";
    pipelineDesc.renderPass = renderPass;
    pipelineDesc.vertex = &vertexState;
    pipelineDesc.primitive = &primitiveState;
    pipelineDesc.sampleCount = GFX_SAMPLE_COUNT_1;

    GfxRenderPipeline pipeline = nullptr;
    result = gfxDeviceCreateRenderPipeline(nullptr, &pipelineDesc, &pipeline);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxShaderDestroy(vertexShader);
    gfxRenderPassDestroy(renderPass);
}

// Test: Create RenderPipeline with NULL descriptor
TEST_P(GfxRenderPipelineTest, CreateRenderPipelineWithNullDescriptor)
{
    GfxRenderPipeline pipeline = nullptr;
    GfxResult result = gfxDeviceCreateRenderPipeline(device, nullptr, &pipeline);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// Test: Create RenderPipeline with NULL output
TEST_P(GfxRenderPipelineTest, CreateRenderPipelineWithNullOutput)
{
    GfxRenderPassColorAttachmentTarget colorTarget = {};
    colorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTarget.ops.loadOp = GFX_LOAD_OP_CLEAR;
    colorTarget.ops.storeOp = GFX_STORE_OP_STORE;
    colorTarget.finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachment = {};
    colorAttachment.target = colorTarget;
    colorAttachment.resolveTarget = nullptr;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxShaderDescriptor shaderDesc = {};
    shaderDesc.label = "Test Vertex Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        shaderDesc.code = spirvVertexShader;
        shaderDesc.codeSize = sizeof(spirvVertexShader);
    } else {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        shaderDesc.code = wgslVertexShader;
        shaderDesc.codeSize = strlen(wgslVertexShader) + 1; // Include null terminator
    }
    shaderDesc.entryPoint = "main";
    GfxShader vertexShader = nullptr;
    result = gfxDeviceCreateShader(device, &shaderDesc, &vertexShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxVertexAttribute vertexAttr = {};
    vertexAttr.format = GFX_TEXTURE_FORMAT_R32G32B32_FLOAT;
    vertexAttr.offset = 0;
    vertexAttr.shaderLocation = 0;

    GfxVertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.arrayStride = 12;
    vertexBufferLayout.attributes = &vertexAttr;
    vertexBufferLayout.attributeCount = 1;
    vertexBufferLayout.stepMode = GFX_VERTEX_STEP_MODE_VERTEX;

    GfxVertexState vertexState = {};
    vertexState.module = vertexShader;
    vertexState.entryPoint = "main";
    vertexState.buffers = &vertexBufferLayout;
    vertexState.bufferCount = 1;

    GfxPrimitiveState primitiveState = {};
    primitiveState.topology = GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    primitiveState.stripIndexFormat = GFX_INDEX_FORMAT_UNDEFINED;
    primitiveState.frontFace = GFX_FRONT_FACE_COUNTER_CLOCKWISE;
    primitiveState.cullMode = GFX_CULL_MODE_NONE;
    primitiveState.polygonMode = GFX_POLYGON_MODE_FILL;

    GfxRenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = "Test Pipeline";
    pipelineDesc.renderPass = renderPass;
    pipelineDesc.vertex = &vertexState;
    pipelineDesc.primitive = &primitiveState;
    pipelineDesc.sampleCount = GFX_SAMPLE_COUNT_1;

    result = gfxDeviceCreateRenderPipeline(device, &pipelineDesc, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxShaderDestroy(vertexShader);
    gfxRenderPassDestroy(renderPass);
}

// Test: Create basic RenderPipeline with vertex shader only
TEST_P(GfxRenderPipelineTest, CreateBasicRenderPipeline)
{
    GfxRenderPassColorAttachmentTarget colorTarget = {};
    colorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTarget.ops.loadOp = GFX_LOAD_OP_CLEAR;
    colorTarget.ops.storeOp = GFX_STORE_OP_STORE;
    colorTarget.finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachment = {};
    colorAttachment.target = colorTarget;
    colorAttachment.resolveTarget = nullptr;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(renderPass, nullptr);

    GfxShaderDescriptor shaderDesc = {};
    shaderDesc.label = "Test Vertex Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        shaderDesc.code = spirvVertexShader;
        shaderDesc.codeSize = sizeof(spirvVertexShader);
    } else {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        shaderDesc.code = wgslVertexShader;
        shaderDesc.codeSize = strlen(wgslVertexShader) + 1; // Include null terminator
    }
    shaderDesc.entryPoint = "main";

    GfxShader vertexShader = nullptr;
    result = gfxDeviceCreateShader(device, &shaderDesc, &vertexShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(vertexShader, nullptr);

    GfxVertexAttribute vertexAttr = {};
    vertexAttr.format = GFX_TEXTURE_FORMAT_R32G32B32_FLOAT;
    vertexAttr.offset = 0;
    vertexAttr.shaderLocation = 0;

    GfxVertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.arrayStride = 12;
    vertexBufferLayout.attributes = &vertexAttr;
    vertexBufferLayout.attributeCount = 1;
    vertexBufferLayout.stepMode = GFX_VERTEX_STEP_MODE_VERTEX;

    GfxVertexState vertexState = {};
    vertexState.module = vertexShader;
    vertexState.entryPoint = "main";
    vertexState.buffers = &vertexBufferLayout;
    vertexState.bufferCount = 1;

    GfxPrimitiveState primitiveState = {};
    primitiveState.topology = GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    primitiveState.stripIndexFormat = GFX_INDEX_FORMAT_UNDEFINED;
    primitiveState.frontFace = GFX_FRONT_FACE_COUNTER_CLOCKWISE;
    primitiveState.cullMode = GFX_CULL_MODE_NONE;
    primitiveState.polygonMode = GFX_POLYGON_MODE_FILL;

    GfxShaderDescriptor fragShaderDesc = {};
    fragShaderDesc.label = "Test Fragment Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        fragShaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        fragShaderDesc.code = spirvFragmentShader;
        fragShaderDesc.codeSize = sizeof(spirvFragmentShader);
    } else {
        fragShaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        fragShaderDesc.code = wgslFragmentShader;
        fragShaderDesc.codeSize = strlen(wgslFragmentShader) + 1; // Include null terminator
    }
    fragShaderDesc.entryPoint = "main";

    GfxShader fragmentShader = nullptr;
    result = gfxDeviceCreateShader(device, &fragShaderDesc, &fragmentShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(fragmentShader, nullptr);

    GfxColorTargetState pipelineColorTarget = {};
    pipelineColorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    pipelineColorTarget.blend = nullptr;
    pipelineColorTarget.writeMask = 0xF; // All channels

    GfxFragmentState fragmentState = {};
    fragmentState.module = fragmentShader;
    fragmentState.entryPoint = "main";
    fragmentState.targets = &pipelineColorTarget;
    fragmentState.targetCount = 1;

    GfxRenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = "Test Pipeline";
    pipelineDesc.renderPass = renderPass;
    pipelineDesc.vertex = &vertexState;
    pipelineDesc.fragment = &fragmentState;
    pipelineDesc.primitive = &primitiveState;
    pipelineDesc.sampleCount = GFX_SAMPLE_COUNT_1;

    GfxRenderPipeline pipeline = nullptr;
    result = gfxDeviceCreateRenderPipeline(device, &pipelineDesc, &pipeline);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(pipeline, nullptr);

    gfxRenderPipelineDestroy(pipeline);
    gfxShaderDestroy(fragmentShader);
    gfxShaderDestroy(vertexShader);
    gfxRenderPassDestroy(renderPass);
}

// Test: Create RenderPipeline with vertex and fragment shaders
TEST_P(GfxRenderPipelineTest, CreateRenderPipelineWithFragmentShader)
{
    GfxRenderPassColorAttachmentTarget colorTarget = {};
    colorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTarget.ops.loadOp = GFX_LOAD_OP_CLEAR;
    colorTarget.ops.storeOp = GFX_STORE_OP_STORE;
    colorTarget.finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachment = {};
    colorAttachment.target = colorTarget;
    colorAttachment.resolveTarget = nullptr;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxShaderDescriptor vertexShaderDesc = {};
    vertexShaderDesc.label = "Test Vertex Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        vertexShaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        vertexShaderDesc.code = spirvVertexShader;
        vertexShaderDesc.codeSize = sizeof(spirvVertexShader);
    } else {
        vertexShaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        vertexShaderDesc.code = wgslVertexShader;
        vertexShaderDesc.codeSize = strlen(wgslVertexShader) + 1; // Include null terminator
    }
    vertexShaderDesc.entryPoint = "main";

    GfxShader vertexShader = nullptr;
    result = gfxDeviceCreateShader(device, &vertexShaderDesc, &vertexShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxShaderDescriptor fragmentShaderDesc = {};
    fragmentShaderDesc.label = "Test Fragment Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        fragmentShaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        fragmentShaderDesc.code = spirvFragmentShader;
        fragmentShaderDesc.codeSize = sizeof(spirvFragmentShader);
    } else {
        fragmentShaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        fragmentShaderDesc.code = wgslFragmentShader;
        fragmentShaderDesc.codeSize = strlen(wgslFragmentShader) + 1; // Include null terminator
    }
    fragmentShaderDesc.entryPoint = "main";

    GfxShader fragmentShader = nullptr;
    result = gfxDeviceCreateShader(device, &fragmentShaderDesc, &fragmentShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxVertexAttribute vertexAttr = {};
    vertexAttr.format = GFX_TEXTURE_FORMAT_R32G32B32_FLOAT;
    vertexAttr.offset = 0;
    vertexAttr.shaderLocation = 0;

    GfxVertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.arrayStride = 12;
    vertexBufferLayout.attributes = &vertexAttr;
    vertexBufferLayout.attributeCount = 1;
    vertexBufferLayout.stepMode = GFX_VERTEX_STEP_MODE_VERTEX;

    GfxVertexState vertexState = {};
    vertexState.module = vertexShader;
    vertexState.entryPoint = "main";
    vertexState.buffers = &vertexBufferLayout;
    vertexState.bufferCount = 1;

    GfxColorTargetState fragmentColorTarget = {};
    fragmentColorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    fragmentColorTarget.writeMask = 0xF; // All channels

    GfxFragmentState fragmentState = {};
    fragmentState.module = fragmentShader;
    fragmentState.entryPoint = "main";
    fragmentState.targets = &fragmentColorTarget;
    fragmentState.targetCount = 1;

    GfxPrimitiveState primitiveState = {};
    primitiveState.topology = GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    primitiveState.stripIndexFormat = GFX_INDEX_FORMAT_UNDEFINED;
    primitiveState.frontFace = GFX_FRONT_FACE_COUNTER_CLOCKWISE;
    primitiveState.cullMode = GFX_CULL_MODE_NONE;
    primitiveState.polygonMode = GFX_POLYGON_MODE_FILL;

    GfxRenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = "Test Pipeline With Fragment";
    pipelineDesc.renderPass = renderPass;
    pipelineDesc.vertex = &vertexState;
    pipelineDesc.fragment = &fragmentState;
    pipelineDesc.primitive = &primitiveState;
    pipelineDesc.sampleCount = GFX_SAMPLE_COUNT_1;

    GfxRenderPipeline pipeline = nullptr;
    result = gfxDeviceCreateRenderPipeline(device, &pipelineDesc, &pipeline);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(pipeline, nullptr);

    gfxRenderPipelineDestroy(pipeline);
    gfxShaderDestroy(fragmentShader);
    gfxShaderDestroy(vertexShader);
    gfxRenderPassDestroy(renderPass);
}

// Test: Create RenderPipeline with only vertex shader (depth-only pass)
TEST_P(GfxRenderPipelineTest, CreateRenderPipelineWithVertexShaderOnly)
{
    // Create a render pass with only depth attachment (no color)
    GfxRenderPassDepthStencilAttachmentTarget depthTarget = {};
    depthTarget.format = GFX_TEXTURE_FORMAT_DEPTH32_FLOAT;
    depthTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    depthTarget.depthOps.loadOp = GFX_LOAD_OP_CLEAR;
    depthTarget.depthOps.storeOp = GFX_STORE_OP_STORE;
    depthTarget.stencilOps.loadOp = GFX_LOAD_OP_DONT_CARE;
    depthTarget.stencilOps.storeOp = GFX_STORE_OP_DONT_CARE;
    depthTarget.finalLayout = GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;

    GfxRenderPassDepthStencilAttachment depthAttachment = {};
    depthAttachment.target = depthTarget;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachments = nullptr;
    renderPassDesc.colorAttachmentCount = 0;
    renderPassDesc.depthStencilAttachment = &depthAttachment;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(renderPass, nullptr);

    GfxShaderDescriptor shaderDesc = {};
    shaderDesc.label = "Test Vertex Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        shaderDesc.code = spirvVertexShader;
        shaderDesc.codeSize = sizeof(spirvVertexShader);
    } else {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        shaderDesc.code = wgslVertexShader;
        shaderDesc.codeSize = strlen(wgslVertexShader) + 1; // Include null terminator
    }
    shaderDesc.entryPoint = "main";

    GfxShader vertexShader = nullptr;
    result = gfxDeviceCreateShader(device, &shaderDesc, &vertexShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(vertexShader, nullptr);

    GfxVertexAttribute vertexAttr = {};
    vertexAttr.format = GFX_TEXTURE_FORMAT_R32G32B32_FLOAT;
    vertexAttr.offset = 0;
    vertexAttr.shaderLocation = 0;

    GfxVertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.arrayStride = 12;
    vertexBufferLayout.attributes = &vertexAttr;
    vertexBufferLayout.attributeCount = 1;
    vertexBufferLayout.stepMode = GFX_VERTEX_STEP_MODE_VERTEX;

    GfxVertexState vertexState = {};
    vertexState.module = vertexShader;
    vertexState.entryPoint = "main";
    vertexState.buffers = &vertexBufferLayout;
    vertexState.bufferCount = 1;

    GfxPrimitiveState primitiveState = {};
    primitiveState.topology = GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    primitiveState.stripIndexFormat = GFX_INDEX_FORMAT_UNDEFINED;
    primitiveState.frontFace = GFX_FRONT_FACE_COUNTER_CLOCKWISE;
    primitiveState.cullMode = GFX_CULL_MODE_NONE;
    primitiveState.polygonMode = GFX_POLYGON_MODE_FILL;

    GfxDepthStencilState depthStencilState = {};
    depthStencilState.format = GFX_TEXTURE_FORMAT_DEPTH32_FLOAT;
    depthStencilState.depthWriteEnabled = true;
    depthStencilState.depthCompare = GFX_COMPARE_FUNCTION_LESS;

    GfxRenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = "Depth-Only Pipeline";
    pipelineDesc.renderPass = renderPass;
    pipelineDesc.vertex = &vertexState;
    pipelineDesc.fragment = nullptr; // No fragment shader for depth-only rendering
    pipelineDesc.primitive = &primitiveState;
    pipelineDesc.depthStencil = &depthStencilState;
    pipelineDesc.sampleCount = GFX_SAMPLE_COUNT_1;

    GfxRenderPipeline pipeline = nullptr;
    result = gfxDeviceCreateRenderPipeline(device, &pipelineDesc, &pipeline);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(pipeline, nullptr);

    gfxRenderPipelineDestroy(pipeline);
    gfxShaderDestroy(vertexShader);
    gfxRenderPassDestroy(renderPass);
}

// Test: Create RenderPipeline with different topologies
TEST_P(GfxRenderPipelineTest, CreateRenderPipelineWithDifferentTopologies)
{
    GfxRenderPassColorAttachmentTarget colorTarget = {};
    colorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTarget.ops.loadOp = GFX_LOAD_OP_CLEAR;
    colorTarget.ops.storeOp = GFX_STORE_OP_STORE;
    colorTarget.finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachment = {};
    colorAttachment.target = colorTarget;
    colorAttachment.resolveTarget = nullptr;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxShaderDescriptor shaderDesc = {};
    shaderDesc.label = "Test Vertex Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        shaderDesc.code = spirvVertexShader;
        shaderDesc.codeSize = sizeof(spirvVertexShader);
    } else {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        shaderDesc.code = wgslVertexShader;
        shaderDesc.codeSize = strlen(wgslVertexShader) + 1; // Include null terminator
    }
    shaderDesc.entryPoint = "main";

    GfxShader vertexShader = nullptr;
    result = gfxDeviceCreateShader(device, &shaderDesc, &vertexShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxShaderDescriptor fragShaderDesc = {};
    fragShaderDesc.label = "Test Fragment Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        fragShaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        fragShaderDesc.code = spirvFragmentShader;
        fragShaderDesc.codeSize = sizeof(spirvFragmentShader);
    } else {
        fragShaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        fragShaderDesc.code = wgslFragmentShader;
        fragShaderDesc.codeSize = strlen(wgslFragmentShader) + 1; // Include null terminator
    }
    fragShaderDesc.entryPoint = "main";

    GfxShader fragmentShader = nullptr;
    result = gfxDeviceCreateShader(device, &fragShaderDesc, &fragmentShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxVertexAttribute vertexAttr = {};
    vertexAttr.format = GFX_TEXTURE_FORMAT_R32G32B32_FLOAT;
    vertexAttr.offset = 0;
    vertexAttr.shaderLocation = 0;

    GfxVertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.arrayStride = 12;
    vertexBufferLayout.attributes = &vertexAttr;
    vertexBufferLayout.attributeCount = 1;
    vertexBufferLayout.stepMode = GFX_VERTEX_STEP_MODE_VERTEX;

    GfxVertexState vertexState = {};
    vertexState.module = vertexShader;
    vertexState.entryPoint = "main";
    vertexState.buffers = &vertexBufferLayout;
    vertexState.bufferCount = 1;

    GfxColorTargetState colorTargetState = {};
    colorTargetState.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTargetState.blend = nullptr;
    colorTargetState.writeMask = 0xF;

    GfxFragmentState fragmentState = {};
    fragmentState.module = fragmentShader;
    fragmentState.entryPoint = "main";
    fragmentState.targets = &colorTargetState;
    fragmentState.targetCount = 1;

    // Test triangle list topology
    GfxPrimitiveState primitiveState = {};
    primitiveState.topology = GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    primitiveState.stripIndexFormat = GFX_INDEX_FORMAT_UNDEFINED;
    primitiveState.frontFace = GFX_FRONT_FACE_COUNTER_CLOCKWISE;
    primitiveState.cullMode = GFX_CULL_MODE_NONE;
    primitiveState.polygonMode = GFX_POLYGON_MODE_FILL;

    GfxRenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = "Triangle List Pipeline";
    pipelineDesc.renderPass = renderPass;
    pipelineDesc.vertex = &vertexState;
    pipelineDesc.fragment = &fragmentState;
    pipelineDesc.primitive = &primitiveState;
    pipelineDesc.sampleCount = GFX_SAMPLE_COUNT_1;

    GfxRenderPipeline pipeline1 = nullptr;
    result = gfxDeviceCreateRenderPipeline(device, &pipelineDesc, &pipeline1);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(pipeline1, nullptr);

    // Test line list topology
    primitiveState.topology = GFX_PRIMITIVE_TOPOLOGY_LINE_LIST;
    pipelineDesc.label = "Line List Pipeline";

    GfxRenderPipeline pipeline2 = nullptr;
    result = gfxDeviceCreateRenderPipeline(device, &pipelineDesc, &pipeline2);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(pipeline2, nullptr);

    // Test point list topology
    primitiveState.topology = GFX_PRIMITIVE_TOPOLOGY_POINT_LIST;
    pipelineDesc.label = "Point List Pipeline";

    GfxRenderPipeline pipeline3 = nullptr;
    result = gfxDeviceCreateRenderPipeline(device, &pipelineDesc, &pipeline3);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(pipeline3, nullptr);

    gfxRenderPipelineDestroy(pipeline1);
    gfxRenderPipelineDestroy(pipeline2);
    gfxRenderPipelineDestroy(pipeline3);
    gfxShaderDestroy(fragmentShader);
    gfxShaderDestroy(vertexShader);
    gfxRenderPassDestroy(renderPass);
}

// Test: Create RenderPipeline with culling
TEST_P(GfxRenderPipelineTest, CreateRenderPipelineWithCulling)
{
    GfxRenderPassColorAttachmentTarget colorTarget = {};
    colorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTarget.ops.loadOp = GFX_LOAD_OP_CLEAR;
    colorTarget.ops.storeOp = GFX_STORE_OP_STORE;
    colorTarget.finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachment = {};
    colorAttachment.target = colorTarget;
    colorAttachment.resolveTarget = nullptr;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxShaderDescriptor shaderDesc = {};
    shaderDesc.label = "Test Vertex Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        shaderDesc.code = spirvVertexShader;
        shaderDesc.codeSize = sizeof(spirvVertexShader);
    } else {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        shaderDesc.code = wgslVertexShader;
        shaderDesc.codeSize = strlen(wgslVertexShader) + 1; // Include null terminator
    }
    shaderDesc.entryPoint = "main";

    GfxShader vertexShader = nullptr;
    result = gfxDeviceCreateShader(device, &shaderDesc, &vertexShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxShaderDescriptor fragShaderDesc = {};
    fragShaderDesc.label = "Test Fragment Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        fragShaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        fragShaderDesc.code = spirvFragmentShader;
        fragShaderDesc.codeSize = sizeof(spirvFragmentShader);
    } else {
        fragShaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        fragShaderDesc.code = wgslFragmentShader;
        fragShaderDesc.codeSize = strlen(wgslFragmentShader) + 1; // Include null terminator
    }
    fragShaderDesc.entryPoint = "main";

    GfxShader fragmentShader = nullptr;
    result = gfxDeviceCreateShader(device, &fragShaderDesc, &fragmentShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxVertexAttribute vertexAttr = {};
    vertexAttr.format = GFX_TEXTURE_FORMAT_R32G32B32_FLOAT;
    vertexAttr.offset = 0;
    vertexAttr.shaderLocation = 0;

    GfxVertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.arrayStride = 12;
    vertexBufferLayout.attributes = &vertexAttr;
    vertexBufferLayout.attributeCount = 1;
    vertexBufferLayout.stepMode = GFX_VERTEX_STEP_MODE_VERTEX;

    GfxVertexState vertexState = {};
    vertexState.module = vertexShader;
    vertexState.entryPoint = "main";
    vertexState.buffers = &vertexBufferLayout;
    vertexState.bufferCount = 1;

    GfxColorTargetState colorTargetState = {};
    colorTargetState.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTargetState.blend = nullptr;
    colorTargetState.writeMask = 0xF;

    GfxFragmentState fragmentState = {};
    fragmentState.module = fragmentShader;
    fragmentState.entryPoint = "main";
    fragmentState.targets = &colorTargetState;
    fragmentState.targetCount = 1;

    // Test back face culling
    GfxPrimitiveState primitiveState = {};
    primitiveState.topology = GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    primitiveState.stripIndexFormat = GFX_INDEX_FORMAT_UNDEFINED;
    primitiveState.frontFace = GFX_FRONT_FACE_COUNTER_CLOCKWISE;
    primitiveState.cullMode = GFX_CULL_MODE_BACK;
    primitiveState.polygonMode = GFX_POLYGON_MODE_FILL;

    GfxRenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = "Back Cull Pipeline";
    pipelineDesc.renderPass = renderPass;
    pipelineDesc.vertex = &vertexState;
    pipelineDesc.fragment = &fragmentState;
    pipelineDesc.primitive = &primitiveState;
    pipelineDesc.sampleCount = GFX_SAMPLE_COUNT_1;

    GfxRenderPipeline pipeline = nullptr;
    result = gfxDeviceCreateRenderPipeline(device, &pipelineDesc, &pipeline);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(pipeline, nullptr);

    gfxRenderPipelineDestroy(pipeline);
    gfxShaderDestroy(fragmentShader);
    gfxShaderDestroy(vertexShader);
    gfxRenderPassDestroy(renderPass);
}

// Test: Create RenderPipeline with depth stencil state
TEST_P(GfxRenderPipelineTest, CreateRenderPipelineWithDepthStencil)
{
    GfxRenderPassColorAttachmentTarget colorTarget = {};
    colorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTarget.ops.loadOp = GFX_LOAD_OP_CLEAR;
    colorTarget.ops.storeOp = GFX_STORE_OP_STORE;
    colorTarget.finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachment = {};
    colorAttachment.target = colorTarget;
    colorAttachment.resolveTarget = nullptr;

    GfxRenderPassDepthStencilAttachmentTarget depthTarget = {};
    depthTarget.format = GFX_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8;
    depthTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    depthTarget.depthOps.loadOp = GFX_LOAD_OP_CLEAR;
    depthTarget.depthOps.storeOp = GFX_STORE_OP_STORE;
    depthTarget.stencilOps.loadOp = GFX_LOAD_OP_CLEAR;
    depthTarget.stencilOps.storeOp = GFX_STORE_OP_STORE;
    depthTarget.finalLayout = GFX_TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;

    GfxRenderPassDepthStencilAttachment depthAttachment = {};
    depthAttachment.target = depthTarget;
    depthAttachment.resolveTarget = nullptr;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.depthStencilAttachment = &depthAttachment;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxShaderDescriptor shaderDesc = {};
    shaderDesc.label = "Test Vertex Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        shaderDesc.code = spirvVertexShader;
        shaderDesc.codeSize = sizeof(spirvVertexShader);
    } else {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        shaderDesc.code = wgslVertexShader;
        shaderDesc.codeSize = strlen(wgslVertexShader) + 1; // Include null terminator
    }
    shaderDesc.entryPoint = "main";

    GfxShader vertexShader = nullptr;
    result = gfxDeviceCreateShader(device, &shaderDesc, &vertexShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxShaderDescriptor fragShaderDesc = {};
    fragShaderDesc.label = "Test Fragment Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        fragShaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        fragShaderDesc.code = spirvFragmentShader;
        fragShaderDesc.codeSize = sizeof(spirvFragmentShader);
    } else {
        fragShaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        fragShaderDesc.code = wgslFragmentShader;
        fragShaderDesc.codeSize = strlen(wgslFragmentShader) + 1; // Include null terminator
    }
    fragShaderDesc.entryPoint = "main";

    GfxShader fragmentShader = nullptr;
    result = gfxDeviceCreateShader(device, &fragShaderDesc, &fragmentShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxVertexAttribute vertexAttr = {};
    vertexAttr.format = GFX_TEXTURE_FORMAT_R32G32B32_FLOAT;
    vertexAttr.offset = 0;
    vertexAttr.shaderLocation = 0;

    GfxVertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.arrayStride = 12;
    vertexBufferLayout.attributes = &vertexAttr;
    vertexBufferLayout.attributeCount = 1;
    vertexBufferLayout.stepMode = GFX_VERTEX_STEP_MODE_VERTEX;

    GfxVertexState vertexState = {};
    vertexState.module = vertexShader;
    vertexState.entryPoint = "main";
    vertexState.buffers = &vertexBufferLayout;
    vertexState.bufferCount = 1;

    GfxColorTargetState colorTargetState = {};
    colorTargetState.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTargetState.blend = nullptr;
    colorTargetState.writeMask = 0xF;

    GfxFragmentState fragmentState = {};
    fragmentState.module = fragmentShader;
    fragmentState.entryPoint = "main";
    fragmentState.targets = &colorTargetState;
    fragmentState.targetCount = 1;

    GfxPrimitiveState primitiveState = {};
    primitiveState.topology = GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    primitiveState.stripIndexFormat = GFX_INDEX_FORMAT_UNDEFINED;
    primitiveState.frontFace = GFX_FRONT_FACE_COUNTER_CLOCKWISE;
    primitiveState.cullMode = GFX_CULL_MODE_NONE;
    primitiveState.polygonMode = GFX_POLYGON_MODE_FILL;

    GfxDepthStencilState depthStencilState = {};
    depthStencilState.format = GFX_TEXTURE_FORMAT_DEPTH24_PLUS_STENCIL8;
    depthStencilState.depthWriteEnabled = true;
    depthStencilState.depthCompare = GFX_COMPARE_FUNCTION_LESS;
    depthStencilState.stencilReadMask = 0xFF;
    depthStencilState.stencilWriteMask = 0xFF;

    GfxRenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = "Depth Stencil Pipeline";
    pipelineDesc.renderPass = renderPass;
    pipelineDesc.vertex = &vertexState;
    pipelineDesc.fragment = &fragmentState;
    pipelineDesc.primitive = &primitiveState;
    pipelineDesc.depthStencil = &depthStencilState;
    pipelineDesc.sampleCount = GFX_SAMPLE_COUNT_1;

    GfxRenderPipeline pipeline = nullptr;
    result = gfxDeviceCreateRenderPipeline(device, &pipelineDesc, &pipeline);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(pipeline, nullptr);

    gfxRenderPipelineDestroy(pipeline);
    gfxShaderDestroy(fragmentShader);
    gfxShaderDestroy(vertexShader);
    gfxRenderPassDestroy(renderPass);
}

// Test: Create RenderPipeline with bind group layouts
TEST_P(GfxRenderPipelineTest, CreateRenderPipelineWithBindGroupLayouts)
{
    GfxRenderPassColorAttachmentTarget colorTarget = {};
    colorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTarget.ops.loadOp = GFX_LOAD_OP_CLEAR;
    colorTarget.ops.storeOp = GFX_STORE_OP_STORE;
    colorTarget.finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachment = {};
    colorAttachment.target = colorTarget;
    colorAttachment.resolveTarget = nullptr;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Create bind group layout
    GfxBindGroupLayoutEntry layoutEntry = {};
    layoutEntry.binding = 0;
    layoutEntry.visibility = GFX_SHADER_STAGE_VERTEX;
    layoutEntry.type = GFX_BINDING_TYPE_BUFFER;
    layoutEntry.buffer.hasDynamicOffset = false;
    layoutEntry.buffer.minBindingSize = 0;

    GfxBindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.entries = &layoutEntry;
    layoutDesc.entryCount = 1;

    GfxBindGroupLayout bindGroupLayout = nullptr;
    result = gfxDeviceCreateBindGroupLayout(device, &layoutDesc, &bindGroupLayout);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxShaderDescriptor shaderDesc = {};
    shaderDesc.label = "Test Vertex Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        shaderDesc.code = spirvVertexShader;
        shaderDesc.codeSize = sizeof(spirvVertexShader);
    } else {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        shaderDesc.code = wgslVertexShader;
        shaderDesc.codeSize = strlen(wgslVertexShader) + 1; // Include null terminator
    }
    shaderDesc.entryPoint = "main";

    GfxShader vertexShader = nullptr;
    result = gfxDeviceCreateShader(device, &shaderDesc, &vertexShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxShaderDescriptor fragShaderDesc = {};
    fragShaderDesc.label = "Test Fragment Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        fragShaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        fragShaderDesc.code = spirvFragmentShader;
        fragShaderDesc.codeSize = sizeof(spirvFragmentShader);
    } else {
        fragShaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        fragShaderDesc.code = wgslFragmentShader;
        fragShaderDesc.codeSize = strlen(wgslFragmentShader) + 1; // Include null terminator
    }
    fragShaderDesc.entryPoint = "main";

    GfxShader fragmentShader = nullptr;
    result = gfxDeviceCreateShader(device, &fragShaderDesc, &fragmentShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxVertexAttribute vertexAttr = {};
    vertexAttr.format = GFX_TEXTURE_FORMAT_R32G32B32_FLOAT;
    vertexAttr.offset = 0;
    vertexAttr.shaderLocation = 0;

    GfxVertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.arrayStride = 12;
    vertexBufferLayout.attributes = &vertexAttr;
    vertexBufferLayout.attributeCount = 1;
    vertexBufferLayout.stepMode = GFX_VERTEX_STEP_MODE_VERTEX;

    GfxVertexState vertexState = {};
    vertexState.module = vertexShader;
    vertexState.entryPoint = "main";
    vertexState.buffers = &vertexBufferLayout;
    vertexState.bufferCount = 1;

    GfxColorTargetState colorTargetState = {};
    colorTargetState.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTargetState.blend = nullptr;
    colorTargetState.writeMask = 0xF;

    GfxFragmentState fragmentState = {};
    fragmentState.module = fragmentShader;
    fragmentState.entryPoint = "main";
    fragmentState.targets = &colorTargetState;
    fragmentState.targetCount = 1;

    GfxPrimitiveState primitiveState = {};
    primitiveState.topology = GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    primitiveState.stripIndexFormat = GFX_INDEX_FORMAT_UNDEFINED;
    primitiveState.frontFace = GFX_FRONT_FACE_COUNTER_CLOCKWISE;
    primitiveState.cullMode = GFX_CULL_MODE_NONE;
    primitiveState.polygonMode = GFX_POLYGON_MODE_FILL;

    GfxRenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = "Pipeline With Bind Group";
    pipelineDesc.renderPass = renderPass;
    pipelineDesc.vertex = &vertexState;
    pipelineDesc.fragment = &fragmentState;
    pipelineDesc.primitive = &primitiveState;
    pipelineDesc.sampleCount = GFX_SAMPLE_COUNT_1;
    pipelineDesc.bindGroupLayouts = &bindGroupLayout;
    pipelineDesc.bindGroupLayoutCount = 1;

    GfxRenderPipeline pipeline = nullptr;
    result = gfxDeviceCreateRenderPipeline(device, &pipelineDesc, &pipeline);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(pipeline, nullptr);

    gfxRenderPipelineDestroy(pipeline);
    gfxBindGroupLayoutDestroy(bindGroupLayout);
    gfxShaderDestroy(fragmentShader);
    gfxShaderDestroy(vertexShader);
    gfxRenderPassDestroy(renderPass);
}

// Test: Create RenderPipeline with multiple vertex attributes
TEST_P(GfxRenderPipelineTest, CreateRenderPipelineWithMultipleVertexAttributes)
{
    GfxRenderPassColorAttachmentTarget colorTarget = {};
    colorTarget.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTarget.sampleCount = GFX_SAMPLE_COUNT_1;
    colorTarget.ops.loadOp = GFX_LOAD_OP_CLEAR;
    colorTarget.ops.storeOp = GFX_STORE_OP_STORE;
    colorTarget.finalLayout = GFX_TEXTURE_LAYOUT_COLOR_ATTACHMENT;

    GfxRenderPassColorAttachment colorAttachment = {};
    colorAttachment.target = colorTarget;
    colorAttachment.resolveTarget = nullptr;

    GfxRenderPassDescriptor renderPassDesc = {};
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.colorAttachmentCount = 1;

    GfxRenderPass renderPass = nullptr;
    GfxResult result = gfxDeviceCreateRenderPass(device, &renderPassDesc, &renderPass);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxShaderDescriptor shaderDesc = {};
    shaderDesc.label = "Test Vertex Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        shaderDesc.code = spirvVertexShader;
        shaderDesc.codeSize = sizeof(spirvVertexShader);
    } else {
        shaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        shaderDesc.code = wgslVertexShader;
        shaderDesc.codeSize = strlen(wgslVertexShader) + 1; // Include null terminator
    }
    shaderDesc.entryPoint = "main";

    GfxShader vertexShader = nullptr;
    result = gfxDeviceCreateShader(device, &shaderDesc, &vertexShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxShaderDescriptor fragShaderDesc = {};
    fragShaderDesc.label = "Test Fragment Shader";
    if (backend == GFX_BACKEND_VULKAN) {
        fragShaderDesc.sourceType = GFX_SHADER_SOURCE_SPIRV;
        fragShaderDesc.code = spirvFragmentShader;
        fragShaderDesc.codeSize = sizeof(spirvFragmentShader);
    } else {
        fragShaderDesc.sourceType = GFX_SHADER_SOURCE_WGSL;
        fragShaderDesc.code = wgslFragmentShader;
        fragShaderDesc.codeSize = strlen(wgslFragmentShader) + 1; // Include null terminator
    }
    fragShaderDesc.entryPoint = "main";

    GfxShader fragmentShader = nullptr;
    result = gfxDeviceCreateShader(device, &fragShaderDesc, &fragmentShader);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    GfxVertexAttribute vertexAttrs[3] = {};

    // Position attribute
    vertexAttrs[0].format = GFX_TEXTURE_FORMAT_R32G32B32_FLOAT;
    vertexAttrs[0].offset = 0;
    vertexAttrs[0].shaderLocation = 0;

    // Normal attribute
    vertexAttrs[1].format = GFX_TEXTURE_FORMAT_R32G32B32_FLOAT;
    vertexAttrs[1].offset = 12;
    vertexAttrs[1].shaderLocation = 1;

    // TexCoord attribute
    vertexAttrs[2].format = GFX_TEXTURE_FORMAT_R32G32_FLOAT;
    vertexAttrs[2].offset = 24;
    vertexAttrs[2].shaderLocation = 2;

    GfxVertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.arrayStride = 32;
    vertexBufferLayout.attributes = vertexAttrs;
    vertexBufferLayout.attributeCount = 3;
    vertexBufferLayout.stepMode = GFX_VERTEX_STEP_MODE_VERTEX;

    GfxVertexState vertexState = {};
    vertexState.module = vertexShader;
    vertexState.entryPoint = "main";
    vertexState.buffers = &vertexBufferLayout;
    vertexState.bufferCount = 1;

    GfxColorTargetState colorTargetState = {};
    colorTargetState.format = GFX_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    colorTargetState.blend = nullptr;
    colorTargetState.writeMask = 0xF;

    GfxFragmentState fragmentState = {};
    fragmentState.module = fragmentShader;
    fragmentState.entryPoint = "main";
    fragmentState.targets = &colorTargetState;
    fragmentState.targetCount = 1;

    GfxPrimitiveState primitiveState = {};
    primitiveState.topology = GFX_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    primitiveState.stripIndexFormat = GFX_INDEX_FORMAT_UNDEFINED;
    primitiveState.frontFace = GFX_FRONT_FACE_COUNTER_CLOCKWISE;
    primitiveState.cullMode = GFX_CULL_MODE_NONE;
    primitiveState.polygonMode = GFX_POLYGON_MODE_FILL;

    GfxRenderPipelineDescriptor pipelineDesc = {};
    pipelineDesc.label = "Multi Attribute Pipeline";
    pipelineDesc.renderPass = renderPass;
    pipelineDesc.vertex = &vertexState;
    pipelineDesc.fragment = &fragmentState;
    pipelineDesc.primitive = &primitiveState;
    pipelineDesc.sampleCount = GFX_SAMPLE_COUNT_1;

    GfxRenderPipeline pipeline = nullptr;
    result = gfxDeviceCreateRenderPipeline(device, &pipelineDesc, &pipeline);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(pipeline, nullptr);

    gfxRenderPipelineDestroy(pipeline);
    gfxShaderDestroy(fragmentShader);
    gfxShaderDestroy(vertexShader);
    gfxRenderPassDestroy(renderPass);
}

// Test: Destroy NULL RenderPipeline
TEST_P(GfxRenderPipelineTest, DestroyNullRenderPipeline)
{
    GfxResult result = gfxRenderPipelineDestroy(nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxRenderPipelineTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

} // namespace
