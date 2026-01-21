#ifndef GFX_WEBGPU_UTILS_H
#define GFX_WEBGPU_UTILS_H

#include "../../common/Common.h"

namespace gfx::backend::webgpu::core {

WGPUStringView toStringView(const char* str);

bool hasStencil(WGPUTextureFormat format);

// Get bytes per pixel for a texture format
uint32_t getFormatBytesPerPixel(WGPUTextureFormat format);

// Align value up to the specified alignment
uint32_t alignUp(uint32_t value, uint32_t alignment);

// Calculate bytesPerRow for texture copy operations
// Returns width * bytesPerPixel aligned to 256 bytes (WebGPU requirement)
uint32_t calculateBytesPerRow(WGPUTextureFormat format, uint32_t width);

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_UTILS_H