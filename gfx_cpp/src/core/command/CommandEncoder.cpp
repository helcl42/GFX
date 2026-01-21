#include "CommandEncoder.h"

#include "ComputePassEncoder.h"
#include "RenderPassEncoder.h"

#include "../render/Framebuffer.h"
#include "../resource/Buffer.h"
#include "../resource/Texture.h"

#include "../../converter/Conversions.h"

#include <stdexcept>

namespace gfx {

CommandEncoderImpl::CommandEncoderImpl(GfxCommandEncoder h)
    : m_handle(h)
{
}

CommandEncoderImpl::~CommandEncoderImpl()
{
    if (m_handle) {
        gfxCommandEncoderDestroy(m_handle);
    }
}

GfxCommandEncoder CommandEncoderImpl::getHandle() const
{
    return m_handle;
}

std::shared_ptr<RenderPassEncoder> CommandEncoderImpl::beginRenderPass(const RenderPassBeginDescriptor& descriptor)
{
    auto framebufferImpl = std::dynamic_pointer_cast<FramebufferImpl>(descriptor.framebuffer);
    if (!framebufferImpl) {
        throw std::runtime_error("Invalid framebuffer type");
    }

    std::vector<GfxColor> cClearValues;
    GfxRenderPassBeginDescriptor cDesc;
    convertRenderPassBeginDescriptor(descriptor, framebufferImpl->getRenderPass(), framebufferImpl->getHandle(), cClearValues, cDesc);

    GfxRenderPassEncoder encoder = nullptr;
    GfxResult result = gfxCommandEncoderBeginRenderPass(m_handle, &cDesc, &encoder);
    if (result != GFX_RESULT_SUCCESS || !encoder) {
        throw std::runtime_error("Failed to begin render pass");
    }
    return std::make_shared<RenderPassEncoderImpl>(encoder);
}

std::shared_ptr<ComputePassEncoder> CommandEncoderImpl::beginComputePass(const ComputePassBeginDescriptor& descriptor)
{
    GfxComputePassBeginDescriptor cDesc;
    convertComputePassBeginDescriptor(descriptor, cDesc);

    GfxComputePassEncoder encoder = nullptr;
    GfxResult result = gfxCommandEncoderBeginComputePass(m_handle, &cDesc, &encoder);
    if (result != GFX_RESULT_SUCCESS || !encoder) {
        throw std::runtime_error("Failed to begin compute pass");
    }
    return std::make_shared<ComputePassEncoderImpl>(encoder);
}

void CommandEncoderImpl::copyBufferToBuffer(const CopyBufferToBufferDescriptor& descriptor)
{
    GfxCopyBufferToBufferDescriptor cDesc;
    convertCopyBufferToBufferDescriptor(descriptor, cDesc);

    GfxResult result = gfxCommandEncoderCopyBufferToBuffer(m_handle, &cDesc);
    if (result != GFX_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to copy buffer to buffer");
    }
}

void CommandEncoderImpl::copyBufferToTexture(const CopyBufferToTextureDescriptor& descriptor)
{
    GfxCopyBufferToTextureDescriptor cDesc;
    convertCopyBufferToTextureDescriptor(descriptor, cDesc);

    GfxResult result = gfxCommandEncoderCopyBufferToTexture(m_handle, &cDesc);
    if (result != GFX_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to copy buffer to texture");
    }
}

void CommandEncoderImpl::copyTextureToBuffer(const CopyTextureToBufferDescriptor& descriptor)
{
    GfxCopyTextureToBufferDescriptor cDesc;
    convertCopyTextureToBufferDescriptor(descriptor, cDesc);

    GfxResult result = gfxCommandEncoderCopyTextureToBuffer(m_handle, &cDesc);
    if (result != GFX_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to copy texture to buffer");
    }
}

void CommandEncoderImpl::copyTextureToTexture(const CopyTextureToTextureDescriptor& descriptor)
{
    GfxCopyTextureToTextureDescriptor cDesc;
    convertCopyTextureToTextureDescriptor(descriptor, cDesc);

    GfxResult result = gfxCommandEncoderCopyTextureToTexture(m_handle, &cDesc);
    if (result != GFX_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to copy texture to texture");
    }
}

void CommandEncoderImpl::blitTextureToTexture(const BlitTextureToTextureDescriptor& descriptor)
{
    GfxBlitTextureToTextureDescriptor cDesc;
    convertBlitTextureToTextureDescriptor(descriptor, cDesc);

    GfxResult result = gfxCommandEncoderBlitTextureToTexture(m_handle, &cDesc);
    if (result != GFX_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to blit texture to texture");
    }
}

void CommandEncoderImpl::pipelineBarrier(const PipelineBarrierDescriptor& descriptor)
{
    GfxPipelineBarrierDescriptor cDesc;
    std::vector<GfxMemoryBarrier> memBarriers;
    std::vector<GfxBufferBarrier> bufBarriers;
    std::vector<GfxTextureBarrier> texBarriers;
    
    convertPipelineBarrierDescriptor(descriptor, cDesc, memBarriers, bufBarriers, texBarriers);
    
    GfxResult result = gfxCommandEncoderPipelineBarrier(m_handle, &cDesc);
    if (result != GFX_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to insert pipeline barrier");
    }
}

void CommandEncoderImpl::generateMipmaps(std::shared_ptr<Texture> texture)
{
    auto tex = std::dynamic_pointer_cast<TextureImpl>(texture);
    if (tex) {
        GfxResult result = gfxCommandEncoderGenerateMipmaps(m_handle, tex->getHandle());
        if (result != GFX_RESULT_SUCCESS) {
            throw std::runtime_error("Failed to generate mipmaps");
        }
    }
}

void CommandEncoderImpl::generateMipmapsRange(std::shared_ptr<Texture> texture, uint32_t baseMipLevel, uint32_t levelCount)
{
    auto tex = std::dynamic_pointer_cast<TextureImpl>(texture);
    if (tex) {
        GfxResult result = gfxCommandEncoderGenerateMipmapsRange(m_handle, tex->getHandle(), baseMipLevel, levelCount);
        if (result != GFX_RESULT_SUCCESS) {
            throw std::runtime_error("Failed to generate mipmaps range");
        }
    }
}

void CommandEncoderImpl::end()
{
    GfxResult result = gfxCommandEncoderEnd(m_handle);
    if (result != GFX_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to end command encoder");
    }
}

void CommandEncoderImpl::begin()
{
    GfxResult result = gfxCommandEncoderBegin(m_handle);
    if (result != GFX_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to begin command encoder");
    }
}

} // namespace gfx
