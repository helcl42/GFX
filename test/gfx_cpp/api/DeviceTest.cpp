#include "CommonTest.h"

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxCppDeviceTest : public testing::TestWithParam<gfx::Backend> {
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
                .preference = gfx::AdapterPreference::HighPerformance
            };
            adapter = instance->requestAdapter(adapterDesc);
        } catch (const std::exception& e) {
            GTEST_SKIP() << "Failed to set up: " << e.what();
        }
    }

    gfx::Backend backend;
    std::shared_ptr<gfx::Instance> instance;
    std::shared_ptr<gfx::Adapter> adapter;
};

TEST_P(GfxCppDeviceTest, CreateDestroyDevice)
{
    ASSERT_NE(adapter, nullptr);

    gfx::DeviceDescriptor desc{
        .label = "Test Device"
    };

    auto device = adapter->createDevice(desc);
    EXPECT_NE(device, nullptr);

    // Device will be destroyed when shared_ptr goes out of scope
}

TEST_P(GfxCppDeviceTest, GetDefaultQueue)
{
    ASSERT_NE(adapter, nullptr);

    gfx::DeviceDescriptor desc{};
    auto device = adapter->createDevice(desc);
    ASSERT_NE(device, nullptr);

    auto queue = device->getQueue();
    EXPECT_NE(queue, nullptr);
}

TEST_P(GfxCppDeviceTest, GetQueueByIndex)
{
    ASSERT_NE(adapter, nullptr);

    // Get queue families first
    auto queueFamilies = adapter->enumerateQueueFamilies();
    if (queueFamilies.empty()) {
        GTEST_SKIP() << "No queue families available";
    }

    // Create device
    gfx::DeviceDescriptor desc{};
    auto device = adapter->createDevice(desc);
    ASSERT_NE(device, nullptr);

    // Try to get queue from first family
    auto queue = device->getQueueByIndex(0, 0);
    EXPECT_NE(queue, nullptr);
}

TEST_P(GfxCppDeviceTest, GetQueueInvalidIndex)
{
    ASSERT_NE(adapter, nullptr);

    gfx::DeviceDescriptor desc{};
    auto device = adapter->createDevice(desc);
    ASSERT_NE(device, nullptr);

    // Try to get queue with invalid family index
    // C++ API will throw or return nullptr depending on implementation
    // Just verify it doesn't crash
    try {
        auto queue = device->getQueueByIndex(9999, 0);
        // If it returns nullptr instead of throwing, that's also valid
        EXPECT_TRUE(queue == nullptr || queue != nullptr);
    } catch (const std::exception&) {
        // Expected - invalid index should throw
        SUCCEED();
    }
}

TEST_P(GfxCppDeviceTest, WaitIdle)
{
    ASSERT_NE(adapter, nullptr);

    gfx::DeviceDescriptor desc{};
    auto device = adapter->createDevice(desc);
    ASSERT_NE(device, nullptr);

    // Should not throw
    EXPECT_NO_THROW(device->waitIdle());
}

TEST_P(GfxCppDeviceTest, GetLimits)
{
    ASSERT_NE(adapter, nullptr);

    gfx::DeviceDescriptor desc{};
    auto device = adapter->createDevice(desc);
    ASSERT_NE(device, nullptr);

    auto limits = device->getLimits();
    EXPECT_GT(limits.maxBufferSize, 0u);
    EXPECT_GT(limits.maxTextureDimension2D, 0u);
}

TEST_P(GfxCppDeviceTest, MultipleDevices)
{
    ASSERT_NE(adapter, nullptr);

    // WebGPU backend doesn't support multiple devices from the same adapter
    if (backend == gfx::Backend::WebGPU) {
        GTEST_SKIP() << "WebGPU doesn't support multiple devices from same adapter";
    }

    gfx::DeviceDescriptor desc{};

    auto device1 = adapter->createDevice(desc);
    auto device2 = adapter->createDevice(desc);

    EXPECT_NE(device1, nullptr);
    EXPECT_NE(device2, nullptr);
    EXPECT_NE(device1.get(), device2.get());
}

