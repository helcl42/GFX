#ifndef GFX_WEBGPU_SURFACE_H
#define GFX_WEBGPU_SURFACE_H

#include "../CoreTypes.h"

namespace gfx::backend::webgpu::core {

class Surface {
public:
    // Prevent copying
    Surface(const Surface&) = delete;
    Surface& operator=(const Surface&) = delete;

    Surface(WGPUInstance instance, WGPUAdapter adapter, const SurfaceCreateInfo& createInfo);
    ~Surface();

    WGPUAdapter adapter() const;
    WGPUSurface handle() const;

    const WGPUSurfaceCapabilities& getCapabilities() const;

    SurfaceInfo getInfo() const;

private:
    WGPUAdapter m_adapter = nullptr;
    WGPUSurface m_surface = nullptr;
    WGPUSurfaceCapabilities m_capabilities = WGPU_SURFACE_CAPABILITIES_INIT;
};

} // namespace gfx::backend::webgpu::core

#endif // GFX_WEBGPU_SURFACE_H