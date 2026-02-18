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

TEST(GfxCppBackendTest, LoadBackendVulkan)
{
    auto result = gfx::loadBackend(gfx::Backend::Vulkan);
    EXPECT_TRUE(gfx::isSuccess(result)) << "Failed to load Vulkan backend: " << static_cast<int32_t>(result);

    result = gfx::loadBackend(gfx::Backend::Vulkan);
    EXPECT_TRUE(gfx::isSuccess(result)) << "Loading Vulkan backend twice should succeed: " << static_cast<int32_t>(result);

    gfx::unloadBackend(gfx::Backend::Vulkan);
}

TEST(GfxCppBackendTest, LoadBackendAuto)
{
    auto result = gfx::loadBackend(gfx::Backend::Auto);
    EXPECT_TRUE(gfx::isSuccess(result)) << "Failed to load Auto backend: " << static_cast<int32_t>(result);

    gfx::unloadBackend(gfx::Backend::Auto);
}

TEST(GfxCppBackendTest, LoadUnloadMultipleTimes)
{
    auto result = gfx::loadBackend(gfx::Backend::Vulkan);
    EXPECT_TRUE(gfx::isSuccess(result));

    gfx::unloadBackend(gfx::Backend::Vulkan);

    result = gfx::loadBackend(gfx::Backend::Vulkan);
    EXPECT_TRUE(gfx::isSuccess(result));

    gfx::unloadBackend(gfx::Backend::Vulkan);
}

} // namespace
