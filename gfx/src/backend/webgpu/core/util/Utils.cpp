#include "Utils.h"

namespace gfx::backend::webgpu::core {

WGPUStringView toStringView(const char* str)
{
    if (!str) {
        return WGPUStringView{ nullptr, WGPU_STRLEN };
    }
    return WGPUStringView{ str, WGPU_STRLEN };
}

bool hasStencil(WGPUTextureFormat format)
{
    return format == WGPUTextureFormat_Stencil8 || format == WGPUTextureFormat_Depth24PlusStencil8 || format == WGPUTextureFormat_Depth32FloatStencil8;
}

} // namespace gfx::backend::webgpu::core