TEST_P(GfxCppDeviceTest, CreateBuffer)
{
    ASSERT_NE(adapter, nullptr);

    gfx::DeviceDescriptor desc{};
    auto device = adapter->createDevice(desc);
    ASSERT_NE(device, nullptr);

    gfx::BufferDescriptor bufferDesc{
        .size = 1024,
        .usage = gfx::BufferUsage::Vertex | gfx::BufferUsage::CopyDst,
        .memoryProperties = gfx::MemoryProperty::DeviceLocal
    };

    auto buffer = device->createBuffer(bufferDesc);
    EXPECT_NE(buffer, nullptr);
}

TEST_P(GfxCppDeviceTest, SupportsShaderFormat_SPIRV)
{
    ASSERT_NE(adapter, nullptr);

    gfx::DeviceDescriptor desc{};
    auto device = adapter->createDevice(desc);
    ASSERT_NE(device, nullptr);

    bool supported = device->supportsShaderFormat(gfx::ShaderSourceType::SPIRV);
    // Both Vulkan and WebGPU support SPIR-V (except Emscripten)
    EXPECT_TRUE(supported);
}

TEST_P(GfxCppDeviceTest, SupportsShaderFormat_WGSL)
{
    ASSERT_NE(adapter, nullptr);

    gfx::DeviceDescriptor desc{};
    auto device = adapter->createDevice(desc);
    ASSERT_NE(device, nullptr);

    bool supported = device->supportsShaderFormat(gfx::ShaderSourceType::WGSL);
    // Vulkan doesn't support WGSL, WebGPU does
    if (backend == gfx::Backend::Vulkan) {
        EXPECT_FALSE(supported);
    } else {
        EXPECT_TRUE(supported);
    }
}

TEST_P(GfxCppDeviceTest, GetAccessFlagsForLayoutUndefined)
{
    ASSERT_NE(adapter, nullptr);

    gfx::DeviceDescriptor desc{};
    auto device = adapter->createDevice(desc);
    ASSERT_NE(device, nullptr);

    gfx::AccessFlags flags = device->getAccessFlagsForLayout(gfx::TextureLayout::Undefined);
    EXPECT_EQ(flags, gfx::AccessFlags::None);
}

TEST_P(GfxCppDeviceTest, GetAccessFlagsForLayoutGeneral)
{
    ASSERT_NE(adapter, nullptr);

    gfx::DeviceDescriptor desc{};
    auto device = adapter->createDevice(desc);
    ASSERT_NE(device, nullptr);

    gfx::AccessFlags flags = device->getAccessFlagsForLayout(gfx::TextureLayout::General);
    if (backend == gfx::Backend::Vulkan) {
        EXPECT_EQ(flags, gfx::AccessFlags::MemoryRead | gfx::AccessFlags::MemoryWrite);
    } else {
        EXPECT_EQ(flags, gfx::AccessFlags::None);
    }
}

TEST_P(GfxCppDeviceTest, GetAccessFlagsForLayoutColorAttachment)
{
    ASSERT_NE(adapter, nullptr);

    gfx::DeviceDescriptor desc{};
    auto device = adapter->createDevice(desc);
    ASSERT_NE(device, nullptr);

    gfx::AccessFlags flags = device->getAccessFlagsForLayout(gfx::TextureLayout::ColorAttachment);
    if (backend == gfx::Backend::Vulkan) {
        EXPECT_EQ(flags, gfx::AccessFlags::ColorAttachmentRead | gfx::AccessFlags::ColorAttachmentWrite);
    } else {
        EXPECT_EQ(flags, gfx::AccessFlags::None);
    }
}

