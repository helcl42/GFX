#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

namespace gfx {

class CFramebufferImpl : public Framebuffer {
public:
    explicit CFramebufferImpl(GfxFramebuffer h, GfxRenderPass renderPass);
    ~CFramebufferImpl() override;

    GfxFramebuffer getHandle() const;
    GfxRenderPass getRenderPass() const;

private:
    GfxFramebuffer m_handle;
    GfxRenderPass m_renderPass;
};

} // namespace gfx
