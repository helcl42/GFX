#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

namespace gfx {

class RenderPassImpl : public RenderPass {
public:
    explicit RenderPassImpl(GfxRenderPass h);
    ~RenderPassImpl() override;

    GfxRenderPass getHandle() const;

private:
    GfxRenderPass m_handle;
};

} // namespace gfx
