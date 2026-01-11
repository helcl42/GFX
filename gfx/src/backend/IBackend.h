#pragma once

#include <gfx/gfx.h>

namespace gfx {
// Backend interface - each backend implements this
class IBackend {
public:
    virtual ~IBackend() = default;

    // Instance functions
    virtual GfxResult createInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance) const = 0;
    virtual GfxResult instanceDestroy(GfxInstance instance) const = 0;
    virtual GfxResult instanceSetDebugCallback(GfxInstance instance, GfxDebugCallback callback, void* userData) const = 0;
    virtual GfxResult instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter) const = 0;
    virtual GfxResult instanceEnumerateAdapters(GfxInstance instance, uint32_t* adapterCount, GfxAdapter* adapters) const = 0;

    // Adapter functions
    virtual GfxResult adapterDestroy(GfxAdapter adapter) const = 0;
    virtual GfxResult adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice) const = 0;
    virtual GfxResult adapterGetInfo(GfxAdapter adapter, GfxAdapterInfo* outInfo) const = 0;
    virtual GfxResult adapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits) const = 0;

    // Device functions
    virtual GfxResult deviceDestroy(GfxDevice device) const = 0;
    virtual GfxResult deviceGetQueue(GfxDevice device, GfxQueue* outQueue) const = 0;
    virtual GfxResult deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface) const = 0;
    virtual GfxResult deviceCreateSwapchain(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain) const = 0;
    virtual GfxResult deviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer) const = 0;
    virtual GfxResult deviceImportBuffer(GfxDevice device, const GfxExternalBufferDescriptor* descriptor, GfxBuffer* outBuffer) const = 0;
    virtual GfxResult deviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture) const = 0;
    virtual GfxResult deviceImportTexture(GfxDevice device, const GfxExternalTextureDescriptor* descriptor, GfxTexture* outTexture) const = 0;
    virtual GfxResult deviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler) const = 0;
    virtual GfxResult deviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader) const = 0;
    virtual GfxResult deviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout) const = 0;
    virtual GfxResult deviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup) const = 0;
    virtual GfxResult deviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline) const = 0;
    virtual GfxResult deviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline) const = 0;
    virtual GfxResult deviceCreateCommandEncoder(GfxDevice device, const GfxCommandEncoderDescriptor* descriptor, GfxCommandEncoder* outEncoder) const = 0;
    virtual GfxResult deviceCreateRenderPass(GfxDevice device, const GfxRenderPassDescriptor* descriptor, GfxRenderPass* outRenderPass) const = 0;
    virtual GfxResult deviceCreateFramebuffer(GfxDevice device, const GfxFramebufferDescriptor* descriptor, GfxFramebuffer* outFramebuffer) const = 0;
    virtual GfxResult deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence) const = 0;
    virtual GfxResult deviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore) const = 0;
    virtual GfxResult deviceWaitIdle(GfxDevice device) const = 0;
    virtual GfxResult deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits) const = 0;

    // Surface functions
    virtual GfxResult surfaceDestroy(GfxSurface surface) const = 0;
    virtual GfxResult surfaceEnumerateSupportedFormats(GfxSurface surface, uint32_t* formatCount, GfxTextureFormat* formats) const = 0;
    virtual GfxResult surfaceEnumerateSupportedPresentModes(GfxSurface surface, uint32_t* presentModeCount, GfxPresentMode* presentModes) const = 0;

    // Swapchain functions
    virtual GfxResult swapchainDestroy(GfxSwapchain swapchain) const = 0;
    virtual GfxResult swapchainGetInfo(GfxSwapchain swapchain, GfxSwapchainInfo* outInfo) const = 0;
    virtual GfxResult swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const = 0;
    virtual GfxResult swapchainGetTextureView(GfxSwapchain swapchain, uint32_t imageIndex, GfxTextureView* outView) const = 0;
    virtual GfxResult swapchainGetCurrentTextureView(GfxSwapchain swapchain, GfxTextureView* outView) const = 0;
    virtual GfxResult swapchainPresent(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo) const = 0;

    // Buffer functions
    virtual GfxResult bufferDestroy(GfxBuffer buffer) const = 0;
    virtual GfxResult bufferGetInfo(GfxBuffer buffer, GfxBufferInfo* outInfo) const = 0;
    virtual GfxResult bufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer) const = 0;
    virtual GfxResult bufferUnmap(GfxBuffer buffer) const = 0;

    // Texture functions
    virtual GfxResult textureDestroy(GfxTexture texture) const = 0;
    virtual GfxResult textureGetInfo(GfxTexture texture, GfxTextureInfo* outInfo) const = 0;
    virtual GfxResult textureGetLayout(GfxTexture texture, GfxTextureLayout* outLayout) const = 0;
    virtual GfxResult textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView) const = 0;

    // TextureView functions
    virtual GfxResult textureViewDestroy(GfxTextureView textureView) const = 0;

    // Sampler functions
    virtual GfxResult samplerDestroy(GfxSampler sampler) const = 0;

    // Shader functions
    virtual GfxResult shaderDestroy(GfxShader shader) const = 0;

    // BindGroupLayout functions
    virtual GfxResult bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout) const = 0;

    // BindGroup functions
    virtual GfxResult bindGroupDestroy(GfxBindGroup bindGroup) const = 0;

    // RenderPipeline functions
    virtual GfxResult renderPipelineDestroy(GfxRenderPipeline renderPipeline) const = 0;

    // ComputePipeline functions
    virtual GfxResult computePipelineDestroy(GfxComputePipeline computePipeline) const = 0;

    // RenderPass functions
    virtual GfxResult renderPassDestroy(GfxRenderPass renderPass) const = 0;

    // Framebuffer functions
    virtual GfxResult framebufferDestroy(GfxFramebuffer framebuffer) const = 0;

    // Queue functions
    virtual GfxResult queueSubmit(GfxQueue queue, const GfxSubmitInfo* submitInfo) const = 0;
    virtual GfxResult queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size) const = 0;
    virtual GfxResult queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
        const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
        = 0;
    virtual GfxResult queueWaitIdle(GfxQueue queue) const = 0;

    // CommandEncoder functions
    virtual GfxResult commandEncoderDestroy(GfxCommandEncoder commandEncoder) const = 0;
    virtual GfxResult commandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder,
        const GfxRenderPassBeginDescriptor* beginDescriptor,
        GfxRenderPassEncoder* outRenderPass) const
        = 0;
    virtual GfxResult commandEncoderBeginComputePass(GfxCommandEncoder commandEncoder,
        const GfxComputePassBeginDescriptor* beginDescriptor,
        GfxComputePassEncoder* outComputePass) const
        = 0;
    virtual GfxResult commandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder,
        GfxBuffer source, uint64_t sourceOffset,
        GfxBuffer destination, uint64_t destinationOffset,
        uint64_t size) const
        = 0;
    virtual GfxResult commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder,
        GfxBuffer source, uint64_t sourceOffset, uint32_t bytesPerRow,
        GfxTexture destination, const GfxOrigin3D* origin,
        const GfxExtent3D* extent, uint32_t mipLevel, GfxTextureLayout finalLayout) const
        = 0;
    virtual GfxResult commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder,
        GfxTexture source, const GfxOrigin3D* origin, uint32_t mipLevel,
        GfxBuffer destination, uint64_t destinationOffset, uint32_t bytesPerRow,
        const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
        = 0;
    virtual GfxResult commandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder,
        GfxTexture source, const GfxOrigin3D* sourceOrigin, uint32_t sourceMipLevel,
        GfxTexture destination, const GfxOrigin3D* destinationOrigin, uint32_t destinationMipLevel,
        const GfxExtent3D* extent, GfxTextureLayout srcFinalLayout, GfxTextureLayout dstFinalLayout) const
        = 0;
    virtual GfxResult commandEncoderBlitTextureToTexture(GfxCommandEncoder commandEncoder,
        GfxTexture source, const GfxOrigin3D* sourceOrigin, const GfxExtent3D* sourceExtent, uint32_t sourceMipLevel,
        GfxTexture destination, const GfxOrigin3D* destinationOrigin, const GfxExtent3D* destinationExtent, uint32_t destinationMipLevel,
        GfxFilterMode filter, GfxTextureLayout srcFinalLayout, GfxTextureLayout dstFinalLayout) const
        = 0;
    virtual GfxResult commandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder,
        const GfxMemoryBarrier* memoryBarriers, uint32_t memoryBarrierCount,
        const GfxBufferBarrier* bufferBarriers, uint32_t bufferBarrierCount,
        const GfxTextureBarrier* textureBarriers, uint32_t textureBarrierCount) const
        = 0;
    virtual GfxResult commandEncoderGenerateMipmaps(GfxCommandEncoder commandEncoder, GfxTexture texture) const = 0;
    virtual GfxResult commandEncoderGenerateMipmapsRange(GfxCommandEncoder commandEncoder, GfxTexture texture,
        uint32_t baseMipLevel, uint32_t levelCount) const
        = 0;
    virtual GfxResult commandEncoderEnd(GfxCommandEncoder commandEncoder) const = 0;
    virtual GfxResult commandEncoderBegin(GfxCommandEncoder commandEncoder) const = 0;

    // RenderPassEncoder functions
    virtual GfxResult renderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline) const = 0;
    virtual GfxResult renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const = 0;
    virtual GfxResult renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size) const = 0;
    virtual GfxResult renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size) const = 0;
    virtual GfxResult renderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport) const = 0;
    virtual GfxResult renderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor) const = 0;
    virtual GfxResult renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const = 0;
    virtual GfxResult renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) const = 0;
    virtual GfxResult renderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder) const = 0;

    // ComputePassEncoder functions
    virtual GfxResult computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline) const = 0;
    virtual GfxResult computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const = 0;
    virtual GfxResult computePassEncoderDispatchWorkgroups(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) const = 0;
    virtual GfxResult computePassEncoderEnd(GfxComputePassEncoder computePassEncoder) const = 0;

    // Fence functions
    virtual GfxResult fenceDestroy(GfxFence fence) const = 0;
    virtual GfxResult fenceGetStatus(GfxFence fence, bool* isSignaled) const = 0;
    virtual GfxResult fenceWait(GfxFence fence, uint64_t timeoutNs) const = 0;
    virtual GfxResult fenceReset(GfxFence fence) const = 0;

    // Semaphore functions
    virtual GfxResult semaphoreDestroy(GfxSemaphore semaphore) const = 0;
    virtual GfxResult semaphoreGetType(GfxSemaphore semaphore, GfxSemaphoreType* outType) const = 0;
    virtual GfxResult semaphoreSignal(GfxSemaphore semaphore, uint64_t value) const = 0;
    virtual GfxResult semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs) const = 0;
    virtual GfxResult semaphoreGetValue(GfxSemaphore semaphore, uint64_t* outValue) const = 0;

    // Helper functions
    virtual GfxAccessFlags getAccessFlagsForLayout(GfxTextureLayout layout) const = 0;
};
} // namespace gfx
