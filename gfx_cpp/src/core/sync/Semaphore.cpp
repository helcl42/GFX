#include "Semaphore.h"

#include "../../converter/Conversions.h"

namespace gfx {

SemaphoreImpl::SemaphoreImpl(GfxSemaphore h)
    : m_handle(h)
{
}

SemaphoreImpl::~SemaphoreImpl()
{
    if (m_handle) {
        gfxSemaphoreDestroy(m_handle);
    }
}

GfxSemaphore SemaphoreImpl::getHandle() const
{
    return m_handle;
}

SemaphoreType SemaphoreImpl::getType() const
{
    GfxSemaphoreType type;
    gfxSemaphoreGetType(m_handle, &type);
    return cSemaphoreTypeToCppSemaphoreType(type);
}

uint64_t SemaphoreImpl::getValue() const
{
    uint64_t value = 0;
    gfxSemaphoreGetValue(m_handle, &value);
    return value;
}

void SemaphoreImpl::signal(uint64_t value)
{
    gfxSemaphoreSignal(m_handle, value);
}

Result SemaphoreImpl::wait(uint64_t value, uint64_t timeoutNanoseconds)
{
    return cResultToCppResult(gfxSemaphoreWait(m_handle, value, timeoutNanoseconds));
}

} // namespace gfx
