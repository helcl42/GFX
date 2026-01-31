#include <backend/webgpu/core/compute/ComputePipeline.h>
#include <backend/webgpu/core/resource/BindGroupLayout.h>
#include <backend/webgpu/core/resource/Shader.h>
#include <backend/webgpu/core/system/Device.h>
#include <backend/webgpu/core/system/Instance.h>

#include <gtest/gtest.h>

#include <cstring>
#include <memory>

namespace {

// Minimal WGSL compute shader
static const char* MINIMAL_COMPUTE_WGSL = R"(
@compute @workgroup_size(1)
fn main() {
}
)";

class WebGPUComputePipelineTest : public testing::Test {
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

TEST_F(WebGPUComputePipelineTest, CreateComputePipeline_Minimal)
{
    gfx::backend::webgpu::core::ShaderCreateInfo shaderInfo{};
    shaderInfo.sourceType = gfx::backend::webgpu::core::ShaderSourceType::WGSL;
    shaderInfo.code = MINIMAL_COMPUTE_WGSL;
    shaderInfo.codeSize = std::strlen(MINIMAL_COMPUTE_WGSL);
    shaderInfo.entryPoint = "main";
    auto shader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), shaderInfo);

    gfx::backend::webgpu::core::ComputePipelineCreateInfo createInfo{};
    createInfo.module = shader->handle();
    createInfo.entryPoint = "main";

    auto pipeline = std::make_unique<gfx::backend::webgpu::core::ComputePipeline>(device.get(), createInfo);

    EXPECT_NE(pipeline->handle(), nullptr);
}

TEST_F(WebGPUComputePipelineTest, Handle_ReturnsValidWGPUComputePipeline)
{
    gfx::backend::webgpu::core::ShaderCreateInfo shaderInfo{};
    shaderInfo.sourceType = gfx::backend::webgpu::core::ShaderSourceType::WGSL;
    shaderInfo.code = MINIMAL_COMPUTE_WGSL;
    shaderInfo.codeSize = std::strlen(MINIMAL_COMPUTE_WGSL);
    shaderInfo.entryPoint = "main";
    auto shader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), shaderInfo);

    gfx::backend::webgpu::core::ComputePipelineCreateInfo createInfo{};
    createInfo.module = shader->handle();
    createInfo.entryPoint = "main";

    auto pipeline = std::make_unique<gfx::backend::webgpu::core::ComputePipeline>(device.get(), createInfo);

    WGPUComputePipeline handle = pipeline->handle();
    EXPECT_NE(handle, nullptr);
}

TEST_F(WebGPUComputePipelineTest, CreateComputePipeline_WithBindGroupLayout)
{
    // Create bind group layout
    gfx::backend::webgpu::core::BindGroupLayoutCreateInfo layoutInfo{};
    gfx::backend::webgpu::core::BindGroupLayoutEntry entry{};
    entry.binding = 0;
    entry.visibility = WGPUShaderStage_Compute;
    entry.bufferType = WGPUBufferBindingType_Storage;
    entry.bufferHasDynamicOffset = false;
    entry.bufferMinBindingSize = 0;
    layoutInfo.entries.push_back(entry);
    auto layout = std::make_unique<gfx::backend::webgpu::core::BindGroupLayout>(device.get(), layoutInfo);

    gfx::backend::webgpu::core::ShaderCreateInfo shaderInfo{};
    shaderInfo.sourceType = gfx::backend::webgpu::core::ShaderSourceType::WGSL;
    shaderInfo.code = MINIMAL_COMPUTE_WGSL;
    shaderInfo.codeSize = std::strlen(MINIMAL_COMPUTE_WGSL);
    shaderInfo.entryPoint = "main";
    auto shader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), shaderInfo);

    gfx::backend::webgpu::core::ComputePipelineCreateInfo createInfo{};
    createInfo.module = shader->handle();
    createInfo.entryPoint = "main";
    createInfo.bindGroupLayouts.push_back(layout->handle());

    auto pipeline = std::make_unique<gfx::backend::webgpu::core::ComputePipeline>(device.get(), createInfo);

    EXPECT_NE(pipeline->handle(), nullptr);
}

TEST_F(WebGPUComputePipelineTest, MultiplePipelines_CanCoexist)
{
    gfx::backend::webgpu::core::ShaderCreateInfo shaderInfo{};
    shaderInfo.sourceType = gfx::backend::webgpu::core::ShaderSourceType::WGSL;
    shaderInfo.code = MINIMAL_COMPUTE_WGSL;
    shaderInfo.codeSize = std::strlen(MINIMAL_COMPUTE_WGSL);
    shaderInfo.entryPoint = "main";
    auto shader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), shaderInfo);

    gfx::backend::webgpu::core::ComputePipelineCreateInfo createInfo{};
    createInfo.module = shader->handle();
    createInfo.entryPoint = "main";

    auto pipeline1 = std::make_unique<gfx::backend::webgpu::core::ComputePipeline>(device.get(), createInfo);
    auto pipeline2 = std::make_unique<gfx::backend::webgpu::core::ComputePipeline>(device.get(), createInfo);

    EXPECT_NE(pipeline1->handle(), nullptr);
    EXPECT_NE(pipeline2->handle(), nullptr);
    EXPECT_NE(pipeline1->handle(), pipeline2->handle());
}

TEST_F(WebGPUComputePipelineTest, Destructor_CleansUpResources)
{
    gfx::backend::webgpu::core::ShaderCreateInfo shaderInfo{};
    shaderInfo.sourceType = gfx::backend::webgpu::core::ShaderSourceType::WGSL;
    shaderInfo.code = MINIMAL_COMPUTE_WGSL;
    shaderInfo.codeSize = std::strlen(MINIMAL_COMPUTE_WGSL);
    shaderInfo.entryPoint = "main";
    auto shader = std::make_unique<gfx::backend::webgpu::core::Shader>(device.get(), shaderInfo);

    {
        gfx::backend::webgpu::core::ComputePipelineCreateInfo createInfo{};
        createInfo.module = shader->handle();
        createInfo.entryPoint = "main";

        auto pipeline = std::make_unique<gfx::backend::webgpu::core::ComputePipeline>(device.get(), createInfo);
        EXPECT_NE(pipeline->handle(), nullptr);
    }

    // If we reach here without crashing, cleanup succeeded
    SUCCEED();
}

} // anonymous namespace
