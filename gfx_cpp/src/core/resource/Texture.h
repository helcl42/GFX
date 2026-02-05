#ifndef GFX_CPP_TEXTURE_H
#define GFX_CPP_TEXTURE_H

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include <memory>

namespace gfx {

class TextureImpl : public Texture {
public:
    explicit TextureImpl(GfxTexture h);
    ~TextureImpl() override;

    GfxTexture getHandle() const;

    TextureInfo getInfo() const override;
    void* getNativeHandle() const override;
    TextureLayout getLayout() const override;
    std::shared_ptr<TextureView> createView(const TextureViewDescriptor& descriptor = {}) const override;

private:
    GfxTexture m_handle;
    GfxTextureInfo m_info;
};

} // namespace gfx

#endif // GFX_CPP_TEXTURE_H
