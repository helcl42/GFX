#pragma once

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>

namespace gfx {

class CTextureImpl : public Texture {
public:
    explicit CTextureImpl(GfxTexture h);
    ~CTextureImpl() override;

    GfxTexture getHandle() const;

    TextureInfo getInfo() override;
    TextureLayout getLayout() const override;
    std::shared_ptr<TextureView> createView(const TextureViewDescriptor& descriptor = {}) override;

private:
    GfxTexture m_handle;
    GfxTextureInfo m_info;
};

} // namespace gfx
