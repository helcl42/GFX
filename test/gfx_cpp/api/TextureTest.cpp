#include "CommonTest.h"

#include <vector>

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxCppTextureTest : public testing::TestWithParam<gfx::Backend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        try {
            gfx::InstanceDescriptor instDesc{
                .backend = backend,
                .enabledExtensions = { gfx::INSTANCE_EXTENSION_DEBUG }
            };
            instance = gfx::createInstance(instDesc);

            gfx::AdapterDescriptor adapterDesc{
                .adapterIndex = 0,
                .preference = gfx::AdapterPreference::HighPerformance
            };
            adapter = instance->requestAdapter(adapterDesc);

            gfx::DeviceDescriptor deviceDesc{
                .label = "Test Device"
            };
            device = adapter->createDevice(deviceDesc);
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up: " << e.what();
        }
    }

    void TearDown() override
    {
        device.reset();
        adapter.reset();
        instance.reset();
    }

    gfx::Backend backend;
    std::shared_ptr<gfx::Instance> instance;
    std::shared_ptr<gfx::Adapter> adapter;
    std::shared_ptr<gfx::Device> device;
};

TEST_P(GfxCppTextureTest, CreateDestroyTexture)
{
    ASSERT_NE(device, nullptr);

    gfx::TextureDescriptor desc{
        .label = "TestTexture",
        .type = gfx::TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::TextureBinding | gfx::TextureUsage::CopyDst
    };

    auto texture = device->createTexture(desc);
    EXPECT_NE(texture, nullptr);
    // Texture will be destroyed when shared_ptr goes out of scope
}

TEST_P(GfxCppTextureTest, CreateTextureZeroSize)
{
    ASSERT_NE(device, nullptr);

    gfx::TextureDescriptor desc{
        .type = gfx::TextureType::Texture2D,
        .size = { 0, 0, 0 }, // Invalid: zero size
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::TextureBinding
    };

    try {
        auto texture = device->createTexture(desc);
        // If it doesn't throw, it should return null
        EXPECT_EQ(texture, nullptr) << "Texture creation with zero size should fail";
    } catch (const std::exception& e) {
        SUCCEED() << "Correctly rejected zero size: " << e.what();
    }
}

TEST_P(GfxCppTextureTest, CreateTextureNoUsage)
{
    ASSERT_NE(device, nullptr);

    gfx::TextureDescriptor desc{
        .type = gfx::TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::None // Invalid: no usage
    };

    try {
        auto texture = device->createTexture(desc);
        EXPECT_EQ(texture, nullptr) << "Texture creation with no usage should fail";
    } catch (const std::exception& e) {
        SUCCEED() << "Correctly rejected no usage: " << e.what();
    }
}

