#ifndef GFX_WEBGPU_UTILS_H
#define GFX_WEBGPU_UTILS_H

#include "../../common/Common.h"

namespace gfx::backend::webgpu::core {

WGPUStringView toStringView(const char* str);

bool hasStencil(WGPUTextureFormat format);

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_UTILS_H