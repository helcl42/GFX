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

namespace gfx::backend::vulkan {

// Instance functions
GfxResult Backend::createInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance) const
{
    GfxResult validationResult = validator::validateCreateInstance(descriptor, outInstance);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto createInfo = converter::gfxDescriptorToInstanceCreateInfo(descriptor);
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

    delete converter::toNative<core::Instance>(instance);
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
        auto createInfo = converter::gfxDescriptorToAdapterCreateInfo(descriptor);
        auto* adapter = inst->requestAdapter(createInfo);
        *outAdapter = converter::toGfx<GfxAdapter>(adapter);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to request adapter: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
    return GFX_RESULT_SUCCESS;
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
        *adapterCount = static_cast<uint32_t>(cachedAdapters.size());
        return GFX_RESULT_SUCCESS;
    }

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
        auto createInfo = converter::gfxDescriptorToDeviceCreateInfo(descriptor);
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

    auto* adap = converter::toNative<core::Adapter>(adapter);
    *outInfo = converter::vkPropertiesToGfxAdapterInfo(adap->getProperties());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::adapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits) const
{
    GfxResult validationResult = validator::validateAdapterGetLimits(adapter, outLimits);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* adap = converter::toNative<core::Adapter>(adapter);
    *outLimits = converter::vkPropertiesToGfxDeviceLimits(adap->getProperties());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::adapterEnumerateQueueFamilies(GfxAdapter adapter, uint32_t* queueFamilyCount, GfxQueueFamilyProperties* queueFamilies) const
{
    GfxResult validationResult = validator::validateAdapterEnumerateQueueFamilies(adapter, queueFamilyCount);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* adap = converter::toNative<core::Adapter>(adapter);
    auto vkProps = adap->getQueueFamilyProperties();
    uint32_t count = static_cast<uint32_t>(vkProps.size());

    if (!queueFamilies) {
        // Just return the count
        *queueFamilyCount = count;
        return GFX_RESULT_SUCCESS;
    }

    // Copy properties to output array
    uint32_t outputCount = std::min(*queueFamilyCount, count);
    for (uint32_t i = 0; i < outputCount; ++i) {
        queueFamilies[i] = converter::vkQueueFamilyPropertiesToGfx(vkProps[i]);
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
    auto* surf = converter::toNative<core::Surface>(surface);

    *outSupported = adap->supportsPresentation(queueFamilyIndex, surf->handle());
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

    auto* dev = converter::toNative<core::Device>(device);
    auto* queue = dev->getQueueByIndex(queueFamilyIndex, queueIndex);

    if (!queue) {
        return GFX_RESULT_ERROR_NOT_FOUND;
    }

    *outQueue = converter::toGfx<GfxQueue>(queue);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::deviceCreateSurface(GfxDevice device, const GfxSurfaceDescriptor* descriptor, GfxSurface* outSurface) const
{
    GfxResult validationResult = validator::validateDeviceCreateSurface(device, descriptor, outSurface);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

#ifdef GFX_HEADLESS_BUILD
    (void)device;
    (void)descriptor;
    (void)outSurface;
    gfx::common::Logger::instance().logError("Surface creation is not available in headless builds");
    return GFX_RESULT_ERROR_FEATURE_NOT_SUPPORTED;
#else
    try {
        auto* dev = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToSurfaceCreateInfo(descriptor);
        auto* surface = new core::Surface(dev->getAdapter(), createInfo);
        *outSurface = converter::toGfx<GfxSurface>(surface);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create surface: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }
#endif
}

GfxResult Backend::deviceCreateSwapchain(GfxDevice device, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain) const
{
    GfxResult validationResult = validator::validateDeviceCreateSwapchain(device, descriptor, outSwapchain);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* dev = converter::toNative<core::Device>(device);
        auto* surf = converter::toNative<core::Surface>(descriptor->surface);
        auto createInfo = converter::gfxDescriptorToSwapchainCreateInfo(descriptor);
        auto* swapchain = new core::Swapchain(dev, surf, createInfo);
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
        auto* dev = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToBufferCreateInfo(descriptor);
        auto* buffer = new core::Buffer(dev, createInfo);
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
        auto* dev = converter::toNative<core::Device>(device);
        VkBuffer vkBuffer = reinterpret_cast<VkBuffer>(descriptor->nativeHandle);
        auto importInfo = converter::gfxExternalDescriptorToBufferImportInfo(descriptor);
        auto* buffer = new core::Buffer(dev, vkBuffer, importInfo);
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
        auto* dev = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToTextureCreateInfo(descriptor);
        auto* texture = new core::Texture(dev, createInfo);
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
        auto* dev = converter::toNative<core::Device>(device);
        VkImage vkImage = reinterpret_cast<VkImage>(descriptor->nativeHandle);
        auto importInfo = converter::gfxExternalDescriptorToTextureImportInfo(descriptor);
        auto* texture = new core::Texture(dev, vkImage, importInfo);
        texture->setLayout(converter::gfxLayoutToVkImageLayout(descriptor->currentLayout));
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
        auto* dev = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToSamplerCreateInfo(descriptor);
        auto* sampler = new core::Sampler(dev, createInfo);
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
        auto* dev = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToShaderCreateInfo(descriptor);
        auto* shader = new core::Shader(dev, createInfo);
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
        auto* dev = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToBindGroupLayoutCreateInfo(descriptor);
        auto* layout = new core::BindGroupLayout(dev, createInfo);
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
        auto* dev = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToBindGroupCreateInfo(descriptor);
        auto* bindGroup = new core::BindGroup(dev, createInfo);
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
        auto* dev = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToRenderPipelineCreateInfo(descriptor);
        auto* pipeline = new core::RenderPipeline(dev, createInfo);
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
        auto* dev = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToComputePipelineCreateInfo(descriptor);
        auto* pipeline = new core::ComputePipeline(dev, createInfo);
        *outPipeline = converter::toGfx<GfxComputePipeline>(pipeline);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create compute pipeline: {}", e.what());
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
        auto* dev = converter::toNative<core::Device>(device);
        auto* encoder = new core::CommandEncoder(dev);
        *outEncoder = converter::toGfx<GfxCommandEncoder>(encoder);
        return GFX_RESULT_SUCCESS;
    } catch (const std::exception& e) {
        gfx::common::Logger::instance().logError("Failed to create command encoder: {}", e.what());
        return GFX_RESULT_ERROR_UNKNOWN;
    }

    (void)descriptor->label; // Unused for now
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
    return GFX_RESULT_SUCCESS;
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

GfxResult Backend::deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence) const
{
    GfxResult validationResult = validator::validateDeviceCreateFence(device, descriptor, outFence);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* dev = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToFenceCreateInfo(descriptor);
        auto* fence = new core::Fence(dev, createInfo);
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
        auto* dev = converter::toNative<core::Device>(device);
        auto createInfo = converter::gfxDescriptorToSemaphoreCreateInfo(descriptor);
        auto* semaphore = new core::Semaphore(dev, createInfo);
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
        auto createInfo = converter::gfxDescriptorToQuerySetCreateInfo(descriptor);
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

    auto* dev = converter::toNative<core::Device>(device);
    dev->waitIdle();
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits) const
{
    GfxResult validationResult = validator::validateDeviceGetLimits(device, outLimits);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* dev = converter::toNative<core::Device>(device);
    *outLimits = converter::vkPropertiesToGfxDeviceLimits(dev->getProperties());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::deviceSupportsShaderFormat(GfxDevice device, GfxShaderSourceType format, bool* outSupported) const
{
    if (!device || !outSupported) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* devicePtr = converter::toNative<core::Device>(device);
    auto internalFormat = converter::gfxShaderSourceTypeToVulkanShaderSourceType(format);
    *outSupported = devicePtr->supportsShaderFormat(internalFormat);
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
    auto surfaceFormats = surf->getSupportedFormats();
    uint32_t count = static_cast<uint32_t>(surfaceFormats.size());

    // Convert to GfxTextureFormat
    if (formats) {
        uint32_t copyCount = std::min(count, *formatCount);
        for (uint32_t i = 0; i < copyCount; ++i) {
            formats[i] = converter::vkFormatToGfxFormat(surfaceFormats[i].format);
        }
    }

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
    auto vkPresentModes = surf->getSupportedPresentModes();
    uint32_t count = static_cast<uint32_t>(vkPresentModes.size());

    // Convert to GfxPresentMode
    if (presentModes) {
        uint32_t copyCount = std::min(count, *presentModeCount);
        for (uint32_t i = 0; i < copyCount; ++i) {
            presentModes[i] = converter::vkPresentModeToGfxPresentMode(vkPresentModes[i]);
        }
    }

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

    auto* sc = converter::toNative<core::Swapchain>(swapchain);
    *outInfo = converter::vkSwapchainInfoToGfxSwapchainInfo(sc->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const
{
    GfxResult validationResult = validator::validateSwapchainAcquireNextImage(swapchain, outImageIndex);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* sc = converter::toNative<core::Swapchain>(swapchain);

    VkSemaphore vkSemaphore = VK_NULL_HANDLE;
    if (imageAvailableSemaphore) {
        auto* sem = converter::toNative<core::Semaphore>(imageAvailableSemaphore);
        vkSemaphore = sem->handle();
    }

    VkFence vkFence = VK_NULL_HANDLE;
    if (fence) {
        auto* f = converter::toNative<core::Fence>(fence);
        vkFence = f->handle();
    }

    VkResult result = sc->acquireNextImage(timeoutNs, vkSemaphore, vkFence, outImageIndex);

    switch (result) {
    case VK_SUCCESS:
    case VK_SUBOPTIMAL_KHR:
        return GFX_RESULT_SUCCESS;
    case VK_TIMEOUT:
        return GFX_RESULT_TIMEOUT;
    case VK_NOT_READY:
        return GFX_RESULT_NOT_READY;
    case VK_ERROR_OUT_OF_DATE_KHR:
        return GFX_RESULT_ERROR_OUT_OF_DATE;
    case VK_ERROR_SURFACE_LOST_KHR:
        return GFX_RESULT_ERROR_SURFACE_LOST;
    case VK_ERROR_DEVICE_LOST:
        return GFX_RESULT_ERROR_DEVICE_LOST;
    default:
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::swapchainGetTextureView(GfxSwapchain swapchain, uint32_t imageIndex, GfxTextureView* outView) const
{
    GfxResult validationResult = validator::validateSwapchainGetTextureView(swapchain, outView);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* sc = converter::toNative<core::Swapchain>(swapchain);
    if (imageIndex >= sc->getImageCount()) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    *outView = converter::toGfx<GfxTextureView>(sc->getTextureView(imageIndex));
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::swapchainGetCurrentTextureView(GfxSwapchain swapchain, GfxTextureView* outView) const
{
    GfxResult validationResult = validator::validateSwapchainGetCurrentTextureView(swapchain, outView);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* sc = converter::toNative<core::Swapchain>(swapchain);
    *outView = converter::toGfx<GfxTextureView>(sc->getCurrentTextureView());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::swapchainPresent(GfxSwapchain swapchain, const GfxPresentDescriptor* presentDescriptor) const
{
    GfxResult validationResult = validator::validateSwapchainPresent(swapchain, presentDescriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* sc = converter::toNative<core::Swapchain>(swapchain);

    std::vector<VkSemaphore> waitSemaphores;
    if (presentDescriptor && presentDescriptor->waitSemaphoreCount > 0) {
        waitSemaphores.reserve(presentDescriptor->waitSemaphoreCount);
        for (uint32_t i = 0; i < presentDescriptor->waitSemaphoreCount; ++i) {
            auto* sem = converter::toNative<core::Semaphore>(presentDescriptor->waitSemaphores[i]);
            if (sem) {
                waitSemaphores.push_back(sem->handle());
            }
        }
    }

    VkResult result = sc->present(waitSemaphores);

    switch (result) {
    case VK_SUCCESS:
        return GFX_RESULT_SUCCESS;
    case VK_SUBOPTIMAL_KHR:
        return GFX_RESULT_SUCCESS; // Still success, just suboptimal
    case VK_ERROR_OUT_OF_DATE_KHR:
        return GFX_RESULT_ERROR_OUT_OF_DATE;
    case VK_ERROR_SURFACE_LOST_KHR:
        return GFX_RESULT_ERROR_SURFACE_LOST;
    case VK_ERROR_DEVICE_LOST:
        return GFX_RESULT_ERROR_DEVICE_LOST;
    default:
        return GFX_RESULT_ERROR_UNKNOWN;
    }
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
    *outInfo = converter::vkBufferToGfxBufferInfo(buf->getInfo());
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

    auto* buf = converter::toNative<core::Buffer>(buffer);
    void* mapped = buf->map(offset, size);
    if (!mapped) {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
    *outMappedPointer = mapped;
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::bufferUnmap(GfxBuffer buffer) const
{
    GfxResult validationResult = validator::validateBufferUnmap(buffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* buf = converter::toNative<core::Buffer>(buffer);
    buf->unmap();
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::bufferFlushMappedRange(GfxBuffer buffer, uint64_t offset, uint64_t size) const
{
    GfxResult validationResult = validator::validateBufferFlushMappedRange(buffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* buf = converter::toNative<core::Buffer>(buffer);
    buf->flushMappedRange(offset, size);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::bufferInvalidateMappedRange(GfxBuffer buffer, uint64_t offset, uint64_t size) const
{
    GfxResult validationResult = validator::validateBufferInvalidateMappedRange(buffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* buf = converter::toNative<core::Buffer>(buffer);
    buf->invalidateMappedRange(offset, size);
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

    auto* tex = converter::toNative<core::Texture>(texture);
    *outInfo = converter::vkTextureInfoToGfxTextureInfo(tex->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::textureGetNativeHandle(GfxTexture texture, void** outHandle) const
{
    GfxResult validationResult = validator::validateTextureGetNativeHandle(texture, outHandle);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* tex = converter::toNative<core::Texture>(texture);
    *outHandle = reinterpret_cast<void*>(tex->handle());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::textureGetLayout(GfxTexture texture, GfxTextureLayout* outLayout) const
{
    GfxResult validationResult = validator::validateTextureGetLayout(texture, outLayout);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* tex = converter::toNative<core::Texture>(texture);
    *outLayout = converter::vkImageLayoutToGfxLayout(tex->getLayout());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView) const
{
    GfxResult validationResult = validator::validateTextureCreateView(texture, descriptor, outView);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* tex = converter::toNative<core::Texture>(texture);
        auto createInfo = converter::gfxDescriptorToTextureViewCreateInfo(descriptor);
        auto* view = new core::TextureView(tex, createInfo);
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
GfxResult Backend::queueSubmit(GfxQueue queue, const GfxSubmitDescriptor* submitDescriptor) const
{
    GfxResult validationResult = validator::validateQueueSubmit(queue, submitDescriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* q = converter::toNative<core::Queue>(queue);
    auto internalSubmitInfo = converter::gfxDescriptorToSubmitInfo(submitDescriptor);
    VkResult result = q->submit(internalSubmitInfo);
    return (result == VK_SUCCESS) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

GfxResult Backend::queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size) const
{
    GfxResult validationResult = validator::validateQueueWriteBuffer(queue, buffer, data);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* q = converter::toNative<core::Queue>(queue);
    auto* buf = converter::toNative<core::Buffer>(buffer);
    q->writeBuffer(buf, offset, data, size);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel, const void* data, uint64_t dataSize, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
{
    GfxResult validationResult = validator::validateQueueWriteTexture(queue, texture, origin, extent, data);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* q = converter::toNative<core::Queue>(queue);
    auto* tex = converter::toNative<core::Texture>(texture);

    VkOffset3D vkOrigin = origin ? converter::gfxOrigin3DToVkOffset3D(origin) : VkOffset3D{ 0, 0, 0 };
    VkExtent3D vkExtent = converter::gfxExtent3DToVkExtent3D(extent);
    VkImageLayout vkLayout = converter::gfxLayoutToVkImageLayout(finalLayout);

    q->writeTexture(tex, vkOrigin, mipLevel, data, dataSize, vkExtent, vkLayout);

    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::queueWaitIdle(GfxQueue queue) const
{
    GfxResult validationResult = validator::validateQueueWaitIdle(queue);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* q = converter::toNative<core::Queue>(queue);
    q->waitIdle();
    return GFX_RESULT_SUCCESS;
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

    auto* enc = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcBuf = converter::toNative<core::Buffer>(descriptor->source);
    auto* dstBuf = converter::toNative<core::Buffer>(descriptor->destination);

    enc->copyBufferToBuffer(srcBuf, descriptor->sourceOffset, dstBuf, descriptor->destinationOffset, descriptor->size);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderCopyBufferToTexture(GfxCommandEncoder commandEncoder, const GfxCopyBufferToTextureDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderCopyBufferToTexture(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* enc = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcBuf = converter::toNative<core::Buffer>(descriptor->source);
    auto* dstTex = converter::toNative<core::Texture>(descriptor->destination);

    VkOffset3D vkOrigin = converter::gfxOrigin3DToVkOffset3D(&descriptor->origin);
    VkExtent3D vkExtent = converter::gfxExtent3DToVkExtent3D(&descriptor->extent);
    VkImageLayout vkLayout = converter::gfxLayoutToVkImageLayout(descriptor->finalLayout);

    enc->copyBufferToTexture(srcBuf, descriptor->sourceOffset, dstTex, vkOrigin, vkExtent, descriptor->mipLevel, vkLayout);

    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderCopyTextureToBuffer(GfxCommandEncoder commandEncoder, const GfxCopyTextureToBufferDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderCopyTextureToBuffer(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* enc = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcTex = converter::toNative<core::Texture>(descriptor->source);
    auto* dstBuf = converter::toNative<core::Buffer>(descriptor->destination);

    VkOffset3D vkOrigin = converter::gfxOrigin3DToVkOffset3D(&descriptor->origin);
    VkExtent3D vkExtent = converter::gfxExtent3DToVkExtent3D(&descriptor->extent);
    VkImageLayout vkLayout = converter::gfxLayoutToVkImageLayout(descriptor->finalLayout);

    enc->copyTextureToBuffer(srcTex, vkOrigin, descriptor->mipLevel, dstBuf, descriptor->destinationOffset, vkExtent, vkLayout);

    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderCopyTextureToTexture(GfxCommandEncoder commandEncoder, const GfxCopyTextureToTextureDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderCopyTextureToTexture(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* enc = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcTex = converter::toNative<core::Texture>(descriptor->source);
    auto* dstTex = converter::toNative<core::Texture>(descriptor->destination);

    VkOffset3D vkSrcOrigin = converter::gfxOrigin3DToVkOffset3D(&descriptor->sourceOrigin);
    VkOffset3D vkDstOrigin = converter::gfxOrigin3DToVkOffset3D(&descriptor->destinationOrigin);
    VkExtent3D vkExtent = converter::gfxExtent3DToVkExtent3D(&descriptor->extent);
    VkImageLayout vkSrcLayout = converter::gfxLayoutToVkImageLayout(descriptor->sourceFinalLayout);
    VkImageLayout vkDstLayout = converter::gfxLayoutToVkImageLayout(descriptor->destinationFinalLayout);

    enc->copyTextureToTexture(srcTex, vkSrcOrigin, descriptor->sourceMipLevel, vkSrcLayout,
        dstTex, vkDstOrigin, descriptor->destinationMipLevel, vkDstLayout,
        vkExtent);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderBlitTextureToTexture(GfxCommandEncoder commandEncoder, const GfxBlitTextureToTextureDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderBlitTextureToTexture(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* enc = converter::toNative<core::CommandEncoder>(commandEncoder);
    auto* srcTex = converter::toNative<core::Texture>(descriptor->source);
    auto* dstTex = converter::toNative<core::Texture>(descriptor->destination);

    VkOffset3D vkSrcOrigin = converter::gfxOrigin3DToVkOffset3D(&descriptor->sourceOrigin);
    VkExtent3D vkSrcExtent = converter::gfxExtent3DToVkExtent3D(&descriptor->sourceExtent);
    VkOffset3D vkDstOrigin = converter::gfxOrigin3DToVkOffset3D(&descriptor->destinationOrigin);
    VkExtent3D vkDstExtent = converter::gfxExtent3DToVkExtent3D(&descriptor->destinationExtent);
    VkFilter vkFilter = converter::gfxFilterToVkFilter(descriptor->filter);
    VkImageLayout vkSrcLayout = converter::gfxLayoutToVkImageLayout(descriptor->sourceFinalLayout);
    VkImageLayout vkDstLayout = converter::gfxLayoutToVkImageLayout(descriptor->destinationFinalLayout);

    enc->blitTextureToTexture(srcTex, vkSrcOrigin, vkSrcExtent, descriptor->sourceMipLevel, vkSrcLayout,
        dstTex, vkDstOrigin, vkDstExtent, descriptor->destinationMipLevel, vkDstLayout,
        vkFilter);
    return GFX_RESULT_SUCCESS;
}

// TODO - add member function to CommandEncoder for pipeline barrier
GfxResult Backend::commandEncoderPipelineBarrier(GfxCommandEncoder commandEncoder, const GfxPipelineBarrierDescriptor* descriptor) const
{
    GfxResult validationResult = validator::validateCommandEncoderPipelineBarrier(commandEncoder, descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);

    // Convert GFX barriers to internal Vulkan barriers
    std::vector<core::MemoryBarrier> internalMemBarriers;
    internalMemBarriers.reserve(descriptor->memoryBarrierCount);
    for (uint32_t i = 0; i < descriptor->memoryBarrierCount; ++i) {
        internalMemBarriers.push_back(converter::gfxMemoryBarrierToMemoryBarrier(descriptor->memoryBarriers[i]));
    }

    std::vector<core::BufferBarrier> internalBufBarriers;
    internalBufBarriers.reserve(descriptor->bufferBarrierCount);
    for (uint32_t i = 0; i < descriptor->bufferBarrierCount; ++i) {
        internalBufBarriers.push_back(converter::gfxBufferBarrierToBufferBarrier(descriptor->bufferBarriers[i]));
    }

    std::vector<core::TextureBarrier> internalTexBarriers;
    internalTexBarriers.reserve(descriptor->textureBarrierCount);
    for (uint32_t i = 0; i < descriptor->textureBarrierCount; ++i) {
        internalTexBarriers.push_back(converter::gfxTextureBarrierToTextureBarrier(descriptor->textureBarriers[i]));
    }

    encoder->pipelineBarrier(
        internalMemBarriers.data(), static_cast<uint32_t>(internalMemBarriers.size()),
        internalBufBarriers.data(), static_cast<uint32_t>(internalBufBarriers.size()),
        internalTexBarriers.data(), static_cast<uint32_t>(internalTexBarriers.size()));
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

GfxResult Backend::commandEncoderGenerateMipmapsRange(GfxCommandEncoder commandEncoder, GfxTexture texture,
    uint32_t baseMipLevel, uint32_t levelCount) const
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

    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);
    encoder->end();
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderBegin(GfxCommandEncoder commandEncoder) const
{
    GfxResult validationResult = validator::validateCommandEncoderBegin(commandEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);
    encoder->reset();
    return GFX_RESULT_SUCCESS;
}

// RenderPassEncoder functions
GfxResult Backend::renderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetPipeline(renderPassEncoder, pipeline);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* pipe = converter::toNative<core::RenderPipeline>(pipeline);
    rpe->setPipeline(pipe);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetBindGroup(renderPassEncoder, bindGroup);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* bg = converter::toNative<core::BindGroup>(bindGroup);
    rpe->setBindGroup(index, bg, dynamicOffsets, dynamicOffsetCount);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetVertexBuffer(renderPassEncoder, buffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* buf = converter::toNative<core::Buffer>(buffer);
    rpe->setVertexBuffer(slot, buf, offset);

    (void)size;
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetIndexBuffer(renderPassEncoder, buffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* buf = converter::toNative<core::Buffer>(buffer);
    VkIndexType indexType = converter::gfxIndexFormatToVkIndexType(format);
    rpe->setIndexBuffer(buf, indexType, offset);

    (void)size;
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderSetViewport(GfxRenderPassEncoder renderPassEncoder, const GfxViewport* viewport) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetViewport(renderPassEncoder, viewport);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    core::Viewport vkViewport = converter::gfxViewportToViewport(viewport);
    rpe->setViewport(vkViewport);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderSetScissorRect(renderPassEncoder, scissor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    core::ScissorRect vkScissor = converter::gfxScissorRectToScissorRect(scissor);
    rpe->setScissorRect(vkScissor);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderDraw(renderPassEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    rpe->draw(vertexCount, instanceCount, firstVertex, firstInstance);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderDrawIndexed(renderPassEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    rpe->drawIndexed(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderDrawIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderDrawIndirect(renderPassEncoder, indirectBuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* buffer = converter::toNative<core::Buffer>(indirectBuffer);
    rpe->drawIndirect(buffer, indirectOffset);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderDrawIndexedIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const
{
    GfxResult validationResult = validator::validateRenderPassEncoderDrawIndexedIndirect(renderPassEncoder, indirectBuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* buffer = converter::toNative<core::Buffer>(indirectBuffer);
    rpe->drawIndexedIndirect(buffer, indirectOffset);
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

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    delete rpe;
    return GFX_RESULT_SUCCESS;
}

// ComputePassEncoder functions
GfxResult Backend::computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline) const
{
    GfxResult validationResult = validator::validateComputePassEncoderSetPipeline(computePassEncoder, pipeline);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* cpe = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    auto* pipe = converter::toNative<core::ComputePipeline>(pipeline);
    cpe->setPipeline(pipe);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    GfxResult validationResult = validator::validateComputePassEncoderSetBindGroup(computePassEncoder, bindGroup);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* cpe = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    auto* bg = converter::toNative<core::BindGroup>(bindGroup);
    cpe->setBindGroup(index, bg, dynamicOffsets, dynamicOffsetCount);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::computePassEncoderDispatch(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) const
{
    GfxResult validationResult = validator::validateComputePassEncoderDispatch(computePassEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* cpe = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    cpe->dispatchWorkgroups(workgroupCountX, workgroupCountY, workgroupCountZ);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::computePassEncoderDispatchIndirect(GfxComputePassEncoder computePassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const
{
    GfxResult validationResult = validator::validateComputePassEncoderDispatchIndirect(computePassEncoder, indirectBuffer);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* cpe = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    auto* buffer = converter::toNative<core::Buffer>(indirectBuffer);
    cpe->dispatchIndirect(buffer, indirectOffset);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::computePassEncoderEnd(GfxComputePassEncoder computePassEncoder) const
{
    GfxResult validationResult = validator::validateComputePassEncoderEnd(computePassEncoder);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* cpe = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    delete cpe;
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

    auto* f = converter::toNative<core::Fence>(fence);
    VkResult result = f->getStatus(isSignaled);

    // Convert VkResult to GfxResult
    if (result == VK_SUCCESS) {
        return GFX_RESULT_SUCCESS;
    } else if (result == VK_ERROR_DEVICE_LOST) {
        return GFX_RESULT_ERROR_DEVICE_LOST;
    } else {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::fenceWait(GfxFence fence, uint64_t timeoutNs) const
{
    GfxResult validationResult = validator::validateFenceWait(fence);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* f = converter::toNative<core::Fence>(fence);
    VkResult result = f->wait(timeoutNs);

    // Convert VkResult to GfxResult
    if (result == VK_SUCCESS) {
        return GFX_RESULT_SUCCESS;
    } else if (result == VK_TIMEOUT) {
        return GFX_RESULT_TIMEOUT;
    } else if (result == VK_ERROR_DEVICE_LOST) {
        return GFX_RESULT_ERROR_DEVICE_LOST;
    } else {
        return GFX_RESULT_ERROR_UNKNOWN;
    }
}

GfxResult Backend::fenceReset(GfxFence fence) const
{
    GfxResult validationResult = validator::validateFenceReset(fence);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* f = converter::toNative<core::Fence>(fence);
    f->reset();
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

    auto* s = converter::toNative<core::Semaphore>(semaphore);
    *outType = converter::vulkanSemaphoreTypeToGfxSemaphoreType(s->getType());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::semaphoreSignal(GfxSemaphore semaphore, uint64_t value) const
{
    GfxResult validationResult = validator::validateSemaphoreSignal(semaphore);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* s = converter::toNative<core::Semaphore>(semaphore);
    VkResult result = s->signal(value);
    return (result == VK_SUCCESS) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

GfxResult Backend::semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs) const
{
    GfxResult validationResult = validator::validateSemaphoreWait(semaphore);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* s = converter::toNative<core::Semaphore>(semaphore);
    VkResult result = s->wait(value, timeoutNs);
    return (result == VK_SUCCESS) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

GfxResult Backend::semaphoreGetValue(GfxSemaphore semaphore, uint64_t* outValue) const
{
    GfxResult validationResult = validator::validateSemaphoreGetValue(semaphore, outValue);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* s = converter::toNative<core::Semaphore>(semaphore);
    *outValue = s->getValue();
    return GFX_RESULT_SUCCESS;
}

GfxAccessFlags Backend::getAccessFlagsForLayout(GfxTextureLayout layout) const
{
    VkImageLayout vkLayout = converter::gfxLayoutToVkImageLayout(layout);
    VkAccessFlags vkAccessFlags = converter::getVkAccessFlagsForLayout(vkLayout);
    return converter::vkAccessFlagsToGfxAccessFlags(vkAccessFlags);
}

} // namespace gfx::backend::vulkan