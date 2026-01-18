#ifndef GFX_WEBGPU_RENDER_PASS_ENCODER_H
#define GFX_WEBGPU_RENDER_PASS_ENCODER_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class CommandEncoder;
class RenderPass;
class Framebuffer;

class RenderPassEncoder {
public:
    // Prevent copying
    RenderPassEncoder(const RenderPassEncoder&) = delete;
    RenderPassEncoder& operator=(const RenderPassEncoder&) = delete;

    RenderPassEncoder(CommandEncoder* commandEncoder, RenderPass* renderPass, Framebuffer* framebuffer, const RenderPassEncoderBeginInfo& beginInfo);
    ~RenderPassEncoder();

    void setPipeline(WGPURenderPipeline pipeline);
    void setBindGroup(uint32_t index, WGPUBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount);
    void setVertexBuffer(uint32_t slot, WGPUBuffer buffer, uint64_t offset, uint64_t size);
    void setIndexBuffer(WGPUBuffer buffer, WGPUIndexFormat format, uint64_t offset, uint64_t size);

    void setViewport(float x, float y, float width, float height, float minDepth, float maxDepth);
    void setScissorRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance);

    WGPURenderPassEncoder handle() const;

private:
    WGPURenderPassEncoder m_encoder = nullptr;
    bool m_ended = false;
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_RENDER_PASS_ENCODER_H