#ifndef GFX_VULKAN_RENDERPASSENCODER_H
#define GFX_VULKAN_RENDERPASSENCODER_H

#include "../CoreTypes.h"

namespace gfx::backend::vulkan::core {

class Device;
class RenderPass;
class Framebuffer;
class CommandEncoder;
class RenderPipeline;
class BindGroup;
class Buffer;

class RenderPassEncoder {
public:
    RenderPassEncoder(const RenderPassEncoder&) = delete;
    RenderPassEncoder& operator=(const RenderPassEncoder&) = delete;

    RenderPassEncoder(CommandEncoder* commandEncoder, RenderPass* renderPass, Framebuffer* framebuffer, const RenderPassEncoderBeginInfo& beginInfo);
    ~RenderPassEncoder();

    VkCommandBuffer handle() const;
    Device* device() const;
    CommandEncoder* commandEncoder() const;

    void setPipeline(RenderPipeline* pipeline);
    void setBindGroup(uint32_t index, BindGroup* bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount);
    void setVertexBuffer(uint32_t slot, Buffer* buffer, uint64_t offset);
    void setIndexBuffer(Buffer* buffer, VkIndexType indexType, uint64_t offset);
    void setViewport(const Viewport& viewport);
    void setScissorRect(const ScissorRect& scissor);

    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance);
    void drawIndirect(Buffer* buffer, uint64_t offset);
    void drawIndexedIndirect(Buffer* buffer, uint64_t offset);

private:
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    Device* m_device = nullptr;
    CommandEncoder* m_commandEncoder = nullptr;
};

} // namespace gfx::backend::vulkan::core

#endif // GFX_VULKAN_RENDERPASSENCODER_H