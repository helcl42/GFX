#include "Surface.h"

#include "../../converter/Conversions.h"

namespace gfx {

SurfaceImpl::SurfaceImpl(GfxSurface h)
    : m_handle(h)
{
}

SurfaceImpl::~SurfaceImpl()
{
    if (m_handle) {
        gfxSurfaceDestroy(m_handle);
    }
}

GfxSurface SurfaceImpl::getHandle() const
{
    return m_handle;
}

SurfaceInfo SurfaceImpl::getInfo() const
{
    GfxSurfaceInfo cInfo = {};
    gfxSurfaceGetInfo(m_handle, &cInfo);
    return cSurfaceInfoToCppSurfaceInfo(cInfo);
}

std::vector<Format> SurfaceImpl::getSupportedFormats() const
{
    // First call: query count
    uint32_t count = 0;
    gfxSurfaceEnumerateSupportedFormats(m_handle, &count, nullptr);

    // Second call: get formats
    std::vector<GfxFormat> formats(count);
    gfxSurfaceEnumerateSupportedFormats(m_handle, &count, formats.data());

    std::vector<Format> result;
    result.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        result.push_back(cFormatToCppFormat(formats[i]));
    }
    return result;
}

std::vector<PresentMode> SurfaceImpl::getSupportedPresentModes() const
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
