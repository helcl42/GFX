#ifndef GFX_CPP_BIND_GROUP_LAYOUT_H
#define GFX_CPP_BIND_GROUP_LAYOUT_H

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

namespace gfx {

class BindGroupLayoutImpl : public BindGroupLayout {
public:
    explicit BindGroupLayoutImpl(GfxBindGroupLayout h);
    ~BindGroupLayoutImpl() override;

    GfxBindGroupLayout getHandle() const;

private:
    GfxBindGroupLayout m_handle;
};

} // namespace gfx

#endif // GFX_CPP_BIND_GROUP_LAYOUT_H
