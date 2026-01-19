#pragma once

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

    void setViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f) override;
    void setScissorRect(int32_t x, int32_t y, uint32_t width, uint32_t height) override;

    void draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) override;
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t baseVertex = 0, uint32_t firstInstance = 0) override;
    void drawIndirect(std::shared_ptr<Buffer> indirectBuffer, uint64_t indirectOffset) override;
    void drawIndexedIndirect(std::shared_ptr<Buffer> indirectBuffer, uint64_t indirectOffset) override;

private:
    GfxRenderPassEncoder m_handle;
};

} // namespace gfx
