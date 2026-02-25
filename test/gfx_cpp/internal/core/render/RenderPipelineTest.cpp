#include "../../common/CommonTest.h"

#include <core/render/RenderPipeline.h>
#include <core/system/Device.h>

#include <cstring>

namespace gfx {

class RenderPipelineImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc{
            .backend = backend,
            .applicationName = "RenderPipelineImplTest"
        };
        ASSERT_EQ(gfxCreateInstance(&instanceDesc, &instance), GFX_RESULT_SUCCESS);

        GfxAdapterDescriptor adapterDesc{
            .sType = GFX_STRUCTURE_TYPE_ADAPTER_DESCRIPTOR,
            .pNext = nullptr,
            .adapterIndex = 0
        };
        ASSERT_EQ(gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter), GFX_RESULT_SUCCESS);

        GfxDeviceDescriptor deviceDesc{
            .sType = GFX_STRUCTURE_TYPE_DEVICE_DESCRIPTOR,
            .pNext = nullptr,
            .label = nullptr,
            .queueRequests = nullptr,
            .queueRequestCount = 0,
            .enabledExtensions = nullptr,
            .enabledExtensionCount = 0
        };
        ASSERT_EQ(gfxAdapterCreateDevice(adapter, &deviceDesc, &device), GFX_RESULT_SUCCESS);
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

    GfxBackend backend;
    GfxInstance instance = nullptr;
    GfxAdapter adapter = nullptr;
    GfxDevice device = nullptr;
};

// Minimal SPIR-V vertex shader (passthrough)
static const uint32_t vertexShaderCode[] = {
    0x07230203, 0x00010000, 0x00080001, 0x0000000d, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0007000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000b, 0x00030003,
    0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00060005, 0x00000009,
    0x565f6c67, 0x65747265, 0x646e4978, 0x00007865, 0x00060005, 0x0000000b, 0x505f6c67, 0x7469736f,
    0x006e6f69, 0x00000000, 0x00040047, 0x00000009, 0x0000000b, 0x0000002a, 0x00040047, 0x0000000b,
    0x0000000b, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00040015,
    0x00000006, 0x00000020, 0x00000001, 0x00040020, 0x00000007, 0x00000001, 0x00000006, 0x0004003b,
    0x00000007, 0x00000009, 0x00000001, 0x00030016, 0x00000008, 0x00000020, 0x00040017, 0x0000000a,
    0x00000008, 0x00000004, 0x00040020, 0x0000000c, 0x00000003, 0x0000000a, 0x0004003b, 0x0000000c,
    0x0000000b, 0x00000003, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8,
    0x00000005, 0x000100fd, 0x00010038
};

// Minimal SPIR-V fragment shader (outputs red)
static const uint32_t fragmentShaderCode[] = {
    0x07230203, 0x00010000, 0x00080001, 0x0000000d, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0006000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x00030010, 0x00000004,
    0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000,
    0x00050005, 0x00000009, 0x4374756f, 0x726f6c6f, 0x00000000, 0x00040047, 0x00000009, 0x0000001e,
    0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006,
    0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020, 0x00000008, 0x00000003,
    0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x0004002b, 0x00000006, 0x0000000a,
    0x3f800000, 0x0004002b, 0x00000006, 0x0000000b, 0x00000000, 0x0007002c, 0x00000007, 0x0000000c,
    0x0000000a, 0x0000000b, 0x0000000b, 0x0000000a, 0x00050036, 0x00000002, 0x00000004, 0x00000000,
    0x00000003, 0x000200f8, 0x00000005, 0x0003003e, 0x00000009, 0x0000000c, 0x000100fd, 0x00010038
};

// WGSL vertex shader
static const char* wgslVertexShader = R"(
@vertex
fn main(@builtin(vertex_index) vertexIndex: u32) -> @builtin(position) vec4<f32> {
    return vec4<f32>(0.0, 0.0, 0.0, 1.0);
}
)";

// WGSL fragment shader
static const char* wgslFragmentShader = R"(
@fragment
fn main() -> @location(0) vec4<f32> {
    return vec4<f32>(1.0, 0.0, 0.0, 1.0);
}
)";

