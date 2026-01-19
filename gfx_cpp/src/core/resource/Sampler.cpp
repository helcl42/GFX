#include "Sampler.h"

namespace gfx {

SamplerImpl::SamplerImpl(GfxSampler h)
    : m_handle(h)
{
}

SamplerImpl::~SamplerImpl()
{
    if (m_handle) {
        gfxSamplerDestroy(m_handle);
    }
}

GfxSampler SamplerImpl::getHandle() const
{
    return m_handle;
}

} // namespace gfx
