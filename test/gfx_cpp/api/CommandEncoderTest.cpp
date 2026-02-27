#include "CommonTest.h"

#include <cstring>

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxCppCommandEncoderTest : public testing::TestWithParam<gfx::Backend> {
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
                .adapterIndex = 0
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

    gfx::Backend backend;
    std::shared_ptr<gfx::Instance> instance;
    std::shared_ptr<gfx::Adapter> adapter;
    std::shared_ptr<gfx::Device> device;
};

// ===========================================================================
// CommandEncoder Tests
// ===========================================================================

TEST_P(GfxCppCommandEncoderTest, CreateCommandEncoder)
{
    ASSERT_NE(device, nullptr);

    gfx::CommandEncoderDescriptor desc{
        .label = "test_encoder"
    };

    auto encoder = device->createCommandEncoder(desc);
    EXPECT_NE(encoder, nullptr);
}

TEST_P(GfxCppCommandEncoderTest, CreateCommandEncoderWithoutLabel)
{
    ASSERT_NE(device, nullptr);

    gfx::CommandEncoderDescriptor desc{};

    auto encoder = device->createCommandEncoder(desc);
    EXPECT_NE(encoder, nullptr);
}

TEST_P(GfxCppCommandEncoderTest, CopyBufferToBuffer)
{
    ASSERT_NE(device, nullptr);

    // Create buffers
    gfx::BufferDescriptor srcBufferDesc{
        .label = "source_buffer",
        .size = 256,
        .usage = gfx::BufferUsage::CopySrc,
        .memoryProperties = gfx::MemoryProperty::DeviceLocal
    };
    auto srcBuffer = device->createBuffer(srcBufferDesc);
    ASSERT_NE(srcBuffer, nullptr);

    gfx::BufferDescriptor dstBufferDesc{
        .label = "destination_buffer",
        .size = 256,
        .usage = gfx::BufferUsage::CopyDst,
        .memoryProperties = gfx::MemoryProperty::DeviceLocal
    };
    auto dstBuffer = device->createBuffer(dstBufferDesc);
    ASSERT_NE(dstBuffer, nullptr);

    // Create command encoder
    gfx::CommandEncoderDescriptor encoderDesc{
        .label = "copy_encoder"
    };
    auto encoder = device->createCommandEncoder(encoderDesc);
    ASSERT_NE(encoder, nullptr);

    // Set up copy operation
    gfx::CopyBufferToBufferDescriptor copyDesc{
        .source = srcBuffer,
        .sourceOffset = 0,
        .destination = dstBuffer,
        .destinationOffset = 0,
        .size = 256
    };

    // Test that copy operation succeeds
    encoder->copyBufferToBuffer(copyDesc);
}

TEST_P(GfxCppCommandEncoderTest, PipelineBarrierEmpty)
{
    ASSERT_NE(device, nullptr);

    gfx::CommandEncoderDescriptor desc{
        .label = "test_encoder"
    };
    auto encoder = device->createCommandEncoder(desc);
    ASSERT_NE(encoder, nullptr);

    gfx::PipelineBarrierDescriptor barrierDesc{};

    encoder->pipelineBarrier(barrierDesc);
}

TEST_P(GfxCppCommandEncoderTest, EndCommandEncoder)
{
    ASSERT_NE(device, nullptr);

    gfx::CommandEncoderDescriptor desc{
        .label = "test_encoder"
    };
    auto encoder = device->createCommandEncoder(desc);
    ASSERT_NE(encoder, nullptr);

    // End the encoder
    encoder->end();
}

TEST_P(GfxCppCommandEncoderTest, CopyBufferToBufferAndEnd)
{
    ASSERT_NE(device, nullptr);

    // Create buffers
    gfx::BufferDescriptor srcBufferDesc{
        .label = "source_buffer",
        .size = 256,
        .usage = gfx::BufferUsage::CopySrc,
        .memoryProperties = gfx::MemoryProperty::DeviceLocal
    };
    auto srcBuffer = device->createBuffer(srcBufferDesc);

    gfx::BufferDescriptor dstBufferDesc{
        .label = "destination_buffer",
        .size = 256,
        .usage = gfx::BufferUsage::CopyDst,
        .memoryProperties = gfx::MemoryProperty::DeviceLocal
    };
    auto dstBuffer = device->createBuffer(dstBufferDesc);

    // Create command encoder and record copy command
    gfx::CommandEncoderDescriptor encoderDesc{
        .label = "copy_encoder"
    };
    auto encoder = device->createCommandEncoder(encoderDesc);

    gfx::CopyBufferToBufferDescriptor copyDesc{
        .source = srcBuffer,
        .sourceOffset = 0,
        .destination = dstBuffer,
        .destinationOffset = 0,
        .size = 256
    };
    encoder->copyBufferToBuffer(copyDesc);

    // End encoding
    encoder->end();
}

