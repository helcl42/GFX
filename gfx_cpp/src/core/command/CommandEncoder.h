#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>
#include <vector>

namespace gfx {

class CCommandEncoderImpl : public CommandEncoder {
public:
    explicit CCommandEncoderImpl(GfxCommandEncoder h);
    ~CCommandEncoderImpl() override;

    GfxCommandEncoder getHandle() const;

    std::shared_ptr<RenderPassEncoder> beginRenderPass(const RenderPassBeginDescriptor& descriptor) override;
    std::shared_ptr<ComputePassEncoder> beginComputePass(const ComputePassBeginDescriptor& descriptor) override;
    void copyBufferToBuffer(
        std::shared_ptr<Buffer> source, uint64_t sourceOffset,
        std::shared_ptr<Buffer> destination, uint64_t destinationOffset,
        uint64_t size) override;
    void copyBufferToTexture(
        std::shared_ptr<Buffer> source, uint64_t sourceOffset, uint32_t bytesPerRow,
        std::shared_ptr<Texture> destination, const Origin3D& origin,
        const Extent3D& extent, uint32_t mipLevel, TextureLayout finalLayout) override;
    void copyTextureToBuffer(
        std::shared_ptr<Texture> source, const Origin3D& origin, uint32_t mipLevel,
        std::shared_ptr<Buffer> destination, uint64_t destinationOffset, uint32_t bytesPerRow,
        const Extent3D& extent, TextureLayout finalLayout) override;
    void copyTextureToTexture(
        std::shared_ptr<Texture> source, const Origin3D& sourceOrigin, uint32_t sourceMipLevel, TextureLayout sourceFinalLayout,
        std::shared_ptr<Texture> destination, const Origin3D& destinationOrigin, uint32_t destinationMipLevel, TextureLayout destinationFinalLayout,
        const Extent3D& extent) override;
    void blitTextureToTexture(
        std::shared_ptr<Texture> source, const Origin3D& sourceOrigin, const Extent3D& sourceExtent, uint32_t sourceMipLevel, TextureLayout sourceFinalLayout,
        std::shared_ptr<Texture> destination, const Origin3D& destinationOrigin, const Extent3D& destinationExtent, uint32_t destinationMipLevel, TextureLayout destinationFinalLayout,
        FilterMode filter) override;
    void pipelineBarrier(
        const std::vector<MemoryBarrier>& memoryBarriers = {},
        const std::vector<BufferBarrier>& bufferBarriers = {},
        const std::vector<TextureBarrier>& textureBarriers = {}) override;
    void generateMipmaps(std::shared_ptr<Texture> texture) override;
    void generateMipmapsRange(std::shared_ptr<Texture> texture, uint32_t baseMipLevel, uint32_t levelCount) override;
    void end() override;
    void begin() override;

private:
    GfxCommandEncoder m_handle;
};

} // namespace gfx
