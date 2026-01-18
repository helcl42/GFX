#include "Queue.h"

#include "../command/CommandEncoder.h"
#include "../resource/Buffer.h"
#include "../resource/Texture.h"
#include "../sync/Fence.h"
#include "../system/Adapter.h"
#include "../system/Device.h"
#include "../system/Instance.h"

namespace gfx::backend::webgpu::core {

Queue::Queue(WGPUQueue queue, Device* device)
    : m_queue(queue)
    , m_device(device)
{
    // Don't add ref - emdawnwebgpu doesn't provide wgpuQueueAddRef
    // The queue is owned by the device and automatically destroyed with it
}

Queue::~Queue()
{
    if (m_queue) {
        wgpuQueueRelease(m_queue);
    }
}

WGPUQueue Queue::handle() const
{
    return m_queue;
}

Device* Queue::getDevice() const
{
    return m_device;
}

bool Queue::submit(const SubmitInfo& submitInfo)
{
    // WebGPU doesn't support semaphore-based sync - just submit command buffers
    for (uint32_t i = 0; i < submitInfo.commandEncoderCount; ++i) {
        if (submitInfo.commandEncoders[i]) {
            auto* encoderPtr = submitInfo.commandEncoders[i];

            WGPUCommandBufferDescriptor cmdDesc = WGPU_COMMAND_BUFFER_DESCRIPTOR_INIT;
            WGPUCommandBuffer cmdBuffer = wgpuCommandEncoderFinish(encoderPtr->handle(), &cmdDesc);

            if (cmdBuffer) {
                wgpuQueueSubmit(m_queue, 1, &cmdBuffer);
                wgpuCommandBufferRelease(cmdBuffer);

                // Mark encoder as finished so it will be recreated on next Begin()
                encoderPtr->markFinished();
            } else {
                return false;
            }
        }
    }

    // Signal fence if provided - use queue work done to wait for GPU completion
    if (submitInfo.signalFence) {
        static auto fenceSignalCallback = [](WGPUQueueWorkDoneStatus status, WGPUStringView, void* userdata1, void*) {
            auto* fence = static_cast<Fence*>(userdata1);
            if (status == WGPUQueueWorkDoneStatus_Success) {
                fence->setSignaled(true);
            }
        };

        WGPUQueueWorkDoneCallbackInfo callbackInfo = WGPU_QUEUE_WORK_DONE_CALLBACK_INFO_INIT;
        callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
        callbackInfo.callback = fenceSignalCallback;
        callbackInfo.userdata1 = submitInfo.signalFence;

        WGPUFuture future = wgpuQueueOnSubmittedWorkDone(m_queue, callbackInfo);

        // Wait for the fence to be signaled (GPU work done)
        WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
        waitInfo.future = future;
        wgpuInstanceWaitAny(m_device->getAdapter()->getInstance()->handle(), 1, &waitInfo, UINT64_MAX);
    }

    return true;
}

void Queue::writeBuffer(Buffer* buffer, uint64_t offset, const void* data, uint64_t size)
{
    wgpuQueueWriteBuffer(m_queue, buffer->handle(), offset, data, size);
}

void Queue::writeTexture(Texture* texture, uint32_t mipLevel, const WGPUOrigin3D& origin, const void* data, uint64_t dataSize, uint32_t bytesPerRow, const WGPUExtent3D& extent)
{
    WGPUTexelCopyTextureInfo dest = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    dest.texture = texture->handle();
    dest.mipLevel = mipLevel;
    dest.origin = origin;

    WGPUTexelCopyBufferLayout layout = WGPU_TEXEL_COPY_BUFFER_LAYOUT_INIT;
    layout.bytesPerRow = bytesPerRow;

    wgpuQueueWriteTexture(m_queue, &dest, data, dataSize, &layout, &extent);
}

bool Queue::waitIdle()
{
    // Submit empty command to ensure all previous work is queued
    static auto queueWorkDoneCallback = [](WGPUQueueWorkDoneStatus status, WGPUStringView, void* userdata1, void*) {
        bool* done = static_cast<bool*>(userdata1);
        if (status == WGPUQueueWorkDoneStatus_Success) {
            *done = true;
        }
    };

    bool workDone = false;
    WGPUQueueWorkDoneCallbackInfo callbackInfo = WGPU_QUEUE_WORK_DONE_CALLBACK_INFO_INIT;
    callbackInfo.mode = WGPUCallbackMode_WaitAnyOnly;
    callbackInfo.callback = queueWorkDoneCallback;
    callbackInfo.userdata1 = &workDone;

    WGPUFuture future = wgpuQueueOnSubmittedWorkDone(m_queue, callbackInfo);

    // Properly wait for the queue work to complete
    WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
    waitInfo.future = future;
    wgpuInstanceWaitAny(m_device->getAdapter()->getInstance()->handle(), 1, &waitInfo, UINT64_MAX);

    return workDone;
}

} // namespace gfx::backend::webgpu::core