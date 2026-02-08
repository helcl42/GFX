#include "Backend.h"

namespace gfx::backend::webgpu {

// Instance functions
GfxResult Backend::createInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance) const
{
    return m_systemComponent.createInstance(descriptor, outInstance);
}

GfxResult Backend::instanceDestroy(GfxInstance instance) const
{
    return m_systemComponent.instanceDestroy(instance);
}

GfxResult Backend::instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter) const
{
    return m_systemComponent.instanceRequestAdapter(instance, descriptor, outAdapter);
}

GfxResult Backend::instanceEnumerateAdapters(GfxInstance instance, uint32_t* adapterCount, GfxAdapter* adapters) const
{
    return m_systemComponent.instanceEnumerateAdapters(instance, adapterCount, adapters);
}

GfxResult Backend::enumerateInstanceExtensions(uint32_t* extensionCount, const char** extensionNames) const
{
    return m_systemComponent.enumerateInstanceExtensions(extensionCount, extensionNames);
}

// Adapter functions
GfxResult Backend::adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice) const
{
    return m_systemComponent.adapterCreateDevice(adapter, descriptor, outDevice);
}

GfxResult Backend::adapterGetInfo(GfxAdapter adapter, GfxAdapterInfo* outInfo) const
{
    return m_systemComponent.adapterGetInfo(adapter, outInfo);
}

GfxResult Backend::adapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits) const
{
    return m_systemComponent.adapterGetLimits(adapter, outLimits);
}

GfxResult Backend::adapterEnumerateQueueFamilies(GfxAdapter adapter, uint32_t* queueFamilyCount, GfxQueueFamilyProperties* queueFamilies) const
{
    return m_systemComponent.adapterEnumerateQueueFamilies(adapter, queueFamilyCount, queueFamilies);
}

GfxResult Backend::adapterGetQueueFamilySurfaceSupport(GfxAdapter adapter, uint32_t queueFamilyIndex, GfxSurface surface, bool* outSupported) const
{
    return m_systemComponent.adapterGetQueueFamilySurfaceSupport(adapter, queueFamilyIndex, surface, outSupported);
}

GfxResult Backend::adapterEnumerateExtensions(GfxAdapter adapter, uint32_t* extensionCount, const char** extensionNames) const
{
    return m_systemComponent.adapterEnumerateExtensions(adapter, extensionCount, extensionNames);
}

// Device functions
GfxResult Backend::deviceDestroy(GfxDevice device) const
{
    return m_systemComponent.deviceDestroy(device);
}

GfxResult Backend::deviceGetQueue(GfxDevice device, GfxQueue* outQueue) const
{
    return m_systemComponent.deviceGetQueue(device, outQueue);
}

GfxResult Backend::deviceGetQueueByIndex(GfxDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, GfxQueue* outQueue) const
{
    return m_systemComponent.deviceGetQueueByIndex(device, queueFamilyIndex, queueIndex, outQueue);
}

GfxResult Backend::deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface) const
{
    return m_presentationComponent.deviceCreateSurface(device, descriptor, outSurface);
}

GfxResult Backend::deviceCreateSwapchain(GfxDevice device, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain) const
{
    return m_presentationComponent.deviceCreateSwapchain(device, descriptor, outSwapchain);
}

GfxResult Backend::deviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer) const
{
    return m_resourceComponent.deviceCreateBuffer(device, descriptor, outBuffer);
}

GfxResult Backend::deviceImportBuffer(GfxDevice device, const GfxBufferImportDescriptor* descriptor, GfxBuffer* outBuffer) const
{
    return m_resourceComponent.deviceImportBuffer(device, descriptor, outBuffer);
}

GfxResult Backend::deviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture) const
{
    return m_resourceComponent.deviceCreateTexture(device, descriptor, outTexture);
}

GfxResult Backend::deviceImportTexture(GfxDevice device, const GfxTextureImportDescriptor* descriptor, GfxTexture* outTexture) const
{
    return m_resourceComponent.deviceImportTexture(device, descriptor, outTexture);
}

GfxResult Backend::deviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler) const
{
    return m_resourceComponent.deviceCreateSampler(device, descriptor, outSampler);
}

GfxResult Backend::deviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader) const
{
    return m_resourceComponent.deviceCreateShader(device, descriptor, outShader);
}

GfxResult Backend::deviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout) const
{
    return m_resourceComponent.deviceCreateBindGroupLayout(device, descriptor, outLayout);
}

