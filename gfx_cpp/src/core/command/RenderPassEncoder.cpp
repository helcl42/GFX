#include "RenderPassEncoder.h"

#include "../render/RenderPipeline.h"
#include "../resource/BindGroup.h"
#include "../resource/Buffer.h"

#include <stdexcept>

namespace gfx {

CRenderPassEncoderImpl::CRenderPassEncoderImpl(GfxRenderPassEncoder h)
    : m_handle(h)
{
}

CRenderPassEncoderImpl::~CRenderPassEncoderImpl()
{
    if (m_handle) {
        GfxResult result = gfxRenderPassEncoderEnd(m_handle);
        if (result != GFX_RESULT_SUCCESS) {
            // Can't throw from destructor, just log or ignore
        }
    }
}

void CRenderPassEncoderImpl::setPipeline(std::shared_ptr<RenderPipeline> pipeline)
{
    auto impl = std::dynamic_pointer_cast<CRenderPipelineImpl>(pipeline);
    if (impl) {
        GfxResult result = gfxRenderPassEncoderSetPipeline(m_handle, impl->getHandle());
        if (result != GFX_RESULT_SUCCESS) {
            throw std::runtime_error("Failed to set render pipeline");
        }
    }
}

void CRenderPassEncoderImpl::setBindGroup(uint32_t index, std::shared_ptr<BindGroup> bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount)
{
    auto impl = std::dynamic_pointer_cast<CBindGroupImpl>(bindGroup);
    if (impl) {
        GfxResult result = gfxRenderPassEncoderSetBindGroup(m_handle, index, impl->getHandle(), dynamicOffsets, dynamicOffsetCount);
        if (result != GFX_RESULT_SUCCESS) {
            throw std::runtime_error("Failed to set bind group");
        }
    }
}

void CRenderPassEncoderImpl::setVertexBuffer(uint32_t slot, std::shared_ptr<Buffer> buffer, uint64_t offset, uint64_t size)
{
    auto impl = std::dynamic_pointer_cast<CBufferImpl>(buffer);
    if (impl) {
        GfxResult result = gfxRenderPassEncoderSetVertexBuffer(m_handle, slot, impl->getHandle(), offset, size);
        if (result != GFX_RESULT_SUCCESS) {
            throw std::runtime_error("Failed to set vertex buffer");
        }
    }
}

void CRenderPassEncoderImpl::setIndexBuffer(std::shared_ptr<Buffer> buffer, IndexFormat format, uint64_t offset, uint64_t size)
{
    auto impl = std::dynamic_pointer_cast<CBufferImpl>(buffer);
    if (impl) {
        GfxIndexFormat cFormat = (format == IndexFormat::Uint16) ? GFX_INDEX_FORMAT_UINT16 : GFX_INDEX_FORMAT_UINT32;
        GfxResult result = gfxRenderPassEncoderSetIndexBuffer(m_handle, impl->getHandle(), cFormat, offset, size);
        if (result != GFX_RESULT_SUCCESS) {
            throw std::runtime_error("Failed to set index buffer");
        }
    }
}

void CRenderPassEncoderImpl::setViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
{
    GfxViewport viewport = { x, y, width, height, minDepth, maxDepth };
    GfxResult result = gfxRenderPassEncoderSetViewport(m_handle, &viewport);
    if (result != GFX_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to set viewport");
    }
}

void CRenderPassEncoderImpl::setScissorRect(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    GfxScissorRect scissor = { x, y, width, height };
    GfxResult result = gfxRenderPassEncoderSetScissorRect(m_handle, &scissor);
    if (result != GFX_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to set scissor rect");
    }
}

void CRenderPassEncoderImpl::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    GfxResult result = gfxRenderPassEncoderDraw(m_handle, vertexCount, instanceCount, firstVertex, firstInstance);
    if (result != GFX_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to draw");
    }
}

void CRenderPassEncoderImpl::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance)
{
    GfxResult result = gfxRenderPassEncoderDrawIndexed(m_handle, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
    if (result != GFX_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to draw indexed");
    }
}

void CRenderPassEncoderImpl::drawIndirect(std::shared_ptr<Buffer> indirectBuffer, uint64_t indirectOffset)
{
    if (!indirectBuffer) {
        throw std::invalid_argument("Indirect buffer cannot be null");
    }
    auto* bufferImpl = dynamic_cast<CBufferImpl*>(indirectBuffer.get());
    if (!bufferImpl) {
        throw std::runtime_error("Invalid buffer implementation");
    }
    GfxResult result = gfxRenderPassEncoderDrawIndirect(m_handle, bufferImpl->getHandle(), indirectOffset);
    if (result != GFX_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to draw indirect");
    }
}

void CRenderPassEncoderImpl::drawIndexedIndirect(std::shared_ptr<Buffer> indirectBuffer, uint64_t indirectOffset)
{
    if (!indirectBuffer) {
        throw std::invalid_argument("Indirect buffer cannot be null");
    }
    auto* bufferImpl = dynamic_cast<CBufferImpl*>(indirectBuffer.get());
    if (!bufferImpl) {
        throw std::runtime_error("Invalid buffer implementation");
    }
    GfxResult result = gfxRenderPassEncoderDrawIndexedIndirect(m_handle, bufferImpl->getHandle(), indirectOffset);
    if (result != GFX_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to draw indexed indirect");
    }
}

} // namespace gfx
