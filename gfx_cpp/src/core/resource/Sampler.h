#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

namespace gfx {

class SamplerImpl : public Sampler {
public:
    explicit SamplerImpl(GfxSampler h);
    ~SamplerImpl() override;

    GfxSampler getHandle() const;

private:
    GfxSampler m_handle;
};

} // namespace gfx
