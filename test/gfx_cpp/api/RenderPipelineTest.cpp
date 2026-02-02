#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

#include <memory>

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxCppRenderPipelineTest : public testing::TestWithParam<gfx::Backend> {
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

// Simple WGSL shaders
static const char* wgslVertexShader = R"(
@vertex
fn main(@location(0) position: vec3<f32>) -> @builtin(position) vec4<f32> {
    return vec4<f32>(position, 1.0);
}
)";

static const char* wgslFragmentShader = R"(
@fragment
fn main() -> @location(0) vec4<f32> {
    return vec4<f32>(1.0, 0.0, 0.0, 1.0);
}
)";

// Simple SPIR-V shaders (as hex strings for simplicity in C++)
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

// Helper functions to convert shader code to vector<uint8_t>
inline std::vector<uint8_t> toShaderCode(const char* text)
{
    const uint8_t* begin = reinterpret_cast<const uint8_t*>(text);
    return std::vector<uint8_t>(begin, begin + strlen(text));
}

inline std::vector<uint8_t> toShaderCode(const uint32_t* spirv, size_t size)
{
    const uint8_t* begin = reinterpret_cast<const uint8_t*>(spirv);
    return std::vector<uint8_t>(begin, begin + size);
}

// ===========================================================================
// RenderPipeline Tests
// ===========================================================================

