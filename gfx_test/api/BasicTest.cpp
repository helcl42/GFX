#include <gfx/gfx.h>
#include <gtest/gtest.h>

// C API tests compiled with C++ for GoogleTest compatibility

// Basic sanity test
TEST(GfxBasicTest, LibraryVersion) {
    // Just verify we can link against the library
    EXPECT_TRUE(true);
}

TEST(GfxBasicTest, ResultEnumValues) {
    // Verify enum values are correct
    EXPECT_EQ(GFX_RESULT_SUCCESS, 0);
    EXPECT_EQ(GFX_RESULT_TIMEOUT, 1);
    EXPECT_EQ(GFX_RESULT_NOT_READY, 2);
    EXPECT_LT(GFX_RESULT_ERROR_INVALID_ARGUMENT, 0);
}

TEST(GfxBasicTest, BackendEnumValues) {
    // Verify backend enum values
    EXPECT_EQ(GFX_BACKEND_VULKAN, 0);
    EXPECT_EQ(GFX_BACKEND_WEBGPU, 1);
}
