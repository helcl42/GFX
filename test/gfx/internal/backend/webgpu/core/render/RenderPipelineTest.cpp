#include <backend/webgpu/core/render/RenderPipeline.h>
#include <backend/webgpu/core/resource/Shader.h>
#include <backend/webgpu/core/system/Device.h>
#include <backend/webgpu/core/system/Instance.h>

#include <gtest/gtest.h>

#include <cstring>
#include <memory>

namespace {

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

class WebGPURenderPipelineTest : public testing::Test {
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

TEST_F(WebGPURenderPipelineTest, CreateRenderPipeline_Minimal)
{
    gfx::backend::webgpu::core::ShaderCreateInfo vertexInfo{};
    vertexInfo.sourceType = gfx::backend::webgpu::core::ShaderSourceType::WGSL;
    vertexInfo.code = MINIMAL_VERTEX_WGSL;
    vertexInfo.codeSize = std::strlen(MINIMAL_VERTEX_WGSL);
    vertexInfo.entryPoint = "main";
    auto vertexShader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), vertexInfo);

    gfx::backend::webgpu::core::ShaderCreateInfo fragmentInfo{};
    fragmentInfo.sourceType = gfx::backend::webgpu::core::ShaderSourceType::WGSL;
    fragmentInfo.code = MINIMAL_FRAGMENT_WGSL;
    fragmentInfo.codeSize = std::strlen(MINIMAL_FRAGMENT_WGSL);
    fragmentInfo.entryPoint = "main";
    auto fragmentShader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), fragmentInfo);

    gfx::backend::webgpu::core::RenderPipelineCreateInfo createInfo{};
    createInfo.vertex.module = vertexShader->handle();
    createInfo.vertex.entryPoint = "main";

    gfx::backend::webgpu::core::FragmentState fragState{};
    fragState.module = fragmentShader->handle();
    fragState.entryPoint = "main";

    gfx::backend::webgpu::core::ColorTargetState colorTarget{};
    colorTarget.format = WGPUTextureFormat_RGBA8Unorm;
    fragState.targets.push_back(colorTarget);
    createInfo.fragment = fragState;

    createInfo.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    createInfo.primitive.frontFace = WGPUFrontFace_CCW;
    createInfo.primitive.cullMode = WGPUCullMode_None;
    createInfo.sampleCount = 1;

    auto pipeline = std::make_unique<gfx::backend::webgpu::core::RenderPipeline>(device.get(), createInfo);

    EXPECT_NE(pipeline->handle(), nullptr);
}

TEST_F(WebGPURenderPipelineTest, Handle_ReturnsValidWGPURenderPipeline)
{
    gfx::backend::webgpu::core::ShaderCreateInfo vertexInfo{};
    vertexInfo.sourceType = gfx::backend::webgpu::core::ShaderSourceType::WGSL;
    vertexInfo.code = MINIMAL_VERTEX_WGSL;
    vertexInfo.codeSize = std::strlen(MINIMAL_VERTEX_WGSL);
    vertexInfo.entryPoint = "main";
    auto vertexShader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), vertexInfo);

    gfx::backend::webgpu::core::ShaderCreateInfo fragmentInfo{};
    fragmentInfo.sourceType = gfx::backend::webgpu::core::ShaderSourceType::WGSL;
    fragmentInfo.code = MINIMAL_FRAGMENT_WGSL;
    fragmentInfo.codeSize = std::strlen(MINIMAL_FRAGMENT_WGSL);
    fragmentInfo.entryPoint = "main";
    auto fragmentShader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), fragmentInfo);

    gfx::backend::webgpu::core::RenderPipelineCreateInfo createInfo{};
    createInfo.vertex.module = vertexShader->handle();
    createInfo.vertex.entryPoint = "main";

    gfx::backend::webgpu::core::FragmentState fragState{};
    fragState.module = fragmentShader->handle();
    fragState.entryPoint = "main";

    gfx::backend::webgpu::core::ColorTargetState colorTarget{};
    colorTarget.format = WGPUTextureFormat_RGBA8Unorm;
    fragState.targets.push_back(colorTarget);
    createInfo.fragment = fragState;

    createInfo.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    createInfo.primitive.frontFace = WGPUFrontFace_CCW;
    createInfo.primitive.cullMode = WGPUCullMode_None;
    createInfo.sampleCount = 1;

    auto pipeline = std::make_unique<gfx::backend::webgpu::core::RenderPipeline>(device.get(), createInfo);

    WGPURenderPipeline handle = pipeline->handle();
    EXPECT_NE(handle, nullptr);
}

TEST_F(WebGPURenderPipelineTest, Destructor_CleansUpResources)
{
    gfx::backend::webgpu::core::ShaderCreateInfo vertexInfo{};
    vertexInfo.sourceType = gfx::backend::webgpu::core::ShaderSourceType::WGSL;
    vertexInfo.code = MINIMAL_VERTEX_WGSL;
    vertexInfo.codeSize = std::strlen(MINIMAL_VERTEX_WGSL);
    vertexInfo.entryPoint = "main";
    auto vertexShader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), vertexInfo);

    gfx::backend::webgpu::core::ShaderCreateInfo fragmentInfo{};
    fragmentInfo.sourceType = gfx::backend::webgpu::core::ShaderSourceType::WGSL;
    fragmentInfo.code = MINIMAL_FRAGMENT_WGSL;
    fragmentInfo.codeSize = std::strlen(MINIMAL_FRAGMENT_WGSL);
    fragmentInfo.entryPoint = "main";
    auto fragmentShader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), fragmentInfo);

    {
        gfx::backend::webgpu::core::RenderPipelineCreateInfo createInfo{};
        createInfo.vertex.module = vertexShader->handle();
        createInfo.vertex.entryPoint = "main";

        gfx::backend::webgpu::core::FragmentState fragState{};
        fragState.module = fragmentShader->handle();
        fragState.entryPoint = "main";

        gfx::backend::webgpu::core::ColorTargetState colorTarget{};
        colorTarget.format = WGPUTextureFormat_RGBA8Unorm;
        fragState.targets.push_back(colorTarget);
        createInfo.fragment = fragState;

        createInfo.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        createInfo.primitive.frontFace = WGPUFrontFace_CCW;
        createInfo.primitive.cullMode = WGPUCullMode_None;
        createInfo.sampleCount = 1;

        auto pipeline = std::make_unique<gfx::backend::webgpu::core::RenderPipeline>(device.get(), createInfo);
        EXPECT_NE(pipeline->handle(), nullptr);
    }

    // If we reach here without crashing, cleanup succeeded
    SUCCEED();
}

} // anonymous namespace
