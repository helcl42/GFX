#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>
#include <vector>

namespace gfx {

class CSurfaceImpl : public Surface {
public:
    explicit CSurfaceImpl(GfxSurface h);
    ~CSurfaceImpl() override;

    GfxSurface getHandle() const;

    std::vector<TextureFormat> getSupportedFormats() const override;
    std::vector<PresentMode> getSupportedPresentModes() const override;

private:
    GfxSurface m_handle;
};

} // namespace gfx
