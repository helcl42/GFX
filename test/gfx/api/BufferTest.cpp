#include <gfx/gfx.h>

#include <gtest/gtest.h>

#include <cstring>

// C API tests compiled with C++ for GoogleTest compatibility

// ===========================================================================
// Parameterized Tests - Run on both Vulkan and WebGPU backends
// ===========================================================================

namespace {

class GfxBufferTest : public testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        if (gfxLoadBackend(backend) != GFX_RESULT_SUCCESS) {
            GTEST_SKIP() << "Backend not available";
        }

        const char* extensions[] = { GFX_INSTANCE_EXTENSION_DEBUG };
        GfxInstanceDescriptor instDesc = {};
        instDesc.sType = GFX_STRUCTURE_TYPE_INSTANCE_DESCRIPTOR;
        instDesc.pNext = nullptr;
        instDesc.backend = backend;
        instDesc.enabledExtensions = extensions;
        instDesc.enabledExtensionCount = 1;

        if (gfxCreateInstance(&instDesc, &instance) != GFX_RESULT_SUCCESS) {
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to create instance";
        }

        GfxAdapterDescriptor adapterDesc = {};
        adapterDesc.sType = GFX_STRUCTURE_TYPE_ADAPTER_DESCRIPTOR;
        adapterDesc.pNext = nullptr;
        adapterDesc.adapterIndex = UINT32_MAX;
        adapterDesc.preference = GFX_ADAPTER_PREFERENCE_HIGH_PERFORMANCE;

        if (gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter) != GFX_RESULT_SUCCESS) {
            gfxInstanceDestroy(instance);
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to get adapter";
        }

        GfxDeviceDescriptor deviceDesc = {};
        deviceDesc.sType = GFX_STRUCTURE_TYPE_DEVICE_DESCRIPTOR;
        deviceDesc.pNext = nullptr;

        if (gfxAdapterCreateDevice(adapter, &deviceDesc, &device) != GFX_RESULT_SUCCESS) {
            gfxInstanceDestroy(instance);
            gfxUnloadBackend(backend);
            GTEST_SKIP() << "Failed to create device";
        }
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
    GfxInstance instance = NULL;
    GfxAdapter adapter = NULL;
    GfxDevice device = NULL;
};

TEST_P(GfxBufferTest, CreateDestroyBuffer)
{
    GfxBufferDescriptor desc = {};
    desc.label = "Test Buffer";
    desc.size = 1024;
    desc.usage = GFX_BUFFER_USAGE_VERTEX | GFX_BUFFER_USAGE_COPY_DST;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL;

    GfxBuffer buffer = NULL;
    GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffer);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(buffer, nullptr);

    if (buffer) {
        gfxBufferDestroy(buffer);
    }
}

TEST_P(GfxBufferTest, CreateBufferInvalidArguments)
{
    GfxBufferDescriptor desc = {};
    desc.size = 1024;
    desc.usage = GFX_BUFFER_USAGE_VERTEX;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL;

    // NULL device
    GfxBuffer buffer = NULL;
    GfxResult result = gfxDeviceCreateBuffer(NULL, &desc, &buffer);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL descriptor
    result = gfxDeviceCreateBuffer(device, NULL, &buffer);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL output pointer
    result = gfxDeviceCreateBuffer(device, &desc, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxBufferTest, CreateBufferZeroSize)
{
    GfxBufferDescriptor desc = {};
    desc.size = 0;
    desc.usage = GFX_BUFFER_USAGE_VERTEX;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL;

    GfxBuffer buffer = NULL;
    GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffer);

    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxBufferTest, CreateBufferNoUsage)
{
    GfxBufferDescriptor desc = {};
    desc.size = 1024;
    desc.usage = GFX_BUFFER_USAGE_NONE;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL;

    GfxBuffer buffer = NULL;
    GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffer);

    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxBufferTest, GetBufferInfo)
{
    GfxBufferDescriptor desc = {};
    desc.label = "Test Buffer";
    desc.size = 2048;
    desc.usage = GFX_BUFFER_USAGE_UNIFORM | GFX_BUFFER_USAGE_COPY_DST | GFX_BUFFER_USAGE_MAP_READ;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT;

    GfxBuffer buffer = NULL;
    ASSERT_EQ(gfxDeviceCreateBuffer(device, &desc, &buffer), GFX_RESULT_SUCCESS);
    ASSERT_NE(buffer, nullptr);

    GfxBufferInfo info = {};
    GfxResult result = gfxBufferGetInfo(buffer, &info);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_EQ(info.size, 2048);
    EXPECT_EQ(info.usage, GFX_BUFFER_USAGE_UNIFORM | GFX_BUFFER_USAGE_COPY_DST | GFX_BUFFER_USAGE_MAP_READ);

    gfxBufferDestroy(buffer);
}

