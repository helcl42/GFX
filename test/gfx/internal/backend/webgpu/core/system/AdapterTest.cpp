#include <backend/webgpu/core/system/Adapter.h>
#include <backend/webgpu/core/system/Device.h>
#include <backend/webgpu/core/system/Instance.h>

#include <gtest/gtest.h>

#include <memory>

namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class WebGPUAdapterTest : public testing::Test {
protected:
    void SetUp() override
    {
        try {
            gfx::backend::webgpu::core::InstanceCreateInfo instInfo{};
            instance = std::make_unique<gfx::backend::webgpu::core::Instance>(instInfo);

            gfx::backend::webgpu::core::AdapterCreateInfo adapterInfo{};
            adapterInfo.adapterIndex = 0;
            adapter = instance->requestAdapter(adapterInfo);
        } catch (const std::exception& e) {
            GTEST_SKIP() << "WebGPU not available: " << e.what();
        }
    }

    std::unique_ptr<gfx::backend::webgpu::core::Instance> instance;
    gfx::backend::webgpu::core::Adapter* adapter = nullptr;
};

// ============================================================================
// Basic Tests
// ============================================================================

TEST_F(WebGPUAdapterTest, Handle_ReturnsValidWGPUAdapter)
{
    WGPUAdapter handle = adapter->handle();
    EXPECT_NE(handle, nullptr);
}

TEST_F(WebGPUAdapterTest, GetInstance_ReturnsCorrectInstance)
{
    auto* inst = adapter->getInstance();
    EXPECT_EQ(inst, instance.get());
}

// ============================================================================
// Info Tests
// ============================================================================

TEST_F(WebGPUAdapterTest, GetInfo_ReturnsValidInfo)
{
    const auto& info = adapter->getInfo();

    EXPECT_FALSE(info.name.empty()) << "Adapter name should not be empty";
}

TEST_F(WebGPUAdapterTest, GetLimits_ReturnsValidLimits)
{
    WGPULimits limits = adapter->getLimits();

    EXPECT_GT(limits.maxTextureDimension1D, 0);
    EXPECT_GT(limits.maxTextureDimension2D, 0);
    EXPECT_GT(limits.maxTextureDimension3D, 0);
    EXPECT_GT(limits.maxBindGroups, 0);
}

// ============================================================================
// Queue Family Tests
// ============================================================================

TEST_F(WebGPUAdapterTest, GetQueueFamilyProperties_ReturnsAtLeastOne)
{
    auto queueFamilies = adapter->getQueueFamilyProperties();
    EXPECT_GE(queueFamilies.size(), 1) << "Should have at least one queue family";
}

TEST_F(WebGPUAdapterTest, SupportsPresentation_ForDefaultQueue)
{
    auto queueFamilies = adapter->getQueueFamilyProperties();
    if (!queueFamilies.empty()) {
        bool supportsPresent = adapter->supportsPresentation(0);
        // WebGPU generally supports presentation
        EXPECT_TRUE(supportsPresent);
    }
}

// ============================================================================
// Extension Tests
// ============================================================================

TEST_F(WebGPUAdapterTest, EnumerateSupportedExtensions_ReturnsVector)
{
    auto extensions = adapter->enumerateSupportedExtensions();
    EXPECT_GE(extensions.size(), 0);
}

// ============================================================================
// Device Creation Tests
// ============================================================================

TEST_F(WebGPUAdapterTest, CreateDevice_WithDefaultSettings)
{
    gfx::backend::webgpu::core::DeviceCreateInfo deviceInfo{};
    auto device = std::make_unique<gfx::backend::webgpu::core::Device>(adapter, deviceInfo);

    EXPECT_NE(device->handle(), nullptr);
    EXPECT_EQ(device->getAdapter(), adapter);
}

} // anonymous namespace
