#pragma once

#include "gfx/gfx.h"

namespace gfx::backend::vulkan::validator {

// ============================================================================
// Public validation interface (called by Backend)
// ============================================================================

// Combined validation functions (parameters + descriptors)
// ============================================================================

GfxResult validateCreateInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance);
GfxResult validateInstanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter);
GfxResult validateInstanceEnumerateAdapters(GfxInstance instance, uint32_t* adapterCount);
GfxResult validateAdapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice);
GfxResult validateAdapterGetInfo(GfxAdapter adapter, GfxAdapterInfo* outInfo);
GfxResult validateAdapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits);
GfxResult validateAdapterEnumerateQueueFamilies(GfxAdapter adapter, uint32_t* queueFamilyCount);
GfxResult validateAdapterGetQueueFamilySurfaceSupport(GfxAdapter adapter, GfxSurface surface, bool* outSupported);
GfxResult validateAdapterEnumerateExtensions(GfxAdapter adapter, uint32_t* extensionCount);
GfxResult validateDeviceGetQueue(GfxDevice device, GfxQueue* outQueue);
GfxResult validateDeviceGetQueueByIndex(GfxDevice device, GfxQueue* outQueue);
GfxResult validateDeviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface);
GfxResult validateDeviceCreateSwapchain(GfxDevice device, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain);
GfxResult validateDeviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer);
GfxResult validateDeviceImportBuffer(GfxDevice device, const GfxBufferImportDescriptor* descriptor, GfxBuffer* outBuffer);
GfxResult validateDeviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture);
GfxResult validateDeviceImportTexture(GfxDevice device, const GfxTextureImportDescriptor* descriptor, GfxTexture* outTexture);
GfxResult validateDeviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler);
GfxResult validateDeviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader);
GfxResult validateDeviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout);
GfxResult validateDeviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup);
GfxResult validateDeviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline);
GfxResult validateDeviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline);
GfxResult validateDeviceCreateRenderPass(GfxDevice device, const GfxRenderPassDescriptor* descriptor, GfxRenderPass* outRenderPass);
GfxResult validateDeviceCreateFramebuffer(GfxDevice device, const GfxFramebufferDescriptor* descriptor, GfxFramebuffer* outFramebuffer);
GfxResult validateDeviceCreateCommandEncoder(GfxDevice device, const GfxCommandEncoderDescriptor* descriptor, GfxCommandEncoder* outEncoder);
GfxResult validateDeviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence);
GfxResult validateDeviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore);
GfxResult validateDeviceCreateQuerySet(GfxDevice device, const GfxQuerySetDescriptor* descriptor, GfxQuerySet* outQuerySet);
GfxResult validateDeviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits);
GfxResult validateSurfaceEnumerateSupportedFormats(GfxSurface surface, uint32_t* formatCount);
GfxResult validateSurfaceEnumerateSupportedPresentModes(GfxSurface surface, uint32_t* presentModeCount);
GfxResult validateSwapchainGetInfo(GfxSwapchain swapchain, GfxSwapchainInfo* outInfo);
GfxResult validateSwapchainAcquireNextImage(GfxSwapchain swapchain, uint32_t* outImageIndex);
GfxResult validateSwapchainGetTextureView(GfxSwapchain swapchain, GfxTextureView* outView);
GfxResult validateSwapchainGetCurrentTextureView(GfxSwapchain swapchain, GfxTextureView* outView);
GfxResult validateSwapchainPresent(GfxSwapchain swapchain, const GfxPresentDescriptor* presentDescriptor);
GfxResult validateBufferGetInfo(GfxBuffer buffer, GfxBufferInfo* outInfo);
GfxResult validateBufferGetNativeHandle(GfxBuffer buffer, void** outHandle);
GfxResult validateBufferMap(GfxBuffer buffer, void** outMappedPointer);
GfxResult validateTextureGetInfo(GfxTexture texture, GfxTextureInfo* outInfo);
GfxResult validateTextureGetNativeHandle(GfxTexture texture, void** outHandle);
GfxResult validateTextureGetLayout(GfxTexture texture, GfxTextureLayout* outLayout);
GfxResult validateTextureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView);
GfxResult validateQueueSubmit(GfxQueue queue, const GfxSubmitDescriptor* submitInfo);
GfxResult validateQueueWriteBuffer(GfxQueue queue, GfxBuffer buffer, const void* data);
GfxResult validateQueueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, const GfxExtent3D* extent, const void* data);
GfxResult validateCommandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder, const GfxRenderPassBeginDescriptor* beginDescriptor, GfxRenderPassEncoder* outRenderPass);
GfxResult validateCommandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const GfxComputePassBeginDescriptor* beginDescriptor, GfxComputePassEncoder* outComputePass);
GfxResult validateCommandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder, const GfxCopyBufferToBufferDescriptor* descriptor);
GfxResult validateCommandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder, const GfxCopyBufferToTextureDescriptor* descriptor);
GfxResult validateCommandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder, const GfxCopyTextureToBufferDescriptor* descriptor);
GfxResult validateCommandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder, const GfxCopyTextureToTextureDescriptor* descriptor);
GfxResult validateCommandEncoderBlitTextureToTexture(GfxCommandEncoder commandEncoder, const GfxBlitTextureToTextureDescriptor* descriptor);
GfxResult validateCommandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder, const GfxPipelineBarrierDescriptor* descriptor);
GfxResult validateCommandEncoderGenerateMipmaps(GfxCommandEncoder commandEncoder, GfxTexture texture);
GfxResult validateCommandEncoderGenerateMipmapsRange(GfxCommandEncoder commandEncoder, GfxTexture texture);
GfxResult validateCommandEncoderWriteTimestamp(GfxCommandEncoder commandEncoder, GfxQuerySet querySet);
GfxResult validateCommandEncoderResolveQuerySet(GfxCommandEncoder commandEncoder, GfxQuerySet querySet, GfxBuffer destinationBuffer);
GfxResult validateRenderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline);
GfxResult validateRenderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, GfxBindGroup bindGroup);
GfxResult validateRenderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer);
GfxResult validateRenderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer);
GfxResult validateRenderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport);
GfxResult validateRenderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor);
GfxResult validateRenderPassEncoderDrawIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer);
GfxResult validateRenderPassEncoderDrawIndexedIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer);
GfxResult validateRenderPassEncoderBeginOcclusionQuery(GfxRenderPassEncoder renderPassEncoder, GfxQuerySet querySet);
GfxResult validateRenderPassEncoderEndOcclusionQuery(GfxRenderPassEncoder renderPassEncoder);
GfxResult validateComputePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline);
GfxResult validateComputePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, GfxBindGroup bindGroup);
GfxResult validateComputePassEncoderDispatchIndirect(GfxComputePassEncoder computePassEncoder, GfxBuffer indirectBuffer);
GfxResult validateFenceGetStatus(GfxFence fence, bool* isSignaled);
GfxResult validateSemaphoreGetType(GfxSemaphore semaphore, GfxSemaphoreType* outType);
GfxResult validateSemaphoreGetValue(GfxSemaphore semaphore, uint64_t* outValue);

