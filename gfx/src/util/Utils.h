#ifndef GFX_UTIL_UTILS_H
#define GFX_UTIL_UTILS_H

#include <gfx/gfx.h>

namespace gfx::util {

// Alignment utilities
uint64_t alignUp(uint64_t value, uint64_t alignment);
uint64_t alignDown(uint64_t value, uint64_t alignment);

// Format utilities
uint32_t getFormatBytesPerPixel(GfxFormat format);

// Result to string conversion
const char* resultToString(GfxResult result);

void* getMetalLayerFromCocoaWindow(void* cocoaWindow);

} // namespace gfx::util

#endif // GFX_UTIL_UTILS_H