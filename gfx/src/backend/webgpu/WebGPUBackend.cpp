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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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

void WebGPUBackend::instanceDestroy(GfxInstance instance) const
{
    if (!instance) {
        return;
    }

    // Process any remaining events before destroying the instance
    // This ensures all pending callbacks are completed
    auto* inst = converter::toNative<Instance>(instance);
    if (inst->handle()) {
        wgpuInstanceProcessEvents(inst->handle());
    }

    delete inst;
}

void WebGPUBackend::instanceSetDebugCallback(GfxInstance instance, GfxDebugCallback callback, void* userData) const
{
    // TODO: Implement debug callback using WebGPU error handling
    (void)instance;
    (void)callback;
    (void)userData;
}

GfxResult WebGPUBackend::instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter) const
{
    if (!instance || !outAdapter) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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

uint32_t WebGPUBackend::instanceEnumerateAdapters(GfxInstance instance, GfxAdapter* adapters, uint32_t maxAdapters) const
{
    if (!instance) {
        return 0;
    }

    auto* inst = converter::toNative<Instance>(instance);
    return gfx::webgpu::Adapter::enumerate(inst, reinterpret_cast<Adapter**>(adapters), maxAdapters);
}

// Adapter functions
void WebGPUBackend::adapterDestroy(GfxAdapter adapter) const
{
    if (!adapter) {
        return;
    }
    delete converter::toNative<Adapter>(adapter);
}

GfxResult WebGPUBackend::adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice) const
{
    if (!adapter || !outDevice) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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

const char* WebGPUBackend::adapterGetName(GfxAdapter adapter) const
{
    if (!adapter) {
        return nullptr;
    }
    auto* adapterPtr = converter::toNative<Adapter>(adapter);
    return adapterPtr->getName();
}

GfxBackend WebGPUBackend::adapterGetBackend(GfxAdapter adapter) const
{
    (void)adapter;
    return GFX_BACKEND_WEBGPU;
}

void WebGPUBackend::adapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits) const
{
    if (!adapter || !outLimits) {
        return;
    }

    auto* adapterPtr = converter::toNative<Adapter>(adapter);
    *outLimits = converter::wgpuLimitsToGfxDeviceLimits(adapterPtr->getLimits());
}

// Device functions
void WebGPUBackend::deviceDestroy(GfxDevice device) const
{
    if (!device) {
        return;
    }
    delete converter::toNative<Device>(device);
}

GfxQueue WebGPUBackend::deviceGetQueue(GfxDevice device) const
{
    if (!device) {
        return nullptr;
    }
    auto* devicePtr = converter::toNative<Device>(device);
    return converter::toGfx<GfxQueue>(devicePtr->getQueue());
}