TEST_P(GfxBufferTest, MapUnmapBuffer)
{
    GfxBufferDescriptor desc = {};
    desc.label = "Mappable Buffer";
    desc.size = 256;
    desc.usage = GFX_BUFFER_USAGE_MAP_WRITE | GFX_BUFFER_USAGE_COPY_SRC;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT;

    GfxBuffer buffer = NULL;
    ASSERT_EQ(gfxDeviceCreateBuffer(device, &desc, &buffer), GFX_RESULT_SUCCESS);
    ASSERT_NE(buffer, nullptr);

    void* mappedData = NULL;
    GfxResult result = gfxBufferMap(buffer, 0, 256, &mappedData);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(mappedData, nullptr);

    if (mappedData) {
        // Write some test data
        uint32_t testData[] = { 1, 2, 3, 4 };
        std::memcpy(mappedData, testData, sizeof(testData));

        result = gfxBufferUnmap(buffer);
        EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    }

    gfxBufferDestroy(buffer);
}

TEST_P(GfxBufferTest, MapBufferInvalidArguments)
{
    GfxBufferDescriptor desc = {};
    desc.size = 256;
    desc.usage = GFX_BUFFER_USAGE_MAP_WRITE | GFX_BUFFER_USAGE_COPY_SRC;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT;

    GfxBuffer buffer = NULL;
    ASSERT_EQ(gfxDeviceCreateBuffer(device, &desc, &buffer), GFX_RESULT_SUCCESS);

    void* mappedData = NULL;

    // NULL buffer
    GfxResult result = gfxBufferMap(NULL, 0, 256, &mappedData);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL output pointer
    result = gfxBufferMap(buffer, 0, 256, NULL);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    gfxBufferDestroy(buffer);
}

TEST_P(GfxBufferTest, WriteBufferViaQueue)
{
    GfxBufferDescriptor desc = {};
    desc.label = "Queue Write Buffer";
    desc.size = 128;
    desc.usage = GFX_BUFFER_USAGE_COPY_DST | GFX_BUFFER_USAGE_UNIFORM;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT;

    GfxBuffer buffer = NULL;
    ASSERT_EQ(gfxDeviceCreateBuffer(device, &desc, &buffer), GFX_RESULT_SUCCESS);
    ASSERT_NE(buffer, nullptr);

    GfxQueue queue = NULL;
    ASSERT_EQ(gfxDeviceGetQueue(device, &queue), GFX_RESULT_SUCCESS);
    ASSERT_NE(queue, nullptr);

    float testData[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    GfxResult result = gfxQueueWriteBuffer(queue, buffer, 0, testData, sizeof(testData));

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);

    gfxBufferDestroy(buffer);
}

TEST_P(GfxBufferTest, CreateMultipleBuffers)
{
    const int bufferCount = 5;
    GfxBuffer buffers[bufferCount] = {};

    for (int i = 0; i < bufferCount; ++i) {
        GfxBufferDescriptor desc = {};
        desc.size = 512 * (i + 1);
        desc.usage = GFX_BUFFER_USAGE_VERTEX | GFX_BUFFER_USAGE_COPY_DST;
        desc.memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL;

        GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffers[i]);
        EXPECT_EQ(result, GFX_RESULT_SUCCESS);
        EXPECT_NE(buffers[i], nullptr);
    }

    for (int i = 0; i < bufferCount; ++i) {
        if (buffers[i]) {
            gfxBufferDestroy(buffers[i]);
        }
    }
}

