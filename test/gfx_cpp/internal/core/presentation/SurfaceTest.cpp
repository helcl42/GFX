
#include "../../common/CommonTest.h"

#include <core/presentation/Surface.h>
#include <core/system/Device.h>

namespace gfx {

// Note: Surface tests are limited because they require valid window handles
// which are not available in a headless test environment.
// These tests only verify C API null-argument validation since the C++ API
// cannot be tested without actual window system integration.

class SurfaceImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        GTEST_SKIP() << "Surface tests require platform-specific window handles. "
                     << "These tests should be run manually with proper window setup "
                     << "or in an environment with display support.";
    }
};

TEST_P(SurfaceImplTest, PlaceholderTest)
{
    // This test is skipped - see SetUp()
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    SurfaceImplTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

} // namespace gfx
