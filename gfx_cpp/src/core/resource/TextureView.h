#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>

namespace gfx {

class CTextureImpl;

class CTextureViewImpl : public TextureView {
public:
    explicit CTextureViewImpl(GfxTextureView h);
    ~CTextureViewImpl() override;

    GfxTextureView getHandle() const;

private:
    GfxTextureView m_handle;
    std::shared_ptr<Texture> m_texture;
};

} // namespace gfx