TEST_P(GfxCppTextureTest, GetTextureInfo)
{
    ASSERT_NE(device, nullptr);

    gfx::TextureDescriptor desc{
        .label = "TestTexture",
        .type = gfx::TextureType::Texture2D,
        .size = { 512, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::TextureBinding
    };

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    auto info = texture->getInfo();
    EXPECT_EQ(info.type, gfx::TextureType::Texture2D);
    EXPECT_EQ(info.size.width, 512u);
    EXPECT_EQ(info.size.height, 256u);
    EXPECT_EQ(info.size.depth, 1u);
    EXPECT_EQ(info.format, gfx::TextureFormat::R8G8B8A8Unorm);
    EXPECT_TRUE(static_cast<bool>(info.usage & gfx::TextureUsage::TextureBinding));
}

TEST_P(GfxCppTextureTest, CreateTextureView)
{
    ASSERT_NE(device, nullptr);

    gfx::TextureDescriptor desc{
        .label = "TestTexture",
        .type = gfx::TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::TextureBinding
    };

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    gfx::TextureViewDescriptor viewDesc{
        .label = "TestTextureView",
        .viewType = gfx::TextureViewType::View2D,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1
    };

    auto view = texture->createView(viewDesc);
    EXPECT_NE(view, nullptr);
}

TEST_P(GfxCppTextureTest, CreateTexture1D)
{
    ASSERT_NE(device, nullptr);

    gfx::TextureDescriptor desc{
        .type = gfx::TextureType::Texture1D,
        .size = { 256, 1, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::TextureBinding
    };

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    auto info = texture->getInfo();
    EXPECT_EQ(info.type, gfx::TextureType::Texture1D);
}

TEST_P(GfxCppTextureTest, CreateTexture3D)
{
    ASSERT_NE(device, nullptr);

    gfx::TextureDescriptor desc{
        .type = gfx::TextureType::Texture3D,
        .size = { 64, 64, 64 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::TextureBinding
    };

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    auto info = texture->getInfo();
    EXPECT_EQ(info.type, gfx::TextureType::Texture3D);
    EXPECT_EQ(info.size.depth, 64u);
}

TEST_P(GfxCppTextureTest, CreateTextureCube)
{
    ASSERT_NE(device, nullptr);

    gfx::TextureDescriptor desc{
        .type = gfx::TextureType::TextureCube,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 6, // Cube must have 6 layers
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::TextureBinding
    };

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    auto info = texture->getInfo();
    // Some backends may represent cube textures as 2D arrays with 6 layers
    // The important thing is that it has 6 array layers
    EXPECT_EQ(info.arrayLayerCount, 6u);
}

TEST_P(GfxCppTextureTest, CreateTextureWithMipmaps)
{
    ASSERT_NE(device, nullptr);

    gfx::TextureDescriptor desc{
        .type = gfx::TextureType::Texture2D,
        .size = { 512, 512, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 9, // log2(512) + 1
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::TextureBinding | gfx::TextureUsage::CopyDst
    };

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    auto info = texture->getInfo();
    EXPECT_EQ(info.mipLevelCount, 9u);
}

TEST_P(GfxCppTextureTest, CreateTextureArray)
{
    ASSERT_NE(device, nullptr);

    gfx::TextureDescriptor desc{
        .type = gfx::TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 8,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::TextureBinding
    };

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    auto info = texture->getInfo();
    EXPECT_EQ(info.arrayLayerCount, 8u);
}

TEST_P(GfxCppTextureTest, CreateMultipleTextures)
{
    ASSERT_NE(device, nullptr);

    const uint32_t textureCount = 10;
    std::vector<std::shared_ptr<gfx::Texture>> textures;

    for (uint32_t i = 0; i < textureCount; ++i) {
        gfx::TextureDescriptor desc{
            .type = gfx::TextureType::Texture2D,
            .size = { 128, 128, 1 },
            .arrayLayerCount = 1,
            .mipLevelCount = 1,
            .sampleCount = gfx::SampleCount::Count1,
            .format = gfx::TextureFormat::R8G8B8A8Unorm,
            .usage = gfx::TextureUsage::TextureBinding
        };

        auto texture = device->createTexture(desc);
        ASSERT_NE(texture, nullptr);
        textures.push_back(texture);
    }

    EXPECT_EQ(textures.size(), textureCount);
    // Textures will be destroyed when vector goes out of scope
}

TEST_P(GfxCppTextureTest, CreateTextureWithAllUsageFlags)
{
    ASSERT_NE(device, nullptr);

    gfx::TextureDescriptor desc{
        .type = gfx::TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::CopySrc | gfx::TextureUsage::CopyDst | gfx::TextureUsage::TextureBinding | gfx::TextureUsage::StorageBinding | gfx::TextureUsage::RenderAttachment
    };

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    auto info = texture->getInfo();
    EXPECT_EQ(info.usage, desc.usage);
}

TEST_P(GfxCppTextureTest, CreateDepthTexture)
{
    ASSERT_NE(device, nullptr);

    gfx::TextureDescriptor desc{
        .type = gfx::TextureType::Texture2D,
        .size = { 512, 512, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::TextureFormat::Depth32Float,
        .usage = gfx::TextureUsage::RenderAttachment | gfx::TextureUsage::TextureBinding
    };

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    auto info = texture->getInfo();
    EXPECT_EQ(info.format, gfx::TextureFormat::Depth32Float);
}

TEST_P(GfxCppTextureTest, GetNativeHandle)
{
    ASSERT_NE(device, nullptr);

    gfx::TextureDescriptor desc{
        .type = gfx::TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::TextureBinding
    };

    auto texture = device->createTexture(desc);
    ASSERT_NE(texture, nullptr);

    void* nativeHandle = texture->getNativeHandle();
    EXPECT_NE(nativeHandle, nullptr);
}

TEST_P(GfxCppTextureTest, ImportTextureInvalidArguments)
{
    ASSERT_NE(device, nullptr);

    // Null device would be a programming error in C++

    // Null native handle - should throw
    gfx::TextureImportDescriptor nullHandleDesc{
        .nativeHandle = nullptr,
        .type = gfx::TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::TextureBinding,
        .currentLayout = gfx::TextureLayout::Undefined
    };
    EXPECT_THROW(device->importTexture(nullHandleDesc), std::exception);

    // Note: Invalid handle (arbitrary pointer like 0xDEADBEEF) cannot be validated
    // by the backend without actually using it, so we can't test for that case.
    // The backend will only catch null handles at the API boundary.
}

TEST_P(GfxCppTextureTest, ImportTextureZeroSize)
{
    ASSERT_NE(device, nullptr);

    gfx::TextureImportDescriptor desc{
        .nativeHandle = (void*)0x1,
        .type = gfx::TextureType::Texture2D,
        .size = { 0, 0, 0 }, // Invalid: zero size
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::TextureBinding,
        .currentLayout = gfx::TextureLayout::Undefined
    };

    // C++ API should throw on invalid arguments
    EXPECT_THROW(device->importTexture(desc), std::exception);
}

TEST_P(GfxCppTextureTest, ImportTextureNoUsage)
{
    ASSERT_NE(device, nullptr);

    gfx::TextureImportDescriptor desc{
        .nativeHandle = (void*)0x1,
        .type = gfx::TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::None, // Invalid: no usage
        .currentLayout = gfx::TextureLayout::Undefined
    };

    // C++ API should throw on invalid arguments
    EXPECT_THROW(device->importTexture(desc), std::exception);
}

TEST_P(GfxCppTextureTest, ImportTextureFromNativeHandle)
{
    ASSERT_NE(device, nullptr);

    // First, create a normal texture
    gfx::TextureDescriptor createDesc{
        .label = "Source Texture",
        .type = gfx::TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::CopySrc | gfx::TextureUsage::CopyDst
    };

    auto sourceTexture = device->createTexture(createDesc);
    ASSERT_NE(sourceTexture, nullptr);

    // Get texture info to verify properties
    auto info = sourceTexture->getInfo();

    // Extract native handle
    void* nativeHandle = sourceTexture->getNativeHandle();
    ASSERT_NE(nativeHandle, nullptr);

    // Now import the native handle
    gfx::TextureImportDescriptor importDesc{
        .nativeHandle = nativeHandle,
        .type = info.type,
        .size = info.size,
        .arrayLayerCount = info.arrayLayerCount,
        .mipLevelCount = info.mipLevelCount,
        .sampleCount = info.sampleCount,
        .format = info.format,
        .usage = info.usage,
        .currentLayout = gfx::TextureLayout::Undefined
    };

    auto importedTexture = device->importTexture(importDesc);
    EXPECT_NE(importedTexture, nullptr);

    // Verify imported texture has correct properties
    if (importedTexture) {
        auto importedInfo = importedTexture->getInfo();
        EXPECT_EQ(importedInfo.type, info.type);
        EXPECT_EQ(importedInfo.size.width, info.size.width);
        EXPECT_EQ(importedInfo.size.height, info.size.height);
        EXPECT_EQ(importedInfo.size.depth, info.size.depth);
        EXPECT_EQ(importedInfo.arrayLayerCount, info.arrayLayerCount);
        EXPECT_EQ(importedInfo.mipLevelCount, info.mipLevelCount);
        EXPECT_EQ(importedInfo.sampleCount, info.sampleCount);
        EXPECT_EQ(importedInfo.format, info.format);
        EXPECT_EQ(importedInfo.usage, info.usage);

        // Imported texture shares the native handle with sourceTexture
        // Both will be destroyed via RAII, which is fine
    }

    // Clean up happens automatically via shared_ptr
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppTextureTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

} // namespace
