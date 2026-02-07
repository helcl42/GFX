#include "SystemComponent.h"

#include "common/Logger.h"

#include "../common/Common.h"
#include "../converter/Conversions.h"
#include "../validator/Validations.h"

#include "../core/system/Adapter.h"
#include "../core/system/Device.h"
#include "../core/system/Instance.h"
#include "../core/system/Queue.h"

#include <stdexcept>

namespace gfx::backend::webgpu::component {

// Instance functions
GfxResult SystemComponent::createInstance(const GfxInstanceDescriptor* descriptor, GfxInstance* outInstance) const
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

GfxResult SystemComponent::instanceDestroy(GfxInstance instance) const
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

GfxResult SystemComponent::instanceRequestAdapter(GfxInstance instance, const GfxAdapterDescriptor* descriptor, GfxAdapter* outAdapter) const
{
    GfxResult validationResult = validator::validateInstanceRequestAdapter(instance, descriptor, outAdapter);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* inst = converter::toNative<core::Instance>(instance);
    auto createInfo = converter::gfxDescriptorToWebGPUAdapterCreateInfo(descriptor);
    auto* adapter = inst->requestAdapter(createInfo);

    *outAdapter = converter::toGfx<GfxAdapter>(adapter);
    return GFX_RESULT_SUCCESS;
}

GfxResult SystemComponent::instanceEnumerateAdapters(GfxInstance instance, uint32_t* adapterCount, GfxAdapter* adapters) const
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

GfxResult SystemComponent::enumerateInstanceExtensions(uint32_t* extensionCount, const char** extensionNames) const
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
GfxResult SystemComponent::adapterCreateDevice(GfxAdapter adapter, const GfxDeviceDescriptor* descriptor, GfxDevice* outDevice) const
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

GfxResult SystemComponent::adapterGetInfo(GfxAdapter adapter, GfxAdapterInfo* outInfo) const
{
    GfxResult validationResult = validator::validateAdapterGetInfo(adapter, outInfo);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* adapterPtr = converter::toNative<core::Adapter>(adapter);
    *outInfo = converter::wgpuAdapterToGfxAdapterInfo(adapterPtr->getInfo());
    return GFX_RESULT_SUCCESS;
}

GfxResult SystemComponent::adapterGetLimits(GfxAdapter adapter, GfxDeviceLimits* outLimits) const
{
    GfxResult validationResult = validator::validateAdapterGetLimits(adapter, outLimits);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* adapterPtr = converter::toNative<core::Adapter>(adapter);
    *outLimits = converter::wgpuLimitsToGfxDeviceLimits(adapterPtr->getLimits());
    return GFX_RESULT_SUCCESS;
}

GfxResult SystemComponent::adapterEnumerateQueueFamilies(GfxAdapter adapter, uint32_t* queueFamilyCount, GfxQueueFamilyProperties* queueFamilies) const
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

GfxResult SystemComponent::adapterGetQueueFamilySurfaceSupport(GfxAdapter adapter, uint32_t queueFamilyIndex, GfxSurface surface, bool* outSupported) const
{
    GfxResult validationResult = validator::validateAdapterGetQueueFamilySurfaceSupport(adapter, surface, outSupported);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* adap = converter::toNative<core::Adapter>(adapter);
    *outSupported = adap->supportsPresentation(queueFamilyIndex);
    return GFX_RESULT_SUCCESS;
}

GfxResult SystemComponent::adapterEnumerateExtensions(GfxAdapter adapter, uint32_t* extensionCount, const char** extensionNames) const
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
GfxResult SystemComponent::deviceDestroy(GfxDevice device) const
{
    GfxResult validationResult = validator::validateDeviceDestroy(device);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Device>(device);
    return GFX_RESULT_SUCCESS;
}

GfxResult SystemComponent::deviceGetQueue(GfxDevice device, GfxQueue* outQueue) const
{
    GfxResult validationResult = validator::validateDeviceGetQueue(device, outQueue);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* dev = converter::toNative<core::Device>(device);
    *outQueue = converter::toGfx<GfxQueue>(dev->getQueue());
    return GFX_RESULT_SUCCESS;
}

GfxResult SystemComponent::deviceGetQueueByIndex(GfxDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, GfxQueue* outQueue) const
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

GfxResult SystemComponent::deviceWaitIdle(GfxDevice device) const
{
    GfxResult validationResult = validator::validateDeviceWaitIdle(device);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* devicePtr = converter::toNative<core::Device>(device);
    devicePtr->waitIdle();
    return GFX_RESULT_SUCCESS;
}

GfxResult SystemComponent::deviceGetLimits(GfxDevice device, GfxDeviceLimits* outLimits) const
{
    GfxResult validationResult = validator::validateDeviceGetLimits(device, outLimits);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* devicePtr = converter::toNative<core::Device>(device);
    *outLimits = converter::wgpuLimitsToGfxDeviceLimits(devicePtr->getLimits());
    return GFX_RESULT_SUCCESS;
}

GfxResult SystemComponent::deviceSupportsShaderFormat(GfxDevice device, GfxShaderSourceType format, bool* outSupported) const
{
    if (!device || !outSupported) {
        return GFX_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto* devicePtr = converter::toNative<core::Device>(device);
    auto internalFormat = converter::gfxShaderSourceTypeToWebGPUShaderSourceType(format);
    *outSupported = devicePtr->supportsShaderFormat(internalFormat);
    return GFX_RESULT_SUCCESS;
}

// Queue functions
GfxResult SystemComponent::queueSubmit(GfxQueue queue, const GfxSubmitDescriptor* submitInfo) const
{
    GfxResult validationResult = validator::validateQueueSubmit(queue, submitInfo);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* queuePtr = converter::toNative<core::Queue>(queue);
    auto submit = converter::gfxDescriptorToWebGPUSubmitInfo(submitInfo);

    return queuePtr->submit(submit) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

GfxResult SystemComponent::queueWriteBuffer(GfxQueue queue, GfxBuffer buffer, uint64_t offset, const void* data, uint64_t size) const
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

GfxResult SystemComponent::queueWriteTexture(GfxQueue queue, GfxTexture texture, const GfxOrigin3D* origin, uint32_t mipLevel, const void* data, uint64_t dataSize, const GfxExtent3D* extent, GfxTextureLayout finalLayout) const
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

GfxResult SystemComponent::queueWaitIdle(GfxQueue queue) const
{
    GfxResult validationResult = validator::validateQueueWaitIdle(queue);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* queuePtr = converter::toNative<core::Queue>(queue);
    return queuePtr->waitIdle() ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

} // namespace gfx::backend::webgpu::component
