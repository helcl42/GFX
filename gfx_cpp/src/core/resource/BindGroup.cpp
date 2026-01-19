#include "BindGroup.h"

namespace gfx {

CBindGroupImpl::CBindGroupImpl(GfxBindGroup h)
    : m_handle(h)
{
}

CBindGroupImpl::~CBindGroupImpl()
{
    if (m_handle) {
        gfxBindGroupDestroy(m_handle);
    }
}

GfxBindGroup CBindGroupImpl::getHandle() const
{
    return m_handle;
}

} // namespace gfx
