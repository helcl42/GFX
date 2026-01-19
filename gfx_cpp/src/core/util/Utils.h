#ifndef GFX_CPP_UTILS_H
#define GFX_CPP_UTILS_H

#include <gfx_cpp/gfx.hpp>

#include <gfx/gfx.h>

#include "../../converter/Conversions.h"

namespace gfx::utils {

uint64_t alignUp(uint64_t value, uint64_t alignment);
uint64_t alignDown(uint64_t value, uint64_t alignment);
AccessFlags getAccessFlagsForLayout(TextureLayout layout);

} // namespace gfx::utils

#endif // GFX_CPP_UTILS_H
