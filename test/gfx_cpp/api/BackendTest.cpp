#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

// Test version query function

namespace {

TEST(GfxCppBackendTest, GetVersion)
{
    auto [major, minor, patch] = gfx::getVersion();

    // Verify expected version (1.0.0)
    EXPECT_EQ(major, 1u);
    EXPECT_EQ(minor, 0u);
    EXPECT_EQ(patch, 0u);
}

} // namespace
