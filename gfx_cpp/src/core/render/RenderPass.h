#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

namespace gfx {

class CRenderPassImpl : public RenderPass {
public:
    explicit CRenderPassImpl(GfxRenderPass h);
    ~CRenderPassImpl() override;

    GfxRenderPass getHandle() const;

private:
    GfxRenderPass m_handle;
};

} // namespace gfx
