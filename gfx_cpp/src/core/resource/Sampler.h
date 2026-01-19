#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

namespace gfx {

class CSamplerImpl : public Sampler {
public:
    explicit CSamplerImpl(GfxSampler h);
    ~CSamplerImpl() override;

    GfxSampler getHandle() const;

private:
    GfxSampler m_handle;
};

} // namespace gfx
