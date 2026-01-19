#include "HandleExtractor.h"

#include "../sync/Fence.h"
#include "../sync/Semaphore.h"

namespace gfx {

// Template specializations for extractNativeHandle (must come after class definitions)
template <>
GfxSemaphore extractNativeHandle<GfxSemaphore>(std::shared_ptr<void> ptr)
{
    if (!ptr) {
        return nullptr;
    }
    auto impl = std::static_pointer_cast<SemaphoreImpl>(ptr);
    return impl->getHandle();
}

template <>
GfxFence extractNativeHandle<GfxFence>(std::shared_ptr<void> ptr)
{
    if (!ptr) {
        return nullptr;
    }
    auto impl = std::static_pointer_cast<FenceImpl>(ptr);
    return impl->getHandle();
}

} // namespace gfx
