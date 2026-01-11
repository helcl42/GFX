#include "Entities.h"

namespace gfx::webgpu {

namespace {
    bool hasStencil(WGPUTextureFormat format)
    {
        return format == WGPUTextureFormat_Stencil8 ||
               format == WGPUTextureFormat_Depth24PlusStencil8 ||
               format == WGPUTextureFormat_Depth32FloatStencil8;
    }
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

void Queue::writeTexture(Texture* texture, uint32_t mipLevel,
    const WGPUOrigin3D& origin, const void* data, uint64_t dataSize,
    uint32_t bytesPerRow, const WGPUExtent3D& extent)
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

// CommandEncoder implementations
void CommandEncoder::copyBufferToBuffer(Buffer* source, uint64_t sourceOffset,
    Buffer* destination, uint64_t destinationOffset, uint64_t size)
{
    wgpuCommandEncoderCopyBufferToBuffer(m_encoder,
        source->handle(), sourceOffset,
        destination->handle(), destinationOffset,
        size);
}

void CommandEncoder::copyBufferToTexture(Buffer* source, uint64_t sourceOffset, uint32_t bytesPerRow,
    Texture* destination, const WGPUOrigin3D& origin,
    const WGPUExtent3D& extent, uint32_t mipLevel)
{
    WGPUTexelCopyBufferInfo sourceInfo = WGPU_TEXEL_COPY_BUFFER_INFO_INIT;
    sourceInfo.buffer = source->handle();
    sourceInfo.layout.offset = sourceOffset;
    sourceInfo.layout.bytesPerRow = bytesPerRow;

    WGPUTexelCopyTextureInfo destInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    destInfo.texture = destination->handle();
    destInfo.mipLevel = mipLevel;
    destInfo.origin = origin;

    wgpuCommandEncoderCopyBufferToTexture(m_encoder, &sourceInfo, &destInfo, &extent);
}

void CommandEncoder::copyTextureToBuffer(Texture* source, const WGPUOrigin3D& origin,
    uint32_t mipLevel, Buffer* destination, uint64_t destinationOffset, uint32_t bytesPerRow,
    const WGPUExtent3D& extent)
{
    WGPUTexelCopyTextureInfo sourceInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    sourceInfo.texture = source->handle();
    sourceInfo.mipLevel = mipLevel;
    sourceInfo.origin = origin;

    WGPUTexelCopyBufferInfo destInfo = WGPU_TEXEL_COPY_BUFFER_INFO_INIT;
    destInfo.buffer = destination->handle();
    destInfo.layout.offset = destinationOffset;
    destInfo.layout.bytesPerRow = bytesPerRow;

    wgpuCommandEncoderCopyTextureToBuffer(m_encoder, &sourceInfo, &destInfo, &extent);
}

void CommandEncoder::copyTextureToTexture(Texture* source, const WGPUOrigin3D& sourceOrigin,
    uint32_t sourceMipLevel, Texture* destination, const WGPUOrigin3D& destinationOrigin,
    uint32_t destinationMipLevel, const WGPUExtent3D& extent)
{
    // For 2D textures and arrays, depth represents layer count
    // For 3D textures, it represents actual depth
    WGPUOrigin3D srcOrigin = sourceOrigin;
    WGPUOrigin3D dstOrigin = destinationOrigin;
    if (source->getDimension() != WGPUTextureDimension_3D) {
        srcOrigin.z = 0;
        dstOrigin.z = 0;
    }

    WGPUTexelCopyTextureInfo sourceInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    sourceInfo.texture = source->handle();
    sourceInfo.mipLevel = sourceMipLevel;
    sourceInfo.origin = srcOrigin;

    WGPUTexelCopyTextureInfo destInfo = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    destInfo.texture = destination->handle();
    destInfo.mipLevel = destinationMipLevel;
    destInfo.origin = dstOrigin;

    wgpuCommandEncoderCopyTextureToTexture(m_encoder, &sourceInfo, &destInfo, &extent);
}

void CommandEncoder::blitTextureToTexture(Texture* source, const WGPUOrigin3D& sourceOrigin, const WGPUExtent3D& sourceExtent, uint32_t sourceMipLevel,
    Texture* destination, const WGPUOrigin3D& destinationOrigin, const WGPUExtent3D& destinationExtent, uint32_t destinationMipLevel,
    WGPUFilterMode filter)
{
    // Get the Blit helper from the device
    Blit* blit = m_device->getBlit();
    blit->execute(m_encoder,
        source->handle(), sourceOrigin, sourceExtent, sourceMipLevel,
        destination->handle(), destinationOrigin, destinationExtent, destinationMipLevel,
        filter);
}

RenderPassEncoder::RenderPassEncoder(CommandEncoder* commandEncoder, RenderPass* renderPass, Framebuffer* framebuffer, const RenderPassEncoderBeginInfo& beginInfo)
{
    // Combine render pass ops with framebuffer views
    const RenderPassCreateInfo& passInfo = renderPass->getCreateInfo();
    const FramebufferCreateInfo& fbInfo = framebuffer->getCreateInfo();

    WGPURenderPassDescriptor wgpuDesc = WGPU_RENDER_PASS_DESCRIPTOR_INIT;

    // Build color attachments directly with clear values from begin info
    std::vector<WGPURenderPassColorAttachment> wgpuColorAttachments;
    for (size_t i = 0; i < fbInfo.colorAttachmentViews.size(); ++i) {
        WGPURenderPassColorAttachment attachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
        // Get actual WGPUTextureView handle from TextureView pointer
        attachment.view = fbInfo.colorAttachmentViews[i] ? fbInfo.colorAttachmentViews[i]->handle() : nullptr;
        attachment.loadOp = passInfo.colorAttachments[i].loadOp;
        attachment.storeOp = passInfo.colorAttachments[i].storeOp;

        // Set resolve target if provided
        if (i < fbInfo.colorResolveTargetViews.size() && fbInfo.colorResolveTargetViews[i]) {
            attachment.resolveTarget = fbInfo.colorResolveTargetViews[i]->handle();
        }

        // Set clear color from begin info
        if (i < beginInfo.colorClearValues.size()) {
            attachment.clearValue = beginInfo.colorClearValues[i];
        }

        wgpuColorAttachments.push_back(attachment);
    }

    if (!wgpuColorAttachments.empty()) {
        wgpuDesc.colorAttachments = wgpuColorAttachments.data();
        wgpuDesc.colorAttachmentCount = static_cast<uint32_t>(wgpuColorAttachments.size());
    }

    // Build depth/stencil attachment directly
    WGPURenderPassDepthStencilAttachment wgpuDepthStencil = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
    if (fbInfo.depthStencilAttachmentView) {
        const auto& depthStencilAtt = passInfo.depthStencilAttachment.value();

        // Get actual WGPUTextureView handle from TextureView pointer
        wgpuDepthStencil.view = fbInfo.depthStencilAttachmentView->handle();
        wgpuDepthStencil.depthLoadOp = depthStencilAtt.depthLoadOp;
        wgpuDepthStencil.depthStoreOp = depthStencilAtt.depthStoreOp;
        wgpuDepthStencil.depthClearValue = beginInfo.depthClearValue;

        // Only set stencil operations for formats that have a stencil aspect
        WGPUTextureFormat format = fbInfo.depthStencilAttachmentView->getTexture()->getFormat();
        if (hasStencil(format)) {
            wgpuDepthStencil.stencilLoadOp = depthStencilAtt.stencilLoadOp;
            wgpuDepthStencil.stencilStoreOp = depthStencilAtt.stencilStoreOp;
            wgpuDepthStencil.stencilClearValue = beginInfo.stencilClearValue;
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
        wgpuDesc.label = toStringView(createInfo.label);
    }

    m_encoder = wgpuCommandEncoderBeginComputePass(commandEncoder->handle(), &wgpuDesc);
    if (!m_encoder) {
        throw std::runtime_error("Failed to create compute pass encoder");
    }
}

// ============================================================================
// Texture Implementation
// ============================================================================

void Texture::generateMipmaps(CommandEncoder* encoder)
{
    if (m_info.mipLevels <= 1) {
        return; // No mipmaps to generate
    }

    generateMipmapsRange(encoder, 0, m_info.mipLevels);
}

void Texture::generateMipmapsRange(CommandEncoder* encoder, uint32_t baseMipLevel, uint32_t levelCount)
{
    if (levelCount <= 1) {
        return; // Nothing to generate
    }

    // Get the Blit helper from the device
    Blit* blit = encoder->getDevice()->getBlit();

    // Generate each mip level by blitting from the previous level
    for (uint32_t i = 0; i < levelCount - 1; ++i) {
        uint32_t srcMip = baseMipLevel + i;
        uint32_t dstMip = srcMip + 1;

        // Calculate extents for each mip level
        WGPUExtent3D srcSize = m_info.size;
        uint32_t srcWidth = std::max(1u, srcSize.width >> srcMip);
        uint32_t srcHeight = std::max(1u, srcSize.height >> srcMip);
        uint32_t dstWidth = std::max(1u, srcSize.width >> dstMip);
        uint32_t dstHeight = std::max(1u, srcSize.height >> dstMip);

        WGPUOrigin3D origin = { 0, 0, 0 };
        WGPUExtent3D srcExtent = { srcWidth, srcHeight, 1 };
        WGPUExtent3D dstExtent = { dstWidth, dstHeight, 1 };

        // Use linear filtering for mipmap generation
        blit->execute(encoder->handle(),
            m_texture, origin, srcExtent, srcMip,
            m_texture, origin, dstExtent, dstMip,
            WGPUFilterMode_Linear);
    }
}

WGPUTextureView TextureView::handle() const
{
    if (m_swapchain) {
        // Get the raw view handle from swapchain (created on-demand in acquireNextImage)
        return m_swapchain->getCurrentNativeTextureView();
    }
    return m_view;
}

} // namespace gfx::webgpu