#include "ComputePipeline.h"

namespace gfx {

CComputePipelineImpl::CComputePipelineImpl(GfxComputePipeline h)
    : m_handle(h)
{
}

CComputePipelineImpl::~CComputePipelineImpl()
{
    if (m_handle) {
        gfxComputePipelineDestroy(m_handle);
    }
}

GfxComputePipeline CComputePipelineImpl::getHandle() const
{
    return m_handle;
}

} // namespace gfx
