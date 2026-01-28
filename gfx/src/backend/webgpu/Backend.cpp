#include "Backend.h"

#include "common/Common.h"
#include "common/Logger.h"
#include "converter/Conversions.h"
#include "validator/Validations.h"

#include "core/command/CommandEncoder.h"
#include "core/command/ComputePassEncoder.h"
#include "core/command/RenderPassEncoder.h"
#include "core/compute/ComputePipeline.h"
#include "core/presentation/Surface.h"
#include "core/presentation/Swapchain.h"
#include "core/query/QuerySet.h"
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

#include <stdexcept>
#include <vector>

namespace gfx::backend::webgpu {

// ============================================================================
// Backend C++ Class Export
// ============================================================================

// Instance functions
GfxResult Backend::createInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance) const
{
    GfxResult validationResult = validator::validateCreateInstance(descriptor, outInstance);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateInstanceDestroy(instance);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateInstanceRequestAdapter(instance, descriptor, outAdapter);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* inst = converter::toNative<core::Instance>(instance);
        auto createInfo = converter::gfxDescriptorToWebGPUAdapterCreateInfo(descriptor);
        auto* adapter = inst->requestAdapter(createInfo);

        *outAdapter = converter::toGfx<GfxAdapter>(adapter);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to request adapter: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::instanceEnumerateAdapters(GfxInstance instance, uint32_t* adapterCount, GfxAdapter* adapters) const
{
    GfxResult validationResult = validator::validateInstanceEnumerateAdapters(instance, adapterCount);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* inst = converter::toNative<core::Instance>(instance);
    const auto& cachedAdapters = inst->getAdapters();

    if (!adapters) {
        // Return the count only
        *adapterCount = static_cast<uint32_t>(cachedAdapters.size());
        return GFX_RESULT_SUCCESS;
    }

    // Return the adapters
    uint32_t count = std::min(*adapterCount, static_cast<uint32_t>(cachedAdapters.size()));
    for (uint32_t i = 0; i < count; ++i) {
        adapters[i] = converter::toGfx<GfxAdapter>(cachedAdapters[i].get());
    }
    *adapterCount = count;
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::enumerateInstanceExtensions(uint32_t* extensionCount, const char** extensionNames) const
{
    if (!extensionCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    const auto internalExtensions = core::Instance::enumerateSupportedExtensions();

    if (!extensionNames) {
        *extensionCount = static_cast<uint32_t>(internalExtensions.size());
        return GFX_RESULT_SUCCESS;
    }

    // Map internal names to public API constants
    uint32_t copyCount = (*extensionCount < internalExtensions.size()) ? *extensionCount : static_cast<uint32_t>(internalExtensions.size());
    for (uint32_t i = 0; i < copyCount; ++i) {
        extensionNames[i] = converter::instanceExtensionNameToGfx(internalExtensions[i]);
    }
    *extensionCount = static_cast<uint32_t>(internalExtensions.size());

    return GFX_RESULT_SUCCESS;
}

// Adapter functions
GfxResult Backend::adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice) const
{
    GfxResult validationResult = validator::validateAdapterCreateDevice(adapter, descriptor, outDevice);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateAdapterGetInfo(adapter, outInfo);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* adapterPtr = converter::toNative<core::Adapter>(adapter);
    *outInfo = converter::wgpuAdapterToGfxAdapterInfo(adapterPtr->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::adapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits) const
{
    GfxResult validationResult = validator::validateAdapterGetLimits(adapter, outLimits);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* adapterPtr = converter::toNative<core::Adapter>(adapter);
    *outLimits = converter::wgpuLimitsToGfxDeviceLimits(adapterPtr->getLimits());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::adapterEnumerateQueueFamilies(GfxAdapter adapter, uint32_t* queueFamilyCount, GfxQueueFamilyProperties* queueFamilies) const
{
    GfxResult validationResult = validator::validateAdapterEnumerateQueueFamilies(adapter, queueFamilyCount);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* adap = converter::toNative<core::Adapter>(adapter);
    auto families = adap->getQueueFamilyProperties();
    uint32_t count = static_cast<uint32_t>(families.size());

    if (!queueFamilies) {
        *queueFamilyCount = count;
        return GFX_RESULT_SUCCESS;
    }

    uint32_t outputCount = std::min(*queueFamilyCount, count);
    for (uint32_t i = 0; i < outputCount; ++i) {
        queueFamilies[i] = converter::wgpuQueueFamilyPropertiesToGfx(families[i]);
    }

    *queueFamilyCount = count;
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::adapterGetQueueFamilySurfaceSupport(GfxAdapter adapter, uint32_t queueFamilyIndex, GfxSurface surface, bool* outSupported) const
{
    GfxResult validationResult = validator::validateAdapterGetQueueFamilySurfaceSupport(adapter, surface, outSupported);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* adap = converter::toNative<core::Adapter>(adapter);
    *outSupported = adap->supportsPresentation(queueFamilyIndex);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::adapterEnumerateExtensions(GfxAdapter adapter, uint32_t* extensionCount, const char** extensionNames) const
{
    GfxResult validationResult = validator::validateAdapterEnumerateExtensions(adapter, extensionCount);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* adap = converter::toNative<core::Adapter>(adapter);
    const auto internalExtensions = adap->enumerateSupportedExtensions();

    if (!extensionNames) {
        *extensionCount = static_cast<uint32_t>(internalExtensions.size());
        return GFX_RESULT_SUCCESS;
    }

    // Map internal names to public API constants
    uint32_t copyCount = (*extensionCount < internalExtensions.size()) ? *extensionCount : static_cast<uint32_t>(internalExtensions.size());
    for (uint32_t i = 0; i < copyCount; ++i) {
        extensionNames[i] = converter::deviceExtensionNameToGfx(internalExtensions[i]);
    }
    *extensionCount = static_cast<uint32_t>(internalExtensions.size());

    return GFX_RESULT_SUCCESS;
}

// Device functions
GfxResult Backend::deviceDestroy(GfxDevice device) const
{
    GfxResult validationResult = validator::validateDeviceDestroy(device);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Device>(device);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::deviceGetQueue(GfxDevice device, GfxQueue* outQueue) const
{
    GfxResult validationResult = validator::validateDeviceGetQueue(device, outQueue);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* dev = converter::toNative<core::Device>(device);
    *outQueue = converter::toGfx<GfxQueue>(dev->getQueue());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::deviceGetQueueByIndex(GfxDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, GfxQueue* outQueue) const
{
    GfxResult validationResult = validator::validateDeviceGetQueueByIndex(device, outQueue);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    // WebGPU only has one queue family (index 0) with one queue (index 0)
    if (queueFamilyIndex != 0 || queueIndex != 0) {
        return GFX_RESULT_ERROR_NOT_FOUND;
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
    GfxResult validationResult = validator::validateDeviceCreateSurface(device, descriptor, outSurface);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateDeviceCreateSwapchain(device, surface, descriptor, outSwapchain);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateDeviceCreateBuffer(device, descriptor, outBuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateDeviceImportBuffer(device, descriptor, outBuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateDeviceCreateTexture(device, descriptor, outTexture);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateDeviceImportTexture(device, descriptor, outTexture);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateDeviceCreateSampler(device, descriptor, outSampler);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateDeviceCreateShader(device, descriptor, outShader);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateDeviceCreateBindGroupLayout(device, descriptor, outLayout);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateDeviceCreateBindGroup(device, descriptor, outBindGroup);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateDeviceCreateRenderPipeline(device, descriptor, outPipeline);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateDeviceCreateComputePipeline(device, descriptor, outPipeline);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateDeviceCreateRenderPass(device, descriptor, outRenderPass);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateDeviceCreateFramebuffer(device, descriptor, outFramebuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateDeviceCreateCommandEncoder(device, descriptor, outEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateDeviceCreateFence(device, descriptor, outFence);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateDeviceCreateSemaphore(device, descriptor, outSemaphore);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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

GfxResult Backend::deviceCreateQuerySet(GfxDevice device, const GfxQuerySetDescriptor* descriptor, GfxQuerySet* outQuerySet) const
{
    GfxResult validationResult = validator::validateDeviceCreateQuerySet(device, descriptor, outQuerySet);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* dev = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToWebGPUQuerySetCreateInfo(descriptor);
        auto* querySet = new core::QuerySet(dev, createInfo);
        *outQuerySet = converter::toGfx<GfxQuerySet>(querySet);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create query set: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::deviceWaitIdle(GfxDevice device) const
{
    GfxResult validationResult = validator::validateDeviceWaitIdle(device);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* devicePtr = converter::toNative<core::Device>(device);
    devicePtr->waitIdle();
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits) const
{
    GfxResult validationResult = validator::validateDeviceGetLimits(device, outLimits);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* devicePtr = converter::toNative<core::Device>(device);
    *outLimits = converter::wgpuLimitsToGfxDeviceLimits(devicePtr->getLimits());
    return GFX_RESULT_SUCCESS;
}

// Surface functions
GfxResult Backend::surfaceDestroy(GfxSurface surface) const
{
    GfxResult validationResult = validator::validateSurfaceDestroy(surface);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Surface>(surface);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::surfaceEnumerateSupportedFormats(GfxSurface surface, uint32_t* formatCount, GfxTextureFormat* formats) const
{
    GfxResult validationResult = validator::validateSurfaceEnumerateSupportedFormats(surface, formatCount);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateSurfaceEnumerateSupportedPresentModes(surface, presentModeCount);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateSwapchainDestroy(swapchain);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Swapchain>(swapchain);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::swapchainGetInfo(GfxSwapchain swapchain, GfxSwapchainInfo* outInfo) const
{
    GfxResult validationResult = validator::validateSwapchainGetInfo(swapchain, outInfo);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* swapchainPtr = converter::toNative<core::Swapchain>(swapchain);
    *outInfo = converter::wgpuSwapchainInfoToGfxSwapchainInfo(swapchainPtr->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const
{
    GfxResult validationResult = validator::validateSwapchainAcquireNextImage(swapchain, outImageIndex);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateSwapchainGetTextureView(swapchain, outView);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    // WebGPU doesn't expose multiple swapchain images by index
    // Always return the current texture view regardless of index
    (void)imageIndex;

    return swapchainGetCurrentTextureView(swapchain, outView);
}

GfxResult Backend::swapchainGetCurrentTextureView(GfxSwapchain swapchain, GfxTextureView* outView) const
{
    GfxResult validationResult = validator::validateSwapchainGetCurrentTextureView(swapchain, outView);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* swapchainPtr = converter::toNative<core::Swapchain>(swapchain);
    *outView = converter::toGfx<GfxTextureView>(swapchainPtr->getCurrentTextureView());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::swapchainPresent(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo) const
{
    GfxResult validationResult = validator::validateSwapchainPresent(swapchain);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateBufferDestroy(buffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Buffer>(buffer);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::bufferGetInfo(GfxBuffer buffer, GfxBufferInfo* outInfo) const
{
    GfxResult validationResult = validator::validateBufferGetInfo(buffer, outInfo);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* buf = converter::toNative<core::Buffer>(buffer);
    *outInfo = converter::wgpuBufferToGfxBufferInfo(buf->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::bufferGetNativeHandle(GfxBuffer buffer, void** outHandle) const
{
    GfxResult validationResult = validator::validateBufferGetNativeHandle(buffer, outHandle);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* buf = converter::toNative<core::Buffer>(buffer);
    *outHandle = reinterpret_cast<void*>(buf->handle());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::bufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer) const
{
    GfxResult validationResult = validator::validateBufferMap(buffer, outMappedPointer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateBufferUnmap(buffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* bufferPtr = converter::toNative<core::Buffer>(buffer);
    bufferPtr->unmap();
    return GFX_RESULT_SUCCESS;
}

// Texture functions
GfxResult Backend::textureDestroy(GfxTexture texture) const
{
    GfxResult validationResult = validator::validateTextureDestroy(texture);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Texture>(texture);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::textureGetInfo(GfxTexture texture, GfxTextureInfo* outInfo) const
{
    GfxResult validationResult = validator::validateTextureGetInfo(texture, outInfo);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* texturePtr = converter::toNative<core::Texture>(texture);
    *outInfo = converter::wgpuTextureInfoToGfxTextureInfo(texturePtr->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::textureGetNativeHandle(GfxTexture texture, void** outHandle) const
{
    GfxResult validationResult = validator::validateTextureGetNativeHandle(texture, outHandle);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* texturePtr = converter::toNative<core::Texture>(texture);
    *outHandle = reinterpret_cast<void*>(texturePtr->handle());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::textureGetLayout(GfxTexture texture, GfxTextureLayout* outLayout) const
{
    // WebGPU doesn't have explicit layouts, return GENERAL as a reasonable default
    GfxResult validationResult = validator::validateTextureGetLayout(texture, outLayout);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    *outLayout = GFX_TEXTURE_LAYOUT_GENERAL;
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView) const
{
    GfxResult validationResult = validator::validateTextureCreateView(texture, descriptor, outView);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateTextureViewDestroy(textureView);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::TextureView>(textureView);
    return GFX_RESULT_SUCCESS;
}

// Sampler functions
GfxResult Backend::samplerDestroy(GfxSampler sampler) const
{
    GfxResult validationResult = validator::validateSamplerDestroy(sampler);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Sampler>(sampler);
    return GFX_RESULT_SUCCESS;
}

// Shader functions
GfxResult Backend::shaderDestroy(GfxShader shader) const
{
    GfxResult validationResult = validator::validateShaderDestroy(shader);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Shader>(shader);
    return GFX_RESULT_SUCCESS;
}

// BindGroupLayout functions
GfxResult Backend::bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout) const
{
    GfxResult validationResult = validator::validateBindGroupLayoutDestroy(bindGroupLayout);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::BindGroupLayout>(bindGroupLayout);
    return GFX_RESULT_SUCCESS;
}

// BindGroup functions
GfxResult Backend::bindGroupDestroy(GfxBindGroup bindGroup) const
{
    GfxResult validationResult = validator::validateBindGroupDestroy(bindGroup);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::BindGroup>(bindGroup);
    return GFX_RESULT_SUCCESS;
}

// RenderPipeline functions
GfxResult Backend::renderPipelineDestroy(GfxRenderPipeline renderPipeline) const
{
    GfxResult validationResult = validator::validateRenderPipelineDestroy(renderPipeline);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::RenderPipeline>(renderPipeline);
    return GFX_RESULT_SUCCESS;
}

// ComputePipeline functions
GfxResult Backend::computePipelineDestroy(GfxComputePipeline computePipeline) const
{
    GfxResult validationResult = validator::validateComputePipelineDestroy(computePipeline);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::ComputePipeline>(computePipeline);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassDestroy(GfxRenderPass renderPass) const
{
    GfxResult validationResult = validator::validateRenderPassDestroy(renderPass);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::RenderPass>(renderPass);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::framebufferDestroy(GfxFramebuffer framebuffer) const
{
    GfxResult validationResult = validator::validateFramebufferDestroy(framebuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Framebuffer>(framebuffer);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::querySetDestroy(GfxQuerySet querySet) const
{
    GfxResult validationResult = validator::validateQuerySetDestroy(querySet);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::QuerySet>(querySet);
    return GFX_RESULT_SUCCESS;
}

// Queue functions
GfxResult Backend::queueSubmit(GfxQueue queue, const GfxSubmitDescriptor* submitInfo) const
{
    GfxResult validationResult = validator::validateQueueSubmit(queue, submitInfo);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* queuePtr = converter::toNative<core::Queue>(queue);
    auto submit = converter::gfxDescriptorToWebGPUSubmitInfo(submitInfo);

    return queuePtr->submit(submit) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

GfxResult Backend::queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size) const
{
    GfxResult validationResult = validator::validateQueueWriteBuffer(queue, buffer, data);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* queuePtr = converter::toNative<core::Queue>(queue);
    auto* bufferPtr = converter::toNative<core::Buffer>(buffer);

    queuePtr->writeBuffer(bufferPtr, offset, data, size);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel, const void* data, uint64_t dataSize, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
{
    GfxResult validationResult = validator::validateQueueWriteTexture(queue, texture, origin, extent, data);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* queuePtr = converter::toNative<core::Queue>(queue);
    auto* texturePtr = converter::toNative<core::Texture>(texture);

    WGPUOrigin3D wgpuOrigin = converter::gfxOrigin3DToWGPUOrigin3D(origin);
    WGPUExtent3D wgpuExtent = converter::gfxExtent3DToWGPUExtent3D(extent);

    queuePtr->writeTexture(texturePtr, mipLevel, wgpuOrigin, data, dataSize, wgpuExtent);

    (void)finalLayout; // WebGPU handles layout transitions automatically
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::queueWaitIdle(GfxQueue queue) const
{
    GfxResult validationResult = validator::validateQueueWaitIdle(queue);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* queuePtr = converter::toNative<core::Queue>(queue);
    return queuePtr->waitIdle() ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

// CommandEncoder functions
GfxResult Backend::commandEncoderDestroy(GfxCommandEncoder commandEncoder) const
{
    GfxResult validationResult = validator::validateCommandEncoderDestroy(commandEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::CommandEncoder>(commandEncoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder, const GfxRenderPassBeginDescriptor* beginDescriptor, GfxRenderPassEncoder* outRenderPass) const
{
    GfxResult validationResult = validator::validateCommandEncoderBeginRenderPass(commandEncoder, beginDescriptor, outRenderPass);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateCommandEncoderBeginComputePass(commandEncoder, beginDescriptor, outComputePass);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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

GfxResult Backend::commandEncoderCopyBufferToBuffer(GfxCommandEncoder commandEncoder, const GfxCopyBufferToBufferDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderCopyBufferToBuffer(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcPtr = converter::toNative<core::Buffer>(descriptor->source);
    auto* dstPtr = converter::toNative<core::Buffer>(descriptor->destination);

    encoderPtr->copyBufferToBuffer(srcPtr, descriptor->sourceOffset, dstPtr, descriptor->destinationOffset, descriptor->size);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder, const GfxCopyBufferToTextureDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderCopyBufferToTexture(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcPtr = converter::toNative<core::Buffer>(descriptor->source);
    auto* dstPtr = converter::toNative<core::Texture>(descriptor->destination);

    WGPUOrigin3D wgpuOrigin = converter::gfxOrigin3DToWGPUOrigin3D(&descriptor->origin);
    WGPUExtent3D wgpuExtent = converter::gfxExtent3DToWGPUExtent3D(&descriptor->extent);

    encoderPtr->copyBufferToTexture(srcPtr, descriptor->sourceOffset, dstPtr, wgpuOrigin, wgpuExtent, descriptor->mipLevel);

    (void)descriptor->finalLayout; // WebGPU handles layout transitions automatically
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder, const GfxCopyTextureToBufferDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderCopyTextureToBuffer(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcPtr = converter::toNative<core::Texture>(descriptor->source);
    auto* dstPtr = converter::toNative<core::Buffer>(descriptor->destination);

    WGPUOrigin3D wgpuOrigin = converter::gfxOrigin3DToWGPUOrigin3D(&descriptor->origin);
    WGPUExtent3D wgpuExtent = converter::gfxExtent3DToWGPUExtent3D(&descriptor->extent);

    encoderPtr->copyTextureToBuffer(srcPtr, wgpuOrigin, descriptor->mipLevel, dstPtr, descriptor->destinationOffset, wgpuExtent);

    (void)descriptor->finalLayout; // WebGPU handles layout transitions automatically
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder, const GfxCopyTextureToTextureDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderCopyTextureToTexture(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcPtr = converter::toNative<core::Texture>(descriptor->source);
    auto* dstPtr = converter::toNative<core::Texture>(descriptor->destination);

    WGPUOrigin3D wgpuSrcOrigin = converter::gfxOrigin3DToWGPUOrigin3D(&descriptor->sourceOrigin);
    WGPUOrigin3D wgpuDstOrigin = converter::gfxOrigin3DToWGPUOrigin3D(&descriptor->destinationOrigin);
    WGPUExtent3D wgpuExtent = converter::gfxExtent3DToWGPUExtent3D(&descriptor->extent);

    encoderPtr->copyTextureToTexture(srcPtr, wgpuSrcOrigin, descriptor->sourceMipLevel, dstPtr, wgpuDstOrigin, descriptor->destinationMipLevel, wgpuExtent);

    (void)descriptor->sourceFinalLayout; // WebGPU handles layout transitions automatically
    (void)descriptor->destinationFinalLayout;
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderBlitTextureToTexture(GfxCommandEncoder commandEncoder, const GfxBlitTextureToTextureDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderBlitTextureToTexture(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcTexture = converter::toNative<core::Texture>(descriptor->source);
    auto* dstTexture = converter::toNative<core::Texture>(descriptor->destination);

    WGPUOrigin3D wgpuSrcOrigin = converter::gfxOrigin3DToWGPUOrigin3D(&descriptor->sourceOrigin);
    WGPUOrigin3D wgpuDstOrigin = converter::gfxOrigin3DToWGPUOrigin3D(&descriptor->destinationOrigin);
    WGPUExtent3D wgpuSrcExtent = converter::gfxExtent3DToWGPUExtent3D(&descriptor->sourceExtent);
    WGPUExtent3D wgpuDstExtent = converter::gfxExtent3DToWGPUExtent3D(&descriptor->destinationExtent);
    WGPUFilterMode wgpuFilter = converter::gfxFilterModeToWGPU(descriptor->filter);

    encoder->blitTextureToTexture(srcTexture, wgpuSrcOrigin, wgpuSrcExtent, descriptor->sourceMipLevel, dstTexture, wgpuDstOrigin, wgpuDstExtent, descriptor->destinationMipLevel, wgpuFilter);

    // WebGPU handles layout transitions automatically
    (void)descriptor->sourceFinalLayout;
    (void)descriptor->destinationFinalLayout;
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder, const GfxPipelineBarrierDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderPipelineBarrier(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    // WebGPU handles synchronization and layout transitions automatically
    // This is a no-op for WebGPU backend
    (void)commandEncoder;
    (void)descriptor;
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderGenerateMipmaps(GfxCommandEncoder commandEncoder, GfxTexture texture) const
{
    GfxResult validationResult = validator::validateCommandEncoderGenerateMipmaps(commandEncoder, texture);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* tex = converter::toNative<core::Texture>(texture);

    tex->generateMipmaps(encoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderGenerateMipmapsRange(GfxCommandEncoder commandEncoder, GfxTexture texture, uint32_t baseMipLevel, uint32_t levelCount) const
{
    GfxResult validationResult = validator::validateCommandEncoderGenerateMipmapsRange(commandEncoder, texture);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* tex = converter::toNative<core::Texture>(texture);

    tex->generateMipmapsRange(encoder, baseMipLevel, levelCount);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderWriteTimestamp(GfxCommandEncoder commandEncoder, GfxQuerySet querySet, uint32_t queryIndex) const
{
    GfxResult validationResult = validator::validateCommandEncoderWriteTimestamp(commandEncoder, querySet);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* query = converter::toNative<core::QuerySet>(querySet);
    encoder->writeTimestamp(query->handle(), queryIndex);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderResolveQuerySet(GfxCommandEncoder commandEncoder, GfxQuerySet querySet, uint32_t firstQuery, uint32_t queryCount, GfxBuffer destinationBuffer, uint64_t destinationOffset) const
{
    GfxResult validationResult = validator::validateCommandEncoderResolveQuerySet(commandEncoder, querySet, destinationBuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* query = converter::toNative<core::QuerySet>(querySet);
    auto* buffer = converter::toNative<core::Buffer>(destinationBuffer);
    encoder->resolveQuerySet(query->handle(), firstQuery, queryCount, buffer->handle(), destinationOffset);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderEnd(GfxCommandEncoder commandEncoder) const
{
    GfxResult validationResult = validator::validateCommandEncoderEnd(commandEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    (void)commandEncoder; // Parameter unused - handled in queueSubmit
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderBegin(GfxCommandEncoder commandEncoder) const
{
    GfxResult validationResult = validator::validateCommandEncoderBegin(commandEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateRenderPassEncoderSetPipeline(renderPassEncoder, pipeline);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* pipelinePtr = converter::toNative<core::RenderPipeline>(pipeline);

    encoderPtr->setPipeline(pipelinePtr->handle());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetBindGroup(renderPassEncoder, bindGroup);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* bindGroupPtr = converter::toNative<core::BindGroup>(bindGroup);

    encoderPtr->setBindGroup(index, bindGroupPtr->handle(), dynamicOffsets, dynamicOffsetCount);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetVertexBuffer(renderPassEncoder, buffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* bufferPtr = converter::toNative<core::Buffer>(buffer);

    encoderPtr->setVertexBuffer(slot, bufferPtr->handle(), offset, size);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetIndexBuffer(renderPassEncoder, buffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* bufferPtr = converter::toNative<core::Buffer>(buffer);

    encoderPtr->setIndexBuffer(bufferPtr->handle(), converter::gfxIndexFormatToWGPU(format), offset, size);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetViewport(renderPassEncoder, viewport);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    encoderPtr->setViewport(viewport->x, viewport->y, viewport->width, viewport->height,
        viewport->minDepth, viewport->maxDepth);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetScissorRect(renderPassEncoder, scissor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    encoderPtr->setScissorRect(scissor->x, scissor->y, scissor->width, scissor->height);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderDraw(renderPassEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    encoderPtr->draw(vertexCount, instanceCount, firstVertex, firstInstance);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderDrawIndexed(renderPassEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    encoderPtr->drawIndexed(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderDrawIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderDrawIndirect(renderPassEncoder, indirectBuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* bufferPtr = converter::toNative<core::Buffer>(indirectBuffer);
    encoderPtr->drawIndirect(bufferPtr->handle(), indirectOffset);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderDrawIndexedIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderDrawIndexedIndirect(renderPassEncoder, indirectBuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* bufferPtr = converter::toNative<core::Buffer>(indirectBuffer);
    encoderPtr->drawIndexedIndirect(bufferPtr->handle(), indirectOffset);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderBeginOcclusionQuery(GfxRenderPassEncoder renderPassEncoder, GfxQuerySet querySet, uint32_t queryIndex) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderBeginOcclusionQuery(renderPassEncoder, querySet);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoder = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* query = converter::toNative<core::QuerySet>(querySet);
    encoder->beginOcclusionQuery(query->handle(), queryIndex);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderEndOcclusionQuery(GfxRenderPassEncoder renderPassEncoder) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderEndOcclusionQuery(renderPassEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoder = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    encoder->endOcclusionQuery();
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderEnd(renderPassEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    delete encoderPtr;
    return GFX_RESULT_SUCCESS;
}

// ComputePassEncoder functions
GfxResult Backend::computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline) const
{
    GfxResult validationResult = validator::validateComputePassEncoderSetPipeline(computePassEncoder, pipeline);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    auto* pipelinePtr = converter::toNative<core::ComputePipeline>(pipeline);

    encoderPtr->setPipeline(pipelinePtr->handle());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    GfxResult validationResult = validator::validateComputePassEncoderSetBindGroup(computePassEncoder, bindGroup);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    auto* bindGroupPtr = converter::toNative<core::BindGroup>(bindGroup);

    encoderPtr->setBindGroup(index, bindGroupPtr->handle(), dynamicOffsets, dynamicOffsetCount);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::computePassEncoderDispatch(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) const
{
    GfxResult validationResult = validator::validateComputePassEncoderDispatch(computePassEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    encoderPtr->dispatchWorkgroups(workgroupCountX, workgroupCountY, workgroupCountZ);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::computePassEncoderDispatchIndirect(GfxComputePassEncoder computePassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const
{
    GfxResult validationResult = validator::validateComputePassEncoderDispatchIndirect(computePassEncoder, indirectBuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    auto* bufferPtr = converter::toNative<core::Buffer>(indirectBuffer);
    encoderPtr->dispatchIndirect(bufferPtr->handle(), indirectOffset);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::computePassEncoderEnd(GfxComputePassEncoder computePassEncoder) const
{
    GfxResult validationResult = validator::validateComputePassEncoderEnd(computePassEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoderPtr = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    delete encoderPtr;
    return GFX_RESULT_SUCCESS;
}

// Fence functions
GfxResult Backend::fenceDestroy(GfxFence fence) const
{
    GfxResult validationResult = validator::validateFenceDestroy(fence);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Fence>(fence);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::fenceGetStatus(GfxFence fence, bool* isSignaled) const
{
    GfxResult validationResult = validator::validateFenceGetStatus(fence, isSignaled);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* fencePtr = converter::toNative<core::Fence>(fence);
    *isSignaled = fencePtr->isSignaled();
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::fenceWait(GfxFence fence, uint64_t timeoutNs) const
{
    GfxResult validationResult = validator::validateFenceWait(fence);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* fencePtr = converter::toNative<core::Fence>(fence);

    // Fence is already properly signaled by queueSubmit after GPU work completes
    // Just check the status
    (void)timeoutNs; // Timeout is handled in queueSubmit

    return fencePtr->isSignaled() ? GFX_RESULT_SUCCESS : GFX_RESULT_TIMEOUT;
}

GfxResult Backend::fenceReset(GfxFence fence) const
{
    GfxResult validationResult = validator::validateFenceReset(fence);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* fencePtr = converter::toNative<core::Fence>(fence);
    fencePtr->setSignaled(false);
    return GFX_RESULT_SUCCESS;
}

// Semaphore functions
GfxResult Backend::semaphoreDestroy(GfxSemaphore semaphore) const
{
    GfxResult validationResult = validator::validateSemaphoreDestroy(semaphore);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Semaphore>(semaphore);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::semaphoreGetType(GfxSemaphore semaphore, GfxSemaphoreType* outType) const
{
    GfxResult validationResult = validator::validateSemaphoreGetType(semaphore, outType);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto type = converter::toNative<core::Semaphore>(semaphore)->getType();
    *outType = converter::webgpuSemaphoreTypeToGfxSemaphoreType(type);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::semaphoreSignal(GfxSemaphore semaphore, uint64_t value) const
{
    GfxResult validationResult = validator::validateSemaphoreSignal(semaphore);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* semaphorePtr = converter::toNative<core::Semaphore>(semaphore);
    if (semaphorePtr->getType() == core::SemaphoreType::Timeline) {
        semaphorePtr->setValue(value);
    }
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs) const
{
    GfxResult validationResult = validator::validateSemaphoreWait(semaphore);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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
    GfxResult validationResult = validator::validateSemaphoreGetValue(semaphore, outValue);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
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