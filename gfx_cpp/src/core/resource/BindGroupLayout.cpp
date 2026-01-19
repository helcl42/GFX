#include "BindGroupLayout.h"

namespace gfx {

BindGroupLayoutImpl::BindGroupLayoutImpl(GfxBindGroupLayout h)
    : m_handle(h)
{
}

BindGroupLayoutImpl::~BindGroupLayoutImpl()
{
    if (m_handle) {
        gfxBindGroupLayoutDestroy(m_handle);
    }
}

GfxBindGroupLayout BindGroupLayoutImpl::getHandle() const
{
    return m_handle;
}

} // namespace gfx
