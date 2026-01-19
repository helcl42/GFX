#ifndef GFX_CPP_FRAMEBUFFER_H
#define GFX_CPP_FRAMEBUFFER_H

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

namespace gfx {

class FramebufferImpl : public Framebuffer {
public:
    explicit FramebufferImpl(GfxFramebuffer h, GfxRenderPass renderPass);
    ~FramebufferImpl() override;

    GfxFramebuffer getHandle() const;
    GfxRenderPass getRenderPass() const;

private:
    GfxFramebuffer m_handle;
    GfxRenderPass m_renderPass;
};

} // namespace gfx

#endif // GFX_CPP_FRAMEBUFFER_H
