#pragma once

#include <gfx/gfx.h>

namespace gfx::util {

// Alignment utilities
uint64_t alignUp(uint64_t value, uint64_t alignment);
uint64_t alignDown(uint64_t value, uint64_t alignment);

// Format utilities
uint32_t getFormatBytesPerPixel(GfxTextureFormat format);

// Result to string conversion
const char* resultToString(GfxResult result);

} // namespace gfx::util