GfxResult Backend::deviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup) const
{
    return m_resourceComponent.deviceCreateBindGroup(device, descriptor, outBindGroup);
}

GfxResult Backend::deviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline) const
{
    return m_renderComponent.deviceCreateRenderPipeline(device, descriptor, outPipeline);
}

GfxResult Backend::deviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline) const
{
    return m_computeComponent.deviceCreateComputePipeline(device, descriptor, outPipeline);
}

GfxResult Backend::deviceCreateRenderPass(GfxDevice device, const GfxRenderPassDescriptor* descriptor, GfxRenderPass* outRenderPass) const
{
    return m_renderComponent.deviceCreateRenderPass(device, descriptor, outRenderPass);
}

GfxResult Backend::deviceCreateFramebuffer(GfxDevice device, const GfxFramebufferDescriptor* descriptor, GfxFramebuffer* outFramebuffer) const
{
    return m_renderComponent.deviceCreateFramebuffer(device, descriptor, outFramebuffer);
}

GfxResult Backend::deviceCreateCommandEncoder(GfxDevice device, const GfxCommandEncoderDescriptor* descriptor, GfxCommandEncoder* outEncoder) const
{
    return m_commandComponent.deviceCreateCommandEncoder(device, descriptor, outEncoder);
}

GfxResult Backend::deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence) const
{
    return m_syncComponent.deviceCreateFence(device, descriptor, outFence);
}

GfxResult Backend::deviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore) const
{
    return m_syncComponent.deviceCreateSemaphore(device, descriptor, outSemaphore);
}

GfxResult Backend::deviceCreateQuerySet(GfxDevice device, const GfxQuerySetDescriptor* descriptor, GfxQuerySet* outQuerySet) const
{
    return m_queryComponent.deviceCreateQuerySet(device, descriptor, outQuerySet);
}

GfxResult Backend::deviceWaitIdle(GfxDevice device) const
{
    return m_systemComponent.deviceWaitIdle(device);
}

GfxResult Backend::deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits) const
{
    return m_systemComponent.deviceGetLimits(device, outLimits);
}

GfxResult Backend::deviceSupportsShaderFormat(GfxDevice device, GfxShaderSourceType format, bool* outSupported) const
{
    return m_systemComponent.deviceSupportsShaderFormat(device, format, outSupported);
}

// Surface functions
GfxResult Backend::surfaceDestroy(GfxSurface surface) const
{
    return m_presentationComponent.surfaceDestroy(surface);
}

GfxResult Backend::surfaceGetInfo(GfxSurface surface, GfxSurfaceInfo* outInfo) const
{
    return m_presentationComponent.surfaceGetInfo(surface, outInfo);
}

GfxResult Backend::surfaceEnumerateSupportedFormats(GfxSurface surface, uint32_t* formatCount, GfxTextureFormat* formats) const
{
    return m_presentationComponent.surfaceEnumerateSupportedFormats(surface, formatCount, formats);
}

GfxResult Backend::surfaceEnumerateSupportedPresentModes(GfxSurface surface, uint32_t* presentModeCount, GfxPresentMode* presentModes) const
{
    return m_presentationComponent.surfaceEnumerateSupportedPresentModes(surface, presentModeCount, presentModes);
}

// Swapchain functions
GfxResult Backend::swapchainDestroy(GfxSwapchain swapchain) const
{
    return m_presentationComponent.swapchainDestroy(swapchain);
}

GfxResult Backend::swapchainGetInfo(GfxSwapchain swapchain, GfxSwapchainInfo* outInfo) const
{
    return m_presentationComponent.swapchainGetInfo(swapchain, outInfo);
}

GfxResult Backend::swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const
{
    return m_presentationComponent.swapchainAcquireNextImage(swapchain, timeoutNs, imageAvailableSemaphore, fence, outImageIndex);
}

GfxResult Backend::swapchainGetTextureView(GfxSwapchain swapchain, uint32_t imageIndex, GfxTextureView* outView) const
{
    return m_presentationComponent.swapchainGetTextureView(swapchain, imageIndex, outView);
}

GfxResult Backend::swapchainGetCurrentTextureView(GfxSwapchain swapchain, GfxTextureView* outView) const
{
    return m_presentationComponent.swapchainGetCurrentTextureView(swapchain, outView);
}

GfxResult Backend::swapchainPresent(GfxSwapchain swapchain, const GfxPresentDescriptor* presentDescriptor) const
{
    return m_presentationComponent.swapchainPresent(swapchain, presentDescriptor);
}

