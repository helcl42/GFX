#include "BindGroupLayout.h"

namespace gfx {

CBindGroupLayoutImpl::CBindGroupLayoutImpl(GfxBindGroupLayout h)
    : m_handle(h)
{
}

CBindGroupLayoutImpl::~CBindGroupLayoutImpl()
{
    if (m_handle) {
        gfxBindGroupLayoutDestroy(m_handle);
    }
}

GfxBindGroupLayout CBindGroupLayoutImpl::getHandle() const
{
    return m_handle;
}

} // namespace gfx
