#include "Shader.h"

namespace gfx {

CShaderImpl::CShaderImpl(GfxShader h)
    : m_handle(h)
{
}

CShaderImpl::~CShaderImpl()
{
    if (m_handle) {
        gfxShaderDestroy(m_handle);
    }
}

GfxShader CShaderImpl::getHandle() const
{
    return m_handle;
}

} // namespace gfx
