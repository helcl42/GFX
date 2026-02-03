#include <backend/webgpu/core/sync/Fence.h>

#include <gtest/gtest.h>

#include <memory>

namespace {

class WebGPUFenceTest : public testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(WebGPUFenceTest, CreateFence_Unsignaled)
{
    auto fence = std::make_unique<gfx::backend::webgpu::core::Fence>(false);
    EXPECT_FALSE(fence->isSignaled());
}

TEST_F(WebGPUFenceTest, CreateFence_Signaled)
{
    auto fence = std::make_unique<gfx::backend::webgpu::core::Fence>(true);
    EXPECT_TRUE(fence->isSignaled());
}

TEST_F(WebGPUFenceTest, SetSignaled_ToTrue)
{
    auto fence = std::make_unique<gfx::backend::webgpu::core::Fence>(false);
    EXPECT_FALSE(fence->isSignaled());

    fence->signal();
    EXPECT_TRUE(fence->isSignaled());
}

TEST_F(WebGPUFenceTest, SetSignaled_ToFalse)
{
    auto fence = std::make_unique<gfx::backend::webgpu::core::Fence>(true);
    EXPECT_TRUE(fence->isSignaled());

    fence->reset();
    EXPECT_FALSE(fence->isSignaled());
}

TEST_F(WebGPUFenceTest, MultipleFences_IndependentState)
{
    auto fence1 = std::make_unique<gfx::backend::webgpu::core::Fence>(false);
    auto fence2 = std::make_unique<gfx::backend::webgpu::core::Fence>(true);

    EXPECT_FALSE(fence1->isSignaled());
    EXPECT_TRUE(fence2->isSignaled());

    fence1->signal();
    EXPECT_TRUE(fence1->isSignaled());
    EXPECT_TRUE(fence2->isSignaled());
}

TEST_F(WebGPUFenceTest, Destructor_CleansUpResources)
{
    {
        auto fence = std::make_unique<gfx::backend::webgpu::core::Fence>(true);
        EXPECT_TRUE(fence->isSignaled());
    }

    // If we reach here without crashing, cleanup succeeded
    SUCCEED();
}

} // anonymous namespace
