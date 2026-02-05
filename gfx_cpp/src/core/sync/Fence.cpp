#include "Fence.h"

#include "../../converter/Conversions.h"

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

Result FenceImpl::wait(uint64_t timeoutNanoseconds)
{
    return cResultToCppResult(gfxFenceWait(m_handle, timeoutNanoseconds));
}

void FenceImpl::reset()
{
    gfxFenceReset(m_handle);
}

} // namespace gfx
