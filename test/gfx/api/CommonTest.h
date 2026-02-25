#ifndef GFX_TEST_COMMON_H
#define GFX_TEST_COMMON_H

#include <gfx/gfx.h>

#include <gtest/gtest.h>

#include <vector>

inline std::vector<GfxBackend> getActiveBackends()
{
    return {
#if defined(GFX_ENABLE_VULKAN)
        GFX_BACKEND_VULKAN,
#endif
#if defined(GFX_ENABLE_WEBGPU)
        GFX_BACKEND_WEBGPU,
#endif
    };
}

inline const char* convertTestParamToString(const testing::TestParamInfo<GfxBackend>& info)
{
    switch (info.param) {
    case GFX_BACKEND_VULKAN:
        return "Vulkan";
    case GFX_BACKEND_WEBGPU:
        return "WebGPU";
    default:
        return "Unknown";
    }
}

#endif // !GFX_TEST_COMMON_H
