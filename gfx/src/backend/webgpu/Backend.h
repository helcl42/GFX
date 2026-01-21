#ifndef GFX_BACKEND_WEBGPU_H
#define GFX_BACKEND_WEBGPU_H

#include <gfx/gfx.h>

#include "../IBackend.h"

namespace gfx::backend::webgpu {
// WebGPU backend implementation
class Backend : public IBackend {
public:
    // Instance functions
    GfxResult createInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance) const override;
    GfxResult instanceDestroy(GfxInstance instance) const override;
    GfxResult instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter) const override;
    GfxResult instanceEnumerateAdapters(GfxInstance instance, uint32_t* adapterCount, GfxAdapter* adapters) const override;

    // Adapter functions
    GfxResult adapterDestroy(GfxAdapter adapter) const override;
    GfxResult adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice) const override;
    GfxResult adapterGetInfo(GfxAdapter adapter, GfxAdapterInfo* outInfo) const override;
    GfxResult adapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits) const override;
    GfxResult adapterEnumerateQueueFamilies(GfxAdapter adapter, uint32_t* queueFamilyCount, GfxQueueFamilyProperties* queueFamilies) const override;
    GfxResult adapterGetQueueFamilySurfaceSupport(GfxAdapter adapter, uint32_t queueFamilyIndex, GfxSurface surface, bool* outSupported) const override;

    // Device functions
    GfxResult deviceDestroy(GfxDevice device) const override;
    GfxResult deviceGetQueue(GfxDevice device, GfxQueue* outQueue) const override;
    GfxResult deviceGetQueueByIndex(GfxDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, GfxQueue* outQueue) const override;
    GfxResult deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface) const override;
    GfxResult deviceCreateSwapchain(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain) const override;
    GfxResult deviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer) const override;
    GfxResult deviceImportBuffer(GfxDevice device, const GfxBufferImportDescriptor* descriptor, GfxBuffer* outBuffer) const override;
    GfxResult deviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture) const override;
    GfxResult deviceImportTexture(GfxDevice device, const GfxTextureImportDescriptor* descriptor, GfxTexture* outTexture) const override;
    GfxResult deviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler) const override;
    GfxResult deviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader) const override;
    GfxResult deviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout) const override;
    GfxResult deviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup) const override;
    GfxResult deviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline) const override;
    GfxResult deviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline) const override;
    GfxResult deviceCreateRenderPass(GfxDevice device, const GfxRenderPassDescriptor* descriptor, GfxRenderPass* outRenderPass) const override;
    GfxResult deviceCreateFramebuffer(GfxDevice device, const GfxFramebufferDescriptor* descriptor, GfxFramebuffer* outFramebuffer) const override;
    GfxResult deviceCreateCommandEncoder(GfxDevice device, const GfxCommandEncoderDescriptor* descriptor, GfxCommandEncoder* outEncoder) const override;
    GfxResult deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence) const override;
    GfxResult deviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore) const override;
    GfxResult deviceWaitIdle(GfxDevice device) const override;
    GfxResult deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits) const override;

    // Surface functions
    GfxResult surfaceDestroy(GfxSurface surface) const override;
    GfxResult surfaceEnumerateSupportedFormats(GfxSurface surface, uint32_t* formatCount, GfxTextureFormat* formats) const override;
    GfxResult surfaceEnumerateSupportedPresentModes(GfxSurface surface, uint32_t* presentModeCount, GfxPresentMode* presentModes) const override;

    // Swapchain functions
    GfxResult swapchainDestroy(GfxSwapchain swapchain) const override;
    GfxResult swapchainGetInfo(GfxSwapchain swapchain, GfxSwapchainInfo* outInfo) const override;
    GfxResult swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const override;
    GfxResult swapchainGetTextureView(GfxSwapchain swapchain, uint32_t imageIndex, GfxTextureView* outView) const override;
    GfxResult swapchainGetCurrentTextureView(GfxSwapchain swapchain, GfxTextureView* outView) const override;
    GfxResult swapchainPresent(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo) const override;

    // Buffer functions
    GfxResult bufferDestroy(GfxBuffer buffer) const override;
    GfxResult bufferGetInfo(GfxBuffer buffer, GfxBufferInfo* outInfo) const override;
    GfxResult bufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer) const override;
    GfxResult bufferUnmap(GfxBuffer buffer) const override;

    // Texture functions
    GfxResult textureDestroy(GfxTexture texture) const override;
    GfxResult textureGetInfo(GfxTexture texture, GfxTextureInfo* outInfo) const override;
    GfxResult textureGetLayout(GfxTexture texture, GfxTextureLayout* outLayout) const override;
    GfxResult textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView) const override;

    // TextureView functions
    GfxResult textureViewDestroy(GfxTextureView textureView) const override;

    // Sampler functions
    GfxResult samplerDestroy(GfxSampler sampler) const override;

    // Shader functions
    GfxResult shaderDestroy(GfxShader shader) const override;

    // BindGroupLayout functions
    GfxResult bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout) const override;

    // BindGroup functions
    GfxResult bindGroupDestroy(GfxBindGroup bindGroup) const override;

    // RenderPipeline functions
    GfxResult renderPipelineDestroy(GfxRenderPipeline renderPipeline) const override;

    // ComputePipeline functions
    GfxResult computePipelineDestroy(GfxComputePipeline computePipeline) const override;

    // RenderPass functions
    GfxResult renderPassDestroy(GfxRenderPass renderPass) const override;

    // Framebuffer functions
    GfxResult framebufferDestroy(GfxFramebuffer framebuffer) const override;

    // Queue functions
    GfxResult queueSubmit(GfxQueue queue, const GfxSubmitDescriptor* submitInfo) const override;
    GfxResult queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size) const override;
    GfxResult queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel, const void* data, uint64_t dataSize, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const override;
    GfxResult queueWaitIdle(GfxQueue queue) const override;

    // CommandEncoder functions
    GfxResult commandEncoderDestroy(GfxCommandEncoder commandEncoder) const override;
    GfxResult commandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder, const GfxRenderPassBeginDescriptor* beginDescriptor, GfxRenderPassEncoder* outRenderPass) const override;
    GfxResult commandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const GfxComputePassBeginDescriptor* beginDescriptor, GfxComputePassEncoder* outComputePass) const override;
    GfxResult commandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder, const GfxCopyBufferToBufferDescriptor* descriptor) const override;
    GfxResult commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder, const GfxCopyBufferToTextureDescriptor* descriptor) const override;
    GfxResult commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder, const GfxCopyTextureToBufferDescriptor* descriptor) const override;
    GfxResult commandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder, const GfxCopyTextureToTextureDescriptor* descriptor) const override;
    GfxResult commandEncoderBlitTextureToTexture(GfxCommandEncoder commandEncoder, const GfxBlitTextureToTextureDescriptor* descriptor) const override;
    GfxResult commandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder, const GfxPipelineBarrierDescriptor* descriptor) const override;
    GfxResult commandEncoderGenerateMipmaps(GfxCommandEncoder commandEncoder, GfxTexture texture) const override;
    GfxResult commandEncoderGenerateMipmapsRange(GfxCommandEncoder commandEncoder, GfxTexture texture, uint32_t baseMipLevel, uint32_t levelCount) const override;
    GfxResult commandEncoderEnd(GfxCommandEncoder commandEncoder) const override;
    GfxResult commandEncoderBegin(GfxCommandEncoder commandEncoder) const override;

    // RenderPassEncoder functions
    GfxResult renderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline) const override;
    GfxResult renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const override;
    GfxResult renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size) const override;
    GfxResult renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size) const override;
    GfxResult renderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport) const override;
    GfxResult renderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor) const override;
    GfxResult renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const override;
    GfxResult renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) const override;
    GfxResult renderPassEncoderDrawIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const override;
    GfxResult renderPassEncoderDrawIndexedIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const override;
    GfxResult renderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder) const override;

    // ComputePassEncoder functions
    GfxResult computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline) const override;
    GfxResult computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const override;
    GfxResult computePassEncoderDispatch(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) const override;
    GfxResult computePassEncoderDispatchIndirect(GfxComputePassEncoder computePassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const override;
    GfxResult computePassEncoderEnd(GfxComputePassEncoder computePassEncoder) const override;

    // Fence functions
    GfxResult fenceDestroy(GfxFence fence) const override;
    GfxResult fenceGetStatus(GfxFence fence, bool* isSignaled) const override;
    GfxResult fenceWait(GfxFence fence, uint64_t timeoutNs) const override;
    GfxResult fenceReset(GfxFence fence) const override;

    // Semaphore functions
    GfxResult semaphoreDestroy(GfxSemaphore semaphore) const override;
    GfxResult semaphoreGetType(GfxSemaphore semaphore, GfxSemaphoreType* outType) const override;
    GfxResult semaphoreSignal(GfxSemaphore semaphore, uint64_t value) const override;
    GfxResult semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs) const override;
    GfxResult semaphoreGetValue(GfxSemaphore semaphore, uint64_t* outValue) const override;

    // Helper functions
    GfxAccessFlags getAccessFlagsForLayout(GfxTextureLayout layout) const override;
};

} // namespace gfx::backend::webgpu

#endif // GFX_WEBGPU_BACKEND_H