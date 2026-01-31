#include <gfx/gfx.h>

// Include internal headers for testing
#include <backend/Factory.h>
#include <backend/IBackend.h>

#include <gtest/gtest.h>

namespace gfx::backend::test {

// Test that Factory creates backend implementations correctly
class FactoryTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(FactoryTest, CreateVulkanBackend)
{
#ifdef GFX_ENABLE_VULKAN
    auto backend = BackendFactory::create(GFX_BACKEND_VULKAN);
    ASSERT_NE(backend, nullptr) << "Vulkan backend creation should succeed when enabled";
#else
    auto backend = BackendFactory::create(GFX_BACKEND_VULKAN);
    ASSERT_EQ(backend, nullptr) << "Vulkan backend creation should fail when disabled";
#endif
}

TEST_F(FactoryTest, CreateWebGPUBackend)
{
#ifdef GFX_ENABLE_WEBGPU
    auto backend = BackendFactory::create(GFX_BACKEND_WEBGPU);
    ASSERT_NE(backend, nullptr) << "WebGPU backend creation should succeed when enabled";
#else
    auto backend = BackendFactory::create(GFX_BACKEND_WEBGPU);
    ASSERT_EQ(backend, nullptr) << "WebGPU backend creation should fail when disabled";
#endif
}

TEST_F(FactoryTest, CreateInvalidBackend)
{
    auto backend = BackendFactory::create(GFX_BACKEND_AUTO);
    ASSERT_EQ(backend, nullptr) << "GFX_BACKEND_AUTO should not create a backend";
}

TEST_F(FactoryTest, CreateOutOfRangeBackend)
{
    auto backend = BackendFactory::create(static_cast<GfxBackend>(999));
    ASSERT_EQ(backend, nullptr) << "Out of range backend should return nullptr";
}

TEST_F(FactoryTest, BackendInterfaceNotNull)
{
    // Create any available backend
#ifdef GFX_ENABLE_VULKAN
    auto backend = BackendFactory::create(GFX_BACKEND_VULKAN);
#elif defined(GFX_ENABLE_WEBGPU)
    auto backend = BackendFactory::create(GFX_BACKEND_WEBGPU);
#else
    auto backend = std::unique_ptr<const IBackend>(nullptr);
#endif

    if (backend) {
        ASSERT_NE(backend.get(), nullptr) << "Backend pointer should not be null";
        // The IBackend interface should be valid (we can't test much without calling its methods,
        // but at least we verify the object was created)
    } else {
        GTEST_SKIP() << "No backends enabled for testing";
    }
}

TEST_F(FactoryTest, MultipleCreationsReturnDifferentInstances)
{
#if defined(GFX_ENABLE_VULKAN)
    auto backend1 = BackendFactory::create(GFX_BACKEND_VULKAN);
    auto backend2 = BackendFactory::create(GFX_BACKEND_VULKAN);

    ASSERT_NE(backend1, nullptr);
    ASSERT_NE(backend2, nullptr);
    ASSERT_NE(backend1.get(), backend2.get()) << "Factory should create distinct instances";
#elif defined(GFX_ENABLE_WEBGPU)
    auto backend1 = BackendFactory::create(GFX_BACKEND_WEBGPU);
    auto backend2 = BackendFactory::create(GFX_BACKEND_WEBGPU);

    ASSERT_NE(backend1, nullptr);
    ASSERT_NE(backend2, nullptr);
    ASSERT_NE(backend1.get(), backend2.get()) << "Factory should create distinct instances";
#else
    GTEST_SKIP() << "No backends enabled for testing";
#endif
}

} // namespace gfx::backend::test
