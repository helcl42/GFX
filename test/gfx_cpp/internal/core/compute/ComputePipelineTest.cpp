#include "../../common/CommonTest.h"

#include <core/compute/ComputePipeline.h>
#include <core/system/Device.h>

namespace gfx {

class ComputePipelineImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc{
            .backend = backend,
            .applicationName = "ComputePipelineImplTest"
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

// Minimal SPIR-V compute shader
static const uint32_t computeShaderCode[] = {
    0x07230203, 0x00010000, 0x00080001, 0x00000006, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0005000f, 0x00000005, 0x00000004, 0x6e69616d, 0x00000000, 0x00060010, 0x00000004, 0x00000011,
    0x00000001, 0x00000001, 0x00000001, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004,
    0x6e69616d, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00050036,
    0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x000100fd, 0x00010038
};

// WGSL compute shader
static const char* wgslComputeShader = R"(
@compute @workgroup_size(1, 1, 1)
fn main() {
}
)";

TEST_P(ComputePipelineImplTest, CreateComputePipeline)
{
    DeviceImpl deviceWrapper(device);

    // Create shader using DeviceImpl
    ShaderDescriptor shaderDesc{
        .sourceType = ShaderSourceType::SPIRV,
        .entryPoint = "main"
    };
    shaderDesc.code.assign(reinterpret_cast<const uint8_t*>(computeShaderCode),
        reinterpret_cast<const uint8_t*>(computeShaderCode) + sizeof(computeShaderCode));

    auto shader = deviceWrapper.createShader(shaderDesc);
    ASSERT_NE(shader, nullptr);

    // Create compute pipeline using DeviceImpl
    ComputePipelineDescriptor pipelineDesc{
        .compute = shader,
        .entryPoint = "main"
    };

    auto pipeline = deviceWrapper.createComputePipeline(pipelineDesc);
    EXPECT_NE(pipeline, nullptr);
}

TEST_P(ComputePipelineImplTest, MultipleComputePipelines_IndependentHandles)
{
    DeviceImpl deviceWrapper(device);

    // Create shader
    ShaderDescriptor shaderDesc{
        .sourceType = ShaderSourceType::SPIRV,
        .entryPoint = "main"
    };
    shaderDesc.code.assign(reinterpret_cast<const uint8_t*>(computeShaderCode),
        reinterpret_cast<const uint8_t*>(computeShaderCode) + sizeof(computeShaderCode));

    auto shader = deviceWrapper.createShader(shaderDesc);
    ASSERT_NE(shader, nullptr);

    // Create multiple pipelines
    ComputePipelineDescriptor pipelineDesc{
        .compute = shader,
        .entryPoint = "main"
    };

    auto pipeline1 = deviceWrapper.createComputePipeline(pipelineDesc);
    auto pipeline2 = deviceWrapper.createComputePipeline(pipelineDesc);

    EXPECT_NE(pipeline1, nullptr);
    EXPECT_NE(pipeline2, nullptr);
    EXPECT_NE(pipeline1, pipeline2);
}

TEST_P(ComputePipelineImplTest, CreateComputePipelineWithBindGroupLayouts)
{
    DeviceImpl deviceWrapper(device);

    // Create shader
    ShaderDescriptor shaderDesc{
        .sourceType = ShaderSourceType::SPIRV,
        .entryPoint = "main"
    };
    shaderDesc.code.assign(reinterpret_cast<const uint8_t*>(computeShaderCode),
        reinterpret_cast<const uint8_t*>(computeShaderCode) + sizeof(computeShaderCode));

    auto shader = deviceWrapper.createShader(shaderDesc);
    ASSERT_NE(shader, nullptr);

    // Create bind group layout
    BindGroupLayoutDescriptor layoutDesc{
        .entries = {
            BindGroupLayoutEntry{
                .binding = 0,
                .visibility = ShaderStage::Compute,
                .resource = BindGroupLayoutEntry::BufferBinding{

                    .hasDynamicOffset = false,
                    .minBindingSize = 0 } } }
    };

    auto bindGroupLayout = deviceWrapper.createBindGroupLayout(layoutDesc);
    ASSERT_NE(bindGroupLayout, nullptr);

    // Create compute pipeline with bind group layout
    ComputePipelineDescriptor pipelineDesc{
        .compute = shader,
        .entryPoint = "main",
        .bindGroupLayouts = { bindGroupLayout }
    };

    auto pipeline = deviceWrapper.createComputePipeline(pipelineDesc);
    EXPECT_NE(pipeline, nullptr);
}

TEST_P(ComputePipelineImplTest, CreateComputePipelineWithMultipleBindGroupLayouts)
{
    DeviceImpl deviceWrapper(device);

    // Create shader
    ShaderDescriptor shaderDesc{
        .sourceType = ShaderSourceType::SPIRV,
        .entryPoint = "main"
    };
    shaderDesc.code.assign(reinterpret_cast<const uint8_t*>(computeShaderCode),
        reinterpret_cast<const uint8_t*>(computeShaderCode) + sizeof(computeShaderCode));

    auto shader = deviceWrapper.createShader(shaderDesc);
    ASSERT_NE(shader, nullptr);

    // Create first bind group layout
    BindGroupLayoutDescriptor layoutDesc1{
        .entries = {
            BindGroupLayoutEntry{
                .binding = 0,
                .visibility = ShaderStage::Compute,
                .resource = BindGroupLayoutEntry::BufferBinding{
                    .hasDynamicOffset = false,
                    .minBindingSize = 0 } } }
    };

    auto bindGroupLayout1 = deviceWrapper.createBindGroupLayout(layoutDesc1);
    ASSERT_NE(bindGroupLayout1, nullptr);

    // Create second bind group layout
    BindGroupLayoutDescriptor layoutDesc2{
        .entries = {
            BindGroupLayoutEntry{
                .binding = 0,
                .visibility = ShaderStage::Compute,
                .resource = BindGroupLayoutEntry::BufferBinding{
                    .hasDynamicOffset = false,
                    .minBindingSize = 0 } } }
    };

    auto bindGroupLayout2 = deviceWrapper.createBindGroupLayout(layoutDesc2);
    ASSERT_NE(bindGroupLayout2, nullptr);

    // Create compute pipeline with multiple bind group layouts
    ComputePipelineDescriptor pipelineDesc{
        .compute = shader,
        .entryPoint = "main",
        .bindGroupLayouts = { bindGroupLayout1, bindGroupLayout2 }
    };

    auto pipeline = deviceWrapper.createComputePipeline(pipelineDesc);
    EXPECT_NE(pipeline, nullptr);
}

TEST_P(ComputePipelineImplTest, CreateComputePipelineWithWGSLShader)
{
    // WGSL only works on WebGPU
    if (backend != GFX_BACKEND_WEBGPU) {
        GTEST_SKIP() << "WGSL shaders only supported on WebGPU backend";
    }

    DeviceImpl deviceWrapper(device);

    // Create shader (WGSL)
    ShaderDescriptor shaderDesc{
        .sourceType = ShaderSourceType::WGSL,
        .entryPoint = "main"
    };
    shaderDesc.code.assign(reinterpret_cast<const uint8_t*>(wgslComputeShader),
        reinterpret_cast<const uint8_t*>(wgslComputeShader) + strlen(wgslComputeShader));

    auto shader = deviceWrapper.createShader(shaderDesc);
    ASSERT_NE(shader, nullptr);

    // Create compute pipeline with WGSL shader
    ComputePipelineDescriptor pipelineDesc{
        .compute = shader,
        .entryPoint = "main"
    };

    auto pipeline = deviceWrapper.createComputePipeline(pipelineDesc);
    EXPECT_NE(pipeline, nullptr);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    ComputePipelineImplTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

} // namespace gfx
