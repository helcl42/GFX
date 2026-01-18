#include <gfx_cpp/Gfx.hpp>
#include <gtest/gtest.h>

using namespace gfx;

// Basic sanity test
TEST(GfxCppBasicTest, LibraryLinkage) {
    // Just verify we can link against the C++ wrapper
    EXPECT_TRUE(true);
}

TEST(GfxCppBasicTest, ResultEnumValues) {
    // Verify enum values match C API
    EXPECT_EQ(static_cast<int>(Result::Success), 0);
    EXPECT_EQ(static_cast<int>(Result::Timeout), 1);
    EXPECT_EQ(static_cast<int>(Result::NotReady), 2);
    EXPECT_LT(static_cast<int>(Result::ErrorInvalidArgument), 0);
}

TEST(GfxCppBasicTest, BackendEnumValues) {
    // Verify backend enum values match C API
    EXPECT_EQ(static_cast<int>(Backend::Vulkan), 0);
    EXPECT_EQ(static_cast<int>(Backend::WebGPU), 1);
}
