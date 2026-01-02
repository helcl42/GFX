#pragma once

#include "../common/WebGPUCommon.h"

#include <gfx/gfx.h>

namespace gfx::webgpu::surface {
class SurfaceFactory {
public:
    WGPUSurface createFromNativeWindow(WGPUInstance instance, const GfxPlatformWindowHandle& platformHandle);
};
} // namespace gfx::webgpu::surface