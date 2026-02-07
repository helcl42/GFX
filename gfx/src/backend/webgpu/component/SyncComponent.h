#ifndef GFX_BACKEND_WEBGPU_SYNC_COMPONENT_H
#define GFX_BACKEND_WEBGPU_SYNC_COMPONENT_H

#include <gfx/gfx.h>

namespace gfx::backend::webgpu::component {

class SyncComponent {
public:
    SyncComponent() = default;
    ~SyncComponent() = default;

    // Prevent copying
    SyncComponent(const SyncComponent&) = delete;
    SyncComponent& operator=(const SyncComponent&) = delete;

    // Fence functions
    GfxResult deviceCreateFence(GfxDevice device, const GfxFenceDescriptor* descriptor, GfxFence* outFence) const;
    GfxResult fenceDestroy(GfxFence fence) const;
    GfxResult fenceGetStatus(GfxFence fence, bool* isSignaled) const;
    GfxResult fenceWait(GfxFence fence, uint64_t timeoutNs) const;
    GfxResult fenceReset(GfxFence fence) const;

    // Semaphore functions
    GfxResult deviceCreateSemaphore(GfxDevice device, const GfxSemaphoreDescriptor* descriptor, GfxSemaphore* outSemaphore) const;
    GfxResult semaphoreDestroy(GfxSemaphore semaphore) const;
    GfxResult semaphoreGetType(GfxSemaphore semaphore, GfxSemaphoreType* outType) const;
    GfxResult semaphoreSignal(GfxSemaphore semaphore, uint64_t value) const;
    GfxResult semaphoreWait(GfxSemaphore semaphore, uint64_t value, uint64_t timeoutNs) const;
    GfxResult semaphoreGetValue(GfxSemaphore semaphore, uint64_t* outValue) const;

    // Synchronization utility - converts texture layout to access flags for barriers
    GfxAccessFlags getAccessFlagsForLayout(GfxTextureLayout layout) const;
};

} // namespace gfx::backend::webgpu::component

#endif // GFX_BACKEND_WEBGPU_SYNC_COMPONENT_H