GfxResult WebGPUBackend::deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface) const
{
    if (!device || !descriptor || !outSurface) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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

GfxResult WebGPUBackend::deviceCreateCommandEncoder(GfxDevice device, const GfxCommandEncoderDescriptor* descriptor, GfxCommandEncoder* outEncoder) const
{
    if (!device || !descriptor || !outEncoder) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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

void WebGPUBackend::deviceWaitIdle(GfxDevice device) const
{
    if (!device) {
        return;
    }
    auto* devicePtr = converter::toNative<Device>(device);
    devicePtr->waitIdle();
}

void WebGPUBackend::deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits) const
{
    if (!device || !outLimits) {
        return;
    }

    auto* devicePtr = converter::toNative<Device>(device);
    *outLimits = converter::wgpuLimitsToGfxDeviceLimits(devicePtr->getLimits());
}

// Surface functions
void WebGPUBackend::surfaceDestroy(GfxSurface surface) const
{
    if (!surface) {
        return;
    }
    delete converter::toNative<Surface>(surface);
}

uint32_t WebGPUBackend::surfaceGetSupportedFormats(GfxSurface surface, GfxTextureFormat* formats, uint32_t maxFormats) const
{
    if (!surface) {
        return 0;
    }

    auto* surf = converter::toNative<Surface>(surface);

    // Query surface capabilities
    WGPUSurfaceCapabilities capabilities = surf->getCapabilities();

    uint32_t formatCount = static_cast<uint32_t>(capabilities.formatCount);

    if (formatCount == 0) {
        wgpuSurfaceCapabilitiesFreeMembers(capabilities);
        return 0;
    }

    // Convert to GfxTextureFormat
    if (formats && maxFormats > 0) {
        uint32_t copyCount = std::min(formatCount, maxFormats);
        for (uint32_t i = 0; i < copyCount; ++i) {
            formats[i] = converter::wgpuFormatToGfxFormat(capabilities.formats[i]);
        }
    }

    wgpuSurfaceCapabilitiesFreeMembers(capabilities);
    return formatCount;
}

uint32_t WebGPUBackend::surfaceGetSupportedPresentModes(GfxSurface surface, GfxPresentMode* presentModes, uint32_t maxModes) const
{
    if (!surface) {
        return 0;
    }

    auto* surf = converter::toNative<Surface>(surface);

    // Query surface capabilities
    WGPUSurfaceCapabilities capabilities = surf->getCapabilities();

    uint32_t modeCount = static_cast<uint32_t>(capabilities.presentModeCount);

    if (modeCount == 0) {
        wgpuSurfaceCapabilitiesFreeMembers(capabilities);
        return 0;
    }

    // Convert to GfxPresentMode
    if (presentModes && maxModes > 0) {
        uint32_t copyCount = std::min(modeCount, maxModes);
        for (uint32_t i = 0; i < copyCount; ++i) {
            presentModes[i] = converter::wgpuPresentModeToGfxPresentMode(capabilities.presentModes[i]);
        }
    }

    wgpuSurfaceCapabilitiesFreeMembers(capabilities);
    return modeCount;
}

// Swapchain functions
void WebGPUBackend::swapchainDestroy(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return;
    }
    delete converter::toNative<Swapchain>(swapchain);
}

uint32_t WebGPUBackend::swapchainGetWidth(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return 0;
    }
    return converter::toNative<Swapchain>(swapchain)->getWidth();
}

uint32_t WebGPUBackend::swapchainGetHeight(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return 0;
    }
    return converter::toNative<Swapchain>(swapchain)->getHeight();
}

GfxTextureFormat WebGPUBackend::swapchainGetFormat(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
    auto* swapchainPtr = converter::toNative<Swapchain>(swapchain);
    return converter::wgpuFormatToGfxFormat(swapchainPtr->getFormat());
}

uint32_t WebGPUBackend::swapchainGetImageCount(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return 0;
    }
    return converter::toNative<Swapchain>(swapchain)->getImageCount();
}

GfxResult WebGPUBackend::swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const
{
    if (!swapchain || !outImageIndex) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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

GfxTextureView WebGPUBackend::swapchainGetImageView(GfxSwapchain swapchain, uint32_t imageIndex) const
{
    if (!swapchain) {
        return nullptr;
    }

    // WebGPU doesn't expose multiple swapchain images by index
    // Always return the current texture view regardless of index
    (void)imageIndex;

    return swapchainGetCurrentTextureView(swapchain);
}

GfxTextureView WebGPUBackend::swapchainGetCurrentTextureView(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return nullptr;
    }

    auto* swapchainPtr = converter::toNative<Swapchain>(swapchain);
    return converter::toGfx<GfxTextureView>(swapchainPtr->getCurrentTextureView());
}

GfxResult WebGPUBackend::swapchainPresent(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo) const
{
    if (!swapchain) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    // WebGPU doesn't support explicit wait semaphores for present
    // The queue submission already ensures ordering, so we just present
    (void)presentInfo; // Wait semaphores are noted but not used in WebGPU

    auto* swapchainPtr = converter::toNative<Swapchain>(swapchain);
    swapchainPtr->present();

    return GFX_RESULT_SUCCESS;
}

// Buffer functions
void WebGPUBackend::bufferDestroy(GfxBuffer buffer) const
{
    if (!buffer) {
        return;
    }
    delete converter::toNative<Buffer>(buffer);
}

uint64_t WebGPUBackend::bufferGetSize(GfxBuffer buffer) const
{
    if (!buffer) {
        return 0;
    }
    return converter::toNative<Buffer>(buffer)->getSize();
}

