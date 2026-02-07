#ifndef GFX_BACKEND_WEBGPU_QUERY_COMPONENT_H
#define GFX_BACKEND_WEBGPU_QUERY_COMPONENT_H

#include <gfx/gfx.h>

namespace gfx::backend::webgpu::component {

class QueryComponent {
public:
    // QuerySet functions
    GfxResult deviceCreateQuerySet(GfxDevice device, const GfxQuerySetDescriptor* descriptor, GfxQuerySet* outQuerySet) const;
    GfxResult querySetDestroy(GfxQuerySet querySet) const;
};

} // namespace gfx::backend::webgpu::component

#endif // GFX_BACKEND_WEBGPU_QUERY_COMPONENT_H
