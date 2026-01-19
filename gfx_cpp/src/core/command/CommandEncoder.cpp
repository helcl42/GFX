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

    // Convert clear values
    std::vector<GfxColor> cClearValues;
    for (const auto& color : descriptor.colorClearValues) {
        cClearValues.push_back({ color.r, color.g, color.b, color.a });
    }

    GfxRenderPassBeginDescriptor cDesc = {};
    cDesc.renderPass = framebufferImpl->getRenderPass();
    cDesc.framebuffer = framebufferImpl->getHandle();
    cDesc.colorClearValues = cClearValues.empty() ? nullptr : cClearValues.data();
    cDesc.colorClearValueCount = static_cast<uint32_t>(cClearValues.size());
    cDesc.depthClearValue = descriptor.depthClearValue;
    cDesc.stencilClearValue = descriptor.stencilClearValue;

    GfxRenderPassEncoder encoder = nullptr;
    GfxResult result = gfxCommandEncoderBeginRenderPass(m_handle, &cDesc, &encoder);
    if (result != GFX_RESULT_SUCCESS || !encoder) {
        throw std::runtime_error("Failed to begin render pass");
    }
    return std::make_shared<RenderPassEncoderImpl>(encoder);
}

std::shared_ptr<ComputePassEncoder> CommandEncoderImpl::beginComputePass(const ComputePassBeginDescriptor& descriptor)
{
    GfxComputePassBeginDescriptor cDesc = {};
    cDesc.label = descriptor.label.c_str();

    GfxComputePassEncoder encoder = nullptr;
    GfxResult result = gfxCommandEncoderBeginComputePass(m_handle, &cDesc, &encoder);
    if (result != GFX_RESULT_SUCCESS || !encoder) {
        throw std::runtime_error("Failed to begin compute pass");
    }
    return std::make_shared<ComputePassEncoderImpl>(encoder);
}

void CommandEncoderImpl::copyBufferToBuffer(
    std::shared_ptr<Buffer> source, uint64_t sourceOffset,
    std::shared_ptr<Buffer> destination, uint64_t destinationOffset,
    uint64_t size)
{
    auto src = std::dynamic_pointer_cast<BufferImpl>(source);
    auto dst = std::dynamic_pointer_cast<BufferImpl>(destination);
    if (src && dst) {
        GfxResult result = gfxCommandEncoderCopyBufferToBuffer(m_handle, src->getHandle(), sourceOffset,
            dst->getHandle(), destinationOffset, size);
        if (result != GFX_RESULT_SUCCESS) {
            throw std::runtime_error("Failed to copy buffer to buffer");
        }
    }
}

void CommandEncoderImpl::copyBufferToTexture(
    std::shared_ptr<Buffer> source, uint64_t sourceOffset, uint32_t bytesPerRow,
    std::shared_ptr<Texture> destination, const Origin3D& origin,
    const Extent3D& extent, uint32_t mipLevel, TextureLayout finalLayout)
{
    auto src = std::dynamic_pointer_cast<BufferImpl>(source);
    auto dst = std::dynamic_pointer_cast<TextureImpl>(destination);
    if (src && dst) {
        GfxOrigin3D cOrigin = { origin.x, origin.y, origin.z };
        GfxExtent3D cExtent = { extent.width, extent.height, extent.depth };
        GfxResult result = gfxCommandEncoderCopyBufferToTexture(m_handle, src->getHandle(), sourceOffset, bytesPerRow,
            dst->getHandle(), &cOrigin, &cExtent, mipLevel, cppLayoutToCLayout(finalLayout));
        if (result != GFX_RESULT_SUCCESS) {
            throw std::runtime_error("Failed to copy buffer to texture");
        }
    }
}

