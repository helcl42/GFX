#include "../../common/CommonTest.h"

#include <core/presentation/Swapchain.h>
#include <core/system/Device.h>

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

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    SwapchainImplTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

} // namespace gfx
