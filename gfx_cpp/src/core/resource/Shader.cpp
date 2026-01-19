#include "Shader.h"

namespace gfx {

ShaderImpl::ShaderImpl(GfxShader h)
    : m_handle(h)
{
}

ShaderImpl::~ShaderImpl()
{
    if (m_handle) {
        gfxShaderDestroy(m_handle);
    }
}

GfxShader ShaderImpl::getHandle() const
{
    return m_handle;
}

} // namespace gfx
