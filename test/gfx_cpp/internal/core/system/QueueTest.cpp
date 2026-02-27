#include "../../common/CommonTest.h"

#include <core/system/Device.h>
#include <core/system/Queue.h>

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

        GfxAdapterDescriptor adapterDesc = {};
        adapterDesc.sType = GFX_STRUCTURE_TYPE_ADAPTER_DESCRIPTOR;
        adapterDesc.pNext = nullptr;
        ASSERT_EQ(gfxInstanceRequestAdapter(instance, &adapterDesc, &adapter), GFX_RESULT_SUCCESS);

        GfxDeviceDescriptor deviceDesc = {};
        deviceDesc.sType = GFX_STRUCTURE_TYPE_DEVICE_DESCRIPTOR;
        deviceDesc.pNext = nullptr;
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
    texDesc.format = Format::R8G8B8A8Unorm;
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

// ===========================================================================
// Test Instantiation
// ===========================================================================

INSTANTIATE_TEST_SUITE_P(
    AllBackends,
    QueueImplTest,
    testing::ValuesIn(getActiveBackends()),
    convertTestParamToString);

} // namespace gfx
