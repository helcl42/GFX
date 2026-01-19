#include "Framebuffer.h"

namespace gfx {

FramebufferImpl::FramebufferImpl(GfxFramebuffer h, GfxRenderPass renderPass)
    : m_handle(h)
    , m_renderPass(renderPass)
{
}

FramebufferImpl::~FramebufferImpl()
{
    if (m_handle) {
        gfxFramebufferDestroy(m_handle);
    }
}

GfxFramebuffer FramebufferImpl::getHandle() const
{
    return m_handle;
}

GfxRenderPass FramebufferImpl::getRenderPass() const
{
    return m_renderPass;
}

} // namespace gfx
