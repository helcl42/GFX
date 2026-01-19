#ifndef GFX_CPP_SHADER_H
#define GFX_CPP_SHADER_H

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

#endif // GFX_CPP_SHADER_H
