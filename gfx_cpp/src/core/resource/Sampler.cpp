#include "Sampler.h"

namespace gfx {

CSamplerImpl::CSamplerImpl(GfxSampler h)
    : m_handle(h)
{
}

CSamplerImpl::~CSamplerImpl()
{
    if (m_handle) {
        gfxSamplerDestroy(m_handle);
    }
}

GfxSampler CSamplerImpl::getHandle() const
{
    return m_handle;
}

} // namespace gfx
