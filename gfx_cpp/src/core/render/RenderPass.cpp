#include "RenderPass.h"

namespace gfx {

CRenderPassImpl::CRenderPassImpl(GfxRenderPass h)
    : m_handle(h)
{
}

CRenderPassImpl::~CRenderPassImpl()
{
    if (m_handle) {
        gfxRenderPassDestroy(m_handle);
    }
}

GfxRenderPass CRenderPassImpl::getHandle() const
{
    return m_handle;
}

} // namespace gfx