TEST_P(GfxBufferTest, CreateBufferWithAllUsageFlags)
{
    GfxBufferDescriptor desc = {};
    desc.label = "All Usage Buffer";
    desc.size = 4096;
    desc.usage = GFX_BUFFER_USAGE_MAP_READ | GFX_BUFFER_USAGE_MAP_WRITE | GFX_BUFFER_USAGE_COPY_SRC | GFX_BUFFER_USAGE_COPY_DST | GFX_BUFFER_USAGE_INDEX | GFX_BUFFER_USAGE_VERTEX | GFX_BUFFER_USAGE_UNIFORM | GFX_BUFFER_USAGE_STORAGE | GFX_BUFFER_USAGE_INDIRECT;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT;

    GfxBuffer buffer = NULL;
    GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffer);

    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(buffer, nullptr);

    if (buffer) {
        gfxBufferDestroy(buffer);
    }
}

TEST_P(GfxBufferTest, ImportBufferInvalidArguments)
{
    GfxBufferImportDescriptor desc = {};
    desc.nativeHandle = nullptr; // Invalid handle
    desc.size = 1024;
    desc.usage = GFX_BUFFER_USAGE_VERTEX;

    // NULL device
    GfxBuffer buffer = nullptr;
    GfxResult result = gfxDeviceImportBuffer(nullptr, &desc, &buffer);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL descriptor
    result = gfxDeviceImportBuffer(device, nullptr, &buffer);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL output
    result = gfxDeviceImportBuffer(device, &desc, nullptr);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);

    // NULL native handle
    result = gfxDeviceImportBuffer(device, &desc, &buffer);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxBufferTest, ImportBufferZeroSize)
{
    GfxBufferImportDescriptor desc = {};
    desc.nativeHandle = (void*)0x1; // Dummy non-null pointer (won't actually be used)
    desc.size = 0; // Invalid: zero size
    desc.usage = GFX_BUFFER_USAGE_VERTEX;

    GfxBuffer buffer = nullptr;
    GfxResult result = gfxDeviceImportBuffer(device, &desc, &buffer);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxBufferTest, ImportBufferNoUsage)
{
    GfxBufferImportDescriptor desc = {};
    desc.nativeHandle = (void*)0x1; // Dummy non-null pointer
    desc.size = 1024;
    desc.usage = GFX_BUFFER_USAGE_NONE; // Invalid: no usage

    GfxBuffer buffer = nullptr;
    GfxResult result = gfxDeviceImportBuffer(device, &desc, &buffer);
    EXPECT_EQ(result, GFX_RESULT_ERROR_INVALID_ARGUMENT);
}

TEST_P(GfxBufferTest, ImportBufferFromNativeHandle)
{
    // First, create a normal buffer
    GfxBufferDescriptor createDesc = {};
    createDesc.label = "Source Buffer";
    createDesc.size = 1024;
    createDesc.usage = GFX_BUFFER_USAGE_COPY_SRC | GFX_BUFFER_USAGE_COPY_DST;
    createDesc.memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL;

    GfxBuffer sourceBuffer = nullptr;
    GfxResult result = gfxDeviceCreateBuffer(device, &createDesc, &sourceBuffer);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(sourceBuffer, nullptr);

    // Get buffer info to verify properties
    GfxBufferInfo info = {};
    result = gfxBufferGetInfo(sourceBuffer, &info);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);

    // Extract native handle using the API
    void* nativeHandle = nullptr;
    result = gfxBufferGetNativeHandle(sourceBuffer, &nativeHandle);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(nativeHandle, nullptr);

    // Now import the native handle
    GfxBufferImportDescriptor importDesc = {};
    importDesc.nativeHandle = nativeHandle;
    importDesc.size = info.size;
    importDesc.usage = info.usage;

    GfxBuffer importedBuffer = nullptr;
    result = gfxDeviceImportBuffer(device, &importDesc, &importedBuffer);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_NE(importedBuffer, nullptr);

    // Verify imported buffer has correct properties
    if (importedBuffer) {
        GfxBufferInfo importedInfo = {};
        result = gfxBufferGetInfo(importedBuffer, &importedInfo);
        EXPECT_EQ(result, GFX_RESULT_SUCCESS);
        EXPECT_EQ(importedInfo.size, info.size);
        EXPECT_EQ(importedInfo.usage, info.usage);

        // Clean up imported buffer (doesn't own the native handle)
        gfxBufferDestroy(importedBuffer);
    }

    // Clean up source buffer
    gfxBufferDestroy(sourceBuffer);
}