TEST_P(RenderPipelineImplTest, CreateRenderPipeline)
{
    DeviceImpl deviceWrapper(device);

    // Create shader using DeviceImpl
    ShaderDescriptor shaderDesc{
        .sourceType = ShaderSourceType::SPIRV,
        .entryPoint = "main"
    };
    shaderDesc.code.assign(reinterpret_cast<const uint8_t*>(vertexShaderCode),
        reinterpret_cast<const uint8_t*>(vertexShaderCode) + sizeof(vertexShaderCode));

    auto shader = deviceWrapper.createShader(shaderDesc);
    ASSERT_NE(shader, nullptr);

    // Create render pass using DeviceImpl
    RenderPassCreateDescriptor renderPassDesc{
        .colorAttachments = {
            RenderPassColorAttachment{
                .target = {
                    .format = TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = SampleCount::Count1,
                    .ops = { LoadOp::Clear, StoreOp::Store },
                    .finalLayout = TextureLayout::ColorAttachment } } }
    };

    auto renderPass = deviceWrapper.createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create render pipeline using DeviceImpl
    RenderPipelineDescriptor pipelineDesc{
        .renderPass = renderPass,
        .vertex = {
            .module = shader,
            .entryPoint = "main",
            .buffers = {} },
        .primitive = { .topology = PrimitiveTopology::TriangleList, .frontFace = FrontFace::CounterClockwise, .cullMode = CullMode::None },
        .sampleCount = SampleCount::Count1
    };

    auto pipeline = deviceWrapper.createRenderPipeline(pipelineDesc);
    EXPECT_NE(pipeline, nullptr);
}

TEST_P(RenderPipelineImplTest, MultipleRenderPipelines_IndependentHandles)
{
    DeviceImpl deviceWrapper(device);

    // Create shader
    ShaderDescriptor shaderDesc{
        .sourceType = ShaderSourceType::SPIRV,
        .entryPoint = "main"
    };
    shaderDesc.code.assign(reinterpret_cast<const uint8_t*>(vertexShaderCode),
        reinterpret_cast<const uint8_t*>(vertexShaderCode) + sizeof(vertexShaderCode));

    auto shader = deviceWrapper.createShader(shaderDesc);
    ASSERT_NE(shader, nullptr);

    // Create render pass
    RenderPassCreateDescriptor renderPassDesc{
        .colorAttachments = {
            RenderPassColorAttachment{
                .target = {
                    .format = TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = SampleCount::Count1,
                    .ops = { LoadOp::Clear, StoreOp::Store },
                    .finalLayout = TextureLayout::ColorAttachment } } }
    };

    auto renderPass = deviceWrapper.createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create render pipeline descriptor
    RenderPipelineDescriptor pipelineDesc{
        .renderPass = renderPass,
        .vertex = {
            .module = shader,
            .entryPoint = "main",
            .buffers = {} },
        .primitive = { .topology = PrimitiveTopology::TriangleList, .frontFace = FrontFace::CounterClockwise, .cullMode = CullMode::None },
        .sampleCount = SampleCount::Count1
    };

    // Create two pipelines with same descriptor
    auto pipeline1 = deviceWrapper.createRenderPipeline(pipelineDesc);
    auto pipeline2 = deviceWrapper.createRenderPipeline(pipelineDesc);

    EXPECT_NE(pipeline1, nullptr);
    EXPECT_NE(pipeline2, nullptr);
    EXPECT_NE(pipeline1, pipeline2);
}

TEST_P(RenderPipelineImplTest, CreateRenderPipelineWithFragmentShader)
{
    DeviceImpl deviceWrapper(device);

    // Create vertex shader (SPIR-V)
    ShaderDescriptor vertexShaderDesc{
        .sourceType = ShaderSourceType::SPIRV,
        .entryPoint = "main"
    };
    vertexShaderDesc.code.assign(reinterpret_cast<const uint8_t*>(vertexShaderCode),
        reinterpret_cast<const uint8_t*>(vertexShaderCode) + sizeof(vertexShaderCode));

    auto vertexShader = deviceWrapper.createShader(vertexShaderDesc);
    ASSERT_NE(vertexShader, nullptr);

    // Create fragment shader (SPIR-V)
    ShaderDescriptor fragmentShaderDesc{
        .sourceType = ShaderSourceType::SPIRV,
        .entryPoint = "main"
    };
    fragmentShaderDesc.code.assign(reinterpret_cast<const uint8_t*>(fragmentShaderCode),
        reinterpret_cast<const uint8_t*>(fragmentShaderCode) + sizeof(fragmentShaderCode));

    auto fragmentShader = deviceWrapper.createShader(fragmentShaderDesc);
    ASSERT_NE(fragmentShader, nullptr);

    // Create render pass
    RenderPassCreateDescriptor renderPassDesc{
        .colorAttachments = {
            RenderPassColorAttachment{
                .target = {
                    .format = TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = SampleCount::Count1,
                    .ops = { LoadOp::Clear, StoreOp::Store },
                    .finalLayout = TextureLayout::ColorAttachment } } }
    };

    auto renderPass = deviceWrapper.createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create render pipeline with fragment shader
    RenderPipelineDescriptor pipelineDesc{
        .renderPass = renderPass,
        .vertex = {
            .module = vertexShader,
            .entryPoint = "main",
            .buffers = {} },
        .fragment = FragmentState{ .module = fragmentShader, .entryPoint = "main", .targets = { ColorTargetState{ .format = TextureFormat::R8G8B8A8Unorm, .writeMask = ColorWriteMask::All } } },
        .primitive = { .topology = PrimitiveTopology::TriangleList, .frontFace = FrontFace::CounterClockwise, .cullMode = CullMode::None },
        .sampleCount = SampleCount::Count1
    };

    auto pipeline = deviceWrapper.createRenderPipeline(pipelineDesc);
    EXPECT_NE(pipeline, nullptr);
}

