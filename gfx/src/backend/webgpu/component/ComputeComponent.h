#ifndef GFX_BACKEND_WEBGPU_COMPUTE_COMPONENT_H
#define GFX_BACKEND_WEBGPU_COMPUTE_COMPONENT_H

#include <gfx/gfx.h>

namespace gfx::backend::webgpu::component {

class ComputeComponent {
public:
    // ComputePipeline functions
    GfxResult deviceCreateComputePipeline(GfxDevice device, const GfxComputePipelineDescriptor* descriptor, GfxComputePipeline* outPipeline) const;
    GfxResult computePipelineDestroy(GfxComputePipeline pipeline) const;
};

} // namespace gfx::backend::webgpu::component

#endif // GFX_BACKEND_WEBGPU_COMPUTE_COMPONENT_H
