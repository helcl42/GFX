#include "RenderPipeline.h"

namespace gfx {

CRenderPipelineImpl::CRenderPipelineImpl(GfxRenderPipeline h)
    : m_handle(h)
{
}

CRenderPipelineImpl::~CRenderPipelineImpl()
{
    if (m_handle) {
        gfxRenderPipelineDestroy(m_handle);
    }
}

GfxRenderPipeline CRenderPipelineImpl::getHandle() const
{
    return m_handle;
}

} // namespace gfx