// Simple validation functions (destroy, wait, etc.)
GfxResult validateInstanceDestroy(GfxInstance instance);
GfxResult validateAdapterDestroy(GfxAdapter adapter);
GfxResult validateDeviceDestroy(GfxDevice device);
GfxResult validateDeviceWaitIdle(GfxDevice device);
GfxResult validateSurfaceDestroy(GfxSurface surface);
GfxResult validateSwapchainDestroy(GfxSwapchain swapchain);
GfxResult validateBufferDestroy(GfxBuffer buffer);
GfxResult validateBufferUnmap(GfxBuffer buffer);
GfxResult validateBufferFlushMappedRange(GfxBuffer buffer);
GfxResult validateBufferInvalidateMappedRange(GfxBuffer buffer);
GfxResult validateTextureDestroy(GfxTexture texture);
GfxResult validateTextureViewDestroy(GfxTextureView textureView);
GfxResult validateSamplerDestroy(GfxSampler sampler);
GfxResult validateShaderDestroy(GfxShader shader);
GfxResult validateBindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout);
GfxResult validateBindGroupDestroy(GfxBindGroup bindGroup);
GfxResult validateRenderPipelineDestroy(GfxRenderPipeline renderPipeline);
GfxResult validateComputePipelineDestroy(GfxComputePipeline computePipeline);
GfxResult validateRenderPassDestroy(GfxRenderPass renderPass);
GfxResult validateFramebufferDestroy(GfxFramebuffer framebuffer);
GfxResult validateQuerySetDestroy(GfxQuerySet querySet);
GfxResult validateQueueWaitIdle(GfxQueue queue);
GfxResult validateCommandEncoderDestroy(GfxCommandEncoder commandEncoder);
GfxResult validateCommandEncoderEnd(GfxCommandEncoder commandEncoder);
GfxResult validateCommandEncoderBegin(GfxCommandEncoder commandEncoder);
GfxResult validateRenderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder);
GfxResult validateRenderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder);
GfxResult validateRenderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder);
GfxResult validateComputePassEncoderDispatch(GfxComputePassEncoder computePassEncoder);
GfxResult validateComputePassEncoderEnd(GfxComputePassEncoder computePassEncoder);
GfxResult validateFenceDestroy(GfxFence fence);
GfxResult validateFenceWait(GfxFence fence);
GfxResult validateFenceReset(GfxFence fence);
GfxResult validateSemaphoreDestroy(GfxSemaphore semaphore);
GfxResult validateSemaphoreSignal(GfxSemaphore semaphore);
GfxResult validateSemaphoreWait(GfxSemaphore semaphore);

} // namespace gfx::backend::vulkan::validator
