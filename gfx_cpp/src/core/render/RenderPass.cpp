#include "RenderPass.h"

namespace gfx {

RenderPassImpl::RenderPassImpl(GfxRenderPass h)
    : m_handle(h)
{
}

RenderPassImpl::~RenderPassImpl()
{
    if (m_handle) {
        gfxRenderPassDestroy(m_handle);
    }
}

GfxRenderPass RenderPassImpl::getHandle() const
{
    return m_handle;
}

} // namespace gfx
