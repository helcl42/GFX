#include "Fence.h"

namespace gfx {

CFenceImpl::CFenceImpl(GfxFence h)
    : m_handle(h)
{
}

CFenceImpl::~CFenceImpl()
{
    if (m_handle) {
        gfxFenceDestroy(m_handle);
    }
}

GfxFence CFenceImpl::getHandle() const
{
    return m_handle;
}

FenceStatus CFenceImpl::getStatus() const
{
    bool signaled;
    gfxFenceGetStatus(m_handle, &signaled);
    return signaled ? FenceStatus::Signaled : FenceStatus::Unsignaled;
}

bool CFenceImpl::wait(uint64_t timeoutNanoseconds)
{
    return gfxFenceWait(m_handle, timeoutNanoseconds) == GFX_RESULT_SUCCESS;
}

void CFenceImpl::reset()
{
    gfxFenceReset(m_handle);
}

} // namespace gfx
