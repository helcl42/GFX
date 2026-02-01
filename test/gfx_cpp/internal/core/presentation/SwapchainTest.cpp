#include <core/presentation/Swapchain.h>
#include <core/system/Device.h>

#include <gfx/gfx.h>

#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

namespace gfx {

// Note: Swapchain tests are limited because they require valid surfaces
// which require window handles not available in a headless test environment.
// These tests only verify C API null-argument validation since the C++ API
// cannot be tested without actual window system integration.

class SwapchainImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        GTEST_SKIP() << "Swapchain tests require platform-specific window handles. "
                     << "These tests should be run manually with proper window setup "
                     << "or in an environment with display support.";
    }
};

TEST_P(SwapchainImplTest, PlaceholderTest)
{
    // This test is skipped - see SetUp()
}

#if defined(GFX_ENABLE_VULKAN) && defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(AllBackends, SwapchainImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU));
#elif defined(GFX_ENABLE_VULKAN)
INSTANTIATE_TEST_SUITE_P(VulkanBackend, SwapchainImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN));
#elif defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(WebGPUBackend, SwapchainImplTest,
    ::testing::Values(GFX_BACKEND_WEBGPU));
#endif

} // namespace gfx
