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
} // namespace gfx::webgpu