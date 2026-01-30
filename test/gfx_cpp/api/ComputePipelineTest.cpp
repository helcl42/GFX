#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

#include <memory>

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxCppComputePipelineTest : public testing::TestWithParam<gfx::Backend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        try {
            gfx::InstanceDescriptor instDesc{
                .backend = backend,
                .enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG }
            };

            instance = gfx::createInstance(instDesc);

            gfx::AdapterDescriptor adapterDesc{
                .adapterIndex = 0
            };

            adapter = instance->requestAdapter(adapterDesc);

            gfx::DeviceDescriptor deviceDesc{
                .label = "Test Device"
            };

            device = adapter->createDevice(deviceDesc);
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
static const uint32_t spirvComputeShader[] = {
    0x07230203, 0x00010000, 0x0008000b, 0x0000000b, 0x00000000, 0x00020011,
    0x00000001, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
    0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0005000f, 0x00000005,
    0x00000004, 0x6e69616d, 0x00000000, 0x00060010, 0x00000004, 0x00000011,
    0x00000040, 0x00000001, 0x00000001, 0x00030003, 0x00000002, 0x000001c2,
    0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00040047, 0x0000000a,
    0x0000000b, 0x00000019, 0x00020013, 0x00000002, 0x00030021, 0x00000003,
    0x00000002, 0x00040015, 0x00000006, 0x00000020, 0x00000000, 0x00040017,
    0x00000007, 0x00000006, 0x00000003, 0x0004002b, 0x00000006, 0x00000008,
    0x00000040, 0x0004002b, 0x00000006, 0x00000009, 0x00000001, 0x0006002c,
    0x00000007, 0x0000000a, 0x00000008, 0x00000009, 0x00000009, 0x00050036,
    0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005,
    0x000100fd, 0x00010038,
};

// NULL parameter validation tests
TEST_P(GfxCppComputePipelineTest, CreateComputePipelineWithNullShader)
{
    ASSERT_NE(device, nullptr);

    gfx::ComputePipelineDescriptor pipelineDesc{
        .label = "Test Compute Pipeline",
        .compute = nullptr,
        .entryPoint = "main"
    };

    // Null shader should throw
    EXPECT_THROW(device->createComputePipeline(pipelineDesc), std::invalid_argument);
}

TEST_P(GfxCppComputePipelineTest, CreateBasicComputePipeline)
{
    ASSERT_NE(device, nullptr);

    gfx::ShaderDescriptor shaderDesc{
        .label = "Test Compute Shader",
        .sourceType = (backend == gfx::Backend::Vulkan) ? gfx::ShaderSourceType::SPIRV : gfx::ShaderSourceType::WGSL,
        .code = (backend == gfx::Backend::Vulkan) 
            ? std::string(reinterpret_cast<const char*>(spirvComputeShader), sizeof(spirvComputeShader)) 
            : std::string(wgslComputeShader),
        .entryPoint = "main"
    };

    auto computeShader = device->createShader(shaderDesc);
    ASSERT_NE(computeShader, nullptr);

    gfx::ComputePipelineDescriptor pipelineDesc{
        .label = "Basic Compute Pipeline",
        .compute = computeShader,
        .entryPoint = "main"
    };

    auto pipeline = device->createComputePipeline(pipelineDesc);
    EXPECT_NE(pipeline, nullptr);
}

TEST_P(GfxCppComputePipelineTest, CreateComputePipelineWithEmptyLabel)
{
    ASSERT_NE(device, nullptr);

    gfx::ShaderDescriptor shaderDesc{
        .label = "Test Compute Shader",
        .sourceType = (backend == gfx::Backend::Vulkan) ? gfx::ShaderSourceType::SPIRV : gfx::ShaderSourceType::WGSL,
        .code = (backend == gfx::Backend::Vulkan) ? std::string(reinterpret_cast<const char*>(spirvComputeShader), sizeof(spirvComputeShader)) : std::string(wgslComputeShader),
        .entryPoint = "main"
    };

    auto computeShader = device->createShader(shaderDesc);
    ASSERT_NE(computeShader, nullptr);

    gfx::ComputePipelineDescriptor pipelineDesc{
        .label = "",
        .compute = computeShader,
        .entryPoint = "main"
    };

    auto pipeline = device->createComputePipeline(pipelineDesc);
    EXPECT_NE(pipeline, nullptr);
}

