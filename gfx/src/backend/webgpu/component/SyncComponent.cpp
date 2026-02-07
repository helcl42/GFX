#include "SyncComponent.h"

#include "common/Logger.h"

#include "../common/Common.h"
#include "../converter/Conversions.h"
#include "../validator/Validations.h"

#include "../core/sync/Fence.h"
#include "../core/sync/Semaphore.h"
#include "../core/system/Device.h"

#include <stdexcept>
#include <vector>

namespace gfx::backend::webgpu::component {

// Fence functions
GfxResult SyncComponent::deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence) const
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

    auto* fencePtr = converter::toNative<core::Fence>(fence);
    *isSignaled = fencePtr->isSignaled();
    return GFX_RESULT_SUCCESS;
}

GfxResult SyncComponent::fenceWait(GfxFence fence, uint64_t timeoutNs) const
{
    GfxResult validationResult = validator::validateFenceWait(fence);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* fencePtr = converter::toNative<core::Fence>(fence);

    // Fence is already properly signaled by queueSubmit after GPU work completes
    // Just check the status
    bool signaled = fencePtr->wait(timeoutNs);
    return signaled ? GFX_RESULT_SUCCESS : GFX_RESULT_TIMEOUT;
}

GfxResult SyncComponent::fenceReset(GfxFence fence) const
{
    GfxResult validationResult = validator::validateFenceReset(fence);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* fencePtr = converter::toNative<core::Fence>(fence);
    fencePtr->reset();
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
        auto semaphoreType = converter::gfxSemaphoreTypeToWebGPUSemaphoreType(descriptor->type);
        auto* semaphore = new core::Semaphore(semaphoreType, descriptor->initialValue);
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

    auto type = converter::toNative<core::Semaphore>(semaphore)->getType();
    *outType = converter::webgpuSemaphoreTypeToGfxSemaphoreType(type);
    return GFX_RESULT_SUCCESS;
}

GfxResult SyncComponent::semaphoreSignal(GfxSemaphore semaphore, uint64_t value) const
{
    GfxResult validationResult = validator::validateSemaphoreSignal(semaphore);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* semaphorePtr = converter::toNative<core::Semaphore>(semaphore);
    semaphorePtr->signal(value);
    return GFX_RESULT_SUCCESS;
}

GfxResult SyncComponent::semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs) const
{
    GfxResult validationResult = validator::validateSemaphoreWait(semaphore);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    auto* semaphorePtr = converter::toNative<core::Semaphore>(semaphore);
    bool satisfied = semaphorePtr->wait(value, timeoutNs);
    return satisfied ? GFX_RESULT_SUCCESS : GFX_RESULT_TIMEOUT;
}

GfxResult SyncComponent::semaphoreGetValue(GfxSemaphore semaphore, uint64_t* outValue) const
{
    GfxResult validationResult = validator::validateSemaphoreGetValue(semaphore, outValue);
    if (validationResult != GFX_RESULT_SUCCESS) {
        return validationResult;
    }

    *outValue = converter::toNative<core::Semaphore>(semaphore)->getValue();
    return GFX_RESULT_SUCCESS;
}

GfxAccessFlags SyncComponent::getAccessFlagsForLayout(GfxTextureLayout layout) const
{
    // WebGPU doesn't use explicit access flags - synchronization is implicit
    (void)layout;
    return GFX_ACCESS_NONE;
}

} // namespace gfx::backend::webgpu::component
