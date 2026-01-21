#ifndef GFX_CPP_COMMAND_ENCODER_H
#define GFX_CPP_COMMAND_ENCODER_H

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>
#include <vector>

namespace gfx {

class CommandEncoderImpl : public CommandEncoder {
public:
    explicit CommandEncoderImpl(GfxCommandEncoder h);
    ~CommandEncoderImpl() override;

    GfxCommandEncoder getHandle() const;

    std::shared_ptr<RenderPassEncoder> beginRenderPass(const RenderPassBeginDescriptor& descriptor) override;
    std::shared_ptr<ComputePassEncoder> beginComputePass(const ComputePassBeginDescriptor& descriptor) override;
    void copyBufferToBuffer(const CopyBufferToBufferDescriptor& descriptor) override;
    void copyBufferToTexture(const CopyBufferToTextureDescriptor& descriptor) override;
    void copyTextureToBuffer(const CopyTextureToBufferDescriptor& descriptor) override;
    void copyTextureToTexture(const CopyTextureToTextureDescriptor& descriptor) override;
    void blitTextureToTexture(const BlitTextureToTextureDescriptor& descriptor) override;
    void pipelineBarrier(const PipelineBarrierDescriptor& descriptor) override;
    void generateMipmaps(std::shared_ptr<Texture> texture) override;
    void generateMipmapsRange(std::shared_ptr<Texture> texture, uint32_t baseMipLevel, uint32_t levelCount) override;
    void end() override;
    void begin() override;

private:
    GfxCommandEncoder m_handle;
};

} // namespace gfx

#endif // GFX_CPP_COMMAND_ENCODER_H