GfxBufferUsage WebGPUBackend::bufferGetUsage(GfxBuffer buffer) const
{
    if (!buffer) {
        return GFX_BUFFER_USAGE_NONE;
    }
    auto usage = converter::toNative<Buffer>(buffer)->getUsage();
    return converter::webgpuBufferUsageToGfxBufferUsage(usage);
}

GfxResult WebGPUBackend::bufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer) const
{
    if (!buffer || !outMappedPointer) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* bufferPtr = converter::toNative<Buffer>(buffer);
    void* mappedData = bufferPtr->map(offset, size);

    if (!mappedData) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    *outMappedPointer = mappedData;
    return GFX_RESULT_SUCCESS;
}

void WebGPUBackend::bufferUnmap(GfxBuffer buffer) const
{
    if (!buffer) {
        return;
    }
    auto* bufferPtr = converter::toNative<Buffer>(buffer);
    bufferPtr->unmap();
}

// Texture functions
void WebGPUBackend::textureDestroy(GfxTexture texture) const
{
    if (!texture) {
        return;
    }
    delete converter::toNative<Texture>(texture);
}

GfxExtent3D WebGPUBackend::textureGetSize(GfxTexture texture) const
{
    if (!texture) {
        return { 0, 0, 0 };
    }
    auto* texturePtr = converter::toNative<Texture>(texture);
    WGPUExtent3D size = texturePtr->getSize();
    return { size.width, size.height, size.depthOrArrayLayers };
}

GfxTextureFormat WebGPUBackend::textureGetFormat(GfxTexture texture) const
{
    if (!texture) {
        return GFX_TEXTURE_FORMAT_UNDEFINED;
    }
    auto* texturePtr = converter::toNative<Texture>(texture);
    return converter::wgpuFormatToGfxFormat(texturePtr->getFormat());
}

uint32_t WebGPUBackend::textureGetMipLevelCount(GfxTexture texture) const
{
    if (!texture) {
        return 0;
    }
    return converter::toNative<Texture>(texture)->getMipLevels();
}

GfxSampleCount WebGPUBackend::textureGetSampleCount(GfxTexture texture) const
{
    if (!texture) {
        return GFX_SAMPLE_COUNT_1;
    }
    auto* texturePtr = converter::toNative<Texture>(texture);
    return converter::wgpuSampleCountToGfxSampleCount(texturePtr->getSampleCount());
}

GfxTextureUsage WebGPUBackend::textureGetUsage(GfxTexture texture) const
{
    if (!texture) {
        return GFX_TEXTURE_USAGE_NONE;
    }

    auto* texturePtr = converter::toNative<Texture>(texture);
    return converter::wgpuTextureUsageToGfxTextureUsage(texturePtr->getUsage());
}

GfxTextureLayout WebGPUBackend::textureGetLayout(GfxTexture texture) const
{
    // WebGPU doesn't have explicit layouts, return GENERAL as a reasonable default
    if (!texture) {
        return GFX_TEXTURE_LAYOUT_UNDEFINED;
    }
    return GFX_TEXTURE_LAYOUT_GENERAL;
}

GfxResult WebGPUBackend::textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView) const
{
    if (!texture || !outView) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
void WebGPUBackend::textureViewDestroy(GfxTextureView textureView) const
{
    if (!textureView) {
        return;
    }
    delete converter::toNative<TextureView>(textureView);
}

// Sampler functions
void WebGPUBackend::samplerDestroy(GfxSampler sampler) const
{
    if (!sampler) {
        return;
    }
    delete converter::toNative<Sampler>(sampler);
}

// Shader functions
void WebGPUBackend::shaderDestroy(GfxShader shader) const
{
    if (!shader) {
        return;
    }
    delete converter::toNative<Shader>(shader);
}

// BindGroupLayout functions
void WebGPUBackend::bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout) const
{
    if (!bindGroupLayout) {
        return;
    }
    delete converter::toNative<BindGroupLayout>(bindGroupLayout);
}

// BindGroup functions
void WebGPUBackend::bindGroupDestroy(GfxBindGroup bindGroup) const
{
    if (!bindGroup) {
        return;
    }
    delete converter::toNative<BindGroup>(bindGroup);
}

