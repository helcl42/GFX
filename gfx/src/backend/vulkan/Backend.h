#ifndef GFX_BACKEND_VULKAN_H
#define GFX_BACKEND_VULKAN_H

#include "../IBackend.h"
#include "component/CommandComponent.h"
#include "component/ComputeComponent.h"
#include "component/PresentationComponent.h"
#include "component/QueryComponent.h"
#include "component/RenderComponent.h"
#include "component/ResourceComponent.h"
#include "component/SyncComponent.h"
#include "component/SystemComponent.h"

#include <gfx/gfx.h>

namespace gfx::backend::vulkan {

// Vulkan backend implementation
class Backend : public IBackend {
public:
    // Instance functions
    GfxResult createInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance) const override;
    GfxResult instanceDestroy(GfxInstance instance) const override;
    GfxResult instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter) const override;
    GfxResult instanceEnumerateAdapters(GfxInstance instance, uint32_t* adapterCount, GfxAdapter* adapters) const override;
    GfxResult enumerateInstanceExtensions(uint32_t* extensionCount, const char** extensionNames) const override;

    // Adapter functions
    GfxResult adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice) const override;
    GfxResult adapterGetInfo(GfxAdapter adapter, GfxAdapterInfo* outInfo) const override;
    GfxResult adapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits) const override;
    GfxResult adapterEnumerateQueueFamilies(GfxAdapter adapter, uint32_t* queueFamilyCount, GfxQueueFamilyProperties* queueFamilies) const override;
    GfxResult adapterGetQueueFamilySurfaceSupport(GfxAdapter adapter, uint32_t queueFamilyIndex, GfxSurface surface, bool* outSupported) const override;
    GfxResult adapterEnumerateExtensions(GfxAdapter adapter, uint32_t* extensionCount, const char** extensionNames) const override;

    // Device functions
    GfxResult deviceDestroy(GfxDevice device) const override;
    GfxResult deviceGetQueue(GfxDevice device, GfxQueue* outQueue) const override;
    GfxResult deviceGetQueueByIndex(GfxDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, GfxQueue* outQueue) const override;
    GfxResult deviceWaitIdle(GfxDevice device) const override;
    GfxResult deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits) const override;
    GfxResult deviceSupportsShaderFormat(GfxDevice device, GfxShaderSourceType format, bool* outSupported) const override;

    // Queue functions
    GfxResult queueSubmit(GfxQueue queue, const GfxSubmitDescriptor* submitDescriptor) const override;
    GfxResult queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size) const override;
    GfxResult queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, const GfxExtent3D* extent, uint32_t mipLevel, const void* data, uint64_t dataSize, GfxTextureLayout finalLayout) const override;
    GfxResult queueWaitIdle(GfxQueue queue) const override;

    // Surface functions
    GfxResult deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface) const override;
    GfxResult surfaceDestroy(GfxSurface surface) const override;
    GfxResult surfaceGetInfo(GfxSurface surface, GfxSurfaceInfo* outInfo) const override;
    GfxResult surfaceEnumerateSupportedFormats(GfxSurface surface, uint32_t* formatCount, GfxFormat* formats) const override;
    GfxResult surfaceEnumerateSupportedPresentModes(GfxSurface surface, uint32_t* presentModeCount, GfxPresentMode* presentModes) const override;

    // Swapchain functions
    GfxResult deviceCreateSwapchain(GfxDevice device, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain) const override;
    GfxResult swapchainDestroy(GfxSwapchain swapchain) const override;
    GfxResult swapchainGetInfo(GfxSwapchain swapchain, GfxSwapchainInfo* outInfo) const override;
    GfxResult swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const override;
    GfxResult swapchainGetTextureView(GfxSwapchain swapchain, uint32_t imageIndex, GfxTextureView* outView) const override;
    GfxResult swapchainGetCurrentTextureView(GfxSwapchain swapchain, GfxTextureView* outView) const override;
    GfxResult swapchainPresent(GfxSwapchain swapchain, const GfxPresentDescriptor* presentDescriptor) const override;

    // Buffer functions
    GfxResult deviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer) const override;
    GfxResult deviceImportBuffer(GfxDevice device, const GfxBufferImportDescriptor* descriptor, GfxBuffer* outBuffer) const override;
    GfxResult bufferDestroy(GfxBuffer buffer) const override;
    GfxResult bufferGetInfo(GfxBuffer buffer, GfxBufferInfo* outInfo) const override;
    GfxResult bufferGetNativeHandle(GfxBuffer buffer, void** outHandle) const override;
    GfxResult bufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer) const override;
    GfxResult bufferUnmap(GfxBuffer buffer) const override;
    GfxResult bufferFlushMappedRange(GfxBuffer buffer, uint64_t offset, uint64_t size) const override;
    GfxResult bufferInvalidateMappedRange(GfxBuffer buffer, uint64_t offset, uint64_t size) const override;

    // Texture functions
    GfxResult deviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture) const override;
    GfxResult deviceImportTexture(GfxDevice device, const GfxTextureImportDescriptor* descriptor, GfxTexture* outTexture) const override;
    GfxResult textureDestroy(GfxTexture texture) const override;
    GfxResult textureGetInfo(GfxTexture texture, GfxTextureInfo* outInfo) const override;
    GfxResult textureGetNativeHandle(GfxTexture texture, void** outHandle) const override;
    GfxResult textureGetLayout(GfxTexture texture, GfxTextureLayout* outLayout) const override;
    GfxResult textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView) const override;

    // TextureView functions
    GfxResult textureViewDestroy(GfxTextureView textureView) const override;

    // Sampler functions
    GfxResult deviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler) const override;
    GfxResult samplerDestroy(GfxSampler sampler) const override;

    // Shader functions
    GfxResult deviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader) const override;
    GfxResult shaderDestroy(GfxShader shader) const override;

    // BindGroupLayout functions
    GfxResult deviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout) const override;
    GfxResult bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout) const override;

    // BindGroup functions
    GfxResult deviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup) const override;
    GfxResult bindGroupDestroy(GfxBindGroup bindGroup) const override;

    // RenderPipeline functions
    GfxResult deviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline) const override;
    GfxResult renderPipelineDestroy(GfxRenderPipeline renderPipeline) const override;

    // ComputePipeline functions
    GfxResult deviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline) const override;
    GfxResult computePipelineDestroy(GfxComputePipeline computePipeline) const override;

    // RenderPass functions
    GfxResult deviceCreateRenderPass(GfxDevice device, const GfxRenderPassDescriptor* descriptor, GfxRenderPass* outRenderPass) const override;
    GfxResult renderPassDestroy(GfxRenderPass renderPass) const override;

    // Framebuffer functions
    GfxResult deviceCreateFramebuffer(GfxDevice device, const GfxFramebufferDescriptor* descriptor, GfxFramebuffer* outFramebuffer) const override;
    GfxResult framebufferDestroy(GfxFramebuffer framebuffer) const override;

    // CommandEncoder functions
    GfxResult deviceCreateCommandEncoder(GfxDevice device, const GfxCommandEncoderDescriptor* descriptor, GfxCommandEncoder* outEncoder) const override;
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
    GfxResult commandEncoderWriteTimestamp(GfxCommandEncoder commandEncoder, GfxQuerySet querySet, uint32_t queryIndex) const override;
    GfxResult commandEncoderResolveQuerySet(GfxCommandEncoder commandEncoder, GfxQuerySet querySet, uint32_t firstQuery, uint32_t queryCount, GfxBuffer destinationBuffer, uint64_t destinationOffset) const override;
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
    GfxResult renderPassEncoderBeginOcclusionQuery(GfxRenderPassEncoder renderPassEncoder, GfxQuerySet querySet, uint32_t queryIndex) const override;
    GfxResult renderPassEncoderEndOcclusionQuery(GfxRenderPassEncoder renderPassEncoder) const override;
    GfxResult renderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder) const override;

    // ComputePassEncoder functions
    GfxResult computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline) const override;
    GfxResult computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const override;
    GfxResult computePassEncoderDispatch(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) const override;
    GfxResult computePassEncoderDispatchIndirect(GfxComputePassEncoder computePassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const override;
    GfxResult computePassEncoderEnd(GfxComputePassEncoder computePassEncoder) const override;

    // Fence functions
    GfxResult deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence) const override;
    GfxResult fenceDestroy(GfxFence fence) const override;
    GfxResult fenceGetStatus(GfxFence fence, bool* isSignaled) const override;
    GfxResult fenceWait(GfxFence fence, uint64_t timeoutNs) const override;
    GfxResult fenceReset(GfxFence fence) const override;

    // Semaphore functions
    GfxResult deviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore) const override;
    GfxResult semaphoreDestroy(GfxSemaphore semaphore) const override;
    GfxResult semaphoreGetType(GfxSemaphore semaphore, GfxSemaphoreType* outType) const override;
    GfxResult semaphoreSignal(GfxSemaphore semaphore, uint64_t value) const override;
    GfxResult semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs) const override;
    GfxResult semaphoreGetValue(GfxSemaphore semaphore, uint64_t* outValue) const override;

    // QuerySet functions
    GfxResult deviceCreateQuerySet(GfxDevice device, const GfxQuerySetDescriptor* descriptor, GfxQuerySet* outQuerySet) const override;
    GfxResult querySetDestroy(GfxQuerySet querySet) const override;

    // Helper functions
    GfxAccessFlags getAccessFlagsForLayout(GfxTextureLayout layout) const override;

private:
    component::SystemComponent m_systemComponent;
    component::PresentationComponent m_presentationComponent;
    component::ResourceComponent m_resourceComponent;
    component::RenderComponent m_renderComponent;
    component::ComputeComponent m_computeComponent;
    component::CommandComponent m_commandComponent;
    component::SyncComponent m_syncComponent;
    component::QueryComponent m_queryComponent;
};

} // namespace gfx::backend::vulkan

#endif // GFX_BACKEND_VULKAN_H