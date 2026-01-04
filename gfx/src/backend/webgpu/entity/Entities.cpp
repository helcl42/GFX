#include "Entities.h"

namespace gfx::webgpu {
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
            auto* fence = static_cast<gfx::webgpu::Fence*>(userdata1);
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
        if (m_device && m_device->getAdapter() && m_device->getAdapter()->getInstance()) {
            WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
            waitInfo.future = future;
            wgpuInstanceWaitAny(m_device->getAdapter()->getInstance()->handle(), 1, &waitInfo, UINT64_MAX);
        }
    }

    return true;
}

void Queue::writeBuffer(WGPUBuffer buffer, uint64_t offset, const void* data, uint64_t size)
{
    wgpuQueueWriteBuffer(m_queue, buffer, offset, data, size);
}

void Queue::writeTexture(WGPUTexture texture, uint32_t mipLevel,
    uint32_t originX, uint32_t originY, uint32_t originZ,
    const void* data, uint64_t dataSize,
    uint32_t bytesPerRow,
    uint32_t width, uint32_t height, uint32_t depth)
{
    WGPUTexelCopyTextureInfo dest = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    dest.texture = texture;
    dest.mipLevel = mipLevel;
    dest.origin = { originX, originY, originZ };

    WGPUTexelCopyBufferLayout layout = WGPU_TEXEL_COPY_BUFFER_LAYOUT_INIT;
    layout.bytesPerRow = bytesPerRow;

    WGPUExtent3D extent = { width, height, depth };

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
    if (m_device && m_device->getAdapter() && m_device->getAdapter()->getInstance()) {
        WGPUFutureWaitInfo waitInfo = WGPU_FUTURE_WAIT_INFO_INIT;
        waitInfo.future = future;
        wgpuInstanceWaitAny(m_device->getAdapter()->getInstance()->handle(), 1, &waitInfo, UINT64_MAX);
    }

    return workDone;
}
} // namespace gfx::webgpu