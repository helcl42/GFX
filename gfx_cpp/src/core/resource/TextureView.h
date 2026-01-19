#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>

namespace gfx {

class TextureImpl;

class TextureViewImpl : public TextureView {
public:
    explicit TextureViewImpl(GfxTextureView h);
    ~TextureViewImpl() override;

    GfxTextureView getHandle() const;

private:
    GfxTextureView m_handle;
    std::shared_ptr<Texture> m_texture;
};

} // namespace gfx
