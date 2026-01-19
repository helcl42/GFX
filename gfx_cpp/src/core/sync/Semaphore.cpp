#include "Semaphore.h"

#include "../../converter/Conversions.h"

namespace gfx {

CSemaphoreImpl::CSemaphoreImpl(GfxSemaphore h)
    : m_handle(h)
{
}

CSemaphoreImpl::~CSemaphoreImpl()
{
    if (m_handle) {
        gfxSemaphoreDestroy(m_handle);
    }
}

GfxSemaphore CSemaphoreImpl::getHandle() const
{
    return m_handle;
}

SemaphoreType CSemaphoreImpl::getType() const
{
    GfxSemaphoreType type;
    gfxSemaphoreGetType(m_handle, &type);
    return cSemaphoreTypeToCppSemaphoreType(type);
}

uint64_t CSemaphoreImpl::getValue() const
{
    uint64_t value = 0;
    gfxSemaphoreGetValue(m_handle, &value);
    return value;
}

void CSemaphoreImpl::signal(uint64_t value)
{
    gfxSemaphoreSignal(m_handle, value);
}

bool CSemaphoreImpl::wait(uint64_t value, uint64_t timeoutNanoseconds)
{
    return gfxSemaphoreWait(m_handle, value, timeoutNanoseconds) == GFX_RESULT_SUCCESS;
}

} // namespace gfx
