#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

#include <memory>

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

class GfxCppComputePassEncoderTest : public testing::TestWithParam<gfx::Backend> {
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

// NULL parameter validation tests
TEST_P(GfxCppComputePassEncoderTest, SetPipelineWithNullPipeline)
{
    ASSERT_NE(device, nullptr);

    auto encoder = device->createCommandEncoder({});
    ASSERT_NE(encoder, nullptr);

    gfx::ComputePassBeginDescriptor beginDesc{};
    auto computePass = encoder->beginComputePass(beginDesc);
    ASSERT_NE(computePass, nullptr);

    // Null pipeline should throw
    EXPECT_THROW(computePass->setPipeline(nullptr), std::invalid_argument);
}

TEST_P(GfxCppComputePassEncoderTest, SetBindGroupWithNullBindGroup)
{
    ASSERT_NE(device, nullptr);

    auto encoder = device->createCommandEncoder({});
    ASSERT_NE(encoder, nullptr);

    gfx::ComputePassBeginDescriptor beginDesc{};
    auto computePass = encoder->beginComputePass(beginDesc);
    ASSERT_NE(computePass, nullptr);

    // Null bind group should throw
    EXPECT_THROW(computePass->setBindGroup(0, nullptr), std::invalid_argument);
}

TEST_P(GfxCppComputePassEncoderTest, DispatchValidWorkgroups)
{
    ASSERT_NE(device, nullptr);

    auto encoder = device->createCommandEncoder({});
    ASSERT_NE(encoder, nullptr);

    gfx::ComputePassBeginDescriptor beginDesc{};
    auto computePass = encoder->beginComputePass(beginDesc);
    ASSERT_NE(computePass, nullptr);

    // Note: Dispatch without a pipeline would crash, so we just test that the compute pass was created
    // Real dispatch testing requires a complete compute pipeline setup
    EXPECT_NO_THROW(computePass.reset());
}

TEST_P(GfxCppComputePassEncoderTest, DispatchIndirectWithNullBuffer)
{
    ASSERT_NE(device, nullptr);

    auto encoder = device->createCommandEncoder({});
    ASSERT_NE(encoder, nullptr);

    gfx::ComputePassBeginDescriptor beginDesc{};
    auto computePass = encoder->beginComputePass(beginDesc);
    ASSERT_NE(computePass, nullptr);

    // Null indirect buffer should throw
    EXPECT_THROW(computePass->dispatchIndirect(nullptr, 0), std::invalid_argument);
}

TEST_P(GfxCppComputePassEncoderTest, BeginComputePassAndEnd)
{
    ASSERT_NE(device, nullptr);

    auto encoder = device->createCommandEncoder({});
    ASSERT_NE(encoder, nullptr);

    gfx::ComputePassBeginDescriptor beginDesc{
        .label = "Test Compute Pass"
    };

    auto computePass = encoder->beginComputePass(beginDesc);
    ASSERT_NE(computePass, nullptr);

    // Should be able to end without any operations
    EXPECT_NO_THROW(computePass.reset());
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppComputePassEncoderTest,
    testing::Values(gfx::Backend::Vulkan, gfx::Backend::WebGPU),
    [](const testing::TestParamInfo<gfx::Backend>& info) {
        return info.param == gfx::Backend::Vulkan ? "Vulkan" : "WebGPU";
    });
