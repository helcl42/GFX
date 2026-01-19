#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>
#include <vector>

namespace gfx {

class SurfaceImpl : public Surface {
public:
    explicit SurfaceImpl(GfxSurface h);
    ~SurfaceImpl() override;

    GfxSurface getHandle() const;

    std::vector<TextureFormat> getSupportedFormats() const override;
    std::vector<PresentMode> getSupportedPresentModes() const override;

private:
    GfxSurface m_handle;
};

} // namespace gfx