void CommandEncoderImpl::copyTextureToBuffer(
    std::shared_ptr<Texture> source, const Origin3D& origin, uint32_t mipLevel,
    std::shared_ptr<Buffer> destination, uint64_t destinationOffset, uint32_t bytesPerRow,
    const Extent3D& extent, TextureLayout finalLayout)
{
    auto src = std::dynamic_pointer_cast<TextureImpl>(source);
    auto dst = std::dynamic_pointer_cast<BufferImpl>(destination);
    if (src && dst) {
        GfxOrigin3D cOrigin = { origin.x, origin.y, origin.z };
        GfxExtent3D cExtent = { extent.width, extent.height, extent.depth };
        GfxResult result = gfxCommandEncoderCopyTextureToBuffer(m_handle, src->getHandle(), &cOrigin, mipLevel,
            dst->getHandle(), destinationOffset, bytesPerRow, &cExtent, cppLayoutToCLayout(finalLayout));
        if (result != GFX_RESULT_SUCCESS) {
            throw std::runtime_error("Failed to copy texture to buffer");
        }
    }
}

void CommandEncoderImpl::copyTextureToTexture(
    std::shared_ptr<Texture> source, const Origin3D& sourceOrigin, uint32_t sourceMipLevel, TextureLayout sourceFinalLayout,
    std::shared_ptr<Texture> destination, const Origin3D& destinationOrigin, uint32_t destinationMipLevel, TextureLayout destinationFinalLayout,
    const Extent3D& extent)
{
    auto src = std::dynamic_pointer_cast<TextureImpl>(source);
    auto dst = std::dynamic_pointer_cast<TextureImpl>(destination);
    if (src && dst) {
        GfxOrigin3D cSourceOrigin = { sourceOrigin.x, sourceOrigin.y, sourceOrigin.z };
        GfxOrigin3D cDestOrigin = { destinationOrigin.x, destinationOrigin.y, destinationOrigin.z };
        GfxExtent3D cExtent = { extent.width, extent.height, extent.depth };
        GfxResult result = gfxCommandEncoderCopyTextureToTexture(m_handle,
            src->getHandle(), &cSourceOrigin, sourceMipLevel, cppLayoutToCLayout(sourceFinalLayout),
            dst->getHandle(), &cDestOrigin, destinationMipLevel, cppLayoutToCLayout(destinationFinalLayout),
            &cExtent);
        if (result != GFX_RESULT_SUCCESS) {
            throw std::runtime_error("Failed to copy texture to texture");
        }
    }
}

void CommandEncoderImpl::blitTextureToTexture(
    std::shared_ptr<Texture> source, const Origin3D& sourceOrigin, const Extent3D& sourceExtent, uint32_t sourceMipLevel, TextureLayout sourceFinalLayout,
    std::shared_ptr<Texture> destination, const Origin3D& destinationOrigin, const Extent3D& destinationExtent, uint32_t destinationMipLevel, TextureLayout destinationFinalLayout,
    FilterMode filter)
{
    auto src = std::dynamic_pointer_cast<TextureImpl>(source);
    auto dst = std::dynamic_pointer_cast<TextureImpl>(destination);
    if (src && dst) {
        GfxOrigin3D cSourceOrigin = { sourceOrigin.x, sourceOrigin.y, sourceOrigin.z };
        GfxExtent3D cSourceExtent = { sourceExtent.width, sourceExtent.height, sourceExtent.depth };
        GfxOrigin3D cDestOrigin = { destinationOrigin.x, destinationOrigin.y, destinationOrigin.z };
        GfxExtent3D cDestExtent = { destinationExtent.width, destinationExtent.height, destinationExtent.depth };
        GfxResult result = gfxCommandEncoderBlitTextureToTexture(m_handle,
            src->getHandle(), &cSourceOrigin, &cSourceExtent, sourceMipLevel, cppLayoutToCLayout(sourceFinalLayout),
            dst->getHandle(), &cDestOrigin, &cDestExtent, destinationMipLevel, cppLayoutToCLayout(destinationFinalLayout),
            cppFilterModeToCFilterMode(filter));
        if (result != GFX_RESULT_SUCCESS) {
            throw std::runtime_error("Failed to blit texture to texture");
        }
    }
}

