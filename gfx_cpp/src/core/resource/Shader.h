#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

namespace gfx {

class CShaderImpl : public Shader {
public:
    explicit CShaderImpl(GfxShader h);
    ~CShaderImpl() override;

    GfxShader getHandle() const;

private:
    GfxShader m_handle;
};

} // namespace gfx
