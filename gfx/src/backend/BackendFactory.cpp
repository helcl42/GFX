#include "BackendFactory.h"
#include "IBackend.h"

#ifdef GFX_ENABLE_VULKAN
#include "vulkan/VulkanBackend.h"
#endif
#ifdef GFX_ENABLE_WEBGPU
#include "webgpu/WebGPUBackend.h"
#endif

namespace gfx {

const IBackend* BackendFactory::createBackend(GfxBackend backend)
{
    switch (backend) {
#ifdef GFX_ENABLE_VULKAN
    case GFX_BACKEND_VULKAN:
        return vulkan::VulkanBackend::create();
#endif
#ifdef GFX_ENABLE_WEBGPU
    case GFX_BACKEND_WEBGPU:
        return webgpu::WebGPUBackend::create();
#endif
    default:
        return nullptr;
    }
}

} // namespace gfx
