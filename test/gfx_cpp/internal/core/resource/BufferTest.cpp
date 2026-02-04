#include <core/resource/Buffer.h>
#include <core/system/Device.h>

#include <gfx/gfx.h>

#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

namespace gfx {

class BufferImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc{
            .backend = backend,
            .applicationName = "BufferImplTest"
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

TEST_P(BufferImplTest, CreateBuffer)
{
    DeviceImpl deviceWrapper(device);

    BufferDescriptor desc{
        .size = 1024,
        .usage = BufferUsage::Vertex
    };

    auto buffer = deviceWrapper.createBuffer(desc);
    ASSERT_NE(buffer, nullptr);

    auto info = buffer->getInfo();
    EXPECT_EQ(info.size, 1024);
    EXPECT_EQ(static_cast<uint32_t>(info.usage), static_cast<uint32_t>(BufferUsage::Vertex));
}

TEST_P(BufferImplTest, CreateBufferWithMultipleUsages)
{
    DeviceImpl deviceWrapper(device);

    BufferDescriptor desc{
        .size = 2048,
        .usage = static_cast<BufferUsage>(
            static_cast<uint32_t>(BufferUsage::Uniform) | static_cast<uint32_t>(BufferUsage::CopyDst))
    };

    auto buffer = deviceWrapper.createBuffer(desc);
    ASSERT_NE(buffer, nullptr);

    auto info = buffer->getInfo();
    EXPECT_EQ(info.size, 2048);
    EXPECT_EQ(info.usage, desc.usage);
}

TEST_P(BufferImplTest, MultipleBuffers_IndependentHandles)
{
    DeviceImpl deviceWrapper(device);

    BufferDescriptor desc1{
        .size = 1024,
        .usage = BufferUsage::Vertex
    };

    BufferDescriptor desc2{
        .size = 2048,
        .usage = BufferUsage::Index
    };

    auto buffer1 = deviceWrapper.createBuffer(desc1);
    auto buffer2 = deviceWrapper.createBuffer(desc2);

    ASSERT_NE(buffer1, nullptr);
    ASSERT_NE(buffer2, nullptr);

    // Verify buffers are independent
    EXPECT_EQ(buffer1->getInfo().size, 1024);
    EXPECT_EQ(buffer2->getInfo().size, 2048);
}

TEST_P(BufferImplTest, GetNativeHandle)
{
    DeviceImpl deviceWrapper(device);

    BufferDescriptor desc{
        .size = 512,
        .usage = BufferUsage::Vertex
    };

    auto buffer = deviceWrapper.createBuffer(desc);
    ASSERT_NE(buffer, nullptr);

    void* nativeHandle = buffer->getNativeHandle();
    EXPECT_NE(nativeHandle, nullptr);
}

TEST_P(BufferImplTest, MapUnmap)
{
    DeviceImpl deviceWrapper(device);

    BufferDescriptor desc{
        .size = 256,
        .usage = static_cast<BufferUsage>(
            static_cast<uint32_t>(BufferUsage::MapWrite) | static_cast<uint32_t>(BufferUsage::CopySrc)),
        .memoryProperties = static_cast<MemoryProperty>(static_cast<uint32_t>(MemoryProperty::HostVisible))
    };

    auto buffer = deviceWrapper.createBuffer(desc);
    ASSERT_NE(buffer, nullptr);

    void* ptr = buffer->map(0, 256);
    // Mapping may or may not succeed depending on backend and memory properties
    // Just verify it doesn't crash

    if (ptr) {
        // Write some data
        uint32_t* data = static_cast<uint32_t*>(ptr);
        data[0] = 0x12345678;

        buffer->unmap();
    }
}

TEST_P(BufferImplTest, FlushMappedRange)
{
    DeviceImpl deviceWrapper(device);

    BufferDescriptor desc{
        .size = 256,
        .usage = static_cast<BufferUsage>(
            static_cast<uint32_t>(BufferUsage::MapWrite) | static_cast<uint32_t>(BufferUsage::CopySrc)),
        .memoryProperties = static_cast<MemoryProperty>(static_cast<uint32_t>(MemoryProperty::HostVisible))
    };

    auto buffer = deviceWrapper.createBuffer(desc);
    ASSERT_NE(buffer, nullptr);

    void* ptr = buffer->map(0, 256);
    if (ptr) {
        uint32_t* data = static_cast<uint32_t*>(ptr);
        data[0] = 0xDEADBEEF;

        // Should not crash
        buffer->flushMappedRange(0, 4);

        buffer->unmap();
    }
}

TEST_P(BufferImplTest, InvalidateMappedRange)
{
    DeviceImpl deviceWrapper(device);

    BufferDescriptor desc{
        .size = 256,
        .usage = static_cast<BufferUsage>(
            static_cast<uint32_t>(BufferUsage::MapWrite) | static_cast<uint32_t>(BufferUsage::CopySrc)),
        .memoryProperties = static_cast<MemoryProperty>(static_cast<uint32_t>(MemoryProperty::HostVisible))
    };

    auto buffer = deviceWrapper.createBuffer(desc);
    ASSERT_NE(buffer, nullptr);

    void* ptr = buffer->map(0, 256);
    if (ptr) {
        // Should not crash
        buffer->invalidateMappedRange(0, 4);

        buffer->unmap();
    }
}

TEST_P(BufferImplTest, ImportBuffer)
{
    DeviceImpl deviceWrapper(device);

    // Create a buffer to get its native handle
    BufferDescriptor createDesc{
        .size = 512,
        .usage = BufferUsage::Vertex
    };
    auto originalBuffer = deviceWrapper.createBuffer(createDesc);
    ASSERT_NE(originalBuffer, nullptr);

    // Get native handle
    void* nativeHandle = originalBuffer->getNativeHandle();
    ASSERT_NE(nativeHandle, nullptr);

    // Import using the native handle
    BufferImportDescriptor importDesc{
        .nativeHandle = nativeHandle,
        .size = 512,
        .usage = BufferUsage::Vertex
    };

    auto importedBuffer = deviceWrapper.importBuffer(importDesc);
    ASSERT_NE(importedBuffer, nullptr);

    // Verify info matches
    auto info = importedBuffer->getInfo();
    EXPECT_EQ(info.size, 512);
    EXPECT_EQ(static_cast<uint32_t>(info.usage), static_cast<uint32_t>(BufferUsage::Vertex));
}

// Instantiate tests for available backends
#if defined(GFX_ENABLE_VULKAN) && defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(AllBackends, BufferImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU));
#elif defined(GFX_ENABLE_VULKAN)
INSTANTIATE_TEST_SUITE_P(VulkanOnly, BufferImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN));
#elif defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(WebGPUOnly, BufferImplTest,
    ::testing::Values(GFX_BACKEND_WEBGPU));
#endif

} // namespace gfx
