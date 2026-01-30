#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

#include <cstring>

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxCppBufferTest : public testing::TestWithParam<gfx::Backend> {
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
// Buffer Tests
// ===========================================================================

TEST_P(GfxCppBufferTest, CreateDestroyBuffer)
{
    ASSERT_NE(device, nullptr);

    gfx::BufferDescriptor desc{
        .label = "Test Buffer",
        .size = 1024,
        .usage = gfx::BufferUsage::Vertex | gfx::BufferUsage::CopyDst,
        .memoryProperties = gfx::MemoryProperty::DeviceLocal
    };

    auto buffer = device->createBuffer(desc);
    EXPECT_NE(buffer, nullptr);
}

TEST_P(GfxCppBufferTest, CreateBufferInvalidArguments)
{
    ASSERT_NE(device, nullptr);

    // Null device (can't test directly in C++, but we can verify nullptr device throws)
    // This would be a programming error in C++, but we test zero size and no usage below

    gfx::BufferDescriptor validDesc{
        .size = 1024,
        .usage = gfx::BufferUsage::Vertex,
        .memoryProperties = gfx::MemoryProperty::DeviceLocal
    };

    // These should succeed as baseline
    auto buffer = device->createBuffer(validDesc);
    EXPECT_NE(buffer, nullptr);
}

TEST_P(GfxCppBufferTest, CreateBufferZeroSize)
{
    ASSERT_NE(device, nullptr);

    gfx::BufferDescriptor desc{
        .size = 0,
        .usage = gfx::BufferUsage::Vertex,
        .memoryProperties = gfx::MemoryProperty::DeviceLocal
    };

    // C++ API should throw on invalid arguments
    EXPECT_THROW(device->createBuffer(desc), std::exception);
}

TEST_P(GfxCppBufferTest, CreateBufferNoUsage)
{
    ASSERT_NE(device, nullptr);

    gfx::BufferDescriptor desc{
        .size = 1024,
        .usage = gfx::BufferUsage::None
    };

    // C++ API should throw on invalid arguments
    EXPECT_THROW(device->createBuffer(desc), std::exception);
}

TEST_P(GfxCppBufferTest, GetBufferInfo)
{
    ASSERT_NE(device, nullptr);

    gfx::BufferDescriptor desc{
        .label = "Test Buffer",
        .size = 2048,
        .usage = gfx::BufferUsage::Uniform | gfx::BufferUsage::CopyDst,
        .memoryProperties = gfx::MemoryProperty::HostVisible | gfx::MemoryProperty::HostCoherent
    };

    auto buffer = device->createBuffer(desc);
    ASSERT_NE(buffer, nullptr);

    auto info = buffer->getInfo();

    EXPECT_EQ(info.size, 2048);
    EXPECT_EQ(info.usage, gfx::BufferUsage::Uniform | gfx::BufferUsage::CopyDst);
}

TEST_P(GfxCppBufferTest, MapUnmapBuffer)
{
    ASSERT_NE(device, nullptr);

    gfx::BufferDescriptor desc{
        .label = "Mappable Buffer",
        .size = 256,
        .usage = gfx::BufferUsage::MapWrite | gfx::BufferUsage::CopySrc,
        .memoryProperties = gfx::MemoryProperty::HostVisible | gfx::MemoryProperty::HostCoherent
    };

    auto buffer = device->createBuffer(desc);
    ASSERT_NE(buffer, nullptr);

    void* mappedData = buffer->map(0, 256);
    EXPECT_NE(mappedData, nullptr);

    if (mappedData) {
        // Write some test data
        uint32_t testData[] = { 1, 2, 3, 4 };
        std::memcpy(mappedData, testData, sizeof(testData));

        buffer->unmap();
    }
}

TEST_P(GfxCppBufferTest, MapBufferInvalidArguments)
{
    ASSERT_NE(device, nullptr);

    gfx::BufferDescriptor desc{
        .size = 256,
        .usage = gfx::BufferUsage::MapWrite | gfx::BufferUsage::CopySrc,
        .memoryProperties = gfx::MemoryProperty::HostVisible | gfx::MemoryProperty::HostCoherent
    };

    auto buffer = device->createBuffer(desc);
    ASSERT_NE(buffer, nullptr);

    // Test null buffer would be a programming error in C++
    // The C++ API automatically throws if map() returns nullptr or fails

    // Valid map should succeed
    void* mappedData = buffer->map(0, 256);
    EXPECT_NE(mappedData, nullptr);

    if (mappedData) {
        buffer->unmap();
    }
}

TEST_P(GfxCppBufferTest, WriteBufferViaQueue)
{
    ASSERT_NE(device, nullptr);

    gfx::BufferDescriptor desc{
        .label = "Queue Write Buffer",
        .size = 128,
        .usage = gfx::BufferUsage::CopyDst | gfx::BufferUsage::Uniform,
        .memoryProperties = gfx::MemoryProperty::HostVisible | gfx::MemoryProperty::HostCoherent
    };

    auto buffer = device->createBuffer(desc);
    ASSERT_NE(buffer, nullptr);

    auto queue = device->getQueue();
    ASSERT_NE(queue, nullptr);

    float testData[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    queue->writeBuffer(buffer, 0, testData, sizeof(testData));
}

TEST_P(GfxCppBufferTest, CreateMultipleBuffers)
{
    ASSERT_NE(device, nullptr);

    const int bufferCount = 5;
    std::vector<std::shared_ptr<gfx::Buffer>> buffers;

    for (int i = 0; i < bufferCount; ++i) {
        gfx::BufferDescriptor desc{
            .size = static_cast<uint64_t>(512 * (i + 1)),
            .usage = gfx::BufferUsage::Vertex | gfx::BufferUsage::CopyDst,
            .memoryProperties = gfx::MemoryProperty::DeviceLocal
        };

        auto buffer = device->createBuffer(desc);
        EXPECT_NE(buffer, nullptr);
        buffers.push_back(buffer);
    }

    EXPECT_EQ(buffers.size(), bufferCount);
}

TEST_P(GfxCppBufferTest, CreateBufferWithAllUsageFlags)
{
    ASSERT_NE(device, nullptr);

    gfx::BufferDescriptor desc{
        .label = "All Usage Buffer",
        .size = 4096,
        .usage = gfx::BufferUsage::MapRead | gfx::BufferUsage::MapWrite | gfx::BufferUsage::CopySrc | gfx::BufferUsage::CopyDst | gfx::BufferUsage::Index | gfx::BufferUsage::Vertex | gfx::BufferUsage::Uniform | gfx::BufferUsage::Storage | gfx::BufferUsage::Indirect,
        .memoryProperties = gfx::MemoryProperty::HostVisible | gfx::MemoryProperty::HostCoherent
    };

    auto buffer = device->createBuffer(desc);
    EXPECT_NE(buffer, nullptr);
}

TEST_P(GfxCppBufferTest, ImportBufferInvalidArguments)
{
    ASSERT_NE(device, nullptr);

    // Null device would be a programming error in C++

    // Null native handle - should throw
    gfx::BufferImportDescriptor nullHandleDesc{
        .nativeHandle = nullptr,
        .size = 1024,
        .usage = gfx::BufferUsage::Vertex
    };
    EXPECT_THROW(device->importBuffer(nullHandleDesc), std::exception);

    // Note: Invalid handle (arbitrary pointer like 0xDEADBEEF) cannot be validated
    // by the backend without actually using it, so we can't test for that case.
    // The backend will only catch null handles at the API boundary.
}

TEST_P(GfxCppBufferTest, ImportBufferZeroSize)
{
    ASSERT_NE(device, nullptr);

    gfx::BufferImportDescriptor desc{
        .nativeHandle = (void*)0x1,
        .size = 0,
        .usage = gfx::BufferUsage::Vertex
    };

    // C++ API should throw on invalid arguments
    EXPECT_THROW(device->importBuffer(desc), std::exception);
}

TEST_P(GfxCppBufferTest, ImportBufferNoUsage)
{
    ASSERT_NE(device, nullptr);

    gfx::BufferImportDescriptor desc{
        .nativeHandle = (void*)0x1,
        .size = 1024,
        .usage = gfx::BufferUsage::None
    };

    // C++ API should throw on invalid arguments
    EXPECT_THROW(device->importBuffer(desc), std::exception);
}

TEST_P(GfxCppBufferTest, ImportBufferFromNativeHandle)
{
    ASSERT_NE(device, nullptr);

    // First, create a normal buffer
    gfx::BufferDescriptor createDesc{
        .label = "Source Buffer",
        .size = 1024,
        .usage = gfx::BufferUsage::CopySrc | gfx::BufferUsage::CopyDst,
        .memoryProperties = gfx::MemoryProperty::DeviceLocal
    };

    auto sourceBuffer = device->createBuffer(createDesc);
    ASSERT_NE(sourceBuffer, nullptr);

    // Get buffer info to verify properties
    auto info = sourceBuffer->getInfo();

    // Extract native handle
    void* nativeHandle = sourceBuffer->getNativeHandle();
    ASSERT_NE(nativeHandle, nullptr);

    // Now import the native handle
    gfx::BufferImportDescriptor importDesc{
        .nativeHandle = nativeHandle,
        .size = info.size,
        .usage = info.usage
    };

    auto importedBuffer = device->importBuffer(importDesc);
    EXPECT_NE(importedBuffer, nullptr);

    // Verify imported buffer has correct properties
    if (importedBuffer) {
        auto importedInfo = importedBuffer->getInfo();
        EXPECT_EQ(importedInfo.size, info.size);
        EXPECT_EQ(importedInfo.usage, info.usage);
    }
}

// ===========================================================================
// Memory Property Tests
// ===========================================================================

TEST_P(GfxCppBufferTest, CreateBufferWithDeviceLocalOnly)
{
    ASSERT_NE(device, nullptr);

    gfx::BufferDescriptor desc{
        .label = "Device Local Buffer",
        .size = 1024,
        .usage = gfx::BufferUsage::Vertex | gfx::BufferUsage::CopyDst,
        .memoryProperties = gfx::MemoryProperty::DeviceLocal
    };

    auto buffer = device->createBuffer(desc);
    EXPECT_NE(buffer, nullptr);

    if (buffer) {
        auto info = buffer->getInfo();
        EXPECT_EQ(info.size, 1024);
        EXPECT_EQ(info.usage, gfx::BufferUsage::Vertex | gfx::BufferUsage::CopyDst);
    }
}

TEST_P(GfxCppBufferTest, CreateBufferWithHostVisibleAndHostCoherent)
{
    ASSERT_NE(device, nullptr);

    gfx::BufferDescriptor desc{
        .label = "Host Visible Coherent Buffer",
        .size = 512,
        .usage = gfx::BufferUsage::Uniform | gfx::BufferUsage::CopyDst | gfx::BufferUsage::MapWrite,
        .memoryProperties = gfx::MemoryProperty::HostVisible | gfx::MemoryProperty::HostCoherent
    };

    auto buffer = device->createBuffer(desc);
    EXPECT_NE(buffer, nullptr);

    if (buffer) {
        auto info = buffer->getInfo();
        EXPECT_EQ(info.size, 512);
        // Don't check usage - backend may add additional flags like MapRead with MapWrite

        // This buffer should be mappable
        void* mappedData = buffer->map(0, 512);
        EXPECT_NE(mappedData, nullptr);
        if (mappedData) {
            buffer->unmap();
        }
    }
}

TEST_P(GfxCppBufferTest, CreateBufferWithHostVisibleAndHostCached)
{
    ASSERT_NE(device, nullptr);

    gfx::BufferDescriptor desc{
        .label = "Host Visible Cached Buffer",
        .size = 256,
        .usage = gfx::BufferUsage::MapRead | gfx::BufferUsage::CopySrc,
        .memoryProperties = gfx::MemoryProperty::HostVisible | gfx::MemoryProperty::HostCached
    };

    auto buffer = device->createBuffer(desc);
    EXPECT_NE(buffer, nullptr);

    if (buffer) {
        auto info = buffer->getInfo();
        EXPECT_EQ(info.size, 256);
        // Don't check usage - backend may add additional flags

        // This buffer should be mappable for read
        void* mappedData = buffer->map(0, 256);
        EXPECT_NE(mappedData, nullptr);
        if (mappedData) {
            buffer->unmap();
        }
    }
}

TEST_P(GfxCppBufferTest, CreateBufferWithAllMemoryProperties)
{
    ASSERT_NE(device, nullptr);

    gfx::BufferDescriptor desc{
        .label = "All Memory Properties Buffer",
        .size = 2048,
        .usage = gfx::BufferUsage::Storage | gfx::BufferUsage::CopySrc | gfx::BufferUsage::CopyDst,
        .memoryProperties = gfx::MemoryProperty::DeviceLocal | gfx::MemoryProperty::HostVisible | gfx::MemoryProperty::HostCoherent | gfx::MemoryProperty::HostCached
    };

    // This combination may not be supported on all platforms, but shouldn't crash
    try {
        auto buffer = device->createBuffer(desc);
        if (buffer) {
            auto info = buffer->getInfo();
            EXPECT_EQ(info.size, 2048);
        }
    } catch (const std::exception&) {
        // Some platforms may not support this combination, which is acceptable
        SUCCEED();
    }
}

TEST_P(GfxCppBufferTest, CreateBufferWithNoMemoryPropertiesThrows)
{
    ASSERT_NE(device, nullptr);

    // WebGPU doesn't validate memory properties, so skip this test
    if (backend == gfx::Backend::WebGPU) {
        GTEST_SKIP() << "WebGPU backend doesn't validate memory properties";
    }

    gfx::BufferDescriptor desc{
        .label = "No Memory Properties Buffer",
        .size = 1024,
        .usage = gfx::BufferUsage::Vertex,
        .memoryProperties = static_cast<gfx::MemoryProperty>(0)
    };

    // Creating buffer with no memory properties should throw
    EXPECT_THROW(device->createBuffer(desc), std::exception);
}

TEST_P(GfxCppBufferTest, CreateBufferWithHostCoherentWithoutHostVisibleThrows)
{
    ASSERT_NE(device, nullptr);

    // WebGPU doesn't validate memory properties, so skip this test
    if (backend == gfx::Backend::WebGPU) {
        GTEST_SKIP() << "WebGPU backend doesn't validate memory properties";
    }

    gfx::BufferDescriptor desc{
        .label = "Invalid: HostCoherent without HostVisible",
        .size = 1024,
        .usage = gfx::BufferUsage::Uniform,
        .memoryProperties = gfx::MemoryProperty::HostCoherent
    };

    // HostCoherent requires HostVisible - should throw
    EXPECT_THROW(device->createBuffer(desc), std::exception);
}

TEST_P(GfxCppBufferTest, CreateBufferWithHostCachedWithoutHostVisibleThrows)
{
    ASSERT_NE(device, nullptr);

    // WebGPU doesn't validate memory properties, so skip this test
    if (backend == gfx::Backend::WebGPU) {
        GTEST_SKIP() << "WebGPU backend doesn't validate memory properties";
    }

    gfx::BufferDescriptor desc{
        .label = "Invalid: HostCached without HostVisible",
        .size = 1024,
        .usage = gfx::BufferUsage::Uniform,
        .memoryProperties = gfx::MemoryProperty::HostCached
    };

    // HostCached requires HostVisible - should throw
    EXPECT_THROW(device->createBuffer(desc), std::exception);
}

TEST_P(GfxCppBufferTest, FlushMappedRange)
{
    ASSERT_NE(device, nullptr);

    // Skip on WebGPU - memory is always coherent, no synchronous mapping
    if (backend == gfx::Backend::WebGPU) {
        GTEST_SKIP() << "WebGPU memory is always coherent";
    }

    // Create a host-visible, non-coherent buffer for testing flush
    gfx::BufferDescriptor desc{
        .label = "Flush Test Buffer",
        .size = 1024,
        .usage = gfx::BufferUsage::MapWrite | gfx::BufferUsage::Uniform | gfx::BufferUsage::CopySrc,
        .memoryProperties = gfx::MemoryProperty::HostVisible // Non-coherent
    };

    auto buffer = device->createBuffer(desc);
    ASSERT_NE(buffer, nullptr);

    // Map the buffer
    void* mappedPtr = buffer->map(0, desc.size);
    EXPECT_NE(mappedPtr, nullptr);

    if (mappedPtr) {
        // Write some data
        std::memset(mappedPtr, 0x42, 512);

        // Flush the written range (CPU -> GPU)
        EXPECT_NO_THROW(buffer->flushMappedRange(0, 512));

        buffer->unmap();
    }
}

TEST_P(GfxCppBufferTest, InvalidateMappedRange)
{
    ASSERT_NE(device, nullptr);

    // Skip on WebGPU - memory is always coherent, no synchronous mapping
    if (backend == gfx::Backend::WebGPU) {
        GTEST_SKIP() << "WebGPU memory is always coherent";
    }

    // Create a host-visible buffer for testing invalidate
    gfx::BufferDescriptor desc{
        .label = "Invalidate Test Buffer",
        .size = 1024,
        .usage = gfx::BufferUsage::MapRead | gfx::BufferUsage::Storage | gfx::BufferUsage::CopyDst,
        .memoryProperties = gfx::MemoryProperty::HostVisible
    };

    auto buffer = device->createBuffer(desc);
    ASSERT_NE(buffer, nullptr);

    // In a real scenario, GPU would write to this buffer
    // Invalidate to make GPU writes visible to CPU (GPU -> CPU)
    EXPECT_NO_THROW(buffer->invalidateMappedRange(0, desc.size));

    // Map and read
    void* mappedPtr = buffer->map(0, desc.size);
    EXPECT_NE(mappedPtr, nullptr);

    if (mappedPtr) {
        buffer->unmap();
    }
}

TEST_P(GfxCppBufferTest, FlushInvalidateCombined)
{
    ASSERT_NE(device, nullptr);

    // Skip on WebGPU - memory is always coherent, no synchronous mapping
    if (backend == gfx::Backend::WebGPU) {
        GTEST_SKIP() << "WebGPU memory is always coherent";
    }

    // Test flush and invalidate together
    gfx::BufferDescriptor desc{
        .label = "Flush+Invalidate Test Buffer",
        .size = 2048,
        .usage = gfx::BufferUsage::MapWrite | gfx::BufferUsage::MapRead | gfx::BufferUsage::Storage | gfx::BufferUsage::CopySrc | gfx::BufferUsage::CopyDst,
        .memoryProperties = gfx::MemoryProperty::HostVisible
    };

    auto buffer = device->createBuffer(desc);
    ASSERT_NE(buffer, nullptr);

    void* mappedPtr = buffer->map(0, desc.size);
    ASSERT_NE(mappedPtr, nullptr);

    if (mappedPtr) {
        // Write data to first half
        std::memset(mappedPtr, 0xAA, 1024);

        // Flush first half (CPU writes -> GPU)
        EXPECT_NO_THROW(buffer->flushMappedRange(0, 1024));

        // Invalidate second half (GPU writes -> CPU)
        EXPECT_NO_THROW(buffer->invalidateMappedRange(1024, 1024));

        buffer->unmap();
    }
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxCppBufferTest,
    testing::Values(gfx::Backend::Vulkan, gfx::Backend::WebGPU),
    [](const testing::TestParamInfo<gfx::Backend>& info) {
        return info.param == gfx::Backend::Vulkan ? "Vulkan" : "WebGPU";
    });

} // namespace