// Buffer functions
GfxResult Backend::bufferDestroy(GfxBuffer buffer) const
{
    return m_resourceComponent.bufferDestroy(buffer);
}

GfxResult Backend::bufferGetInfo(GfxBuffer buffer, GfxBufferInfo* outInfo) const
{
    return m_resourceComponent.bufferGetInfo(buffer, outInfo);
}

GfxResult Backend::bufferGetNativeHandle(GfxBuffer buffer, void** outHandle) const
{
    return m_resourceComponent.bufferGetNativeHandle(buffer, outHandle);
}

GfxResult Backend::bufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer) const
{
    return m_resourceComponent.bufferMap(buffer, offset, size, outMappedPointer);
}

GfxResult Backend::bufferUnmap(GfxBuffer buffer) const
{
    return m_resourceComponent.bufferUnmap(buffer);
}

GfxResult Backend::bufferFlushMappedRange(GfxBuffer buffer, uint64_t offset, uint64_t size) const
{
    return m_resourceComponent.bufferFlushMappedRange(buffer, offset, size);
}

GfxResult Backend::bufferInvalidateMappedRange(GfxBuffer buffer, uint64_t offset, uint64_t size) const
{
    return m_resourceComponent.bufferInvalidateMappedRange(buffer, offset, size);
}

// Texture functions
GfxResult Backend::textureDestroy(GfxTexture texture) const
{
    return m_resourceComponent.textureDestroy(texture);
}

GfxResult Backend::textureGetInfo(GfxTexture texture, GfxTextureInfo* outInfo) const
{
    return m_resourceComponent.textureGetInfo(texture, outInfo);
}

GfxResult Backend::textureGetNativeHandle(GfxTexture texture, void** outHandle) const
{
    return m_resourceComponent.textureGetNativeHandle(texture, outHandle);
}

GfxResult Backend::textureGetLayout(GfxTexture texture, GfxTextureLayout* outLayout) const
{
    return m_resourceComponent.textureGetLayout(texture, outLayout);
}

GfxResult Backend::textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView) const
{
    return m_resourceComponent.textureCreateView(texture, descriptor, outView);
}

// TextureView functions
GfxResult Backend::textureViewDestroy(GfxTextureView textureView) const
{
    return m_resourceComponent.textureViewDestroy(textureView);
}

// Sampler functions
GfxResult Backend::samplerDestroy(GfxSampler sampler) const
{
    return m_resourceComponent.samplerDestroy(sampler);
}

// Shader functions
GfxResult Backend::shaderDestroy(GfxShader shader) const
{
    return m_resourceComponent.shaderDestroy(shader);
}

// BindGroupLayout functions
GfxResult Backend::bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout) const
{
    return m_resourceComponent.bindGroupLayoutDestroy(bindGroupLayout);
}

// BindGroup functions
GfxResult Backend::bindGroupDestroy(GfxBindGroup bindGroup) const
{
    return m_resourceComponent.bindGroupDestroy(bindGroup);
}

// RenderPipeline functions
GfxResult Backend::renderPipelineDestroy(GfxRenderPipeline renderPipeline) const
{
    return m_renderComponent.renderPipelineDestroy(renderPipeline);
}

// ComputePipeline functions
GfxResult Backend::computePipelineDestroy(GfxComputePipeline computePipeline) const
{
    return m_computeComponent.computePipelineDestroy(computePipeline);
}

GfxResult Backend::renderPassDestroy(GfxRenderPass renderPass) const
{
    return m_renderComponent.renderPassDestroy(renderPass);
}

GfxResult Backend::framebufferDestroy(GfxFramebuffer framebuffer) const
{
    return m_renderComponent.framebufferDestroy(framebuffer);
}

GfxResult Backend::querySetDestroy(GfxQuerySet querySet) const
{
    return m_queryComponent.querySetDestroy(querySet);
}

// Queue functions
GfxResult Backend::queueSubmit(GfxQueue queue, const GfxSubmitDescriptor* submitInfo) const
{
    return m_systemComponent.queueSubmit(queue, submitInfo);
}

GfxResult Backend::queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size) const
{
    return m_systemComponent.queueWriteBuffer(queue, buffer, offset, data, size);
}

GfxResult Backend::queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel, const void* data, uint64_t dataSize, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
{
    return m_systemComponent.queueWriteTexture(queue, texture, origin, mipLevel, data, dataSize, extent, finalLayout);
}

GfxResult Backend::queueWaitIdle(GfxQueue queue) const
{
    return m_systemComponent.queueWaitIdle(queue);
}

