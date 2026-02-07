#ifndef GFX_BACKEND_WEBGPU_COMMAND_COMPONENT_H
#define GFX_BACKEND_WEBGPU_COMMAND_COMPONENT_H

#include <gfx/gfx.h>

namespace gfx::backend::webgpu::component {

class CommandComponent {
public:
    CommandComponent() = default;
    ~CommandComponent() = default;

    // Prevent copying
    CommandComponent(const CommandComponent&) = delete;
    CommandComponent& operator=(const CommandComponent&) = delete;

    // CommandEncoder functions
    GfxResult deviceCreateCommandEncoder(GfxDevice device, const GfxCommandEncoderDescriptor* descriptor, GfxCommandEncoder* outEncoder) const;
    GfxResult commandEncoderDestroy(GfxCommandEncoder commandEncoder) const;
    GfxResult commandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder, const GfxRenderPassBeginDescriptor* beginDescriptor, GfxRenderPassEncoder* outRenderPass) const;
    GfxResult commandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const GfxComputePassBeginDescriptor* beginDescriptor, GfxComputePassEncoder* outComputePass) const;
    GfxResult commandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder, const GfxCopyBufferToBufferDescriptor* descriptor) const;
    GfxResult commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder, const GfxCopyBufferToTextureDescriptor* descriptor) const;
    GfxResult commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder, const GfxCopyTextureToBufferDescriptor* descriptor) const;
    GfxResult commandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder, const GfxCopyTextureToTextureDescriptor* descriptor) const;
    GfxResult commandEncoderBlitTextureToTexture(GfxCommandEncoder commandEncoder, const GfxBlitTextureToTextureDescriptor* descriptor) const;
    GfxResult commandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder, const GfxPipelineBarrierDescriptor* descriptor) const;
    GfxResult commandEncoderGenerateMipmaps(GfxCommandEncoder commandEncoder, GfxTexture texture) const;
    GfxResult commandEncoderGenerateMipmapsRange(GfxCommandEncoder commandEncoder, GfxTexture texture, uint32_t baseMipLevel, uint32_t levelCount) const;
    GfxResult commandEncoderWriteTimestamp(GfxCommandEncoder commandEncoder, GfxQuerySet querySet, uint32_t queryIndex) const;
    GfxResult commandEncoderResolveQuerySet(GfxCommandEncoder commandEncoder, GfxQuerySet querySet, uint32_t firstQuery, uint32_t queryCount, GfxBuffer destinationBuffer, uint64_t destinationOffset) const;
    GfxResult commandEncoderEnd(GfxCommandEncoder commandEncoder) const;
    GfxResult commandEncoderBegin(GfxCommandEncoder commandEncoder) const;

    // RenderPassEncoder functions
    GfxResult renderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline) const;
    GfxResult renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const;
    GfxResult renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size) const;
    GfxResult renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size) const;
    GfxResult renderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport) const;
    GfxResult renderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor) const;
    GfxResult renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const;
    GfxResult renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) const;
    GfxResult renderPassEncoderDrawIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const;
    GfxResult renderPassEncoderDrawIndexedIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const;
    GfxResult renderPassEncoderBeginOcclusionQuery(GfxRenderPassEncoder renderPassEncoder, GfxQuerySet querySet, uint32_t queryIndex) const;
    GfxResult renderPassEncoderEndOcclusionQuery(GfxRenderPassEncoder renderPassEncoder) const;
    GfxResult renderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder) const;

    // ComputePassEncoder functions
    GfxResult computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline) const;
    GfxResult computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const;
    GfxResult computePassEncoderDispatch(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) const;
    GfxResult computePassEncoderDispatchIndirect(GfxComputePassEncoder computePassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const;
    GfxResult computePassEncoderEnd(GfxComputePassEncoder computePassEncoder) const;
};

} // namespace gfx::backend::webgpu::component

#endif // GFX_BACKEND_WEBGPU_COMMAND_COMPONENT_H
