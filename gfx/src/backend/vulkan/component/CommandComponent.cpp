#include "CommandComponent.h"

#include "common/Logger.h"

#include "backend/vulkan/common/Common.h"
#include "backend/vulkan/converter/Conversions.h"
#include "backend/vulkan/validator/Validations.h"

#include "backend/vulkan/core/command/CommandEncoder.h"
#include "backend/vulkan/core/command/ComputePassEncoder.h"
#include "backend/vulkan/core/command/RenderPassEncoder.h"
#include "backend/vulkan/core/compute/ComputePipeline.h"
#include "backend/vulkan/core/query/QuerySet.h"
#include "backend/vulkan/core/render/Framebuffer.h"
#include "backend/vulkan/core/render/RenderPass.h"
#include "backend/vulkan/core/render/RenderPipeline.h"
#include "backend/vulkan/core/resource/BindGroup.h"
#include "backend/vulkan/core/resource/Buffer.h"
#include "backend/vulkan/core/resource/Texture.h"
#include "backend/vulkan/core/system/Device.h"

#include <stdexcept>

namespace gfx::backend::vulkan::component {

// CommandEncoder functions
GfxResult CommandComponent::deviceCreateCommandEncoder(GfxDevice device, const GfxCommandEncoderDescriptor* descriptor, GfxCommandEncoder* outEncoder) const
{
    GfxResult validationResult = validator::validateDeviceCreateCommandEncoder(device, descriptor, outEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* dev = converter::toNative<core::Device>(device);
        auto* encoder = new core::CommandEncoder(dev);
        *outEncoder = converter::toGfx<GfxCommandEncoder>(encoder);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create command encoder: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult CommandComponent::commandEncoderDestroy(GfxCommandEncoder commandEncoder) const
{
    GfxResult validationResult = validator::validateCommandEncoderDestroy(commandEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::CommandEncoder>(commandEncoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::commandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder, const GfxRenderPassBeginDescriptor* beginDescriptor, GfxRenderPassEncoder* outRenderPass) const
{
    GfxResult validationResult = validator::validateCommandEncoderBeginRenderPass(commandEncoder, beginDescriptor, outRenderPass);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* renderPass = converter::toNative<core::RenderPass>(beginDescriptor->renderPass);
    auto* framebuffer = converter::toNative<core::Framebuffer>(beginDescriptor->framebuffer);
    auto beginInfo = converter::gfxRenderPassBeginDescriptorToBeginInfo(beginDescriptor);
    auto* renderPassEncoder = new core::RenderPassEncoder(encoderPtr, renderPass, framebuffer, beginInfo);
    *outRenderPass = converter::toGfx<GfxRenderPassEncoder>(renderPassEncoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::commandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const GfxComputePassBeginDescriptor* beginDescriptor, GfxComputePassEncoder* outComputePass) const
{
    GfxResult validationResult = validator::validateCommandEncoderBeginComputePass(commandEncoder, beginDescriptor, outComputePass);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto createInfo = converter::gfxComputePassBeginDescriptorToCreateInfo(beginDescriptor);
    auto* computePassEncoder = new core::ComputePassEncoder(encoderPtr, createInfo);
    *outComputePass = converter::toGfx<GfxComputePassEncoder>(computePassEncoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::commandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder, const GfxCopyBufferToBufferDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderCopyBufferToBuffer(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* enc = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcBuf = converter::toNative<core::Buffer>(descriptor->source);
    auto* dstBuf = converter::toNative<core::Buffer>(descriptor->destination);

    enc->copyBufferToBuffer(srcBuf, descriptor->sourceOffset, dstBuf, descriptor->destinationOffset, descriptor->size);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder, const GfxCopyBufferToTextureDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderCopyBufferToTexture(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* enc = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcBuf = converter::toNative<core::Buffer>(descriptor->source);
    auto* dstTex = converter::toNative<core::Texture>(descriptor->destination);

    VkOffset3D vkOrigin = converter::gfxOrigin3DToVkOffset3D(&descriptor->origin);
    VkExtent3D vkExtent = converter::gfxExtent3DToVkExtent3D(&descriptor->extent);
    VkImageLayout vkLayout = converter::gfxLayoutToVkImageLayout(descriptor->finalLayout);

    enc->copyBufferToTexture(srcBuf, descriptor->sourceOffset, dstTex, vkOrigin, vkExtent, descriptor->mipLevel, vkLayout);

    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder, const GfxCopyTextureToBufferDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderCopyTextureToBuffer(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* enc = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcTex = converter::toNative<core::Texture>(descriptor->source);
    auto* dstBuf = converter::toNative<core::Buffer>(descriptor->destination);

    VkOffset3D vkOrigin = converter::gfxOrigin3DToVkOffset3D(&descriptor->origin);
    VkExtent3D vkExtent = converter::gfxExtent3DToVkExtent3D(&descriptor->extent);
    VkImageLayout vkLayout = converter::gfxLayoutToVkImageLayout(descriptor->finalLayout);

    enc->copyTextureToBuffer(srcTex, vkOrigin, descriptor->mipLevel, dstBuf, descriptor->destinationOffset, vkExtent, vkLayout);

    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::commandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder, const GfxCopyTextureToTextureDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderCopyTextureToTexture(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* enc = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcTex = converter::toNative<core::Texture>(descriptor->source);
    auto* dstTex = converter::toNative<core::Texture>(descriptor->destination);

    VkOffset3D vkSrcOrigin = converter::gfxOrigin3DToVkOffset3D(&descriptor->sourceOrigin);
    VkOffset3D vkDstOrigin = converter::gfxOrigin3DToVkOffset3D(&descriptor->destinationOrigin);
    VkExtent3D vkExtent = converter::gfxExtent3DToVkExtent3D(&descriptor->extent);
    VkImageLayout vkSrcLayout = converter::gfxLayoutToVkImageLayout(descriptor->sourceFinalLayout);
    VkImageLayout vkDstLayout = converter::gfxLayoutToVkImageLayout(descriptor->destinationFinalLayout);

    enc->copyTextureToTexture(srcTex, vkSrcOrigin, descriptor->sourceMipLevel, vkSrcLayout,
        dstTex, vkDstOrigin, descriptor->destinationMipLevel, vkDstLayout,
        vkExtent);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::commandEncoderBlitTextureToTexture(GfxCommandEncoder commandEncoder, const GfxBlitTextureToTextureDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderBlitTextureToTexture(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* enc = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcTex = converter::toNative<core::Texture>(descriptor->source);
    auto* dstTex = converter::toNative<core::Texture>(descriptor->destination);

    VkOffset3D vkSrcOrigin = converter::gfxOrigin3DToVkOffset3D(&descriptor->sourceOrigin);
    VkExtent3D vkSrcExtent = converter::gfxExtent3DToVkExtent3D(&descriptor->sourceExtent);
    VkOffset3D vkDstOrigin = converter::gfxOrigin3DToVkOffset3D(&descriptor->destinationOrigin);
    VkExtent3D vkDstExtent = converter::gfxExtent3DToVkExtent3D(&descriptor->destinationExtent);
    VkFilter vkFilter = converter::gfxFilterToVkFilter(descriptor->filter);
    VkImageLayout vkSrcLayout = converter::gfxLayoutToVkImageLayout(descriptor->sourceFinalLayout);
    VkImageLayout vkDstLayout = converter::gfxLayoutToVkImageLayout(descriptor->destinationFinalLayout);

    enc->blitTextureToTexture(srcTex, vkSrcOrigin, vkSrcExtent, descriptor->sourceMipLevel, vkSrcLayout,
        dstTex, vkDstOrigin, vkDstExtent, descriptor->destinationMipLevel, vkDstLayout,
        vkFilter);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::commandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder, const GfxPipelineBarrierDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderPipelineBarrier(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);

    // Convert GFX barriers to internal Vulkan barriers
    std::vector<core::MemoryBarrier> internalMemBarriers;
    internalMemBarriers.reserve(descriptor->memoryBarrierCount);
    for (uint32_t i = 0; i < descriptor->memoryBarrierCount; ++i) {
        internalMemBarriers.push_back(converter::gfxMemoryBarrierToMemoryBarrier(descriptor->memoryBarriers[i]));
    }

    std::vector<core::BufferBarrier> internalBufBarriers;
    internalBufBarriers.reserve(descriptor->bufferBarrierCount);
    for (uint32_t i = 0; i < descriptor->bufferBarrierCount; ++i) {
        internalBufBarriers.push_back(converter::gfxBufferBarrierToBufferBarrier(descriptor->bufferBarriers[i]));
    }

    std::vector<core::TextureBarrier> internalTexBarriers;
    internalTexBarriers.reserve(descriptor->textureBarrierCount);
    for (uint32_t i = 0; i < descriptor->textureBarrierCount; ++i) {
        internalTexBarriers.push_back(converter::gfxTextureBarrierToTextureBarrier(descriptor->textureBarriers[i]));
    }

    encoder->pipelineBarrier(
        internalMemBarriers.data(), static_cast<uint32_t>(internalMemBarriers.size()),
        internalBufBarriers.data(), static_cast<uint32_t>(internalBufBarriers.size()),
        internalTexBarriers.data(), static_cast<uint32_t>(internalTexBarriers.size()));
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::commandEncoderGenerateMipmaps(GfxCommandEncoder commandEncoder, GfxTexture texture) const
{
    GfxResult validationResult = validator::validateCommandEncoderGenerateMipmaps(commandEncoder, texture);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* tex = converter::toNative<core::Texture>(texture);
    tex->generateMipmaps(encoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::commandEncoderGenerateMipmapsRange(GfxCommandEncoder commandEncoder, GfxTexture texture,
    uint32_t baseMipLevel, uint32_t levelCount) const
{
    GfxResult validationResult = validator::validateCommandEncoderGenerateMipmapsRange(commandEncoder, texture);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* tex = converter::toNative<core::Texture>(texture);
    tex->generateMipmapsRange(encoder, baseMipLevel, levelCount);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::commandEncoderWriteTimestamp(GfxCommandEncoder commandEncoder, GfxQuerySet querySet, uint32_t queryIndex) const
{
    GfxResult validationResult = validator::validateCommandEncoderWriteTimestamp(commandEncoder, querySet);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* query = converter::toNative<core::QuerySet>(querySet);
    encoder->writeTimestamp(query->handle(), queryIndex);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::commandEncoderResolveQuerySet(GfxCommandEncoder commandEncoder, GfxQuerySet querySet, uint32_t firstQuery, uint32_t queryCount, GfxBuffer destinationBuffer, uint64_t destinationOffset) const
{
    GfxResult validationResult = validator::validateCommandEncoderResolveQuerySet(commandEncoder, querySet, destinationBuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* query = converter::toNative<core::QuerySet>(querySet);
    auto* buffer = converter::toNative<core::Buffer>(destinationBuffer);
    encoder->resolveQuerySet(query->handle(), firstQuery, queryCount, buffer->handle(), destinationOffset);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::commandEncoderEnd(GfxCommandEncoder commandEncoder) const
{
    GfxResult validationResult = validator::validateCommandEncoderEnd(commandEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);
    encoder->end();
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::commandEncoderBegin(GfxCommandEncoder commandEncoder) const
{
    GfxResult validationResult = validator::validateCommandEncoderBegin(commandEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);
    encoder->reset();
    return GFX_RESULT_SUCCESS;
}

// RenderPassEncoder functions
GfxResult CommandComponent::renderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetPipeline(renderPassEncoder, pipeline);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* pipe = converter::toNative<core::RenderPipeline>(pipeline);
    rpe->setPipeline(pipe);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetBindGroup(renderPassEncoder, bindGroup);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* bg = converter::toNative<core::BindGroup>(bindGroup);
    rpe->setBindGroup(index, bg, dynamicOffsets, dynamicOffsetCount);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetVertexBuffer(renderPassEncoder, buffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* buf = converter::toNative<core::Buffer>(buffer);
    rpe->setVertexBuffer(slot, buf, offset);

    (void)size;
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetIndexBuffer(renderPassEncoder, buffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* buf = converter::toNative<core::Buffer>(buffer);
    VkIndexType indexType = converter::gfxIndexFormatToVkIndexType(format);
    rpe->setIndexBuffer(buf, indexType, offset);

    (void)size;
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetViewport(renderPassEncoder, viewport);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    core::Viewport vkViewport = converter::gfxViewportToViewport(viewport);
    rpe->setViewport(vkViewport);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetScissorRect(renderPassEncoder, scissor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    core::ScissorRect vkScissor = converter::gfxScissorRectToScissorRect(scissor);
    rpe->setScissorRect(vkScissor);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderDraw(renderPassEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    rpe->draw(vertexCount, instanceCount, firstVertex, firstInstance);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderDrawIndexed(renderPassEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    rpe->drawIndexed(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderDrawIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderDrawIndirect(renderPassEncoder, indirectBuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* buffer = converter::toNative<core::Buffer>(indirectBuffer);
    rpe->drawIndirect(buffer, indirectOffset);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderDrawIndexedIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderDrawIndexedIndirect(renderPassEncoder, indirectBuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* buffer = converter::toNative<core::Buffer>(indirectBuffer);
    rpe->drawIndexedIndirect(buffer, indirectOffset);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderBeginOcclusionQuery(GfxRenderPassEncoder renderPassEncoder, GfxQuerySet querySet, uint32_t queryIndex) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderBeginOcclusionQuery(renderPassEncoder, querySet);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoder = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* query = converter::toNative<core::QuerySet>(querySet);
    encoder->beginOcclusionQuery(query->handle(), queryIndex);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderEndOcclusionQuery(GfxRenderPassEncoder renderPassEncoder) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderEndOcclusionQuery(renderPassEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoder = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    encoder->endOcclusionQuery();
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderEnd(renderPassEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    delete rpe;
    return GFX_RESULT_SUCCESS;
}

// ComputePassEncoder functions
GfxResult CommandComponent::computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline) const
{
    GfxResult validationResult = validator::validateComputePassEncoderSetPipeline(computePassEncoder, pipeline);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* cpe = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    auto* pipe = converter::toNative<core::ComputePipeline>(pipeline);
    cpe->setPipeline(pipe);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    GfxResult validationResult = validator::validateComputePassEncoderSetBindGroup(computePassEncoder, bindGroup);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* cpe = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    auto* bg = converter::toNative<core::BindGroup>(bindGroup);
    cpe->setBindGroup(index, bg, dynamicOffsets, dynamicOffsetCount);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::computePassEncoderDispatch(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) const
{
    GfxResult validationResult = validator::validateComputePassEncoderDispatch(computePassEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* cpe = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    cpe->dispatchWorkgroups(workgroupCountX, workgroupCountY, workgroupCountZ);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::computePassEncoderDispatchIndirect(GfxComputePassEncoder computePassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const
{
    GfxResult validationResult = validator::validateComputePassEncoderDispatchIndirect(computePassEncoder, indirectBuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* cpe = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    auto* buffer = converter::toNative<core::Buffer>(indirectBuffer);
    cpe->dispatchIndirect(buffer, indirectOffset);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::computePassEncoderEnd(GfxComputePassEncoder computePassEncoder) const
{
    GfxResult validationResult = validator::validateComputePassEncoderEnd(computePassEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* cpe = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    delete cpe;
    return GFX_RESULT_SUCCESS;
}

} // namespace gfx::backend::vulkan::component