// CommandEncoder functions
GfxResult Backend::commandEncoderDestroy(GfxCommandEncoder commandEncoder) const
{
    return m_commandComponent.commandEncoderDestroy(commandEncoder);
}

GfxResult Backend::commandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder, const GfxRenderPassBeginDescriptor* beginDescriptor, GfxRenderPassEncoder* outRenderPass) const
{
    return m_commandComponent.commandEncoderBeginRenderPass(commandEncoder, beginDescriptor, outRenderPass);
}

GfxResult Backend::commandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const GfxComputePassBeginDescriptor* beginDescriptor, GfxComputePassEncoder* outComputePass) const
{
    return m_commandComponent.commandEncoderBeginComputePass(commandEncoder, beginDescriptor, outComputePass);
}

GfxResult Backend::commandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder, const GfxCopyBufferToBufferDescriptor* descriptor) const
{
    return m_commandComponent.commandEncoderCopyBufferToBuffer(commandEncoder, descriptor);
}

GfxResult Backend::commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder, const GfxCopyBufferToTextureDescriptor* descriptor) const
{
    return m_commandComponent.commandEncoderCopyBufferToTexture(commandEncoder, descriptor);
}

GfxResult Backend::commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder, const GfxCopyTextureToBufferDescriptor* descriptor) const
{
    return m_commandComponent.commandEncoderCopyTextureToBuffer(commandEncoder, descriptor);
}

GfxResult Backend::commandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder, const GfxCopyTextureToTextureDescriptor* descriptor) const
{
    return m_commandComponent.commandEncoderCopyTextureToTexture(commandEncoder, descriptor);
}

GfxResult Backend::commandEncoderBlitTextureToTexture(GfxCommandEncoder commandEncoder, const GfxBlitTextureToTextureDescriptor* descriptor) const
{
    return m_commandComponent.commandEncoderBlitTextureToTexture(commandEncoder, descriptor);
}

GfxResult Backend::commandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder, const GfxPipelineBarrierDescriptor* descriptor) const
{
    return m_commandComponent.commandEncoderPipelineBarrier(commandEncoder, descriptor);
}

GfxResult Backend::commandEncoderGenerateMipmaps(GfxCommandEncoder commandEncoder, GfxTexture texture) const
{
    return m_commandComponent.commandEncoderGenerateMipmaps(commandEncoder, texture);
}

GfxResult Backend::commandEncoderGenerateMipmapsRange(GfxCommandEncoder commandEncoder, GfxTexture texture, uint32_t baseMipLevel, uint32_t levelCount) const
{
    return m_commandComponent.commandEncoderGenerateMipmapsRange(commandEncoder, texture, baseMipLevel, levelCount);
}

GfxResult Backend::commandEncoderWriteTimestamp(GfxCommandEncoder commandEncoder, GfxQuerySet querySet, uint32_t queryIndex) const
{
    return m_commandComponent.commandEncoderWriteTimestamp(commandEncoder, querySet, queryIndex);
}

GfxResult Backend::commandEncoderResolveQuerySet(GfxCommandEncoder commandEncoder, GfxQuerySet querySet, uint32_t firstQuery, uint32_t queryCount, GfxBuffer destinationBuffer, uint64_t destinationOffset) const
{
    return m_commandComponent.commandEncoderResolveQuerySet(commandEncoder, querySet, firstQuery, queryCount, destinationBuffer, destinationOffset);
}

GfxResult Backend::commandEncoderEnd(GfxCommandEncoder commandEncoder) const
{
    return m_commandComponent.commandEncoderEnd(commandEncoder);
}

GfxResult Backend::commandEncoderBegin(GfxCommandEncoder commandEncoder) const
{
    return m_commandComponent.commandEncoderBegin(commandEncoder);
}

// RenderPassEncoder functions
GfxResult Backend::renderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline) const
{
    return m_commandComponent.renderPassEncoderSetPipeline(renderPassEncoder, pipeline);
}

GfxResult Backend::renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    return m_commandComponent.renderPassEncoderSetBindGroup(renderPassEncoder, index, bindGroup, dynamicOffsets, dynamicOffsetCount);
}

GfxResult Backend::renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size) const
{
    return m_commandComponent.renderPassEncoderSetVertexBuffer(renderPassEncoder, slot, buffer, offset, size);
}

GfxResult Backend::renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size) const
{
    return m_commandComponent.renderPassEncoderSetIndexBuffer(renderPassEncoder, buffer, format, offset, size);
}