// RenderPipeline functions
void WebGPUBackend::renderPipelineDestroy(GfxRenderPipeline renderPipeline) const
{
    if (!renderPipeline) {
        return;
    }
    delete converter::toNative<RenderPipeline>(renderPipeline);
}

// ComputePipeline functions
void WebGPUBackend::computePipelineDestroy(GfxComputePipeline computePipeline) const
{
    if (!computePipeline) {
        return;
    }
    delete converter::toNative<ComputePipeline>(computePipeline);
}

// Queue functions
GfxResult WebGPUBackend::queueSubmit(GfxQueue queue, const GfxSubmitInfo* submitInfo) const
{
    if (!queue || !submitInfo) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* queuePtr = converter::toNative<Queue>(queue);
    auto submit = converter::gfxDescriptorToWebGPUSubmitInfo(submitInfo);

    return queuePtr->submit(submit) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

void WebGPUBackend::queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size) const
{
    if (!queue || !buffer || !data) {
        return;
    }

    auto* queuePtr = converter::toNative<Queue>(queue);
    auto* bufferPtr = converter::toNative<Buffer>(buffer);

    queuePtr->writeBuffer(bufferPtr, offset, data, size);
}

void WebGPUBackend::queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
    const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
{
    if (!queue || !texture || !origin || !extent || !data) {
        return;
    }

    auto* queuePtr = converter::toNative<Queue>(queue);
    auto* texturePtr = converter::toNative<Texture>(texture);

    WGPUOrigin3D wgpuOrigin = converter::gfxOrigin3DToWGPUOrigin3D(origin);
    WGPUExtent3D wgpuExtent = converter::gfxExtent3DToWGPUExtent3D(extent);

    queuePtr->writeTexture(texturePtr, mipLevel, wgpuOrigin, data, dataSize, bytesPerRow, wgpuExtent);

    (void)finalLayout; // WebGPU handles layout transitions automatically
}

GfxResult WebGPUBackend::queueWaitIdle(GfxQueue queue) const
{
    if (!queue) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* queuePtr = converter::toNative<Queue>(queue);
    return queuePtr->waitIdle() ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

// CommandEncoder functions
void WebGPUBackend::commandEncoderDestroy(GfxCommandEncoder commandEncoder) const
{
    if (!commandEncoder) {
        return;
    }
    delete converter::toNative<CommandEncoder>(commandEncoder);
}

GfxResult WebGPUBackend::commandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder,
    const GfxRenderPassDescriptor* descriptor,
    GfxRenderPassEncoder* outRenderPass) const
{
    if (!commandEncoder || !descriptor || !outRenderPass) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* encoderPtr = converter::toNative<CommandEncoder>(commandEncoder);
        auto createInfo = converter::gfxRenderPassDescriptorToCreateInfo(descriptor);
        RenderPassEncoder* renderPassEncoder = new RenderPassEncoder(encoderPtr, createInfo);
        *outRenderPass = converter::toGfx<GfxRenderPassEncoder>(renderPassEncoder);
    } catch (const std::exception&) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::commandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const GfxComputePassDescriptor* descriptor, GfxComputePassEncoder* outComputePass) const
{
    if (!commandEncoder || !descriptor || !outComputePass) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    try {
        auto* encoderPtr = converter::toNative<CommandEncoder>(commandEncoder);
        auto createInfo = converter::gfxComputePassDescriptorToCreateInfo(descriptor);
        auto* computePassEncoder = new ComputePassEncoder(encoderPtr, createInfo);
        *outComputePass = converter::toGfx<GfxComputePassEncoder>(computePassEncoder);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception&) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

void WebGPUBackend::commandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder, GfxBuffer source, uint64_t sourceOffset,
    GfxBuffer destination, uint64_t destinationOffset, uint64_t size) const
{
    if (!commandEncoder || !source || !destination) {
        return;
    }

    auto* encoderPtr = converter::toNative<CommandEncoder>(commandEncoder);
    auto* srcPtr = converter::toNative<Buffer>(source);
    auto* dstPtr = converter::toNative<Buffer>(destination);

    encoderPtr->copyBufferToBuffer(srcPtr, sourceOffset, dstPtr, destinationOffset, size);
}

