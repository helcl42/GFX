#include <gfx/gfx.h>
#include <gtest/gtest.h>

// C API tests compiled with C++ for GoogleTest compatibility

// Test instance creation and destruction
TEST(GfxInstanceTest, CreateDestroyVulkan) {
    GfxInstanceDescriptor desc = {};
    desc.backend = static_cast<GfxBackend>(GFX_BACKEND_VULKAN);
    desc.enableValidation = false;
    
    GfxInstance instance = NULL;
    GfxResult result = gfxCreateInstance(&desc, &instance);
    
    if (result == GFX_RESULT_SUCCESS) {
        EXPECT_NE(instance, nullptr);
        
        // Cleanup
        gfxInstanceDestroy(instance);
        SUCCEED();
    } else {
        // Vulkan might not be available on all systems
        GTEST_SKIP() << "Vulkan backend not available";
    }
}

TEST(GfxInstanceTest, CreateDestroyWebGPU) {
    GfxInstanceDescriptor desc = {};
    desc.backend = static_cast<GfxBackend>(GFX_BACKEND_WEBGPU);
    desc.enableValidation = false;
    
    GfxInstance instance = NULL;
    GfxResult result = gfxCreateInstance(&desc, &instance);
    
    if (result == GFX_RESULT_SUCCESS) {
        EXPECT_NE(instance, nullptr);
        
        // Cleanup
        gfxInstanceDestroy(instance);
        SUCCEED();
    } else {
        // WebGPU might not be available on all systems
        GTEST_SKIP() << "WebGPU backend not available";
    }
}

TEST(GfxInstanceTest, InvalidArguments) {
    // Test NULL output pointer
    GfxInstanceDescriptor desc = {};
    desc.backend = static_cast<GfxBackend>(GFX_BACKEND_VULKAN);
    
    GfxResult result = gfxCreateInstance(&desc, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
    
    // Test NULL descriptor
    GfxInstance instance = NULL;
    result = gfxCreateInstance(NULL, &instance);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST(GfxInstanceTest, DestroyNullInstance) {
    // Should handle NULL gracefully
    GfxResult result = gfxInstanceDestroy(NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}
