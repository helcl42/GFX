#include <core/system/Device.h>
#include <core/system/Queue.h>

#include <gfx/gfx.h>

#include <gfx_cpp/gfx.hpp>

#include <gtest/gtest.h>

namespace gfx {

class QueueImplTest : public ::testing::TestWithParam<GfxBackend> {
protected:
    void SetUp() override
    {
        backend = GetParam();

        ASSERT_EQ(gfxLoadBackend(backend), GFX_RESULT_SUCCESS);

        GfxInstanceDescriptor instanceDesc{
            .backend = backend,
            .applicationName = "QueueImplTest"
        };
        ASSERT_EQ(gfxCreateInstance(&instanceDesc, &instance), GFX_RESULT_SUCCESS);

        GfxAdapterDescriptor adapterDesc = GFX_ADAPTER_DESCRIPTOR_INIT;
        ASSERT_EQ(gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter), GFX_RESULT_SUCCESS);

        GfxDeviceDescriptor deviceDesc = GFX_DEVICE_DESCRIPTOR_INIT;
        ASSERT_EQ(gfxAdapterCreateDevice(adapter, &deviceDesc, &device), GFX_RESULT_SUCCESS);

        ASSERT_EQ(gfxDeviceGetQueue(device, &queue), GFX_RESULT_SUCCESS);
        ASSERT_NE(queue, nullptr);
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
    GfxQueue queue = nullptr;
};

TEST_P(QueueImplTest, CreateWrapper)
{
    QueueImpl wrapper(queue);
    // Wrapper created successfully
}

TEST_P(QueueImplTest, WaitIdle)
{
    QueueImpl wrapper(queue);

    // Should not crash
    wrapper.waitIdle();
}

TEST_P(QueueImplTest, WriteBuffer)
{
    DeviceImpl deviceWrapper(device);
    QueueImpl queueWrapper(queue);

    // Create buffer
    BufferDescriptor bufferDesc = {};
    bufferDesc.size = 256;
    bufferDesc.usage = BufferUsage::CopyDst;
    auto buffer = deviceWrapper.createBuffer(bufferDesc);

    // Write data
    uint32_t data[64] = { 0 };
    for (int i = 0; i < 64; i++) {
        data[i] = i;
    }

    // Should not crash
    queueWrapper.writeBuffer(buffer, 0, data, sizeof(data));
}

TEST_P(QueueImplTest, WriteTexture)
{
    DeviceImpl deviceWrapper(device);
    QueueImpl queueWrapper(queue);

    // Create texture
    TextureDescriptor texDesc = {};
    texDesc.size = { 16, 16, 1 };
    texDesc.mipLevelCount = 1;
    texDesc.arrayLayerCount = 1;
    texDesc.format = TextureFormat::R8G8B8A8Unorm;
    texDesc.usage = TextureUsage::CopyDst;
    texDesc.type = TextureType::Texture2D;
    auto texture = deviceWrapper.createTexture(texDesc);

    // Write data
    std::vector<uint8_t> data(16 * 16 * 4, 255);

    Origin3D origin = { 0, 0, 0 };
    Extent3D extent = { 16, 16, 1 };

    // Should not crash
    queueWrapper.writeTexture(texture, origin, 0, data.data(), data.size(), extent, TextureLayout::Undefined);
}

TEST_P(QueueImplTest, Submit)
{
    DeviceImpl deviceWrapper(device);
    QueueImpl queueWrapper(queue);

    // Create command encoder and finish
    CommandEncoderDescriptor encDesc = {};
    auto encoder = deviceWrapper.createCommandEncoder(encDesc);

    // Submit empty command buffer
    SubmitDescriptor submitDesc = {};
    // Should not crash
    queueWrapper.submit(submitDesc);
}

// Instantiate tests for available backends
#if defined(GFX_ENABLE_VULKAN) && defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(AllBackends, QueueImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN, GFX_BACKEND_WEBGPU));
#elif defined(GFX_ENABLE_VULKAN)
INSTANTIATE_TEST_SUITE_P(VulkanOnly, QueueImplTest,
    ::testing::Values(GFX_BACKEND_VULKAN));
#elif defined(GFX_ENABLE_WEBGPU)
INSTANTIATE_TEST_SUITE_P(WebGPUOnly, QueueImplTest,
    ::testing::Values(GFX_BACKEND_WEBGPU));
#endif

} // namespace gfx