TEST_P(GfxCppRenderPipelineTest, CreateBasicRenderPipeline)
{
    // Create render pass
    gfx::RenderPassCreateDescriptor rpDesc{
        .colorAttachments = {
            gfx::RenderPassColorAttachment{
                .target = {
                    .format = gfx::TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = gfx::SampleCount::Count1,
                    .loadOp = gfx::LoadOp::Clear,
                    .storeOp = gfx::StoreOp::Store,
                    .finalLayout = gfx::TextureLayout::ColorAttachment } } }
    };
    auto renderPass = device->createRenderPass(rpDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create vertex shader
    gfx::ShaderDescriptor shaderDesc{
        .label = "Vertex Shader",
        .sourceType = backend == gfx::Backend::Vulkan ? gfx::ShaderSourceType::SPIRV : gfx::ShaderSourceType::WGSL,
        .code = backend == gfx::Backend::Vulkan
            ? toShaderCode(spirvVertexShader, sizeof(spirvVertexShader))
            : toShaderCode(wgslVertexShader),
        .entryPoint = "main"
    };
    auto vertexShader = device->createShader(shaderDesc);
    ASSERT_NE(vertexShader, nullptr);

    // Create pipeline
    gfx::RenderPipelineDescriptor pipelineDesc{
        .label = "Test Pipeline",
        .renderPass = renderPass,
        .vertex = {
            .module = vertexShader,
            .entryPoint = "main",
            .buffers = {
                gfx::VertexBufferLayout{
                    .arrayStride = 12,
                    .attributes = {
                        gfx::VertexAttribute{
                            .format = gfx::TextureFormat::R32G32B32Float,
                            .offset = 0,
                            .shaderLocation = 0 } } } } },
        .primitive = { .topology = gfx::PrimitiveTopology::TriangleList, .cullMode = gfx::CullMode::None }
    };

    auto pipeline = device->createRenderPipeline(pipelineDesc);
    EXPECT_NE(pipeline, nullptr);
}

TEST_P(GfxCppRenderPipelineTest, CreateRenderPipelineWithFragmentShader)
{
    // Create render pass
    auto renderPass = device->createRenderPass({ .colorAttachments = {
                                                     gfx::RenderPassColorAttachment{
                                                         .target = {
                                                             .format = gfx::TextureFormat::R8G8B8A8Unorm,
                                                             .sampleCount = gfx::SampleCount::Count1,
                                                             .loadOp = gfx::LoadOp::Clear,
                                                             .storeOp = gfx::StoreOp::Store,
                                                             .finalLayout = gfx::TextureLayout::ColorAttachment } } } });
    ASSERT_NE(renderPass, nullptr);

    // Create shaders
    auto vertexShader = device->createShader({ .label = "Vertex Shader",
        .sourceType = backend == gfx::Backend::Vulkan ? gfx::ShaderSourceType::SPIRV : gfx::ShaderSourceType::WGSL,
        .code = backend == gfx::Backend::Vulkan
            ? toShaderCode(spirvVertexShader, sizeof(spirvVertexShader))
            : toShaderCode(wgslVertexShader),
        .entryPoint = "main" });
    ASSERT_NE(vertexShader, nullptr);

    auto fragmentShader = device->createShader({ .label = "Fragment Shader",
        .sourceType = backend == gfx::Backend::Vulkan ? gfx::ShaderSourceType::SPIRV : gfx::ShaderSourceType::WGSL,
        .code = backend == gfx::Backend::Vulkan
            ? toShaderCode(spirvFragmentShader, sizeof(spirvFragmentShader))
            : toShaderCode(wgslFragmentShader),
        .entryPoint = "main" });
    ASSERT_NE(fragmentShader, nullptr);

    // Create pipeline with fragment shader
    auto pipeline = device->createRenderPipeline({ .label = "Pipeline with Fragment",
        .renderPass = renderPass,
        .vertex = {
            .module = vertexShader,
            .entryPoint = "main",
            .buffers = {
                gfx::VertexBufferLayout{
                    .arrayStride = 12,
                    .attributes = {
                        gfx::VertexAttribute{
                            .format = gfx::TextureFormat::R32G32B32Float,
                            .offset = 0,
                            .shaderLocation = 0 } } } } },
        .fragment = gfx::FragmentState{ .module = fragmentShader, .entryPoint = "main", .targets = { gfx::ColorTargetState{ .format = gfx::TextureFormat::R8G8B8A8Unorm, .writeMask = gfx::ColorWriteMask::All } } },
        .primitive = { .topology = gfx::PrimitiveTopology::TriangleList } });

    EXPECT_NE(pipeline, nullptr);
}

TEST_P(GfxCppRenderPipelineTest, CreateRenderPipelineWithNullVertexShader)
{
    auto renderPass = device->createRenderPass({ .colorAttachments = {
                                                     gfx::RenderPassColorAttachment{
                                                         .target = {
                                                             .format = gfx::TextureFormat::R8G8B8A8Unorm,
                                                             .sampleCount = gfx::SampleCount::Count1,
                                                             .loadOp = gfx::LoadOp::Clear,
                                                             .storeOp = gfx::StoreOp::Store,
                                                             .finalLayout = gfx::TextureLayout::ColorAttachment } } } });
    ASSERT_NE(renderPass, nullptr);

    // Try to create pipeline with null vertex shader
    EXPECT_THROW({ device->createRenderPipeline({ .renderPass = renderPass,
                       .vertex = {
                           .module = nullptr, // NULL shader
                           .entryPoint = "main" } }); }, std::exception);
}

TEST_P(GfxCppRenderPipelineTest, CreateRenderPipelineWithNullRenderPass)
{
    auto vertexShader = device->createShader({ .sourceType = backend == gfx::Backend::Vulkan ? gfx::ShaderSourceType::SPIRV : gfx::ShaderSourceType::WGSL,
        .code = backend == gfx::Backend::Vulkan
            ? toShaderCode(spirvVertexShader, sizeof(spirvVertexShader))
            : toShaderCode(wgslVertexShader),
        .entryPoint = "main" });
    ASSERT_NE(vertexShader, nullptr);

    // Try to create pipeline with null render pass
    EXPECT_THROW({ device->createRenderPipeline({ .renderPass = nullptr, // NULL render pass
                       .vertex = {
                           .module = vertexShader,
                           .entryPoint = "main" } }); }, std::exception);
}

TEST_P(GfxCppRenderPipelineTest, CreateRenderPipelineWithDifferentTopologies)
{
    auto renderPass = device->createRenderPass({ .colorAttachments = {
                                                     gfx::RenderPassColorAttachment{
                                                         .target = {
                                                             .format = gfx::TextureFormat::R8G8B8A8Unorm,
                                                             .sampleCount = gfx::SampleCount::Count1,
                                                             .loadOp = gfx::LoadOp::Clear,
                                                             .storeOp = gfx::StoreOp::Store,
                                                             .finalLayout = gfx::TextureLayout::ColorAttachment } } } });
    ASSERT_NE(renderPass, nullptr);

    auto vertexShader = device->createShader({ .sourceType = backend == gfx::Backend::Vulkan ? gfx::ShaderSourceType::SPIRV : gfx::ShaderSourceType::WGSL,
        .code = backend == gfx::Backend::Vulkan
            ? toShaderCode(spirvVertexShader, sizeof(spirvVertexShader))
            : toShaderCode(wgslVertexShader),
        .entryPoint = "main" });
    ASSERT_NE(vertexShader, nullptr);

    // Test different topologies
    std::vector<gfx::PrimitiveTopology> topologies = {
        gfx::PrimitiveTopology::PointList,
        gfx::PrimitiveTopology::LineList,
        gfx::PrimitiveTopology::LineStrip,
        gfx::PrimitiveTopology::TriangleList,
        gfx::PrimitiveTopology::TriangleStrip
    };

    for (auto topology : topologies) {
        auto pipeline = device->createRenderPipeline({ .renderPass = renderPass,
            .vertex = {
                .module = vertexShader,
                .entryPoint = "main",
                .buffers = {
                    gfx::VertexBufferLayout{
                        .arrayStride = 12,
                        .attributes = {
                            gfx::VertexAttribute{
                                .format = gfx::TextureFormat::R32G32B32Float,
                                .offset = 0,
                                .shaderLocation = 0 } } } } },
            .primitive = { .topology = topology } });

        EXPECT_NE(pipeline, nullptr);
    }
}

TEST_P(GfxCppRenderPipelineTest, CreateRenderPipelineWithCulling)
{
    auto renderPass = device->createRenderPass({ .colorAttachments = {
                                                     gfx::RenderPassColorAttachment{
                                                         .target = {
                                                             .format = gfx::TextureFormat::R8G8B8A8Unorm,
                                                             .sampleCount = gfx::SampleCount::Count1,
                                                             .loadOp = gfx::LoadOp::Clear,
                                                             .storeOp = gfx::StoreOp::Store,
                                                             .finalLayout = gfx::TextureLayout::ColorAttachment } } } });
    ASSERT_NE(renderPass, nullptr);

    auto vertexShader = device->createShader({ .sourceType = backend == gfx::Backend::Vulkan ? gfx::ShaderSourceType::SPIRV : gfx::ShaderSourceType::WGSL,
        .code = backend == gfx::Backend::Vulkan
            ? toShaderCode(spirvVertexShader, sizeof(spirvVertexShader))
            : toShaderCode(wgslVertexShader),
        .entryPoint = "main" });
    ASSERT_NE(vertexShader, nullptr);

    // Test back face culling
    auto pipelineBack = device->createRenderPipeline({ .renderPass = renderPass,
        .vertex = {
            .module = vertexShader,
            .entryPoint = "main",
            .buffers = {
                gfx::VertexBufferLayout{
                    .arrayStride = 12,
                    .attributes = {
                        gfx::VertexAttribute{
                            .format = gfx::TextureFormat::R32G32B32Float,
                            .offset = 0,
                            .shaderLocation = 0 } } } } },
        .primitive = { .topology = gfx::PrimitiveTopology::TriangleList, .cullMode = gfx::CullMode::Back } });
    EXPECT_NE(pipelineBack, nullptr);

    // Test front face culling
    auto pipelineFront = device->createRenderPipeline({ .renderPass = renderPass,
        .vertex = {
            .module = vertexShader,
            .entryPoint = "main",
            .buffers = {
                gfx::VertexBufferLayout{
                    .arrayStride = 12,
                    .attributes = {
                        gfx::VertexAttribute{
                            .format = gfx::TextureFormat::R32G32B32Float,
                            .offset = 0,
                            .shaderLocation = 0 } } } } },
        .primitive = { .topology = gfx::PrimitiveTopology::TriangleList, .cullMode = gfx::CullMode::Front } });
    EXPECT_NE(pipelineFront, nullptr);
}

TEST_P(GfxCppRenderPipelineTest, CreateRenderPipelineWithDepthStencil)
{
    gfx::RenderPassDepthStencilAttachment depthAttachment{
        .target = {
            .format = gfx::TextureFormat::Depth32Float,
            .sampleCount = gfx::SampleCount::Count1,
            .depthLoadOp = gfx::LoadOp::Clear,
            .depthStoreOp = gfx::StoreOp::Store,
            .stencilLoadOp = gfx::LoadOp::DontCare,
            .stencilStoreOp = gfx::StoreOp::DontCare,
            .finalLayout = gfx::TextureLayout::DepthStencilAttachment }
    };

    gfx::RenderPassCreateDescriptor rpDesc{
        .colorAttachments = {
            gfx::RenderPassColorAttachment{
                .target = {
                    .format = gfx::TextureFormat::R8G8B8A8Unorm,
                    .sampleCount = gfx::SampleCount::Count1,
                    .loadOp = gfx::LoadOp::Clear,
                    .storeOp = gfx::StoreOp::Store,
                    .finalLayout = gfx::TextureLayout::ColorAttachment } } },
        .depthStencilAttachment = depthAttachment
    };
    auto renderPass = device->createRenderPass(rpDesc);
    ASSERT_NE(renderPass, nullptr);

    auto vertexShader = device->createShader({ .sourceType = backend == gfx::Backend::Vulkan ? gfx::ShaderSourceType::SPIRV : gfx::ShaderSourceType::WGSL,
        .code = backend == gfx::Backend::Vulkan
            ? toShaderCode(spirvVertexShader, sizeof(spirvVertexShader))
            : toShaderCode(wgslVertexShader),
        .entryPoint = "main" });
    ASSERT_NE(vertexShader, nullptr);

    // Create pipeline with depth stencil
    gfx::RenderPipelineDescriptor pipelineDesc{
        .renderPass = renderPass,
        .vertex = {
            .module = vertexShader,
            .entryPoint = "main",
            .buffers = {
                gfx::VertexBufferLayout{
                    .arrayStride = 12,
                    .attributes = {
                        gfx::VertexAttribute{
                            .format = gfx::TextureFormat::R32G32B32Float,
                            .offset = 0,
                            .shaderLocation = 0 } } } } },
        .primitive = { .topology = gfx::PrimitiveTopology::TriangleList }
    };
    pipelineDesc.depthStencil = gfx::DepthStencilState{
        .format = gfx::TextureFormat::Depth32Float,
        .depthWriteEnabled = true,
        .depthCompare = gfx::CompareFunction::Less
    };
    auto pipeline = device->createRenderPipeline(pipelineDesc);

    EXPECT_NE(pipeline, nullptr);
}

TEST_P(GfxCppRenderPipelineTest, CreateRenderPipelineWithMultipleVertexAttributes)
{
    auto renderPass = device->createRenderPass({ .colorAttachments = {
                                                     gfx::RenderPassColorAttachment{
                                                         .target = {
                                                             .format = gfx::TextureFormat::R8G8B8A8Unorm,
                                                             .sampleCount = gfx::SampleCount::Count1,
                                                             .loadOp = gfx::LoadOp::Clear,
                                                             .storeOp = gfx::StoreOp::Store,
                                                             .finalLayout = gfx::TextureLayout::ColorAttachment } } } });
    ASSERT_NE(renderPass, nullptr);

    auto vertexShader = device->createShader({ .sourceType = backend == gfx::Backend::Vulkan ? gfx::ShaderSourceType::SPIRV : gfx::ShaderSourceType::WGSL,
        .code = backend == gfx::Backend::Vulkan
            ? toShaderCode(spirvVertexShader, sizeof(spirvVertexShader))
            : toShaderCode(wgslVertexShader),
        .entryPoint = "main" });
    ASSERT_NE(vertexShader, nullptr);

    // Create pipeline with multiple vertex attributes
    auto pipeline = device->createRenderPipeline({ .renderPass = renderPass,
        .vertex = {
            .module = vertexShader,
            .entryPoint = "main",
            .buffers = {
                gfx::VertexBufferLayout{
                    .arrayStride = 32, // position (12) + normal (12) + texcoord (8)
                    .attributes = {
                        gfx::VertexAttribute{
                            .format = gfx::TextureFormat::R32G32B32Float,
                            .offset = 0,
                            .shaderLocation = 0 }, // position
                        gfx::VertexAttribute{
                            .format = gfx::TextureFormat::R32G32B32Float,
                            .offset = 12,
                            .shaderLocation = 1 }, // normal
                        gfx::VertexAttribute{
                            .format = gfx::TextureFormat::R32G32Float,
                            .offset = 24,
                            .shaderLocation = 2 } // texcoord
                    } } } },
        .primitive = { .topology = gfx::PrimitiveTopology::TriangleList } });

    EXPECT_NE(pipeline, nullptr);
}

TEST_P(GfxCppRenderPipelineTest, CreateRenderPipelineWithSPIRVShaders)
{
    auto renderPass = device->createRenderPass({ .colorAttachments = { gfx::RenderPassColorAttachment{
                                                     .target = {
                                                         .format = gfx::TextureFormat::R8G8B8A8Unorm,
                                                         .sampleCount = gfx::SampleCount::Count1,
                                                         .loadOp = gfx::LoadOp::Clear,
                                                         .storeOp = gfx::StoreOp::Store,
                                                         .finalLayout = gfx::TextureLayout::ColorAttachment } } } });
    ASSERT_NE(renderPass, nullptr);

    // Create shaders using SPIR-V on both backends to test SPIR-V support
    auto vertexShader = device->createShader({ .label = "SPIR-V Vertex Shader",
        .sourceType = gfx::ShaderSourceType::SPIRV,
        .code = toShaderCode(spirvVertexShader, sizeof(spirvVertexShader)),
        .entryPoint = "main" });
    ASSERT_NE(vertexShader, nullptr);

    auto fragmentShader = device->createShader({ .label = "SPIR-V Fragment Shader",
        .sourceType = gfx::ShaderSourceType::SPIRV,
        .code = toShaderCode(spirvFragmentShader, sizeof(spirvFragmentShader)),
        .entryPoint = "main" });
    ASSERT_NE(fragmentShader, nullptr);

    // Create pipeline with SPIR-V shaders - should work on both Vulkan and WebGPU
    auto pipeline = device->createRenderPipeline({ .label = "SPIR-V Pipeline",
        .renderPass = renderPass,
        .vertex = {
            .module = vertexShader,
            .entryPoint = "main",
            .buffers = {
                gfx::VertexBufferLayout{
                    .arrayStride = 12,
                    .attributes = {
                        gfx::VertexAttribute{
                            .format = gfx::TextureFormat::R32G32B32Float,
                            .offset = 0,
                            .shaderLocation = 0 } } } } },
        .fragment = gfx::FragmentState{ .module = fragmentShader, .entryPoint = "main", .targets = { gfx::ColorTargetState{ .format = gfx::TextureFormat::R8G8B8A8Unorm, .writeMask = gfx::ColorWriteMask::All } } },
        .primitive = { .topology = gfx::PrimitiveTopology::TriangleList } });

    EXPECT_NE(pipeline, nullptr);
}

TEST_P(GfxCppRenderPipelineTest, CreateRenderPipelineWithBindGroupLayouts)
{
    auto renderPass = device->createRenderPass({ .colorAttachments = { gfx::RenderPassColorAttachment{
                                                     .target = {
                                                         .format = gfx::TextureFormat::R8G8B8A8Unorm,
                                                         .sampleCount = gfx::SampleCount::Count1,
                                                         .loadOp = gfx::LoadOp::Clear,
                                                         .storeOp = gfx::StoreOp::Store,
                                                         .finalLayout = gfx::TextureLayout::ColorAttachment } } } });
    ASSERT_NE(renderPass, nullptr);

    // Create bind group layout
    auto bindGroupLayout = device->createBindGroupLayout({ .entries = { gfx::BindGroupLayoutEntry{
                                                               .binding = 0,
                                                               .visibility = gfx::ShaderStage::Vertex,
                                                               .resource = gfx::BindGroupLayoutEntry::BufferBinding{
                                                                   .hasDynamicOffset = false,
                                                                   .minBindingSize = 0 } } } });
    ASSERT_NE(bindGroupLayout, nullptr);

    // Create shaders based on backend
    gfx::ShaderSourceType sourceType = backend == gfx::Backend::Vulkan ? gfx::ShaderSourceType::SPIRV : gfx::ShaderSourceType::WGSL;
    auto vertexCode = backend == gfx::Backend::Vulkan ? toShaderCode(spirvVertexShader, sizeof(spirvVertexShader)) : toShaderCode(wgslVertexShader);
    auto fragmentCode = backend == gfx::Backend::Vulkan ? toShaderCode(spirvFragmentShader, sizeof(spirvFragmentShader)) : toShaderCode(wgslFragmentShader);

    auto vertexShader = device->createShader({ .label = "Vertex Shader",
        .sourceType = sourceType,
        .code = vertexCode,
        .entryPoint = "main" });
    ASSERT_NE(vertexShader, nullptr);

    auto fragmentShader = device->createShader({ .label = "Fragment Shader",
        .sourceType = sourceType,
        .code = fragmentCode,
        .entryPoint = "main" });
    ASSERT_NE(fragmentShader, nullptr);

    // Create pipeline with bind group layouts
    auto pipeline = device->createRenderPipeline({ .label = "Pipeline With Bind Group",
        .renderPass = renderPass,
        .vertex = {
            .module = vertexShader,
            .entryPoint = "main",
            .buffers = {
                gfx::VertexBufferLayout{
                    .arrayStride = 12,
                    .attributes = {
                        gfx::VertexAttribute{
                            .format = gfx::TextureFormat::R32G32B32Float,
                            .offset = 0,
                            .shaderLocation = 0 } } } } },
        .fragment = gfx::FragmentState{ .module = fragmentShader, .entryPoint = "main", .targets = { gfx::ColorTargetState{ .format = gfx::TextureFormat::R8G8B8A8Unorm, .writeMask = gfx::ColorWriteMask::All } } },
        .primitive = { .topology = gfx::PrimitiveTopology::TriangleList, .frontFace = gfx::FrontFace::CounterClockwise, .cullMode = gfx::CullMode::None },
        .bindGroupLayouts = { bindGroupLayout } });

    EXPECT_NE(pipeline, nullptr);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppRenderPipelineTest,
    testing::Values(gfx::Backend::Vulkan, gfx::Backend::WebGPU),
    [](const testing::TestParamInfo<gfx::Backend>& info) {
        return info.param == gfx::Backend::Vulkan ? "Vulkan" : "WebGPU";
    });

} // namespace