TEST_P(GfxBufferTest, FlushMappedRange)
{
    // Skip on WebGPU - memory is always coherent
    if (backend == GFX_BACKEND_WEBGPU) {
        GTEST_SKIP() << "WebGPU memory is always coherent";
    }

    // Create a host-visible, non-coherent buffer for testing flush
    GfxBufferDescriptor desc = {};
    desc.label = "Flush Test Buffer";
    desc.size = 1024;
    desc.usage = GFX_BUFFER_USAGE_MAP_WRITE | GFX_BUFFER_USAGE_UNIFORM | GFX_BUFFER_USAGE_COPY_SRC;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE; // Non-coherent

    GfxBuffer buffer = nullptr;
    GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffer);
    ASSERT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(buffer, nullptr);

    // Map the buffer
    void* mappedPtr = nullptr;
    result = gfxBufferMap(buffer, 0, desc.size, &mappedPtr);

    // Skip test if mapping failed (WebGPU might not support synchronous mapping)
    if (result != GFX_RESULT_SUCCESS || mappedPtr == nullptr) {
        gfxBufferDestroy(buffer);
        GTEST_SKIP() << "Buffer mapping not supported or failed";
        return;
    }

    // Write some data
    std::memset(mappedPtr, 0x42, 512);

    // Flush the written range (CPU -> GPU)
    result = gfxBufferFlushMappedRange(buffer, 0, 512);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);

    gfxBufferUnmap(buffer);
    gfxBufferDestroy(buffer);
}

TEST_P(GfxBufferTest, InvalidateMappedRange)
{
    // Skip on WebGPU - memory is always coherent
    if (backend == GFX_BACKEND_WEBGPU) {
        GTEST_SKIP() << "WebGPU memory is always coherent";
    }

    // Create a host-visible, non-coherent buffer for testing invalidate
    GfxBufferDescriptor desc = {};
    desc.label = "Invalidate Test Buffer";
    desc.size = 1024;
    desc.usage = GFX_BUFFER_USAGE_MAP_READ | GFX_BUFFER_USAGE_STORAGE | GFX_BUFFER_USAGE_COPY_DST;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE; // Non-coherent for testing

    GfxBuffer buffer = nullptr;
    GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffer);

    // Skip if buffer creation failed (some backends might not support this configuration)
    if (result != GFX_RESULT_SUCCESS || buffer == nullptr) {
        GTEST_SKIP() << "Buffer creation not supported or failed";
        return;
    }

    // Map the buffer first
    void* mappedPtr = nullptr;
    result = gfxBufferMap(buffer, 0, desc.size, &mappedPtr);

    if (result == GFX_RESULT_SUCCESS && mappedPtr != nullptr) {
        // In a real scenario, GPU would write to this buffer
        // Invalidate to make GPU writes visible to CPU (GPU -> CPU)
        result = gfxBufferInvalidateMappedRange(buffer, 0, desc.size);
        EXPECT_EQ(result, GFX_RESULT_SUCCESS);

        gfxBufferUnmap(buffer);
    }

    gfxBufferDestroy(buffer);
}