TEST_P(RenderPipelineImplTest, CreateRenderPipelineWithWGSLShaders)
{
    // WGSL only works on WebGPU
    if (backend != GFX_BACKEND_WEBGPU) {
        GTEST_SKIP() << "WGSL shaders only supported on WebGPU backend";
    }

    DeviceImpl deviceWrapper(device);

    // Create vertex shader (WGSL)
    ShaderDescriptor vertexShaderDesc{
        .sourceType = ShaderSourceType::WGSL,
        .entryPoint = "main"
    };
    vertexShaderDesc.code.assign(reinterpret_cast<const uint8_t*>(wgslVertexShader),
        reinterpret_cast<const uint8_t*>(wgslVertexShader) + strlen(wgslVertexShader));

    auto vertexShader = deviceWrapper.createShader(vertexShaderDesc);
    ASSERT_NE(vertexShader, nullptr);

    // Create fragment shader (WGSL)
    ShaderDescriptor fragmentShaderDesc{
        .sourceType = ShaderSourceType::WGSL,
        .entryPoint = "main"
    };
    fragmentShaderDesc.code.assign(reinterpret_cast<const uint8_t*>(wgslFragmentShader),
        reinterpret_cast<const uint8_t*>(wgslFragmentShader) + strlen(wgslFragmentShader));

    auto fragmentShader = deviceWrapper.createShader(fragmentShaderDesc);
    ASSERT_NE(fragmentShader, nullptr);

    // Create render pass
    RenderPassCreateDescriptor renderPassDesc{
        .colorAttachments = {
            RenderPassColorAttachment{
                .target = {
                    .format = TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = SampleCount::Count1,
                    .ops = { LoadOp::Clear, StoreOp::Store },
                    .finalLayout = TextureLayout::ColorAttachment } } }
    };

    auto renderPass = deviceWrapper.createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create render pipeline with WGSL shaders
    RenderPipelineDescriptor pipelineDesc{
        .renderPass = renderPass,
        .vertex = {
            .module = vertexShader,
            .entryPoint = "main",
            .buffers = {} },
        .fragment = FragmentState{ .module = fragmentShader, .entryPoint = "main", .targets = { ColorTargetState{ .format = TextureFormat::R8G8B8A8Unorm, .writeMask = ColorWriteMask::All } } },
        .primitive = { .topology = PrimitiveTopology::TriangleList, .frontFace = FrontFace::CounterClockwise, .cullMode = CullMode::None },
        .sampleCount = SampleCount::Count1
    };

    auto pipeline = deviceWrapper.createRenderPipeline(pipelineDesc);
    EXPECT_NE(pipeline, nullptr);
}

