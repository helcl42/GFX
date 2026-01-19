#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

namespace gfx {

class ShaderImpl : public Shader {
public:
    explicit ShaderImpl(GfxShader h);
    ~ShaderImpl() override;

    GfxShader getHandle() const;

private:
    GfxShader m_handle;
};

} // namespace gfx
