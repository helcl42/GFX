#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

namespace gfx {

class BindGroupImpl : public BindGroup {
public:
    explicit BindGroupImpl(GfxBindGroup h);
    ~BindGroupImpl() override;

    GfxBindGroup getHandle() const;

private:
    GfxBindGroup m_handle;
};

} // namespace gfx
