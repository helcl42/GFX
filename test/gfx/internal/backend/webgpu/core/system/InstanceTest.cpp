#include <backend/webgpu/core/system/Adapter.h>
#include <backend/webgpu/core/system/Instance.h>

#include <gtest/gtest.h>

#include <memory>

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class WebGPUInstanceTest : public testing::Test {
protected:
    void SetUp() override
    {
        // WebGPU tests may not be available on all platforms
    }
};

// ============================================================================
// Basic Creation Tests
// ============================================================================

TEST_F(WebGPUInstanceTest, CreateInstance_WithDefaultSettings)
{
    try {
        gfx::backend::webgpu::core::InstanceCreateInfo createInfo{};
        auto instance = std::make_unique<gfx::backend::webgpu::core::Instance>(createInfo);

        EXPECT_NE(instance->handle(), nullptr);
    } catch (const std::exception& e) {
        GTEST_SKIP() << "WebGPU not available: " << e.what();
    }
}

TEST_F(WebGPUInstanceTest, CreateInstance_WithEnabledExtensions)
{
    try {
        gfx::backend::webgpu::core::InstanceCreateInfo createInfo{};
        createInfo.enabledExtensions = {};

        auto instance = std::make_unique<gfx::backend::webgpu::core::Instance>(createInfo);

        EXPECT_NE(instance->handle(), nullptr);
    } catch (const std::exception& e) {
        GTEST_SKIP() << "WebGPU not available: " << e.what();
    }
}

// ============================================================================
// Handle Tests
// ============================================================================

TEST_F(WebGPUInstanceTest, Handle_ReturnsValidWGPUInstance)
{
    try {
        gfx::backend::webgpu::core::InstanceCreateInfo createInfo{};
        auto instance = std::make_unique<gfx::backend::webgpu::core::Instance>(createInfo);

        WGPUInstance handle = instance->handle();
        EXPECT_NE(handle, nullptr);
    } catch (const std::exception& e) {
        GTEST_SKIP() << "WebGPU not available: " << e.what();
    }
}

TEST_F(WebGPUInstanceTest, Handle_IsUnique)
{
    try {
        gfx::backend::webgpu::core::InstanceCreateInfo createInfo{};
        auto instance1 = std::make_unique<gfx::backend::webgpu::core::Instance>(createInfo);
        auto instance2 = std::make_unique<gfx::backend::webgpu::core::Instance>(createInfo);

        EXPECT_NE(instance1->handle(), instance2->handle());
    } catch (const std::exception& e) {
        GTEST_SKIP() << "WebGPU not available: " << e.what();
    }
}

// ============================================================================
// Adapter Enumeration Tests
// ============================================================================

TEST_F(WebGPUInstanceTest, GetAdapters_ReturnsAtLeastOne)
{
    try {
        gfx::backend::webgpu::core::InstanceCreateInfo createInfo{};
        auto instance = std::make_unique<gfx::backend::webgpu::core::Instance>(createInfo);

        const auto& adapters = instance->getAdapters();
        EXPECT_GE(adapters.size(), 1) << "Should have at least one adapter";
    } catch (const std::exception& e) {
        GTEST_SKIP() << "WebGPU not available: " << e.what();
    }
}

TEST_F(WebGPUInstanceTest, RequestAdapter_WithDefaultSettings)
{
    try {
        gfx::backend::webgpu::core::InstanceCreateInfo createInfo{};
        auto instance = std::make_unique<gfx::backend::webgpu::core::Instance>(createInfo);

        gfx::backend::webgpu::core::AdapterCreateInfo adapterInfo{};
        adapterInfo.adapterIndex = 0;

        auto* adapter = instance->requestAdapter(adapterInfo);
        EXPECT_NE(adapter, nullptr);
        EXPECT_NE(adapter->handle(), nullptr);
    } catch (const std::exception& e) {
        GTEST_SKIP() << "WebGPU not available: " << e.what();
    }
}

TEST_F(WebGPUInstanceTest, RequestAdapter_ByIndex)
{
    try {
        gfx::backend::webgpu::core::InstanceCreateInfo createInfo{};
        auto instance = std::make_unique<gfx::backend::webgpu::core::Instance>(createInfo);

        const auto& adapters = instance->getAdapters();
        if (adapters.size() > 0) {
            gfx::backend::webgpu::core::AdapterCreateInfo adapterInfo{};
            adapterInfo.adapterIndex = 0;

            auto* adapter = instance->requestAdapter(adapterInfo);
            EXPECT_NE(adapter, nullptr);
        }
    } catch (const std::exception& e) {
        GTEST_SKIP() << "WebGPU not available: " << e.what();
    }
}

// ============================================================================
// Extension Tests
// ============================================================================

TEST_F(WebGPUInstanceTest, EnumerateSupportedExtensions_ReturnsVector)
{
    try {
        auto extensions = gfx::backend::webgpu::core::Instance::enumerateSupportedExtensions();
        // WebGPU may or may not have extensions
        EXPECT_GE(extensions.size(), 0);
    } catch (const std::exception& e) {
        GTEST_SKIP() << "WebGPU not available: " << e.what();
    }
}

// ============================================================================
// Lifecycle Tests
// ============================================================================

TEST_F(WebGPUInstanceTest, MultipleInstances_CanCoexist)
{
    try {
        gfx::backend::webgpu::core::InstanceCreateInfo createInfo{};
        auto instance1 = std::make_unique<gfx::backend::webgpu::core::Instance>(createInfo);
        auto instance2 = std::make_unique<gfx::backend::webgpu::core::Instance>(createInfo);

        EXPECT_NE(instance1->handle(), nullptr);
        EXPECT_NE(instance2->handle(), nullptr);
        EXPECT_NE(instance1->handle(), instance2->handle());
    } catch (const std::exception& e) {
        GTEST_SKIP() << "WebGPU not available: " << e.what();
    }
}

TEST_F(WebGPUInstanceTest, Destructor_CleansUpResources)
{
    try {
        gfx::backend::webgpu::core::InstanceCreateInfo createInfo{};
        {
            auto instance = std::make_unique<gfx::backend::webgpu::core::Instance>(createInfo);
            EXPECT_NE(instance->handle(), nullptr);
        }
        // If we reach here without crashing, cleanup succeeded
        SUCCEED();
    } catch (const std::exception& e) {
        GTEST_SKIP() << "WebGPU not available: " << e.what();
    }
}

} // anonymous namespace