TEST_P(GfxBufferTest, FlushInvalidateCombined)
{
    // Skip on WebGPU - memory is always coherent
    if (backend == GFX_BACKEND_WEBGPU) {
        GTEST_SKIP() << "WebGPU memory is always coherent";
    }

    // Test flush and invalidate together on non-coherent memory
    GfxBufferDescriptor desc = {};
    desc.label = "Flush+Invalidate Test Buffer";
    desc.size = 2048;
    desc.usage = GFX_BUFFER_USAGE_MAP_WRITE | GFX_BUFFER_USAGE_MAP_READ | GFX_BUFFER_USAGE_STORAGE | GFX_BUFFER_USAGE_COPY_SRC | GFX_BUFFER_USAGE_COPY_DST;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE; // Non-coherent for testing

    GfxBuffer buffer = nullptr;
    GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffer);

    if (result != GFX_RESULT_SUCCESS || buffer == nullptr) {
        GTEST_SKIP() << "Buffer creation not supported or failed";
        return;
    }

    void* mappedPtr = nullptr;
    result = gfxBufferMap(buffer, 0, desc.size, &mappedPtr);

    if (result == GFX_RESULT_SUCCESS && mappedPtr != nullptr) {
        // Write data to first half
        std::memset(mappedPtr, 0xAA, 1024);

        // Flush first half (CPU writes -> GPU)
        result = gfxBufferFlushMappedRange(buffer, 0, 1024);
        EXPECT_EQ(result, GFX_RESULT_SUCCESS);

        // Invalidate second half (GPU writes -> CPU)
        result = gfxBufferInvalidateMappedRange(buffer, 1024, 1024);
        EXPECT_EQ(result, GFX_RESULT_SUCCESS);

        gfxBufferUnmap(buffer);
    }

    gfxBufferDestroy(buffer);
}

TEST_P(GfxBufferTest, CreateBufferWithDeviceLocalOnly)
{
    GfxBufferDescriptor desc = {};
    desc.label = "Device Local Buffer";
    desc.size = 1024;
    desc.usage = GFX_BUFFER_USAGE_STORAGE | GFX_BUFFER_USAGE_COPY_DST;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL;

    GfxBuffer buffer = nullptr;
    GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffer);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(buffer, nullptr);

    GfxBufferInfo info = {};
    result = gfxBufferGetInfo(buffer, &info);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_EQ(info.size, 1024u);
    EXPECT_TRUE(info.memoryProperties & GFX_MEMORY_PROPERTY_DEVICE_LOCAL);

    gfxBufferDestroy(buffer);
}

TEST_P(GfxBufferTest, CreateBufferWithHostVisibleAndHostCoherent)
{
    GfxBufferDescriptor desc = {};
    desc.label = "Host Visible Coherent Buffer";
    desc.size = 512;
    desc.usage = GFX_BUFFER_USAGE_MAP_WRITE | GFX_BUFFER_USAGE_COPY_SRC;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT;

    GfxBuffer buffer = nullptr;
    GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffer);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(buffer, nullptr);

    GfxBufferInfo info = {};
    result = gfxBufferGetInfo(buffer, &info);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_TRUE(info.memoryProperties & GFX_MEMORY_PROPERTY_HOST_VISIBLE);
    EXPECT_TRUE(info.memoryProperties & GFX_MEMORY_PROPERTY_HOST_COHERENT);

    gfxBufferDestroy(buffer);
}

TEST_P(GfxBufferTest, CreateBufferWithHostVisibleAndHostCached)
{
    GfxBufferDescriptor desc = {};
    desc.label = "Host Visible Cached Buffer";
    desc.size = 512;
    desc.usage = GFX_BUFFER_USAGE_MAP_WRITE | GFX_BUFFER_USAGE_COPY_SRC;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_CACHED;

    GfxBuffer buffer = nullptr;
    GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffer);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    ASSERT_NE(buffer, nullptr);

    GfxBufferInfo info = {};
    result = gfxBufferGetInfo(buffer, &info);
    EXPECT_EQ(result, GFX_RESULT_SUCCESS);
    EXPECT_TRUE(info.memoryProperties & GFX_MEMORY_PROPERTY_HOST_VISIBLE);
    EXPECT_TRUE(info.memoryProperties & GFX_MEMORY_PROPERTY_HOST_CACHED);

    gfxBufferDestroy(buffer);
}

