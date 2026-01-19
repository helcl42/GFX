#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

namespace gfx {

class CBindGroupImpl : public BindGroup {
public:
    explicit CBindGroupImpl(GfxBindGroup h);
    ~CBindGroupImpl() override;

    GfxBindGroup getHandle() const;

private:
    GfxBindGroup m_handle;
};

} // namespace gfx
