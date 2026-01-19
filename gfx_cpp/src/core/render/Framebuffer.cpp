#include "Framebuffer.h"

namespace gfx {

CFramebufferImpl::CFramebufferImpl(GfxFramebuffer h, GfxRenderPass renderPass)
    : m_handle(h)
    , m_renderPass(renderPass)
{
}

CFramebufferImpl::~CFramebufferImpl()
{
    if (m_handle) {
        gfxFramebufferDestroy(m_handle);
    }
}

GfxFramebuffer CFramebufferImpl::getHandle() const
{
    return m_handle;
}

GfxRenderPass CFramebufferImpl::getRenderPass() const
{
    return m_renderPass;
}

} // namespace gfx
