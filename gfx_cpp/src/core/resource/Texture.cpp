#include "Texture.h"

#include "TextureView.h"

#include "../../converter/Conversions.h"

#include <stdexcept>

namespace gfx {

TextureImpl::TextureImpl(GfxTexture h)
    : m_handle(h)
{
    GfxResult result = gfxTextureGetInfo(m_handle, &m_info);
    if (result != GFX_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to get texture info");
    }
}

TextureImpl::~TextureImpl()
{
    if (m_handle) {
        gfxTextureDestroy(m_handle);
    }
}

GfxTexture TextureImpl::getHandle() const
{
    return m_handle;
}

TextureInfo TextureImpl::getInfo()
{
    return cTextureInfoToCppTextureInfo(m_info);
}

void* TextureImpl::getNativeHandle() const
{
    void* handle = nullptr;
    GfxResult result = gfxTextureGetNativeHandle(m_handle, &handle);
    if (result != GFX_RESULT_SUCCESS) {
        return nullptr;
    }
    return handle;
}

TextureLayout TextureImpl::getLayout() const
{
    GfxTextureLayout layout = GFX_TEXTURE_LAYOUT_UNDEFINED;
    gfxTextureGetLayout(m_handle, &layout);
    return cLayoutToCppLayout(layout);
}

std::shared_ptr<TextureView> TextureImpl::createView(const TextureViewDescriptor& descriptor)
{
    GfxTextureViewDescriptor cDesc;
    convertTextureViewDescriptor(descriptor, cDesc);

    GfxTextureView view = nullptr;
    GfxResult result = gfxTextureCreateView(m_handle, &cDesc, &view);
    if (result != GFX_RESULT_SUCCESS || !view) {
        throw std::runtime_error("Failed to create texture view");
    }

    return std::make_shared<TextureViewImpl>(view);
}

} // namespace gfx
