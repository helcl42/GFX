#include "Texture.h"

#include "TextureView.h"

#include "../../converter/Conversions.h"

#include <stdexcept>

namespace gfx {

CTextureImpl::CTextureImpl(GfxTexture h)
    : m_handle(h)
{
    GfxResult result = gfxTextureGetInfo(m_handle, &m_info);
    if (result != GFX_RESULT_SUCCESS) {
        // Handle error - set default values
        m_info.type = GFX_TEXTURE_TYPE_2D;
        m_info.size = { 0, 0, 0 };
        m_info.arrayLayerCount = 1;
        m_info.mipLevelCount = 1;
        m_info.sampleCount = GFX_SAMPLE_COUNT_1;
        m_info.format = GFX_TEXTURE_FORMAT_UNDEFINED;
        m_info.usage = GFX_TEXTURE_USAGE_NONE;
    }
}

CTextureImpl::~CTextureImpl()
{
    if (m_handle) {
        gfxTextureDestroy(m_handle);
    }
}

GfxTexture CTextureImpl::getHandle() const
{
    return m_handle;
}

TextureInfo CTextureImpl::getInfo()
{
    TextureInfo info;
    info.type = cTextureTypeToCppType(m_info.type);
    info.size = Extent3D(m_info.size.width, m_info.size.height, m_info.size.depth);
    info.arrayLayerCount = m_info.arrayLayerCount;
    info.mipLevelCount = m_info.mipLevelCount;
    info.sampleCount = cSampleCountToCppCount(m_info.sampleCount);
    info.format = cFormatToCppFormat(m_info.format);
    info.usage = cTextureUsageToCppUsage(m_info.usage);
    return info;
}

TextureLayout CTextureImpl::getLayout() const
{
    GfxTextureLayout layout = GFX_TEXTURE_LAYOUT_UNDEFINED;
    gfxTextureGetLayout(m_handle, &layout);
    return cLayoutToCppLayout(layout);
}

std::shared_ptr<TextureView> CTextureImpl::createView(const TextureViewDescriptor& descriptor)
{
    GfxTextureViewDescriptor cDesc = {};
    cDesc.label = descriptor.label.c_str();
    cDesc.viewType = cppTextureViewTypeToCType(descriptor.viewType);
    cDesc.format = cppFormatToCFormat(descriptor.format);
    cDesc.baseMipLevel = descriptor.baseMipLevel;
    cDesc.mipLevelCount = descriptor.mipLevelCount;
    cDesc.baseArrayLayer = descriptor.baseArrayLayer;
    cDesc.arrayLayerCount = descriptor.arrayLayerCount;

    GfxTextureView view = nullptr;
    GfxResult result = gfxTextureCreateView(m_handle, &cDesc, &view);
    if (result != GFX_RESULT_SUCCESS || !view) {
        throw std::runtime_error("Failed to create texture view");
    }

    return std::make_shared<CTextureViewImpl>(view);
}

} // namespace gfx
