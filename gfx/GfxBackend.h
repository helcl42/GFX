#pragma once

#include "GfxApi.h"

#ifdef __cplusplus
extern "C" {
#endif

// Backend function table - each backend implements these
typedef struct {
    // Instance functions
    GfxInstance (*createInstance)(const GfxInstanceDescriptor* descriptor);
    void (*instanceDestroy)(GfxInstance instance);
    GfxAdapter (*instanceRequestAdapter)(GfxInstance instance, const GfxAdapterDescriptor* descriptor);
    uint32_t (*instanceEnumerateAdapters)(GfxInstance instance, GfxAdapter* adapters, uint32_t maxAdapters);

    // Adapter functions
    void (*adapterDestroy)(GfxAdapter adapter);
    GfxDevice (*adapterCreateDevice)(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor);
    const char* (*adapterGetName)(GfxAdapter adapter);
    GfxBackend (*adapterGetBackend)(GfxAdapter adapter);

    // Device functions
    void (*deviceDestroy)(GfxDevice device);
    GfxQueue (*deviceGetQueue)(GfxDevice device);
    GfxSurface (*deviceCreateSurface)(GfxDevice device, const GfxSurfaceDescriptor* descriptor);
    GfxSwapchain (*deviceCreateSwapchain)(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor);
    GfxBuffer (*deviceCreateBuffer)(GfxDevice device, const GfxBufferDescriptor* descriptor);
    GfxTexture (*deviceCreateTexture)(GfxDevice device, const GfxTextureDescriptor* descriptor);
    GfxSampler (*deviceCreateSampler)(GfxDevice device, const GfxSamplerDescriptor* descriptor);
    GfxShader (*deviceCreateShader)(GfxDevice device, const GfxShaderDescriptor* descriptor);
    GfxBindGroupLayout (*deviceCreateBindGroupLayout)(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor);
    GfxBindGroup (*deviceCreateBindGroup)(GfxDevice device, const GfxBindGroupDescriptor* descriptor);
    GfxRenderPipeline (*deviceCreateRenderPipeline)(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor);
    GfxComputePipeline (*deviceCreateComputePipeline)(GfxDevice device, const GfxComputePipelineDescriptor* descriptor);
    GfxCommandEncoder (*deviceCreateCommandEncoder)(GfxDevice device, const char* label);
    GfxFence (*deviceCreateFence)(GfxDevice device, const GfxFenceDescriptor* descriptor);
    GfxSemaphore (*deviceCreateSemaphore)(GfxDevice device, const GfxSemaphoreDescriptor* descriptor);
    void (*deviceWaitIdle)(GfxDevice device);

    // Surface functions
    void (*surfaceDestroy)(GfxSurface surface);
    uint32_t (*surfaceGetWidth)(GfxSurface surface);
    uint32_t (*surfaceGetHeight)(GfxSurface surface);
    void (*surfaceResize)(GfxSurface surface, uint32_t width, uint32_t height);
    uint32_t (*surfaceGetSupportedFormats)(GfxSurface surface, GfxTextureFormat* formats, uint32_t maxFormats);
    uint32_t (*surfaceGetSupportedPresentModes)(GfxSurface surface, GfxPresentMode* presentModes, uint32_t maxModes);
    GfxPlatformWindowHandle (*surfaceGetPlatformHandle)(GfxSurface surface);

    // Swapchain functions
    void (*swapchainDestroy)(GfxSwapchain swapchain);
    uint32_t (*swapchainGetWidth)(GfxSwapchain swapchain);
    uint32_t (*swapchainGetHeight)(GfxSwapchain swapchain);
    GfxTextureFormat (*swapchainGetFormat)(GfxSwapchain swapchain);
    uint32_t (*swapchainGetBufferCount)(GfxSwapchain swapchain);
    GfxTextureView (*swapchainGetCurrentTextureView)(GfxSwapchain swapchain);
    void (*swapchainPresent)(GfxSwapchain swapchain);
    void (*swapchainResize)(GfxSwapchain swapchain, uint32_t width, uint32_t height);
    bool (*swapchainNeedsRecreation)(GfxSwapchain swapchain);

    // Buffer functions
    void (*bufferDestroy)(GfxBuffer buffer);
    uint64_t (*bufferGetSize)(GfxBuffer buffer);
    GfxBufferUsage (*bufferGetUsage)(GfxBuffer buffer);
    void* (*bufferMapAsync)(GfxBuffer buffer, uint64_t offset, uint64_t size);
    void (*bufferUnmap)(GfxBuffer buffer);

    // Texture functions
    void (*textureDestroy)(GfxTexture texture);
    GfxExtent3D (*textureGetSize)(GfxTexture texture);
    GfxTextureFormat (*textureGetFormat)(GfxTexture texture);
    uint32_t (*textureGetMipLevelCount)(GfxTexture texture);
    uint32_t (*textureGetSampleCount)(GfxTexture texture);
    GfxTextureUsage (*textureGetUsage)(GfxTexture texture);
    GfxTextureView (*textureCreateView)(GfxTexture texture, const GfxTextureViewDescriptor* descriptor);

    // TextureView functions
    void (*textureViewDestroy)(GfxTextureView textureView);
    GfxTexture (*textureViewGetTexture)(GfxTextureView textureView);

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
    void (*queueSubmit)(GfxQueue queue, GfxCommandEncoder commandEncoder);
    void (*queueSubmitWithSync)(GfxQueue queue, const GfxSubmitInfo* submitInfo);
    void (*queueWriteBuffer)(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size);
    void (*queueWriteTexture)(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
        const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent);
    void (*queueWaitIdle)(GfxQueue queue);

    // CommandEncoder functions
    void (*commandEncoderDestroy)(GfxCommandEncoder commandEncoder);
    GfxRenderPassEncoder (*commandEncoderBeginRenderPass)(GfxCommandEncoder commandEncoder,
        const GfxTextureView* colorAttachments, uint32_t colorAttachmentCount,
        const GfxColor* clearColors,
        GfxTextureView depthStencilAttachment,
        float depthClearValue, uint32_t stencilClearValue);
    GfxComputePassEncoder (*commandEncoderBeginComputePass)(GfxCommandEncoder commandEncoder, const char* label);
    void (*commandEncoderCopyBufferToBuffer)(GfxCommandEncoder commandEncoder,
        GfxBuffer source, uint64_t sourceOffset,
        GfxBuffer destination, uint64_t destinationOffset,
        uint64_t size);
    void (*commandEncoderCopyBufferToTexture)(GfxCommandEncoder commandEncoder,
        GfxBuffer source, uint64_t sourceOffset, uint32_t bytesPerRow,
        GfxTexture destination, const GfxOrigin3D* origin,
        const GfxExtent3D* extent, uint32_t mipLevel);
    void (*commandEncoderCopyTextureToBuffer)(GfxCommandEncoder commandEncoder,
        GfxTexture source, const GfxOrigin3D* origin, uint32_t mipLevel,
        GfxBuffer destination, uint64_t destinationOffset, uint32_t bytesPerRow,
        const GfxExtent3D* extent);
    void (*commandEncoderFinish)(GfxCommandEncoder commandEncoder);

    // RenderPassEncoder functions
    void (*renderPassEncoderDestroy)(GfxRenderPassEncoder renderPassEncoder);
    void (*renderPassEncoderSetPipeline)(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline);
    void (*renderPassEncoderSetBindGroup)(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup);
    void (*renderPassEncoderSetVertexBuffer)(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size);
    void (*renderPassEncoderSetIndexBuffer)(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size);
    void (*renderPassEncoderDraw)(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void (*renderPassEncoderDrawIndexed)(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance);
    void (*renderPassEncoderEnd)(GfxRenderPassEncoder renderPassEncoder);

    // ComputePassEncoder functions
    void (*computePassEncoderDestroy)(GfxComputePassEncoder computePassEncoder);
    void (*computePassEncoderSetPipeline)(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline);
    void (*computePassEncoderSetBindGroup)(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup);
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
