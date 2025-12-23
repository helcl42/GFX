#pragma once

#include <gfx/GfxApi.h>

#ifdef __cplusplus
extern "C" {
#endif

// Backend function table - each backend implements these
typedef struct {
    // Instance functions
    GfxResult (*createInstance)(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance);
    void (*instanceDestroy)(GfxInstance instance);
    void (*instanceSetDebugCallback)(GfxInstance instance, GfxDebugCallback callback, void* userData);
    GfxResult (*instanceRequestAdapter)(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter);
    uint32_t (*instanceEnumerateAdapters)(GfxInstance instance, GfxAdapter* adapters, uint32_t maxAdapters);

    // Adapter functions
    void (*adapterDestroy)(GfxAdapter adapter);
    GfxResult (*adapterCreateDevice)(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice);
    const char* (*adapterGetName)(GfxAdapter adapter);
    GfxBackend (*adapterGetBackend)(GfxAdapter adapter);

    // Device functions
    void (*deviceDestroy)(GfxDevice device);
    GfxQueue (*deviceGetQueue)(GfxDevice device);
    GfxResult (*deviceCreateSurface)(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface);
    GfxResult (*deviceCreateSwapchain)(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain);
    GfxResult (*deviceCreateBuffer)(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer);
    GfxResult (*deviceCreateTexture)(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture);
    GfxResult (*deviceCreateSampler)(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler);
    GfxResult (*deviceCreateShader)(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader);
    GfxResult (*deviceCreateBindGroupLayout)(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout);
    GfxResult (*deviceCreateBindGroup)(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup);
    GfxResult (*deviceCreateRenderPipeline)(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline);
    GfxResult (*deviceCreateComputePipeline)(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline);
    GfxResult (*deviceCreateCommandEncoder)(GfxDevice device, const char* label, GfxCommandEncoder* outEncoder);
    GfxResult (*deviceCreateFence)(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence);
    GfxResult (*deviceCreateSemaphore)(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore);
    void (*deviceWaitIdle)(GfxDevice device);
    void (*deviceGetLimits)(GfxDevice device, GfxDeviceLimits* outLimits);

    // Surface functions
    void (*surfaceDestroy)(GfxSurface surface);
    uint32_t (*surfaceGetSupportedFormats)(GfxSurface surface, GfxTextureFormat* formats, uint32_t maxFormats);
    uint32_t (*surfaceGetSupportedPresentModes)(GfxSurface surface, GfxPresentMode* presentModes, uint32_t maxModes);
    GfxPlatformWindowHandle (*surfaceGetPlatformHandle)(GfxSurface surface);

    // Swapchain functions
    void (*swapchainDestroy)(GfxSwapchain swapchain);
    uint32_t (*swapchainGetWidth)(GfxSwapchain swapchain);
    uint32_t (*swapchainGetHeight)(GfxSwapchain swapchain);
    GfxTextureFormat (*swapchainGetFormat)(GfxSwapchain swapchain);
    uint32_t (*swapchainGetBufferCount)(GfxSwapchain swapchain);
    GfxResult (*swapchainAcquireNextImage)(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex);
    GfxTextureView (*swapchainGetImageView)(GfxSwapchain swapchain, uint32_t imageIndex);
    GfxTextureView (*swapchainGetCurrentTextureView)(GfxSwapchain swapchain);
    GfxResult (*swapchainPresent)(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo);

    // Buffer functions
    void (*bufferDestroy)(GfxBuffer buffer);
    uint64_t (*bufferGetSize)(GfxBuffer buffer);
    GfxBufferUsage (*bufferGetUsage)(GfxBuffer buffer);
    GfxResult (*bufferMapAsync)(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer);
    void (*bufferUnmap)(GfxBuffer buffer);

    // Texture functions
    void (*textureDestroy)(GfxTexture texture);
    GfxExtent3D (*textureGetSize)(GfxTexture texture);
    GfxTextureFormat (*textureGetFormat)(GfxTexture texture);
    uint32_t (*textureGetMipLevelCount)(GfxTexture texture);
    GfxSampleCount (*textureGetSampleCount)(GfxTexture texture);
    GfxTextureUsage (*textureGetUsage)(GfxTexture texture);
    GfxTextureLayout (*textureGetLayout)(GfxTexture texture);
    GfxResult (*textureCreateView)(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView);

    // TextureView functions
    void (*textureViewDestroy)(GfxTextureView textureView);

    // Sampler functions
    void (*samplerDestroy)(GfxSampler sampler);

    // Shader functions
    void (*shaderDestroy)(GfxShader shader);

    // BindGroupLayout functions
    void (*bindGroupLayoutDestroy)(GfxBindGroupLayout bindGroupLayout);

    // BindGroup functions
    void (*bindGroupDestroy)(GfxBindGroup bindGroup);

    // RenderPipeline functions
    void (*renderPipelineDestroy)(GfxRenderPipeline renderPipeline);

    // ComputePipeline functions
    void (*computePipelineDestroy)(GfxComputePipeline computePipeline);

    // Queue functions
    GfxResult (*queueSubmit)(GfxQueue queue, GfxCommandEncoder commandEncoder);
    GfxResult (*queueSubmitWithSync)(GfxQueue queue, const GfxSubmitInfo* submitInfo);
    void (*queueWriteBuffer)(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size);
    void (*queueWriteTexture)(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
        const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout);
    GfxResult (*queueWaitIdle)(GfxQueue queue);

    // CommandEncoder functions
    void (*commandEncoderDestroy)(GfxCommandEncoder commandEncoder);
    GfxResult (*commandEncoderBeginRenderPass)(GfxCommandEncoder commandEncoder,
        const GfxTextureView* colorAttachments, uint32_t colorAttachmentCount,
        const GfxColor* clearColors,
        const GfxTextureLayout* colorFinalLayouts,
        GfxTextureView depthStencilAttachment,
        float depthClearValue, uint32_t stencilClearValue,
        GfxTextureLayout depthFinalLayout,
        GfxRenderPassEncoder* outRenderPass);
    GfxResult (*commandEncoderBeginComputePass)(GfxCommandEncoder commandEncoder, const char* label, GfxComputePassEncoder* outComputePass);
    void (*commandEncoderCopyBufferToBuffer)(GfxCommandEncoder commandEncoder,
        GfxBuffer source, uint64_t sourceOffset,
        GfxBuffer destination, uint64_t destinationOffset,
        uint64_t size);
    void (*commandEncoderCopyBufferToTexture)(GfxCommandEncoder commandEncoder,
        GfxBuffer source, uint64_t sourceOffset, uint32_t bytesPerRow,
        GfxTexture destination, const GfxOrigin3D* origin,
        const GfxExtent3D* extent, uint32_t mipLevel, GfxTextureLayout finalLayout);
    void (*commandEncoderCopyTextureToBuffer)(GfxCommandEncoder commandEncoder,
        GfxTexture source, const GfxOrigin3D* origin, uint32_t mipLevel,
        GfxBuffer destination, uint64_t destinationOffset, uint32_t bytesPerRow,
        const GfxExtent3D* extent, GfxTextureLayout finalLayout);
    void (*commandEncoderCopyTextureToTexture)(GfxCommandEncoder commandEncoder,
        GfxTexture source, const GfxOrigin3D* sourceOrigin, uint32_t sourceMipLevel,
        GfxTexture destination, const GfxOrigin3D* destinationOrigin, uint32_t destinationMipLevel,
        const GfxExtent3D* extent, GfxTextureLayout srcFinalLayout, GfxTextureLayout dstFinalLayout);
    void (*commandEncoderPipelineBarrier)(GfxCommandEncoder commandEncoder,
        const GfxTextureBarrier* textureBarriers, uint32_t textureBarrierCount);
    void (*commandEncoderEnd)(GfxCommandEncoder commandEncoder);
    void (*commandEncoderBegin)(GfxCommandEncoder commandEncoder);

    // RenderPassEncoder functions
    void (*renderPassEncoderDestroy)(GfxRenderPassEncoder renderPassEncoder);
    void (*renderPassEncoderSetPipeline)(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline);
    void (*renderPassEncoderSetBindGroup)(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount);
    void (*renderPassEncoderSetVertexBuffer)(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size);
    void (*renderPassEncoderSetIndexBuffer)(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size);
    void (*renderPassEncoderSetViewport)(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport);
    void (*renderPassEncoderSetScissorRect)(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor);
    void (*renderPassEncoderDraw)(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void (*renderPassEncoderDrawIndexed)(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance);
    void (*renderPassEncoderEnd)(GfxRenderPassEncoder renderPassEncoder);

    // ComputePassEncoder functions
    void (*computePassEncoderDestroy)(GfxComputePassEncoder computePassEncoder);
    void (*computePassEncoderSetPipeline)(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline);
    void (*computePassEncoderSetBindGroup)(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount);
    void (*computePassEncoderDispatchWorkgroups)(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ);
    void (*computePassEncoderEnd)(GfxComputePassEncoder computePassEncoder);

    // Fence functions
    void (*fenceDestroy)(GfxFence fence);
    GfxResult (*fenceGetStatus)(GfxFence fence, bool* isSignaled);
    GfxResult (*fenceWait)(GfxFence fence, uint64_t timeoutNs);
    void (*fenceReset)(GfxFence fence);

    // Semaphore functions
    void (*semaphoreDestroy)(GfxSemaphore semaphore);
    GfxSemaphoreType (*semaphoreGetType)(GfxSemaphore semaphore);
    GfxResult (*semaphoreSignal)(GfxSemaphore semaphore, uint64_t value);
    GfxResult (*semaphoreWait)(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs);
    uint64_t (*semaphoreGetValue)(GfxSemaphore semaphore);
} GfxBackendAPI;

// Backend registration - returns the function table for each backend
extern const GfxBackendAPI* gfxGetVulkanBackend(void);
extern const GfxBackendAPI* gfxGetWebGPUBackend(void);

#ifdef __cplusplus
}
#endif
