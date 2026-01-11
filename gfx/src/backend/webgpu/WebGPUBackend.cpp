#include "common/WebGPUCommon.h"

#include "WebGPUBackend.h"
#include "converter/GfxWebGPUConverter.h"
#include "entity/Entities.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace gfx::webgpu {
using converter::toNative;

// ============================================================================
// Backend C++ Class Export
// ============================================================================

// Instance functions
GfxResult WebGPUBackend::createInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance) const
{
    if (!outInstance) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto createInfo = converter::gfxDescriptorToWebGPUInstanceCreateInfo(descriptor);
        auto* instance = new gfx::webgpu::Instance(createInfo);
        *outInstance = converter::toGfx<GfxInstance>(instance);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::instanceDestroy(GfxInstance instance) const
{
    if (!instance) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Process any remaining events before destroying the instance
    // This ensures all pending callbacks are completed
    auto* inst = converter::toNative<Instance>(instance);
    if (inst->handle()) {
        wgpuInstanceProcessEvents(inst->handle());
    }

    delete inst;
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::instanceSetDebugCallback(GfxInstance instance, GfxDebugCallback callback, void* userData) const
{
    // TODO: Implement debug callback using WebGPU error handling
    (void)instance;
    (void)callback;
    (void)userData;
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter) const
{
    if (!instance || !outAdapter) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* inst = converter::toNative<Instance>(instance);
        auto createInfo = converter::gfxDescriptorToWebGPUAdapterCreateInfo(descriptor);
        auto* adapter = new gfx::webgpu::Adapter(inst, createInfo);
        *outAdapter = converter::toGfx<GfxAdapter>(adapter);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::instanceEnumerateAdapters(GfxInstance instance, uint32_t* adapterCount, GfxAdapter* adapters) const
{
    if (!instance || !adapterCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* inst = converter::toNative<Instance>(instance);
    uint32_t count = gfx::webgpu::Adapter::enumerate(inst, reinterpret_cast<Adapter**>(adapters), adapters ? *adapterCount : 0);
    *adapterCount = count;
    return GFX_RESULT_SUCCESS;
}

// Adapter functions
GfxResult WebGPUBackend::adapterDestroy(GfxAdapter adapter) const
{
    if (!adapter) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    delete converter::toNative<Adapter>(adapter);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice) const
{
    if (!adapter || !outDevice) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* adapterPtr = converter::toNative<Adapter>(adapter);
        auto createInfo = converter::gfxDescriptorToWebGPUDeviceCreateInfo(descriptor);
        auto* device = new gfx::webgpu::Device(adapterPtr, createInfo);
        *outDevice = converter::toGfx<GfxDevice>(device);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::adapterGetInfo(GfxAdapter adapter, GfxAdapterInfo* outInfo) const
{
    if (!adapter || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* adapterPtr = converter::toNative<Adapter>(adapter);
    *outInfo = converter::wgpuAdapterToGfxAdapterInfo(adapterPtr->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::adapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits) const
{
    if (!adapter || !outLimits) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* adapterPtr = converter::toNative<Adapter>(adapter);
    *outLimits = converter::wgpuLimitsToGfxDeviceLimits(adapterPtr->getLimits());
    return GFX_RESULT_SUCCESS;
    return GFX_RESULT_SUCCESS;
}

// Device functions
GfxResult WebGPUBackend::deviceDestroy(GfxDevice device) const
{
    if (!device) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    delete converter::toNative<Device>(device);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::deviceGetQueue(GfxDevice device, GfxQueue* outQueue) const
{
    if (!device || !outQueue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* dev = converter::toNative<Device>(device);
    *outQueue = converter::toGfx<GfxQueue>(dev->getQueue());
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface) const
{
    if (!device || !descriptor || !outSurface) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUSurfaceCreateInfo(descriptor);
        auto* surface = new gfx::webgpu::Surface(devicePtr->getAdapter()->getInstance()->handle(), devicePtr->getAdapter()->handle(), createInfo);
        *outSurface = converter::toGfx<GfxSurface>(surface);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::deviceCreateSwapchain(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain) const
{
    if (!device || !surface || !descriptor || !outSwapchain) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<Device>(device);
        auto* surfacePtr = converter::toNative<Surface>(surface);
        auto createInfo = converter::gfxDescriptorToWebGPUSwapchainCreateInfo(descriptor);
        auto* swapchain = new gfx::webgpu::Swapchain(devicePtr, surfacePtr, createInfo);
        *outSwapchain = converter::toGfx<GfxSwapchain>(swapchain);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::deviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer) const
{
    if (!device || !descriptor || !outBuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUBufferCreateInfo(descriptor);
        auto* buffer = new gfx::webgpu::Buffer(devicePtr, createInfo);
        *outBuffer = converter::toGfx<GfxBuffer>(buffer);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::deviceImportBuffer(GfxDevice device, const GfxExternalBufferDescriptor* descriptor, GfxBuffer* outBuffer) const
{
    if (!device || !descriptor || !outBuffer || !descriptor->nativeHandle) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<Device>(device);
        auto wgpuBuffer = reinterpret_cast<WGPUBuffer>(const_cast<void*>(descriptor->nativeHandle));
        auto importInfo = converter::gfxExternalDescriptorToWebGPUBufferImportInfo(descriptor);
        auto* buffer = new gfx::webgpu::Buffer(devicePtr, wgpuBuffer, importInfo);
        *outBuffer = converter::toGfx<GfxBuffer>(buffer);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::deviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture) const
{
    if (!device || !descriptor || !outTexture) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUTextureCreateInfo(descriptor);
        auto* texture = new gfx::webgpu::Texture(devicePtr, createInfo);
        *outTexture = converter::toGfx<GfxTexture>(texture);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::deviceImportTexture(GfxDevice device, const GfxExternalTextureDescriptor* descriptor, GfxTexture* outTexture) const
{
    if (!device || !descriptor || !outTexture || !descriptor->nativeHandle) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<Device>(device);
        auto wgpuTexture = reinterpret_cast<WGPUTexture>(const_cast<void*>(descriptor->nativeHandle));
        auto importInfo = converter::gfxExternalDescriptorToWebGPUTextureImportInfo(descriptor);
        auto* texture = new gfx::webgpu::Texture(devicePtr, wgpuTexture, importInfo);
        *outTexture = converter::toGfx<GfxTexture>(texture);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::deviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler) const
{
    if (!device || !descriptor || !outSampler) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUSamplerCreateInfo(descriptor);
        auto* sampler = new gfx::webgpu::Sampler(devicePtr, createInfo);
        *outSampler = converter::toGfx<GfxSampler>(sampler);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::deviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader) const
{
    if (!device || !descriptor || !descriptor->code || !outShader) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUShaderCreateInfo(descriptor);
        auto* shader = new gfx::webgpu::Shader(devicePtr, createInfo);
        *outShader = converter::toGfx<GfxShader>(shader);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::deviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout) const
{
    if (!device || !descriptor || !outLayout) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUBindGroupLayoutCreateInfo(descriptor);
        auto* layout = new gfx::webgpu::BindGroupLayout(devicePtr, createInfo);
        *outLayout = converter::toGfx<GfxBindGroupLayout>(layout);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::deviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup) const
{
    if (!device || !descriptor || !descriptor->layout || !outBindGroup) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<Device>(device);
        auto* layoutPtr = converter::toNative<BindGroupLayout>(descriptor->layout);
        auto createInfo = converter::gfxDescriptorToWebGPUBindGroupCreateInfo(descriptor, layoutPtr->handle());
        auto* bindGroup = new gfx::webgpu::BindGroup(devicePtr, createInfo);
        *outBindGroup = converter::toGfx<GfxBindGroup>(bindGroup);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::deviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline) const
{
    if (!device || !descriptor || !outPipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPURenderPipelineCreateInfo(descriptor);
        auto* pipeline = new gfx::webgpu::RenderPipeline(devicePtr, createInfo);
        *outPipeline = converter::toGfx<GfxRenderPipeline>(pipeline);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::deviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline) const
{
    if (!device || !descriptor || !descriptor->compute || !outPipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUComputePipelineCreateInfo(descriptor);
        auto* pipeline = new gfx::webgpu::ComputePipeline(devicePtr, createInfo);
        *outPipeline = converter::toGfx<GfxComputePipeline>(pipeline);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::deviceCreateRenderPass(GfxDevice device, const GfxRenderPassDescriptor* descriptor, GfxRenderPass* outRenderPass) const
{
    if (!device || !descriptor || !outRenderPass) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* dev = converter::toNative<Device>(device);
        auto createInfo = converter::gfxRenderPassDescriptorToRenderPassCreateInfo(descriptor);
        auto* renderPass = new gfx::webgpu::RenderPass(dev, createInfo);
        *outRenderPass = converter::toGfx<GfxRenderPass>(renderPass);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create render pass: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::deviceCreateFramebuffer(GfxDevice device, const GfxFramebufferDescriptor* descriptor, GfxFramebuffer* outFramebuffer) const
{
    if (!device || !descriptor || !outFramebuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* dev = converter::toNative<Device>(device);
        auto createInfo = converter::gfxFramebufferDescriptorToFramebufferCreateInfo(descriptor);
        auto* framebuffer = new gfx::webgpu::Framebuffer(dev, createInfo);
        *outFramebuffer = converter::toGfx<GfxFramebuffer>(framebuffer);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to create framebuffer: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::deviceCreateCommandEncoder(GfxDevice device, const GfxCommandEncoderDescriptor* descriptor, GfxCommandEncoder* outEncoder) const
{
    if (!device || !descriptor || !outEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUCommandEncoderCreateInfo(descriptor);
        auto* encoder = new gfx::webgpu::CommandEncoder(devicePtr, createInfo);
        *outEncoder = converter::toGfx<GfxCommandEncoder>(encoder);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence) const
{
    if (!device || !descriptor || !outFence) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* fence = new gfx::webgpu::Fence(descriptor->signaled);
        *outFence = converter::toGfx<GfxFence>(fence);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::deviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore) const
{
    if (!device || !descriptor || !outSemaphore) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto semaphoreType = converter::gfxSemaphoreTypeToWebGPUSemaphoreType(descriptor->type);
        auto* semaphore = new gfx::webgpu::Semaphore(semaphoreType, descriptor->initialValue);
        *outSemaphore = converter::toGfx<GfxSemaphore>(semaphore);
        return GFX_RESULT_SUCCESS;
    } catch (...) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::deviceWaitIdle(GfxDevice device) const
{
    if (!device) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* devicePtr = converter::toNative<Device>(device);
    devicePtr->waitIdle();
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits) const
{
    if (!device || !outLimits) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* devicePtr = converter::toNative<Device>(device);
    *outLimits = converter::wgpuLimitsToGfxDeviceLimits(devicePtr->getLimits());
    return GFX_RESULT_SUCCESS;
}

// Surface functions
GfxResult WebGPUBackend::surfaceDestroy(GfxSurface surface) const
{
    if (!surface) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    delete converter::toNative<Surface>(surface);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::surfaceEnumerateSupportedFormats(GfxSurface surface, uint32_t* formatCount, GfxTextureFormat* formats) const
{
    if (!surface || !formatCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* surf = converter::toNative<Surface>(surface);

    // Query surface capabilities
    WGPUSurfaceCapabilities capabilities = surf->getCapabilities();

    uint32_t count = static_cast<uint32_t>(capabilities.formatCount);

    if (count == 0) {
        wgpuSurfaceCapabilitiesFreeMembers(capabilities);
        *formatCount = 0;
        return GFX_RESULT_SUCCESS;
    }

    // Convert to GfxTextureFormat
    if (formats) {
        uint32_t copyCount = std::min(count, *formatCount);
        for (uint32_t i = 0; i < copyCount; ++i) {
            formats[i] = converter::wgpuFormatToGfxFormat(capabilities.formats[i]);
        }
    }

    wgpuSurfaceCapabilitiesFreeMembers(capabilities);
    *formatCount = count;
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::surfaceEnumerateSupportedPresentModes(GfxSurface surface, uint32_t* presentModeCount, GfxPresentMode* presentModes) const
{
    if (!surface || !presentModeCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* surf = converter::toNative<Surface>(surface);

    // Query surface capabilities
    WGPUSurfaceCapabilities capabilities = surf->getCapabilities();

    uint32_t count = static_cast<uint32_t>(capabilities.presentModeCount);

    if (count == 0) {
        wgpuSurfaceCapabilitiesFreeMembers(capabilities);
        *presentModeCount = 0;
        return GFX_RESULT_SUCCESS;
    }

    // Convert to GfxPresentMode
    if (presentModes) {
        uint32_t copyCount = std::min(count, *presentModeCount);
        for (uint32_t i = 0; i < copyCount; ++i) {
            presentModes[i] = converter::wgpuPresentModeToGfxPresentMode(capabilities.presentModes[i]);
        }
    }

    wgpuSurfaceCapabilitiesFreeMembers(capabilities);
    *presentModeCount = count;
    return GFX_RESULT_SUCCESS;
}

// Swapchain functions
GfxResult WebGPUBackend::swapchainDestroy(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    delete converter::toNative<Swapchain>(swapchain);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::swapchainGetInfo(GfxSwapchain swapchain, GfxSwapchainInfo* outInfo) const
{
    if (!swapchain || !outInfo) {
        if (outInfo) {
            outInfo->width = 0;
            outInfo->height = 0;
            outInfo->format = GFX_TEXTURE_FORMAT_UNDEFINED;
            outInfo->imageCount = 0;
        }
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* swapchainPtr = converter::toNative<Swapchain>(swapchain);
    *outInfo = converter::wgpuSwapchainInfoToGfxSwapchainInfo(swapchainPtr->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const
{
    if (!swapchain || !outImageIndex) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // WebGPU doesn't have explicit acquire semantics with semaphores
    // The surface texture is acquired implicitly when we call wgpuSurfaceGetCurrentTexture
    // For now, we just return image index 0 (WebGPU doesn't expose multiple image indices)
    // The semaphore and fence are noted but WebGPU doesn't support explicit synchronization primitives

    (void)timeoutNs;
    (void)imageAvailableSemaphore; // WebGPU doesn't support explicit semaphore signaling

    auto* swapchainPtr = converter::toNative<Swapchain>(swapchain);

    WGPUSurfaceGetCurrentTextureStatus status = swapchainPtr->acquireNextImage();

    GfxResult result = GFX_RESULT_SUCCESS;
    switch (status) {
    case WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal:
    case WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal:
        *outImageIndex = 0; // WebGPU only exposes current image
        result = GFX_RESULT_SUCCESS;
        break;
    case WGPUSurfaceGetCurrentTextureStatus_Timeout:
        result = GFX_RESULT_TIMEOUT;
        break;
    case WGPUSurfaceGetCurrentTextureStatus_Outdated:
        result = GFX_RESULT_ERROR_OUT_OF_DATE;
        break;
    case WGPUSurfaceGetCurrentTextureStatus_Lost:
        result = GFX_RESULT_ERROR_SURFACE_LOST;
        break;
    default:
        result = GFX_RESULT_ERROR_UNKNOWN;
        break;
    }

    // Signal fence if provided (even though WebGPU doesn't truly have fences)
    if (fence && result == GFX_RESULT_SUCCESS) {
        auto* fencePtr = converter::toNative<Fence>(fence);
        fencePtr->setSignaled(true);
    }

    return result;
}

GfxResult WebGPUBackend::swapchainGetTextureView(GfxSwapchain swapchain, uint32_t imageIndex, GfxTextureView* outView) const
{
    if (!swapchain || !outView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // WebGPU doesn't expose multiple swapchain images by index
    // Always return the current texture view regardless of index
    (void)imageIndex;

    return swapchainGetCurrentTextureView(swapchain, outView);
}

GfxResult WebGPUBackend::swapchainGetCurrentTextureView(GfxSwapchain swapchain, GfxTextureView* outView) const
{
    if (!swapchain || !outView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* swapchainPtr = converter::toNative<Swapchain>(swapchain);
    *outView = converter::toGfx<GfxTextureView>(swapchainPtr->getCurrentTextureView());
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::swapchainPresent(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo) const
{
    if (!swapchain) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // WebGPU doesn't support explicit wait semaphores for present
    // The queue submission already ensures ordering, so we just present
    (void)presentInfo; // Wait semaphores are noted but not used in WebGPU

    auto* swapchainPtr = converter::toNative<Swapchain>(swapchain);
    swapchainPtr->present();

    return GFX_RESULT_SUCCESS;
}

// Buffer functions
GfxResult WebGPUBackend::bufferDestroy(GfxBuffer buffer) const
{
    if (!buffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    delete converter::toNative<Buffer>(buffer);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::bufferGetInfo(GfxBuffer buffer, GfxBufferInfo* outInfo) const
{
    if (!buffer || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* buf = converter::toNative<Buffer>(buffer);
    *outInfo = converter::wgpuBufferToGfxBufferInfo(buf->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::bufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer) const
{
    if (!buffer || !outMappedPointer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* bufferPtr = converter::toNative<Buffer>(buffer);
    void* mappedData = bufferPtr->map(offset, size);

    if (!mappedData) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    *outMappedPointer = mappedData;
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::bufferUnmap(GfxBuffer buffer) const
{
    if (!buffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* bufferPtr = converter::toNative<Buffer>(buffer);
    bufferPtr->unmap();
    return GFX_RESULT_SUCCESS;
}

// Texture functions
GfxResult WebGPUBackend::textureDestroy(GfxTexture texture) const
{
    if (!texture) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    delete converter::toNative<Texture>(texture);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::textureGetInfo(GfxTexture texture, GfxTextureInfo* outInfo) const
{
    if (!texture || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* texturePtr = converter::toNative<Texture>(texture);
    *outInfo = converter::wgpuTextureInfoToGfxTextureInfo(texturePtr->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::textureGetLayout(GfxTexture texture, GfxTextureLayout* outLayout) const
{
    // WebGPU doesn't have explicit layouts, return GENERAL as a reasonable default
    if (!texture || !outLayout) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    *outLayout = GFX_TEXTURE_LAYOUT_GENERAL;
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView) const
{
    if (!texture || !outView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* texturePtr = converter::toNative<Texture>(texture);
        auto createInfo = converter::gfxDescriptorToWebGPUTextureViewCreateInfo(descriptor);
        auto* view = new gfx::webgpu::TextureView(texturePtr, createInfo);
        *outView = converter::toGfx<GfxTextureView>(view);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception&) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

// TextureView functions
GfxResult WebGPUBackend::textureViewDestroy(GfxTextureView textureView) const
{
    if (!textureView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    delete converter::toNative<TextureView>(textureView);
    return GFX_RESULT_SUCCESS;
}

// Sampler functions
GfxResult WebGPUBackend::samplerDestroy(GfxSampler sampler) const
{
    if (!sampler) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    delete converter::toNative<Sampler>(sampler);
    return GFX_RESULT_SUCCESS;
}

// Shader functions
GfxResult WebGPUBackend::shaderDestroy(GfxShader shader) const
{
    if (!shader) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    delete converter::toNative<Shader>(shader);
    return GFX_RESULT_SUCCESS;
}

// BindGroupLayout functions
GfxResult WebGPUBackend::bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout) const
{
    if (!bindGroupLayout) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    delete converter::toNative<BindGroupLayout>(bindGroupLayout);
    return GFX_RESULT_SUCCESS;
}

// BindGroup functions
GfxResult WebGPUBackend::bindGroupDestroy(GfxBindGroup bindGroup) const
{
    if (!bindGroup) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    delete converter::toNative<BindGroup>(bindGroup);
    return GFX_RESULT_SUCCESS;
}

// RenderPipeline functions
GfxResult WebGPUBackend::renderPipelineDestroy(GfxRenderPipeline renderPipeline) const
{
    if (!renderPipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    delete converter::toNative<RenderPipeline>(renderPipeline);
    return GFX_RESULT_SUCCESS;
}

// ComputePipeline functions
GfxResult WebGPUBackend::computePipelineDestroy(GfxComputePipeline computePipeline) const
{
    if (!computePipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    delete converter::toNative<ComputePipeline>(computePipeline);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::renderPassDestroy(GfxRenderPass renderPass) const
{
    if (!renderPass) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    delete converter::toNative<RenderPass>(renderPass);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::framebufferDestroy(GfxFramebuffer framebuffer) const
{
    if (!framebuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    delete converter::toNative<Framebuffer>(framebuffer);
    return GFX_RESULT_SUCCESS;
}

// Queue functions
GfxResult WebGPUBackend::queueSubmit(GfxQueue queue, const GfxSubmitInfo* submitInfo) const
{
    if (!queue || !submitInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* queuePtr = converter::toNative<Queue>(queue);
    auto submit = converter::gfxDescriptorToWebGPUSubmitInfo(submitInfo);

    return queuePtr->submit(submit) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

GfxResult WebGPUBackend::queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size) const
{
    if (!queue || !buffer || !data) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* queuePtr = converter::toNative<Queue>(queue);
    auto* bufferPtr = converter::toNative<Buffer>(buffer);

    queuePtr->writeBuffer(bufferPtr, offset, data, size);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
    const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
{
    if (!queue || !texture || !origin || !extent || !data) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* queuePtr = converter::toNative<Queue>(queue);
    auto* texturePtr = converter::toNative<Texture>(texture);

    WGPUOrigin3D wgpuOrigin = converter::gfxOrigin3DToWGPUOrigin3D(origin);
    WGPUExtent3D wgpuExtent = converter::gfxExtent3DToWGPUExtent3D(extent);

    queuePtr->writeTexture(texturePtr, mipLevel, wgpuOrigin, data, dataSize, bytesPerRow, wgpuExtent);

    (void)finalLayout; // WebGPU handles layout transitions automatically
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::queueWaitIdle(GfxQueue queue) const
{
    if (!queue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* queuePtr = converter::toNative<Queue>(queue);
    return queuePtr->waitIdle() ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

// CommandEncoder functions
GfxResult WebGPUBackend::commandEncoderDestroy(GfxCommandEncoder commandEncoder) const
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    delete converter::toNative<CommandEncoder>(commandEncoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::commandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder,
    const GfxRenderPassBeginDescriptor* beginDescriptor,
    GfxRenderPassEncoder* outRenderPass) const
{
    if (!commandEncoder || !beginDescriptor || !outRenderPass || !beginDescriptor->framebuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* encoderPtr = converter::toNative<CommandEncoder>(commandEncoder);
        auto* renderPass = converter::toNative<RenderPass>(beginDescriptor->renderPass);
        auto* framebuffer = converter::toNative<Framebuffer>(beginDescriptor->framebuffer);
        auto beginInfo = converter::gfxRenderPassBeginDescriptorToBeginInfo(beginDescriptor);
        auto* renderPassEncoder = new RenderPassEncoder(encoderPtr, renderPass, framebuffer, beginInfo);
        *outRenderPass = converter::toGfx<GfxRenderPassEncoder>(renderPassEncoder);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "Failed to begin render pass: %s\n", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::commandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const GfxComputePassBeginDescriptor* beginDescriptor, GfxComputePassEncoder* outComputePass) const
{
    if (!commandEncoder || !beginDescriptor || !outComputePass) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* encoderPtr = converter::toNative<CommandEncoder>(commandEncoder);
        auto createInfo = converter::gfxComputePassBeginDescriptorToCreateInfo(beginDescriptor);
        auto* computePassEncoder = new ComputePassEncoder(encoderPtr, createInfo);
        *outComputePass = converter::toGfx<GfxComputePassEncoder>(computePassEncoder);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception&) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult WebGPUBackend::commandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder, GfxBuffer source, uint64_t sourceOffset,
    GfxBuffer destination, uint64_t destinationOffset, uint64_t size) const
{
    if (!commandEncoder || !source || !destination) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<CommandEncoder>(commandEncoder);
    auto* srcPtr = converter::toNative<Buffer>(source);
    auto* dstPtr = converter::toNative<Buffer>(destination);

    encoderPtr->copyBufferToBuffer(srcPtr, sourceOffset, dstPtr, destinationOffset, size);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder, GfxBuffer source, uint64_t sourceOffset, uint32_t bytesPerRow,
    GfxTexture destination, const GfxOrigin3D* origin, const GfxExtent3D* extent, uint32_t mipLevel, GfxTextureLayout finalLayout) const
{
    if (!commandEncoder || !source || !destination || !origin || !extent) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<CommandEncoder>(commandEncoder);
    auto* srcPtr = converter::toNative<Buffer>(source);
    auto* dstPtr = converter::toNative<Texture>(destination);

    WGPUOrigin3D wgpuOrigin = converter::gfxOrigin3DToWGPUOrigin3D(origin);
    WGPUExtent3D wgpuExtent = converter::gfxExtent3DToWGPUExtent3D(extent);

    encoderPtr->copyBufferToTexture(srcPtr, sourceOffset, bytesPerRow, dstPtr,
        wgpuOrigin, wgpuExtent, mipLevel);

    (void)finalLayout; // WebGPU handles layout transitions automatically
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder, GfxTexture source, const GfxOrigin3D* origin, uint32_t mipLevel,
    GfxBuffer destination, uint64_t destinationOffset, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
{
    if (!commandEncoder || !source || !destination || !origin || !extent) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<CommandEncoder>(commandEncoder);
    auto* srcPtr = converter::toNative<Texture>(source);
    auto* dstPtr = converter::toNative<Buffer>(destination);

    WGPUOrigin3D wgpuOrigin = converter::gfxOrigin3DToWGPUOrigin3D(origin);
    WGPUExtent3D wgpuExtent = converter::gfxExtent3DToWGPUExtent3D(extent);

    encoderPtr->copyTextureToBuffer(srcPtr, wgpuOrigin,
        mipLevel, dstPtr, destinationOffset, bytesPerRow, wgpuExtent);

    (void)finalLayout; // WebGPU handles layout transitions automatically
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::commandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder, GfxTexture source, const GfxOrigin3D* sourceOrigin, uint32_t sourceMipLevel,
    GfxTexture destination, const GfxOrigin3D* destinationOrigin, uint32_t destinationMipLevel, const GfxExtent3D* extent,
    GfxTextureLayout srcFinalLayout, GfxTextureLayout dstFinalLayout) const
{
    if (!commandEncoder || !source || !destination || !sourceOrigin || !destinationOrigin || !extent) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<CommandEncoder>(commandEncoder);
    auto* srcPtr = converter::toNative<Texture>(source);
    auto* dstPtr = converter::toNative<Texture>(destination);

    WGPUOrigin3D wgpuSrcOrigin = converter::gfxOrigin3DToWGPUOrigin3D(sourceOrigin);
    WGPUOrigin3D wgpuDstOrigin = converter::gfxOrigin3DToWGPUOrigin3D(destinationOrigin);
    WGPUExtent3D wgpuExtent = converter::gfxExtent3DToWGPUExtent3D(extent);

    encoderPtr->copyTextureToTexture(srcPtr, wgpuSrcOrigin,
        sourceMipLevel, dstPtr, wgpuDstOrigin,
        destinationMipLevel, wgpuExtent);

    (void)srcFinalLayout; // WebGPU handles layout transitions automatically
    (void)dstFinalLayout;
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::commandEncoderBlitTextureToTexture(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* sourceOrigin, const GfxExtent3D* sourceExtent, uint32_t sourceMipLevel,
    GfxTexture destination, const GfxOrigin3D* destinationOrigin, const GfxExtent3D* destinationExtent, uint32_t destinationMipLevel,
    GfxFilterMode filter, GfxTextureLayout srcFinalLayout, GfxTextureLayout dstFinalLayout) const
{
    auto* encoder = toNative<CommandEncoder>(commandEncoder);
    auto* srcTexture = toNative<Texture>(source);
    auto* dstTexture = toNative<Texture>(destination);

    WGPUOrigin3D wgpuSrcOrigin = converter::gfxOrigin3DToWGPUOrigin3D(sourceOrigin);
    WGPUOrigin3D wgpuDstOrigin = converter::gfxOrigin3DToWGPUOrigin3D(destinationOrigin);
    WGPUExtent3D wgpuSrcExtent = converter::gfxExtent3DToWGPUExtent3D(sourceExtent);
    WGPUExtent3D wgpuDstExtent = converter::gfxExtent3DToWGPUExtent3D(destinationExtent);
    WGPUFilterMode wgpuFilter = converter::gfxFilterModeToWGPU(filter);

    encoder->blitTextureToTexture(srcTexture, wgpuSrcOrigin, wgpuSrcExtent,
        sourceMipLevel, dstTexture, wgpuDstOrigin, wgpuDstExtent,
        destinationMipLevel, wgpuFilter);

    // WebGPU handles layout transitions automatically
    (void)srcFinalLayout;
    (void)dstFinalLayout;
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::commandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder,
    const GfxMemoryBarrier* memoryBarriers, uint32_t memoryBarrierCount,
    const GfxBufferBarrier* bufferBarriers, uint32_t bufferBarrierCount,
    const GfxTextureBarrier* textureBarriers, uint32_t textureBarrierCount) const
{
    // WebGPU handles synchronization and layout transitions automatically
    // This is a no-op for WebGPU backend
    (void)commandEncoder;
    (void)memoryBarriers;
    (void)memoryBarrierCount;
    (void)bufferBarriers;
    (void)bufferBarrierCount;
    (void)textureBarriers;
    (void)textureBarrierCount;
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::commandEncoderGenerateMipmaps(GfxCommandEncoder commandEncoder, GfxTexture texture) const
{
    if (!commandEncoder || !texture) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoder = converter::toNative<CommandEncoder>(commandEncoder);
    auto* tex = converter::toNative<Texture>(texture);

    tex->generateMipmaps(encoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::commandEncoderGenerateMipmapsRange(GfxCommandEncoder commandEncoder, GfxTexture texture,
    uint32_t baseMipLevel, uint32_t levelCount) const
{
    if (!commandEncoder || !texture) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoder = converter::toNative<CommandEncoder>(commandEncoder);
    auto* tex = converter::toNative<Texture>(texture);

    tex->generateMipmapsRange(encoder, baseMipLevel, levelCount);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::commandEncoderEnd(GfxCommandEncoder commandEncoder) const
{
    (void)commandEncoder; // Parameter unused - handled in queueSubmit
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::commandEncoderBegin(GfxCommandEncoder commandEncoder) const
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<CommandEncoder>(commandEncoder);

    // WebGPU encoders can't be reused after Finish() - recreate if needed
    if (!encoderPtr->recreateIfNeeded()) {
        fprintf(stderr, "[WebGPU ERROR] Failed to recreate command encoder\n");
        return GFX_RESULT_ERROR_UNKNOWN;
    }
    return GFX_RESULT_SUCCESS;
}

// RenderPassEncoder functions
void WebGPUBackend::renderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline) const
{
    if (!renderPassEncoder || !pipeline) {
        return;
    }

    auto* encoderPtr = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    auto* pipelinePtr = converter::toNative<RenderPipeline>(pipeline);

    encoderPtr->setPipeline(pipelinePtr->handle());
}

void WebGPUBackend::renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    if (!renderPassEncoder || !bindGroup) {
        return;
    }

    auto* encoderPtr = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    auto* bindGroupPtr = converter::toNative<BindGroup>(bindGroup);

    encoderPtr->setBindGroup(index, bindGroupPtr->handle(), dynamicOffsets, dynamicOffsetCount);
}

void WebGPUBackend::renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size) const
{
    if (!renderPassEncoder || !buffer) {
        return;
    }

    auto* encoderPtr = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    auto* bufferPtr = converter::toNative<Buffer>(buffer);

    encoderPtr->setVertexBuffer(slot, bufferPtr->handle(), offset, size);
}

void WebGPUBackend::renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size) const
{
    if (!renderPassEncoder || !buffer) {
        return;
    }

    auto* encoderPtr = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    auto* bufferPtr = converter::toNative<Buffer>(buffer);

    encoderPtr->setIndexBuffer(bufferPtr->handle(), converter::gfxIndexFormatToWGPU(format), offset, size);
}

void WebGPUBackend::renderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport) const
{
    if (!renderPassEncoder || !viewport) {
        return;
    }

    auto* encoderPtr = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    encoderPtr->setViewport(viewport->x, viewport->y, viewport->width, viewport->height,
        viewport->minDepth, viewport->maxDepth);
}

void WebGPUBackend::renderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor) const
{
    if (!renderPassEncoder || !scissor) {
        return;
    }

    auto* encoderPtr = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    encoderPtr->setScissorRect(scissor->x, scissor->y, scissor->width, scissor->height);
}

void WebGPUBackend::renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
{
    if (!renderPassEncoder) {
        return;
    }

    auto* encoderPtr = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    encoderPtr->draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void WebGPUBackend::renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) const
{
    if (!renderPassEncoder) {
        return;
    }

    auto* encoderPtr = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    encoderPtr->drawIndexed(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}

void WebGPUBackend::renderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder) const
{
    if (!renderPassEncoder) {
        return;
    }

    auto* encoderPtr = converter::toNative<RenderPassEncoder>(renderPassEncoder);
    delete encoderPtr;
}

// ComputePassEncoder functions
void WebGPUBackend::computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline) const
{
    if (!computePassEncoder || !pipeline) {
        return;
    }

    auto* encoderPtr = converter::toNative<ComputePassEncoder>(computePassEncoder);
    auto* pipelinePtr = converter::toNative<ComputePipeline>(pipeline);

    encoderPtr->setPipeline(pipelinePtr->handle());
}

void WebGPUBackend::computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    if (!computePassEncoder || !bindGroup) {
        return;
    }

    auto* encoderPtr = converter::toNative<ComputePassEncoder>(computePassEncoder);
    auto* bindGroupPtr = converter::toNative<BindGroup>(bindGroup);

    encoderPtr->setBindGroup(index, bindGroupPtr->handle(), dynamicOffsets, dynamicOffsetCount);
}

void WebGPUBackend::computePassEncoderDispatchWorkgroups(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) const
{
    if (!computePassEncoder) {
        return;
    }

    auto* encoderPtr = converter::toNative<ComputePassEncoder>(computePassEncoder);
    encoderPtr->dispatchWorkgroups(workgroupCountX, workgroupCountY, workgroupCountZ);
}

void WebGPUBackend::computePassEncoderEnd(GfxComputePassEncoder computePassEncoder) const
{
    if (!computePassEncoder) {
        return;
    }

    auto* encoderPtr = converter::toNative<ComputePassEncoder>(computePassEncoder);
    delete encoderPtr;
}

// Fence functions
GfxResult WebGPUBackend::fenceDestroy(GfxFence fence) const
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    delete converter::toNative<Fence>(fence);
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::fenceGetStatus(GfxFence fence, bool* isSignaled) const
{
    if (!fence || !isSignaled) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* fencePtr = converter::toNative<Fence>(fence);
    *isSignaled = fencePtr->isSignaled();
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::fenceWait(GfxFence fence, uint64_t timeoutNs) const
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* fencePtr = converter::toNative<Fence>(fence);

    // Fence is already properly signaled by queueSubmit after GPU work completes
    // Just check the status
    (void)timeoutNs; // Timeout is handled in queueSubmit

    return fencePtr->isSignaled() ? GFX_RESULT_SUCCESS : GFX_RESULT_TIMEOUT;
}

void WebGPUBackend::fenceReset(GfxFence fence) const
{
    if (!fence) {
        return;
    }

    auto* fencePtr = converter::toNative<Fence>(fence);
    fencePtr->setSignaled(false);
}

// Semaphore functions
GfxResult WebGPUBackend::semaphoreDestroy(GfxSemaphore semaphore) const
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    delete converter::toNative<Semaphore>(semaphore);
    return GFX_RESULT_SUCCESS;
}

GfxSemaphoreType WebGPUBackend::semaphoreGetType(GfxSemaphore semaphore) const
{
    if (!semaphore) {
        return GFX_SEMAPHORE_TYPE_BINARY;
    }
    auto type = converter::toNative<Semaphore>(semaphore)->getType();
    return converter::webgpuSemaphoreTypeToGfxSemaphoreType(type);
}

GfxResult WebGPUBackend::semaphoreSignal(GfxSemaphore semaphore, uint64_t value) const
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* semaphorePtr = converter::toNative<Semaphore>(semaphore);
    if (semaphorePtr->getType() == gfx::webgpu::SemaphoreType::Timeline) {
        semaphorePtr->setValue(value);
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs) const
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    (void)timeoutNs; // WebGPU doesn't support timeout for semaphore waits

    auto* semaphorePtr = converter::toNative<Semaphore>(semaphore);
    if (semaphorePtr->getType() == gfx::webgpu::SemaphoreType::Timeline) {
        return (semaphorePtr->getValue() >= value) ? GFX_RESULT_SUCCESS : GFX_RESULT_TIMEOUT;
    }
    return GFX_RESULT_SUCCESS;
}

uint64_t WebGPUBackend::semaphoreGetValue(GfxSemaphore semaphore) const
{
    if (!semaphore) {
        return 0;
    }
    return converter::toNative<Semaphore>(semaphore)->getValue();
}

GfxAccessFlags WebGPUBackend::getAccessFlagsForLayout(GfxTextureLayout layout) const
{
    // WebGPU doesn't use explicit access flags - synchronization is implicit
    (void)layout;
    return GFX_ACCESS_NONE;
}

const IBackend* WebGPUBackend::create()
{
    static WebGPUBackend webgpuBackend;
    return &webgpuBackend;
}

} // namespace gfx::webgpu