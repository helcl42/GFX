#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

namespace gfx {

class CBindGroupLayoutImpl : public BindGroupLayout {
public:
    explicit CBindGroupLayoutImpl(GfxBindGroupLayout h);
    ~CBindGroupLayoutImpl() override;

    GfxBindGroupLayout getHandle() const;

private:
    GfxBindGroupLayout m_handle;
};

} // namespace gfx
