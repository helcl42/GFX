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
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace gfx::backend::vulkan {

// Instance functions
GfxResult Backend::createInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance) const
{
    if (!outInstance) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents (NULL is valid for optional descriptor)
    GfxResult validationResult = validator::validateInstanceDescriptor(descriptor);
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
    delete converter::toNative<core::Instance>(instance);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter) const
{
    if (!instance || !outAdapter) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents (NULL is valid for optional descriptor)
    GfxResult validationResult = validator::validateAdapterDescriptor(descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* inst = converter::toNative<core::Instance>(instance);
        auto createInfo = converter::gfxDescriptorToAdapterCreateInfo(descriptor);
        auto* adapter = new core::Adapter(inst, createInfo);
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
    delete converter::toNative<core::Adapter>(adapter);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice) const
{
    if (!adapter || !outDevice) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents (NULL is valid for optional descriptor)
    GfxResult validationResult = validator::validateDeviceDescriptor(descriptor);
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
    if (!adapter || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* adap = converter::toNative<core::Adapter>(adapter);
    *outInfo = converter::vkPropertiesToGfxAdapterInfo(adap->getProperties());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::adapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits) const
{
    if (!adapter || !outLimits) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* adap = converter::toNative<core::Adapter>(adapter);
    *outLimits = converter::vkPropertiesToGfxDeviceLimits(adap->getProperties());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::adapterEnumerateQueueFamilies(GfxAdapter adapter, uint32_t* queueFamilyCount, GfxQueueFamilyProperties* queueFamilies) const
{
    if (!adapter || !queueFamilyCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
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
    if (!adapter || !surface || !outSupported) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* adap = converter::toNative<core::Adapter>(adapter);
    auto* surf = converter::toNative<core::Surface>(surface);

    *outSupported = adap->supportsPresentation(queueFamilyIndex, surf->handle());
    return GFX_RESULT_SUCCESS;
}

// Device functions
GfxResult Backend::deviceDestroy(GfxDevice device) const
{
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

GfxResult Backend::deviceGetQueueByIndex(GfxDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, GfxQueue* outQueue) const
{
    if (!device || !outQueue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
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

GfxResult Backend::deviceCreateSwapchain(GfxDevice device, GfxSurface surface, const GfxSwapchainDescriptor* descriptor, GfxSwapchain* outSwapchain) const
{
    if (!device || !surface || !descriptor || !outSwapchain) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validateSwapchainDescriptor(descriptor);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    try {
        auto* dev = converter::toNative<core::Device>(device);
        auto* surf = converter::toNative<core::Surface>(surface);
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
    if (!device || !descriptor || !outBuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validateBufferDescriptor(descriptor);
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
    if (!device || !outBuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validateBufferImportDescriptor(descriptor);
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
    if (!device || !descriptor || !outTexture) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validateTextureDescriptor(descriptor);
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
    if (!device || !outTexture) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validateTextureImportDescriptor(descriptor);
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
    if (!device || !descriptor || !outSampler) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validateSamplerDescriptor(descriptor);
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
    if (!device || !descriptor || !outShader) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validateShaderDescriptor(descriptor);
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
    if (!device || !descriptor || !outLayout) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validateBindGroupLayoutDescriptor(descriptor);
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
    if (!device || !descriptor || !outBindGroup) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validateBindGroupDescriptor(descriptor);
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
    if (!device || !descriptor || !outPipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validateRenderPipelineDescriptor(descriptor);
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
    if (!device || !descriptor || !outPipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validateComputePipelineDescriptor(descriptor);
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
    if (!device || !descriptor || !outEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
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
    if (!device || !descriptor || !outRenderPass) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validateRenderPassDescriptor(descriptor);
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
    if (!device || !descriptor || !outFramebuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validateFramebufferDescriptor(descriptor);
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
    if (!device || !outFence) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents (NULL is valid for optional descriptor)
    GfxResult validationResult = validator::validateFenceDescriptor(descriptor);
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
    if (!device || !outSemaphore) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents (NULL is valid for optional descriptor)
    GfxResult validationResult = validator::validateSemaphoreDescriptor(descriptor);
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

GfxResult Backend::deviceWaitIdle(GfxDevice device) const
{
    if (!device) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* dev = converter::toNative<core::Device>(device);
    dev->waitIdle();
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits) const
{
    if (!device || !outLimits) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* dev = converter::toNative<core::Device>(device);
    *outLimits = converter::vkPropertiesToGfxDeviceLimits(dev->getProperties());
    return GFX_RESULT_SUCCESS;
}

// Surface functions
GfxResult Backend::surfaceDestroy(GfxSurface surface) const
{
    delete converter::toNative<core::Surface>(surface);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::surfaceEnumerateSupportedFormats(GfxSurface surface, uint32_t* formatCount, GfxTextureFormat* formats) const
{
    if (!surface || !formatCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
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
    if (!surface || !presentModeCount) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
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
    delete converter::toNative<core::Swapchain>(swapchain);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::swapchainGetInfo(GfxSwapchain swapchain, GfxSwapchainInfo* outInfo) const
{
    if (!swapchain || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* sc = converter::toNative<core::Swapchain>(swapchain);
    *outInfo = converter::vkSwapchainInfoToGfxSwapchainInfo(sc->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::swapchainAcquireNextImage(GfxSwapchain swapchain, uint64_t timeoutNs, GfxSemaphore imageAvailableSemaphore, GfxFence fence, uint32_t* outImageIndex) const
{
    if (!swapchain || !outImageIndex) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
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
    if (!swapchain || !outView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
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
    if (!swapchain || !outView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* sc = converter::toNative<core::Swapchain>(swapchain);
    *outView = converter::toGfx<GfxTextureView>(sc->getCurrentTextureView());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::swapchainPresent(GfxSwapchain swapchain, const GfxPresentInfo* presentInfo) const
{
    if (!swapchain) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* sc = converter::toNative<core::Swapchain>(swapchain);

    std::vector<VkSemaphore> waitSemaphores;
    if (presentInfo && presentInfo->waitSemaphoreCount > 0) {
        waitSemaphores.reserve(presentInfo->waitSemaphoreCount);
        for (uint32_t i = 0; i < presentInfo->waitSemaphoreCount; ++i) {
            auto* sem = converter::toNative<core::Semaphore>(presentInfo->waitSemaphores[i]);
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
    delete converter::toNative<core::Buffer>(buffer);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::bufferGetInfo(GfxBuffer buffer, GfxBufferInfo* outInfo) const
{
    if (!buffer || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* buf = converter::toNative<core::Buffer>(buffer);
    *outInfo = converter::vkBufferToGfxBufferInfo(buf->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::bufferGetNativeHandle(GfxBuffer buffer, void** outHandle) const
{
    if (!buffer || !outHandle) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* buf = converter::toNative<core::Buffer>(buffer);
    *outHandle = reinterpret_cast<void*>(buf->handle());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::bufferMap(GfxBuffer buffer, uint64_t offset, uint64_t size, void** outMappedPointer) const
{
    if (!buffer || !outMappedPointer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    (void)offset;
    (void)size;

    auto* buf = converter::toNative<core::Buffer>(buffer);
    *outMappedPointer = buf->map();
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::bufferUnmap(GfxBuffer buffer) const
{
    if (!buffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* buf = converter::toNative<core::Buffer>(buffer);
    buf->unmap();
    return GFX_RESULT_SUCCESS;
}

// Texture functions
GfxResult Backend::textureDestroy(GfxTexture texture) const
{
    delete converter::toNative<core::Texture>(texture);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::textureGetInfo(GfxTexture texture, GfxTextureInfo* outInfo) const
{
    if (!texture || !outInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* tex = converter::toNative<core::Texture>(texture);
    *outInfo = converter::vkTextureInfoToGfxTextureInfo(tex->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::textureGetNativeHandle(GfxTexture texture, void** outHandle) const
{
    if (!texture || !outHandle) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* tex = converter::toNative<core::Texture>(texture);
    *outHandle = reinterpret_cast<void*>(tex->handle());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::textureGetLayout(GfxTexture texture, GfxTextureLayout* outLayout) const
{
    if (!texture || !outLayout) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* tex = converter::toNative<core::Texture>(texture);
    *outLayout = converter::vkImageLayoutToGfxLayout(tex->getLayout());
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::textureCreateView(GfxTexture texture, const GfxTextureViewDescriptor* descriptor, GfxTextureView* outView) const
{
    if (!texture || !outView) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents (NULL is valid for optional descriptor)
    GfxResult validationResult = validator::validateTextureViewDescriptor(descriptor);
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
    delete converter::toNative<core::TextureView>(textureView);
    return GFX_RESULT_SUCCESS;
}

// Sampler functions
GfxResult Backend::samplerDestroy(GfxSampler sampler) const
{
    delete converter::toNative<core::Sampler>(sampler);
    return GFX_RESULT_SUCCESS;
}

// Shader functions
GfxResult Backend::shaderDestroy(GfxShader shader) const
{
    delete converter::toNative<core::Shader>(shader);
    return GFX_RESULT_SUCCESS;
}

// BindGroupLayout functions
GfxResult Backend::bindGroupLayoutDestroy(GfxBindGroupLayout bindGroupLayout) const
{
    delete converter::toNative<core::BindGroupLayout>(bindGroupLayout);
    return GFX_RESULT_SUCCESS;
}

// BindGroup functions
GfxResult Backend::bindGroupDestroy(GfxBindGroup bindGroup) const
{
    delete converter::toNative<core::BindGroup>(bindGroup);
    return GFX_RESULT_SUCCESS;
}

// RenderPipeline functions
GfxResult Backend::renderPipelineDestroy(GfxRenderPipeline renderPipeline) const
{
    delete converter::toNative<core::RenderPipeline>(renderPipeline);
    return GFX_RESULT_SUCCESS;
}

// ComputePipeline functions
GfxResult Backend::computePipelineDestroy(GfxComputePipeline computePipeline) const
{
    delete converter::toNative<core::ComputePipeline>(computePipeline);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassDestroy(GfxRenderPass renderPass) const
{
    delete converter::toNative<core::RenderPass>(renderPass);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::framebufferDestroy(GfxFramebuffer framebuffer) const
{
    delete converter::toNative<core::Framebuffer>(framebuffer);
    return GFX_RESULT_SUCCESS;
}

// Queue functions
GfxResult Backend::queueSubmit(GfxQueue queue, const GfxSubmitDescriptor* submitInfo) const
{
    if (!queue || !submitInfo) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* q = converter::toNative<core::Queue>(queue);
    auto internalSubmitInfo = converter::gfxDescriptorToSubmitInfo(submitInfo);
    VkResult result = q->submit(internalSubmitInfo);
    return (result == VK_SUCCESS) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

GfxResult Backend::queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size) const
{
    if (!queue || !buffer || !data) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* q = converter::toNative<core::Queue>(queue);
    auto* buf = converter::toNative<core::Buffer>(buffer);
    q->writeBuffer(buf, offset, data, size);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel, const void* data, uint64_t dataSize, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
{
    if (!queue || !texture || !data || !extent || dataSize == 0) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
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
    if (!queue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* q = converter::toNative<core::Queue>(queue);
    q->waitIdle();
    return GFX_RESULT_SUCCESS;
}

// CommandEncoder functions
GfxResult Backend::commandEncoderDestroy(GfxCommandEncoder commandEncoder) const
{
    delete converter::toNative<core::CommandEncoder>(commandEncoder);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderBeginRenderPass(GfxCommandEncoder commandEncoder, const GfxRenderPassBeginDescriptor* beginDescriptor, GfxRenderPassEncoder* outRenderPass) const
{
    if (!commandEncoder || !outRenderPass) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validateRenderPassBeginDescriptor(beginDescriptor);
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
    if (!commandEncoder || !outComputePass) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validateComputePassBeginDescriptor(beginDescriptor);
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
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validateCopyBufferToBufferDescriptor(descriptor);
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

    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validateCopyBufferToTextureDescriptor(descriptor);
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
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validateCopyTextureToBufferDescriptor(descriptor);
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
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validateCopyTextureToTextureDescriptor(descriptor);
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
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validateBlitTextureToTextureDescriptor(descriptor);
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
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Validate descriptor contents
    GfxResult validationResult = validator::validatePipelineBarrierDescriptor(descriptor);
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
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);
    encoder->end();
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::commandEncoderBegin(GfxCommandEncoder commandEncoder) const
{
    if (!commandEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* encoder = converter::toNative<core::CommandEncoder>(commandEncoder);
    encoder->reset();
    return GFX_RESULT_SUCCESS;
}

// RenderPassEncoder functions
GfxResult Backend::renderPassEncoderSetPipeline(GfxRenderPassEncoder renderPassEncoder, GfxRenderPipeline pipeline) const
{
    if (!renderPassEncoder || !pipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* pipe = converter::toNative<core::RenderPipeline>(pipeline);
    rpe->setPipeline(pipe);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderSetBindGroup(GfxRenderPassEncoder renderPassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    if (!renderPassEncoder || !bindGroup) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* bg = converter::toNative<core::BindGroup>(bindGroup);
    rpe->setBindGroup(index, bg, dynamicOffsets, dynamicOffsetCount);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderSetVertexBuffer(GfxRenderPassEncoder renderPassEncoder, uint32_t slot, GfxBuffer buffer, uint64_t offset, uint64_t size) const
{
    if (!renderPassEncoder || !buffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* buf = converter::toNative<core::Buffer>(buffer);
    rpe->setVertexBuffer(slot, buf, offset);

    (void)size;
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderSetIndexBuffer(GfxRenderPassEncoder renderPassEncoder, GfxBuffer buffer, GfxIndexFormat format, uint64_t offset, uint64_t size) const
{
    if (!renderPassEncoder || !buffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
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
    if (!renderPassEncoder || !viewport) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    core::Viewport vkViewport = converter::gfxViewportToViewport(viewport);
    rpe->setViewport(vkViewport);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderSetScissorRect(GfxRenderPassEncoder renderPassEncoder, const GfxScissorRect* scissor) const
{
    if (!renderPassEncoder || !scissor) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    core::ScissorRect vkScissor = converter::gfxScissorRectToScissorRect(scissor);
    rpe->setScissorRect(vkScissor);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderDraw(GfxRenderPassEncoder renderPassEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
{
    if (!renderPassEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    rpe->draw(vertexCount, instanceCount, firstVertex, firstInstance);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderDrawIndexed(GfxRenderPassEncoder renderPassEncoder, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance) const
{
    if (!renderPassEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    rpe->drawIndexed(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderDrawIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const
{
    if (!renderPassEncoder || !indirectBuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* buffer = converter::toNative<core::Buffer>(indirectBuffer);
    rpe->drawIndirect(buffer, indirectOffset);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderDrawIndexedIndirect(GfxRenderPassEncoder renderPassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const
{
    if (!renderPassEncoder || !indirectBuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    auto* buffer = converter::toNative<core::Buffer>(indirectBuffer);
    rpe->drawIndexedIndirect(buffer, indirectOffset);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::renderPassEncoderEnd(GfxRenderPassEncoder renderPassEncoder) const
{
    if (!renderPassEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* rpe = converter::toNative<core::RenderPassEncoder>(renderPassEncoder);
    delete rpe;
    return GFX_RESULT_SUCCESS;
}

// ComputePassEncoder functions
GfxResult Backend::computePassEncoderSetPipeline(GfxComputePassEncoder computePassEncoder, GfxComputePipeline pipeline) const
{
    if (!computePassEncoder || !pipeline) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* cpe = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    auto* pipe = converter::toNative<core::ComputePipeline>(pipeline);
    cpe->setPipeline(pipe);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::computePassEncoderSetBindGroup(GfxComputePassEncoder computePassEncoder, uint32_t index, GfxBindGroup bindGroup, const uint32_t* dynamicOffsets, uint32_t dynamicOffsetCount) const
{
    if (!computePassEncoder || !bindGroup) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* cpe = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    auto* bg = converter::toNative<core::BindGroup>(bindGroup);
    cpe->setBindGroup(index, bg, dynamicOffsets, dynamicOffsetCount);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::computePassEncoderDispatch(GfxComputePassEncoder computePassEncoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) const
{
    if (!computePassEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* cpe = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    cpe->dispatchWorkgroups(workgroupCountX, workgroupCountY, workgroupCountZ);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::computePassEncoderDispatchIndirect(GfxComputePassEncoder computePassEncoder, GfxBuffer indirectBuffer, uint64_t indirectOffset) const
{
    if (!computePassEncoder || !indirectBuffer) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* cpe = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    auto* buffer = converter::toNative<core::Buffer>(indirectBuffer);
    cpe->dispatchIndirect(buffer, indirectOffset);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::computePassEncoderEnd(GfxComputePassEncoder computePassEncoder) const
{
    if (!computePassEncoder) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* cpe = converter::toNative<core::ComputePassEncoder>(computePassEncoder);
    delete cpe;
    return GFX_RESULT_SUCCESS;
}

// Fence functions
GfxResult Backend::fenceDestroy(GfxFence fence) const
{
    delete converter::toNative<core::Fence>(fence);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::fenceGetStatus(GfxFence fence, bool* isSignaled) const
{
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
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
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
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
    if (!fence) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* f = converter::toNative<core::Fence>(fence);
    f->reset();
    return GFX_RESULT_SUCCESS;
}

// Semaphore functions
GfxResult Backend::semaphoreDestroy(GfxSemaphore semaphore) const
{
    delete converter::toNative<core::Semaphore>(semaphore);
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::semaphoreGetType(GfxSemaphore semaphore, GfxSemaphoreType* outType) const
{
    if (!semaphore || !outType) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* s = converter::toNative<core::Semaphore>(semaphore);
    *outType = s->getType() == core::SemaphoreType::Timeline ? GFX_SEMAPHORE_TYPE_TIMELINE : GFX_SEMAPHORE_TYPE_BINARY;
    return GFX_RESULT_SUCCESS;
}

GfxResult Backend::semaphoreSignal(GfxSemaphore semaphore, uint64_t value) const
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* s = converter::toNative<core::Semaphore>(semaphore);
    VkResult result = s->signal(value);
    return (result == VK_SUCCESS) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

GfxResult Backend::semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs) const
{
    if (!semaphore) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* s = converter::toNative<core::Semaphore>(semaphore);
    VkResult result = s->wait(value, timeoutNs);
    return (result == VK_SUCCESS) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

GfxResult Backend::semaphoreGetValue(GfxSemaphore semaphore, uint64_t* outValue) const
{
    if (!semaphore || !outValue) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
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