TEST_P(GfxCppComputePipelineTest, CreateComputePipelineWithBindGroupLayouts)
{
    ASSERT_NE(device, nullptr);

    // Create a storage buffer bind group layout
    gfx::BindGroupLayoutEntry entry{
        .binding = 0,
        .visibility = gfx::ShaderStage::Compute,
        .resource = gfx::BindGroupLayoutEntry::BufferBinding{
            .hasDynamicOffset = false,
            .minBindingSize = 0
        }
    };

    gfx::BindGroupLayoutDescriptor layoutDesc{
        .label = "Compute Bind Group Layout",
        .entries = { entry }
    };

    auto bindGroupLayout = device->createBindGroupLayout(layoutDesc);
    ASSERT_NE(bindGroupLayout, nullptr);

    // Create compute shader
    gfx::ShaderDescriptor shaderDesc{
        .label = "Test Compute Shader",
        .sourceType = (backend == gfx::Backend::Vulkan) ? gfx::ShaderSourceType::SPIRV : gfx::ShaderSourceType::WGSL,
        .code = (backend == gfx::Backend::Vulkan) ? std::string(reinterpret_cast<const char*>(spirvComputeShader), sizeof(spirvComputeShader)) : std::string(wgslComputeShader),
        .entryPoint = "main"
    };

    auto computeShader = device->createShader(shaderDesc);
    ASSERT_NE(computeShader, nullptr);

    // Create compute pipeline with bind group layout
    gfx::ComputePipelineDescriptor pipelineDesc{
        .label = "Compute Pipeline with Bind Groups",
        .compute = computeShader,
        .entryPoint = "main",
        .bindGroupLayouts = { bindGroupLayout }
    };

    auto pipeline = device->createComputePipeline(pipelineDesc);
    EXPECT_NE(pipeline, nullptr);
}

TEST_P(GfxCppComputePipelineTest, CreateComputePipelineWithMultipleBindGroupLayouts)
{
    ASSERT_NE(device, nullptr);

    // Create first bind group layout (storage buffer)
    gfx::BindGroupLayoutEntry entry1{
        .binding = 0,
        .visibility = gfx::ShaderStage::Compute,
        .resource = gfx::BindGroupLayoutEntry::BufferBinding{
            .hasDynamicOffset = false,
            .minBindingSize = 0
        }
    };

    gfx::BindGroupLayoutDescriptor layoutDesc1{
        .label = "Storage Buffer Layout",
        .entries = { entry1 }
    };

    auto bindGroupLayout1 = device->createBindGroupLayout(layoutDesc1);
    ASSERT_NE(bindGroupLayout1, nullptr);

    // Create second bind group layout (uniform buffer)
    gfx::BindGroupLayoutEntry entry2{
        .binding = 0,
        .visibility = gfx::ShaderStage::Compute,
        .resource = gfx::BindGroupLayoutEntry::BufferBinding{
            .hasDynamicOffset = false,
            .minBindingSize = 0
        }
    };

    gfx::BindGroupLayoutDescriptor layoutDesc2{
        .label = "Uniform Buffer Layout",
        .entries = { entry2 }
    };

    auto bindGroupLayout2 = device->createBindGroupLayout(layoutDesc2);
    ASSERT_NE(bindGroupLayout2, nullptr);

    // Create compute shader
    gfx::ShaderDescriptor shaderDesc{
        .label = "Test Compute Shader",
        .sourceType = (backend == gfx::Backend::Vulkan) ? gfx::ShaderSourceType::SPIRV : gfx::ShaderSourceType::WGSL,
        .code = (backend == gfx::Backend::Vulkan) ? std::string(reinterpret_cast<const char*>(spirvComputeShader), sizeof(spirvComputeShader)) : std::string(wgslComputeShader),
        .entryPoint = "main"
    };

    auto computeShader = device->createShader(shaderDesc);
    ASSERT_NE(computeShader, nullptr);

    // Create pipeline with multiple bind group layouts
    gfx::ComputePipelineDescriptor pipelineDesc{
        .label = "Compute Pipeline with Multiple Bind Groups",
        .compute = computeShader,
        .entryPoint = "main",
        .bindGroupLayouts = { bindGroupLayout1, bindGroupLayout2 }
    };

    auto pipeline = device->createComputePipeline(pipelineDesc);
    EXPECT_NE(pipeline, nullptr);
}

TEST_P(GfxCppComputePipelineTest, CreateComputePipelineWithEmptyBindGroupLayouts)
{
    ASSERT_NE(device, nullptr);

    gfx::ShaderDescriptor shaderDesc{
        .label = "Test Compute Shader",
        .sourceType = (backend == gfx::Backend::Vulkan) ? gfx::ShaderSourceType::SPIRV : gfx::ShaderSourceType::WGSL,
        .code = (backend == gfx::Backend::Vulkan) ? std::string(reinterpret_cast<const char*>(spirvComputeShader), sizeof(spirvComputeShader)) : std::string(wgslComputeShader),
        .entryPoint = "main"
    };

    auto computeShader = device->createShader(shaderDesc);
    ASSERT_NE(computeShader, nullptr);

    gfx::ComputePipelineDescriptor pipelineDesc{
        .label = "Compute Pipeline with Empty Bind Groups",
        .compute = computeShader,
        .entryPoint = "main",
        .bindGroupLayouts = {}
    };

    auto pipeline = device->createComputePipeline(pipelineDesc);
    EXPECT_NE(pipeline, nullptr);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppComputePipelineTest,
    testing::Values(gfx::Backend::Vulkan, gfx::Backend::WebGPU),
    [](const testing::TestParamInfo<gfx::Backend>& info) {
        return info.param == gfx::Backend::Vulkan ? "Vulkan" : "WebGPU";
    });

} // namespace