void WebGPUBackend::commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder, GfxBuffer source, uint64_t sourceOffset, uint32_t bytesPerRow,
    GfxTexture destination, const GfxOrigin3D* origin, const GfxExtent3D* extent, uint32_t mipLevel, GfxTextureLayout finalLayout) const
{
    if (!commandEncoder || !source || !destination || !origin || !extent) {
        return;
    }

    auto* encoderPtr = converter::toNative<CommandEncoder>(commandEncoder);
    auto* srcPtr = converter::toNative<Buffer>(source);
    auto* dstPtr = converter::toNative<Texture>(destination);

    WGPUOrigin3D wgpuOrigin = converter::gfxOrigin3DToWGPUOrigin3D(origin);
    WGPUExtent3D wgpuExtent = converter::gfxExtent3DToWGPUExtent3D(extent);

    encoderPtr->copyBufferToTexture(srcPtr, sourceOffset, bytesPerRow, dstPtr,
        wgpuOrigin, wgpuExtent, mipLevel);

    (void)finalLayout; // WebGPU handles layout transitions automatically
}

void WebGPUBackend::commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder, GfxTexture source, const GfxOrigin3D* origin, uint32_t mipLevel,
    GfxBuffer destination, uint64_t destinationOffset, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
{
    if (!commandEncoder || !source || !destination || !origin || !extent) {
        return;
    }

    auto* encoderPtr = converter::toNative<CommandEncoder>(commandEncoder);
    auto* srcPtr = converter::toNative<Texture>(source);
    auto* dstPtr = converter::toNative<Buffer>(destination);

    WGPUOrigin3D wgpuOrigin = converter::gfxOrigin3DToWGPUOrigin3D(origin);
    WGPUExtent3D wgpuExtent = converter::gfxExtent3DToWGPUExtent3D(extent);

    encoderPtr->copyTextureToBuffer(srcPtr, wgpuOrigin,
        mipLevel, dstPtr, destinationOffset, bytesPerRow, wgpuExtent);

    (void)finalLayout; // WebGPU handles layout transitions automatically
}

void WebGPUBackend::commandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder, GfxTexture source, const GfxOrigin3D* sourceOrigin, uint32_t sourceMipLevel,
    GfxTexture destination, const GfxOrigin3D* destinationOrigin, uint32_t destinationMipLevel, const GfxExtent3D* extent,
    GfxTextureLayout srcFinalLayout, GfxTextureLayout dstFinalLayout) const
{
    if (!commandEncoder || !source || !destination || !sourceOrigin || !destinationOrigin || !extent) {
        return;
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
}

void WebGPUBackend::commandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder,
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
}

void WebGPUBackend::commandEncoderEnd(GfxCommandEncoder commandEncoder) const
{
    (void)commandEncoder; // Parameter unused - handled in queueSubmit
}

void WebGPUBackend::commandEncoderBegin(GfxCommandEncoder commandEncoder) const
{
    if (!commandEncoder) {
        return;
    }

    auto* encoderPtr = converter::toNative<CommandEncoder>(commandEncoder);

    // WebGPU encoders can't be reused after Finish() - recreate if needed
    if (!encoderPtr->recreateIfNeeded()) {
        fprintf(stderr, "[WebGPU ERROR] Failed to recreate command encoder\n");
    }
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
void WebGPUBackend::fenceDestroy(GfxFence fence) const
{
    if (!fence) {
        return;
    }
    delete converter::toNative<Fence>(fence);
}

GfxResult WebGPUBackend::fenceGetStatus(GfxFence fence, bool* isSignaled) const
{
    if (!fence || !isSignaled) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
    }

    auto* fencePtr = converter::toNative<Fence>(fence);
    *isSignaled = fencePtr->isSignaled();
    return GFX_RESULT_SUCCESS;
}

GfxResult WebGPUBackend::fenceWait(GfxFence fence, uint64_t timeoutNs) const
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
void WebGPUBackend::semaphoreDestroy(GfxSemaphore semaphore) const
{
    if (!semaphore) {
        return;
    }
    delete converter::toNative<Semaphore>(semaphore);
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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
        return GFX_RESULT_ERROR_INVALID_PARAMETER;
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