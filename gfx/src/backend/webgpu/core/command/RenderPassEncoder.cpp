#include "RenderPassEncoder.h"

#include "../command/CommandEncoder.h"
#include "../render/Framebuffer.h"
#include "../render/RenderPass.h"
#include "../resource/Texture.h"
#include "../resource/TextureView.h"
#include "../util/Utils.h"

#include <stdexcept>

namespace gfx::backend::webgpu::core {

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

RenderPassEncoder::~RenderPassEncoder()
{
    if (m_encoder) {
        if (!m_ended) {
            wgpuRenderPassEncoderEnd(m_encoder);
        }
        wgpuRenderPassEncoderRelease(m_encoder);
    }
}

void RenderPassEncoder::setPipeline(WGPURenderPipeline pipeline)
{
    wgpuRenderPassEncoderSetPipeline(m_encoder, pipeline);
}

void RenderPassEncoder::setBindGroup(uint32_t index, WGPUBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount)
{
    wgpuRenderPassEncoderSetBindGroup(m_encoder, index, bindGroup, dynamicOffsetCount, dynamicOffsets);
}

void RenderPassEncoder::setVertexBuffer(uint32_t slot, WGPUBuffer buffer, uint64_t offset, uint64_t size)
{
    wgpuRenderPassEncoderSetVertexBuffer(m_encoder, slot, buffer, offset, size);
}

void RenderPassEncoder::setIndexBuffer(WGPUBuffer buffer, WGPUIndexFormat format, uint64_t offset, uint64_t size)
{
    wgpuRenderPassEncoderSetIndexBuffer(m_encoder, buffer, format, offset, size);
}

void RenderPassEncoder::setViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
{
    wgpuRenderPassEncoderSetViewport(m_encoder, x, y, width, height, minDepth, maxDepth);
}

void RenderPassEncoder::setScissorRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    wgpuRenderPassEncoderSetScissorRect(m_encoder, x, y, width, height);
}

void RenderPassEncoder::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    wgpuRenderPassEncoderDraw(m_encoder, vertexCount, instanceCount, firstVertex, firstInstance);
}

void RenderPassEncoder::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance)
{
    wgpuRenderPassEncoderDrawIndexed(m_encoder, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}

WGPURenderPassEncoder RenderPassEncoder::handle() const
{
    return m_encoder;
}

} // namespace gfx::backend::webgpu::core