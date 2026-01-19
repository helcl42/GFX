#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

namespace gfx {

class CRenderPipelineImpl : public RenderPipeline {
public:
    explicit CRenderPipelineImpl(GfxRenderPipeline h);
    ~CRenderPipelineImpl() override;

    GfxRenderPipeline getHandle() const;

private:
    GfxRenderPipeline m_handle;
};

} // namespace gfx
