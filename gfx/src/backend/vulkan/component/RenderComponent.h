#ifndef GFX_BACKEND_VULKAN_RENDER_COMPONENT_H
#define GFX_BACKEND_VULKAN_RENDER_COMPONENT_H

#include <gfx/gfx.h>

namespace gfx::backend::vulkan::component {

class RenderComponent {
public:
    // RenderPass functions
    GfxResult deviceCreateRenderPass(GfxDevice device, const GfxRenderPassDescriptor* descriptor, GfxRenderPass* outRenderPass) const;
    GfxResult renderPassDestroy(GfxRenderPass renderPass) const;

    // Framebuffer functions
    GfxResult deviceCreateFramebuffer(GfxDevice device, const GfxFramebufferDescriptor* descriptor, GfxFramebuffer* outFramebuffer) const;
    GfxResult framebufferDestroy(GfxFramebuffer framebuffer) const;

    // RenderPipeline functions
    GfxResult deviceCreateRenderPipeline(GfxDevice device, const GfxRenderPipelineDescriptor* descriptor, GfxRenderPipeline* outPipeline) const;
    GfxResult renderPipelineDestroy(GfxRenderPipeline pipeline) const;
};

} // namespace gfx::backend::vulkan::component

#endif // GFX_BACKEND_VULKAN_RENDER_COMPONENT_H