TEST_P(GfxCppCommandEncoderTest, MultipleCommandEncoders)
{
    ASSERT_NE(device, nullptr);

    const int encoderCount = 3;
    std::vector<std::shared_ptr<gfx::CommandEncoder>> encoders;

    for (int i = 0; i < encoderCount; ++i) {
        gfx::CommandEncoderDescriptor desc{};
        auto encoder = device->createCommandEncoder(desc);
        EXPECT_NE(encoder, nullptr);
        encoders.push_back(encoder);
    }

    EXPECT_EQ(encoders.size(), encoderCount);
}

TEST_P(GfxCppCommandEncoderTest, CopyWithOffsets)
{
    ASSERT_NE(device, nullptr);

    // Create buffers
    gfx::BufferDescriptor srcBufferDesc{
        .size = 512,
        .usage = gfx::BufferUsage::CopySrc,
        .memoryProperties = gfx::MemoryProperty::DeviceLocal
    };
    auto srcBuffer = device->createBuffer(srcBufferDesc);

    gfx::BufferDescriptor dstBufferDesc{
        .size = 512,
        .usage = gfx::BufferUsage::CopyDst,
        .memoryProperties = gfx::MemoryProperty::DeviceLocal
    };
    auto dstBuffer = device->createBuffer(dstBufferDesc);

    // Create command encoder
    auto encoder = device->createCommandEncoder({});

    // Copy with offsets
    gfx::CopyBufferToBufferDescriptor copyDesc{
        .source = srcBuffer,
        .sourceOffset = 128,
        .destination = dstBuffer,
        .destinationOffset = 256,
        .size = 128
    };
    encoder->copyBufferToBuffer(copyDesc);

    encoder->end();
}

TEST_P(GfxCppCommandEncoderTest, MultipleCopyOperations)
{
    ASSERT_NE(device, nullptr);

    // Create buffers
    gfx::BufferDescriptor bufferDesc{
        .size = 256,
        .usage = gfx::BufferUsage::CopySrc | gfx::BufferUsage::CopyDst,
        .memoryProperties = gfx::MemoryProperty::DeviceLocal
    };
    auto buffer1 = device->createBuffer(bufferDesc);
    auto buffer2 = device->createBuffer(bufferDesc);
    auto buffer3 = device->createBuffer(bufferDesc);

    // Create command encoder
    auto encoder = device->createCommandEncoder({});

    // Record multiple copy operations
    encoder->copyBufferToBuffer({ .source = buffer1,
        .sourceOffset = 0,
        .destination = buffer2,
        .destinationOffset = 0,
        .size = 128 });

    encoder->copyBufferToBuffer({ .source = buffer2,
        .sourceOffset = 0,
        .destination = buffer3,
        .destinationOffset = 0,
        .size = 128 });

    encoder->end();
}

TEST_P(GfxCppCommandEncoderTest, WriteTimestamp)
{
    ASSERT_NE(device, nullptr);

    // Create query set for timestamps
    gfx::QuerySetDescriptor querySetDesc{
        .type = gfx::QueryType::Timestamp,
        .count = 2
    };

    std::shared_ptr<gfx::QuerySet> querySet;
    try {
        querySet = device->createQuerySet(querySetDesc);
    } catch (const std::exception&) {
        GTEST_SKIP() << "Timestamp queries not supported";
    }

    ASSERT_NE(querySet, nullptr);

    auto encoder = device->createCommandEncoder({});

    // Write timestamp at beginning
    encoder->writeTimestamp(querySet, 0);

    // Write timestamp at end
    encoder->writeTimestamp(querySet, 1);

    encoder->end();
}

TEST_P(GfxCppCommandEncoderTest, CopyBufferToTexture)
{
    ASSERT_NE(device, nullptr);

    // Create source buffer
    gfx::BufferDescriptor bufferDesc{
        .size = 256 * 256 * 4,
        .usage = gfx::BufferUsage::CopySrc,
        .memoryProperties = gfx::MemoryProperty::DeviceLocal
    };
    auto buffer = device->createBuffer(bufferDesc);
    ASSERT_NE(buffer, nullptr);

    // Create destination texture
    gfx::TextureDescriptor textureDesc{
        .type = gfx::TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::Format::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::CopyDst
    };
    auto texture = device->createTexture(textureDesc);
    ASSERT_NE(texture, nullptr);

    // Create command encoder
    auto encoder = device->createCommandEncoder({});

    // Copy buffer to texture
    gfx::CopyBufferToTextureDescriptor copyDesc{
        .source = buffer,
        .sourceOffset = 0,
        .destination = texture,
        .origin = { 0, 0, 0 },
        .extent = { 256, 256, 1 },
        .mipLevel = 0
    };

    encoder->copyBufferToTexture(copyDesc);

    encoder->end();
}

