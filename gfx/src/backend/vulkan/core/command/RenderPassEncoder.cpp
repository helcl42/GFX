#include "RenderPassEncoder.h"

#include "CommandEncoder.h"

#include "../render/Framebuffer.h"
#include "../render/RenderPass.h"
#include "../render/RenderPipeline.h"
#include "../resource/BindGroup.h"
#include "../resource/Buffer.h"
#include "../system/Device.h"

namespace gfx::backend::vulkan::core {

RenderPassEncoder::RenderPassEncoder(CommandEncoder* commandEncoder, RenderPass* renderPass, Framebuffer* framebuffer, const RenderPassEncoderBeginInfo& beginInfo)
    : m_commandBuffer(commandEncoder->handle())
    , m_device(commandEncoder->getDevice())
    , m_commandEncoder(commandEncoder)
{
    // Build clear values array
    std::vector<VkClearValue> clearValues;

    // Add color clear values with dummy values for resolve attachments
    const auto& colorHasResolve = renderPass->colorHasResolve();
    for (size_t i = 0; i < beginInfo.colorClearValues.size(); ++i) {
        // Add the color attachment clear value
        VkClearValue clearValue{};
        clearValue.color = beginInfo.colorClearValues[i];
        clearValues.push_back(clearValue);

        // If this color attachment has a resolve target, add a dummy clear value for it
        // (resolve attachments use LOAD_OP_DONT_CARE so the value doesn't matter)
        if (i < colorHasResolve.size() && colorHasResolve[i]) {
            VkClearValue dummyClear{};
            dummyClear.color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
            clearValues.push_back(dummyClear);
        }
    }

    // Add depth/stencil clear value if needed
    if (renderPass->hasDepthStencil()) {
        VkClearValue depthStencilClear{};
        depthStencilClear.depthStencil.depth = beginInfo.depthClearValue;
        depthStencilClear.depthStencil.stencil = beginInfo.stencilClearValue;
        clearValues.push_back(depthStencilClear);
    }

    // Begin render pass
    VkRenderPassBeginInfo vkBeginInfo{};
    vkBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    vkBeginInfo.renderPass = renderPass->handle();
    vkBeginInfo.framebuffer = framebuffer->handle();
    vkBeginInfo.renderArea.offset = { 0, 0 };
    vkBeginInfo.renderArea.extent.width = framebuffer->width();
    vkBeginInfo.renderArea.extent.height = framebuffer->height();
    vkBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    vkBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(m_commandBuffer, &vkBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

RenderPassEncoder::~RenderPassEncoder()
{
    vkCmdEndRenderPass(m_commandBuffer);
}

VkCommandBuffer RenderPassEncoder::handle() const
{
    return m_commandBuffer;
}

Device* RenderPassEncoder::device() const
{
    return m_device;
}

CommandEncoder* RenderPassEncoder::commandEncoder() const
{
    return m_commandEncoder;
}

void RenderPassEncoder::setPipeline(RenderPipeline* pipeline)
{
    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->handle());
    m_commandEncoder->setCurrentPipelineLayout(pipeline->layout());
}

void RenderPassEncoder::setBindGroup(uint32_t index, BindGroup* bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount)
{
    VkPipelineLayout layout = m_commandEncoder->currentPipelineLayout();
    if (layout != VK_NULL_HANDLE) {
        VkDescriptorSet set = bindGroup->handle();
        vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, index, 1, &set, dynamicOffsetCount, dynamicOffsets);
    }
}

void RenderPassEncoder::setVertexBuffer(uint32_t slot, Buffer* buffer, uint64_t offset)
{
    VkBuffer vkBuf = buffer->handle();
    VkDeviceSize offsets[] = { offset };
    vkCmdBindVertexBuffers(m_commandBuffer, slot, 1, &vkBuf, offsets);
}

void RenderPassEncoder::setIndexBuffer(Buffer* buffer, VkIndexType indexType, uint64_t offset)
{
    vkCmdBindIndexBuffer(m_commandBuffer, buffer->handle(), offset, indexType);
}

void RenderPassEncoder::setViewport(const Viewport& viewport)
{
    VkViewport vkViewport{};
    vkViewport.x = viewport.x;
    vkViewport.y = viewport.y;
    vkViewport.width = viewport.width;
    vkViewport.height = viewport.height;
    vkViewport.minDepth = viewport.minDepth;
    vkViewport.maxDepth = viewport.maxDepth;
    vkCmdSetViewport(m_commandBuffer, 0, 1, &vkViewport);
}

void RenderPassEncoder::setScissorRect(const ScissorRect& scissor)
{
    VkRect2D vkScissor{};
    vkScissor.offset = { scissor.x, scissor.y };
    vkScissor.extent = { scissor.width, scissor.height };
    vkCmdSetScissor(m_commandBuffer, 0, 1, &vkScissor);
}

void RenderPassEncoder::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    vkCmdDraw(m_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void RenderPassEncoder::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance)
{
    vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}

void RenderPassEncoder::drawIndirect(Buffer* buffer, uint64_t offset)
{
    vkCmdDrawIndirect(m_commandBuffer, buffer->handle(), offset, 1, 0);
}

void RenderPassEncoder::drawIndexedIndirect(Buffer* buffer, uint64_t offset)
{
    vkCmdDrawIndexedIndirect(m_commandBuffer, buffer->handle(), offset, 1, 0);
}

} // namespace gfx::backend::vulkan::core