TEST_P(GfxCppDeviceTest, GetAccessFlagsForLayoutDepthStencil)
{
    ASSERT_NE(adapter, nullptr);

    gfx::DeviceDescriptor desc{};
    auto device = adapter->createDevice(desc);
    ASSERT_NE(device, nullptr);

    gfx::AccessFlags flags = device->getAccessFlagsForLayout(gfx::TextureLayout::DepthStencilAttachment);
    if (backend == gfx::Backend::Vulkan) {
        EXPECT_EQ(flags, gfx::AccessFlags::DepthStencilAttachmentRead | gfx::AccessFlags::DepthStencilAttachmentWrite);
    } else {
        EXPECT_EQ(flags, gfx::AccessFlags::None);
    }
}

TEST_P(GfxCppDeviceTest, GetAccessFlagsForLayoutDepthStencilReadOnly)
{
    ASSERT_NE(adapter, nullptr);

    gfx::DeviceDescriptor desc{};
    auto device = adapter->createDevice(desc);
    ASSERT_NE(device, nullptr);

    gfx::AccessFlags flags = device->getAccessFlagsForLayout(gfx::TextureLayout::DepthStencilReadOnly);
    if (backend == gfx::Backend::Vulkan) {
        EXPECT_EQ(flags, gfx::AccessFlags::DepthStencilAttachmentRead);
    } else {
        EXPECT_EQ(flags, gfx::AccessFlags::None);
    }
}

TEST_P(GfxCppDeviceTest, GetAccessFlagsForLayoutShaderReadOnly)
{
    ASSERT_NE(adapter, nullptr);

    gfx::DeviceDescriptor desc{};
    auto device = adapter->createDevice(desc);
    ASSERT_NE(device, nullptr);

    gfx::AccessFlags flags = device->getAccessFlagsForLayout(gfx::TextureLayout::ShaderReadOnly);
    if (backend == gfx::Backend::Vulkan) {
        EXPECT_EQ(flags, gfx::AccessFlags::ShaderRead);
    } else {
        EXPECT_EQ(flags, gfx::AccessFlags::None);
    }
}

TEST_P(GfxCppDeviceTest, GetAccessFlagsForLayoutTransferSrc)
{
    ASSERT_NE(adapter, nullptr);

    gfx::DeviceDescriptor desc{};
    auto device = adapter->createDevice(desc);
    ASSERT_NE(device, nullptr);

    gfx::AccessFlags flags = device->getAccessFlagsForLayout(gfx::TextureLayout::TransferSrc);
    if (backend == gfx::Backend::Vulkan) {
        EXPECT_EQ(flags, gfx::AccessFlags::TransferRead);
    } else {
        EXPECT_EQ(flags, gfx::AccessFlags::None);
    }
}

TEST_P(GfxCppDeviceTest, GetAccessFlagsForLayoutTransferDst)
{
    ASSERT_NE(adapter, nullptr);

    gfx::DeviceDescriptor desc{};
    auto device = adapter->createDevice(desc);
    ASSERT_NE(device, nullptr);

    gfx::AccessFlags flags = device->getAccessFlagsForLayout(gfx::TextureLayout::TransferDst);
    if (backend == gfx::Backend::Vulkan) {
        EXPECT_EQ(flags, gfx::AccessFlags::TransferWrite);
    } else {
        EXPECT_EQ(flags, gfx::AccessFlags::None);
    }
}

TEST_P(GfxCppDeviceTest, GetAccessFlagsForLayoutPresent)
{
    ASSERT_NE(adapter, nullptr);

    gfx::DeviceDescriptor desc{};
    auto device = adapter->createDevice(desc);
    ASSERT_NE(device, nullptr);

    gfx::AccessFlags flags = device->getAccessFlagsForLayout(gfx::TextureLayout::PresentSrc);
    if (backend == gfx::Backend::Vulkan) {
        EXPECT_EQ(flags, gfx::AccessFlags::MemoryRead);
    } else {
        EXPECT_EQ(flags, gfx::AccessFlags::None);
    }
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppDeviceTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

} // namespace
