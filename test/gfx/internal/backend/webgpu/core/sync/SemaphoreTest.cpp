#include <backend/webgpu/core/sync/Semaphore.h>

#include <gtest/gtest.h>

#include <memory>

namespace {

class WebGPUSemaphoreTest : public testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(WebGPUSemaphoreTest, CreateSemaphore_Binary)
{
    auto semaphore = std::make_unique<gfx::backend::webgpu::core::Semaphore>(
        gfx::backend::webgpu::core::SemaphoreType::Binary, 0);
    
    EXPECT_EQ(semaphore->getType(), gfx::backend::webgpu::core::SemaphoreType::Binary);
    EXPECT_EQ(semaphore->getValue(), 0);
}

TEST_F(WebGPUSemaphoreTest, CreateSemaphore_Timeline)
{
    auto semaphore = std::make_unique<gfx::backend::webgpu::core::Semaphore>(
        gfx::backend::webgpu::core::SemaphoreType::Timeline, 42);
    
    EXPECT_EQ(semaphore->getType(), gfx::backend::webgpu::core::SemaphoreType::Timeline);
    EXPECT_EQ(semaphore->getValue(), 42);
}

TEST_F(WebGPUSemaphoreTest, SetValue_UpdatesValue)
{
    auto semaphore = std::make_unique<gfx::backend::webgpu::core::Semaphore>(
        gfx::backend::webgpu::core::SemaphoreType::Timeline, 0);
    
    EXPECT_EQ(semaphore->getValue(), 0);
    
    semaphore->setValue(100);
    EXPECT_EQ(semaphore->getValue(), 100);
}

TEST_F(WebGPUSemaphoreTest, MultipleSemaphores_IndependentState)
{
    auto sem1 = std::make_unique<gfx::backend::webgpu::core::Semaphore>(
        gfx::backend::webgpu::core::SemaphoreType::Binary, 0);
    auto sem2 = std::make_unique<gfx::backend::webgpu::core::Semaphore>(
        gfx::backend::webgpu::core::SemaphoreType::Timeline, 50);
    
    EXPECT_EQ(sem1->getType(), gfx::backend::webgpu::core::SemaphoreType::Binary);
    EXPECT_EQ(sem1->getValue(), 0);
    
    EXPECT_EQ(sem2->getType(), gfx::backend::webgpu::core::SemaphoreType::Timeline);
    EXPECT_EQ(sem2->getValue(), 50);
    
    sem2->setValue(75);
    EXPECT_EQ(sem1->getValue(), 0);  // sem1 unchanged
    EXPECT_EQ(sem2->getValue(), 75); // sem2 changed
}

TEST_F(WebGPUSemaphoreTest, Destructor_CleansUpResources)
{
    {
        auto semaphore = std::make_unique<gfx::backend::webgpu::core::Semaphore>(
            gfx::backend::webgpu::core::SemaphoreType::Timeline, 123);
        EXPECT_EQ(semaphore->getValue(), 123);
    }
    
    // If we reach here without crashing, cleanup succeeded
    SUCCEED();
}

} // anonymous namespace
