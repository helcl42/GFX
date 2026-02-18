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

const char* resultToString(Result result)
{
    switch (result) {
    case Result::Success:
        return "Result::Success";
    case Result::Timeout:
        return "Result::Timeout";
    case Result::NotReady:
        return "Result::NotReady";
    case Result::ErrorInvalidArgument:
        return "Result::ErrorInvalidArgument";
    case Result::ErrorNotFound:
        return "Result::ErrorNotFound";
    case Result::ErrorOutOfMemory:
        return "Result::ErrorOutOfMemory";
    case Result::ErrorDeviceLost:
        return "Result::ErrorDeviceLost";
    case Result::ErrorSurfaceLost:
        return "Result::ErrorSurfaceLost";
    case Result::ErrorOutOfDate:
        return "Result::ErrorOutOfDate";
    case Result::ErrorBackendNotLoaded:
        return "Result::ErrorBackendNotLoaded";
    case Result::ErrorFeatureNotSupported:
        return "Result::ErrorFeatureNotSupported";
    case Result::ErrorUnknown:
        return "Result::ErrorUnknown";
    default:
        return "Result::Unknown";
    }
}

} // namespace gfx::utils
