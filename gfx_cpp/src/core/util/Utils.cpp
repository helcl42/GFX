#include "Utils.h"

namespace gfx::utils {

uint64_t alignUp(uint64_t value, uint64_t alignment)
{
    return gfxAlignUp(value, alignment);
}

uint64_t alignDown(uint64_t value, uint64_t alignment)
{
    return gfxAlignDown(value, alignment);
}

AccessFlags getAccessFlagsForLayout(TextureLayout layout)
{
    GfxAccessFlags cFlags = gfxGetAccessFlagsForLayout(cppLayoutToCLayout(layout));
    return cAccessFlagsToCppAccessFlags(cFlags);
}

uint32_t getFormatBytesPerPixel(TextureFormat format)
{
    return gfxGetFormatBytesPerPixel(cppFormatToCFormat(format));
}

} // namespace gfx::utils