GfxResult Backend::renderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport) const
{
    return m_commandComponent.renderPassEncoderSetViewport(renderPassEncoder, viewport);
}

GfxResult Backend::renderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor) const
{
    return m_commandComponent.renderPassEncoderSetScissorRect(renderPassEncoder, scissor);
}

GfxResult Backend::renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
{
    return m_commandComponent.renderPassEncoderDraw(renderPassEncoder, vertexCount, instanceCount, firstVertex, firstInstance);
}

GfxResult Backend::renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) const
{
    return m_commandComponent.renderPassEncoderDrawIndexed(renderPassEncoder, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}

GfxResult Backend::renderPassEncoderDrawIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const
{
    return m_commandComponent.renderPassEncoderDrawIndirect(renderPassEncoder, indirectBuffer, indirectOffset);
}

GfxResult Backend::renderPassEncoderDrawIndexedIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const
{
    return m_commandComponent.renderPassEncoderDrawIndexedIndirect(renderPassEncoder, indirectBuffer, indirectOffset);
}

GfxResult Backend::renderPassEncoderBeginOcclusionQuery(GfxRenderPassEncoder renderPassEncoder, GfxQuerySet querySet, uint32_t queryIndex) const
{
    return m_commandComponent.renderPassEncoderBeginOcclusionQuery(renderPassEncoder, querySet, queryIndex);
}

GfxResult Backend::renderPassEncoderEndOcclusionQuery(GfxRenderPassEncoder renderPassEncoder) const
{
    return m_commandComponent.renderPassEncoderEndOcclusionQuery(renderPassEncoder);
}

GfxResult Backend::renderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder) const
{
    return m_commandComponent.renderPassEncoderEnd(renderPassEncoder);
}

// ComputePassEncoder functions
GfxResult Backend::computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline) const
{
    return m_commandComponent.computePassEncoderSetPipeline(computePassEncoder, pipeline);
}

GfxResult Backend::computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    return m_commandComponent.computePassEncoderSetBindGroup(computePassEncoder, index, bindGroup, dynamicOffsets, dynamicOffsetCount);
}

GfxResult Backend::computePassEncoderDispatch(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) const
{
    return m_commandComponent.computePassEncoderDispatch(computePassEncoder, workgroupCountX, workgroupCountY, workgroupCountZ);
}

GfxResult Backend::computePassEncoderDispatchIndirect(GfxComputePassEncoder computePassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const
{
    return m_commandComponent.computePassEncoderDispatchIndirect(computePassEncoder, indirectBuffer, indirectOffset);
}

GfxResult Backend::computePassEncoderEnd(GfxComputePassEncoder computePassEncoder) const
{
    return m_commandComponent.computePassEncoderEnd(computePassEncoder);
}

// Fence functions
GfxResult Backend::fenceDestroy(GfxFence fence) const
{
    return m_syncComponent.fenceDestroy(fence);
}

GfxResult Backend::fenceGetStatus(GfxFence fence, bool* isSignaled) const
{
    return m_syncComponent.fenceGetStatus(fence, isSignaled);
}

GfxResult Backend::fenceWait(GfxFence fence, uint64_t timeoutNs) const
{
    return m_syncComponent.fenceWait(fence, timeoutNs);
}

GfxResult Backend::fenceReset(GfxFence fence) const
{
    return m_syncComponent.fenceReset(fence);
}

// Semaphore functions
GfxResult Backend::semaphoreDestroy(GfxSemaphore semaphore) const
{
    return m_syncComponent.semaphoreDestroy(semaphore);
}

GfxResult Backend::semaphoreGetType(GfxSemaphore semaphore, GfxSemaphoreType* outType) const
{
    return m_syncComponent.semaphoreGetType(semaphore, outType);
}

GfxResult Backend::semaphoreSignal(GfxSemaphore semaphore, uint64_t value) const
{
    return m_syncComponent.semaphoreSignal(semaphore, value);
}

GfxResult Backend::semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs) const
{
    return m_syncComponent.semaphoreWait(semaphore, value, timeoutNs);
}

GfxResult Backend::semaphoreGetValue(GfxSemaphore semaphore, uint64_t* outValue) const
{
    return m_syncComponent.semaphoreGetValue(semaphore, outValue);
}

GfxAccessFlags Backend::getAccessFlagsForLayout(GfxTextureLayout layout) const
{
    return m_syncComponent.getAccessFlagsForLayout(layout);
}

} // namespace gfx::backend::webgpu