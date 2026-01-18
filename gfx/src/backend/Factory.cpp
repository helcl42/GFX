#include "Factory.h"
#include "IBackend.h"

#ifdef GFX_ENABLE_VULKAN
#include "vulkan/Backend.h"
#endif
#ifdef GFX_ENABLE_WEBGPU
#include "webgpu/Backend.h"
#endif

namespace gfx::backend {

std::unique_ptr<const IBackend> BackendFactory::create(GfxBackend backend)
{
    switch (backend) {
#ifdef GFX_ENABLE_VULKAN
    case GFX_BACKEND_VULKAN:
        return std::make_unique<vulkan::Backend>();
#endif
#ifdef GFX_ENABLE_WEBGPU
    case GFX_BACKEND_WEBGPU:
        return std::make_unique<webgpu::Backend>();
#endif
    default:
        return nullptr;
    }
}

} // namespace gfx::backend
