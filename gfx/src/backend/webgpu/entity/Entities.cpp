#include "Entities.h"
#include "../converter/GfxWebGPUConverter.h"

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

void Queue::writeBuffer(Buffer* buffer, uint64_t offset, const void* data, uint64_t size)
{
    wgpuQueueWriteBuffer(m_queue, buffer->handle(), offset, data, size);
}

void Queue::writeTexture(Texture* texture, uint32_t mipLevel,
    uint32_t originX, uint32_t originY, uint32_t originZ,
    const void* data, uint64_t dataSize,
    uint32_t bytesPerRow,
    uint32_t width, uint32_t height, uint32_t depth)
{
    WGPUTexelCopyTextureInfo dest = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    dest.texture = texture->handle();
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

RenderPassEncoder::RenderPassEncoder(CommandEncoder* commandEncoder, const RenderPassEncoderCreateInfo& createInfo)
{
    WGPURenderPassDescriptor wgpuDesc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;

    // Convert color attachments
    std::vector<WGPURenderPassColorAttachment> wgpuColorAttachments;
    for (const ColorAttachment& colorAttachment : createInfo.colorAttachments) {
        WGPURenderPassColorAttachment attachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
        attachment.view = colorAttachment.target.view;
        attachment.loadOp = colorAttachment.target.ops.loadOp;
        attachment.storeOp = colorAttachment.target.ops.storeOp;
        attachment.clearValue = colorAttachment.target.ops.clearColor;

        // Handle resolve target if present
        if (colorAttachment.resolveTarget.has_value()) {
            attachment.resolveTarget = colorAttachment.resolveTarget->view;
        }
        wgpuColorAttachments.push_back(attachment);
    }

    if (!wgpuColorAttachments.empty()) {
        wgpuDesc.colorAttachments = wgpuColorAttachments.data();
        wgpuDesc.colorAttachmentCount = static_cast<uint32_t>(wgpuColorAttachments.size());
    }

    // Convert depth/stencil attachment
    WGPURenderPassDepthStencilAttachment wgpuDepthStencil = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
    if (createInfo.depthStencilAttachment.has_value()) {
        const DepthStencilAttachmentTarget& target = createInfo.depthStencilAttachment->target;
        wgpuDepthStencil.view = target.view;

        // Handle depth operations
        if (target.depthOps.has_value()) {
            wgpuDepthStencil.depthLoadOp = target.depthOps->loadOp;
            wgpuDepthStencil.depthStoreOp = target.depthOps->storeOp;
            wgpuDepthStencil.depthClearValue = target.depthOps->clearValue;
        } else {
            wgpuDepthStencil.depthLoadOp = WGPULoadOp_Undefined;
            wgpuDepthStencil.depthStoreOp = WGPUStoreOp_Undefined;
            wgpuDepthStencil.depthClearValue = 1.0f;
        }

        // Only set stencil operations for formats that have a stencil aspect
        GfxTextureFormat format = converter::wgpuFormatToGfxFormat(target.format);
        if (converter::formatHasStencil(format)) {
            if (target.stencilOps.has_value()) {
                wgpuDepthStencil.stencilLoadOp = target.stencilOps->loadOp;
                wgpuDepthStencil.stencilStoreOp = target.stencilOps->storeOp;
                wgpuDepthStencil.stencilClearValue = target.stencilOps->clearValue;
            } else {
                wgpuDepthStencil.stencilLoadOp = WGPULoadOp_Undefined;
                wgpuDepthStencil.stencilStoreOp = WGPUStoreOp_Undefined;
                wgpuDepthStencil.stencilClearValue = 0;
            }
        } else {
            wgpuDepthStencil.stencilLoadOp = WGPULoadOp_Undefined;
            wgpuDepthStencil.stencilStoreOp = WGPUStoreOp_Undefined;
            wgpuDepthStencil.stencilClearValue = 0;
        }

        wgpuDesc.depthStencilAttachment = &wgpuDepthStencil;
    }

    m_encoder = wgpuCommandEncoderBeginRenderPass(commandEncoder->handle(), &wgpuDesc);
    if (!m_encoder) {
        throw std::runtime_error("Failed to create WebGPU render pass encoder");
    }
}

ComputePassEncoder::ComputePassEncoder(CommandEncoder* commandEncoder, const ComputePassEncoderCreateInfo& createInfo)
{
    WGPUComputePassDescriptor wgpuDesc = WGPU_COMPUTE_PASS_DESCRIPTOR_INIT;
    if (createInfo.label) {
        wgpuDesc.label = converter::gfxStringView(createInfo.label);
    }

    m_encoder = wgpuCommandEncoderBeginComputePass(commandEncoder->handle(), &wgpuDesc);
    if (!m_encoder) {
        throw std::runtime_error("Failed to create compute pass encoder");
    }
}

} // namespace gfx::webgpu