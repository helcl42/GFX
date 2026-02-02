#include <core/command/CommandEncoder.h>
#include <core/command/ComputePassEncoder.h>
#include <core/system/Device.h>

#include <gfx/gfx.h>

#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

namespace gfx {

class ComputePassEncoderImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc{
            .backend = backend,
            .applicationName = "ComputePassEncoderTest",
            .applicationVersion = 1
        };
        ASSERT_EQ(gfxCreateInstance(&instanceDesc, &instance), GFX_RESULT_SUCCESS);

        GfxAdapterDescriptor adapterDesc{
            .adapterIndex = 0
        };
        ASSERT_EQ(gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter), GFX_RESULT_SUCCESS);

        GfxDeviceDescriptor deviceDesc{};
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

TEST_P(ComputePassEncoderImplTest, BeginEndComputePass)
{
    DeviceImpl deviceWrapper(device);

    // Create command encoder
    CommandEncoderDescriptor encoderDesc{
        .label = "Compute Pass Test Encoder"
    };

    auto encoder = deviceWrapper.createCommandEncoder(encoderDesc);
    ASSERT_NE(encoder, nullptr);
    encoder->begin();

    // Begin compute pass
    {
        ComputePassBeginDescriptor cpBeginDesc{
            .label = "Test Compute Pass"
        };

        auto computePassEncoder = encoder->beginComputePass(cpBeginDesc);
        EXPECT_NE(computePassEncoder, nullptr);
    } // Compute pass encoder ends here (RAII)

    // End command encoder
    encoder->end();
}

// ============================================================================
// Null/Error Handling Tests - Skipped (test C API, not C++ implementation)
// ============================================================================

#if defined(GFX_ENABLE_VULKAN) && defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(AllBackends, ComputePassEncoderImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU));
#elif defined(GFX_ENABLE_VULKAN)
INSTANTIATE_TEST_SUITE_P(VulkanBackend, ComputePassEncoderImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN));
#elif defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(WebGPUBackend, ComputePassEncoderImplTest,
    ::testing::Values(GFX_BACKEND_WEBGPU));
#endif

} // namespace gfx
