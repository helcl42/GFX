#include <gfx_cpp/gfx.hpp>
#include <gtest/gtest.h>

using namespace gfx;

// Test instance creation and destruction with C++ wrapper
TEST(GfxCppInstanceTest, CreateDestroyVulkan) {
    InstanceDescriptor desc{};
    desc.backend = Backend::Vulkan;
    desc.enabledExtensions = { INSTANCE_EXTENSION_DEBUG };
    
    try {
        auto instance = createInstance(desc);
        EXPECT_NE(instance, nullptr);
        // Instance automatically destroyed via shared_ptr
        SUCCEED();
    } catch (const std::exception& e) {
        // Vulkan might not be available on all systems
        GTEST_SKIP() << "Vulkan backend not available: " << e.what();
    }
}

TEST(GfxCppInstanceTest, CreateDestroyWebGPU) {
    InstanceDescriptor desc{};
    desc.backend = Backend::WebGPU;
    desc.enabledExtensions = { INSTANCE_EXTENSION_DEBUG };
    
    try {
        auto instance = createInstance(desc);
        EXPECT_NE(instance, nullptr);
        // Instance automatically destroyed via shared_ptr
        SUCCEED();
    } catch (const std::exception& e) {
        // WebGPU might not be available on all systems
        GTEST_SKIP() << "WebGPU backend not available: " << e.what();
    }
}

TEST(GfxCppInstanceTest, SharedPointerSemantics) {
    InstanceDescriptor desc{};
    desc.backend = Backend::Vulkan;
    desc.enabledExtensions = { INSTANCE_EXTENSION_DEBUG };
    
    try {
        auto instance1 = createInstance(desc);
        EXPECT_NE(instance1, nullptr);
        
        // Copy shared_ptr
        auto instance2 = instance1;
        EXPECT_EQ(instance1, instance2);
        EXPECT_EQ(instance1.get(), instance2.get());
        
        SUCCEED();
    } catch (const std::exception&) {
        GTEST_SKIP() << "Vulkan backend not available";
    }
}

TEST(GfxCppInstanceTest, NullInstance) {
    std::shared_ptr<Instance> instance; // Default constructed = null
    EXPECT_EQ(instance, nullptr);
}
