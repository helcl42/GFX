#ifndef GFX_WEBGPU_BACKEND_H
#define GFX_WEBGPU_BACKEND_H

#include <gfx/gfx.h>

#include "IBackend.h"

namespace gfx::webgpu {
// WebGPU backend implementation
class WebGPUBackend : public IBackend {
public:
    // Instance functions
    GfxResult createInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance) const override;
    void instanceDestroy(GfxInstance instance) const override;
    void instanceSetDebugCallback(GfxInstance instance, GfxDebugCallback callback, void* userData) const override;
    GfxResult instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter) const override;
    uint32_t instanceEnumerateAdapters(GfxInstance instance, GfxAdapter* adapters, uint32_t maxAdapters) const override;

    // Adapter functions
    void adapterDestroy(GfxAdapter adapter) const override;
    GfxResult adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice) const override;
    const char* adapterGetName(GfxAdapter adapter) const override;
    GfxBackend adapterGetBackend(GfxAdapter adapter) const override;

    // Device functions
    void deviceDestroy(GfxDevice device) const override;
    GfxQueue deviceGetQueue(GfxDevice device) const override;
    GfxResult deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface) const override;
    GfxResult deviceCreateSwapchain(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain) const override;
    GfxResult deviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer) const override;
    GfxResult deviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture) const override;
    GfxResult deviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler) const override;
    GfxResult deviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader) const override;
    GfxResult deviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout) const override;
    GfxResult deviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup) const override;
    GfxResult deviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline) const override;
    GfxResult deviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline) const override;
    GfxResult deviceCreateCommandEncoder(GfxDevice device, const char* label, GfxCommandEncoder* outEncoder) const override;
    GfxResult deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence) const override;
    GfxResult deviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore) const override;
    void deviceWaitIdle(GfxDevice device) const override;
    void deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits) const override;

    // Surface functions
    void surfaceDestroy(GfxSurface surface) const override;
    uint32_t surfaceGetSupportedFormats(GfxSurface surface, GfxTextureFormat* formats, uint32_t maxFormats) const override;
    uint32_t surfaceGetSupportedPresentModes(GfxSurface surface, GfxPresentMode* presentModes, uint32_t maxModes) const override;
    GfxPlatformWindowHandle surfaceGetPlatformHandle(GfxSurface surface) const override;

    // Swapchain functions
    void swapchainDestroy(GfxSwapchain swapchain) const override;
    uint32_t swapchainGetWidth(GfxSwapchain swapchain) const override;
    uint32_t swapchainGetHeight(GfxSwapchain swapchain) const override;
    GfxTextureFormat swapchainGetFormat(GfxSwapchain swapchain) const override;
    uint32_t swapchainGetBufferCount(GfxSwapchain swapchain) const override;
    GfxResult swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const override;
    GfxTextureView swapchainGetImageView(GfxSwapchain swapchain, uint32_t imageIndex) const override;
    GfxTextureView swapchainGetCurrentTextureView(GfxSwapchain swapchain) const override;
    GfxResult swapchainPresent(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo) const override;

    // Buffer functions
    void bufferDestroy(GfxBuffer buffer) const override;
    uint64_t bufferGetSize(GfxBuffer buffer) const override;
    GfxBufferUsage bufferGetUsage(GfxBuffer buffer) const override;
    GfxResult bufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer) const override;
    void bufferUnmap(GfxBuffer buffer) const override;

    // Texture functions
    void textureDestroy(GfxTexture texture) const override;
    GfxExtent3D textureGetSize(GfxTexture texture) const override;
    GfxTextureFormat textureGetFormat(GfxTexture texture) const override;
    uint32_t textureGetMipLevelCount(GfxTexture texture) const override;
    GfxSampleCount textureGetSampleCount(GfxTexture texture) const override;
    GfxTextureUsage textureGetUsage(GfxTexture texture) const override;
    GfxTextureLayout textureGetLayout(GfxTexture texture) const override;
    GfxResult textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView) const override;

    // TextureView functions
    void textureViewDestroy(GfxTextureView textureView) const override;

    // Sampler functions
    void samplerDestroy(GfxSampler sampler) const override;

    // Shader functions
    void shaderDestroy(GfxShader shader) const override;

    // BindGroupLayout functions
    void bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout) const override;

    // BindGroup functions
    void bindGroupDestroy(GfxBindGroup bindGroup) const override;

    // RenderPipeline functions
    void renderPipelineDestroy(GfxRenderPipeline renderPipeline) const override;

    // ComputePipeline functions
    void computePipelineDestroy(GfxComputePipeline computePipeline) const override;

    // Queue functions
    GfxResult queueSubmit(GfxQueue queue, const GfxSubmitInfo* submitInfo) const override;
    void queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size) const override;
    void queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
        const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const override;
    GfxResult queueWaitIdle(GfxQueue queue) const override;

    // CommandEncoder functions
    void commandEncoderDestroy(GfxCommandEncoder commandEncoder) const override;
    GfxResult commandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder,
        GfxTextureView const* colorAttachments, uint32_t colorAttachmentCount,
        const GfxColor* clearColors, const GfxTextureLayout* colorLayouts,
        GfxTextureView depthStencilAttachment, float depthClearValue, uint32_t stencilClearValue,
        GfxTextureLayout depthStencilLayout,
        GfxRenderPassEncoder* outRenderPass) const override;
    GfxResult commandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const char* label, GfxComputePassEncoder* outComputePass) const override;
    void commandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder, GfxBuffer source, uint64_t sourceOffset,
        GfxBuffer destination, uint64_t destinationOffset, uint64_t size) const override;
    void commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder, GfxBuffer source, uint64_t sourceOffset, uint32_t bytesPerRow,
        GfxTexture destination, const GfxOrigin3D* origin, const GfxExtent3D* extent, uint32_t mipLevel, GfxTextureLayout finalLayout) const override;
    void commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder, GfxTexture source, const GfxOrigin3D* origin, uint32_t mipLevel,
        GfxBuffer destination, uint64_t destinationOffset, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const override;
    void commandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder, GfxTexture source, const GfxOrigin3D* sourceOrigin, uint32_t sourceMipLevel,
        GfxTexture destination, const GfxOrigin3D* destinationOrigin, uint32_t destinationMipLevel, const GfxExtent3D* extent,
        GfxTextureLayout srcFinalLayout, GfxTextureLayout dstFinalLayout) const override;
    void commandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder,
        const GfxMemoryBarrier* memoryBarriers, uint32_t memoryBarrierCount,
        const GfxBufferBarrier* bufferBarriers, uint32_t bufferBarrierCount,
        const GfxTextureBarrier* textureBarriers, uint32_t textureBarrierCount) const override;
    void commandEncoderEnd(GfxCommandEncoder commandEncoder) const override;
    void commandEncoderBegin(GfxCommandEncoder commandEncoder) const override;

    // RenderPassEncoder functions
    void renderPassEncoderDestroy(GfxRenderPassEncoder renderPassEncoder) const override;
    void renderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline) const override;
    void renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const override;
    void renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size) const override;
    void renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size) const override;
    void renderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport) const override;
    void renderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor) const override;
    void renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const override;
    void renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) const override;
    void renderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder) const override;

    // ComputePassEncoder functions
    void computePassEncoderDestroy(GfxComputePassEncoder computePassEncoder) const override;
    void computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline) const override;
    void computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const override;
    void computePassEncoderDispatchWorkgroups(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) const override;
    void computePassEncoderEnd(GfxComputePassEncoder computePassEncoder) const override;

    // Fence functions
    void fenceDestroy(GfxFence fence) const override;
    GfxResult fenceGetStatus(GfxFence fence) const override;
    GfxResult fenceWait(GfxFence fence, uint64_t timeoutNs) const override;
    void fenceReset(GfxFence fence) const override;

    // Semaphore functions
    void semaphoreDestroy(GfxSemaphore semaphore) const override;
    GfxSemaphoreType semaphoreGetType(GfxSemaphore semaphore) const override;
    GfxResult semaphoreSignal(GfxSemaphore semaphore, uint64_t value) const override;
    GfxResult semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs) const override;
    uint64_t semaphoreGetValue(GfxSemaphore semaphore) const override;

public:
    static const IBackend* create();
};

} // namespace gfx::webgpu

#endif // GFX_WEBGPU_BACKEND_H