void CommandEncoderImpl::pipelineBarrier(
    const std::vector<MemoryBarrier>& memoryBarriers,
    const std::vector<BufferBarrier>& bufferBarriers,
    const std::vector<TextureBarrier>& textureBarriers)
{
    if (memoryBarriers.empty() && bufferBarriers.empty() && textureBarriers.empty()) {
        return;
    }

    std::vector<GfxMemoryBarrier> cMemoryBarriers;
    cMemoryBarriers.reserve(memoryBarriers.size());

    for (const auto& barrier : memoryBarriers) {
        GfxMemoryBarrier cBarrier{};
        cBarrier.srcStageMask = cppPipelineStageToCPipelineStage(barrier.srcStageMask);
        cBarrier.dstStageMask = cppPipelineStageToCPipelineStage(barrier.dstStageMask);
        cBarrier.srcAccessMask = cppAccessFlagsToCAccessFlags(barrier.srcAccessMask);
        cBarrier.dstAccessMask = cppAccessFlagsToCAccessFlags(barrier.dstAccessMask);
        cMemoryBarriers.push_back(cBarrier);
    }

    std::vector<GfxBufferBarrier> cBufferBarriers;
    cBufferBarriers.reserve(bufferBarriers.size());

    for (const auto& barrier : bufferBarriers) {
        auto buf = std::dynamic_pointer_cast<BufferImpl>(barrier.buffer);
        if (buf) {
            GfxBufferBarrier cBarrier{};
            cBarrier.buffer = buf->getHandle();
            cBarrier.srcStageMask = cppPipelineStageToCPipelineStage(barrier.srcStageMask);
            cBarrier.dstStageMask = cppPipelineStageToCPipelineStage(barrier.dstStageMask);
            cBarrier.srcAccessMask = cppAccessFlagsToCAccessFlags(barrier.srcAccessMask);
            cBarrier.dstAccessMask = cppAccessFlagsToCAccessFlags(barrier.dstAccessMask);
            cBarrier.offset = barrier.offset;
            cBarrier.size = barrier.size;
            cBufferBarriers.push_back(cBarrier);
        }
    }

    std::vector<GfxTextureBarrier> cTextureBarriers;
    cTextureBarriers.reserve(textureBarriers.size());

    for (const auto& barrier : textureBarriers) {
        auto tex = std::dynamic_pointer_cast<TextureImpl>(barrier.texture);
        if (tex) {
            GfxTextureBarrier cBarrier{};
            cBarrier.texture = tex->getHandle();
            cBarrier.oldLayout = cppLayoutToCLayout(barrier.oldLayout);
            cBarrier.newLayout = cppLayoutToCLayout(barrier.newLayout);
            cBarrier.srcStageMask = cppPipelineStageToCPipelineStage(barrier.srcStageMask);
            cBarrier.dstStageMask = cppPipelineStageToCPipelineStage(barrier.dstStageMask);

            // Auto-deduce access masks if not explicitly set
            cBarrier.srcAccessMask = (barrier.srcAccessMask == AccessFlags::None)
                ? gfxGetAccessFlagsForLayout(cBarrier.oldLayout)
                : cppAccessFlagsToCAccessFlags(barrier.srcAccessMask);
            cBarrier.dstAccessMask = (barrier.dstAccessMask == AccessFlags::None)
                ? gfxGetAccessFlagsForLayout(cBarrier.newLayout)
                : cppAccessFlagsToCAccessFlags(barrier.dstAccessMask);

            cBarrier.baseMipLevel = barrier.baseMipLevel;
            cBarrier.mipLevelCount = barrier.mipLevelCount;
            cBarrier.baseArrayLayer = barrier.baseArrayLayer;
            cBarrier.arrayLayerCount = barrier.arrayLayerCount;
            cTextureBarriers.push_back(cBarrier);
        }
    }

    if (!cMemoryBarriers.empty() || !cBufferBarriers.empty() || !cTextureBarriers.empty()) {
        GfxResult result = gfxCommandEncoderPipelineBarrier(m_handle,
            cMemoryBarriers.empty() ? nullptr : cMemoryBarriers.data(),
            static_cast<uint32_t>(cMemoryBarriers.size()),
            cBufferBarriers.empty() ? nullptr : cBufferBarriers.data(),
            static_cast<uint32_t>(cBufferBarriers.size()),
            cTextureBarriers.empty() ? nullptr : cTextureBarriers.data(),
            static_cast<uint32_t>(cTextureBarriers.size()));
        if (result != GFX_RESULT_SUCCESS) {
            throw std::runtime_error("Failed to insert pipeline barrier");
        }
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
