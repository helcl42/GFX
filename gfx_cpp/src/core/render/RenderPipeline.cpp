#include "RenderPipeline.h"

namespace gfx {

RenderPipelineImpl::RenderPipelineImpl(GfxRenderPipeline h)
    : m_handle(h)
{
}

RenderPipelineImpl::~RenderPipelineImpl()
{
    if (m_handle) {
        gfxRenderPipelineDestroy(m_handle);
    }
}

GfxRenderPipeline RenderPipelineImpl::getHandle() const
{
    return m_handle;
}

} // namespace gfx
