#include "SyncComponent.h"

#include "common/Logger.h"

#include "backend/vulkan/common/Common.h"
#include "backend/vulkan/converter/Conversions.h"
#include "backend/vulkan/validator/Validations.h"

#include "backend/vulkan/core/sync/Fence.h"
#include "backend/vulkan/core/sync/Semaphore.h"
#include "backend/vulkan/core/system/Device.h"

#include <stdexcept>

namespace gfx::backend::vulkan::component {

// Fence functions
GfxResult SyncComponent::deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence) const
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

GfxResult SyncComponent::fenceDestroy(GfxFence fence) const
{
    GfxResult validationResult = validator::validateFenceDestroy(fence);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Fence>(fence);
    return GFX_RESULT_SUCCESS;
}

GfxResult SyncComponent::fenceGetStatus(GfxFence fence, bool* isSignaled) const
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

GfxResult SyncComponent::fenceWait(GfxFence fence, uint64_t timeoutNs) const
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

GfxResult SyncComponent::fenceReset(GfxFence fence) const
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
GfxResult SyncComponent::deviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore) const
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

GfxResult SyncComponent::semaphoreDestroy(GfxSemaphore semaphore) const
{
    GfxResult validationResult = validator::validateSemaphoreDestroy(semaphore);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    delete converter::toNative<core::Semaphore>(semaphore);
    return GFX_RESULT_SUCCESS;
}

GfxResult SyncComponent::semaphoreGetType(GfxSemaphore semaphore, GfxSemaphoreType* outType) const
{
    GfxResult validationResult = validator::validateSemaphoreGetType(semaphore, outType);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* s = converter::toNative<core::Semaphore>(semaphore);
    *outType = converter::vulkanSemaphoreTypeToGfxSemaphoreType(s->getType());
    return GFX_RESULT_SUCCESS;
}

GfxResult SyncComponent::semaphoreSignal(GfxSemaphore semaphore, uint64_t value) const
{
    GfxResult validationResult = validator::validateSemaphoreSignal(semaphore);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* s = converter::toNative<core::Semaphore>(semaphore);
    VkResult result = s->signal(value);
    return (result == VK_SUCCESS) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

GfxResult SyncComponent::semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs) const
{
    GfxResult validationResult = validator::validateSemaphoreWait(semaphore);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* s = converter::toNative<core::Semaphore>(semaphore);
    VkResult result = s->wait(value, timeoutNs);
    return (result == VK_SUCCESS) ? GFX_RESULT_SUCCESS : GFX_RESULT_ERROR_UNKNOWN;
}

GfxResult SyncComponent::semaphoreGetValue(GfxSemaphore semaphore, uint64_t* outValue) const
{
    GfxResult validationResult = validator::validateSemaphoreGetValue(semaphore, outValue);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* s = converter::toNative<core::Semaphore>(semaphore);
    *outValue = s->getValue();
    return GFX_RESULT_SUCCESS;
}

// Synchronization utility - converts texture layout to access flags for barriers
GfxAccessFlags SyncComponent::getAccessFlagsForLayout(GfxTextureLayout layout) const
{
    VkImageLayout vkLayout = converter::gfxLayoutToVkImageLayout(layout);
    VkAccessFlags vkAccessFlags = converter::getVkAccessFlagsForLayout(vkLayout);
    return converter::vkAccessFlagsToGfxAccessFlags(vkAccessFlags);
}

} // namespace gfx::backend::vulkan::component
