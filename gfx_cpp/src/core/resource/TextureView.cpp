#include "TextureView.h"

namespace gfx {

TextureViewImpl::TextureViewImpl(GfxTextureView h)
    : m_handle(h)
{
}

TextureViewImpl::~TextureViewImpl()
{
    if (m_handle) {
        gfxTextureViewDestroy(m_handle);
    }
}

GfxTextureView TextureViewImpl::getHandle() const
{
    return m_handle;
}

} // namespace gfx
