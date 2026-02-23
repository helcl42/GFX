#include "CommandComponent.h"

#include "common/Logger.h"

#include "../common/Common.h"
#include "../converter/Conversions.h"
#include "../validator/Validations.h"

#include "../core/command/CommandEncoder.h"
#include "../core/command/ComputePassEncoder.h"
#include "../core/command/RenderPassEncoder.h"
#include "../core/compute/ComputePipeline.h"
#include "../core/query/QuerySet.h"
#include "../core/render/RenderPipeline.h"
#include "../core/resource/BindGroup.h"
#include "../core/resource/Buffer.h"
#include "../core/resource/Texture.h"
#include "../core/system/Device.h"

#include <stdexcept>

namespace gfx::backend::webgpu::component {

// CommandEncoder functions
GfxResult CommandComponent::deviceCreateCommandEncoder(GfxDevice device, const GfxCommandEncoderDescriptor* descriptor, GfxCommandEncoder* outEncoder) const
{
    GfxResult validationResult = validator::validateDeviceCreateCommandEncoder(device, descriptor, outEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUCommandEncoderCreateInfo(descriptor);
        auto* encoder = new core::CommandEncoder(devicePtr, createInfo);
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

    auto* encoderPtr = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcPtr = converter::toNative<core::Buffer>(descriptor->source);
    auto* dstPtr = converter::toNative<core::Buffer>(descriptor->destination);

    encoderPtr->copyBufferToBuffer(srcPtr, descriptor->sourceOffset, dstPtr, descriptor->destinationOffset, descriptor->size);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder, const GfxCopyBufferToTextureDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderCopyBufferToTexture(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcPtr = converter::toNative<core::Buffer>(descriptor->source);
    auto* dstPtr = converter::toNative<core::Texture>(descriptor->destination);

    WGPUOrigin3D wgpuOrigin = converter::gfxOrigin3DToWGPUOrigin3D(&descriptor->origin);
    WGPUExtent3D wgpuExtent = converter::gfxExtent3DToWGPUExtent3D(&descriptor->extent);

    encoderPtr->copyBufferToTexture(srcPtr, descriptor->sourceOffset, dstPtr, wgpuOrigin, wgpuExtent, descriptor->mipLevel);

    (void)descriptor->finalLayout; // WebGPU handles layout transitions automatically
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder, const GfxCopyTextureToBufferDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderCopyTextureToBuffer(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcPtr = converter::toNative<core::Texture>(descriptor->source);
    auto* dstPtr = converter::toNative<core::Buffer>(descriptor->destination);

    WGPUOrigin3D wgpuOrigin = converter::gfxOrigin3DToWGPUOrigin3D(&descriptor->origin);
    WGPUExtent3D wgpuExtent = converter::gfxExtent3DToWGPUExtent3D(&descriptor->extent);

    encoderPtr->copyTextureToBuffer(srcPtr, wgpuOrigin, descriptor->mipLevel, dstPtr, descriptor->destinationOffset, wgpuExtent);

    (void)descriptor->finalLayout; // WebGPU handles layout transitions automatically
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::commandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder, const GfxCopyTextureToTextureDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderCopyTextureToTexture(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcPtr = converter::toNative<core::Texture>(descriptor->source);
    auto* dstPtr = converter::toNative<core::Texture>(descriptor->destination);

    WGPUOrigin3D wgpuSrcOrigin = converter::gfxOrigin3DToWGPUOrigin3D(&descriptor->sourceOrigin);
    WGPUOrigin3D wgpuDstOrigin = converter::gfxOrigin3DToWGPUOrigin3D(&descriptor->destinationOrigin);
    WGPUExtent3D wgpuExtent = converter::gfxExtent3DToWGPUExtent3D(&descriptor->extent);

    encoderPtr->copyTextureToTexture(srcPtr, wgpuSrcOrigin, descriptor->sourceMipLevel, dstPtr, wgpuDstOrigin, descriptor->destinationMipLevel, wgpuExtent);

    (void)descriptor->sourceFinalLayout; // WebGPU handles layout transitions automatically
    (void)descriptor->destinationFinalLayout;
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::commandEncoderBlitTextureToTexture(GfxCommandEncoder commandEncoder, const GfxBlitTextureToTextureDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderBlitTextureToTexture(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcTexture = converter::toNative<core::Texture>(descriptor->source);
    auto* dstTexture = converter::toNative<core::Texture>(descriptor->destination);

    WGPUOrigin3D wgpuSrcOrigin = converter::gfxOrigin3DToWGPUOrigin3D(&descriptor->sourceOrigin);
    WGPUOrigin3D wgpuDstOrigin = converter::gfxOrigin3DToWGPUOrigin3D(&descriptor->destinationOrigin);
    WGPUExtent3D wgpuSrcExtent = converter::gfxExtent3DToWGPUExtent3D(&descriptor->sourceExtent);
    WGPUExtent3D wgpuDstExtent = converter::gfxExtent3DToWGPUExtent3D(&descriptor->destinationExtent);
    WGPUFilterMode wgpuFilter = converter::gfxFilterModeToWGPU(descriptor->filter);

    encoder->blitTextureToTexture(srcTexture, wgpuSrcOrigin, wgpuSrcExtent, descriptor->sourceMipLevel, dstTexture, wgpuDstOrigin, wgpuDstExtent, descriptor->destinationMipLevel, wgpuFilter);

    // WebGPU handles layout transitions automatically
    (void)descriptor->sourceFinalLayout;
    (void)descriptor->destinationFinalLayout;
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::commandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder, const GfxPipelineBarrierDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderPipelineBarrier(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    // WebGPU handles synchronization and layout transitions automatically
    // This is a no-op for WebGPU backend
    (void)commandEncoder;
    (void)descriptor;
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

GfxResult CommandComponent::commandEncoderGenerateMipmapsRange(GfxCommandEncoder commandEncoder, GfxTexture texture, uint32_t baseMipLevel, uint32_t levelCount) const
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

    (void)commandEncoder; // Parameter unused - handled in queueSubmit
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::commandEncoderBegin(GfxCommandEncoder commandEncoder) const
{
    GfxResult validationResult = validator::validateCommandEncoderBegin(commandEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::CommandEncoder>(commandEncoder);

    // WebGPU encoders can't be reused after Finish() - recreate if needed
    if (!encoderPtr->recreateIfNeeded()) {
        gfx::common::Logger::instance().logError("[WebGPU ERROR] Failed to recreate command encoder");
        return GFX_RESULT_ERROR_UNKNOWN;
    }
    return GFX_RESULT_SUCCESS;
}

// RenderPassEncoder functions
GfxResult CommandComponent::renderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetPipeline(renderPassEncoder, pipeline);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* pipelinePtr = converter::toNative<core::RenderPipeline>(pipeline);

    encoderPtr->setPipeline(pipelinePtr->handle());
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetBindGroup(renderPassEncoder, bindGroup);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* bindGroupPtr = converter::toNative<core::BindGroup>(bindGroup);

    encoderPtr->setBindGroup(index, bindGroupPtr->handle(), dynamicOffsets, dynamicOffsetCount);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetVertexBuffer(renderPassEncoder, buffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* bufferPtr = converter::toNative<core::Buffer>(buffer);

    encoderPtr->setVertexBuffer(slot, bufferPtr, offset, size);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetIndexBuffer(renderPassEncoder, buffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* bufferPtr = converter::toNative<core::Buffer>(buffer);

    encoderPtr->setIndexBuffer(bufferPtr, converter::gfxIndexFormatToWGPU(format), offset, size);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetViewport(renderPassEncoder, viewport);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    encoderPtr->setViewport(viewport->x, viewport->y, viewport->width, viewport->height, viewport->minDepth, viewport->maxDepth);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetScissorRect(renderPassEncoder, scissor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    encoderPtr->setScissorRect(scissor->origin.x, scissor->origin.y, scissor->extent.width, scissor->extent.height);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderDraw(renderPassEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    encoderPtr->draw(vertexCount, instanceCount, firstVertex, firstInstance);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderDrawIndexed(renderPassEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    encoderPtr->drawIndexed(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderDrawIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderDrawIndirect(renderPassEncoder, indirectBuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* bufferPtr = converter::toNative<core::Buffer>(indirectBuffer);
    encoderPtr->drawIndirect(bufferPtr->handle(), indirectOffset);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::renderPassEncoderDrawIndexedIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderDrawIndexedIndirect(renderPassEncoder, indirectBuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* bufferPtr = converter::toNative<core::Buffer>(indirectBuffer);
    encoderPtr->drawIndexedIndirect(bufferPtr->handle(), indirectOffset);
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

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    delete encoderPtr;
    return GFX_RESULT_SUCCESS;
}

// ComputePassEncoder functions
GfxResult CommandComponent::computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline) const
{
    GfxResult validationResult = validator::validateComputePassEncoderSetPipeline(computePassEncoder, pipeline);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    auto* pipelinePtr = converter::toNative<core::ComputePipeline>(pipeline);

    encoderPtr->setPipeline(pipelinePtr->handle());
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    GfxResult validationResult = validator::validateComputePassEncoderSetBindGroup(computePassEncoder, bindGroup);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    auto* bindGroupPtr = converter::toNative<core::BindGroup>(bindGroup);

    encoderPtr->setBindGroup(index, bindGroupPtr->handle(), dynamicOffsets, dynamicOffsetCount);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::computePassEncoderDispatch(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) const
{
    GfxResult validationResult = validator::validateComputePassEncoderDispatch(computePassEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    encoderPtr->dispatchWorkgroups(workgroupCountX, workgroupCountY, workgroupCountZ);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::computePassEncoderDispatchIndirect(GfxComputePassEncoder computePassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const
{
    GfxResult validationResult = validator::validateComputePassEncoderDispatchIndirect(computePassEncoder, indirectBuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    auto* bufferPtr = converter::toNative<core::Buffer>(indirectBuffer);
    encoderPtr->dispatchIndirect(bufferPtr->handle(), indirectOffset);
    return GFX_RESULT_SUCCESS;
}

GfxResult CommandComponent::computePassEncoderEnd(GfxComputePassEncoder computePassEncoder) const
{
    GfxResult validationResult = validator::validateComputePassEncoderEnd(computePassEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    delete encoderPtr;
    return GFX_RESULT_SUCCESS;
}

} // namespace gfx::backend::webgpu::component
