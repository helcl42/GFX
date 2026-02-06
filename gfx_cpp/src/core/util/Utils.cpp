#include "Utils.h"

#include "../../converter/Conversions.h"

namespace gfx::utils {

uint64_t alignUp(uint64_t value, uint64_t alignment)
{
    return gfxAlignUp(value, alignment);
}

uint64_t alignDown(uint64_t value, uint64_t alignment)
{
    return gfxAlignDown(value, alignment);
}

uint32_t getFormatBytesPerPixel(TextureFormat format)
{
    return gfxGetFormatBytesPerPixel(cppFormatToCFormat(format));
}

} // namespace gfx::utils
