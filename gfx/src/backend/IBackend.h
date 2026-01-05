#pragma once

#include <gfx/gfx.h>

namespace gfx {
// Backend interface - each backend implements this
class IBackend {
public:
    virtual ~IBackend() = default;

    // Instance functions
    virtual GfxResult createInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance) const = 0;
    virtual void instanceDestroy(GfxInstance instance) const = 0;
    virtual void instanceSetDebugCallback(GfxInstance instance, GfxDebugCallback callback, void* userData) const = 0;
    virtual GfxResult instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter) const = 0;
    virtual uint32_t instanceEnumerateAdapters(GfxInstance instance, GfxAdapter* adapters, uint32_t maxAdapters) const = 0;

    // Adapter functions
    virtual void adapterDestroy(GfxAdapter adapter) const = 0;
    virtual GfxResult adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice) const = 0;
    virtual const char* adapterGetName(GfxAdapter adapter) const = 0;
    virtual GfxBackend adapterGetBackend(GfxAdapter adapter) const = 0;
    virtual void adapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits) const = 0;

    // Device functions
    virtual void deviceDestroy(GfxDevice device) const = 0;
    virtual GfxQueue deviceGetQueue(GfxDevice device) const = 0;
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
    virtual GfxResult deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence) const = 0;
    virtual GfxResult deviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore) const = 0;
    virtual void deviceWaitIdle(GfxDevice device) const = 0;
    virtual void deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits) const = 0;

    // Surface functions
    virtual void surfaceDestroy(GfxSurface surface) const = 0;
    virtual uint32_t surfaceGetSupportedFormats(GfxSurface surface, GfxTextureFormat* formats, uint32_t maxFormats) const = 0;
    virtual uint32_t surfaceGetSupportedPresentModes(GfxSurface surface, GfxPresentMode* presentModes, uint32_t maxModes) const = 0;

    // Swapchain functions
    virtual void swapchainDestroy(GfxSwapchain swapchain) const = 0;
    virtual uint32_t swapchainGetWidth(GfxSwapchain swapchain) const = 0;
    virtual uint32_t swapchainGetHeight(GfxSwapchain swapchain) const = 0;
    virtual GfxTextureFormat swapchainGetFormat(GfxSwapchain swapchain) const = 0;
    virtual uint32_t swapchainGetBufferCount(GfxSwapchain swapchain) const = 0;
    virtual GfxResult swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const = 0;
    virtual GfxTextureView swapchainGetImageView(GfxSwapchain swapchain, uint32_t imageIndex) const = 0;
    virtual GfxTextureView swapchainGetCurrentTextureView(GfxSwapchain swapchain) const = 0;
    virtual GfxResult swapchainPresent(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo) const = 0;

    // Buffer functions
    virtual void bufferDestroy(GfxBuffer buffer) const = 0;
    virtual uint64_t bufferGetSize(GfxBuffer buffer) const = 0;
    virtual GfxBufferUsage bufferGetUsage(GfxBuffer buffer) const = 0;
    virtual GfxResult bufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer) const = 0;
    virtual void bufferUnmap(GfxBuffer buffer) const = 0;

    // Texture functions
    virtual void textureDestroy(GfxTexture texture) const = 0;
    virtual GfxExtent3D textureGetSize(GfxTexture texture) const = 0;
    virtual GfxTextureFormat textureGetFormat(GfxTexture texture) const = 0;
    virtual uint32_t textureGetMipLevelCount(GfxTexture texture) const = 0;
    virtual GfxSampleCount textureGetSampleCount(GfxTexture texture) const = 0;
    virtual GfxTextureUsage textureGetUsage(GfxTexture texture) const = 0;
    virtual GfxTextureLayout textureGetLayout(GfxTexture texture) const = 0;
    virtual GfxResult textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView) const = 0;

    // TextureView functions
    virtual void textureViewDestroy(GfxTextureView textureView) const = 0;

    // Sampler functions
    virtual void samplerDestroy(GfxSampler sampler) const = 0;

    // Shader functions
    virtual void shaderDestroy(GfxShader shader) const = 0;

    // BindGroupLayout functions
    virtual void bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout) const = 0;

    // BindGroup functions
    virtual void bindGroupDestroy(GfxBindGroup bindGroup) const = 0;

    // RenderPipeline functions
    virtual void renderPipelineDestroy(GfxRenderPipeline renderPipeline) const = 0;

    // ComputePipeline functions
    virtual void computePipelineDestroy(GfxComputePipeline computePipeline) const = 0;

    // Queue functions
    virtual GfxResult queueSubmit(GfxQueue queue, const GfxSubmitInfo* submitInfo) const = 0;
    virtual void queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size) const = 0;
    virtual void queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
        const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
        = 0;
    virtual GfxResult queueWaitIdle(GfxQueue queue) const = 0;

    // CommandEncoder functions
    virtual void commandEncoderDestroy(GfxCommandEncoder commandEncoder) const = 0;
    virtual GfxResult commandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder,
        const GfxRenderPassDescriptor* descriptor,
        GfxRenderPassEncoder* outRenderPass) const
        = 0;
    virtual GfxResult commandEncoderBeginComputePass(GfxCommandEncoder commandEncoder,
        const GfxComputePassDescriptor* descriptor,
        GfxComputePassEncoder* outComputePass) const
        = 0;
    virtual void commandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder,
        GfxBuffer source, uint64_t sourceOffset,
        GfxBuffer destination, uint64_t destinationOffset,
        uint64_t size) const
        = 0;
    virtual void commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder,
        GfxBuffer source, uint64_t sourceOffset, uint32_t bytesPerRow,
        GfxTexture destination, const GfxOrigin3D* origin,
        const GfxExtent3D* extent, uint32_t mipLevel, GfxTextureLayout finalLayout) const
        = 0;
    virtual void commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder,
        GfxTexture source, const GfxOrigin3D* origin, uint32_t mipLevel,
        GfxBuffer destination, uint64_t destinationOffset, uint32_t bytesPerRow,
        const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
        = 0;
    virtual void commandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder,
        GfxTexture source, const GfxOrigin3D* sourceOrigin, uint32_t sourceMipLevel,
        GfxTexture destination, const GfxOrigin3D* destinationOrigin, uint32_t destinationMipLevel,
        const GfxExtent3D* extent, GfxTextureLayout srcFinalLayout, GfxTextureLayout dstFinalLayout) const
        = 0;
    virtual void commandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder,
        const GfxMemoryBarrier* memoryBarriers, uint32_t memoryBarrierCount,
        const GfxBufferBarrier* bufferBarriers, uint32_t bufferBarrierCount,
        const GfxTextureBarrier* textureBarriers, uint32_t textureBarrierCount) const
        = 0;
    virtual void commandEncoderEnd(GfxCommandEncoder commandEncoder) const = 0;
    virtual void commandEncoderBegin(GfxCommandEncoder commandEncoder) const = 0;

    // RenderPassEncoder functions
    virtual void renderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline) const = 0;
    virtual void renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const = 0;
    virtual void renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size) const = 0;
    virtual void renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size) const = 0;
    virtual void renderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport) const = 0;
    virtual void renderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor) const = 0;
    virtual void renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const = 0;
    virtual void renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) const = 0;
    virtual void renderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder) const = 0;

    // ComputePassEncoder functions
    virtual void computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline) const = 0;
    virtual void computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const = 0;
    virtual void computePassEncoderDispatchWorkgroups(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) const = 0;
    virtual void computePassEncoderEnd(GfxComputePassEncoder computePassEncoder) const = 0;

    // Fence functions
    virtual void fenceDestroy(GfxFence fence) const = 0;
    virtual GfxResult fenceGetStatus(GfxFence fence, bool* isSignaled) const = 0;
    virtual GfxResult fenceWait(GfxFence fence, uint64_t timeoutNs) const = 0;
    virtual void fenceReset(GfxFence fence) const = 0;

    // Semaphore functions
    virtual void semaphoreDestroy(GfxSemaphore semaphore) const = 0;
    virtual GfxSemaphoreType semaphoreGetType(GfxSemaphore semaphore) const = 0;
    virtual GfxResult semaphoreSignal(GfxSemaphore semaphore, uint64_t value) const = 0;
    virtual GfxResult semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs) const = 0;
    virtual uint64_t semaphoreGetValue(GfxSemaphore semaphore) const = 0;

    // Helper functions
    virtual GfxAccessFlags getAccessFlagsForLayout(GfxTextureLayout layout) const = 0;
};
} // namespace gfx
