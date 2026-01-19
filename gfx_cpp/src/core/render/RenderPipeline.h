#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

namespace gfx {

class RenderPipelineImpl : public RenderPipeline {
public:
    explicit RenderPipelineImpl(GfxRenderPipeline h);
    ~RenderPipelineImpl() override;

    GfxRenderPipeline getHandle() const;

private:
    GfxRenderPipeline m_handle;
};

} // namespace gfx
