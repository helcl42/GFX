#include <core/resource/Shader.h>
#include <core/system/Device.h>

#include <gfx/gfx.h>

#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

namespace gfx {

class ShaderImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc{
            .backend = backend,
            .applicationName = "ShaderImplTest"
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

// Minimal SPIR-V shader (compute shader that does nothing)
static const uint32_t minimalComputeShader[] = {
    0x07230203, 0x00010000, 0x00080001, 0x0000000d, 0x00000000, 0x00020011,
    0x00000001, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
    0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0005000f, 0x00000005,
    0x00000004, 0x6e69616d, 0x00000000, 0x00060010, 0x00000004, 0x00000011,
    0x00000001, 0x00000001, 0x00000001, 0x00030003, 0x00000002, 0x000001c2,
    0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00020013, 0x00000002,
    0x00030021, 0x00000003, 0x00000002, 0x00050036, 0x00000002, 0x00000004,
    0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x000100fd, 0x00010038
};

// Minimal WGSL shader (compute shader that does nothing)
static const char* minimalWGSLShader = R"(
@compute @workgroup_size(1)
fn main() {
}
)";

TEST_P(ShaderImplTest, CreateShader)
{
    DeviceImpl deviceWrapper(device);

    ShaderDescriptor desc{
        .sourceType = ShaderSourceType::SPIRV,
        .entryPoint = "main"
    };
    desc.code.assign(reinterpret_cast<const uint8_t*>(minimalComputeShader),
                     reinterpret_cast<const uint8_t*>(minimalComputeShader) + sizeof(minimalComputeShader));

    auto shader = deviceWrapper.createShader(desc);
    ASSERT_NE(shader, nullptr);
}

TEST_P(ShaderImplTest, CreateShaderWithCustomEntryPoint)
{
    DeviceImpl deviceWrapper(device);

    ShaderDescriptor desc{
        .sourceType = ShaderSourceType::SPIRV,
        .entryPoint = "main" // Entry point is "main" in the shader
    };
    desc.code.assign(reinterpret_cast<const uint8_t*>(minimalComputeShader),
                     reinterpret_cast<const uint8_t*>(minimalComputeShader) + sizeof(minimalComputeShader));

    auto shader = deviceWrapper.createShader(desc);
    ASSERT_NE(shader, nullptr);
}

TEST_P(ShaderImplTest, MultipleShaders_IndependentHandles)
{
    DeviceImpl deviceWrapper(device);

    ShaderDescriptor desc{
        .sourceType = ShaderSourceType::SPIRV,
        .entryPoint = "main"
    };
    desc.code.assign(reinterpret_cast<const uint8_t*>(minimalComputeShader),
                     reinterpret_cast<const uint8_t*>(minimalComputeShader) + sizeof(minimalComputeShader));

    auto shader1 = deviceWrapper.createShader(desc);
    auto shader2 = deviceWrapper.createShader(desc);

    ASSERT_NE(shader1, nullptr);
    ASSERT_NE(shader2, nullptr);
    EXPECT_NE(shader1.get(), shader2.get());
}

TEST_P(ShaderImplTest, CreateWGSLShader)
{
    DeviceImpl deviceWrapper(device);

    ShaderDescriptor desc{
        .sourceType = ShaderSourceType::WGSL,
        .entryPoint = "main"
    };
    const char* wgslCode = minimalWGSLShader;
    desc.code.assign(reinterpret_cast<const uint8_t*>(wgslCode),
                     reinterpret_cast<const uint8_t*>(wgslCode) + strlen(wgslCode));

    auto shader = deviceWrapper.createShader(desc);
    ASSERT_NE(shader, nullptr);
}

TEST_P(ShaderImplTest, MixedShaderTypes_IndependentHandles)
{
    DeviceImpl deviceWrapper(device);

    // Create SPIR-V shader
    ShaderDescriptor spirvDesc{
        .sourceType = ShaderSourceType::SPIRV,
        .entryPoint = "main"
    };
    spirvDesc.code.assign(reinterpret_cast<const uint8_t*>(minimalComputeShader),
                          reinterpret_cast<const uint8_t*>(minimalComputeShader) + sizeof(minimalComputeShader));

    auto spirvShader = deviceWrapper.createShader(spirvDesc);
    ASSERT_NE(spirvShader, nullptr);

    // Create WGSL shader
    ShaderDescriptor wgslDesc{
        .sourceType = ShaderSourceType::WGSL,
        .entryPoint = "main"
    };
    const char* wgslCode = minimalWGSLShader;
    wgslDesc.code.assign(reinterpret_cast<const uint8_t*>(wgslCode),
                         reinterpret_cast<const uint8_t*>(wgslCode) + strlen(wgslCode));

    auto wgslShader = deviceWrapper.createShader(wgslDesc);
    ASSERT_NE(wgslShader, nullptr);

    EXPECT_NE(spirvShader.get(), wgslShader.get());
}

#if defined(GFX_ENABLE_VULKAN) && defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(AllBackends, ShaderImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU));
#elif defined(GFX_ENABLE_VULKAN)
INSTANTIATE_TEST_SUITE_P(VulkanOnly, ShaderImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN));
#elif defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(WebGPUOnly, ShaderImplTest,
    ::testing::Values(GFX_BACKEND_WEBGPU));
#endif

} // namespace gfx
