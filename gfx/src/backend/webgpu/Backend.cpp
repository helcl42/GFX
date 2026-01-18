#include "Backend.h"

#include "common/Common.h"
#include "common/Logger.h"
#include "converter/Conversions.h"

#include "core/command/CommandEncoder.h"
#include "core/command/ComputePassEncoder.h"
#include "core/command/RenderPassEncoder.h"
#include "core/compute/ComputePipeline.h"
#include "core/presentation/Surface.h"
#include "core/presentation/Swapchain.h"
#include "core/render/Framebuffer.h"
#include "core/render/RenderPass.h"
#include "core/render/RenderPipeline.h"
#include "core/resource/BindGroup.h"
#include "core/resource/BindGroupLayout.h"
#include "core/resource/Buffer.h"
#include "core/resource/Sampler.h"
#include "core/resource/Shader.h"
#include "core/resource/Texture.h"
#include "core/resource/TextureView.h"
#include "core/sync/Fence.h"
#include "core/sync/Semaphore.h"
#include "core/system/Adapter.h"
#include "core/system/Device.h"
#include "core/system/Instance.h"
#include "core/system/Queue.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace gfx::backend::webgpu {

// ============================================================================
// Backend C++ Class Export
// ============================================================================

// Instance functions
GfxResult Backend::createInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance) const
{
    if (!outInstance) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto createInfo = converter::gfxDescriptorToWebGPUInstanceCreateInfo(descriptor);
        auto* instance = new core::Instance(createInfo);
        *outInstance = converter::toGfx<GfxInstance>(instance);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create instance: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::instanceDestroy(GfxInstance instance) const
{
    if (!instance) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Process any remaining events before destroying the instance
    // This ensures all pending callbacks are completed
    auto* inst = converter::toNative<core::Instance>(instance);
    if (inst->handle()) {
        wgpuInstanceProcessEvents(inst->handle());
    }

    delete inst;
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter) const
{
    if (!instance || !outAdapter) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* inst = converter::toNative<core::Instance>(instance);
        auto createInfo = converter::gfxDescriptorToWebGPUAdapterCreateInfo(descriptor);
        auto* adapter = new core::Adapter(inst, createInfo);
        *outAdapter = converter::toGfx<GfxAdapter>(adapter);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to request adapter: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::instanceEnumerateAdapters(GfxInstance instance, uint32_t* adapterCount, GfxAdapter* adapters) const
{
    if (!instance || !adapterCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* inst = converter::toNative<core::Instance>(instance);
    uint32_t count = core::Adapter::enumerate(inst, reinterpret_cast<core::Adapter**>(adapters), adapters ? *adapterCount : 0);
    *adapterCount = count;
    return GFX_RESULT_SUCCESS;
}

// Adapter functions
GfxResult Backend::adapterDestroy(GfxAdapter adapter) const
{
    if (!adapter) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    delete converter::toNative<core::Adapter>(adapter);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice) const
{
    if (!adapter || !outDevice) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* adapterPtr = converter::toNative<core::Adapter>(adapter);
        auto createInfo = converter::gfxDescriptorToWebGPUDeviceCreateInfo(descriptor);
        auto* device = new core::Device(adapterPtr, createInfo);
        *outDevice = converter::toGfx<GfxDevice>(device);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create device: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::adapterGetInfo(GfxAdapter adapter, GfxAdapterInfo* outInfo) const
{
    if (!adapter || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* adapterPtr = converter::toNative<core::Adapter>(adapter);
    *outInfo = converter::wgpuAdapterToGfxAdapterInfo(adapterPtr->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::adapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits) const
{
    if (!adapter || !outLimits) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* adapterPtr = converter::toNative<core::Adapter>(adapter);
    *outLimits = converter::wgpuLimitsToGfxDeviceLimits(adapterPtr->getLimits());
    return GFX_RESULT_SUCCESS;
    return GFX_RESULT_SUCCESS;
}

// Device functions
GfxResult Backend::deviceDestroy(GfxDevice device) const
{
    if (!device) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    delete converter::toNative<core::Device>(device);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::deviceGetQueue(GfxDevice device, GfxQueue* outQueue) const
{
    if (!device || !outQueue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* dev = converter::toNative<core::Device>(device);
    *outQueue = converter::toGfx<GfxQueue>(dev->getQueue());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface) const
{
#ifdef GFX_HEADLESS_BUILD
    (void)device;
    (void)descriptor;
    (void)outSurface;
    gfx::common::Logger::instance().logError("Surface creation is not available in headless builds");
    return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
#else
    if (!device || !descriptor || !outSurface) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUSurfaceCreateInfo(descriptor);
        auto* surface = new core::Surface(devicePtr->getAdapter()->getInstance()->handle(), devicePtr->getAdapter()->handle(), createInfo);
        *outSurface = converter::toGfx<GfxSurface>(surface);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create surface: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
#endif
}

GfxResult Backend::deviceCreateSwapchain(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain) const
{
    if (!device || !surface || !descriptor || !outSwapchain) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto* surfacePtr = converter::toNative<core::Surface>(surface);
        auto createInfo = converter::gfxDescriptorToWebGPUSwapchainCreateInfo(descriptor);
        auto* swapchain = new core::Swapchain(devicePtr, surfacePtr, createInfo);
        *outSwapchain = converter::toGfx<GfxSwapchain>(swapchain);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create swapchain: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::deviceCreateBuffer(GfxDevice device, const GfxBufferDescriptor* descriptor, GfxBuffer* outBuffer) const
{
    if (!device || !descriptor || !outBuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUBufferCreateInfo(descriptor);
        auto* buffer = new core::Buffer(devicePtr, createInfo);
        *outBuffer = converter::toGfx<GfxBuffer>(buffer);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create buffer: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::deviceImportBuffer(GfxDevice device, const GfxBufferImportDescriptor* descriptor, GfxBuffer* outBuffer) const
{
    if (!device || !descriptor || !outBuffer || !descriptor->nativeHandle) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto wgpuBuffer = reinterpret_cast<WGPUBuffer>(descriptor->nativeHandle);
        auto importInfo = converter::gfxExternalDescriptorToWebGPUBufferImportInfo(descriptor);
        auto* buffer = new core::Buffer(devicePtr, wgpuBuffer, importInfo);
        *outBuffer = converter::toGfx<GfxBuffer>(buffer);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to import buffer: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::deviceCreateTexture(GfxDevice device, const GfxTextureDescriptor* descriptor, GfxTexture* outTexture) const
{
    if (!device || !descriptor || !outTexture) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUTextureCreateInfo(descriptor);
        auto* texture = new core::Texture(devicePtr, createInfo);
        *outTexture = converter::toGfx<GfxTexture>(texture);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create texture: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::deviceImportTexture(GfxDevice device, const GfxTextureImportDescriptor* descriptor, GfxTexture* outTexture) const
{
    if (!device || !descriptor || !outTexture || !descriptor->nativeHandle) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto wgpuTexture = reinterpret_cast<WGPUTexture>(descriptor->nativeHandle);
        auto importInfo = converter::gfxExternalDescriptorToWebGPUTextureImportInfo(descriptor);
        auto* texture = new core::Texture(devicePtr, wgpuTexture, importInfo);
        *outTexture = converter::toGfx<GfxTexture>(texture);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to import texture: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::deviceCreateSampler(GfxDevice device, const GfxSamplerDescriptor* descriptor, GfxSampler* outSampler) const
{
    if (!device || !descriptor || !outSampler) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUSamplerCreateInfo(descriptor);
        auto* sampler = new core::Sampler(devicePtr, createInfo);
        *outSampler = converter::toGfx<GfxSampler>(sampler);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create sampler: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::deviceCreateShader(GfxDevice device, const GfxShaderDescriptor* descriptor, GfxShader* outShader) const
{
    if (!device || !descriptor || !descriptor->code || !outShader) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUShaderCreateInfo(descriptor);
        auto* shader = new core::Shader(devicePtr, createInfo);
        *outShader = converter::toGfx<GfxShader>(shader);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create shader: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::deviceCreateBindGroupLayout(GfxDevice device, const GfxBindGroupLayoutDescriptor* descriptor, GfxBindGroupLayout* outLayout) const
{
    if (!device || !descriptor || !outLayout) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUBindGroupLayoutCreateInfo(descriptor);
        auto* layout = new core::BindGroupLayout(devicePtr, createInfo);
        *outLayout = converter::toGfx<GfxBindGroupLayout>(layout);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create bind group layout: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::deviceCreateBindGroup(GfxDevice device, const GfxBindGroupDescriptor* descriptor, GfxBindGroup* outBindGroup) const
{
    if (!device || !descriptor || !descriptor->layout || !outBindGroup) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto* layoutPtr = converter::toNative<core::BindGroupLayout>(descriptor->layout);
        auto createInfo = converter::gfxDescriptorToWebGPUBindGroupCreateInfo(descriptor, layoutPtr->handle());
        auto* bindGroup = new core::BindGroup(devicePtr, createInfo);
        *outBindGroup = converter::toGfx<GfxBindGroup>(bindGroup);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create bind group: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::deviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline) const
{
    if (!device || !descriptor || !outPipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPURenderPipelineCreateInfo(descriptor);
        auto* pipeline = new core::RenderPipeline(devicePtr, createInfo);
        *outPipeline = converter::toGfx<GfxRenderPipeline>(pipeline);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create render pipeline: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::deviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline) const
{
    if (!device || !descriptor || !descriptor->compute || !outPipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUComputePipelineCreateInfo(descriptor);
        auto* pipeline = new core::ComputePipeline(devicePtr, createInfo);
        *outPipeline = converter::toGfx<GfxComputePipeline>(pipeline);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create compute pipeline: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::deviceCreateRenderPass(GfxDevice device, const GfxRenderPassDescriptor* descriptor, GfxRenderPass* outRenderPass) const
{
    if (!device || !descriptor || !outRenderPass) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* dev = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxRenderPassDescriptorToRenderPassCreateInfo(descriptor);
        auto* renderPass = new core::RenderPass(dev, createInfo);
        *outRenderPass = converter::toGfx<GfxRenderPass>(renderPass);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create render pass: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::deviceCreateFramebuffer(GfxDevice device, const GfxFramebufferDescriptor* descriptor, GfxFramebuffer* outFramebuffer) const
{
    if (!device || !descriptor || !outFramebuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* dev = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxFramebufferDescriptorToFramebufferCreateInfo(descriptor);
        auto* framebuffer = new core::Framebuffer(dev, createInfo);
        *outFramebuffer = converter::toGfx<GfxFramebuffer>(framebuffer);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create framebuffer: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::deviceCreateCommandEncoder(GfxDevice device, const GfxCommandEncoderDescriptor* descriptor, GfxCommandEncoder* outEncoder) const
{
    if (!device || !descriptor || !outEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* devicePtr = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUCommandEncoderCreateInfo(descriptor);
        auto* encoder = new core::CommandEncoder(devicePtr, createInfo);
        *outEncoder = converter::toGfx<GfxCommandEncoder>(encoder);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create command encoder: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence) const
{
    if (!device || !descriptor || !outFence) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* fence = new core::Fence(descriptor->signaled);
        *outFence = converter::toGfx<GfxFence>(fence);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create fence: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::deviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore) const
{
    if (!device || !descriptor || !outSemaphore) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto semaphoreType = converter::gfxSemaphoreTypeToWebGPUSemaphoreType(descriptor->type);
        auto* semaphore = new core::Semaphore(semaphoreType, descriptor->initialValue);
        *outSemaphore = converter::toGfx<GfxSemaphore>(semaphore);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create semaphore: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::deviceWaitIdle(GfxDevice device) const
{
    if (!device) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* devicePtr = converter::toNative<core::Device>(device);
    devicePtr->waitIdle();
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits) const
{
    if (!device || !outLimits) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* devicePtr = converter::toNative<core::Device>(device);
    *outLimits = converter::wgpuLimitsToGfxDeviceLimits(devicePtr->getLimits());
    return GFX_RESULT_SUCCESS;
}

// Surface functions
GfxResult Backend::surfaceDestroy(GfxSurface surface) const
{
    if (!surface) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    delete converter::toNative<core::Surface>(surface);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::surfaceEnumerateSupportedFormats(GfxSurface surface, uint32_t* formatCount, GfxTextureFormat* formats) const
{
    if (!surface || !formatCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* surf = converter::toNative<core::Surface>(surface);

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

GfxResult Backend::surfaceEnumerateSupportedPresentModes(GfxSurface surface, uint32_t* presentModeCount, GfxPresentMode* presentModes) const
{
    if (!surface || !presentModeCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* surf = converter::toNative<core::Surface>(surface);

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
GfxResult Backend::swapchainDestroy(GfxSwapchain swapchain) const
{
    if (!swapchain) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    delete converter::toNative<core::Swapchain>(swapchain);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::swapchainGetInfo(GfxSwapchain swapchain, GfxSwapchainInfo* outInfo) const
{
    if (!swapchain || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* swapchainPtr = converter::toNative<core::Swapchain>(swapchain);
    *outInfo = converter::wgpuSwapchainInfoToGfxSwapchainInfo(swapchainPtr->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const
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

    auto* swapchainPtr = converter::toNative<core::Swapchain>(swapchain);

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
        auto* fencePtr = converter::toNative<core::Fence>(fence);
        fencePtr->setSignaled(true);
    }

    return result;
}

GfxResult Backend::swapchainGetTextureView(GfxSwapchain swapchain, uint32_t imageIndex, GfxTextureView* outView) const
{
    if (!swapchain || !outView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // WebGPU doesn't expose multiple swapchain images by index
    // Always return the current texture view regardless of index
    (void)imageIndex;

    return swapchainGetCurrentTextureView(swapchain, outView);
}

GfxResult Backend::swapchainGetCurrentTextureView(GfxSwapchain swapchain, GfxTextureView* outView) const
{
    if (!swapchain || !outView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* swapchainPtr = converter::toNative<core::Swapchain>(swapchain);
    *outView = converter::toGfx<GfxTextureView>(swapchainPtr->getCurrentTextureView());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::swapchainPresent(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo) const
{
    if (!swapchain) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // WebGPU doesn't support explicit wait semaphores for present
    // The queue submission already ensures ordering, so we just present
    (void)presentInfo; // Wait semaphores are noted but not used in WebGPU

    auto* swapchainPtr = converter::toNative<core::Swapchain>(swapchain);
    swapchainPtr->present();

    return GFX_RESULT_SUCCESS;
}

// Buffer functions
GfxResult Backend::bufferDestroy(GfxBuffer buffer) const
{
    if (!buffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    delete converter::toNative<core::Buffer>(buffer);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::bufferGetInfo(GfxBuffer buffer, GfxBufferInfo* outInfo) const
{
    if (!buffer || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* buf = converter::toNative<core::Buffer>(buffer);
    *outInfo = converter::wgpuBufferToGfxBufferInfo(buf->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::bufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer) const
{
    if (!buffer || !outMappedPointer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* bufferPtr = converter::toNative<core::Buffer>(buffer);
    void* mappedData = bufferPtr->map(offset, size);

    if (!mappedData) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    *outMappedPointer = mappedData;
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::bufferUnmap(GfxBuffer buffer) const
{
    if (!buffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* bufferPtr = converter::toNative<core::Buffer>(buffer);
    bufferPtr->unmap();
    return GFX_RESULT_SUCCESS;
}

// Texture functions
GfxResult Backend::textureDestroy(GfxTexture texture) const
{
    if (!texture) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    delete converter::toNative<core::Texture>(texture);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::textureGetInfo(GfxTexture texture, GfxTextureInfo* outInfo) const
{
    if (!texture || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* texturePtr = converter::toNative<core::Texture>(texture);
    *outInfo = converter::wgpuTextureInfoToGfxTextureInfo(texturePtr->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::textureGetLayout(GfxTexture texture, GfxTextureLayout* outLayout) const
{
    // WebGPU doesn't have explicit layouts, return GENERAL as a reasonable default
    if (!texture || !outLayout) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    *outLayout = GFX_TEXTURE_LAYOUT_GENERAL;
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView) const
{
    if (!texture || !outView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* texturePtr = converter::toNative<core::Texture>(texture);
        auto createInfo = converter::gfxDescriptorToWebGPUTextureViewCreateInfo(descriptor);
        auto* view = new core::TextureView(texturePtr, createInfo);
        *outView = converter::toGfx<GfxTextureView>(view);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create texture view: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

// TextureView functions
GfxResult Backend::textureViewDestroy(GfxTextureView textureView) const
{
    if (!textureView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    delete converter::toNative<core::TextureView>(textureView);
    return GFX_RESULT_SUCCESS;
}

// Sampler functions
GfxResult Backend::samplerDestroy(GfxSampler sampler) const
{
    if (!sampler) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    delete converter::toNative<core::Sampler>(sampler);
    return GFX_RESULT_SUCCESS;
}

// Shader functions
GfxResult Backend::shaderDestroy(GfxShader shader) const
{
    if (!shader) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    delete converter::toNative<core::Shader>(shader);
    return GFX_RESULT_SUCCESS;
}

// BindGroupLayout functions
GfxResult Backend::bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout) const
{
    if (!bindGroupLayout) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    delete converter::toNative<core::BindGroupLayout>(bindGroupLayout);
    return GFX_RESULT_SUCCESS;
}

// BindGroup functions
GfxResult Backend::bindGroupDestroy(GfxBindGroup bindGroup) const
{
    if (!bindGroup) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    delete converter::toNative<core::BindGroup>(bindGroup);
    return GFX_RESULT_SUCCESS;
}

// RenderPipeline functions
GfxResult Backend::renderPipelineDestroy(GfxRenderPipeline renderPipeline) const
{
    if (!renderPipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    delete converter::toNative<core::RenderPipeline>(renderPipeline);
    return GFX_RESULT_SUCCESS;
}

// ComputePipeline functions
GfxResult Backend::computePipelineDestroy(GfxComputePipeline computePipeline) const
{
    if (!computePipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    delete converter::toNative<core::ComputePipeline>(computePipeline);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassDestroy(GfxRenderPass renderPass) const
{
    if (!renderPass) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    delete converter::toNative<core::RenderPass>(renderPass);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::framebufferDestroy(GfxFramebuffer framebuffer) const
{
    if (!framebuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    delete converter::toNative<core::Framebuffer>(framebuffer);
    return GFX_RESULT_SUCCESS;
}

// Queue functions
GfxResult Backend::queueSubmit(GfxQueue queue, const GfxSubmitDescriptor* submitInfo) const
{
    if (!queue || !submitInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* queuePtr = converter::toNative<core::Queue>(queue);
    auto submit = converter::gfxDescriptorToWebGPUSubmitInfo(submitInfo);

    return queuePtr->submit(submit) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

GfxResult Backend::queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size) const
{
    if (!queue || !buffer || !data) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* queuePtr = converter::toNative<core::Queue>(queue);
    auto* bufferPtr = converter::toNative<core::Buffer>(buffer);

    queuePtr->writeBuffer(bufferPtr, offset, data, size);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel,
    const void* data, uint64_t dataSize, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
{
    if (!queue || !texture || !origin || !extent || !data) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* queuePtr = converter::toNative<core::Queue>(queue);
    auto* texturePtr = converter::toNative<core::Texture>(texture);

    WGPUOrigin3D wgpuOrigin = converter::gfxOrigin3DToWGPUOrigin3D(origin);
    WGPUExtent3D wgpuExtent = converter::gfxExtent3DToWGPUExtent3D(extent);

    queuePtr->writeTexture(texturePtr, mipLevel, wgpuOrigin, data, dataSize, bytesPerRow, wgpuExtent);

    (void)finalLayout; // WebGPU handles layout transitions automatically
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::queueWaitIdle(GfxQueue queue) const
{
    if (!queue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* queuePtr = converter::toNative<core::Queue>(queue);
    return queuePtr->waitIdle() ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

// CommandEncoder functions
GfxResult Backend::commandEncoderDestroy(GfxCommandEncoder commandEncoder) const
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    delete converter::toNative<core::CommandEncoder>(commandEncoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder,
    const GfxRenderPassBeginDescriptor* beginDescriptor,
    GfxRenderPassEncoder* outRenderPass) const
{
    if (!commandEncoder || !beginDescriptor || !outRenderPass || !beginDescriptor->framebuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* encoderPtr = converter::toNative<core::CommandEncoder>(commandEncoder);
        auto* renderPass = converter::toNative<core::RenderPass>(beginDescriptor->renderPass);
        auto* framebuffer = converter::toNative<core::Framebuffer>(beginDescriptor->framebuffer);
        auto beginInfo = converter::gfxRenderPassBeginDescriptorToBeginInfo(beginDescriptor);
        auto* renderPassEncoder = new core::RenderPassEncoder(encoderPtr, renderPass, framebuffer, beginInfo);
        *outRenderPass = converter::toGfx<GfxRenderPassEncoder>(renderPassEncoder);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to begin render pass: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::commandEncoderBeginComputePass(GfxCommandEncoder commandEncoder, const GfxComputePassBeginDescriptor* beginDescriptor, GfxComputePassEncoder* outComputePass) const
{
    if (!commandEncoder || !beginDescriptor || !outComputePass) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    try {
        auto* encoderPtr = converter::toNative<core::CommandEncoder>(commandEncoder);
        auto createInfo = converter::gfxComputePassBeginDescriptorToCreateInfo(beginDescriptor);
        auto* computePassEncoder = new core::ComputePassEncoder(encoderPtr, createInfo);
        *outComputePass = converter::toGfx<GfxComputePassEncoder>(computePassEncoder);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to begin compute pass: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::commandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder, GfxBuffer source, uint64_t sourceOffset,
    GfxBuffer destination, uint64_t destinationOffset, uint64_t size) const
{
    if (!commandEncoder || !source || !destination) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcPtr = converter::toNative<core::Buffer>(source);
    auto* dstPtr = converter::toNative<core::Buffer>(destination);

    encoderPtr->copyBufferToBuffer(srcPtr, sourceOffset, dstPtr, destinationOffset, size);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder, GfxBuffer source, uint64_t sourceOffset, uint32_t bytesPerRow,
    GfxTexture destination, const GfxOrigin3D* origin, const GfxExtent3D* extent, uint32_t mipLevel, GfxTextureLayout finalLayout) const
{
    if (!commandEncoder || !source || !destination || !origin || !extent) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcPtr = converter::toNative<core::Buffer>(source);
    auto* dstPtr = converter::toNative<core::Texture>(destination);

    WGPUOrigin3D wgpuOrigin = converter::gfxOrigin3DToWGPUOrigin3D(origin);
    WGPUExtent3D wgpuExtent = converter::gfxExtent3DToWGPUExtent3D(extent);

    encoderPtr->copyBufferToTexture(srcPtr, sourceOffset, bytesPerRow, dstPtr, wgpuOrigin, wgpuExtent, mipLevel);

    (void)finalLayout; // WebGPU handles layout transitions automatically
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder, GfxTexture source, const GfxOrigin3D* origin, uint32_t mipLevel,
    GfxBuffer destination, uint64_t destinationOffset, uint32_t bytesPerRow, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
{
    if (!commandEncoder || !source || !destination || !origin || !extent) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcPtr = converter::toNative<core::Texture>(source);
    auto* dstPtr = converter::toNative<core::Buffer>(destination);

    WGPUOrigin3D wgpuOrigin = converter::gfxOrigin3DToWGPUOrigin3D(origin);
    WGPUExtent3D wgpuExtent = converter::gfxExtent3DToWGPUExtent3D(extent);

    encoderPtr->copyTextureToBuffer(srcPtr, wgpuOrigin, mipLevel, dstPtr, destinationOffset, bytesPerRow, wgpuExtent);

    (void)finalLayout; // WebGPU handles layout transitions automatically
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder, GfxTexture source, const GfxOrigin3D* sourceOrigin, uint32_t sourceMipLevel, GfxTextureLayout srcFinalLayout,
    GfxTexture destination, const GfxOrigin3D* destinationOrigin, uint32_t destinationMipLevel, GfxTextureLayout dstFinalLayout, const GfxExtent3D* extent) const
{
    if (!commandEncoder || !source || !destination || !sourceOrigin || !destinationOrigin || !extent) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcPtr = converter::toNative<core::Texture>(source);
    auto* dstPtr = converter::toNative<core::Texture>(destination);

    WGPUOrigin3D wgpuSrcOrigin = converter::gfxOrigin3DToWGPUOrigin3D(sourceOrigin);
    WGPUOrigin3D wgpuDstOrigin = converter::gfxOrigin3DToWGPUOrigin3D(destinationOrigin);
    WGPUExtent3D wgpuExtent = converter::gfxExtent3DToWGPUExtent3D(extent);

    encoderPtr->copyTextureToTexture(srcPtr, wgpuSrcOrigin, sourceMipLevel, dstPtr, wgpuDstOrigin, destinationMipLevel, wgpuExtent);

    (void)srcFinalLayout; // WebGPU handles layout transitions automatically
    (void)dstFinalLayout;
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderBlitTextureToTexture(GfxCommandEncoder commandEncoder,
    GfxTexture source, const GfxOrigin3D* sourceOrigin, const GfxExtent3D* sourceExtent, uint32_t sourceMipLevel, GfxTextureLayout srcFinalLayout,
    GfxTexture destination, const GfxOrigin3D* destinationOrigin, const GfxExtent3D* destinationExtent, uint32_t destinationMipLevel, GfxTextureLayout dstFinalLayout,
    GfxFilterMode filter) const
{
    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcTexture = converter::toNative<core::Texture>(source);
    auto* dstTexture = converter::toNative<core::Texture>(destination);

    WGPUOrigin3D wgpuSrcOrigin = converter::gfxOrigin3DToWGPUOrigin3D(sourceOrigin);
    WGPUOrigin3D wgpuDstOrigin = converter::gfxOrigin3DToWGPUOrigin3D(destinationOrigin);
    WGPUExtent3D wgpuSrcExtent = converter::gfxExtent3DToWGPUExtent3D(sourceExtent);
    WGPUExtent3D wgpuDstExtent = converter::gfxExtent3DToWGPUExtent3D(destinationExtent);
    WGPUFilterMode wgpuFilter = converter::gfxFilterModeToWGPU(filter);

    encoder->blitTextureToTexture(srcTexture, wgpuSrcOrigin, wgpuSrcExtent, sourceMipLevel, dstTexture, wgpuDstOrigin, wgpuDstExtent, destinationMipLevel, wgpuFilter);

    // WebGPU handles layout transitions automatically
    (void)srcFinalLayout;
    (void)dstFinalLayout;
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder,
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

GfxResult Backend::commandEncoderGenerateMipmaps(GfxCommandEncoder commandEncoder, GfxTexture texture) const
{
    if (!commandEncoder || !texture) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* tex = converter::toNative<core::Texture>(texture);

    tex->generateMipmaps(encoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderGenerateMipmapsRange(GfxCommandEncoder commandEncoder, GfxTexture texture,
    uint32_t baseMipLevel, uint32_t levelCount) const
{
    if (!commandEncoder || !texture) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* tex = converter::toNative<core::Texture>(texture);

    tex->generateMipmapsRange(encoder, baseMipLevel, levelCount);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderEnd(GfxCommandEncoder commandEncoder) const
{
    (void)commandEncoder; // Parameter unused - handled in queueSubmit
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderBegin(GfxCommandEncoder commandEncoder) const
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<core::CommandEncoder>(commandEncoder);

    // WebGPU encoders can't be reused after Finish() - recreate if needed
    if (!encoderPtr->recreateIfNeeded()) {
        gfx::common::Logger::instance().logError("[WebGPU ERROR] Failed to recreate command encoder");
        return GFX_RESULT_ERROR_UNKNOWN;
    }
    return GFX_RESULT_SUCCESS;
}

// RenderPassEncoder functions
GfxResult Backend::renderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline) const
{
    if (!renderPassEncoder || !pipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* pipelinePtr = converter::toNative<core::RenderPipeline>(pipeline);

    encoderPtr->setPipeline(pipelinePtr->handle());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    if (!renderPassEncoder || !bindGroup) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* bindGroupPtr = converter::toNative<core::BindGroup>(bindGroup);

    encoderPtr->setBindGroup(index, bindGroupPtr->handle(), dynamicOffsets, dynamicOffsetCount);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size) const
{
    if (!renderPassEncoder || !buffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* bufferPtr = converter::toNative<core::Buffer>(buffer);

    encoderPtr->setVertexBuffer(slot, bufferPtr->handle(), offset, size);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size) const
{
    if (!renderPassEncoder || !buffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* bufferPtr = converter::toNative<core::Buffer>(buffer);

    encoderPtr->setIndexBuffer(bufferPtr->handle(), converter::gfxIndexFormatToWGPU(format), offset, size);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport) const
{
    if (!renderPassEncoder || !viewport) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    encoderPtr->setViewport(viewport->x, viewport->y, viewport->width, viewport->height,
        viewport->minDepth, viewport->maxDepth);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor) const
{
    if (!renderPassEncoder || !scissor) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    encoderPtr->setScissorRect(scissor->x, scissor->y, scissor->width, scissor->height);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
{
    if (!renderPassEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    encoderPtr->draw(vertexCount, instanceCount, firstVertex, firstInstance);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) const
{
    if (!renderPassEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    encoderPtr->drawIndexed(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder) const
{
    if (!renderPassEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    delete encoderPtr;
    return GFX_RESULT_SUCCESS;
}

// ComputePassEncoder functions
GfxResult Backend::computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline) const
{
    if (!computePassEncoder || !pipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    auto* pipelinePtr = converter::toNative<core::ComputePipeline>(pipeline);

    encoderPtr->setPipeline(pipelinePtr->handle());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    if (!computePassEncoder || !bindGroup) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    auto* bindGroupPtr = converter::toNative<core::BindGroup>(bindGroup);

    encoderPtr->setBindGroup(index, bindGroupPtr->handle(), dynamicOffsets, dynamicOffsetCount);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::computePassEncoderDispatchWorkgroups(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) const
{
    if (!computePassEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    encoderPtr->dispatchWorkgroups(workgroupCountX, workgroupCountY, workgroupCountZ);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::computePassEncoderEnd(GfxComputePassEncoder computePassEncoder) const
{
    if (!computePassEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoderPtr = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    delete encoderPtr;
    return GFX_RESULT_SUCCESS;
}

// Fence functions
GfxResult Backend::fenceDestroy(GfxFence fence) const
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    delete converter::toNative<core::Fence>(fence);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::fenceGetStatus(GfxFence fence, bool* isSignaled) const
{
    if (!fence || !isSignaled) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* fencePtr = converter::toNative<core::Fence>(fence);
    *isSignaled = fencePtr->isSignaled();
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::fenceWait(GfxFence fence, uint64_t timeoutNs) const
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* fencePtr = converter::toNative<core::Fence>(fence);

    // Fence is already properly signaled by queueSubmit after GPU work completes
    // Just check the status
    (void)timeoutNs; // Timeout is handled in queueSubmit

    return fencePtr->isSignaled() ? GFX_RESULT_SUCCESS : GFX_RESULT_TIMEOUT;
}

GfxResult Backend::fenceReset(GfxFence fence) const
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* fencePtr = converter::toNative<core::Fence>(fence);
    fencePtr->setSignaled(false);
    return GFX_RESULT_SUCCESS;
}

// Semaphore functions
GfxResult Backend::semaphoreDestroy(GfxSemaphore semaphore) const
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    delete converter::toNative<core::Semaphore>(semaphore);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::semaphoreGetType(GfxSemaphore semaphore, GfxSemaphoreType* outType) const
{
    if (!semaphore || !outType) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto type = converter::toNative<core::Semaphore>(semaphore)->getType();
    *outType = converter::webgpuSemaphoreTypeToGfxSemaphoreType(type);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::semaphoreSignal(GfxSemaphore semaphore, uint64_t value) const
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* semaphorePtr = converter::toNative<core::Semaphore>(semaphore);
    if (semaphorePtr->getType() == core::SemaphoreType::Timeline) {
        semaphorePtr->setValue(value);
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs) const
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    (void)timeoutNs; // WebGPU doesn't support timeout for semaphore waits

    auto* semaphorePtr = converter::toNative<core::Semaphore>(semaphore);
    if (semaphorePtr->getType() == core::SemaphoreType::Timeline) {
        return (semaphorePtr->getValue() >= value) ? GFX_RESULT_SUCCESS : GFX_RESULT_TIMEOUT;
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::semaphoreGetValue(GfxSemaphore semaphore, uint64_t* outValue) const
{
    if (!semaphore || !outValue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    *outValue = converter::toNative<core::Semaphore>(semaphore)->getValue();
    return GFX_RESULT_SUCCESS;
}

GfxAccessFlags Backend::getAccessFlagsForLayout(GfxTextureLayout layout) const
{
    // WebGPU doesn't use explicit access flags - synchronization is implicit
    (void)layout;
    return GFX_ACCESS_NONE;
}

} // namespace gfx::backend::webgpu