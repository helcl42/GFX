#ifndef GFX_CPP_RENDER_PIPELINE_H
#define GFX_CPP_RENDER_PIPELINE_H

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

#endif // GFX_CPP_RENDER_PIPELINE_H
