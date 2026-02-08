#ifndef GFX_CPP_RENDER_PASS_ENCODER_H
#define GFX_CPP_RENDER_PASS_ENCODER_H

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>

namespace gfx {

class RenderPassEncoderImpl : public RenderPassEncoder {
public:
    explicit RenderPassEncoderImpl(GfxRenderPassEncoder h);
    ~RenderPassEncoderImpl() override;

    void setPipeline(std::shared_ptr<RenderPipeline> pipeline) override;
    void setBindGroup(uint32_t index, std::shared_ptr<BindGroup> bindGroup, const uint32_t* dynamicOffsets = nullptr, uint32_t dynamicOffsetCount = 0) override;
    void setVertexBuffer(uint32_t slot, std::shared_ptr<Buffer> buffer, uint64_t offset = 0, uint64_t size = 0) override;
    void setIndexBuffer(std::shared_ptr<Buffer> buffer, IndexFormat format, uint64_t offset = 0, uint64_t size = UINT64_MAX) override;

    void setViewport(const Viewport& viewport) override;
    void setScissorRect(const ScissorRect& scissor) override;

    void draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) override;
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t baseVertex = 0, uint32_t firstInstance = 0) override;
    void drawIndirect(std::shared_ptr<Buffer> indirectBuffer, uint64_t indirectOffset) override;
    void drawIndexedIndirect(std::shared_ptr<Buffer> indirectBuffer, uint64_t indirectOffset) override;

    void beginOcclusionQuery(std::shared_ptr<QuerySet> querySet, uint32_t queryIndex) override;
    void endOcclusionQuery() override;

private:
    GfxRenderPassEncoder m_handle;
};

} // namespace gfx

#endif // GFX_CPP_RENDER_PASS_ENCODER_H
