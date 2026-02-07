#ifndef GFX_BACKEND_VULKAN_QUERY_COMPONENT_H
#define GFX_BACKEND_VULKAN_QUERY_COMPONENT_H

#include <gfx/gfx.h>

namespace gfx::backend::vulkan::component {

class QueryComponent {
public:
    // QuerySet functions
    GfxResult deviceCreateQuerySet(GfxDevice device, const GfxQuerySetDescriptor* descriptor, GfxQuerySet* outQuerySet) const;
    GfxResult querySetDestroy(GfxQuerySet querySet) const;
};

} // namespace gfx::backend::vulkan::component

#endif // GFX_BACKEND_VULKAN_QUERY_COMPONENT_H