TEST_P(GfxCppCommandEncoderTest, CopyTextureToBuffer)
{
    ASSERT_NE(device, nullptr);

    // Create source texture
    gfx::TextureDescriptor textureDesc{
        .type = gfx::TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::Format::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::CopySrc
    };
    auto texture = device->createTexture(textureDesc);
    ASSERT_NE(texture, nullptr);

    // Create destination buffer
    gfx::BufferDescriptor bufferDesc{
        .size = 256 * 256 * 4,
        .usage = gfx::BufferUsage::CopyDst,
        .memoryProperties = gfx::MemoryProperty::DeviceLocal
    };
    auto buffer = device->createBuffer(bufferDesc);
    ASSERT_NE(buffer, nullptr);

    // Create command encoder
    auto encoder = device->createCommandEncoder({});

    // Copy texture to buffer
    gfx::CopyTextureToBufferDescriptor copyDesc{
        .source = texture,
        .origin = { 0, 0, 0 },
        .mipLevel = 0,
        .destination = buffer,
        .destinationOffset = 0,
        .extent = { 256, 256, 1 }
    };

    encoder->copyTextureToBuffer(copyDesc);

    encoder->end();
}

TEST_P(GfxCppCommandEncoderTest, CopyTextureToTexture)
{
    ASSERT_NE(device, nullptr);

    // Create source texture
    gfx::TextureDescriptor srcTextureDesc{
        .type = gfx::TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::Format::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::CopySrc
    };
    auto srcTexture = device->createTexture(srcTextureDesc);
    ASSERT_NE(srcTexture, nullptr);

    // Create destination texture
    gfx::TextureDescriptor dstTextureDesc{
        .type = gfx::TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::Format::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::CopyDst
    };
    auto dstTexture = device->createTexture(dstTextureDesc);
    ASSERT_NE(dstTexture, nullptr);

    // Create command encoder
    auto encoder = device->createCommandEncoder({});

    // Copy texture to texture
    gfx::CopyTextureToTextureDescriptor copyDesc{
        .source = srcTexture,
        .sourceOrigin = { 0, 0, 0 },
        .sourceMipLevel = 0,
        .destination = dstTexture,
        .destinationOrigin = { 0, 0, 0 },
        .destinationMipLevel = 0,
        .extent = { 256, 256, 1 }
    };

    encoder->copyTextureToTexture(copyDesc);

    encoder->end();
}

TEST_P(GfxCppCommandEncoderTest, BlitTextureToTexture)
{
    ASSERT_NE(device, nullptr);

    // Create source texture
    gfx::TextureDescriptor srcTextureDesc{
        .type = gfx::TextureType::Texture2D,
        .size = { 512, 512, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::Format::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::TextureBinding
    };
    auto srcTexture = device->createTexture(srcTextureDesc);
    ASSERT_NE(srcTexture, nullptr);

    // Create destination texture
    gfx::TextureDescriptor dstTextureDesc{
        .type = gfx::TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::Format::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::RenderAttachment
    };
    auto dstTexture = device->createTexture(dstTextureDesc);
    ASSERT_NE(dstTexture, nullptr);

    // Create command encoder
    auto encoder = device->createCommandEncoder({});

    // Blit texture to texture (with scaling)
    gfx::BlitTextureToTextureDescriptor blitDesc{
        .source = srcTexture,
        .sourceOrigin = { 0, 0, 0 },
        .sourceExtent = { 512, 512, 1 },
        .sourceMipLevel = 0,
        .destination = dstTexture,
        .destinationOrigin = { 0, 0, 0 },
        .destinationExtent = { 256, 256, 1 },
        .destinationMipLevel = 0,
        .filter = gfx::FilterMode::Linear
    };

    encoder->blitTextureToTexture(blitDesc);

    encoder->end();
}

TEST_P(GfxCppCommandEncoderTest, GenerateMipmaps)
{
    ASSERT_NE(device, nullptr);

    // Create texture with multiple mip levels
    gfx::TextureDescriptor textureDesc{
        .type = gfx::TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 9,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::Format::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::TextureBinding | gfx::TextureUsage::RenderAttachment
    };
    auto texture = device->createTexture(textureDesc);
    ASSERT_NE(texture, nullptr);

    // Create command encoder
    auto encoder = device->createCommandEncoder({});

    // Generate mipmaps
    encoder->generateMipmaps(texture);

    encoder->end();
}

TEST_P(GfxCppCommandEncoderTest, GenerateMipmapsRange)
{
    ASSERT_NE(device, nullptr);

    // Create texture with multiple mip levels
    gfx::TextureDescriptor textureDesc{
        .type = gfx::TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 9,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::Format::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::TextureBinding | gfx::TextureUsage::RenderAttachment
    };
    auto texture = device->createTexture(textureDesc);
    ASSERT_NE(texture, nullptr);

    // Create command encoder
    auto encoder = device->createCommandEncoder({});

    // Generate mipmaps for a specific range
    encoder->generateMipmapsRange(texture, 0, 4);

    encoder->end();
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppCommandEncoderTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

} // namespace
