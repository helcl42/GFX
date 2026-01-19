#include "Fence.h"

namespace gfx {

FenceImpl::FenceImpl(GfxFence h)
    : m_handle(h)
{
}

FenceImpl::~FenceImpl()
{
    if (m_handle) {
        gfxFenceDestroy(m_handle);
    }
}

GfxFence FenceImpl::getHandle() const
{
    return m_handle;
}

FenceStatus FenceImpl::getStatus() const
{
    bool signaled;
    gfxFenceGetStatus(m_handle, &signaled);
    return signaled ? FenceStatus::Signaled : FenceStatus::Unsignaled;
}

bool FenceImpl::wait(uint64_t timeoutNanoseconds)
{
    return gfxFenceWait(m_handle, timeoutNanoseconds) == GFX_RESULT_SUCCESS;
}

void FenceImpl::reset()
{
    gfxFenceReset(m_handle);
}

} // namespace gfx
