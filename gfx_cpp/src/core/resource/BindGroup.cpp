#include "BindGroup.h"

namespace gfx {

BindGroupImpl::BindGroupImpl(GfxBindGroup h)
    : m_handle(h)
{
}

BindGroupImpl::~BindGroupImpl()
{
    if (m_handle) {
        gfxBindGroupDestroy(m_handle);
    }
}

GfxBindGroup BindGroupImpl::getHandle() const
{
    return m_handle;
}

} // namespace gfx
