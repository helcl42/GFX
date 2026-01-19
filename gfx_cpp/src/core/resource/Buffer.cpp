#include "Buffer.h"

#include "../../converter/Conversions.h"

namespace gfx {

CBufferImpl::CBufferImpl(GfxBuffer h)
    : m_handle(h)
{
    GfxResult result = gfxBufferGetInfo(m_handle, &m_info);
    if (result != GFX_RESULT_SUCCESS) {
        // Handle error - set default values
        m_info.size = 0;
        m_info.usage = GFX_BUFFER_USAGE_NONE;
    }
}

CBufferImpl::~CBufferImpl()
{
    if (m_handle) {
        gfxBufferDestroy(m_handle);
    }
}

GfxBuffer CBufferImpl::getHandle() const
{
    return m_handle;
}

BufferInfo CBufferImpl::getInfo() const
{
    BufferInfo info;
    info.size = m_info.size;
    info.usage = cBufferUsageToCppUsage(m_info.usage);
    return info;
}

void* CBufferImpl::map(uint64_t offset, uint64_t size)
{
    void* mappedPointer = nullptr;
    GfxResult result = gfxBufferMap(m_handle, offset, size, &mappedPointer);
    if (result != GFX_RESULT_SUCCESS) {
        return nullptr;
    }
    return mappedPointer;
}

void CBufferImpl::unmap()
{
    gfxBufferUnmap(m_handle);
}

} // namespace gfx
