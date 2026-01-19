#include "ComputePipeline.h"

namespace gfx {

ComputePipelineImpl::ComputePipelineImpl(GfxComputePipeline h)
    : m_handle(h)
{
}

ComputePipelineImpl::~ComputePipelineImpl()
{
    if (m_handle) {
        gfxComputePipelineDestroy(m_handle);
    }
}

GfxComputePipeline ComputePipelineImpl::getHandle() const
{
    return m_handle;
}

} // namespace gfx
