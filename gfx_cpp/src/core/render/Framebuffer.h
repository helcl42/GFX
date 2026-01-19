#pragma once

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
