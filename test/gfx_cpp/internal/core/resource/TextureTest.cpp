#include <core/resource/Texture.h>
#include <core/system/Device.h>

#include <gfx/gfx.h>

#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

namespace gfx {

class TextureImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc{
            .backend = backend,
            .applicationName = "TextureImplTest"
        };
        ASSERT_EQ(gfxCreateInstance(&instanceDesc, &instance), GFX_RESULT_SUCCESS);

        GfxAdapterDescriptor adapterDesc{
            .sType = GFX_STRUCTURE_TYPE_ADAPTER_DESCRIPTOR,
            .pNext = nullptr,
            .adapterIndex = 0
        };
        ASSERT_EQ(gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter), GFX_RESULT_SUCCESS);

        GfxDeviceDescriptor deviceDesc{
            .sType = GFX_STRUCTURE_TYPE_DEVICE_DESCRIPTOR,
            .pNext = nullptr,
            .label = nullptr,
            .queueRequests = nullptr,
            .queueRequestCount = 0,
            .enabledExtensions = nullptr,
            .enabledExtensionCount = 0
        };
        ASSERT_EQ(gfxAdapterCreateDevice(adapter, &deviceDesc, &device), GFX_RESULT_SUCCESS);
    }

    void TearDown() override
    {
        if (device) {
            gfxDeviceDestroy(device);
        }
        if (instance) {
            gfxInstanceDestroy(instance);
        }
        gfxUnloadBackend(backend);
    }

    GfxBackend backend;
    GfxInstance instance = nullptr;
    GfxAdapter adapter = nullptr;
    GfxDevice device = nullptr;
};

TEST_P(TextureImplTest, CreateTexture)
{
    DeviceImpl deviceWrapper(device);

    TextureDescriptor desc{
        .type = TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .format = TextureFormat::R8G8B8A8Unorm,
        .usage = TextureUsage::TextureBinding
    };

    auto texture = deviceWrapper.createTexture(desc);
    ASSERT_NE(texture, nullptr);

    auto info = texture->getInfo();
    EXPECT_EQ(info.size.width, 256);
    EXPECT_EQ(info.size.height, 256);
    EXPECT_EQ(info.format, TextureFormat::R8G8B8A8Unorm);
}

TEST_P(TextureImplTest, CreateTextureWithMipLevels)
{
    DeviceImpl deviceWrapper(device);

    TextureDescriptor desc{
        .type = TextureType::Texture2D,
        .size = { 512, 512, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 4,
        .format = TextureFormat::R32Float,
        .usage = TextureUsage::RenderAttachment
    };

    auto texture = deviceWrapper.createTexture(desc);
    ASSERT_NE(texture, nullptr);

    auto info = texture->getInfo();
    EXPECT_EQ(info.size.width, 512);
    EXPECT_EQ(info.size.height, 512);
    EXPECT_EQ(info.mipLevelCount, 4);
    EXPECT_EQ(info.format, TextureFormat::R32Float);
}

TEST_P(TextureImplTest, MultipleTextures_IndependentHandles)
{
    DeviceImpl deviceWrapper(device);

    TextureDescriptor desc1{
        .type = TextureType::Texture2D,
        .size = { 128, 128, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .format = TextureFormat::R8G8B8A8Unorm,
        .usage = TextureUsage::TextureBinding
    };

    TextureDescriptor desc2{
        .type = TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .format = TextureFormat::R16G16B16A16Float,
        .usage = TextureUsage::RenderAttachment
    };

    auto texture1 = deviceWrapper.createTexture(desc1);
    auto texture2 = deviceWrapper.createTexture(desc2);

    ASSERT_NE(texture1, nullptr);
    ASSERT_NE(texture2, nullptr);

    // Verify textures are independent
    EXPECT_EQ(texture1->getInfo().size.width, 128);
    EXPECT_EQ(texture2->getInfo().size.width, 256);
}

TEST_P(TextureImplTest, GetNativeHandle)
{
    DeviceImpl deviceWrapper(device);

    TextureDescriptor desc{
        .type = TextureType::Texture2D,
        .size = { 64, 64, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .format = TextureFormat::R8G8B8A8Unorm,
        .usage = TextureUsage::TextureBinding
    };

    auto texture = deviceWrapper.createTexture(desc);
    ASSERT_NE(texture, nullptr);

    void* nativeHandle = texture->getNativeHandle();
    EXPECT_NE(nativeHandle, nullptr);
}

TEST_P(TextureImplTest, GetLayout)
{
    DeviceImpl deviceWrapper(device);

    TextureDescriptor desc{
        .type = TextureType::Texture2D,
        .size = { 128, 128, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .format = TextureFormat::R8G8B8A8Unorm,
        .usage = TextureUsage::TextureBinding
    };

    auto texture = deviceWrapper.createTexture(desc);
    ASSERT_NE(texture, nullptr);

    // Should return some valid layout (exact value depends on backend)
    auto layout = texture->getLayout();
    // Just verify it doesn't crash
}

TEST_P(TextureImplTest, ImportTexture)
{
    DeviceImpl deviceWrapper(device);

    // Create a texture to get its native handle
    TextureDescriptor createDesc{
        .type = TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .format = TextureFormat::R8G8B8A8Unorm,
        .usage = TextureUsage::TextureBinding
    };

    auto originalTexture = deviceWrapper.createTexture(createDesc);
    ASSERT_NE(originalTexture, nullptr);

    // Get native handle
    void* nativeHandle = originalTexture->getNativeHandle();
    ASSERT_NE(nativeHandle, nullptr);

    // Import using the native handle
    TextureImportDescriptor importDesc{
        .nativeHandle = nativeHandle,
        .type = TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .format = TextureFormat::R8G8B8A8Unorm,
        .usage = TextureUsage::TextureBinding
    };

    auto importedTexture = deviceWrapper.importTexture(importDesc);
    ASSERT_NE(importedTexture, nullptr);

    // Verify info matches
    auto info = importedTexture->getInfo();
    EXPECT_EQ(info.size.width, 256);
    EXPECT_EQ(info.size.height, 256);
    EXPECT_EQ(info.format, TextureFormat::R8G8B8A8Unorm);
}

// Instantiate tests for available backends
#if defined(GFX_ENABLE_VULKAN) && defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(AllBackends, TextureImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU));
#elif defined(GFX_ENABLE_VULKAN)
INSTANTIATE_TEST_SUITE_P(VulkanOnly, TextureImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN));
#elif defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(WebGPUOnly, TextureImplTest,
    ::testing::Values(GFX_BACKEND_WEBGPU));
#endif

} // namespace gfx