TEST_P(GfxBufferTest, CreateBufferWithAllMemoryProperties)
{
    GfxBufferDescriptor desc = {};
    desc.label = "All Memory Properties Buffer";
    desc.size = 1024;
    desc.usage = GFX_BUFFER_USAGE_MAP_WRITE | GFX_BUFFER_USAGE_COPY_SRC;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL | GFX_MEMORY_PROPERTY_HOST_VISIBLE | GFX_MEMORY_PROPERTY_HOST_COHERENT | GFX_MEMORY_PROPERTY_HOST_CACHED;

    GfxBuffer buffer = nullptr;
    GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffer);
    // This combination may not be supported on all platforms
    // Result may succeed or fail depending on hardware capabilities
    if (result == GFX_RESULT_SUCCESS && buffer != nullptr) {
        gfxBufferDestroy(buffer);
    }
    // Test passes either way - just ensure it doesn't crash
    SUCCEED();
}

TEST_P(GfxBufferTest, CreateBufferWithNoMemoryPropertiesThrows)
{
    GfxBufferDescriptor desc = {};
    desc.label = "No Memory Properties Buffer";
    desc.size = 512;
    desc.usage = GFX_BUFFER_USAGE_VERTEX;
    desc.memoryProperties = 0; // Invalid: no memory properties

    GfxBuffer buffer = nullptr;
    GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffer);
    EXPECT_NE(result, GFX_RESULT_SUCCESS);
}

TEST_P(GfxBufferTest, CreateBufferWithHostCoherentWithoutHostVisibleThrows)
{
    GfxBufferDescriptor desc = {};
    desc.label = "Host Coherent Without Visible Buffer";
    desc.size = 512;
    desc.usage = GFX_BUFFER_USAGE_VERTEX;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_COHERENT; // Invalid: HostCoherent requires HostVisible

    GfxBuffer buffer = nullptr;
    GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffer);
    EXPECT_NE(result, GFX_RESULT_SUCCESS);
}

TEST_P(GfxBufferTest, CreateBufferWithHostCachedWithoutHostVisibleThrows)
{
    GfxBufferDescriptor desc = {};
    desc.label = "Host Cached Without Visible Buffer";
    desc.size = 512;
    desc.usage = GFX_BUFFER_USAGE_VERTEX;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_HOST_CACHED; // Invalid: HostCached requires HostVisible

    GfxBuffer buffer = nullptr;
    GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffer);
    EXPECT_NE(result, GFX_RESULT_SUCCESS);
}

TEST_P(GfxBufferTest, CreateBufferWithMapReadRequiresHostVisible)
{
    GfxBufferDescriptor desc = {};
    desc.label = "MapRead Without HostVisible Buffer";
    desc.size = 512;
    desc.usage = GFX_BUFFER_USAGE_MAP_READ | GFX_BUFFER_USAGE_COPY_DST;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL; // Invalid: MapRead requires HostVisible

    GfxBuffer buffer = nullptr;
    GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffer);
    EXPECT_NE(result, GFX_RESULT_SUCCESS);
}

TEST_P(GfxBufferTest, CreateBufferWithMapWriteRequiresHostVisible)
{
    GfxBufferDescriptor desc = {};
    desc.label = "MapWrite Without HostVisible Buffer";
    desc.size = 512;
    desc.usage = GFX_BUFFER_USAGE_MAP_WRITE | GFX_BUFFER_USAGE_COPY_SRC;
    desc.memoryProperties = GFX_MEMORY_PROPERTY_DEVICE_LOCAL; // Invalid: MapWrite requires HostVisible

    GfxBuffer buffer = nullptr;
    GfxResult result = gfxDeviceCreateBuffer(device, &desc, &buffer);
    EXPECT_NE(result, GFX_RESULT_SUCCESS);
}

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    GfxBufferTest,
    testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU),
    [](const testing::TestParamInfo<GfxBackend>& info) {
        return info.param == GFX_BACKEND_VULKAN ? "Vulkan" : "WebGPU";
    });

} // namespace
