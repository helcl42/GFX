#include "TextureView.h"

namespace gfx {

CTextureViewImpl::CTextureViewImpl(GfxTextureView h)
    : m_handle(h)
{
}

CTextureViewImpl::~CTextureViewImpl()
{
    if (m_handle) {
        gfxTextureViewDestroy(m_handle);
    }
}

GfxTextureView CTextureViewImpl::getHandle() const
{
    return m_handle;
}

} // namespace gfx
