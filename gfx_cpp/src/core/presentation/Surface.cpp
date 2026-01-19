#include "Surface.h"

#include "../../converter/Conversions.h"

namespace gfx {

CSurfaceImpl::CSurfaceImpl(GfxSurface h)
    : m_handle(h)
{
}

CSurfaceImpl::~CSurfaceImpl()
{
    if (m_handle) {
        gfxSurfaceDestroy(m_handle);
    }
}

GfxSurface CSurfaceImpl::getHandle() const
{
    return m_handle;
}

std::vector<TextureFormat> CSurfaceImpl::getSupportedFormats() const
{
    // First call: query count
    uint32_t count = 0;
    gfxSurfaceEnumerateSupportedFormats(m_handle, &count, nullptr);

    // Second call: get formats
    std::vector<GfxTextureFormat> formats(count);
    gfxSurfaceEnumerateSupportedFormats(m_handle, &count, formats.data());

    std::vector<TextureFormat> result;
    result.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        result.push_back(cFormatToCppFormat(formats[i]));
    }
    return result;
}

std::vector<PresentMode> CSurfaceImpl::getSupportedPresentModes() const
{
    // First call: query count
    uint32_t count = 0;
    gfxSurfaceEnumerateSupportedPresentModes(m_handle, &count, nullptr);

    // Second call: get present modes
    std::vector<GfxPresentMode> modes(count);
    gfxSurfaceEnumerateSupportedPresentModes(m_handle, &count, modes.data());

    std::vector<PresentMode> result;
    result.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        result.push_back(cPresentModeToCppPresentMode(modes[i]));
    }
    return result;
}

} // namespace gfx