TEST_P(RenderPipelineImplTest, CreateRenderPipelineWithMixedShaderFormats)
{
    // WGSL only works on WebGPU, so skip on Vulkan
    if (backend != GFX_BACKEND_WEBGPU) {
        GTEST_SKIP() << "Mixed shader formats test only valid on WebGPU backend";
    }

    DeviceImpl deviceWrapper(device);

    // Create vertex shader (WGSL)
    ShaderDescriptor vertexShaderDesc{
        .sourceType = ShaderSourceType::WGSL,
        .entryPoint = "main"
    };
    vertexShaderDesc.code.assign(reinterpret_cast<const uint8_t*>(wgslVertexShader),
        reinterpret_cast<const uint8_t*>(wgslVertexShader) + strlen(wgslVertexShader));

    auto vertexShader = deviceWrapper.createShader(vertexShaderDesc);
    ASSERT_NE(vertexShader, nullptr);

    // Create fragment shader (SPIR-V)
    ShaderDescriptor fragmentShaderDesc{
        .sourceType = ShaderSourceType::SPIRV,
        .entryPoint = "main"
    };
    fragmentShaderDesc.code.assign(reinterpret_cast<const uint8_t*>(fragmentShaderCode),
        reinterpret_cast<const uint8_t*>(fragmentShaderCode) + sizeof(fragmentShaderCode));

    auto fragmentShader = deviceWrapper.createShader(fragmentShaderDesc);
    ASSERT_NE(fragmentShader, nullptr);

    // Create render pass
    RenderPassCreateDescriptor renderPassDesc{
        .colorAttachments = {
            RenderPassColorAttachment{
                .target = {
                    .format = TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = SampleCount::Count1,
                    .ops = { LoadOp::Clear, StoreOp::Store },
                    .finalLayout = TextureLayout::ColorAttachment } } }
    };

    auto renderPass = deviceWrapper.createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create render pipeline with mixed shader formats (WGSL vertex, SPIRV fragment)
    RenderPipelineDescriptor pipelineDesc{
        .renderPass = renderPass,
        .vertex = {
            .module = vertexShader,
            .entryPoint = "main",
            .buffers = {} },
        .fragment = FragmentState{ .module = fragmentShader, .entryPoint = "main", .targets = { ColorTargetState{ .format = TextureFormat::R8G8B8A8Unorm, .writeMask = ColorWriteMask::All } } },
        .primitive = { .topology = PrimitiveTopology::TriangleList, .frontFace = FrontFace::CounterClockwise, .cullMode = CullMode::None },
        .sampleCount = SampleCount::Count1
    };

    auto pipeline = deviceWrapper.createRenderPipeline(pipelineDesc);
    EXPECT_NE(pipeline, nullptr);
}

TEST_P(RenderPipelineImplTest, CreateRenderPipelineWithMultisampling)
{
    DeviceImpl deviceWrapper(device);

    // Create vertex shader (SPIR-V)
    ShaderDescriptor vertexShaderDesc{
        .sourceType = ShaderSourceType::SPIRV,
        .entryPoint = "main"
    };
    vertexShaderDesc.code.assign(reinterpret_cast<const uint8_t*>(vertexShaderCode),
        reinterpret_cast<const uint8_t*>(vertexShaderCode) + sizeof(vertexShaderCode));

    auto vertexShader = deviceWrapper.createShader(vertexShaderDesc);
    ASSERT_NE(vertexShader, nullptr);

    // Create fragment shader (SPIR-V)
    ShaderDescriptor fragmentShaderDesc{
        .sourceType = ShaderSourceType::SPIRV,
        .entryPoint = "main"
    };
    fragmentShaderDesc.code.assign(reinterpret_cast<const uint8_t*>(fragmentShaderCode),
        reinterpret_cast<const uint8_t*>(fragmentShaderCode) + sizeof(fragmentShaderCode));

    auto fragmentShader = deviceWrapper.createShader(fragmentShaderDesc);
    ASSERT_NE(fragmentShader, nullptr);

    // Create render pass with MSAA x4
    RenderPassCreateDescriptor renderPassDesc{
        .colorAttachments = {
            RenderPassColorAttachment{
                .target = {
                    .format = TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = SampleCount::Count4,
                    .ops = { LoadOp::Clear, StoreOp::Store },
                    .finalLayout = TextureLayout::ColorAttachment } } }
    };

    auto renderPass = deviceWrapper.createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create render pipeline with MSAA x4
    RenderPipelineDescriptor pipelineDesc{
        .renderPass = renderPass,
        .vertex = {
            .module = vertexShader,
            .entryPoint = "main",
            .buffers = {} },
        .fragment = FragmentState{ .module = fragmentShader, .entryPoint = "main", .targets = { ColorTargetState{ .format = TextureFormat::R8G8B8A8Unorm, .writeMask = ColorWriteMask::All } } },
        .primitive = { .topology = PrimitiveTopology::TriangleList, .frontFace = FrontFace::CounterClockwise, .cullMode = CullMode::None },
        .sampleCount = SampleCount::Count4
    };

    auto pipeline = deviceWrapper.createRenderPipeline(pipelineDesc);
    EXPECT_NE(pipeline, nullptr);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    RenderPipelineImplTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

} // namespace gfx
