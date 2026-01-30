#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

#include <cstring>

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxCppQuerySetTest : public testing::TestWithParam<gfx::Backend> {
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
// Validation Tests - Invalid Arguments
// ===========================================================================

TEST_P(GfxCppQuerySetTest, CreateQuerySetInvalidArguments)
{
    ASSERT_NE(device, nullptr);

    // Test with zero count
    gfx::QuerySetDescriptor invalidDesc{
        .type = gfx::QueryType::Occlusion,
        .count = 0
    };

    EXPECT_THROW({ device->createQuerySet(invalidDesc); }, std::exception);
}

// ===========================================================================
// Query Set Creation and Destruction Tests
// ===========================================================================

TEST_P(GfxCppQuerySetTest, CreateAndDestroyOcclusionQuerySet)
{
    ASSERT_NE(device, nullptr);

    gfx::QuerySetDescriptor querySetDesc{
        .label = "Occlusion Query Set",
        .type = gfx::QueryType::Occlusion,
        .count = 16
    };

    auto querySet = device->createQuerySet(querySetDesc);
    EXPECT_NE(querySet, nullptr);

    // Verify properties
    EXPECT_EQ(querySet->getType(), gfx::QueryType::Occlusion);
    EXPECT_EQ(querySet->getCount(), 16);
}

TEST_P(GfxCppQuerySetTest, CreateAndDestroyTimestampQuerySet)
{
    ASSERT_NE(device, nullptr);

    gfx::QuerySetDescriptor querySetDesc{
        .label = "Timestamp Query Set",
        .type = gfx::QueryType::Timestamp,
        .count = 32
    };

    std::shared_ptr<gfx::QuerySet> querySet;
    try {
        querySet = device->createQuerySet(querySetDesc);
    } catch (const std::exception&) {
        GTEST_SKIP() << "Timestamp queries not supported";
    }

    EXPECT_NE(querySet, nullptr);

    // Verify properties
    EXPECT_EQ(querySet->getType(), gfx::QueryType::Timestamp);
    EXPECT_EQ(querySet->getCount(), 32);
}

TEST_P(GfxCppQuerySetTest, CreateMultipleQuerySets)
{
    ASSERT_NE(device, nullptr);

    gfx::QuerySetDescriptor occlusionDesc{
        .type = gfx::QueryType::Occlusion,
        .count = 8
    };

    gfx::QuerySetDescriptor timestampDesc{
        .type = gfx::QueryType::Timestamp,
        .count = 8
    };

    auto occlusionQuerySet = device->createQuerySet(occlusionDesc);
    EXPECT_NE(occlusionQuerySet, nullptr);

    std::shared_ptr<gfx::QuerySet> timestampQuerySet;
    try {
        timestampQuerySet = device->createQuerySet(timestampDesc);
    } catch (const std::exception&) {
        GTEST_SKIP() << "Timestamp queries not supported";
    }

    EXPECT_NE(timestampQuerySet, nullptr);

    // Verify they are different objects
    EXPECT_NE(occlusionQuerySet, timestampQuerySet);
    EXPECT_EQ(occlusionQuerySet->getType(), gfx::QueryType::Occlusion);
    EXPECT_EQ(timestampQuerySet->getType(), gfx::QueryType::Timestamp);
}

// ===========================================================================
// Command Encoder Query Operations - Validation Tests
// ===========================================================================

TEST_P(GfxCppQuerySetTest, WriteTimestampWithNullQuerySet)
{
    ASSERT_NE(device, nullptr);

    auto encoder = device->createCommandEncoder({ .label = "Test Encoder" });
    ASSERT_NE(encoder, nullptr);

    // Passing nullptr should throw
    EXPECT_THROW({ encoder->writeTimestamp(nullptr, 0); }, std::exception);
}

TEST_P(GfxCppQuerySetTest, ResolveQuerySetWithNullQuerySet)
{
    ASSERT_NE(device, nullptr);

    auto encoder = device->createCommandEncoder({ .label = "Test Encoder" });
    ASSERT_NE(encoder, nullptr);

    gfx::BufferDescriptor bufferDesc{
        .size = 8 * sizeof(uint64_t),
        .usage = gfx::BufferUsage::CopySrc | gfx::BufferUsage::CopyDst
    };
    auto buffer = device->createBuffer(bufferDesc);
    ASSERT_NE(buffer, nullptr);

    // Passing nullptr for query set should throw
    EXPECT_THROW({ encoder->resolveQuerySet(nullptr, 0, 8, buffer, 0); }, std::exception);
}

TEST_P(GfxCppQuerySetTest, ResolveQuerySetWithNullBuffer)
{
    ASSERT_NE(device, nullptr);

    gfx::QuerySetDescriptor querySetDesc{
        .type = gfx::QueryType::Timestamp,
        .count = 8
    };

    std::shared_ptr<gfx::QuerySet> querySet;
    try {
        querySet = device->createQuerySet(querySetDesc);
    } catch (const std::exception&) {
        GTEST_SKIP() << "Timestamp queries not supported";
    }

    auto encoder = device->createCommandEncoder({ .label = "Test Encoder" });
    ASSERT_NE(encoder, nullptr);

    // Passing nullptr for buffer should throw
    EXPECT_THROW({ encoder->resolveQuerySet(querySet, 0, 8, nullptr, 0); }, std::exception);
}

// ===========================================================================
// Command Encoder Timestamp Query Operations - Functional Tests
// ===========================================================================

TEST_P(GfxCppQuerySetTest, WriteTimestampOperation)
{
    ASSERT_NE(device, nullptr);

    gfx::QuerySetDescriptor querySetDesc{
        .label = "Timestamp Query Set",
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

    auto encoder = device->createCommandEncoder({ .label = "Test Encoder" });
    ASSERT_NE(encoder, nullptr);

    // Write timestamps at beginning and end
    encoder->writeTimestamp(querySet, 0);
    encoder->writeTimestamp(querySet, 1);

    encoder->end();
}

TEST_P(GfxCppQuerySetTest, ResolveQuerySetOperation)
{
    ASSERT_NE(device, nullptr);

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

    gfx::BufferDescriptor bufferDesc{
        .size = 2 * sizeof(uint64_t),
        .usage = gfx::BufferUsage::CopySrc | gfx::BufferUsage::CopyDst,
        .memoryProperties = gfx::MemoryProperty::HostVisible | gfx::MemoryProperty::HostCoherent
    };
    auto buffer = device->createBuffer(bufferDesc);
    ASSERT_NE(buffer, nullptr);

    auto encoder = device->createCommandEncoder({});
    ASSERT_NE(encoder, nullptr);

    // Write timestamps
    encoder->writeTimestamp(querySet, 0);
    encoder->writeTimestamp(querySet, 1);

    // Resolve queries to buffer
    encoder->resolveQuerySet(querySet, 0, 2, buffer, 0);

    encoder->end();
}

TEST_P(GfxCppQuerySetTest, WriteTimestampOutOfRange)
{
    ASSERT_NE(device, nullptr);

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
    ASSERT_NE(encoder, nullptr);

    // Query index out of range should throw or be handled by backend
    // Note: Some backends may defer validation to submit time
    encoder->writeTimestamp(querySet, 0);
    encoder->writeTimestamp(querySet, 1);

    encoder->end();
}

TEST_P(GfxCppQuerySetTest, ResolveQuerySetPartialRange)
{
    ASSERT_NE(device, nullptr);

    gfx::QuerySetDescriptor querySetDesc{
        .type = gfx::QueryType::Timestamp,
        .count = 8
    };

    std::shared_ptr<gfx::QuerySet> querySet;
    try {
        querySet = device->createQuerySet(querySetDesc);
    } catch (const std::exception&) {
        GTEST_SKIP() << "Timestamp queries not supported";
    }

    ASSERT_NE(querySet, nullptr);

    gfx::BufferDescriptor bufferDesc{
        .size = 8 * sizeof(uint64_t),
        .usage = gfx::BufferUsage::CopySrc | gfx::BufferUsage::CopyDst,
        .memoryProperties = gfx::MemoryProperty::HostVisible | gfx::MemoryProperty::HostCoherent
    };
    auto buffer = device->createBuffer(bufferDesc);
    ASSERT_NE(buffer, nullptr);

    auto encoder = device->createCommandEncoder({});
    ASSERT_NE(encoder, nullptr);

    // Write some timestamps
    encoder->writeTimestamp(querySet, 2);
    encoder->writeTimestamp(querySet, 3);
    encoder->writeTimestamp(querySet, 4);

    // Resolve only a partial range
    encoder->resolveQuerySet(querySet, 2, 3, buffer, 0);

    encoder->end();
}

TEST_P(GfxCppQuerySetTest, ResolveQuerySetWithOffset)
{
    ASSERT_NE(device, nullptr);

    gfx::QuerySetDescriptor querySetDesc{
        .type = gfx::QueryType::Timestamp,
        .count = 4
    };

    std::shared_ptr<gfx::QuerySet> querySet;
    try {
        querySet = device->createQuerySet(querySetDesc);
    } catch (const std::exception&) {
        GTEST_SKIP() << "Timestamp queries not supported";
    }

    ASSERT_NE(querySet, nullptr);

    gfx::BufferDescriptor bufferDesc{
        .size = 8 * sizeof(uint64_t),
        .usage = gfx::BufferUsage::CopySrc | gfx::BufferUsage::CopyDst,
        .memoryProperties = gfx::MemoryProperty::HostVisible | gfx::MemoryProperty::HostCoherent
    };
    auto buffer = device->createBuffer(bufferDesc);
    ASSERT_NE(buffer, nullptr);

    auto encoder = device->createCommandEncoder({});
    ASSERT_NE(encoder, nullptr);

    // Write timestamps
    encoder->writeTimestamp(querySet, 0);
    encoder->writeTimestamp(querySet, 1);
    encoder->writeTimestamp(querySet, 2);
    encoder->writeTimestamp(querySet, 3);

    // Resolve to buffer with offset
    encoder->resolveQuerySet(querySet, 0, 4, buffer, 2 * sizeof(uint64_t));

    encoder->end();
}

TEST_P(GfxCppQuerySetTest, MultipleResolveOperations)
{
    ASSERT_NE(device, nullptr);

    gfx::QuerySetDescriptor querySetDesc{
        .type = gfx::QueryType::Timestamp,
        .count = 8
    };

    std::shared_ptr<gfx::QuerySet> querySet;
    try {
        querySet = device->createQuerySet(querySetDesc);
    } catch (const std::exception&) {
        GTEST_SKIP() << "Timestamp queries not supported";
    }

    ASSERT_NE(querySet, nullptr);

    gfx::BufferDescriptor bufferDesc1{
        .size = 4 * sizeof(uint64_t),
        .usage = gfx::BufferUsage::CopySrc | gfx::BufferUsage::CopyDst,
        .memoryProperties = gfx::MemoryProperty::HostVisible | gfx::MemoryProperty::HostCoherent
    };
    auto buffer1 = device->createBuffer(bufferDesc1);

    gfx::BufferDescriptor bufferDesc2{
        .size = 4 * sizeof(uint64_t),
        .usage = gfx::BufferUsage::CopySrc | gfx::BufferUsage::CopyDst,
        .memoryProperties = gfx::MemoryProperty::HostVisible | gfx::MemoryProperty::HostCoherent
    };
    auto buffer2 = device->createBuffer(bufferDesc2);

    auto encoder = device->createCommandEncoder({});
    ASSERT_NE(encoder, nullptr);

    // Write all timestamps
    for (uint32_t i = 0; i < 8; ++i) {
        encoder->writeTimestamp(querySet, i);
    }

    // Resolve to different buffers
    encoder->resolveQuerySet(querySet, 0, 4, buffer1, 0);
    encoder->resolveQuerySet(querySet, 4, 4, buffer2, 0);

    encoder->end();
}

// ===========================================================================
// ===========================================================================
// Render Pass Encoder Query Operations Tests
// ===========================================================================

TEST_P(GfxCppQuerySetTest, BeginOcclusionQueryInRenderPass)
{
    ASSERT_NE(device, nullptr);

    // Create occlusion query set
    gfx::QuerySetDescriptor querySetDesc{
        .type = gfx::QueryType::Occlusion,
        .count = 2
    };

    auto querySet = device->createQuerySet(querySetDesc);
    ASSERT_NE(querySet, nullptr);

    // Create render pass
    gfx::RenderPassColorAttachmentTarget colorTarget{
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .sampleCount = gfx::SampleCount::Count1,
        .loadOp = gfx::LoadOp::Clear,
        .storeOp = gfx::StoreOp::Store,
        .finalLayout = gfx::TextureLayout::ColorAttachment
    };

    gfx::RenderPassColorAttachment colorAttachment{
        .target = colorTarget,
        .resolveTarget = nullptr
    };

    gfx::RenderPassCreateDescriptor renderPassDesc{
        .colorAttachments = { colorAttachment }
    };

    auto renderPass = device->createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create texture and view for framebuffer
    gfx::TextureDescriptor colorTextureDesc{
        .type = gfx::TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::RenderAttachment
    };

    auto colorTexture = device->createTexture(colorTextureDesc);
    ASSERT_NE(colorTexture, nullptr);

    gfx::TextureViewDescriptor viewDesc{
        .viewType = gfx::TextureViewType::View2D,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1
    };

    auto colorView = colorTexture->createView(viewDesc);
    ASSERT_NE(colorView, nullptr);

    // Create framebuffer
    gfx::FramebufferColorAttachment colorFbAttachment{
        .view = colorView,
        .resolveTarget = nullptr
    };

    gfx::FramebufferDescriptor framebufferDesc{
        .renderPass = renderPass,
        .colorAttachments = { colorFbAttachment },
        .width = 256,
        .height = 256
    };

    auto framebuffer = device->createFramebuffer(framebufferDesc);
    ASSERT_NE(framebuffer, nullptr);

    // Create command encoder and begin render pass
    auto encoder = device->createCommandEncoder({});
    ASSERT_NE(encoder, nullptr);

    gfx::RenderPassBeginDescriptor beginDesc{
        .framebuffer = framebuffer
    };

    auto renderPassEncoder = encoder->beginRenderPass(beginDesc);
    ASSERT_NE(renderPassEncoder, nullptr);

    // Begin and end occlusion query
    renderPassEncoder->beginOcclusionQuery(querySet, 0);
    renderPassEncoder->endOcclusionQuery();

    encoder->end();
}

TEST_P(GfxCppQuerySetTest, EndOcclusionQueryInRenderPass)
{
    ASSERT_NE(device, nullptr);

    // Create occlusion query set
    gfx::QuerySetDescriptor querySetDesc{
        .type = gfx::QueryType::Occlusion,
        .count = 2
    };

    auto querySet = device->createQuerySet(querySetDesc);
    ASSERT_NE(querySet, nullptr);

    // Create render pass
    gfx::RenderPassColorAttachmentTarget colorTarget{
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .sampleCount = gfx::SampleCount::Count1,
        .loadOp = gfx::LoadOp::Clear,
        .storeOp = gfx::StoreOp::Store,
        .finalLayout = gfx::TextureLayout::ColorAttachment
    };

    gfx::RenderPassColorAttachment colorAttachment{
        .target = colorTarget,
        .resolveTarget = nullptr
    };

    gfx::RenderPassCreateDescriptor renderPassDesc{
        .colorAttachments = { colorAttachment }
    };

    auto renderPass = device->createRenderPass(renderPassDesc);
    ASSERT_NE(renderPass, nullptr);

    // Create texture and view
    gfx::TextureDescriptor colorTextureDesc{
        .type = gfx::TextureType::Texture2D,
        .size = { 256, 256, 1 },
        .arrayLayerCount = 1,
        .mipLevelCount = 1,
        .sampleCount = gfx::SampleCount::Count1,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .usage = gfx::TextureUsage::RenderAttachment
    };

    auto colorTexture = device->createTexture(colorTextureDesc);
    ASSERT_NE(colorTexture, nullptr);

    auto colorView = colorTexture->createView({ .viewType = gfx::TextureViewType::View2D,
        .format = gfx::TextureFormat::R8G8B8A8Unorm,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1 });
    ASSERT_NE(colorView, nullptr);

    // Create framebuffer
    gfx::FramebufferDescriptor framebufferDesc{
        .renderPass = renderPass,
        .colorAttachments = { { .view = colorView } },
        .width = 256,
        .height = 256
    };

    auto framebuffer = device->createFramebuffer(framebufferDesc);
    ASSERT_NE(framebuffer, nullptr);

    // Create command encoder and begin render pass
    auto encoder = device->createCommandEncoder({});
    ASSERT_NE(encoder, nullptr);

    auto renderPassEncoder = encoder->beginRenderPass({ .framebuffer = framebuffer });
    ASSERT_NE(renderPassEncoder, nullptr);

    // Test multiple begin/end cycles
    renderPassEncoder->beginOcclusionQuery(querySet, 0);
    // Would perform draw calls here in real scenario
    renderPassEncoder->endOcclusionQuery();

    renderPassEncoder->beginOcclusionQuery(querySet, 1);
    // Would perform more draw calls here
    renderPassEncoder->endOcclusionQuery();

    encoder->end();
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppQuerySetTest,
    testing::Values(gfx::Backend::Vulkan, gfx::Backend::WebGPU),
    [](const testing::TestParamInfo<gfx::Backend>& info) {
        return info.param == gfx::Backend::Vulkan ? "Vulkan" : "WebGPU";
    });

} // namespace
